
;		Musicline Editor - © by Musicline in 1995


;  Part Effects Structure

;  Part Data
;	00  --- 00 00xy 0000 0000 0000 0000
;	  \    \  \  \ \
;	   \    \  \  \ Effect Parameter
;	    \    \  \  Effect Number
;	     \    \  Instrument
;	      \    Note
;	       Position

FXMac		MACRO
\1		equ	\2
		dc.l	p\1
		ENDM

FXMacUn		MACRO
		dc.l	p\1
		ENDM


FX_JumpTable
*·····            Pitch                             ·····*
	FXMac	fx_UNUSED,$00
	FXMac	fx_SlideUp,$01			xy = 00-FF
	FXMac	fx_SlideDown,$02		xy = 00-FF
	FXMac	fx_Portamento,$03		xy = 00-FF
	FXMac	fx_InitInstrumentPortamento,$04	xy = --
	FXMac	fx_PitchUp,$05			xy = 00-FF
	FXMac	fx_PitchDown,$06		xy = 00-FF
	FXMac	fx_VibratoSpeed,$07		xy = 00-FF
	FXMac	fx_VibratoUp,$08		xy = 00-40  Init Vibrato
	FXMac	fx_VibratoDown,$09		xy = 00-40  Init Vibrato
	FXMac	fx_VibratoWave,$0A		xy = 00-03  00=Sinus
		;					    01=Down Ramp
		;					    02=Saw Tooth
		;					    03=Square
	FXMac	fx_SetFinetune,$0B
	FXMacUn	fx_UNUSED,$0C
	FXMacUn	fx_UNUSED,$0D
	FXMacUn	fx_UNUSED,$0E
	FXMacUn	fx_UNUSED,$0F

*·····            Instrument Volume                 ·····*
	FXMac	fx_Volume,$10			xy = 00-40
 	FXMac	fx_VolumeSlideUp,$11		xy = 00-FF
	FXMac	fx_VolumeSlideDown,$12		xy = 00-FF
	FXMac	fx_VolumeSlideToVolSet,$13	xy = 00-40
	FXMac	fx_VolumeSlideToVol,$14		xy = 00-FF
	FXMac	fx_VolumeAdd,$15		xy = 00-40
	FXMac	fx_VolumeSub,$16		xy = 00-40
	FXMac	fx_TremoloSpeed,$17		xy = 00-FF
	FXMac	fx_TremoloUp,$18		xy = 00-40  Init Tremolo
	FXMac	fx_TremoloDown,$19		xy = 00-40  Init Tremolo
	FXMac	fx_TremoloWave,$1A		xy = 00-03  00=Sinus
		;					    01=Down Ramp
		;					    02=Saw Tooth
		;					    03=Square
	FXMacUn	fx_UNUSED,$1B
	FXMacUn	fx_UNUSED,$1C
	FXMacUn	fx_UNUSED,$1D
	FXMacUn	fx_UNUSED,$1E
	FXMacUn	fx_UNUSED,$1F

*·····            Channel Volume                    ·····*
	FXMac	fx_ChannelVol,$20		xy = 00-40
	FXMac	fx_ChannelVolSlideUp,$21	xy = 00-FF
	FXMac	fx_ChannelVolSlideDown,$22	xy = 00-FF
	FXMac	fx_ChannelVolSlideToVolSet,$23	xy = 00-40
	FXMac	fx_ChannelVolSlideToVol,$24	xy = 00-FF
	FXMac	fx_ChannelVolAdd,$25		xy = 00-40
	FXMac	fx_ChannelVolSub,$26		xy = 00-40
	FXMac	fx_AllChannelVol,$27		xy = 00-40
	FXMacUn	fx_UNUSED,$28
	FXMacUn	fx_UNUSED,$29
	FXMacUn	fx_UNUSED,$2A
	FXMacUn	fx_UNUSED,$2B
	FXMacUn	fx_UNUSED,$2C
	FXMacUn	fx_UNUSED,$2D
	FXMacUn	fx_UNUSED,$2E
	FXMacUn	fx_UNUSED,$2F

*·····            Master Volume                     ·····*
	FXMac	fx_MasterVol,$30		xy = 00-40
	FXMac	fx_MasterVolSlideUp,$31		xy = 00-FF
	FXMac	fx_MasterVolSlideDown,$32	xy = 00-FF
	FXMac	fx_MasterVolSlideToVolSet,$33	xy = 00-40
	FXMac	fx_MasterVolSlideToVol,$34	xy = 00-FF
	FXMac	fx_MasterVolAdd,$35		xy = 00-40
	FXMac	fx_MasterVolSub,$36		xy = 00-40
	FXMacUn	fx_UNUSED,$37
	FXMacUn	fx_UNUSED,$38
	FXMacUn	fx_UNUSED,$39
	FXMacUn	fx_UNUSED,$3A
	FXMacUn	fx_UNUSED,$3B
	FXMacUn	fx_UNUSED,$3C
	FXMacUn	fx_UNUSED,$3D
	FXMacUn	fx_UNUSED,$3E
	FXMacUn	fx_UNUSED,$3F

*·····            Other                  ·····*
	FXMac	fx_SpeedPart,$40		xy = 00-1F
	FXMac	fx_GroovePart,$41		xy = 00-1F
	FXMac	fx_SpeedAll,$42			xy = 00-FF  00-1F=Speed
		;					    20-FF=Tempo
	FXMac	fx_GrooveAll,$43		xy = 00-1F
	FXMac	fx_ArpeggioList,$44		xy = 00-FF
	FXMac	fx_ArpeggioListOneStep,$45	xy = 00-FF
	FXMac	fx_HoldSustain,$46	 	xy = 00-01  00=ReleaseSustain
		;					    01=HoldSustain
	FXMac	fx_Filter,$47			xy = 00-01  00=Off
		;					    01=On
	FXMac	fx_SampleOffset,$48		xy = 00-FF  SampleOffset<<8 (21=2100)
	FXMac	fx_RestartNoVolume,$49		xy = --     Restarts Instrument without volume update
	FXMac	fx_WaveSample,$4A		xy = 00-FF  WaveSample Select
	FXMac	fx_InitInstrument,$4B		xy = --     Restarts all Instrument effects
	FXMacUn	fx_UNUSED,$4C
	FXMacUn	fx_UNUSED,$4D
	FXMacUn	fx_UNUSED,$4E
	FXMacUn	fx_UNUSED,$4F

fx_set	set	$50
	REPT	$E0-$50
	FXMacUn	fx_UNUSED,fx_set
fx_set	set	fx_set+1
	ENDR	

*·····            Protracker Pitch           ·····*
	FXMacUn	fx_UNUSED,$E0
	FXMac	fx_PTSlideUp,$E1		1xx : upspeed
	FXMac	fx_PTSlideDown,$E2		2xx : downspeed
	FXMac	fx_PTPortamento,$E3		3xx : up/down speed
	FXMac	fx_PTFineSlideUp,$E4		E1x : value
	FXMac	fx_PTFineSlideDown,$E5		E2x : value
	FXMac	fx_PTVolSlideUp,$E6
	FXMac	fx_PTVolSlideDown,$E7
	FXMac	fx_PTTremolo,$E8
	FXMac	fx_PTTremoloWave,$E9		E4x : 0-sine, 1-ramp down, 2-square
	FXMac	fx_PTVibrato,$EA		4xy : x-speed,   y-depth
	FXMac	fx_PTVibratoWave,$EB		E4x : 0-sine, 1-ramp down, 2-square
	FXMacUn	fx_UNUSED,$EC
	FXMacUn	fx_UNUSED,$ED
	FXMacUn	fx_UNUSED,$EE
	FXMacUn	fx_UNUSED,$EF

*·····            UserCommand            ·····*

	FXMac	fx_UserCommand,$F0		xy = 00-FF
fx_set	set	$F1
	REPT	$f
	FXMacUn	fx_UserCommand,fx_set		xy = 00-FF
fx_set	set	fx_set+1
	ENDR
