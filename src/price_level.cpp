#include "asterbook/book.hpp"
namespace asterbook {
Result PriceLevelBook::apply(const Command &c) {
    usable();
    try {
        auto err = validate(c, symbol_);
        if (err != Error::None)
            return reject(c, err);
        auto found = index_.find(c.id.value);
        if (c.action == Action::Cancel) {
            if (found == index_.end())
                return reject(c, Error::Unknown);
            auto loc = found->second;
            const auto &o = *loc.order;
            auto result =
                Result{event(Kind::Cancelled, symbol_, c.id, o.acceptance, o.remaining, o.price)};
            auto q = o.remaining.value;
            loc.level->second.total -= q;
            active_qty_ -= q;
            loc.level->second.orders.erase(loc.order);
            if (loc.level->second.orders.empty())
                (loc.side == Side::Buy ? bids_ : asks_).erase(loc.level);
            index_.erase(found);
            return result;
        }
        if (found != index_.end())
            return reject(c, Error::Duplicate);
        if (accepted_ == limits_.counter || c.qty.value > limits_.quantity - active_qty_ ||
            index_.size() > limits_.counter - trades_)
            return reject(c, Error::Capacity);
        auto a = ++accepted_;
        auto remaining = c.qty.value;
        Result result{event(Kind::Accepted, symbol_, c.id, a, c.qty, c.price)};
        auto &opposite = c.side == Side::Buy ? asks_ : bids_;
        while (remaining && !opposite.empty()) {
            auto level = c.side == Side::Buy ? opposite.begin() : std::prev(opposite.end());
            if (c.side == Side::Buy ? level->first > c.price : level->first < c.price)
                break;
            auto &resting = level->second.orders.front();
            auto q = std::min(remaining, resting.remaining.value);
            auto e = event(Kind::Trade, symbol_, c.id, a, {q}, level->first);
            e.maker = resting.id;
            e.maker_acceptance = resting.acceptance;
            e.trade = ++trades_;
            result.push_back(e);
            remaining -= q;
            resting.remaining.value -= q;
            level->second.total -= q;
            active_qty_ -= q;
            if (!resting.remaining.value) {
                index_.erase(resting.id.value);
                level->second.orders.pop_front();
            }
            if (level->second.orders.empty())
                opposite.erase(level);
        }
        if (remaining) {
            if (c.tif == Tif::GTC) {
                auto &own = c.side == Side::Buy ? bids_ : asks_;
                auto level = own.try_emplace(c.price).first;
                level->second.orders.push_back({c.id, c.side, c.price, c.qty, {remaining}, a});
                index_.emplace(c.id.value,
                               Locator{c.side, level, std::prev(level->second.orders.end())});
                level->second.total += remaining;
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
std::vector<Order> PriceLevelBook::snapshot() const {
    usable();
    std::vector<Order> out;
    for (auto it = bids_.rbegin(); it != bids_.rend(); ++it)
        out.insert(out.end(), it->second.orders.begin(), it->second.orders.end());
    for (const auto &[price, level] : asks_) {
        (void)price;
        out.insert(out.end(), level.orders.begin(), level.orders.end());
    }
    return out;
}
bool PriceLevelBook::invariant() const {
    usable();
    std::uint64_t total = 0;
    std::size_t count = 0;
    for (auto side : {Side::Buy, Side::Sell}) {
        const auto &levels = side == Side::Buy ? bids_ : asks_;
        for (auto l = levels.begin(); l != levels.end(); ++l) {
            std::uint64_t sum = 0, last = 0;
            if (l->second.orders.empty())
                return false;
            for (auto o = l->second.orders.begin(); o != l->second.orders.end(); ++o) {
                auto i = index_.find(o->id.value);
                if (i == index_.end() || i->second.side != side || i->second.level != l ||
                    i->second.order != o || o->side != side || o->price != l->first ||
                    !o->remaining.value || o->remaining > o->initial || o->acceptance <= last)
                    return false;
                last = o->acceptance;
                sum += o->remaining.value;
                ++count;
            }
            if (sum != l->second.total)
                return false;
            total += sum;
        }
    }
    return count == index_.size() && total == active_qty_ &&
           (bids_.empty() || asks_.empty() || bids_.rbegin()->first < asks_.begin()->first);
}
} // namespace asterbook
