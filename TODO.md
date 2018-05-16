
## 1.3 TODO

### TEXT MODE

* Gray bg
* Next song on add
* Print next song
* Print length
* Sync colors
* Indicate favorite, mark favorite
* Show meta data

### NORMAL MODE

* Clipboard support
* (Drag & Drop support)
* (Command mode)

meta

what is current song and what is new song?



## ISSUES

* Deal with no Internet
* Deal with 404
* UADE SONG END does not go to next?
* Loop detecton, max play or remember skip position?
* Limit stream speed
* _lib2_ and _lib3_ support for PSF
* Common resampler in player, not in plugins

* Repeating tweens sometimes die (title scrolling stops, selection hilight gone)
* webgetter renaming fails when downloading the same song in parallell.
* Format colors wrong
* Parse collection color from db.lua ?
* sid titles wrong encoding 
* Mouse click select and scroll
* Search hits shown twice when term is in both composer and title
* Preload next song (RemoteLoader.assureCached() ?)
* Indicate error if lua fails

* Pause tween not stopped when new song starts


## TASKS / FEATURES

* Seeking in MP3s
* Seeking in MP3 streams
* Shuffle current play queue

* Print more info (KB size, source)

### Deal with different encodings

### Deal with multi file songs

Problem: Some SNG files does not require INS files.

### Playlist support

Multiple playlists
Online playlist
Smart playlists

Separate Playlist / Playqueue screen with edit capabilities


>> NETWORK RULES <<
>>>>>>>>><<<<<<<<<<




RELOGIN (NO SUCH USER)

Song is PATH in COLLECTION
FULL_PATH depends on offline/online

PATH,COLLECTION -> FULL_PATH
FULL_PATH -> PATH,COLLECTION

songLoader.addSource(name, local_dir, url)


File = SONG_LOADER(path, collection)

LOADER is a class that loads files either locally or from a remote location, and can cache the files




* Network threads stops quitting


filtering

filters.lua

NAME, set of formats

TAB to cycle filters    

PARTY MODE
----------

Tween probs: Pause and party red
release lock after song end wven if no new song
subsong++ -- when song not switched...


2 second grace only allows sub song?
only applies to songs started with ENTER



v1 SIMPLE
* Song must play for at least 60 seconds before new song can be selected, unless ALT is held


ALWAYS
* Queue remains with ENTER 





Own lock for fft 

SUBSONG does not work after ending in
USF64

Silence at start should have higher threshhold




DC and DS needs to rename libnames to lowercase




* Subtitle in USF64 (and dreamcast?)
* Old info returns if song cant be played
* position next info to right edge
* larger sizes on larger screen

* subsong grouping... RSN vs USF64 for instance.. search on subsongs?





----------------

BUGS

* PI cant always keep up with fft audio
* PI jerky scroll, high cpu when more than ~15 texts on search screen?

* Dont index secondary files
* Dont duplicate fonts
* openmpt type name too long
* Freeze when starting mods?
* Speed up inc search? Remember last set?

FX

* Scroll screens (& stars) left right
* Scroll stars up/down with playlist scrolling


GUI

* Edit playlist screen
* KEY brings up EDITOR of CURRENT list
* Can save to any list name and replaces old
* Find show playlist hits first



starttune
