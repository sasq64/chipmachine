	incdir	"Amiga:Includes/"
	include "misc/DeliPlayer.i"

;
;
	SECTION Player,Code
;
;

	PLAYERHEADER PlayerTagArray

	dc.b '$VER: Protracker 3.0b player 2007-02-06',0
	even

PlayerTagArray
	dc.l	DTP_PlayerVersion,1
	dc.l	DTP_PlayerName,PName
	dc.l	DTP_ModuleName,MNamepointer
	dc.l	DTP_Creator,CName
	dc.l	DTP_Check2,Chk
	dc.l	DTP_Interrupt,pt_play
	dc.l	DTP_InitPlayer,InitPlay
	dc.l	DTP_EndPlayer,EndPlay
	dc.l	DTP_InitSound,InitSnd
	dc.l	DTP_EndSound,EndSnd
	dc.l	DTP_Flags,PLYF_SONGEND
	dc.l	DTP_FormatName,Formatpointer
	dc.l	DTP_SubSongRange,mod_SubSongRange
	dc.l	TAG_DONE
	
*-----------------------------------------------------------------------*
;
; Player/Creatorname und lokale Daten
MNamepointer	dc.l	Mname
Formatpointer	dc.l	Format
delibase	dc.l	0
song		dc.l	0
size		dc.l	0
pt_pckddata	dc.l	0
pt_pckdsize	dc.l	0


Format			dc.b	"type: "
FName			ds.b	31
			dc.b	0

PName			dc.b 'Protracker and family',0
CName			dc.b 'based on a Protracker 3.0b replay by Welder/Divine',10
			dc.b 'replay fixes deriving from EP 2.04 by Eagleeye',10
        		dc.b 'subsong detection based on work by Don Adan/Wanted Team',10
			dc.b 'PTCalcTime V1.1 by H. Pedersen',10,10
			dc.b 'adapted for UADE by mld and shd',10
			dc.b '----------------------------------------------',10
			dc.b 'note: filedetection checks for UADE!',0

MName:			ds.b 21
PTK_Name:		dc.b 'Protracker',0
PTK_Comp_Name:		dc.b 'Protracker compatible',0
PTK_vblank_Name:	dc.b 'Protracker (vblank)',0
NTK_2_Name:		dc.b 'Noisetracker 2.x',0
NTK_1_Name:		dc.b 'Noisetracker 1.x',0
NTK_AMP_Name:		dc.b 'Noisetracker (M&K!)',0
FLT4_Name:		dc.b 'Startrekker (4ch)',0
STK_Name:		dc.b 'Soundtracker II (31 instr.)',0
STK15_Name:		dc.b 'Soundtracker II (15 instr.)',0


cfgfile:		dc.b	'ENV:EaglePlayer/EP-PTK-Prowiz.cfg',0
			even
cfgbuffer:		ds.b	256

;--- Protracker types ---
PTK30 = 0
PTK23 = 1
PTK21 = 2
PTK10 = 3

PAL=0
NTSC=1

*-----------------------------------------------------------------------*
;
; Test Soundtracker-Module

Chk:
	; -- set some defaults for new Check
	; -- probl. breaks on Deliplayer :)
	; --
	move.l 	#0,pt_songposition
	move.w	#-1,pt_chkfail
	move.b	#0,pt_restart
	move.b	#0,pt_ntkporta
	move.b	#0,pt_vblank
	move.b	#0,pt_smpl_in_bytes
	move.l	#7,pt_vibshift
	move.w	#31-1,pt_noofinstr
	move.w	#0,pt_blkadj
	move.w	#0,pt_seqadj
	move.w	#36*2,pt_oldstk
	move.w	#36*2,pt_oldstk2
	move.l	dtg_ChkData(a5),a0		; get song data
	move.l	dtg_ChkSize(a5),d1
	move.l	a0,song
	move.l	d1,size

	bsr.w	query_eopts			; set eagleoptions

	; Check for Tracker clone and convert
	jsr	Convertmod
	tst.l	d0
	beq.w	is_clone

	; no Tracker clone
	move.b	pt_tracker_family,d0
	cmp.b	#mod_NTK_2,d0
	beq.w	is_NTK2
	cmp.b	#mod_NTK_1,d0
	beq.w	is_NTK1
	cmp.b	#mod_NTK_AMP,d0
	beq.w	is_NTK_AMP

	cmp.b	#mod_STK,d0
	beq.w	is_STK
	cmp.b	#mod_DOC,d0
	beq.w	is_STK15

	cmp.b	#mod_FLT4,d0
	beq.w	is_FLT4

	cmp.b	#mod_PTK_vblank,d0
	beq.w	is_PTK_vblank
	cmp.b	#mod_PTK,d0
	beq.w	is_PTK
	cmp.b	#mod_PTK_comp,d0
	beq.w	is_PTK_comp
	cmp.b	#mod_FTK,d0
	beq.w	is_PTK_comp

Chk_fail:	
	st	pt_chkfail	
	moveq	#-1,d0
	rts
Chk_ok:
	lea.l	FName,a2
	move.w	#30,d1
	bsr.w	strncpy

Chk_ok_MName:
	move.l	song,a1
	lea.l	MName(pc),a2	
	move.w	#20,d1
	bsr.w	strncpy

	;move.l	song,a0
	bsr.w	mod_probe_subsongs
	sf	pt_chkfail
	moveq	#0,d0
	rts

;-------------------------------------------------------------------------
;reads the ptk config file
;
query_eopts:
	movem.l	d0-d7/a0-a6,-(sp)
	move.b	#0,pt_tracker_family
	
	tst.l	uadebase
	bne	.uadelib_open
	jsr	open_uade_library
.uadelib_open
	jsr query_eagleopts		* get options from uade.conf

.ptktype
	tst.b	typenamereceived
	beq	.end
	lea	typestr,a0
	cmp.l	#"type",(a0)+
	bne	.end
	addq.l	#1,a0


.mod_STK15:			; DOC Soundtracker with 15 instruments
	cmp.l	#"st20",(a0)
	bne	.mod_NTK_1
	move.b	#mod_DOC,pt_tracker_family
	;jsr	uade_debug
	bra.w	.end

.mod_NTK_1
	cmp.l	#"nt10",(a0)
	bne	.mod_NTK_2
	move.b	#mod_NTK_1,pt_tracker_family
	;jsr	uade_debug
	bra.w	.end

.mod_NTK_2
	cmp.l	#"nt20",(a0)
	bne	.mod_NTK_AMP
	move.b	#mod_NTK_2,pt_tracker_family
	;jsr	uade_debug
	bra.w	.end

.mod_NTK_AMP
	cmp.l	#"m&k.",(a0)
	bne	.mod_STK
	move.b	#mod_NTK_AMP,pt_tracker_family
	;jsr	uade_debug
	bra.w	.end

.mod_STK
	cmp.l	#"st24",(a0)
	bne	.mod_FLT4
	move.b	#mod_STK,pt_tracker_family
	;jsr	uade_debug
	bra.w	.end

.mod_FLT4
	cmp.l	#"flt4",(a0)
	bne	.mod_ptk
	move.b	#mod_FLT4,pt_tracker_family
	;jsr	uade_debug
	bra.w	.end

.mod_ptk
	; check playtime anomalies
	; of protracker mods.
	move.b	#0,pt_vblank		; 0 = CIA
	bsr.w	mcheck_moduledata	; check for vblank
	move.b	d0,pt_tracker_family
	tst.b	vblankflag
	beq	.mod_comp
.VBI	addq.b	#1,pt_vblank		; 1 = VBI

.mod_comp	
	move.b	d0,pt_tracker_family
	cmp.l	#"comp",(a0)
	bne	.ptk23
	 move.b	#mod_PTK_comp,pt_tracker_family
	 move.b	#PTK30,pt_ptk_type
	 st	pt_ptk30_cme
	 ;jsr	uade_debug
	 bra.w	.end
	
.ptk23	cmp.l	#"pt23",(a0)
	bne	.ptkdef
	move.b	#PTK23,pt_ptk_type
	;jsr	uade_debug
	bra.w	.end

.ptkdef	cmp.l	#"hack",(a0)
	bne.s	.ptk30			; 3.0 Volume setting
	 move.b	#PTK30,pt_ptk_type
	 ;jsr	uade_debug
	 bra.w	.end

.ptk30	cmp.l	#"pt30",(a0)
	bne.s	.ptk10c
	 move.b	#PTK30,pt_ptk_type
	 st	pt_ptk30_cme
	 ;jsr	uade_debug
	 bra.w	.end

.ptk10c	cmp.l	#"pt10",(a0)
	bne.s	.ptk11b
	 move.b	#PTK10,pt_ptk_type
	 move.b	#6,pt_vibshift
	 move.w	#37*2,pt_oldstk		; apart from the different vibrato
	 move.w	#36*2,pt_oldstk2	; pt10c uses a mixed up value
	 ;jsr	uade_debug
	 bra.w 	.end			; for accessing the period table
.ptk11b:
	cmp.l	#"pt11",(a0)
	bne.s	.end
	 move.b	#PTK21,pt_ptk_type
	 move.w	#37*2,pt_oldstk		; mixed value accesing the period
	 move.w	#36*2,pt_oldstk2	; table...
	 ;jsr	uade_debug
.end
	movem.l	(sp)+,d0-d7/a0-a6
    	rts


;-------------------------------------------------------------------------
;defines and flags for the different modtypes
;
is_PTK:
	lea.l	PTK_Name,a1
	bra.w	Chk_ok

is_PTK_comp:
	lea.l	PTK_Comp_Name,a1
	bra.w	Chk_ok

is_PTK_vblank:
	st	pt_vblank
	lea.l	PTK_vblank_Name,a1
	bra.w	Chk_ok

is_FLT4:
	st	pt_ntkporta
	st	pt_vblank
	move.l	#7,pt_vibshift
	move.w	#36*2,pt_oldstk
	move.w	#36*2,pt_oldstk2
	lea.l	FLT4_Name,a1
	bra.w	Chk_ok

is_STK:
	st	pt_ntkporta
	st	pt_vblank
	move.l	#6,pt_vibshift
	move.w	#36*2,pt_oldstk
	move.w	#36*2,pt_oldstk2
	st 	pt_smpl_in_bytes
	lea.l	STK_Name,a1
	bra.w	Chk_ok

is_STK15:
	st	pt_ntkporta
	st	pt_vblank
	st	pt_smpl_in_bytes
	move.w	#15-1,pt_noofinstr
	move.l	#6,pt_vibshift
	move.w	#$1e0,pt_seqadj
	move.w	#$1e4,pt_blkadj
	move.w	#36*2,pt_oldstk
	lea.l	STK15_Name,a1
	bra.w	Chk_Ok
	

is_NTK1:
	move.b	#6,pt_vibshift
	lea.l	NTK_1_Name,a1
	bra.b	is_NTK

is_NTK_AMP:
	move.b	#5,pt_vibshift
	lea.l	NTK_AMP_Name,a1
	bra.b is_NTK

is_NTK2:
	lea.l	NTK_2_Name,a1

is_NTK:
	move.w	#36*2,pt_oldstk
	move.w	#36*2,pt_oldstk2
	move.b	950(a0),pt_restart
	st	pt_ntkporta
	st	pt_vblank
	bra.w	Chk_ok

is_Clone:
	move.l	pt_pckddata,a0
	move.l	pt_pckdsize,d0	

	;move.l	#"CONV",(a0)		; Debug Prowiz

	move.l	a0,song
	move.l	d0,size
	move.l	FormName,a1
	lea.l	FName,a2
	move.w	#30,d1
	bsr.b	strncpy
	bra.w	Chk_ok_Mname



*-----------------------------------------------------------------------*
; strncpy
;
; a1=src pointer (-1 Terminated)
; a2=dst pointer
; d1=sizeof dst

strncpy:
	moveq	#0,d2
	move.b	(a1)+,d2
	cmp.w	#159,d2
	bgt.b	.strn_putchar
	cmp.w	#127,d2
	bgt.b	.strn_nogoodchar
	cmp.w	#31,d2
	bgt.b	.strn_putchar

	cmp.w	#0,d2
	bne.b	.strn_nogoodchar
	bra.b	.strn_end

.strn_nogoodchar:
	move.b #".",d2
.strn_putchar:
	move.b	d2,(a2)+
	dbra	d1,strncpy
	move.b	#0,-1(a2)
.strn_end
	rts

*-----------------------------------------------------------------------*
;
; Init Player

InitPlay:
	move.l	a5,delibase
	moveq	#0,d0
	move.l	dtg_AudioAlloc(a5),a0		; Function
	jsr	(a0)				; returncode is already set !
	rts

*-----------------------------------------------------------------------*
;
; End Player

EndPlay
	move.l	dtg_AudioFree(a5),a0		; Function
	jsr	(a0)
	rts

	incdir	"amiga:work/players/tracker/common/"
	include "mod_check.s"
	include "ep_misc.s"

*-----------------------------------------------------------------------*
;
; InitSnd
InitSnd:
	bsr.b	pt_end
	bsr.b	pt_deinit

	move.l	4.w,a6
	cmp.b	#60,$212(a6)
	seq	pt_ntsc		; Set ntsc flag
	bsr.b	pt_init
	st	pt_Enable
	rts

*-----------------------------------------------------------------------*
;
; EndSnd
EndSnd:
	sf	pt_Enable
	bsr.b	pt_End
	rts


*-----------------------------------------------------------------------*
pt_deinit:
		move.b	#0,pt_metspd
		move.b	#0,pt_metrochannel
		move.b	#6,pt_speed
		move.b	#0,pt_songpos
		move.b	#0,pt_pbreakposition
		move.b	#0,pt_posjumpassert
		move.b	#0,pt_pbreakflag
		move.b  #0,pt_lowmask
		move.b	#0,pt_pattdelaytime
		move.b  #0,pt_pattdelaytime2
		move.b  #0,pt_Enable
		move.l	#0,pt_timervalue
		move.l  #0,pt_counter
		move.l  #6,pt_currspeed
		move.w	#0,pt_pattpos
		move.w	#0,pt_dmacontemp
		move.w	#%00001111,pt_activechannels
		move.l	#0,pt_patternptr
		move.l	#0,pt_patternposition
		move.l 	#0,pt_songdataptr


clear_uade_playtable:

		move.w   #129*64/8,d0
		subq	#1,d0
		lea.l	uade_playtable,a0
clearplayt:	move.b	#0,(a0)+
		dbra	d0,clearplayt

		rts

**************************************************
*	»» Protracker 3.00B playroutine ««	 *
*		By The Welder/Divine		 *
*						 *
* VBL version with CIA-Tempo handling		 *
* Call:	   pt_init - To initialise the module	 *
*	   pt_play - Every interrupt		 *
*	   pt_end  - To stop the music		 *
*	  »» Position Indepedent Code ««	 *
**************************************************


pt_init:
		movem.l	d0-d2/a0-a2,-(sp)
		move.l	song,a0
		lea	pt_songdataptr(pc),a1
		move.l	a0,(a1)		
		lea	952(a0),a1		; song pos for 31 Instr
		suba.w	pt_seqadj(pc),a1	; adjust for 15instr

pt_init1:	moveq	#128-1,d0
		moveq	#0,d1
		moveq	#0,d2
pt_lop2:
		move.b	(a1)+,d1
		cmp.b	d2,d1
		ble.b	pt_lop
		move.l	d1,d2
pt_lop:
		dbf	d0,pt_lop2

		addq.w	#1,d2
		asl.l	#8,d2
		asl.l	#2,d2
;		lea	4(a1,d2.l),a2

 		 add.l	#1084,d2
		 add.l	a0,d2
		 move.l	d2,a2

		suba.w	pt_blkadj(pc),a2
		lea	pt_samplestarts(pc),a1
		lea	42(a0),a0		; First sample length
		move.w	pt_noofinstr(pc),d0	; No of inst
pt_lop3:	cmp.w	#2,(a0)			; smpllen  <= 0
		bls.s	.Noe
		clr.l	(a2)			; Clear first words
.Noe:		move.l	a2,(a1)+
    		moveq	#0,d1

		move.w	(a0),d1
		add.l	d1,d1
		adda.l	d1,a2			; next sample

		tst.b	pt_smpl_in_bytes
		beq.b	repeat_ok
		lsr.w	4(a0)
repeat_ok:
		lea	30(a0),a0
		dbf	d0,pt_lop3
		lea	pt_speed(pc),a1
		move.b	#6,(a1)			; Default speed
		clr.b	pt_songpos-pt_speed(a1)
		clr.b	pt_counter-pt_speed(a1)
		clr.b	pt_pattpos-pt_speed(a1)

		tst.b	pt_vblank		; tst if "vblank" mod
		beq.s 	.nontsc			; if no  always default to PAL Speed

		tst.b	pt_ntsc			; set PAL/NTSC according to cfg
		beq.b	.nontsc
		move.l	#1491477,d0		;7.15 Mhz, 60hz, 125 bpm
		bra.b	.setcia

.nontsc		move.l	#1773447,d0		;7.09 Mhz, 50hz, 125 bpm
.setcia		move.l	d0,pt_timervalue-pt_speed(a1)
		divu	#125,d0
		bsr.w	ep_SetCIASpeed
		bsr.w	mod_Set_Subsong
		movem.l	(sp)+,d0-d2/a0-a2
		rts

	 ********************************


pt_end:
		movem.l	d0/a6,-(sp)
		moveq	#0,d0
		sf	pt_Enable
		lea	$dff000,a6
		move.w	d0,$a8(a6)
		move.w	d0,$b8(a6)
		move.w	d0,$c8(a6)
		move.w	d0,$d8(a6)
		move.w	#$f,$96(a6)		; Stop Audio DMA
		movem.l	(sp)+,d0/a6
		rts

	 ********************************

pt_play:
		movem.l	d0-d7/a0-a6,-(sp)
		tst.b	pt_chkfail
		bne.s	pt_play_exit
		tst.b	pt_Enable
		beq.b	pt_play_exit
		bsr.b	pt_playit
pt_play_exit		movem.l	(sp)+,d0-d7/a0-a6
		rts
pt_playit:
		lea	pt_metspd(pc),a4
		addq.l	#1,pt_counter-pt_metspd(a4)
		move.l	pt_songdataptr(pc),a0
		move.l	pt_counter(pc),d0
		cmp.l	pt_currspeed(pc),d0
		blo.b	pt_nonewnote
		clr.l	pt_counter-pt_metspd(a4)
		tst.b	pt_pattdelaytime2-pt_metspd(a4)
		beq.b	pt_getnewnote
		bsr.b	pt_nonewallchannels
		bra.b	pt_dskip
pt_nonewnote:
		bsr.b	pt_nonewallchannels
		bra.b	pt_nonewpositionyet
pt_nonewallchannels:
		lea	pt_audchan1temp(pc),a6
		lea	$dff0a0,a5
		bsr.b	pt_checkeffects
		lea	pt_audchan2temp(pc),a6
		lea	$dff0b0,a5
		bsr.b	pt_checkeffects
		lea	pt_audchan3temp(pc),a6
		lea	$dff0c0,a5
		bsr.b	pt_checkeffects
		lea	pt_audchan4temp(pc),a6
		lea	$dff0d0,a5
		bra.b	pt_checkeffects
pt_getnewnote:
		lea	12(a0),a3
		lea	952(a0),a2			;pattern position
		suba.w	pt_seqadj(pc),a2
		lea	1084(a0),a0			;patterndata
		suba.w	pt_blkadj,a0
		
		moveq	#0,d1
		move.l	pt_songposition(pc),d0
		move.b	(a2,d0.w),d1

* uade patch starts
                 movem.l         d0-d1/a0-a1,-(a7)
                 lea     uade_playtable,a0
                 move.l  pt_songposition(pc),d1
                 lsl     #3,d1
                 add     d1,a0
                 move.l  pt_patternposition(pc),d1
                 lsr.l   #4,d1
                 move.l  d1,d0
                 lsr     #3,d0
                 add     d0,a0
                 moveq   #7,d0
                 sub     d1,d0
                 * no song end test if a loop takes place on the region with
                 * loop counter > 0
                 tst.b   pt_audchan1temp+34
                 bne.b   notvisitedbefore
                 tst.b   pt_audchan2temp+34
                 bne.b   notvisitedbefore
                 tst.b   pt_audchan3temp+34
                 bne.b   notvisitedbefore
                 tst.b   pt_audchan4temp+34
                 bne.b   notvisitedbefore
                 btst    d0,(a0)
                 beq.b   notvisitedbefore
                 movem.l         d0-d7/a0-a6,-(sp)
                 move.l  delibase(pc),a5
                 move.l  dtg_SongEnd(a5),a1
                 jsr     (a1)
                 movem.l         (sp)+,d0-d7/a0-a6
notvisitedbefore
                 bset    d0,(a0)
                 movem.l         (a7)+,d0-d1/a0-a1

		asl.l	#8,d1
		asl.l	#2,d1
		add.l	pt_patternposition(pc),d1
		move.l	d1,pt_patternptr-pt_metspd(a4)
		clr.w	pt_dmacontemp-pt_metspd(a4)
		cmp.b	#PTK30,pt_ptk_type
		bne.s	.pt23_playvoices
.pt30_playvoices
		lea	$dff0a0,a5
		lea	pt_audchan1temp(pc),a6
		moveq	#1,d2
		bsr.b	pt_playvoice
		moveq	#0,d0
		move.b	19(a6),d0
		move.w	d0,8(a5)
		lea	$dff0b0,a5
		lea	pt_audchan2temp(pc),a6
		moveq	#2,d2
		bsr.b	pt_playvoice
		moveq	#0,d0
		move.b	19(a6),d0
		move.w	d0,8(a5)
		lea	$dff0c0,a5
		lea	pt_audchan3temp(pc),a6
		moveq	#3,d2
		bsr.b	pt_playvoice
		moveq	#0,d0
		move.b	19(a6),d0
		move.w	d0,8(a5)
		lea	$dff0d0,a5
		lea	pt_audchan4temp(pc),a6
		moveq	#4,d2
		bsr.b	pt_playvoice
		moveq	#0,d0
		move.b	19(a6),d0
		move.w	d0,8(a5)
		bra.b	pt_setdma
.pt23_playvoices
		lea	$dff0a0,a5
		lea	pt_audchan1temp(pc),a6
		bsr.b	pt_playvoice
		lea	$dff0b0,a5
		lea	pt_audchan2temp(pc),a6
		bsr.b	pt_playvoice
		lea	$dff0c0,a5
		lea	pt_audchan3temp(pc),a6
		bsr.b	pt_playvoice
		lea	$dff0d0,a5
		lea	pt_audchan4temp(pc),a6
		bsr.b	pt_playvoice
		bra.b	pt_setdma

pt_checkmetronome:
		cmp.b	pt_metrochannel(pc),d2
		bne.b	pt_quit
		move.b	pt_metspd(pc),d2
		beq.b	pt_quit
		move.l	pt_patternposition(pc),d3
		lsr.l	#4,d3
		divu	d2,d3
		swap d3
		tst.w	d3
		bne.b	pt_quit
		andi.l	#$00000fff,(a6)
		ori.l	#$10d6f000,(a6)		; Play sample $1f period $0d6
pt_quit:
		rts
pt_playvoice:
		tst.l	(a6)
		bne.b	pt_plvskip
		move.w	16(a6),6(a5)
pt_plvskip:
		move.l	(a0,d1.l),(a6)		; Read one track from pattern
		bsr.b	pt_checkmetronome
		addq.l	#4,d1
		moveq	#0,d2
		move.b	2(a6),d2		; Get lower bits of instrument
		andi.b	#$f0,d2
		lsr.b	#4,d2
		move.b	(a6),d0			; Get higher bits of instrument
		andi.b	#$f0,d0
		or.b	d0,d2
		tst.b	d2
		beq.b	pt_setregisters
		
		 cmp.b	#$1f,d2			; check for buggy mods
		 bhi.b	pt_gone
		
		moveq	#0,d3
		lea	pt_samplestarts(pc),a1
		move.w	d2,d4
		cmp.b	#PTK30,pt_ptk_type
		bne.b	.ptk23
		 move.b	d2,43(a6)		; ptk30 sets n_samplenum
.ptk23		subq.l	#1,d2
		lsl.l	#2,d2
		mulu	#30,d4
		
		 ;--- from EP 2.04 ---
		 ; empty Instrument handling
		 
		 tst.l	0(a1,d2.l)
		 bne.s	.yosam
		 clr.w	(a6)
		 move.b	2(a6),d0
		 and.w	#$f,d0
		 move.b	d0,2(a6)
		 clr.l	4(a6)
		 clr.w	8(a6)
		 clr.w	$28(a6)
		 clr.w	$12(a6)
		 clr.l	10(a6)
		 clr.l	$24(a6)
		 clr.w	14(a6)
		 clr.b	$1f(a6)
		 clr.b	43(a6)
		 ;----------------------
		 
.yosam		move.l	(a1,d2.l),4(a6)
		move.w	(a3,d4.l),8(a6)
		move.w	(a3,d4.l),40(a6)
		move.b	2(a3,d4.l),18(a6)
		move.b	3(a3,d4.l),19(a6)
		move.w	4(a3,d4.l),d3		; Get repeat
		tst.w	d3
		beq.b	pt_noloop
		move.l	4(a6),d2		; Get start
		add.w	d3,d3
		add.l	d3,d2			; Add repeat
		move.l	d2,10(a6)
		move.l	d2,36(a6)
		move.w	4(a3,d4.l),d0		; Get repeat
		 tst.w	6(a3,d4.l)		; Test repeat length is zero
		 bne.b	rplen_ok
		 add.w	#2,d0			; set std replen for mods
rplen_ok	add.w	6(a3,d4.l),d0		; Add repeat length
		move.w	d0,8(a6)
		move.w	6(a3,d4.l),14(a6)	; Save replen

		cmp.b	#PTK30,pt_ptk_type
		beq.s	pt_setregisters
		 moveq	#0,d0			; ptk23 sets volume here
		 move.b	19(a6),d0
		 move.w	d0,8(a5)
		bra.b	pt_setregisters

pt_noloop:
		move.l	4(a6),d2
		add.l	d3,d2
		move.l	d2,10(a6)
		move.l	d2,36(a6)
		move.w	6(a3,d4.l),14(a6)	; Save repeat length
		 tst.w	14(a6)			; repeat length zero?
		 bne.b	.ptk30vol		; nope
		 move.w	#2,14(a6)		; otherwise set it to std
.ptk30vol	cmp.b	#PTK30,pt_ptk_type
		beq.s	pt_setregisters
		 moveq	#0,d0			; ptk23 sets volume here
		 move.b	19(a6),d0
		 move.w	d0,8(a5)
pt_setregisters:
		move.w	(a6),d0
		andi.w	#$0fff,d0
		beq.b	pt_checkmoreeffects
		move.w	2(a6),d0
		andi.w	#$0ff0,d0
		cmpi.w	#$0e50,d0		; Finetune ?
		beq.b	pt_dosetfinetune
		move.b	2(a6),d0
		andi.b	#15,d0
		cmpi.b	#3,d0			; Toneportamento ?
		beq.b	pt_chktoneporta
		cmpi.b	#5,d0			; Toneportamento + volslide ?
		beq.b	pt_chktoneporta
		cmpi.b	#9,d0			; Sample offset ?
		bne.b	pt_setperiod
		bsr.b	pt_checkmoreeffects
		bra.b	pt_setperiod
pt_dosetfinetune:
		bsr.b	pt_setfinetune
		bra.b	pt_setperiod
pt_chktoneporta:
		bsr.b	pt_settoneporta
		bra.b	pt_checkmoreeffects
pt_setperiod:
		;MOVEM.L	D0-D6/A0-A1,-(sp)
		move.w	(a6),d6
		andi.w	#$0fff,d6
		lea	pt_periodtable(pc),a1
		moveq	#0,d0
		moveq	#36,d7
pt_ftuloop:
		cmp.w	(a1,d0.w),d6
		bhs.b	pt_ftufound
		addq.l	#2,d0
		dbf	d7,pt_ftuloop
pt_ftufound:
		moveq	#0,d6
		move.b	18(a6),d6
		mulu	pt_oldstk2,d6
		adda.l	d6,a1
		move.w	(a1,d0.w),16(a6)
		;MOVEM.L	(sp)+,D0-D6/A0-A1

		move.w	2(a6),d0
		andi.w	#$0ff0,d0
		cmpi.w	#$0ed0,d0
		beq.b	pt_checkmoreeffects
		move.w	20(a6),$dff096
		btst	#2,30(a6)
		bne.b	pt_vibnoc
		clr.b	27(a6)
pt_vibnoc:
		btst	#6,30(a6)
		bne.b	pt_trenoc
		clr.b	29(a6)
pt_trenoc:
		tst.b	pt_ptk30_cme
		bne.s	.pt30_trenoc
		; pt2.3 style
		move.w	8(a6),4(a5)		; Set length
		move.l	4(a6),(a5)		; Set start
		move.w	16(a6),d0
		move.w	d0,6(a5)		; Set period
		;st.b	42(a6)
		move.w	20(a6),d0
		or.w	d0,pt_dmacontemp-pt_metspd(a4)
		bra.b	pt_checkmoreeffects

.pt30_trenoc:
		move.w	8(a6),4(a5)		; Set length
		move.l	4(a6),(a5)		; Set start
		bne.b	pt_sdmaskp
		clr.l	10(a6)
		moveq	#1,d0
		move.w	d0,4(a5)
		move.w	d0,14(a6)
pt_sdmaskp:
		move.w	16(a6),d0
		move.w	d0,6(a5)		; Set period
		;st.b	42(a6)
		move.w	20(a6),d0
		or.w	d0,pt_dmacontemp-pt_metspd(a4)
		bra.b	pt_checkmoreeffects
pt_setdma:
		bsr.b	pt_raster
		move.w	pt_dmacontemp(pc),d0
		and.w	pt_activechannels(pc),d0
		ori.w	#$8000,d0
		lea	$dff000,a5
		move.w	d0,$96(a5)
		bsr.b	pt_raster
		lea	pt_audchan1temp(pc),a6
		move.l	10(a6),$a0(a5)
		move.w	14(a6),$a4(a5)
		move.l	54(a6),$b0(a5)
		move.w	58(a6),$b4(a5)
		move.l	98(a6),$c0(a5)
		move.w	102(a6),$c4(a5)
		move.l	142(a6),$d0(a5)
		move.w	146(a6),$d4(a5)
pt_dskip:
		addi.l	#16,pt_patternposition-pt_metspd(a4)
		move.b	pt_pattdelaytime(pc),d0
		beq.b	pt_dskpc
		move.b	d0,pt_pattdelaytime2-pt_metspd(a4)
		clr.b	pt_pattdelaytime-pt_metspd(a4)
pt_dskpc:
		tst.b	pt_pattdelaytime2-pt_metspd(a4)
		beq.b	pt_dskpa
		subq.b	#1,pt_pattdelaytime2-pt_metspd(a4)
		beq.b	pt_dskpa
		subi.l	#16,pt_patternposition-pt_metspd(a4)
pt_dskpa:
		tst.b	pt_pbreakflag-pt_metspd(a4)
		beq.b	pt_nnpysk
		clr.b	pt_pbreakflag-pt_metspd(a4)
		moveq	#0,d0
		move.b	pt_pbreakposition(pc),d0
		lsl.w	#4,d0
		move.l	d0,pt_patternposition-pt_metspd(a4)
		clr.b	pt_pbreakposition-pt_metspd(a4)
pt_nnpysk:
		cmpi.l	#1024,pt_patternposition-pt_metspd(a4)
		bne.b	pt_nonewpositionyet
pt_nextposition:
		moveq	#0,d0
		move.b	pt_pbreakposition(pc),d0
		lsl.w	#4,d0
		move.l	d0,pt_patternposition-pt_metspd(a4)
		clr.b	pt_pbreakposition-pt_metspd(a4)
		clr.b	pt_posjumpassert-pt_metspd(a4)
		addq.l	#1,pt_songposition-pt_metspd(a4)
		andi.l	#127,pt_songposition-pt_metspd(a4)
		move.l	pt_songposition(pc),d1
		move.l	pt_songdataptr(pc),a0
		suba.w	pt_seqadj(pc),a0
		cmp.b	950(a0),d1
		blo.b	pt_nonewpositionyet

		clr.l	pt_songposition-pt_metspd(a4)
		tst.b	pt_restart-pt_metspd(a4)
		beq.b	pt_nonewpositionyet		;SongEnd
		move.b	pt_restart(pc),pt_songposition-pt_metspd(a4)
pt_nonewpositionyet:
		tst.b	pt_posjumpassert-pt_metspd(a4)
		bne.b	pt_nextposition
		rts
pt_checkeffects:
		bsr.b	pt_chkefx2
		cmp.b	#PTK30,pt_ptk_type		; tst if ptk 1.0/2.3 or 3.0
		bne.s 	pt_gone
		 moveq	#0,d0
		 move.b	19(a6),d0
		 move.w	d0,8(a5)
pt_gone:	rts
pt_chkefx2:
		bsr.b	pt_updatefunk
		move.w	2(a6),d0
		andi.w	#$0fff,d0
		beq.b	pt_pernop
		move.b	2(a6),d0
		andi.b	#15,d0
		tst.b	d0
		beq.b	pt_arpeggio
		cmpi.b	#1,d0
		beq.b	pt_portaup
		cmpi.b	#2,d0
		beq.b	pt_portadown
		cmpi.b	#3,d0
		beq.b	pt_toneportamento
		cmpi.b	#4,d0
		beq.b	pt_vibrato
		cmpi.b	#5,d0
		beq.b	pt_toneplusvolslide
		cmpi.b	#6,d0
		beq.b	pt_vibratoplusvolslide
		cmpi.b	#14,d0
		beq.b	pt_ecommands
pt_setback:
		move.w	16(a6),6(a5)
		cmpi.b	#7,d0
		beq.b	pt_tremolo
		cmpi.b	#10,d0
		beq.b	pt_volumeslide
pt_return:
		rts
		
pt_pernop:	tst.b	pt_ptk30_cme
		bne.s 	.ptk3	
		move.w 16(a6),6(a5)
.ptk3		rts

pt_arpeggio:
		moveq	#0,d0
		move.l	pt_counter(pc),d0
		divs	#3,d0
		swap d0
		cmpi.w	#1,d0
		beq.b	pt_arpeggio1
		cmpi.w	#2,d0
		beq.b	pt_arpeggio2
pt_arpeggio0:
		move.w	16(a6),d2
		bra.b	pt_arpeggioset
pt_arpeggio1:
		moveq	#0,d0
		move.b	3(a6),d0
		lsr.b	#4,d0
		bra.b	pt_arpeggiofind
pt_arpeggio2:
		moveq	#0,d0
		move.b	3(a6),d0
		andi.b	#15,d0
pt_arpeggiofind:
		add.w	d0,d0
		moveq	#0,d1
		move.b	18(a6),d1
		mulu	pt_oldstk2,d1
		lea	pt_periodtable(pc),a0
		adda.l	d1,a0
		moveq	#0,d1
		move.w	16(a6),d1
		moveq	#36,d7
pt_arploop:
		move.w	(a0,d0.w),d2
		;cmp.w	(a0)+,d1
		cmp.w	(a0),d1
		bhs.b	pt_arpeggioset
		addq.l	#2,a0
		dbf	d7,pt_arploop
		rts
pt_arpeggioset:
		move.w	d2,6(a5)
		rts
pt_fineportaup:
		tst.l	pt_counter-pt_metspd(a4)
		bne.b	pt_leave
		move.b	#15,pt_lowmask-pt_metspd(a4)
pt_portaup:
		moveq	#0,d0
		move.b	3(a6),d0
		tst.b	pt_ntkporta			;old portamento
		bne.s	nonewportaup	
		and.b	pt_lowmask(pc),d0
		move.b	#$FF,pt_lowmask-pt_metspd(a4)
nonewportaup:	sub.w	d0,16(a6)
		move.w	16(a6),d0
		andi.w	#$0fff,d0
		cmpi.w	#113,d0
		bpl.b	pt_portauskip
		andi.w	#$f000,16(a6)
		ori.w	#113,16(a6)
pt_portauskip:
		move.w	16(a6),d0
		andi.w	#$0fff,d0
		move.w	d0,6(a5)
pt_leave:
		rts
pt_fineportadown:
		tst.l	pt_counter-pt_metspd(a4)
		bne.b	pt_leave
		move.b	#15,pt_lowmask-pt_metspd(a4)
pt_portadown:
		clr.w	d0
		move.b	3(a6),d0
		tst.b	pt_ntkporta			;old portamento
		bne.s	nonewportadown	
		and.b	pt_lowmask(pc),d0
		move.b	#$ff,pt_lowmask-pt_metspd(a4)
nonewportadown:	add.w	d0,16(a6)
		move.w	16(a6),d0
		andi.w	#$0fff,d0
		cmpi.w	#$0358,d0
		bmi.b	pt_portadskip
		andi.w	#$f000,16(a6)
		ori.w	#$0358,16(a6)
pt_portadskip:
		move.w	16(a6),d0
		andi.w	#$0fff,d0
		move.w	d0,6(a5)
		rts
pt_settoneporta:
		;MOVE.L	A0,-(SP)
		move.w	(a6),d2
		andi.w	#$0fff,d2
		moveq	#0,d0
		move.b	18(a6),d0
		mulu	pt_oldstk,d0
		lea	pt_periodtable(pc),a1
		adda.l	d0,a1
		moveq	#0,d0
pt_stploop:
		cmp.w	(a1,d0.w),d2
		bhs.b	pt_stpfound
		addq.w	#2,d0
		cmp.w	pt_oldstk,d0
		blo.b	pt_stploop
		moveq	#35*2,d0
pt_stpfound:
		move.b	18(a6),d2
		andi.b	#8,d2
		beq.b	pt_stpgoss
		tst.w	d0
		beq.b	pt_stpgoss
		subq.w	#2,d0
pt_stpgoss:
		move.w	(a1,d0.w),d2
		;MOVE.L	(SP)+,A0
		move.w	d2,24(a6)
		move.w	16(a6),d0
		clr.b	22(a6)
		cmp.w	d0,d2
		beq.b	pt_cleartoneporta
		bge.b	pt_skip
		move.b	#1,22(a6)
pt_skip:
		rts
pt_cleartoneporta:
		clr.w	24(a6)
		rts
pt_toneportamento:
		move.b	3(a6),d0
		beq.b	pt_toneportnochange
		move.b	d0,23(a6)
		clr.b	3(a6)
pt_toneportnochange:
		tst.w	24(a6)
		beq.b	pt_skip
		moveq	#0,d0
		move.b	23(a6),d0
		tst.b	22(a6)
		bne.b	pt_toneportaup
pt_toneportadown:
		add.w	d0,16(a6)
		move.w	24(a6),d0
		cmp.w	16(a6),d0
		bgt.b	pt_toneportasetper
		move.w	24(a6),16(a6)
		clr.w	24(a6)
		bra.b	pt_toneportasetper
pt_toneportaup:
		sub.w	d0,16(a6)
		move.w	24(a6),d0
		cmp.w	16(a6),d0
		blt.b	pt_toneportasetper
		move.w	24(a6),16(a6)
		clr.w	24(a6)
pt_toneportasetper:
		move.w	16(a6),d2
		move.b	31(a6),d0
		andi.b	#15,d0
		beq.b	pt_glissskip
		moveq	#0,d0
		move.b	18(a6),d0
		mulu	pt_oldstk2,d0
		lea	pt_periodtable(pc),a0
		adda.l	d0,a0
		moveq	#0,d0
pt_glissloop:
		cmp.w	(a0,d0.w),d2
		bhs.b	pt_glissfound
		addq.w	#2,d0
		cmp.w	pt_oldstk2,d0
		blo.b	pt_glissloop
		moveq	#35*2,d0
pt_glissfound:
		move.w	(a0,d0.w),d2
pt_glissskip:
		move.w	d2,6(a5) 		; Set period
		rts
pt_vibrato:
		move.b	3(a6),d0
		beq.b	pt_vibrato2
		move.b	26(a6),d2
		andi.b	#15,d0
		beq.b	pt_vibskip
		andi.b	#$f0,d2
		or.b	d0,d2
pt_vibskip:
		move.b	3(a6),d0
		andi.b	#$f0,d0
		beq.b	pt_vibskip2
		andi.b	#15,d2
		or.b	d0,d2
pt_vibskip2:
		move.b	d2,26(a6)
pt_vibrato2:
		move.b	27(a6),d0
		lea	pt_vibratotable(pc),a3
		lsr.w	#2,d0
		andi.w	#31,d0
		moveq	#0,d2
		move.b	30(a6),d2
		andi.b	#3,d2
		beq.b	pt_vib_sine
		lsl.b	#3,d0
		cmpi.b	#1,d2
		beq.b	pt_vib_rampdown
		move.b	#$ff,d2
		bra.b	pt_vib_set
pt_vib_rampdown:
		tst.b	27(a6)
		bpl.b	pt_vib_rampdown2
		move.b	#$ff,d2
		sub.b	d0,d2
		bra.b	pt_vib_set
pt_vib_rampdown2:
		move.b	d0,d2
		bra.b	pt_vib_set
pt_vib_sine:
		move.b	(a3,d0.w),d2
pt_vib_set:
		move.b	26(a6),d0
		andi.w	#15,d0
		mulu	d0,d2
		
		move.l	pt_vibshift,d0
		lsr.w	d0,d2			;6 for NTK; 7 for PTK

		move.w	16(a6),d0
		tst.b	27(a6)
		bmi.b	pt_vibratoneg
		add.w	d2,d0
		bra.b	pt_vibrato3
pt_vibratoneg:
		sub.w	d2,d0
pt_vibrato3:
		move.w	d0,6(a5)
		move.b	26(a6),d0
		lsr.w	#2,d0
		andi.w	#60,d0
		add.b	d0,27(a6)
		rts
pt_toneplusvolslide:
		bsr.w	pt_toneportnochange
		bra.b	pt_volumeslide
pt_vibratoplusvolslide:
		bsr.b	pt_vibrato2
		bra.b	pt_volumeslide
pt_tremolo:
		move.b	3(a6),d0
		beq.b	pt_tremolo2
		move.b	28(a6),d2
		andi.b	#15,d0
		beq.b	pt_treskip
		andi.b	#$f0,d2
		or.b	d0,d2
pt_treskip:
		move.b	3(a6),d0
		andi.b	#$f0,d0
		beq.b	pt_treskip2
		andi.b	#15,d2
		or.b	d0,d2
pt_treskip2:
		move.b	d2,28(a6)
pt_tremolo2:
		move.b	29(a6),d0
		lea	pt_vibratotable(pc),a3
		lsr.w	#2,d0
		andi.w	#31,d0
		moveq	#0,d2
		move.b	30(a6),d2
		lsr.b	#4,d2
		andi.b	#3,d2
		beq.b	pt_tre_sine
		lsl.b	#3,d0
		cmpi.b	#1,d2
		beq.b	pt_tre_rampdown
		move.b	#$ff,d2
		bra.b	pt_tre_set
pt_tre_rampdown:
		tst.b	27(a6)
		bpl.b	pt_tre_rampdown2
		move.b	#$ff,d2
		sub.b	d0,d2
		bra.b	pt_tre_set
pt_tre_rampdown2:
		move.b	d0,d2
		bra.b	pt_tre_set
pt_tre_sine:
		move.b	(a3,d0.w),d2
pt_tre_set:
		move.b	28(a6),d0
		andi.w	#15,d0
		mulu	d0,d2
		lsr.w	#6,d2
		moveq	#0,d0
		move.b	19(a6),d0
		tst.b	29(a6)
		bmi.b	pt_tremoloneg
		add.w	d2,d0
		bra.b	pt_tremolo3
pt_tremoloneg:
		sub.w	d2,d0
pt_tremolo3:
		bpl.b	pt_tremoloskip
		clr.w	d0
pt_tremoloskip:
		cmpi.w	#64,d0
		bls.b	pt_tremolook
		moveq	#64,d0
pt_tremolook:
		move.w	d0,8(a5)
		move.b	28(a6),d0
		lsr.w	#2,d0
		andi.w	#60,d0
		add.b	d0,29(a6)
		addq.l	#4,sp		; missing in 2.3a
		rts
pt_sampleoffset:
		moveq	#0,d0
		move.b	3(a6),d0
		beq.b	pt_sononew
		move.b	d0,32(a6)
pt_sononew:
		move.b	32(a6),d0
		lsl.w	#7,d0
		cmp.w	8(a6),d0
		bge.b	pt_sofskip
		sub.w	d0,8(a6)
		add.w	d0,d0
		add.l	d0,4(a6)
		rts
pt_sofskip:
		move.w	#1,8(a6)
		rts
pt_volumeslide:
		moveq	#0,d0
		move.b	3(a6),d0
		lsr.b	#4,d0
		tst.b	d0
		beq.b	pt_volslidedown
pt_volslideup:
		add.b	d0,19(a6)
		cmpi.b	#64,19(a6)
		bmi.b	pt_vsuskip
		move.b	#64,19(a6)
pt_vsuskip:
		move.b	19(a6),d0
		cmp.b	#PTK30,pt_ptk_type		; tst if ptk 1.0/2.3 or 3.0
		beq.s 	.ptk3	
		move.w	d0,8(a5)
.ptk3		rts
pt_volslidedown:
		moveq	#0,d0
		move.b	3(a6),d0
		andi.b	#15,d0
pt_volslidedown2:
		sub.b	d0,19(a6)
		bpl.b	pt_vsdskip
		clr.b	19(a6)
pt_vsdskip:
		move.b	19(a6),d0
		cmp.b	#PTK30,pt_ptk_type		; tst if ptk 1.0/2.3 or 3.0
		beq.s 	.ptk3	
		move.w	d0,8(a5)
.ptk3		rts
pt_positionjump:
		moveq	#0,d0
		move.b	3(a6),d0
		subq.b	#1,d0
		move.l	d0,pt_songposition-pt_metspd(a4)
pt_pj2:
		clr.b	pt_pbreakposition-pt_metspd(a4)
		st.b	pt_posjumpassert-pt_metspd(a4)
		rts
pt_volumechange:
		moveq	#0,d0
		move.b	3(a6),d0
		cmpi.b	#64,d0
		bls.b	pt_volumeok
		moveq	#64,d0
pt_volumeok:
		move.b	d0,19(a6)
		cmp.b	#PTK30,pt_ptk_type		; tst if ptk 1.0/2.3 or 3.0
		beq.s 	.ptk3	
		move.w	d0,8(a5)
.ptk3		rts
pt_patternbreak:
		moveq	#0,d0
		move.b	3(a6),d0
		move.l	d0,d2
		lsr.b	#4,d0
		mulu	#10,d0
		andi.b	#15,d2
		add.b	d2,d0
		cmpi.b	#63,d0
		bhi.b	pt_pj2
		move.b	d0,pt_pbreakposition-pt_metspd(a4)
		st.b	pt_posjumpassert-pt_metspd(a4)
		rts
pt_setspeed:
		move.b	3(a6),d0
		andi.w	#$ff,d0
		beq.b	pt_speednull
		tst.b	pt_vblank		; tst if vblank playing
		bne.s 	ntkspeed	
ptkspeed	cmpi.w	#32,d0			; cia timing for ptk
		bhs.b	pt_settempo
		bra.b	pt_ssp1
ntkspeed	cmp.b	#$1f,d0			; vbi timing
		ble.s	pt_ssp1
		move.b	#$1f,d0
pt_ssp1:	clr.l	pt_counter-pt_metspd(a4)
		move.w	d0,pt_currspeed+2-pt_metspd(a4)
		rts
pt_speednull:
		rts
pt_settempo:
		move.l	pt_timervalue-pt_metspd(a4),d2
		and.w	#$ff,d0
		;cmp.w	#$ff,d0
		;beq	pt_settempoend		; for mod.coolspot
		divu.w	d0,d2

		move.w	d2,d0
		bsr.w	ep_SetCIASpeed
pt_settempoend	rts


pt_checkmoreeffects:
		tst.b	pt_ptk30_cme
		bne.s 	.ptk3
		 bsr.b	pt_updatefunk		; ptk 3.0 doesn't call it
.ptk3		move.b	2(a6),d0
		andi.b	#15,d0
		cmpi.b	#9,d0
		beq.w	pt_sampleoffset
		cmpi.b	#11,d0
		beq.w	pt_positionjump
		cmpi.b	#13,d0
		beq.w	pt_patternbreak
		cmpi.b	#14,d0
		beq.b	pt_ecommands
		cmpi.b	#15,d0
		beq.b	pt_setspeed
		cmpi.b	#12,d0
		beq.w	pt_volumechange

		cmp.b	#PTK10,pt_ptk_type	; Ptk 1.0c doesn't set period
		beq.s	.end			; here.
		move.w	16(a6),6(a5)
.end		rts

pt_ecommands:
		moveq	#0,d0
		move.b	3(a6),d0
		andi.b	#$f0,d0
		lsr.b	#3,d0
		move.w	pt_table2(pc,d0.w),d0
		jmp	pt_table2(pc,d0.w)

	 *********************************

pt_table2:
		dc.w	pt_filteronoff-pt_table2	; e0
		dc.w	pt_fineportaup-pt_table2	; e1
		dc.w	pt_fineportadown-pt_table2	; e2
		dc.w	pt_setglisscontrol-pt_table2	; e3
		dc.w	pt_setvibratocontrol-pt_table2	; e4
		dc.w	pt_setfinetune-pt_table2	; e5
		dc.w	pt_jumploop-pt_table2		; e6
		dc.w	pt_settremolocontrol-pt_table2	; e7
		dc.w	pt_karplusstrong-pt_table2
		dc.w	pt_retrignote-pt_table2		; e9
		dc.w	pt_volumefineup-pt_table2	; ea
		dc.w	pt_volumefinedown-pt_table2	; eb
		dc.w	pt_notecut-pt_table2		; ec
		dc.w	pt_notedelay-pt_table2		; ed
		dc.w	pt_patterndelay-pt_table2	; ee
		dc.w	pt_funkit-pt_table2		; ef

	 *********************************

pt_filteronoff:
		move.b	3(a6),d0
		andi.b	#1,d0
		add.b	d0,d0
		andi.b	#$fd,$bfe001
		or.b	d0,$bfe001
		rts
pt_setglisscontrol:
		move.b	3(a6),d0
		andi.b	#15,d0
		andi.b	#$f0,31(a6)
		or.b	d0,31(a6)
		rts
pt_setvibratocontrol:
		move.b	3(a6),d0
		andi.b	#15,d0
		andi.b	#$f0,30(a6)
		or.b	d0,30(a6)
		rts
pt_setfinetune:
		move.b	3(a6),d0
		andi.b	#15,d0
		move.b	d0,18(a6)
pt_left:
		rts
pt_jumploop:
		tst.l	pt_counter-pt_metspd(a4)
		bne.b	pt_left
		move.b	3(a6),d0
		andi.b	#15,d0
		beq.b	pt_setloop
		tst.b	34(a6)
		beq.b	pt_jumpcnt
		subq.b	#1,34(a6)
		beq.b	pt_left
pt_jmploop:
		move.b	33(a6),pt_pbreakposition-pt_metspd(a4)
		st.b	pt_pbreakflag-pt_metspd(a4)
		rts
pt_jumpcnt:
		move.b	d0,34(a6)
		bra.b	pt_jmploop
pt_setloop:
		move.l	pt_patternposition(pc),d0
		lsr.l	#4,d0
		andi.b	#63,d0
		move.b	d0,33(a6)
		rts
pt_settremolocontrol:
		move.b	3(a6),d0
		andi.b	#15,d0
		lsl.b	#4,d0
		andi.b	#15,30(a6)
		or.b	d0,30(a6)
		rts
pt_karplusstrong:
		movem.l	d1-d7/a0-a6,-(sp)
		tst.b	pt_ptk30_cme
		beq.s 	.no
		move.l	10(a6),d1
		beq.s	.no
		move.l	d1,a1
		move.l	a1,a3
		move.w	14(a6),d0
		add.w	d0,d0
		subq.w	#2,d0
		blt.b	.no
.pt_karplop:
		move.b	(a1),d6
		ext.w	d6
		move.b	1(a1),d7
		ext.w	d7
		add.w	d6,d7
		asr.w	#1,d7
		move.b	d7,(a1)+
		dbf	d0,.pt_karplop
		move.b	(a1),d6
		ext.w	d6
		move.b	(a3),d7
		ext.w	d7
		add.w	d6,d7
		asr.w	#1,d7
		move.b	d7,(a1)
.no		movem.l	(sp)+,d1-d7/a0-a6
		rts

pt_retrignote:
		moveq	#0,d0
		move.b	3(a6),d0
		andi.b	#15,d0
		beq.b	pt_rtnend
		move.l	pt_counter(pc),d7
		bne.b	pt_rtnskp
		move.w	(a6),d7
		andi.w	#$0fff,d7
		bne.b	pt_rtnend
		move.l	pt_counter(pc),d7
pt_rtnskp:
		divu	d0,d7
		swap d7
		tst.w	d7
		bne.b	pt_rtnend
pt_doretrg:
		move.w	20(a6),$dff096
		move.l	4(a6),(a5)
		move.w	8(a6),4(a5)
		cmp.b	#PTK30,pt_ptk_type
		bne.s	.ptk23
		 move.w	16(a6),6(a5)		; ptk30 sets period here
.ptk23		bsr.b	pt_raster
		move.w	20(a6),d0
		ori.w	#$8000,d0
		move.w	d0,$dff096
		bsr.b	pt_raster
		move.l	10(a6),(a5)
		move.l	14(a6),4(a5)
pt_rtnend:
		rts
pt_volumefineup:
		tst.l	pt_counter-pt_metspd(a4)
		bne.b	pt_rtnend
		moveq	#0,d0
		move.b	3(a6),d0
		andi.b	#15,d0
		bra.w	pt_volslideup
pt_volumefinedown:
		tst.l	pt_counter-pt_metspd(a4)
		bne.b	pt_rtnend
		moveq	#0,d0
		move.b	3(a6),d0
		andi.b	#15,d0
		bra.w	pt_volslidedown2
pt_notecut:
		moveq	#0,d0
		move.b	3(a6),d0
		andi.b	#15,d0
		cmp.l	pt_counter(pc),d0
		bne.b	pt_rtnend
		clr.b	19(a6)

		cmp.b	#PTK30,pt_ptk_type	
		beq.s 	.ptk3	
		 move.w	#0,8(a5)
.ptk3		rts

pt_notedelay:
		moveq	#0,d0
		move.b	3(a6),d0
		andi.b	#15,d0
		cmp.l	pt_counter(pc),d0
		bne.w	pt_return
		move.w	(a6),d0
		andi.w	#$0fff,d0
		beq.b	pt_rtnend
		bra.w	pt_doretrg
pt_patterndelay:
		tst.l	pt_counter-pt_metspd(a4)
		bne.b	pt_endit
		moveq	#0,d0
		move.b	3(a6),d0
		andi.b	#15,d0
		tst.b	pt_pattdelaytime2-pt_metspd(a4)
		bne.b	pt_endit
		addq.b	#1,d0
		move.b	d0,pt_pattdelaytime-pt_metspd(a4)
pt_endit:
		rts
pt_funkit:
		tst.l	pt_counter-pt_metspd(a4)
		bne.b	pt_endit
		move.b	3(a6),d0
		andi.b	#15,d0
		lsl.b	#4,d0
		andi.b	#15,31(a6)
		or.b	d0,31(a6)
		tst.b	d0
		beq.b	pt_endit
pt_updatefunk:
		cmp.b	#PTK10,pt_ptk_type		; Protracker 1.0c
		beq.s	mt_UpdateFunk

		moveq	#0,d0
		move.b	31(a6),d0
		lsr.b	#4,d0
		beq.b	pt_funkend
		lea	pt_funktable(pc),a1
		move.b	(a1,d0.w),d0
		add.b	d0,35(a6)
		btst	#7,35(a6)
		beq.b	pt_funkend
		clr.b	35(a6)
		move.l	10(a6),d0
		moveq	#0,d7
		move.w	14(a6),d7
		add.l	d7,d0
		add.l	d7,d0
		move.l	36(a6),a1
		addq.l	#1,a1
		cmp.l	d0,a1
		blo.b	pt_funkok
		move.l	10(a6),a1
pt_funkok:
		move.l	a1,36(a6)
		;-- EP2.04 --
		cmp.l	#0,a1
		beq.s	pt_funkend
		;------------		
		moveq	#-1,d0
		sub.b	(a1),d0
		move.b	d0,(a1)
pt_funkend:	rts

; Protracker 1.0c Funkrepeat
mt_UpdateFunk:
	MOVEM.L	A0/D1-D2,-(SP)
	MOVEQ	#0,D0
	MOVE.B	31(A6),D0
	LSR.B	#4,D0
	BEQ.s	mt_funkend
	LEA	pt_funktable(PC),A0
	MOVE.B	(A0,D0.W),D0
	ADD.B	D0,35(A6)
	BTST	#7,35(A6)
	BEQ.s	mt_funkend
	CLR.B	35(A6)

	MOVE.L	4(A6),D1
	MOVEQ	#0,D2
	MOVE.W	40(A6),D2
	LSL.W	#1,D2
	ADD.L	D2,D1
	MOVE.W	14(A6),D2
	LSL.L	#1,D2
	SUB.L	D2,D1

	MOVE.L	36(A6),D2
	MOVEQ	#0,D0
	MOVE.W	14(A6),D0
	LSL.L	#1,D0
	ADD.L	D0,D2
	CMP.L	D1,D2
	BLS.s	mt_funkok
	MOVE.L	10(A6),D2
mt_funkok:
	MOVE.L	D2,36(A6)
	MOVE.L	D2,(A5)
mt_funkend:
	MOVEM.L	(SP)+,A0/D1-D2
	rts
; Protracker 1.0c Funkrepeat

pt_raster:
		movem.l d0-d7/a0-a6,-(a7)
		move.l	delibase,a5
		move.l	dtg_WaitAudioDMA(a5),a0
		jsr	(a0)
		movem.l	(a7)+,d0-d7/a0-a6
		rts

		Even

	 ********************************

pt_funktable:
		dc.b	0,5,6,7,8,10,11,13,16,19,22,26,32,43,64,128
pt_vibratotable:
		dc.b	0,24,49,74,97,120,141,161
		dc.b	180,197,212,224,235,244,250,253
		dc.b	255,253,250,244,235,224,212,197
		dc.b	180,161,141,120,97,74,49,24
		Even

	 ********************************

pt_metspd:
		dc.b	0
pt_metrochannel:
		dc.b	0
pt_speed:
		dc.b	6
pt_songpos:
		dc.b	0
pt_pbreakposition:
		dc.b	0
pt_posjumpassert:
		dc.b	0
pt_pbreakflag:
		dc.b	0
pt_lowmask:
		dc.b	0
pt_pattdelaytime:
		dc.b	0
pt_pattdelaytime2:
		dc.b	0
pt_Enable:
		dc.b	0
*/ my vars /*

pt_chkfail		dc.b	-1

pt_restart		dc.b	0

pt_ntkporta		dc.b	0

pt_tracker_family	dc.b	0

pt_ptk_type		dc.b	PTK30	; default Type is PTK 3.0b

pt_ptk30_cme		dc.b	0

pt_ntsc			dc.b	PAL	; 0 for pal, 1 = ntsc

pt_vblank		dc.b	0

pt_smpl_in_bytes	dc.b	0



		Even
pt_vibshift		dc.l	7

pt_noofinstr		dc.w	30

pt_blkadj		dc.w	0

pt_seqadj		dc.w	0

pt_oldstk		dc.w	36*2

pt_oldstk2		dc.w	36*2

*/

pt_timervalue:
		dc.l	0
pt_counter:
		dc.l	0
pt_currspeed:
		dc.l	6
pt_pattpos:
		dc.w	0
pt_dmacontemp:
		dc.w	0
pt_activechannels:
		dc.w	%00001111
pt_patternptr:
		dc.l	0
pt_patternposition:
		dc.l	0
pt_songposition:
		dc.l	0
pt_songdataptr:
		dc.l	0

uade_playtable   dcb.b   129*64/8,0

	 ********************************

pt_periodtable:
; -> tuning 0
		dc.w	856,808,762,720,678,640,604,570,538,508,480,453
		dc.w	428,404,381,360,339,320,302,285,269,254,240,226
		dc.w	214,202,190,180,170,160,151,143,135,127,120,113

; -> tuning 1
		dc.w	850,802,757,715,674,637,601,567,535,505,477,450
		dc.w	425,401,379,357,337,318,300,284,268,253,239,225
		dc.w	213,201,189,179,169,159,150,142,134,126,119,113
; -> tuning 2
		dc.w	844,796,752,709,670,632,597,563,532,502,474,447
		dc.w	422,398,376,355,335,316,298,282,266,251,237,224
		dc.w	211,199,188,177,167,158,149,141,133,125,118,112
; -> tuning 3
		dc.w	838,791,746,704,665,628,592,559,528,498,470,444
		dc.w	419,395,373,352,332,314,296,280,264,249,235,222
		dc.w	209,198,187,176,166,157,148,140,132,125,118,111
; -> tuning 4
		dc.w	832,785,741,699,660,623,588,555,524,495,467,441
		dc.w	416,392,370,350,330,312,294,278,262,247,233,220
		dc.w	208,196,185,175,165,156,147,139,131,124,117,110
; -> tuning 5
		dc.w	826,779,736,694,655,619,584,551,520,491,463,437
		dc.w	413,390,368,347,328,309,292,276,260,245,232,219
		dc.w	206,195,184,174,164,155,146,138,130,123,116,109
; -> tuning 6
		dc.w	820,774,730,689,651,614,580,547,516,487,460,434
		dc.w	410,387,365,345,325,307,290,274,258,244,230,217
		dc.w	205,193,183,172,163,154,145,137,129,122,115,109
; -> tuning 7
		dc.w	814,768,725,684,646,610,575,543,513,484,457,431
		dc.w	407,384,363,342,323,305,288,272,256,242,228,216
		dc.w	204,192,181,171,161,152,144,136,128,121,114,108
; -> tuning -8
		dc.w	907,856,808,762,720,678,640,604,570,538,508,480
		dc.w	453,428,404,381,360,339,320,302,285,269,254,240
		dc.w	226,214,202,190,180,170,160,151,143,135,127,120
; -> tuning -7
		dc.w	900,850,802,757,715,675,636,601,567,535,505,477
		dc.w	450,425,401,379,357,337,318,300,284,268,253,238
		dc.w	225,212,200,189,179,169,159,150,142,134,126,119
; -> tuning -6
		dc.w	894,844,796,752,709,670,632,597,563,532,502,474
		dc.w	447,422,398,376,355,335,316,298,282,266,251,237
		dc.w	223,211,199,188,177,167,158,149,141,133,125,118
; -> tuning -5
		dc.w	887,838,791,746,704,665,628,592,559,528,498,470
		dc.w	444,419,395,373,352,332,314,296,280,264,249,235
		dc.w	222,209,198,187,176,166,157,148,140,132,125,118
; -> tuning -4
		dc.w	881,832,785,741,699,660,623,588,555,524,494,467
		dc.w	441,416,392,370,350,330,312,294,278,262,247,233
		dc.w	220,208,196,185,175,165,156,147,139,131,123,117
; -> tuning -3
		dc.w	875,826,779,736,694,655,619,584,551,520,491,463
		dc.w	437,413,390,368,347,328,309,292,276,260,245,232
		dc.w	219,206,195,184,174,164,155,146,138,130,123,116
; -> tuning -2
		dc.w	868,820,774,730,689,651,614,580,547,516,487,460
		dc.w	434,410,387,365,345,325,307,290,274,258,244,230
		dc.w	217,205,193,183,172,163,154,145,137,129,122,115
; -> tuning -1
		dc.w	862,814,768,725,684,646,610,575,543,513,484,457
		dc.w	431,407,384,363,342,323,305,288,272,256,242,228
		dc.w	216,203,192,181,171,161,152,144,136,128,121,114

	 ********************************

pt_audchan1temp:
		dc.l	0,0,0,0,0,$00010000,0,0,0,0,0
pt_audchan2temp:
		dc.l	0,0,0,0,0,$00020000,0,0,0,0,0
pt_audchan3temp:
		dc.l	0,0,0,0,0,$00040000,0,0,0,0,0
pt_audchan4temp:
		dc.l	0,0,0,0,0,$00080000,0,0,0,0,0
pt_samplestarts:
		dcb.l	31,0

	 ********************************


	incdir	"amiga:work/players/tracker/converter/"
	include	"Deli-Wizard.s"

