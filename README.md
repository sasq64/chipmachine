chipmachine
===========

A portable C++ music player and 'framework'

## Building the main application (Linux/Debian)

All required packages;

	# sudo apt-get install git g++ libao-dev libfreetype6-dev libpng12-dev libasound2-dev libglfw-dev libcurl4-gnutls-dev libglew-dev fftw-dev

For UADE support;

	# git clone --depth 1 https://github.com/heikkiorsila/bencode-tools
	# cd bencode-tools ; ./configure ; make ; sudo make-install ; cd ..
	# git clone --depth 1 git://zakalwe.fi/uade
	# cd uade ; ./configure ; make -j8 ; sudo make-install ; cd ..

Chipmachine binary;

	# git clone --depth 1 https://github.com/sasq64/cpp-mods.git
	# git clone --depth 1 https://github.com/sasq64/chipmachine.git
	# cd chipmachine
	# make -j8

## Using the application

* Type words separated by spaces for incremental search
* ENTER to play, *SHIFT-ENTER* to enque
* *F1* = Player screen, *F2* = Search screen
* *F9* = Next Song, *F10* = Clear search field, *F12* = Clear playlist

## Raspberry PI notes

* Building on the PI takes at least 45 minutes compared to about 1 minute cross compiling from a normal PC
* You need to `sudo chmod a+rw /dev/hidraw0` to get keyboard working.
* You need Raspbian **Jessie**, not **Wheezy** since chipmachine requires a modern C++ standard library.
* Database and index are generated on first start which may take some time


