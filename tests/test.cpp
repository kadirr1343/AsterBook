#include "asterbook/io.hpp"
#include <cstdlib>
#include <iostream>
#include <random>
using namespace asterbook;
static std::size_t checks = 0;
void require(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
Command add(std::uint64_t id, Side side, std::uint64_t price, std::uint64_t qty,
            Tif tif = Tif::GTC) {
    return {1, Action::Add, {1}, {id}, side, {price}, {qty}, tif};
}
Command cancel(std::uint64_t id) { return {1, Action::Cancel, {1}, {id}}; }
template <class B> void golden() {
    B b({1});
    require(b.invariant() && b.snapshot().empty(), "empty");
    b.apply(add(1, Side::Sell, 101, 4));
    b.apply(add(2, Side::Sell, 100, 5));
    b.apply(add(3, Side::Sell, 100, 7));
    auto copy = b.snapshot();
    require(copy.size() == 3 && copy[0].id.value == 2, "ask priority");
    auto before = b.snapshot();
    auto ac = b.accepted(), tc = b.trades();
    auto dup = b.apply(add(2, Side::Buy, 200, 99));
    require(dup[0].error == Error::Duplicate && b.snapshot() == before && b.accepted() == ac &&
                b.trades() == tc,
            "duplicate before matching");
    auto r = b.apply(add(4, Side::Buy, 101, 14));
    require(r.size() == 4 && r[1].maker.value == 2 && r[1].qty.value == 5 &&
                r[2].maker.value == 3 && r[2].qty.value == 7 && r[3].maker.value == 1 &&
                r[3].qty.value == 2,
            "golden sweep FIFO");
    require(b.snapshot()[0].remaining.value == 2 && copy[0].remaining.value == 5,
            "partial and snapshot ownership");
    auto removed = b.apply(cancel(1));
    require(removed[0].qty.value == 2 && removed[0].kind == Kind::Cancelled, "partial cancel");
    require(b.apply(cancel(1))[0].error == Error::Unknown, "repeat cancel");
    require(b.apply(cancel(2))[0].error == Error::Unknown, "filled removed");
    b.apply(add(1, Side::Buy, 98, 2));
    require(b.snapshot()[0].acceptance == 5, "ID reuse generation");
    auto ioc = b.apply(add(7, Side::Sell, 99, 8, Tif::IOC));
    require(ioc.back().kind == Kind::Expired && ioc.back().qty.value == 8, "IOC no fill");
    auto partial = b.apply(add(8, Side::Sell, 98, 5, Tif::IOC));
    require(partial.size() == 3 && partial[1].qty.value == 2 && partial.back().qty.value == 3 &&
                b.snapshot().empty(),
            "IOC partial");
    b.apply(add(9, Side::Buy, 100, 3));
    auto full = b.apply(add(10, Side::Sell, 100, 3, Tif::IOC));
    require(full.size() == 2 && b.snapshot().empty(), "IOC full");
    for (auto c :
         {add(11, Side::Buy, 0, 1), add(11, Side::Buy, 1, 0), add(11, static_cast<Side>(9), 1, 1),
          add(11, Side::Buy, 1, 1, static_cast<Tif>(9))}) {
        ac = b.accepted();
        tc = b.trades();
        require(b.apply(c)[0].error == Error::Invalid && b.accepted() == ac && b.trades() == tc &&
                    b.snapshot().empty(),
                "invalid atomic reject");
    }
    auto wrong = add(11, Side::Buy, 1, 1);
    wrong.symbol = {2};
    require(b.apply(wrong)[0].error == Error::WrongSymbol, "wrong symbol");
    wrong = add(11, Side::Buy, 1, 1);
    wrong.action = static_cast<Action>(9);
    require(b.apply(wrong)[0].error == Error::Invalid, "wrong action");
    B q({1});
    for (std::uint64_t id = 1; id <= 5; ++id)
        q.apply(add(id, Side::Buy, 100, 1));
    q.apply(cancel(1));
    q.apply(cancel(3));
    q.apply(cancel(5));
    require(q.snapshot().size() == 2 && q.snapshot()[0].id.value == 2 &&
                q.snapshot()[1].id.value == 4 && q.invariant(),
            "first middle last cancel");
    q.apply(add(6, Side::Buy, 101, 1));
    require(q.top(Side::Buy)->price.value == 101, "bid priority");
    q.apply(cancel(6));
    q.apply(cancel(2));
    q.apply(cancel(4));
    require(q.level_count() == 0 && q.invariant(), "last level removed");
    B bids({1});
    bids.apply(add(1, Side::Buy, 100, 4));
    bids.apply(add(2, Side::Buy, 101, 2));
    bids.apply(add(3, Side::Buy, 101, 3));
    auto sell = bids.apply(add(4, Side::Sell, 100, 6));
    require(sell.size() == 4 && sell[1].maker.value == 2 && sell[1].price.value == 101 &&
                sell[2].maker.value == 3 && sell[2].price.value == 101 &&
                sell[3].maker.value == 1 && sell[3].qty.value == 1,
            "sell bid sweep FIFO");
    require(bids.depth(1).size() == 1 && bids.depth(1)[0].qty.value == 3 && bids.depth(0).empty(),
            "bounded depth aggregation");
    auto bid_state = bids.snapshot();
    auto bid_ac = bids.accepted();
    auto bid_tc = bids.trades();
    require(bids.apply(cancel(999))[0].error == Error::Unknown && bids.snapshot() == bid_state &&
                bids.accepted() == bid_ac && bids.trades() == bid_tc,
            "unknown cancel atomicity");
    B limited({1}, {2, 10});
    limited.apply(add(1, Side::Buy, 1, 9));
    require(limited.apply(add(2, Side::Buy, 1, 2))[0].error == Error::Capacity &&
                limited.accepted() == 1,
            "quantity bound");
    limited.apply(cancel(1));
    limited.apply(add(2, Side::Buy, 1, 1));
    require(limited.apply(add(3, Side::Buy, 1, 1))[0].error == Error::Capacity &&
                limited.accepted() == 2,
            "acceptance bound");
    B trade_limit({1}, {3, 100});
    trade_limit.apply(add(1, Side::Sell, 10, 1));
    trade_limit.apply(add(2, Side::Sell, 10, 1));
    auto trades = trade_limit.apply(add(3, Side::Buy, 10, 2));
    require(trades.size() == 3 && trade_limit.trades() == 2, "bounded trade sequence");
    B maxq({1});
    maxq.apply(add(1, Side::Buy, 1, std::numeric_limits<std::uint64_t>::max()));
    require(maxq.depth(1)[0].qty.value == std::numeric_limits<std::uint64_t>::max() &&
                maxq.apply(add(2, Side::Buy, 1, 1))[0].error == Error::Capacity && maxq.invariant(),
            "uint64 quantity boundary");
}
void differential(std::uint64_t seed, std::size_t count) {
    ReferenceBook r({1});
    PriceLevelBook p({1});
    std::mt19937_64 rng(seed);
    std::vector<Command> prefix;
    std::uint64_t accepted = 0, filled = 0, dropped = 0;
    for (std::size_t i = 0; i < count; ++i) {
        auto c = add(rng() % 80, rng() % 2 ? Side::Buy : Side::Sell, 90 + rng() % 21,
                     1 + rng() % 25, rng() % 4 ? Tif::GTC : Tif::IOC);
        if (rng() % 4 == 0)
            c = cancel(rng() % 80);
        if (rng() % 23 == 0)
            c.qty = {0};
        c.seq = i + 1;
        prefix.push_back(c);
        auto a = r.apply(c), b = p.apply(c);
        try {
            require(a == b, "events differential");
            require(r.snapshot() == p.snapshot(), "snapshot differential");
            require(r.depth(3) == p.depth(3) && r.top(Side::Buy) == p.top(Side::Buy),
                    "depth differential");
            require(r.accepted() == p.accepted() && r.trades() == p.trades(),
                    "counter differential");
            require(r.invariant() && p.invariant(), "invariant");
            for (auto e : a) {
                if (e.kind == Kind::Accepted)
                    accepted += e.qty.value;
                if (e.kind == Kind::Trade)
                    filled += e.qty.value;
                if (e.kind == Kind::Cancelled || e.kind == Kind::Expired)
                    dropped += e.qty.value;
            }
            std::uint64_t active = 0;
            for (auto o : r.snapshot())
                active += o.remaining.value;
            require(accepted == active + 2 * filled + dropped, "quantity conservation");
        } catch (...) {
            write_commands("out/failing-prefix.csv", prefix);
            std::cerr << "seed=" << seed << " event=" << i << " prefix=out/failing-prefix.csv\n";
            throw;
        }
    }
}
void parser() {
    for (auto s : {"1,ADD,1,1,BUY,-1,1,GTC", "1,ADD,1,1,BUY,1,18446744073709551616,GTC",
                   "1,ADD,1,1,BUY,1,1,FOK", "1,ADD,1,1,BOGUS,1,1,GTC", "1,CANCEL,1,1,BUY,,,",
                   "1,ADD", "0,CANCEL,1,1,,,,"})
        require(!parse_line(s), "malformed parser");
    require(parse_line("1,CANCEL,1,1,,,,").has_value(), "cancel grammar");
    write_text("out/parser.csv",
               std::string(header) + "\r\n1,CANCEL,1,1,,,,\r\n1,CANCEL,1,1,,,,\r\n");
    bool threw = false;
    try {
        read_commands("out/parser.csv");
    } catch (const std::exception &) {
        threw = true;
    }
    require(threw, "sequence reject");
    write_text("out/parser.csv", std::string(header) + "\r\n1,CANCEL,1,1,,,,\r\n");
    require(read_commands("out/parser.csv").size() == 1, "CRLF");
}
int main(int argc, char **argv) {
    try {
        golden<ReferenceBook>();
        golden<PriceLevelBook>();
        parser();
        bool extended = argc == 2 && std::string(argv[1]) == "--extended";
        for (std::uint64_t seed = 1; seed <= (extended ? 100 : 8); ++seed)
            differential(seed, extended ? 10000 : 1500);
        auto c = generate(42, 1000, "mixed");
        auto a = replay(c, "reference", true), b = replay(c, "price-level", true);
        require(a == b && b == replay(c, "price-level", true), "determinism");
        c = {add(1, Side::Buy, 1, 2), add(1, Side::Buy, 1, 2)};
        c[1].symbol = {2};
        auto multi = replay(c, "price-level", true);
        require(multi.events.size() == 4, "symbol isolation");
        std::cout << checks << " active checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
