# ADR 001: vector oracle and price levels
Status: accepted.

Keep the vector reference readable, using independent matching and linear ID lookup. The second motor uses ascending maps for both sides (buy best is rbegin/prev(end)), lists for FIFO and a hash locator index. No shared matching/cancel/severity mutation helper. A shared externally specified validation helper and owned depth formatter reduce adapter drift; golden expectations remain independent.

Stable list and map nodes make cancellation direct, at the cost of allocations and weak spatial locality. Locators hold both map and list iterators. Index rehash invalidates hash iterators, not the locators' foreign iterators; no hash iterator is retained across insertion. A list erase invalidates exactly that order locator, removed before further use. A map level is erased only when empty. Book copy and move are deleted, including through the base. Rebuilding an index for copies is a future choice, not an accidental default.

Do not promise universal speedups: small active books can favor vector. Top/count/depth use snapshot copies in v1, prioritizing a simple safe public API.
