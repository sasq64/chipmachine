chipmachine
===========

A portable, multi format, demo scene C++ music player 

## Building the main application (Linux/Debian)

#### All required packages (hopefully):

	# sudo apt-get install cmake git g++ zlib1g-dev libao-dev libgl1-mesa-dev libasound2-dev libglfw3-dev libcurl4-gnutls-dev libglew-dev 

#### Building:

	# git clone https://github.com/sasq64/apone.git
	# git clone https://github.com/sasq64/chipmachine.git
	# mkdir build ; cd build
	# cmake ../chipmachine -DCMAKE_BUILD_TYPE=Release
	# make -j8

## Using the application

* Type words separated by spaces for incremental search
* *ENTER* to play, *SHIFT-ENTER* to enque
* *F1* = Player screen, *F2* = Search screen
* *F5* = Play/Pause
* *F6* = Next Song (or *ENTER* from Player Screen)
* *ESC* = Clear search field
* *SHIFT-ESC* = Quit
* *F7* = Toggle Favorite

## Configuring the application

* Start with *-f* for fullscreen or *-d* for debug output
* Checkout `lua/screen.lua` for GUI layout

## Raspberry PI notes

* Building on the PI takes about 100x longer
* Database and index are generated on first start which may take some time

## For developers

* Look at the minimal player in `miniplay/` to see how the plugin interface works.
* Build it with make -f miniplay.mk

