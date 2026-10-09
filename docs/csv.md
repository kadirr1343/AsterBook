# CSV v1

Header exactly `seq,action,symbol,order_id,side,price_ticks,quantity,tif`. UTF-8 ASCII tokens, comma-delimited, eight fields, LF or CRLF. No quoting, comments, blank rows, whitespace trimming, BOM or embedded commas/newlines. Header-only file is a valid empty replay; benchmark requires commands. No row is silently skipped.

ADD: positive sequence, unsigned symbol and ID, BUY/SELL, unsigned price and quantity, GTC/IOC. Zero price/quantity parse as typed values and are business rejections. Negative/overflow/non-numeric fields are parse failures. CANCEL: first four fields populated, last four empty: `9,CANCEL,1,42,,,,`. Sequence must strictly increase; gaps are allowed. Engines ignore external sequence when assigning FIFO.

Events are canonical in command order, with columns `kind,symbol,id,maker,acceptance,maker_acceptance,trade,price_ticks,quantity,error`. Kind codes: Accepted=0, Trade=1, Rested=2, Cancelled=3, Expired=4, Rejected=5. Error codes: None=0, Invalid=1, WrongSymbol=2, Duplicate=3, Unknown=4, Capacity=5. Zero fields mean not applicable (ID 0 itself is legal, so interpret with kind). Trade identifies taker ID/acceptance and maker ID/acceptance; trade number is per-symbol. Rejected has no acceptance generation.

Snapshot header `symbol,side,price_ticks,id,initial,remaining,acceptance`; symbol ascending, BUY descending prices then SELL ascending prices, FIFO. Empty books add no rows. No `unordered_map` iteration leaks into output. Generated inputs use std::mt19937_64 output modulo bounded ranges, not implementation-specific distributions. Replay determinism assumes the same corpus and contract/config; input generation is an untimed helper using the reference model to select live cancellation IDs. It copies state, so it is not intended as a high-throughput generator.

Files are ordinary recordings, not atomic transactions or WAL. An output write failure may leave one file written and another incomplete. CLI returns failure; do not consume partially written outputs as a successful run.
