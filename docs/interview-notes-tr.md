# Mülakat notları

## 1. Neden iki motor?

ReferenceBook okunur bir oracle; PriceLevelBook bağımsız mutation mantığıyla veri yapısı karşılaştırması sağlar. Ortak matching helper yok. Kod: [ReferenceBook::apply](../src/reference.cpp).

## 2. FIFO nereden gelir?

Kabul edilmiş emir için ++accepted_; dış seq ve saat öncelik belirlemez. List push_back ve vector acceptance sırası aynı kuralı uygular. Kod: [PriceLevelBook::apply](../src/price_level.cpp).

## 3. Trade fiyatı neden maker fiyatı?

Sözleşme bekleyen fiyatı seçer. Reference it->price, price-level level->first kullanır. Kod: [ReferenceBook::apply](../src/reference.cpp).

## 4. Locator ne tutar?

Side, map seviye iterator ve list emir iterator. Hash rehash bu farklı container iteratorlarını bozmaz. Kod: [PriceLevelBook::Locator](../include/asterbook/book.hpp).

## 5. Neden book taşınamaz?

İndeksin container düğümlerine bağımlılığı yanlış copy/move ile bozulabilir. v1 base copy/move işlemlerini siler. Kod: [Book](../include/asterbook/book.hpp).

## 6. İptal gerçekten O(1) mi?

Hash ortalama O(1), kötü O(N); list erase O(1); map iterator erase amortize O(1), bireysel dengeleme O(log L). Baştan tarama yok. Kod: [PriceLevelBook::apply](../src/price_level.cpp).

## 7. Vector kötü durum maliyeti?

Linear arama, erase kaydırma, kalan eklenince tam sort; sweep çok erase ile O(KN). Oracle hız için tasarlanmadı. Kod: [ReferenceBook::apply](../src/reference.cpp).

## 8. Snapshot sahibi kim?

Çağıran vector<Order> değerinin sahibi; borç alınmış iterator sızmaz. Motor değişikliği eski snapshotı değiştirmez. Kod: [PriceLevelBook::snapshot](../src/price_level.cpp).

## 9. ID tekrar kullanılır mı?

Yalnız aktif ID duplicate. İptal/dolum/IOC sonrası aynı ID yeni acceptance nesliyle kabul edilir. Kod: [golden](../tests/test.cpp).

## 10. Tahsis hatasında ne olur?

apply catch tüm exceptionlarda poison işaretler ve rethrow eder. Rollback yok, yeniden kullanım yasak. Fault injection bunu kontrol eder. Kod: [check](../tests/allocation.cpp).

## 11. Reddin yan etkisi var mı?

Validation/duplicate/capacity matching ve sayaç artışından önce. Rejected event tahsisi hata verirse bu operasyonel fail-stop olur. Kod: [ReferenceBook::apply](../src/reference.cpp).

## 12. Taşmayı nasıl önlüyorsun?

Unsigned parse from_chars ile bütün tokenı doğrular; motor aktif+gelen miktarı ve sayaç kapasitesini mutation öncesi konservatif kontrol eder. Kod: [number](../src/io.cpp).

## 13. Doğruluğu nasıl test ettin?

Golden sonuçlar, her komutta event/snapshot/depth/counter eşitliği ve bağımsız invariants; accepted=active+2*trades+dropped. Release kontrolleri kapanmaz. Kod: [differential](../tests/test.cpp).

## 14. Benchmark neyi ölçüyor?

Önceden parse edilmiş aynı corpus, boş motorlar, apply ve event tahsisi/result saklama. Throughput döngüsü ve latency geçişi ayrı; I/O ve checksum dışarıda. Kod: [bench](../benchmarks/harness.cpp).

## 15. Price-level neden bazen daha yavaş?

Küçük N için node tahsisleri ve pointer erişimi vector yerelliğine yenilebilir. Ham ölçüm bunu gösterir; profiling yapılmadığından neden hipotezdir. Kod: [raw experiments](../benchmarks/results/).

## 16. Deterministik replay nasıl sağlanır?

Sıralı input, per-symbol sayaç, maker fiyatı ve kabul FIFO; canonical symbol/side/price/FIFO çıktı. unordered_map yalnız lookup içindir. Kod: [replay](../src/io.cpp).
