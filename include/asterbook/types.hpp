#pragma once
#include <algorithm>
#include <compare>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>
namespace asterbook {
template <class Tag> struct Value {
    std::uint64_t value{};
    auto operator<=>(const Value &) const = default;
};
using OrderId = Value<struct IdTag>;
using PriceTicks = Value<struct PriceTag>;
using Quantity = Value<struct QtyTag>;
using SymbolId = Value<struct SymbolTag>;
enum class Side { Buy, Sell };
enum class Tif { GTC, IOC };
enum class Action { Add, Cancel };
enum class Error { None, Invalid, WrongSymbol, Duplicate, Unknown, Capacity };
enum class Kind { Accepted, Trade, Rested, Cancelled, Expired, Rejected };
struct Command {
    std::uint64_t seq{};
    Action action{};
    SymbolId symbol{};
    OrderId id{};
    Side side{};
    PriceTicks price{};
    Quantity qty{};
    Tif tif{};
};
struct Order {
    OrderId id;
    Side side;
    PriceTicks price;
    Quantity initial, remaining;
    std::uint64_t acceptance;
    auto operator<=>(const Order &) const = default;
};
struct Event {
    Kind kind{};
    SymbolId symbol{};
    OrderId id{}, maker{};
    std::uint64_t acceptance{}, maker_acceptance{}, trade{};
    PriceTicks price{};
    Quantity qty{};
    Error error{};
    auto operator<=>(const Event &) const = default;
};
using Result = std::vector<Event>;
struct Level {
    Side side;
    PriceTicks price;
    Quantity qty;
    std::size_t orders;
    auto operator<=>(const Level &) const = default;
};
struct Limits {
    std::uint64_t counter = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t quantity = std::numeric_limits<std::uint64_t>::max();
};
inline bool priority(const Order &a, const Order &b) {
    if (a.side != b.side)
        return a.side < b.side;
    if (a.price != b.price)
        return a.side == Side::Buy ? a.price > b.price : a.price < b.price;
    return a.acceptance < b.acceptance;
}
inline std::vector<Level> depth_of(const std::vector<Order> &orders, std::size_t n) {
    std::vector<Level> out;
    std::size_t buys = 0, sells = 0;
    for (const auto &o : orders) {
        if (!out.empty() && out.back().side == o.side && out.back().price == o.price) {
            out.back().qty.value += o.remaining.value;
            ++out.back().orders;
        } else {
            auto &count = o.side == Side::Buy ? buys : sells;
            if (count++ < n)
                out.push_back({o.side, o.price, o.remaining, 1});
        }
    }
    return out;
}
inline Error validate(const Command &c, SymbolId symbol) {
    if (c.symbol != symbol)
        return Error::WrongSymbol;
    if (c.action != Action::Add && c.action != Action::Cancel)
        return Error::Invalid;
    if (c.action == Action::Cancel)
        return Error::None;
    if ((c.side != Side::Buy && c.side != Side::Sell) || (c.tif != Tif::GTC && c.tif != Tif::IOC) ||
        !c.price.value || !c.qty.value)
        return Error::Invalid;
    return Error::None;
}
inline Event event(Kind kind, SymbolId symbol, OrderId id, std::uint64_t acceptance = 0,
                   Quantity qty = {}, PriceTicks price = {}) {
    return {kind, symbol, id, {}, acceptance, 0, 0, price, qty, Error::None};
}
} // namespace asterbook
