; medtrkdata.i --- ext. plr routine version

; The Song structure
; Instrument data here (504 bytes = 63 * 8)
msng_numblocks	EQU	504
msng_songlen	EQU	506
msng_playseq	EQU	508
msng_deftempo	EQU	764
msng_playtransp	EQU	766
msng_flags	EQU	767
msng_flags2	EQU	768
msng_tempo2	EQU	769
; msng_trkvol applies to MMD0/MMD1 only.
msng_trkvol	EQU	770
msng_mastervol	EQU	786
msng_numsamples	EQU	787
; Fields below apply to MMD2 modules only.
msng_pseqs	EQU	508
msng_sections	EQU	512
msng_trkvoltbl	EQU	516
msng_numtracks	EQU	520
msng_numpseqs	EQU	522
msng_trkpan	EQU	524
msng_flags3	EQU	528
msng_voladj	EQU	532
msng_channels	EQU	534
msng_m_echo	EQU	536
msng_m_echodep	EQU	537
msng_m_echolen	EQU	538
msng_m_stsep	EQU	540
FLAG3B_STEREO	EQU	0

		RSRESET
;Fastmem play data structure
fmp_currptr	RS.L	1	;current sample play pointer
fmp_currlen	RS.L	1	;remaining sample length
fmp_repeatptr	RS.L	1	;repeat restart (0 = no repeat)
fmp_repeatlen	RS.L	1	;repeat length
fmp_bufferptr	RS.L	1	;DMA audio buffer pointer (buffer 1)
fmp_bufferptr2	RS.L	1	;DMA audio buffer pointer (buffer 2)
fmp_audioaddr	RS.L	1	;ptr to channel's audio hardware
fmp_intmask	RS.W	1	;interrupt mask
fmp_whichbuff	RS.B	1	;currently used buffer
fmp_active	RS.B	1	;this points to a meaningful sample
fmp_synstart	RS.B	1	;start synthetic sound
fmp_pad		RS.B	1
fmp_buffsize	RS.W	1
*fmp_sizeof	EQU	__RS
fmp_sizeof	RS.L	1

; macros for entering offsets
DEFWORD	MACRO
\1	EQU	OFFS
OFFS	SET	OFFS+2
	ENDM
DEFBYTE	MACRO
\1	EQU	OFFS
OFFS	SET	OFFS+1
	ENDM
DEFLONG	MACRO
\1	EQU	OFFS
OFFS	SET	OFFS+4
	ENDM

OFFS	SET	0
; the track-data structure definition:
	DEFBYTE	trk_prevnote	;previous note number (0 = none, 1 = C-1..)
	DEFBYTE	trk_previnstr	;previous instrument number
	DEFBYTE	trk_prevvol	;previous volume
	DEFBYTE	trk_prevmidich	;previous MIDI channel
	DEFBYTE	trk_prevmidin	;previous MIDI note
	DEFBYTE	trk_noteoffcnt	;note-off counter (hold)
	DEFBYTE	trk_inithold	;default hold for this instrument
	DEFBYTE	trk_initdecay	;default decay for....
	DEFBYTE	trk_stransp	;instrument transpose
	DEFBYTE	trk_finetune	;finetune
	DEFWORD	trk_soffset	;new sample offset | don't sep this and 2 below!
	DEFBYTE	trk_miscflags	;bit: 7 = cmd 3 exists, 0 = cmd E exists
	DEFBYTE	trk_currnote	;note on CURRENT line (0 = none, 1 = C-1...)
	DEFBYTE	trk_outputdev	;output device
	DEFBYTE	trk_fxtype	;fx type: 0 = norm, 1 = none, -1 = MIDI
	DEFLONG	trk_previnstra	;address of the previous instrument data
; the following data only on tracks 0 - 3
	DEFLONG	trk_cinfo
	DEFLONG	trk_audioaddr	;hardware audio channel base address
	DEFLONG	trk_sampleptr	;pointer to sample
	DEFWORD	trk_samplelen	;length (>> 1)
	DEFLONG	trk_fmp
	DEFWORD	trk_prevper	;previous period
	DEFWORD	trk_porttrgper	;portamento (cmd 3) target period
	DEFBYTE	trk_vibshift	;vibrato shift for ASR instruction
	DEFBYTE	trk_vibrspd	;vibrato speed/size (cmd 4 qualifier)
	DEFWORD	trk_vibrsz	;vibrato size
	DEFLONG	trk_synthptr	;pointer to synthetic/hybrid instrument
	DEFWORD	trk_arpgoffs	;SYNTH: current arpeggio offset
	DEFWORD	trk_arpsoffs	;SYNTH: arpeggio restart offset
	DEFBYTE	trk_volxcnt	;SYNTH: volume execute counter
	DEFBYTE	trk_wfxcnt	;SYNTH: waveform execute counter
	DEFWORD	trk_volcmd	;SYNTH: volume command pointer
	DEFWORD	trk_wfcmd	;SYNTH: waveform command pointer
	DEFBYTE	trk_volwait	;SYNTH: counter for WAI (volume list)
	DEFBYTE	trk_wfwait	;SYNTH: counter for WAI (waveform list)
	DEFWORD	trk_synthvibspd	;SYNTH: vibrato speed
	DEFWORD	trk_wfchgspd	;SYNTH: period change
	DEFWORD	trk_perchg	;SYNTH: curr. period change from trk_prevper
	DEFLONG	trk_envptr	;SYNTH: envelope waveform pointer
	DEFWORD	trk_synvibdep	;SYNTH: vibrato depth
	DEFLONG	trk_synvibwf	;SYNTH: vibrato waveform
	DEFWORD	trk_synviboffs	;SYNTH: vibrato pointer
	DEFBYTE	trk_initvolxspd	;SYNTH: volume execute speed
	DEFBYTE	trk_initwfxspd	;SYNTH: waveform execute speed
	DEFBYTE	trk_volchgspd	;SYNTH: volume change
	DEFBYTE	trk_prevnote2	;SYNTH: previous note
	DEFBYTE	trk_synvol	;SYNTH: current volume
	DEFBYTE	trk_synthtype	;>0 = synth, -1 = hybrid, 0 = no synth
	DEFLONG	trk_periodtbl	;pointer to period table
	DEFWORD	trk_prevportspd	;portamento (cmd 3) speed
	DEFBYTE	trk_decay	;decay
	DEFBYTE	trk_fadespd	;decay speed
	DEFLONG	trk_envrestart	;SYNTH: envelope waveform restart point
	DEFBYTE	trk_envcount	;SYNTH: envelope counter
	DEFBYTE	trk_split	;0 = this channel not splitted (OctaMED V2)
	DEFWORD	trk_newper	;new period (for synth use)
	DEFBYTE	trk_vibroffs	;vibrato table offset \ DON'T SEPARATE
	DEFBYTE	trk_tremoffs	;tremolo table offset /
	DEFWORD	trk_tremsz	;tremolo size
	DEFBYTE	trk_tremspd	;tremolo speed
	DEFBYTE	trk_tempvol	;temporary volume (for tremolo)
	DEFWORD	trk_vibradjust	;vibrato +/- change from base period \ DON'T SEPARATE
	DEFWORD	trk_arpadjust	;arpeggio +/- change from base period/
	DEFWORD	trk_trackvol
TRACKDATASZ	EQU	OFFS

DMACON	EQU	$DFF096
INTENA	EQU	$DFF09A
INTREQ	EQU	$DFF09C

