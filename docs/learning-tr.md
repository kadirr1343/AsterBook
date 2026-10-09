# Kod üzerinden C++ öğrenme rotası

| Dosya | Görev |
|---|---|
| [types.hpp](../include/asterbook/types.hpp) | Güçlü türler, enum class, komut ve olay değerleri |
| [book.hpp](../include/asterbook/book.hpp) | API, sahiplik sözleşmesi, locator türleri |
| [reference.cpp](../src/reference.cpp) | İzlenebilir vector matching/iptal modeli |
| [price_level.cpp](../src/price_level.cpp) | Fiyat seviyeleri, FIFO ve kimlik indeksi |
| [io.cpp](../src/io.cpp) | CSV parse, generator, sembol router ve kanonik çıktı |
| [main.cpp](../apps/main.cpp) | Kullanıcı argümanları ve exit code |
| [harness.cpp](../benchmarks/harness.cpp) | Ayrı throughput/latency geçişleri |
| [test.cpp](../tests/test.cpp) | Golden, differential, invariant ve miktar korunumu |
| [allocation.cpp](../tests/allocation.cpp) | Fail-stop hata enjeksiyonu |
| [cli.py](../tests/cli.py), [report.py](../tests/report.py) | Uçtan uca ve HTML hata yolları |
| [experiments.py](../tools/experiments.py), [report.py](../tools/report.py) | Gerçek metadata ve tek dosyalı rapor |

## Referans ile kopyayı ayır
`auto& resting = level->second.orders.front()` mevcut liste öğesinin takma adıdır. `resting.remaining.value -= q` defteri değiştirir. `auto loc = found->second` ise küçük Locator değerinin kopyasıdır; iterator kopyaları aynı düğümlere işaret eder. Bu sahipli bir Order kopyası değildir. `const auto& o = *loc.order` kopyalamadan okur, ama list erase sonrası kullanılmamalıdır. `snapshot()` vector<Order> döndürür: motor değişse bile snapshot'ın öğeleri yaşamaya devam eder.

## pair ve second
Map öğesi `pair<const PriceTicks, PriceLevel>` biçimindedir. `level->first` fiyat anahtarı, `level->second` seviye değeridir. Hash indeksindeki `found->second` ise Locator'dır. `->`, iterator'ın gösterdiği nesneye erişir; bütün iterator'lar ham pointer değildir. `try_emplace(...).first` burada ekleme sonucunun iterator'ını seçer; map öğesinin fiyatını seçmez. Aynı kelime farklı pair katmanında kullanılabilir.

## Geçerlilik tablosu
| İşlem | Etki |
|---|---|
| vector push_back ve yeniden tahsis | Tüm eski pointer/referans/iterator geçersiz |
| vector erase | Silinen ve sonrasındaki konumlar geçersiz |
| vector sort | Konumlar kalabilir ama o konumdaki emir kimliği değişebilir |
| list push_back | Mevcut emir iterator'ları geçerli |
| list erase/pop_front | Yalnız silinen düğüm geçersiz |
| map insert | Mevcut iterator/referans geçerli |
| map erase | Silinen seviyeye ait iterator/referans geçersiz |
| unordered_map rehash | Hash iterator'ları geçersiz; ayrı list/map iterator'ları etkilenmez |

ReferenceBook `it = orders_.erase(it)` kullanır; silinen iterator'ı artırmaz. PriceLevelBook `resting` referansını pop_front sonrası kullanmaz. Boş seviyeyi silmeden önce bütün emirleri ve indekslerini kaldırır. Kopyalama ve taşıma silinmiştir; raw iterator bağımlılıkları gizli biçimde başka book'a taşınmaz.

## Sahiplik ve RAII
Map listeleri, listeler Order değerlerini sahiplenir; hash yalnız konumları borç alır. `unique_ptr<Book>` tek sahibi belirler, kapsam sonunda virtual destructor doğru motoru yok eder. `std::span<const Command>` yalnız çağrı boyunca corpus'u borç alır; saklanmaz. Motorun API'si dışarıya iterator vermez. Exception sırasında container destructor'ları tahsisleri temizler; bu rollback anlamına gelmez. Poisoned nesne güvenle yok edilir ama işlemeye devam edemez.

## Türler ve expected
`OrderId`, `PriceTicks`, `Quantity`, `SymbolId` farklı `Value<Tag>` türleridir: bir fiyatı yanlışlıkla ID parametresine veremezsin. İçteki `.value` uint64 aritmetiğine geçiş noktasıdır; tag bütün aritmetik güvenliğini otomatik sağlamaz. `parse_line` bir `std::expected<Command,string>` döndürür. `if (!c)` hata dalı, `c.error()` açıklaması; `*c` yalnız başarı dalında komuttur. İş kuralı reddi typed Rejected event'tir, operasyonel hata exception'dır. `top` boş tarafta `std::optional` döndürür, sahte fiyat üretmez.

## Bir emrin yaşamı
Demo'da SELL id=3, fiyat=101, ilk miktar=4:
| Komut | Başlangıç | Birikmiş dolum | Aktif | Düşen |
|---|---:|---:|---:|---:|
| Kabul (acceptance=3) | 4 | 0 | 4 | 0 |
| BUY id=4 sweep | 4 | 2 | 2 | 0 |
| CANCEL id=3 | 4 | 2 | 0 | 2 |
İptal dolum değildir. Demo id=1 yeniden kabul edildiğinde acceptance=6 olur; eski id=1 acceptance=1 ile ayrılır. Dış seq=4'teki duplicate kabul sayacını tüketmez.

## Gerçek maliyet
Reference add kalanında bütün vector'ı sort eder: O(N log N), iptal arama/kaydırma O(N), sweep tekrarlı erase ile O(KN). Price-level yeni seviye O(log L), liste ekleme O(1), hash ortalama O(1) / kötü O(N). İptalde map.erase(iterator) amortize O(1); tek bir ağaç dengelemesi O(log L) olabilir. Snapshot/top/depth/count şu an O(N) kopya/tarama yapar. `invariant()` üretim hızlı yoluna konmamıştır. Küçük N için vector'ın ardışık belleği daha avantajlı olabilir; ölçüm bunu gösteriyor, profiling nedenini henüz kanıtlamıyor.

Sıra: önce `golden`, sonra ReferenceBook::apply, ardından Locator ve PriceLevelBook::apply, son olarak differential ve benchmark. Kendi deneyin için qty/fiyat/seed değiştir; expected sonucu elle hesapla ve test et.
