# Progress and verified execution

Completed locally on 2026-10-09, Europe/Istanbul. Sources are in this directory; no Git remote, commit, push, repository publication or release created. Parent workspace had an unusable .git placeholder and no user source files. No applicable AGENTS.md was found. GCC 13.3, libstdc++ release 13, CMake 3.28.3, Ninja, Python 3.12.3 and clang-format-18 were available. std::expected configure test succeeded. No external C++ dependencies or downloaded frameworks.

| Stage | Verified outcome |
|---|---|
| 1: contract/build | CMake debug smoke compiled and ran; presets schema 6/minimum CMake 3.25 |
| 2: reference model | Hand-computed price/FIFO, fill, IOC, cancellation, ID reuse and rejection cases |
| 3: price levels | Independent matching/erase logic, stable locator invariants, per-command differential comparison, conservation |
| 4: adapters/CLI | demo/replay/verify/generate, strict input and exit errors, repeated canonical replay, multi-symbol routing |
| 5: experiments | Eight recorded corpora; both engines, five throughput and separate latency passes, SHA-256 metadata, eight HTML reports |
| 6: documentation/CI | English/Turkish README, learning/interview/portfolio guides, ADRs, MIT, workflow and publication checklist |
| 7: clean-copy validation | Fresh build directories in a source-only copy; debug/release/sanitizer, full README quickstart and extended tests passed |

Final Debug and Release: 6/6 CTest cases passed each. ASan+UBSan: 6/6 with ASAN_OPTIONS=detect_leaks=0 and UBSAN_OPTIONS=halt_on_error=1. Full local sanitizer invocation initially failed: "LeakSanitizer has encountered a fatal error ... LeakSanitizer does not work under ptrace". Leak detection is unavailable here, not a passing result. CI leaves detect_leaks=1; GitHub workflow itself has not run yet.

Final default correctness executable performs 72,068 active checks. Extended: 100 seeds × 10,000 commands, 6,000,068 active checks passed. Separate allocation executable: 9 injected failure points poisoned the engine and refused subsequent use. These injected points cover a partial sweep plus resting remainder, not all conceivable allocation paths. No profiling/fuzzing/second-compiler result claimed.

Executed from project root (and again from the clean source copy):
```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
cmake --preset release
cmake --build --preset release
ctest --preset release
cmake --preset sanitize
cmake --build --preset sanitize
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --preset sanitize
./build/release/asterbook_tests --extended
./build/release/allocation_tests
./build/release/asterbook demo
./build/release/asterbook replay --input examples/sweep.csv --engine price-level --events out/events.csv --snapshot out/book.csv
./build/release/asterbook verify --input examples/sweep.csv
./build/release/asterbook generate --seed 42 --count 10000 --scenario mixed --output out/workload.csv
./build/release/asterbook bench --input out/workload.csv --engine both --output out/bench.csv
python3 tools/report.py --bench out/bench.csv --output out/report.html
```
Recorded experiments ran with `python3 tools/experiments.py`; all eight inputs were verified before timing. Formatting ran with `find include src apps tests benchmarks -name '*.cpp' -o -name '*.hpp' | xargs clang-format-18 --dry-run --Werror` and passed. All local Markdown links and retained corpus/event/snapshot hashes were verified; two replay output pairs were byte-identical. At measurement completion, source/binary digests matched the implementation. Later publication-only helper whitespace cleanup is documented in publication-changes.md; the measured engine binary is unchanged. Canonical demo is captured in examples/demo.txt. Clean test logs are retained in [validation.txt](validation.txt); personal absolute paths are removed from those logs.

Remaining publication work: choose actual GitHub repository/remote, review license attribution (from real local git user.name), make real commit(s), run GitHub CI including full leak checking, then approve/tag/publish v0.1.0. See release-checklist.md. No local implementation stub remains; roadmap features are explicitly outside v1.

## GitHub publication preparation

Owner authorized publication on 2026-10-09; local Git initialized on main with real configured author name and account noreply email. README engineering overview, EditorConfig and GitHub generated-file attributes added. Authentication is pending; no remote creation, push, release or hosted CI run yet. See github-publication.md.

GitHub account kadirr1343 authenticated; public https://github.com/kadirr1343/AsterBook created. Source upload and hosted CI are in progress. Historical local statements above refer to their recorded development stage.
