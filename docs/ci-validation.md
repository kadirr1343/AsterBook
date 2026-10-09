# Hosted CI validation

Verified run: [https://github.com/kadirr1343/AsterBook/actions/runs/37989181624](https://github.com/kadirr1343/AsterBook/actions/runs/37989181624). Source commit: `4199552bbda4913e65d11733fcd727d74b6ae9e4`.

Debug, Release and sanitizer jobs completed successfully. Each CTest invocation passed all six tests. Formatting and CLI/benchmark smoke checks passed. Sanitizer job logs confirmed `ASAN_OPTIONS=detect_leaks=1` and `UBSAN_OPTIONS=halt_on_error=1`. This verifies leak detection on the GitHub runner; the earlier local ptrace limitation remains documented.

A separate public GitHub clean clone configured and built Release successfully, passed 6/6 CTest tests and verified all 8 commands in `examples/sweep.csv`. No additional performance measurement is claimed by this publication check.
