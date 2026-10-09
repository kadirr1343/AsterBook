# Measurement method

[harness.cpp](../benchmarks/harness.cpp) receives already parsed commands. One untimed warmup is discarded. Each of five repetitions builds fresh books and routes before timing. Throughput times one whole apply loop, including virtual call, matching/cancellation, event/node/hash allocations and writing the owning Result into a preallocated results array. Old results are not destroyed inside that pass; retained events add memory/cache costs. Parsing, generation, symbol lookup preparation, snapshot/depth, checksum, CSV and HTML are outside timing.

A separate empty-state pass times each command with two steady_clock reads; storing latency samples is outside the pair. Latency result allocations and retention are included. p50/p95/p99 are nearest rank `ceil(q*N)-1` after sorting, independently per repetition; max is the largest observation. The timer_ns column is a mean of 1,000 back-to-back clock pairs, not a subtracted correction. It cannot remove OS jitter or characterize the entire observation overhead. Small N=1,000/6,000 samples do not establish reliable production tails. No pinning, perf stat, flame graph, HFT or service capacity claim.

Unsigned rolling checksums consume throughput events/snapshots and separate latency results after timing; they are optimizer observations, not cryptographic identities. Correctness compares actual values. Python SHA-256 identifies recorded corpus/events/snapshot, binary and implementation/build/test/tool sources. Metadata includes actual configured compiler probe, libstdc++ macros, flags, CPU, OS, seed, date, revision (no commit in this local run), peak active/level sizes, and sample/repetition counts. A bare CLI bench generates no environment metadata; use experiments.py for the full artifact set.

## Corpora and reproduction
```sh
cmake --preset release
cmake --build --preset release
python3 tools/experiments.py --output out/experiments
```
Eight pairs: scenarios × (1,000 commands/8 requested levels per side, 6,000/128). Seed=42. Each corpus is generated once, verified by both engines before timing, replayed to captured events/snapshot, then supplied to both engines unchanged. ADD prices are separated bid/ask ranges; generator picks known active cancellations from its untimed reference state. Mixed has cancel attempts at roll<25; cancel-heavy <60 when live orders exist; otherwise adds. Mixed crossing IOC at roll>70; sweep-heavy crossing IOC at roll<70. Large IOC quantities sweep available makers. Add-only never crosses and grows the book. No intentional rejects in these corpora. Actual accepted/cancelled/trade/expiry/reject counts appear in every raw CSV row.

Requested price spread is not actual occupied depth. Cancel/mixed/sweep frequently keep tiny active books; metadata peak sizes expose this. Add-only gives both growing sizes. These are experiments on these streams, not steady-state performance for every workload. Per-operation latency histograms and profiling are future work. Engine order is reference then price-level, so cache/order bias remains; results are descriptive, not a controlled hardware study.

## Recorded results
Measured 2026-10-09 Europe/Istanbul, Intel Core i7-10750H, Ubuntu/Linux x86_64, GCC 13.3 + libstdc++, Release -O3 -DNDEBUG -std=c++23. See per-file metadata for exact OS/compiler/flags/digests and clock baseline. Median throughput across five repetitions is summarized below; raw latency distributions and counters are linked with each experiment.

| Corpus | Peak orders / levels | Reference ops/s | Price-level ops/s | Evidence |
|---|---:|---:|---:|---|
| add-only-1000-8 | 1000 / 16 | 41,352 | 8,926,180 | [CSV](../benchmarks/results/add-only-1000-8.csv), [metadata](../benchmarks/results/add-only-1000-8.meta.json), [HTML](../benchmarks/results/add-only-1000-8.html) |
| add-only-6000-128 | 6000 / 256 | 5,230 | 6,873,210 | [CSV](../benchmarks/results/add-only-6000-128.csv), [metadata](../benchmarks/results/add-only-6000-128.meta.json), [HTML](../benchmarks/results/add-only-6000-128.html) |
| cancel-heavy-1000-8 | 4 / 4 | 18,285,500 | 11,850,400 | [CSV](../benchmarks/results/cancel-heavy-1000-8.csv), [metadata](../benchmarks/results/cancel-heavy-1000-8.meta.json), [HTML](../benchmarks/results/cancel-heavy-1000-8.html) |
| cancel-heavy-6000-128 | 4 / 4 | 21,372,100 | 12,725,300 | [CSV](../benchmarks/results/cancel-heavy-6000-128.csv), [metadata](../benchmarks/results/cancel-heavy-6000-128.meta.json), [HTML](../benchmarks/results/cancel-heavy-6000-128.html) |
| mixed-1000-8 | 10 / 9 | 11,233,100 | 8,333,820 | [CSV](../benchmarks/results/mixed-1000-8.csv), [metadata](../benchmarks/results/mixed-1000-8.meta.json), [HTML](../benchmarks/results/mixed-1000-8.html) |
| mixed-6000-128 | 14 / 14 | 12,075,300 | 8,140,480 | [CSV](../benchmarks/results/mixed-6000-128.csv), [metadata](../benchmarks/results/mixed-6000-128.meta.json), [HTML](../benchmarks/results/mixed-6000-128.html) |
| sweep-heavy-1000-8 | 8 / 6 | 12,756,600 | 10,753,800 | [CSV](../benchmarks/results/sweep-heavy-1000-8.csv), [metadata](../benchmarks/results/sweep-heavy-1000-8.meta.json), [HTML](../benchmarks/results/sweep-heavy-1000-8.html) |
| sweep-heavy-6000-128 | 8 / 8 | 15,256,300 | 11,145,800 | [CSV](../benchmarks/results/sweep-heavy-6000-128.csv), [metadata](../benchmarks/results/sweep-heavy-6000-128.meta.json), [HTML](../benchmarks/results/sweep-heavy-6000-128.html) |

Price-level wins the growing add-only corpus, while the reference wins these small-active cancel/mixed/sweep streams. Reference insertion sorts the whole vector; list/map nodes pay allocation and pointer traversal overhead. These are plausible explanations based on code, not profiling proof. Compare active-state sizes before comparing headline rates. Different repetitions expose timer/OS variation; no robust universal speedup or production tail claim follows.
