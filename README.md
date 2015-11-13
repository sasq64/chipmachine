chipmachine
===========

## Intro

*A demoscene/retro Jukebox/spotify-like  music player*

* **Type anything to incrementally search in entire database**
![ONE](http://apone.org:8080/chipmachine/cs1.png)

* **Hit enter to play directly**
![TWO](http://apone.org:8080/chipmachine/cs2.png)


## Building

	# git clone https://github.com/sasq64/apone.git
	# git clone https://github.com/sasq64/chipmachine.git
	# mkdir build ; cd build
	# cmake ../chipmachine -DCMAKE_BUILD_TYPE=Release
	# make -j8

### Linux/Debian

	# sudo apt-get install cmake git g++ zlib1g-dev libao-dev libgl1-mesa-dev libasound2-dev libglfw3-dev libcurl4-gnutls-dev libglew-dev libmpg123-dev

### OSX

	# brew install ...


### Windows

* Requires Mingw x64 (Recommend MSYS2)

### Raspberry PI

...


## Using the application

* Type words separated by spaces for incremental search
* *ENTER* to play, *SHIFT-ENTER* to enque
* *F1* = Player screen, *F2* = Search screen
* *F5* = Play/Pause
* *F6* = Next Song (or *ENTER* from Player Screen)
* *ESC* = Clear search field
* *SHIFT-ESC* = Quit
* *F7* = Toggle Favorite

## Data Sources

* Modland - http://ftp.modland.com/
* High Voltage SID collection - http://www.hvsc.c64.org/
* Amiga remix - http://amigaremix.com/
* RKO - http://remix.kwed.org/
* Atari ST - http://sndh.atari.org/
* SNES Music - http://snesmusic.org/
* Atari SAP - http://asma.atari.org
* Bitjam Podcasts - http://www.bitfellas.org/podcast/
* CSDb Demos - http://csdb.dk/
* Sounds of Scenesat - http://sos.scenesat.com/

## Formats

Many
