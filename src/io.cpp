#include "asterbook/io.hpp"
#include <charconv>
#include <fstream>
#include <random>
#include <sstream>
namespace asterbook {
std::uint64_t number(const std::string &s) {
    std::uint64_t value = 0;
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (s.empty() || ec != std::errc{} || end != s.data() + s.size())
        throw std::runtime_error("invalid unsigned integer: " + s);
    return value;
}
std::expected<Command, std::string> parse_line(const std::string &line) {
    try {
        std::vector<std::string> f;
        std::size_t pos = 0;
        while (true) {
            auto end = line.find(',', pos);
            f.push_back(line.substr(pos, end == std::string::npos ? end : end - pos));
            if (end == std::string::npos)
                break;
            pos = end + 1;
        }
        if (f.size() != 8)
            return std::unexpected("expected 8 fields");
        Command c;
        c.seq = number(f[0]);
        if (!c.seq)
            return std::unexpected("sequence must be positive");
        c.symbol = {number(f[2])};
        c.id = {number(f[3])};
        if (f[1] == "CANCEL") {
            c.action = Action::Cancel;
            for (std::size_t i = 4; i < 8; ++i)
                if (!f[i].empty())
                    return std::unexpected("cancel fields must be empty");
        } else if (f[1] == "ADD") {
            c.action = Action::Add;
            if (f[4] == "BUY")
                c.side = Side::Buy;
            else if (f[4] == "SELL")
                c.side = Side::Sell;
            else
                return std::unexpected("unknown side");
            c.price = {number(f[5])};
            c.qty = {number(f[6])};
            if (f[7] == "GTC")
                c.tif = Tif::GTC;
            else if (f[7] == "IOC")
                c.tif = Tif::IOC;
            else
                return std::unexpected("unknown tif");
        } else
            return std::unexpected("unknown action");
        return c;
    } catch (const std::exception &e) {
        return std::unexpected(e.what());
    }
}
std::vector<Command> read_commands(const std::filesystem::path &path) {
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot read input");
    std::string line;
    std::size_t row = 0;
    std::uint64_t last = 0;
    std::vector<Command> out;
    while (std::getline(in, line)) {
        ++row;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (row == 1) {
            if (line != header)
                throw std::runtime_error("schema v1 header required");
            continue;
        }
        auto c = parse_line(line);
        if (!c)
            throw std::runtime_error("line " + std::to_string(row) + ": " + c.error());
        if (c->seq <= last)
            throw std::runtime_error("line " + std::to_string(row) + ": non-increasing sequence");
        last = c->seq;
        out.push_back(*c);
    }
    if (!row || in.bad())
        throw std::runtime_error("empty or failed input");
    return out;
}
void write_text(const std::filesystem::path &path, const std::string &text) {
    if (!path.parent_path().empty())
        std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    if (!out)
        throw std::runtime_error("cannot open output");
    out << text;
    out.close();
    if (!out)
        throw std::runtime_error("output write failed");
}
void write_commands(const std::filesystem::path &path, std::span<const Command> commands) {
    std::ostringstream out;
    out << header << '\n';
    for (const auto &c : commands) {
        out << c.seq << ',' << (c.action == Action::Add ? "ADD" : "CANCEL") << ',' << c.symbol.value
            << ',' << c.id.value << ',';
        if (c.action == Action::Add)
            out << (c.side == Side::Buy ? "BUY" : "SELL") << ',' << c.price.value << ','
                << c.qty.value << ',' << (c.tif == Tif::GTC ? "GTC" : "IOC");
        else
            out << ",,,";
        out << '\n';
    }
    write_text(path, out.str());
}
std::vector<Command> generate(std::uint64_t seed, std::size_t count, const std::string &scenario,
                              std::size_t levels) {
    if (!levels || levels > 1000000)
        throw std::runtime_error("levels must be 1..1000000");
    if (scenario != "mixed" && scenario != "add-only" && scenario != "cancel-heavy" &&
        scenario != "sweep-heavy")
        throw std::runtime_error("unknown scenario");
    std::mt19937_64 rng(seed);
    ReferenceBook state({1});
    std::vector<Command> out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        auto roll = rng() % 100;
        auto snapshot = state.snapshot();
        bool cancel = (scenario == "cancel-heavy" ? roll < 60 : scenario == "mixed" && roll < 25) &&
                      !snapshot.empty();
        Command c;
        c.seq = i + 1;
        c.symbol = {1};
        c.id = {i + 1};
        if (cancel) {
            c.action = Action::Cancel;
            c.id = snapshot[static_cast<std::size_t>(rng() % snapshot.size())].id;
        } else {
            c.action = Action::Add;
            c.side = rng() % 2 ? Side::Buy : Side::Sell;
            c.price = {c.side == Side::Buy ? 1000000 - rng() % levels : 1000001 + rng() % levels};
            c.qty = {1 + rng() % 100};
            c.tif = Tif::GTC;
            if (scenario != "add-only" && (scenario == "sweep-heavy" ? roll < 70 : roll > 70)) {
                c.price = {c.side == Side::Buy ? 2000002ULL : 1ULL};
                c.qty = {1 + rng() % 500};
                c.tif = Tif::IOC;
            }
        }
        state.apply(c);
        out.push_back(c);
    }
    return out;
}
std::string events_csv(std::span<const Event> events) {
    std::ostringstream out;
    out << "kind,symbol,id,maker,acceptance,maker_acceptance,trade,price_ticks,quantity,error\n";
    for (const auto &e : events)
        out << static_cast<int>(e.kind) << ',' << e.symbol.value << ',' << e.id.value << ','
            << e.maker.value << ',' << e.acceptance << ',' << e.maker_acceptance << ',' << e.trade
            << ',' << e.price.value << ',' << e.qty.value << ',' << static_cast<int>(e.error)
            << '\n';
    return out.str();
}
std::string snapshot_csv(SymbolId symbol, std::span<const Order> orders) {
    std::ostringstream out;
    out << "symbol,side,price_ticks,id,initial,remaining,acceptance\n";
    for (const auto &o : orders)
        out << symbol.value << ',' << (o.side == Side::Buy ? "BUY" : "SELL") << ',' << o.price.value
            << ',' << o.id.value << ',' << o.initial.value << ',' << o.remaining.value << ','
            << o.acceptance << '\n';
    return out.str();
}
std::unique_ptr<Book> make_book(const std::string &engine, SymbolId symbol) {
    if (engine == "reference")
        return std::make_unique<ReferenceBook>(symbol);
    if (engine == "price-level")
        return std::make_unique<PriceLevelBook>(symbol);
    throw std::runtime_error("unknown engine");
}
Replay replay(std::span<const Command> commands, const std::string &engine, bool check) {
    std::map<SymbolId, std::unique_ptr<Book>> books;
    Replay out;
    for (const auto &c : commands) {
        auto &b = books[c.symbol];
        if (!b)
            b = make_book(engine, c.symbol);
        auto events = b->apply(c);
        out.events.insert(out.events.end(), events.begin(), events.end());
        if (check && !b->invariant())
            throw std::runtime_error("invariant failed");
    }
    out.snapshot = "symbol,side,price_ticks,id,initial,remaining,acceptance\n";
    for (const auto &[symbol, b] : books) {
        auto s = snapshot_csv(symbol, b->snapshot());
        out.snapshot += s.substr(s.find('\n') + 1);
    }
    return out;
}
} // namespace asterbook
