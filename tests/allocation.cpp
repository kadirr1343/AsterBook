#include "asterbook/book.hpp"
#include <cstdlib>
#include <iostream>
#include <new>
static long budget = -1;
void *operator new(std::size_t n) {
    if (budget == 0)
        throw std::bad_alloc();
    if (budget > 0)
        --budget;
    if (auto p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
using namespace asterbook;
template <class B> int check() {
    int failures = 0;
    for (long n = 0; n < 35; ++n) {
        B b({1});
        b.apply({1, Action::Add, {1}, {1}, Side::Sell, {100}, {5}, Tif::GTC});
        budget = n;
        bool failed = false;
        try {
            b.apply({2, Action::Add, {1}, {2}, Side::Buy, {100}, {8}, Tif::GTC});
        } catch (const std::bad_alloc &) {
            failed = true;
        }
        budget = -1;
        if (failed) {
            ++failures;
            if (!b.poisoned())
                throw std::runtime_error("not poisoned");
            bool refused = false;
            try {
                b.apply({3, Action::Cancel, {1}, {1}});
            } catch (const std::runtime_error &) {
                refused = true;
            }
            if (!refused)
                throw std::runtime_error("poison reuse");
        } else if (!b.invariant())
            throw std::runtime_error("success invariant");
    }
    if (!failures)
        throw std::runtime_error("no injected failures");
    return failures;
}
int main() {
    try {
        auto n = check<ReferenceBook>() + check<PriceLevelBook>();
        std::cout << n << " injected failures refused reuse\n";
        return 0;
    } catch (const std::exception &e) {
        budget = -1;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
