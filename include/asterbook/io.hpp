#pragma once
#include "book.hpp"
#include <filesystem>
#include <memory>
#include <span>
#include <string>
namespace asterbook {
inline constexpr const char *header = "seq,action,symbol,order_id,side,price_ticks,quantity,tif";
std::expected<Command, std::string> parse_line(const std::string &);
std::vector<Command> read_commands(const std::filesystem::path &);
void write_commands(const std::filesystem::path &, std::span<const Command>);
std::vector<Command> generate(std::uint64_t seed, std::size_t count, const std::string &scenario,
                              std::size_t levels = 32);
std::string events_csv(std::span<const Event>);
std::string snapshot_csv(SymbolId, std::span<const Order>);
void write_text(const std::filesystem::path &, const std::string &);
std::unique_ptr<Book> make_book(const std::string &, SymbolId);
struct Replay {
    Result events;
    std::string snapshot;
    auto operator<=>(const Replay &) const = default;
};
Replay replay(std::span<const Command>, const std::string &, bool check = false);
std::uint64_t number(const std::string &);
void bench(std::span<const Command>, const std::string &, const std::filesystem::path &);
} // namespace asterbook
