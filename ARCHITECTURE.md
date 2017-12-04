
SONGS

Each song is part of exactly one _collection_. The collection has a root path where
all songs are fetched from, it's _url_.

Any song paths received from `MusicDatabase` should be a `RemotePath`. A RemotePath is
formed as <collection_id>::<song_path>

PRODUCTS

A `Product` is a demo or a game.
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


THREADING

IN GENERAL

Use threads for background work but report result on originating thread. Makes for nice
async design through labdas such as;

```c++
getUrl("http://some.thing", [=](const std::string &result) {
	// Lambda is called on same thread that called getUrl()
	doSomething(result); // <- Safe
});
```


SPECIFICS

The MusicPlayer runs it's own thread, and all calls to MusicPlayerList must be thread
safe. The player reads files or gets binary data, and plays audio.

FFT data is calculated in the musicplayer thread by the `SpectrumAnalyzer` class and
fetched through `getSpetrum()` and the `fftMutex` makes sure the analyzer is not accessed
simultaniously.

The `infoMutex` is used to make sure the SongInfo struct is updated atomically.

MusicPlayerList -> MusicPlayer




THREADING CHANGES

MusicPlayer have no threading operations.

MusicPlayerList takes care of all sync since it runs the thread


