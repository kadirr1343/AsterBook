#pragma once
#include "types.hpp"
#include <list>
#include <map>
#include <unordered_map>
namespace asterbook {
class Book {
  protected:
    SymbolId symbol_;
    Limits limits_;
    std::uint64_t accepted_ = 0, trades_ = 0, active_qty_ = 0;
    bool poisoned_ = false;
    explicit Book(SymbolId symbol, Limits limits) : symbol_(symbol), limits_(limits) {}
    void usable() const {
        if (poisoned_)
            throw std::runtime_error("poisoned engine");
    }
    Result reject(const Command &c, Error e) const {
        auto x = event(Kind::Rejected, symbol_, c.id);
        x.error = e;
        return {x};
    }

  public:
    virtual ~Book() = default;
    Book(const Book &) = delete;
    Book &operator=(const Book &) = delete;
    Book(Book &&) = delete;
    Book &operator=(Book &&) = delete;
    virtual Result apply(const Command &) = 0;
    virtual std::vector<Order> snapshot() const = 0;
    virtual bool invariant() const = 0;
    std::vector<Level> depth(std::size_t n) const { return depth_of(snapshot(), n); }
    std::optional<Level> top(Side side) const {
        for (auto l : depth(1))
            if (l.side == side)
                return l;
        return {};
    }
    std::size_t active_count() const { return snapshot().size(); }
    std::size_t level_count() const {
        return depth(std::numeric_limits<std::size_t>::max()).size();
    }
    std::uint64_t accepted() const { return accepted_; }
    std::uint64_t trades() const { return trades_; }
    bool poisoned() const { return poisoned_; }
};
class ReferenceBook final : public Book {
    std::vector<Order> orders_;

  public:
    explicit ReferenceBook(SymbolId s, Limits l = {}) : Book(s, l) {}
    Result apply(const Command &) override;
    std::vector<Order> snapshot() const override {
        usable();
        return orders_;
    }
    bool invariant() const override;
};
class PriceLevelBook final : public Book {
    struct PriceLevel {
        std::list<Order> orders;
        std::uint64_t total = 0;
    };
    using Levels = std::map<PriceTicks, PriceLevel>;
    struct Locator {
        Side side;
        Levels::iterator level;
        std::list<Order>::iterator order;
    };
    Levels bids_, asks_;
    std::unordered_map<std::uint64_t, Locator> index_;

  public:
    explicit PriceLevelBook(SymbolId s, Limits l = {}) : Book(s, l) {}
    Result apply(const Command &) override;
    std::vector<Order> snapshot() const override;
    bool invariant() const override;
};
} // namespace asterbook
