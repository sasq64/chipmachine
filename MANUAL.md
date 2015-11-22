# Chipmachine

## INTRO

Chipmachine is a fullscreen, graphical music player for various (retro) formats.
It uses instant incremental search to locate songs in it's song database -- meaning
you don't need to download or add music files yourself.
Initially, it searches the *Modland* colletion and downloads songs from *ftp.modland.com*.

## QUICKSTART

* Install the player and start it (''chipmachine'')
* Begin typing parts of a song and/or composer.
* Press Enter to play the selected match.

## SHORTCUTS

* **A-Z,0-9,SPACE,BACKSPACE** = Edit search field
* **UP/DOWN/PAGEUP/PAGEDOWN** = Select song in search screen
* **LEFT/RIGHT** = Switch subtune
* **ENTER** = Play selected song OR play next song (from Player screen)
* **SHIFT-ENTER** = Add song to play queue
* **F1** = Player screen
* **F2** = Search Screen
* **F5** = Play/Pause
* **F6** = Next song
* **F8** = Clear play queue

## ADDING NEW SONG COLLECTIONS

Song collections are configured in `lua/db.lua`. The High Voltage Sid Collection is
preconfigured, just unpack the `C64Music` directory into the chipmachine directory and
start chipmachine to index it. At this point you may also want to add *exlude_formats* to
the Modland section, to exclude all SIDS from there and avoid duplicate songs in the
database.

## CHIPMACHINE FILES

Chipmachine reads and write several files in it's directory that can be good to know about.

* `music.db` - This is the sqlite3 database with all the songs. It is generated from the
  configured sources if it does not exist.
* `index.dat` - This is the index used for incremental search. It is calculated on startup
  if it does not exist.
* `data/` - This directory contains several files that Chipmachine requires to run, like
  BIOS files for music emulators, and song information data like `songlengths.dat` and
`STIL.txt`.
* `_files/` - This is a local cache of files fetched from the Internet. If this directory
  becomes too large you can clear all o rsome of the files in it.
* `lua/init.lua` - This is a settings file that defines the layout of the screen. You can
  try playing around with it.  It is automatically reloaded if changed while chipmachine
is running.
* `lua/db.lua` - This file defines the music sources for the database.

If you make changes to `db.lua` you probably want to delete both `music.db` and
`index.dat` 
