# Publication checklist

- [x] Local GCC 13.3 C++23 expected configure probe; debug/release correctness.
- [x] ASan/UBSan local run with detect_leaks=0; ptrace blocks local LeakSanitizer. Run full sanitizer including leak detection on an unrestricted machine/CI.
- [x] MIT holder from actual `git config user.name`: abdulkadir tas. Confirm desired spelling/legal attribution before publication.
- [x] Deterministic examples and real measured corpora/reports. No external runtime dependency or vendored content.
- [ ] Create/select your GitHub repository and set a real remote. A local main repository is now prepared; remote creation and push await GitHub authentication. No hosted release exists.
- [ ] First GitHub Actions run: workflow is prepared, not yet run on GitHub. Require all jobs including leak detection before tagging.
- [ ] After initial commit, clean clone and run README commands again; local clean-copy validation is recorded in progress.md.
- [ ] Review `git diff --cached`, ignored build/out files, personal paths, secrets, licenses and repository links. Add badge only with a real URL.
- [ ] Make real commits with current authorship/date. Tag v0.1.0 only after review, push/tag/release only with explicit authorization.

Example commands for the owner: `git init -b main`, `git add .`, `git diff --cached`, `git commit -m "Implement AsterBook v0.1.0"`. Set remote to your actual repository, not an invented address. Source SHA-256 in measurements covers implementation/build/tests/tools; no fabricated commit hash.

Future work: cached top/count queries, per-operation latency histograms, allocation profiles, optional fuzz target, wider compiler verification. Market/FOK/replace, networking and parallel writers remain outside v1. C++26 is not required or claimed.

Action checkout pin is the verified [v4.2.2 commit](https://github.com/actions/checkout/commit/11bd71901bbe5b1630ceea73d27597364c9af683); least privilege and immutable action refs follow [GitHub security guidance](https://docs.github.com/en/actions/reference/security/secure-use). Official [GCC support](https://gcc.gnu.org/projects/cxx-status.html) and [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) were consulted.
