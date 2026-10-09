# Portfolio

AsterBook is a C++23 single-writer matching and deterministic replay laboratory, with independent vector and price-level implementations and reproducible experimental evidence.

Verifiable CV bullets:
- Implemented integer-tick price-time matching, FIFO, partial fills, IOC expiry, cancellation and active-ID reuse across isolated symbols.
- Validated independent engines with hand-computed golden tests and 100 fixed seeds × 10,000 commands, comparing events/state/depth after each command and checking quantity conservation.
- Built strict replay/generator CLI and fresh-state throughput/latency benchmarks with raw corpora, SHA-256 metadata and dependency-free HTML reporting.
- Exercised allocation failures and fail-stop semantics; Debug/Release and ASan/UBSan passed locally with local leak checking unavailable under ptrace.

No universal speedup, production readiness, real exchange or GitHub CI success claim. See benchmarking.md for actual measurements and active-state sizes.

## Two-minute demo (Türkçe)
0:00–0:20: README ve iki veri yapısını aç; amacın gerçek borsa bağlantısı değil deterministik bir C++ laboratuvarı olduğunu anlat.
0:20–0:50: `build/release/asterbook demo`. Aynı fiyat id=1 sonra id=2 FIFO; id=3 kısmi dolum; iptal kalan 2; IOC kalan 3; duplicate kabul sayacını tüketmez; id=1 acceptance=6 ile yeniden kabul edilir.
0:50–1:20: `build/release/asterbook verify --input examples/sweep.csv`; tests/test.cpp differential fonksiyonunu aç. İki motorun ortak matching helper kullanmadığını ve miktar denklemindeki 2 katsayısını açıkla.
1:20–1:45: price_level.cpp iptal yolunu göster. Locator düğümleri borç alır, snapshot sahipli kopyadır. Hash araması ortalama O(1), bütün işlemin kötü durumu O(1) diye iddia etme.
1:45–2:00: benchmarks/results HTML raporunu aç. Büyük add-only ve küçük aktif mixed farkını, timing kapsamını ve profiling yapılmadığını anlat. Yayın öncesi CI/leak kontrolünün durumunu belirt.
