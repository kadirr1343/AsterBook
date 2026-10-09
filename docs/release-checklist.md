# Publication checklist

- [x] Local GCC 13.3 C++23 expected configure probe; debug/release correctness.
- [x] ASan/UBSan local run with detect_leaks=0; ptrace blocks local LeakSanitizer. Run full sanitizer including leak detection on an unrestricted machine/CI.
- [x] MIT holder from actual `git config user.name`: abdulkadir tas. Confirm desired spelling/legal attribution before publication.
- [x] Deterministic examples and real measured corpora/reports. No external runtime dependency or vendored content.
- [x] Public https://github.com/kadirr1343/AsterBook created; source uploaded and origin/main configured. No hosted release exists.
- [x] First GitHub Actions run passed all jobs including leak detection: https://github.com/kadirr1343/AsterBook/actions/runs/37989181624.
- [x] Public GitHub clean clone: Release configure/build, 6/6 CTest and sweep verification (8 commands) passed.
- [x] Publication source review, build/cache exclusion and real CI badge completed.
- [ ] Make real commits with current authorship/date. Tag v0.1.0 only after review, push/tag/release only with explicit authorization.

Example commands for the owner: `git init -b main`, `git add .`, `git diff --cached`, `git commit -m "Implement AsterBook v0.1.0"`. Set remote to your actual repository, not an invented address. Source SHA-256 in measurements covers implementation/build/tests/tools; no fabricated commit hash.

Future work: cached top/count queries, per-operation latency histograms, allocation profiles, optional fuzz target, wider compiler verification. Market/FOK/replace, networking and parallel writers remain outside v1. C++26 is not required or claimed.

Action checkout pin is the verified [v4.2.2 commit](https://github.com/actions/checkout/commit/11bd71901bbe5b1630ceea73d27597364c9af683); least privilege and immutable action refs follow [GitHub security guidance](https://docs.github.com/en/actions/reference/security/secure-use). Official [GCC support](https://gcc.gnu.org/projects/cxx-status.html) and [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) were consulted.
