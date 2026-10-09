#include "asterbook/io.hpp"
#include <chrono>
#include <cmath>
#include <sstream>
namespace asterbook {
using Clock = std::chrono::steady_clock;
static double ns(Clock::duration duration) {
    return std::chrono::duration<double, std::nano>(duration).count();
}
void bench(std::span<const Command> commands, const std::string &engine,
           const std::filesystem::path &output) {
    if (commands.empty())
        throw std::runtime_error("benchmark needs commands");
    std::vector<std::string> engines = engine == "both"
                                           ? std::vector<std::string>{"reference", "price-level"}
                                           : std::vector<std::string>{engine};
    std::ostringstream csv;
    csv << "engine,repeat,commands,throughput_ops_s,p50_ns,p95_ns,p99_ns,max_ns,timer_ns,accepted,"
           "rejected,cancelled,trades,expired,active,levels,checksum\n";
    for (const auto &name : engines) {
        replay(commands, name); // untimed warmup, discarded state
        for (int repeat = 0; repeat < 5; ++repeat) {
            std::map<SymbolId, std::unique_ptr<Book>> books;
            for (auto c : commands)
                if (!books.contains(c.symbol))
                    books.emplace(c.symbol, make_book(name, c.symbol));
            std::vector<Book *> route;
            route.reserve(commands.size());
            for (auto c : commands)
                route.push_back(books.at(c.symbol).get());
            std::vector<Result> results(commands.size());
            auto start = Clock::now();
            for (std::size_t i = 0; i < commands.size(); ++i)
                results[i] = route[i]->apply(commands[i]);
            auto elapsed = ns(Clock::now() - start);
            std::uint64_t accepted = 0, rejected = 0, cancelled = 0, trades = 0, expired = 0,
                          checksum = 0;
            for (const auto &result : results)
                for (const auto &e : result) {
                    accepted += e.kind == Kind::Accepted;
                    rejected += e.kind == Kind::Rejected;
                    cancelled += e.kind == Kind::Cancelled;
                    trades += e.kind == Kind::Trade;
                    expired += e.kind == Kind::Expired;
                    checksum = checksum * 1099511628211ULL + e.qty.value + e.id.value + e.trade +
                               static_cast<std::uint64_t>(e.kind);
                }
            std::size_t active = 0, levels = 0;
            for (auto &[symbol, b] : books) {
                (void)symbol;
                auto snapshot = b->snapshot();
                active += snapshot.size();
                levels += b->level_count();
                for (auto o : snapshot)
                    checksum =
                        checksum * 1099511628211ULL + o.id.value + o.remaining.value + o.acceptance;
            }
            // Separate latency pass starts from empty books; no throughput timer per command.
            books.clear();
            for (auto c : commands)
                if (!books.contains(c.symbol))
                    books.emplace(c.symbol, make_book(name, c.symbol));
            for (std::size_t i = 0; i < commands.size(); ++i)
                route[i] = books.at(commands[i].symbol).get();
            results.clear();
            results.resize(commands.size());
            std::vector<double> latency;
            latency.reserve(commands.size());
            for (std::size_t i = 0; i < commands.size(); ++i) {
                auto t = Clock::now();
                results[i] = route[i]->apply(commands[i]);
                latency.push_back(ns(Clock::now() - t));
            }
            std::uint64_t consumed = 0;
            for (auto &result : results)
                for (auto &e : result)
                    consumed += e.qty.value;
            checksum ^= consumed; // Observable latency-pass consumption, outside timing.
            std::sort(latency.begin(), latency.end());
            auto quantile = [&](double q) {
                return latency[static_cast<std::size_t>(
                                   std::ceil(q * static_cast<double>(latency.size()))) -
                               1];
            };
            double overhead = 0;
            for (int i = 0; i < 1000; ++i) {
                auto t = Clock::now();
                overhead += ns(Clock::now() - t);
            }
            overhead /= 1000;
            csv << name << ',' << repeat << ',' << commands.size() << ','
                << static_cast<double>(commands.size()) * 1e9 / elapsed << ',' << quantile(.50)
                << ',' << quantile(.95) << ',' << quantile(.99) << ',' << latency.back() << ','
                << overhead << ',' << accepted << ',' << rejected << ',' << cancelled << ','
                << trades << ',' << expired << ',' << active << ',' << levels << ',' << checksum
                << '\n';
        }
    }
    write_text(output, csv.str());
}
} // namespace asterbook
