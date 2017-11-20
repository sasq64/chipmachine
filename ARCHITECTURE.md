
Each song is part of exactly one _collection_. The collection has a root path where
all songs are fetched from.

Any song paths received from the song database should be a `RemotePath`. A RemotePath is
formed as <collection_id>::<song_path>

A product is a demo or a game.
A Product contains a set of songs, and songs can be part of many products.

Products and songs are similarly indexed for searching.

Products all come after songs in the index, so we know if a search result is a product if
it's index is larger than `productStartIndex`.

A `SongInfo` can contain a product. The `RemotePath` is then formed as "product::<product_id>"

in `playCurrent()`, the path is checked and the SongInfo expanded to a list of songs by
calling `getProductSongs()`, and the result replaces the current play queue.


SCREENSHOTS

If the current song has a SCREENSHOT metadata, it is used, otherwise
`getSongScreenshots()` is called. This should not happen on a product.

getSongScreenshots() tries to find a product that contains ths current song, and is a
"Demo" or "Trackmo" and uses
screenshots from that product.

Each song in a product will get the same set screenshots from the product info.


