# Chipmachine

## INTRO

Chipmachine is a fullscreen, graphical music player for various (retro) formats.
It uses instant incremental search to locate songs in it's song database -- meaning
you don't need to download or add music files yourself.

## QUICKSTART

* Install the player and start it (''chipmachine'')
* Begin typing parts of a song and/or composer.
* Press Enter to play the selected match.
* Press TAB to toggle command screen to see what else you can do.

## SHORTCUTS

* **A-Z,0-9,SPACE,BACKSPACE** = Edit search field
* **UP/DOWN/PAGEUP/PAGEDOWN** = Select song in search screen
* **LEFT/RIGHT** = Switch subtune
* **ENTER** = Play selected song OR play next song (from Player screen)
* **SHIFT-ENTER** = Add song to play queue
* **F1** = Player screen
* **F2** = Search Screen
* **F3** = Command Screen
* **F5** = Play/Pause
* **F6** = Next song
* **F7** = Toggle Favorite
* **F8** = Clear play queue

## CHIPMACHINE FILES

Chipmachine reads and write several files in it's directory that can be good to know about.

* `$HOME/.cache/chipmachine/music.db` - This is the sqlite3 database with all the songs. It is generated from the
  configured sources if it does not exist.
* `$HOME/.cache/chipmachine/index.dat` - This is the index used for incremental search. It is calculated on startup
  if it does not exist.
* `$HOME/.config/chipmachine/playlists/` - This directory contains your playlists, initially only *Favorites*.
  Playlists are simply text files with one song per line, you can manipulate and duplicate them outside Chipmachine.
* `data/` - This directory contains several files that Chipmachine requires to run, like
  BIOS files for music emulators, and song information data like `songlengths.dat` and
`STIL.txt`.
* `$HOME/.cache/chipmachine/_webfiles/` - This is a local cache of files fetched from the Internet. If this directory
  becomes too large you can clear all or some of the files in it.
* `lua/screen.lua` - This is a settings file that defines the layout of the screen. You can
  try playing around with it.  It is automatically reloaded if changed while chipmachine
is running.
* `lua/db.lua` - This file defines the music sources for the database.

If you make changes to `db.lua` you probably want to delete both `music.db` and
`index.dat`, or increase the version number.

## Data Sources

### Music Collections

* Modland - http://ftp.modland.com/
* High Voltage SID collection - http://www.hvsc.c64.org/
* Amiga remix - http://amigaremix.com/
* RKO - http://remix.kwed.org/
* Atari ST - http://sndh.atari.org/
* SNES Music - http://snesmusic.org/
* Atari SAP - http://asma.atari.org/
* Sounds of Scenesat - http://sos.scenesat.com/
* AmigaVibes - http://www.amigavibes.org/
* Demovibes - http://www.demovibes.org/

### Demo databases

* Pouet - http://pouet.net/
* Bitworld - http://janeway.exotica.org.uk/
* CSDb - http://csdb.dk/

### Podcasts

* Bitjam - http://www.bitfellas.org/podcast
* Syntax Error - http://www.syntaxerror.nu/
* C64 Take Away - http://c64takeaway.com/
* Gamewave - http://gamewave.yays.co/
* This Week in Chiptune - http://thisweekinchiptune.com/
* Bitar Till Kaffet - http://www.bitartillkaffet.se/

### Shoutcast Radio

Scenesat - http://www.scenesat.com/
SLAY Radio - http://www.slayradio.org/
Nectarine - https://www.scenemusic.net/
VGM Radio - http://vgmradio.com/
NoLife-Radio - http://nolife-radio.com/
Rainwave - http://chiptune.rainwave.cc/
ChipBit - http://www.chipbit.net/
The Sid Station - http://c64radio.com/
Radio Parallax - http://www.radio-paralax.de/
CGM UKScene Radio - http://www.lmp.d2g.com/
Retro PC Game Music Streaming Radio - http://gyusyabu.ddo.jp/

## Music Plugins (Supported formats)

### OpenMPT

Support for Amiga and PC tracker formats

* ProTracker, ScreamTracker III, FastTracker II, Impulse Tracker, OpenMPT, ScreamTracker II, NoiseTracker, Soundtracker, Mod's Grave, UltraTracker, Composer 669 / UNIS 669, MultiTracker, OctaMed, Farandole Composer, DigiTracker, Extreme's Tracker, Velvet Studio, DSIK Format, DSMI, ASYLUM, Oktalyzer, X-Tracker, PolyTracker, Epic Megagames, MASI, MadTracker 2, DigiBooster Pro, DigiBooster, Imago Orpheus, Galaxy Sound System

### High Technology

Support for Dreamcast and Sega Saturn music

### Higly Experimental

Support for Playstation 1 & 2 music

### NDS

Support for Nintendo DS music

### Game Music Emulator

Support for various 8 bit console music

* ZX Spectrum, Amstrad CPC, Nintendo Game Boy, Sega Genesis, Mega Drive, NEC TurboGrafx-16, PC Engine, MSX Home Computer, other Z80 systems, Nintendo NES, Famicom (with VRC 6, Namco 106, and FME-7 sound), Atari systems using POKEY sound chip, Super Nintendo, Super Famicom, Sega Master System, Mark III, Sega Genesis, Mega Drive, BBC Micro

### SC68

Support for Atari 16 bit music

### USF

Support for Nintendo 64 music

### StSound

Support for Atari ST music (older formats)

### ADplug

Support for PC soundcard music

* AdLib Tracker 2 by subz3ro, 
Westwood ADL File Format, 
AMUSIC Adlib Tracker by Elyssis, 
Bob's Adlib Music Format, 
BoomTracker 4.0 by CUD, 
Creative Music File Format by Creative Technology, 
EdLib by Vibrants, 
Digital-FM by R.Verhaag, 
Twin TrackPlayer by TwinTeam, 
DOSBox Raw OPL Format, 
DeFy Adlib Tracker by DeFy, 
HSC Adlib Composer by Hannes Seifert, HSC-Tracker by Electronic Rats, 
HSC Packed by Number Six / Aegis Corp., 
Apogee IMF File Format, 
Ken Silverman's Music Format, 
LucasArts AdLib Audio File Format by LucasArts, 
LOUDNESS Sound System, 
igin AdLib Music Format, 
Mlat Adlib Tracker, 
MIDI Audio File Format, 
MKJamz by M \ K Productions (preliminary), 
AdLib MSCplay, 
MPU-401 Trakker by SuBZeR0, 
Reality ADlib Tracker by Reality, 
RdosPlay RAW file format by RDOS, 
Softstar RIX OPL Music Format, 
AdLib Visual Composer by AdLib Inc., 
Screamtracker 3 by Future Crew, 
Surprise! Adlib Tracker 2 by Surprise! Productions, 
Surprise! Adlib Tracker by Surprise! Productions, 
Sierra's AdLib Audio File Format, 
SNGPlay by BUGSY of OBSESSION, 
Faust Music Creator by FAUST, 
Adlib Tracker 1.0 by TJ, 
eXotic ADlib Format by Riven the Mage, 
XMS-Tracker by MaDoKaN/E.S.G, 
eXtra Simple Music by Davey W Taylor, 

### MP3

Support for MP3 music

### Vice

Support for Commodore C64 music

### Hively

Support for AHX and HVL amiga music

### RSN

Support for RAR packed music (primarily SNES)

### Ayfly

Support for various XZ Spectrum formats

### MDX

Support for the Sharp X68000 Music Macro Language

### S98

Support for NEC PC98 Music

### AudioOverload

Support for Sega Saturn and Capcom Q music

### GSF

Support for Gameboy Advance music

### UADE

Support for Amiga exotic (Delitracker) formats

* ActionAmics AbyssHighestExperience ADPCM-mono AM-Composer AMOS ArtAndMagic Alcatraz-Packer ArtOfNoise-4V ArtOfNoise-8V AudioSculpture BeathovenSynthesizer BenDaglish BenDaglish-SID BladePacker ChipTracker Cinemaware CoreDesign custom CustomMade DariusZendeh DaveLowe DaveLowe-Deli DaveLoweNew DavidHanney DavidWhittaker DeltaMusic2.0 DeltaMusic1.3 Desire DIGI-Booster DigitalSonixChrome DigitalSoundStudio DynamicSynthesizer EMS EMS-6 FashionTracker FutureComposer1.3 FutureComposer1.4 Fred FredGray FutureComposer-BSI FuturePlayer ForgottenWorlds-Game GlueMon EarAche HowieDavies JochenHippel-CoSo QuadraComposer ImagesMusicSystem Infogrames InStereo InStereo2.0 JamCracker JankoMrsicFlogel JasonBrooke JasonPage JasonPage-JP JeroenTel JesperOlsen JochenHippel JochenHippel-7V Jochen-Hippel-ST KrisHatlelid Laxity LegglessMusicEditor ManiacsOfNoise MagneticFieldsPacker MajorTom Mark-Cooksey Mark-Cooksey-Old MarkII MartinWalker Maximum-Effect MCMD MED Medley MIDI-Loriciel MikeDavies MMDC Mugician MugicianII MusicAssembler MusicMaker-4V MusicMaker-8V MultiMedia-Sound NovoTradePacker NTSP-system Octa-MED Oktalyzer onEscapee PaulRobotham PaulShields PaulSummers PeterVerswyvelen PierreAdane ProfessionalSoundArtists PTK-Prowiz PumaTracker RichardJoseph RiffRaff RobHubbard RobHubbardOld Lionheart-Game SCUMM SeanConnolly SeanConran SIDMon1.0 SIDMon2.0 Silmarils SonicArranger SonicArranger-pc-all SonixMusicDriver SoundProgrammingLanguage SoundControl SoundFactory Sound-FX SoundImages SoundMaster SoundMon2.0 SoundMon2.2 SoundPlayer Special-FX Special-FX-ST SpeedyA1System SpeedySystem SteveBarrett SteveTurner SUN-Tronic Synth SynthDream SynthPack SynTracker TFMX TFMX-1.5-TFHD TFMX-7V TFMX-7V-TFHD TFMX-Pro TFMX-Pro-TFHD TFMX-ST ThomasHermann TimFollin TheMusicalEnlightenment TomyTracker Tronic UFO UltimateSoundtracker VoodooSupremeSynthesizer WallyBeben YM-2149 MusiclineEditor Soundtracker-IV Sierra-AGI DirkBialluch Quartet Quartet-PSG Quartet-ST 

### TedPlay

Support for Plus/4 music

### FFMPeg

Support for (Youtube) streaming audio

* AAC
* Ogg/Vorbis
