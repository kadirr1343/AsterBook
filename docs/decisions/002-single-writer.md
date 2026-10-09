# ADR 002: single writer and fail-stop
Status: accepted.

One ordered stream per process, per-symbol books. No locks, wall-clock FIFO or concurrency claims. This makes replay deterministic without scheduling assumptions. A future parallel router would need a defined ordering and output merge policy.

Allocation failure can occur after quantity/counter mutation or node insertion. Strong rollback across map/list/hash/event allocations would add transaction machinery. Catch all exceptions within mutating apply, poison the book and rethrow; all subsequent state operations refuse use. Destruction via RAII is safe even with partially linked state. Business rejections do not mutate, but an allocation failure while creating the rejection event is still an operational error. tests/allocation.cpp injects failure budgets into ordinary global new paths for a partial sweep/remainder; it is not exhaustive proof of all allocators/operations. No recovery or fault-tolerant service claim.
