# Contributing
Use GCC 13+, CMake 3.25+, Ninja, Python 3. Configure/build/test debug and release presets, then sanitize. Format C++ with clang-format-18. Controls are always active; do not replace require with NDEBUG-dependent assert. Keep matching implementations independent. Update semantics and golden cases before changing business rules. A failed differential run writes out/failing-prefix.csv with seed/event information.

Run `build/release/asterbook_tests --extended` for 100 seeds × 10,000 commands. Default is 8 × 1,500. No strict performance gates on shared CI runners. Regenerate raw measurements only in a documented environment; retain corpus and metadata.
