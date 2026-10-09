# AsterBook — Deterministik Eşleştirme ve Replay Motoru

[![CI](https://github.com/kadirr1343/AsterBook/actions/workflows/ci.yml/badge.svg)](https://github.com/kadirr1343/AsterBook/actions/workflows/ci.yml)

C++23 ile hazırlanmış, sentetik limit emirlerini fiyat-zaman önceliğiyle eşleştiren çalıştırılabilir laboratuvar. Bağımsız vector referans modeli ve map/list/hash fiyat seviyeli motor, aynı olayları ve final durumu üretmek için test edilir. Gerçek emir göndermez; strateji veya piyasa veri defteri değildir.

## Çalıştırma
Proje kökünde GCC 13+ (std::expected kontrolü), CMake 3.25+, Ninja ve Python 3 gerekir. Yerelde GCC 13.3/CMake 3.28.3/Python 3.12.3 ile doğrulandı. Ağdan C++ bağımlılığı indirilmez.

```sh
git clone https://github.com/kadirr1343/AsterBook.git
cd AsterBook
cmake --preset release
cmake --build --preset release
ctest --preset release
./build/release/asterbook demo
./build/release/asterbook replay --input examples/sweep.csv --engine price-level --events out/events.csv --snapshot out/book.csv
./build/release/asterbook verify --input examples/sweep.csv
./build/release/asterbook generate --seed 42 --count 10000 --scenario mixed --output out/workload.csv
./build/release/asterbook bench --input out/workload.csv --engine both --output out/bench.csv
python3 tools/report.py --bench out/bench.csv --output out/report.html
```

## Neyi öğreniyorsun?
[Öğrenme rotası](docs/learning-tr.md), `auto&`, `it->second`, vector erase sonrası iterator, map/list düğüm ömrü, hash rehash ve RAII'yi gerçek kodla anlatır. [Mülakat notları](docs/interview-notes-tr.md) ilgili fonksiyonlara bağlı 16 cevap içerir. [Sözleşme](docs/semantics.md) ve [mimari](docs/architecture.md) kodun sınırlarını tanımlar.

| Özellik | Durum |
|---|---|
| Buy/Sell limit, GTC/IOC, kısmi ve çok seviyeli dolum | Uygulandı |
| Aktif ID iptali, ID yeniden kullanımı, sembol izolasyonu | Uygulandı ve test edildi |
| Kanonik CSV replay/verify ve corpus generator | Uygulandı ve uçtan uca doğrulandı |
| İki motor, invariant/differential/miktar korunumu | Her komut sonrası kontrol |
| Throughput ve ayrı latency ölçümü | Gerçek CSV + metadata + HTML |
| Debug/Release, ASan/UBSan | Yerelde çalıştırıldı |
| LeakSanitizer | ptrace engeli nedeniyle yerelde doğrulanamadı |
| GitHub CI | Dosyaları hazır; GitHub'da henüz çalışmadı |

Referans model eklemelerde bütün vector'ı sıralar; fiyat seviyeli motor düğüm tahsisi yapar. İkinci motor her senaryoda hızlı değildir. Ölçümleri [benchmark belgesinden](docs/benchmarking.md) ham CSV'ye kadar takip edebilirsin. Küçük aktif defterli akışlar büyük defter benchmark'ı gibi sunulmaz.

Mutasyon sırasında tahsis/operasyonel hata oluşursa rollback yoktur: motor poisoned olur, yeniden kullanım reddedilir. Snapshot/depth/top/count sorguları O(N) kopya/tarama yapar. Market/FOK/replace, concurrency, ağ, WAL ve C++26 kapsam dışında.

Uzun test: `./build/release/asterbook_tests --extended`. Sanitizer: configure/build sanitize, sonra normal `ctest --preset sanitize`; bu kısıtlı ortamda yalnız `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --preset sanitize` başarılı oldu. Sızıntı denetimi sınırlamasını gizlemiyoruz.

Tam kayıtlı deneyler: `python3 tools/experiments.py --output out/experiments`. Varsayılan konum benchmarks/results olduğu için bu seçenek mevcut baz ölçümleri korur. Bare bench yalnız CSV yazar; yanında metadata yoksa rapor platformu bilinmiyor olarak gösterir.

[Yayın kontrol listesi](docs/release-checklist.md): ilk CI ve tam sızıntı kontrolünü doğrulamak, lisans adını gözden geçirmek ve gerçek commit/tag oluşturmak sahibine bırakıldı. Public repo: https://github.com/kadirr1343/AsterBook. MIT.

## Ölçülmüş örnek

| 6,000 commands, seed 42 | Reference ops/s | Price-level ops/s | Raw data |
|---|---:|---:|---|
| add-only | 5,230 | 6,873,210 | [CSV](benchmarks/results/add-only-6000-128.csv) |
| cancel-heavy | 21,372,100 | 12,725,300 | [CSV](benchmarks/results/cancel-heavy-6000-128.csv) |
| mixed | 12,075,300 | 8,140,480 | [CSV](benchmarks/results/mixed-6000-128.csv) |
| sweep-heavy | 15,256,300 | 11,145,800 | [CSV](benchmarks/results/sweep-heavy-6000-128.csv) |

Kayıtlı makinede beş tekrarın medyanı; taşınabilir hız garantisi değildir. Mixed/cancel/sweep küçük aktif defterler tutar. [Yöntem, metadata ve tekrar komutları](docs/benchmarking.md).
