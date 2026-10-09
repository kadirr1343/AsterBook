#include "asterbook/book.hpp"
namespace asterbook {
Result ReferenceBook::apply(const Command &c) {
    usable();
    try {
        auto err = validate(c, symbol_);
        if (err != Error::None)
            return reject(c, err);
        auto found = std::find_if(orders_.begin(), orders_.end(),
                                  [&](const auto &o) { return o.id == c.id; });
        if (c.action == Action::Cancel) {
            if (found == orders_.end())
                return reject(c, Error::Unknown);
            auto result = Result{event(Kind::Cancelled, symbol_, c.id, found->acceptance,
                                       found->remaining, found->price)};
            active_qty_ -= found->remaining.value;
            orders_.erase(found);
            return result;
        }
        if (found != orders_.end())
            return reject(c, Error::Duplicate);
        // Conservative admission limits guarantee all subsequent arithmetic fits.
        if (accepted_ == limits_.counter || c.qty.value > limits_.quantity - active_qty_ ||
            orders_.size() > limits_.counter - trades_)
            return reject(c, Error::Capacity);
        auto a = ++accepted_;
        auto remaining = c.qty.value;
        Result result{event(Kind::Accepted, symbol_, c.id, a, c.qty, c.price)};
        for (auto it = orders_.begin(); it != orders_.end() && remaining;) {
            bool crosses = it->side != c.side &&
                           (c.side == Side::Buy ? c.price >= it->price : c.price <= it->price);
            if (!crosses) {
                ++it;
                continue;
            }
            auto q = std::min(remaining, it->remaining.value);
            auto e = event(Kind::Trade, symbol_, c.id, a, {q}, it->price);
            e.maker = it->id;
            e.maker_acceptance = it->acceptance;
            e.trade = ++trades_;
            result.push_back(e);
            remaining -= q;
            it->remaining.value -= q;
            active_qty_ -= q;
            if (!it->remaining.value)
                it = orders_.erase(it);
            else
                ++it;
        }
        if (remaining) {
            if (c.tif == Tif::GTC) {
                orders_.push_back({c.id, c.side, c.price, c.qty, {remaining}, a});
                std::sort(orders_.begin(), orders_.end(), priority);
                active_qty_ += remaining;
                result.push_back(event(Kind::Rested, symbol_, c.id, a, {remaining}, c.price));
            } else
                result.push_back(event(Kind::Expired, symbol_, c.id, a, {remaining}, c.price));
        }
        return result;
    } catch (...) {
        poisoned_ = true;
        throw;
    }
}
bool ReferenceBook::invariant() const {
    usable();
    std::uint64_t total = 0;
    for (std::size_t i = 0; i < orders_.size(); ++i) {
        const auto &o = orders_[i];
        if (!o.remaining.value || o.remaining > o.initial)
            return false;
        total += o.remaining.value;
        if (i && priority(o, orders_[i - 1]))
            return false;
        for (std::size_t j = 0; j < i; ++j)
            if (o.id == orders_[j].id)
                return false;
    }
    auto b = top(Side::Buy), a = top(Side::Sell);
    return total == active_qty_ && (!b || !a || b->price < a->price);
}
} // namespace asterbook
