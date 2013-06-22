
;		Musicline Editor - © by Musicline in 1995

;	00  --- 00 00xy 0000 0000 0000 0000
;	  \    \  \  \ \
;	   \    \  \  \ Effect Parameter
;	    \    \  \  Effect Number
;	     \    \  Instrument
;	      \    Note
;	       Position


		dc.b	"************ Pitch Commands ************",10,10
		dc.b	"Num/       Effect Name       /  Syntax
		dc.b	"00  -----",10
		dc.b	"01  SlideUp                   xy = 00-FF",10
		dc.b	"02  SlideDown                 xy = 00-FF",10
		dc.b	"03  Glide                     xy = 00-FF",10
		dc.b	"04  InitInstrumentGlide       xy = --",10
		dc.b	"05  PitchAdd                  xy = 00-FF",10
		dc.b	"06  PitchSub                  xy = 00-FF",10
		dc.b	"07  VibratoSpeed              xy = 00-FF",10
		dc.b	"08  VibratoUp                 xy = 00-40  Inits Vibrato",10
		dc.b	"09  VibratoDown               xy = 00-40  Inits Vibrato",10
		dc.b	"0A  VibratoWave	       xy = 00-03  00=Sinus",10
		dc.b	"                                          01=Down Ramp",10
		dc.b	"                                          02=Saw Tooth",10
		dc.b	"                                          03=Square",10
		dc.b	"0B  -----",10
		dc.b	"0C  -----",10
		dc.b	"0D  -----",10
		dc.b	"0E  -----",10
		dc.b	"0F  -----",10

*·····            Instrument Volume                 ·····*
		Volume10			xy = 00-40
 		VolumeSlideUp11		xy = 00-FF
		VolumeSlideDown12		xy = 00-FF
		VolumeSlideToVolSet13	xy = 00-40
		VolumeSlideToVol14		xy = 00-FF
		VolumeAdd15		xy = 00-40
		VolumeSub16		xy = 00-40
		TremoloSpeed17		xy = 00-FF
		TremoloUp18		xy = 00-40  Init Tremolo
		TremoloDown19		xy = 00-40  Init Tremolo
		TremoloWave1A		xy = 00-03  00=Sinus
		;					    01=Down Ramp
		;					    02=Saw Tooth
		;					    03=Square
		1B
		1C
		1D
		1E
		1F

*·····            Channel Volume                    ·····*
		ChannelVol20		xy = 00-40
		ChannelVolSlideUp21	xy = 00-FF
		ChannelVolSlideDown22	xy = 00-FF
		ChannelVolSlideToVolSet23	xy = 00-40
		ChannelVolSlideToVol24	xy = 00-FF
		ChannelVolAdd25		xy = 00-40
		ChannelVolSub26		xy = 00-40
		AllChannelVol27		xy = 00-40
		28
		29
		2A
		2B
		2C
		2D
		2E
		2F

*·····            Master Volume                     ·····*
		MasterVol30		xy = 00-40
		MasterVolSlideUp31		xy = 00-FF
		MasterVolSlideDown32	xy = 00-FF
		MasterVolSlideToVolSet33	xy = 00-40
		MasterVolSlideToVol34	xy = 00-FF
		MasterVolAdd35		xy = 00-40
		MasterVolSub36		xy = 00-40
		37
		38
		39
		3A
		3B
		3C
		3D
		3E
		3F

*·····            Other                  ·····*
		SpeedPart40		xy = 00-1F
		GroovePart41		xy = 00-1F
		SpeedAll42			xy = 00-FF  00-1F=Speed
		;					    20-FF=Tempo
		GrooveAll43		xy = 00-1F
		ArpeggioList44		xy = 00-FF
		ArpeggioListOneStep45	xy = 00-FF
		HoldSustain46	 	xy = 00-01  00=ReleaseSustain
		;					    01=HoldSustain
		Filter47			xy = 00-01  00=Off
		;					    01=On
		SampleOffset48		xy = 00-FF  SampleOffset<<8 (21=2100)
		RestartNoVolume49		xy = --     Restarts Instrument without volume update
		WaveSample4A		xy = 00-FF  WaveSample Select
		InitInstrument4B		xy = --     Restarts all Instrument effects
		4C
		4D
		4E
		4F

*·····            Protracker Pitch           ·····*
		E0
		PTSlideUpE1		1xx : upspeed
		PTSlideDownE2		2xx : downspeed
		PTPortamentoE3		3xx : up/down speed
		PTSetFinetuneE4		E5x : set finetune
		PTFineSlideUpE5		E1x : value
		PTFineSlideDownE6		E2x : value
		E7
		E8
		PTVibratoE9		4xy : x-speed,   y-depth
		PTVibratoWaveEA		E4x : 0-sine, 1-ramp down, 2-square
		EB
		EC
		ED
		EE
		EF

*·····            UserCommand            ·····*

		UserCommandF0		xy = 00-FF

*	Arpeggio:
*Effekt styrd arp.lista enligt följande effekt nummer
*	SlideUp		= 1
*	SlideDown	= 2
*	Volume		= 3
*	VolumeSlideUp	= 4
*	VolumeSlideDown	= 5
*	Restart		= 6
