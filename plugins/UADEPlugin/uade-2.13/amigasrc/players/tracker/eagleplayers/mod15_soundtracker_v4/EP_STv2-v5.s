
	incdir	"Amiga:Includes/"
	include "misc/DeliPlayer.i"

;
;
	SECTION Player,Code
;
;

	PLAYERHEADER PlayerTagArray

	dc.b '$VER: Soundtracker IV player 2005-11-30',0
	even

PlayerTagArray
	dc.l	DTP_PlayerVersion,1
	dc.l	DTP_PlayerName,PName
	dc.l	DTP_Creator,CName
	dc.l	DTP_Check2,Chk
	dc.l	DTP_Interrupt,mt_newirq
	dc.l	DTP_InitPlayer,InitPlay
	dc.l	DTP_EndPlayer,EndPlay
	dc.l	DTP_InitSound,InitSnd
	dc.l	DTP_EndSound,RemSnd
	dc.l	DTP_Flags,PLYF_SONGEND
	dc.l	DTP_FormatName,Formatpointer
	dc.l	TAG_DONE
	
*-----------------------------------------------------------------------*
;
; Player/Creatorname und lokale Daten
Formatpointer	dc.l	FName
delibase	dc.l	0
song		dc.l	0

PName	dc.b 'Soundtracker IV',0
FName	dc.b 'Soundtracker II - V or compatible',0
CName	dc.b "'88 by DOC, Il Scuro, The Exterminator",10
	dc.b 'adapted for UADE by mld',10
	dc.b '-----------------------',10
	dc.b 'note: filedetection checks for uade!',0
	
	even


*-----------------------------------------------------------------------*
;
; Test Soundtracker-Module

Chk	
	move.l	dtg_ChkData(a5),a0
	move.l	dtg_ChkSize(a5),d1
	move.l	a0,mt_song
	bsr.w	mcheck_moduledata		; currently -1 for no mod
	cmp.l	#"M15.",d0
	bne	.Chk_fail
.Chk_ok: moveq	#0,d0
	rts

.Chk_fail:	
	moveq	#-1,d0
	rts


*-----------------------------------------------------------------------*
;
; Init Player

InitPlay
	move.l	a5,delibase
	moveq	#0,d0

	move.l	dtg_AudioAlloc(a5),a0		; Function
	jsr	(a0)				; returncode is already set !
	sf 	ep_songendflag
	rts

*-----------------------------------------------------------------------*
;
; End Player

EndPlay
	sf 	ep_songendflag
	move.l	dtg_AudioFree(a5),a0		; Function
	jsr	(a0)
	rts


*-----------------------------------------------------------------------*
;
; Remove Sound

RemSnd
	lea	$dff000,a0
	moveq	#0,d0
	move.w	d0,$a8(a0)
	move.w	d0,$b8(a0)
	move.w	d0,$c8(a0)
	move.w	d0,$d8(a0)
	move.w	#$000F,$96(a0)			; End Sound
	rts


*-----------------------------------------------------------------------*




***********************************
***********************************
**                               **
** SoundTracker V4.0 Playroutine **
**                               **
**   Coder 1 : Karsten Obarski   **
**                               **
**   Coder 2 : The Exterminator  **
**                               **
**   Coder 3 : Il Scuro          **
**                               **
**   Coder 4 : DOC               **
**                               **
***********************************
***********************************



InitSnd:
    move.l	mt_song,a0
    add.l	#$1d8,a0
    move.l	#$80,d0
    moveq	#$00,d1

mt_init1:
    move.l	d1,d2
    subq.w	#1,d0
mt_init2:
    move.b	(a0)+,d1
    cmp.b	d2,d1
    bgt.s	mt_init1
    dbf		d0,mt_init2
    addq.b	#1,d2

mt_init3:
    move.l	mt_song,a0
    lea		mt_sample1(pc),a1
    asl.l	#8,d2
    asl.l	#2,d2
    add.l	#$258,d2
    add.l	a0,d2
    moveq	#$0e,d0
mt_init4:
    move.l	d2,(a1)+
    move.l	d2,a2
    clr.w	(a2)
    moveq	#$00,d1
    move.w	42(a0),d1
    asl.l	#1,d1
    add.l	d1,d2
    add.l	#$1e,a0
    dbf		d0,mt_init4
  
    move.w	#0,$dff0a8
    move.w	#0,$dff0b8
    move.w	#0,$dff0c8
    move.w	#0,$dff0d8
    eor.w	#$c000,mt_voice0+12
    clr.l	mt_partnrplay
    clr.l	mt_partnote
    clr.l	mt_partpoint

    move.l	mt_song,a0
    move.b	$1d6(a0),mt_maxpart+1
    move.b	$1d7(a0),mt_kn1+1
    rts


mt_newirq:
    tst.b	ep_songendflag
    beq		.mt_replay
    bsr		ep_Songend
.mt_replay
    movem.l	d0-d7/a0-a6,-(a7)
    bsr		mt_music
    movem.l	(a7)+,d0-d7/a0-a6
    rts


mt_music:
    addq.l	#1,mt_counter
    move.l	mt_tempo,d0
    cmp.l	mt_counter,d0
    bne.s	mt_notsix
    clr.l	mt_counter
    bra.L	mt_rout2

mt_notsix:
    lea		mt_aud1temp,a6
    tst.b	3(a6)
    beq.s	mt_arp1
    move.l	#$dff0a0,a5		
    bsr.s	mt_arprout

mt_arp1:
    lea		mt_aud2temp,a6
    tst.b	3(a6)
    beq.s	mt_arp2
    move.l	#$dff0b0,a5
    bsr.s	mt_arprout

mt_arp2:
    lea		mt_aud3temp,a6
    tst.b	3(a6)
    beq.s	mt_arp3
    move.l	#$dff0c0,a5
    bsr.s	mt_arprout

mt_arp3:
    lea		mt_aud4temp,a6
    tst.b	3(a6)
    beq.s	mt_arp4
    move.l	#$dff0d0,a5
    bsr.s	mt_arprout
mt_arp4:
    bra.L	mt_stop


mt_arprout:
    tst.w	24(a6)
    beq.s	mt_noslide

    clr.w	d0
    move.b	25(a6),d0
    lsr.b	#4,d0
    tst.b	d0
    beq.s	mt_voldwn2
    bsr.L	mt_pushvol1
    bra.s	mt_noslide

mt_voldwn2:
    clr.w	d0
    move.b	25(a6),d0
    bsr.L	mt_pushvol2

mt_noslide:
    move.b	2(a6),d0
    and.b	#$0f,d0
    tst.b	d0
    beq.L	mt_arpegrt
    cmp.b	#3,d0
    beq.L	mt_arpegrt
    cmp.b	#4,d0
    beq.L	mt_arpegrt
    cmp.b	#5,d0
    beq.L	mt_arpegrt
    cmp.b	#1,d0
    beq.s	mt_portup
    cmp.b	#6,d0
    beq.s	mt_portup
    cmp.b	#7,d0
    beq.s	mt_portup
    cmp.b	#8,d0
    beq.s	mt_portup
    cmp.b	#2,d0
    beq.s	mt_portdwn
    cmp.b	#9,d0
    beq.s	mt_portdwn
    cmp.b	#10,d0
    beq.s	mt_portdwn
    cmp.b	#11,d0
    beq.s	mt_portdwn
    cmp.b	#13,d0
    beq.s	mt_volup
    rts

mt_portup:
    clr.w	d0
    move.b	3(a6),d0
    sub.w	d0,22(a6)
    cmp.w	#$71,22(a6)
    bpl.s	mt_ok1
    move.w	#$71,22(a6)

mt_ok1:
    move.w	22(a6),6(a5)
    rts

mt_portdwn:
    clr.w	d0
    move.b	3(a6),d0
    add.w	d0,22(a6)
    cmp.w	#$358,22(a6)
    bmi.s	mt_ok2
    move.w	#$358,22(a6)
mt_ok2:
    move.w	22(a6),6(a5)
    rts

mt_volup:
    clr.w	d0
    move.b	3(a6),d0
    lsr.b	#4,d0
    tst.b	d0
    beq.s	mt_voldwn
mt_pushvol1:
    add.w	d0,$12(a6)
    cmp.w	#$40,$12(a6)
    bmi.s	mt_ok3
    move.w	#$40,$12(a6)
mt_ok3:
    move.w	$12(a6),8(a5)
    rts

mt_voldwn:
    clr.w	d0
    move.b	3(a6),d0
mt_pushvol2:
    and.b	#$0f,d0
    sub.w	d0,$12(a6)
    bpl.s	mt_ok4
    clr.w	$12(a6)
mt_ok4:
    move.w	$12(a6),8(a5)
    rts

mt_arpegrt:
    cmp.l	#1,mt_counter
    beq.s	mt_loop2
    cmp.l	#2,mt_counter
    beq.s	mt_loop3
    cmp.l	#3,mt_counter
    beq.s	mt_loop4
    cmp.l	#4,mt_counter
    beq.s	mt_loop2
    cmp.l	#5,mt_counter
    beq.s	mt_loop3
    rts

mt_loop2:
    clr.l	d0
    move.b	3(a6),d0
    lsr.b	#4,d0
    bra.s	mt_cont

mt_loop3:
    clr.l	d0
    move.b	3(a6),d0
    and.b	#$0f,d0
    bra.s	mt_cont

mt_loop4:
    move.w	16(a6),d2
    bra.s	mt_endpart

mt_cont:
    lsl.w	#1,d0
    clr.l	d1
    move.w	16(a6),d1
    lea		mt_arpeggio,a0
mt_loop5:
    move.w	(a0,d0),d2
    cmp.w	(a0),d1
    beq.s	mt_endpart
    addq.l	#2,a0
    bra.s	mt_loop5

mt_endpart:
    move.w		d2,6(a5)
    rts

mt_rout2:
    move.l	mt_song,a0
    move.l	a0,a3
    add.l	#$0c,a3
    move.l	a0,a2
    add.l	#$1d8,a2
    add.l	#$258,a0
    move.l	mt_partnrplay,d0
    clr.l	d1
    move.b	(a2,d0),d1
    mulu	#$0400,d1
    add.l	mt_partnote,d1
    move.l	d1,mt_partpoint
    clr.w	mt_dmacon

    move.l	#$dff0a0,a5
    lea		mt_aud1temp,a6
    bsr.L	mt_playit

    move.l	#$dff0b0,a5
    lea		mt_aud2temp,a6
    bsr.L	mt_playit

    move.l	#$dff0c0,a5
    lea		mt_aud3temp,a6
    bsr.L	mt_playit

    move.l	#$dff0d0,a5
    lea		mt_aud4temp,a6
    bsr.L	mt_playit

mt_rls:
    movem.l	d0-d7/a0-a6,-(a7)
    move.l	delibase,a5
    move.l	dtg_WaitAudioDMA(a5),a0		; Function
    jsr	(a0)
    movem.l	(a7)+,d0-d7/a0-a6

    move.l	#$8000,d0
    add.w	mt_dmacon,d0
    move.w	d0,$dff096

    move.l	#mt_aud4temp,a6
    cmp.w	#1,14(a6)
    bne.s	mt_voice3
    move.l	10(a6),$dff0d0
    move.w	#1,$dff0d4
mt_voice3:
    move.l	#mt_aud3temp,a6
    cmp.w	#1,14(a6)
    bne.s	mt_voice2
    move.l	10(a6),$dff0c0
    move.w	#1,$dff0c4
mt_voice2:
    move.l	#mt_aud2temp,a6
    cmp.w	#1,14(a6)
    bne.s	mt_voice1
    move.l	10(a6),$dff0b0
    move.w	#1,$dff0b4
mt_voice1:
    move.l	#mt_aud1temp,a6
    cmp.w	#1,14(a6)
    bne.s	mt_voice0
    move.l	10(a6),$dff0a0
    move.w	#1,$dff0a4

mt_voice0:
    lea		mt_modulate,a0
    move.l	mt_partnote,d0
    lsl.b	#7,d0
    add.l	#$10,d0
    move.l	d0,mt_partnote
    cmp.l	#$400,d0
    bne.s	mt_stop
    clr.l	mt_partnote
    addq.l	#1,mt_partnrplay
    clr.l	d0
    move.w	mt_maxpart,d0
    move.l	mt_partnrplay,d1
    cmp.l	d0,d1
    bne.s	mt_stop
    st		ep_songendflag
    clr.l	mt_partnrplay
mt_stop:
    rts


mt_playit:
    move.l	(a0,d1),(a6)
    addq.l	#4,d1
    clr.l	d2
    move.b	2(a6),d2
    and.b	#$f0,d2
    lsr.b	#4,d2
    tst.b	d2
    beq.s	mt_nosamplechange

    clr.l	d3
    lea		mt_samples,a1
    move.l	d2,d4
    mulu	#4,d2
    mulu	#$1e,d4
    move.l	(a1,d2),4(a6)
    move.w	(a3,d4),8(a6)
    move.w	2(a3,d4),18(a6)
    move.w	4(a3,d4),d3
    tst.w	d3
    beq.s	mt_displace
    move.l	4(a6),d2
    add.l	d3,d2
    move.l	d2,4(a6)
    move.l	d2,10(a6)
    move.w	6(a3,d4),8(a6)
    move.w	6(a3,d4),14(a6)
    move.w	18(a6),8(a5)
    bra.s	mt_nosamplechange
mt_displace:
    move.l	4(a6),d2
    add.l	d3,d2
    move.l	d2,10(a6)
    move.w	6(a3,d4),14(a6)
    move.w	18(a6),8(a5)
mt_nosamplechange:
    tst.w	(a6)
    beq.s	mt_retrout
    move.w	(a6),16(a6)
    move.w	20(a6),$dff096
    move.l	4(a6),(a5)
    move.w	8(a6),4(a5)
    move.w	(a6),6(a5)
    move.w	20(a6),d0
    or.w	d0,mt_dmacon

mt_retrout:
    move.w	20(a6),d0
    lsl.w	#4,d0
    add.w	20(a6),d0
    move.w	d0,$dff09e

    tst.w	(a6)
    beq.s	mt_nonewper
    move.w	(a6),22(a6)
mt_nonewper:

    move.b	2(a6),d0
    and.b	#$0f,d0
    cmp.b	#14,d0
    beq.s	mt_zx1
    cmp.b	#15,d0
    bne.s	mt_noset

    move.w	2(a6),d0
    and.l	#$f,d0
    move.l	d0,mt_tempo
    rts

mt_zx1:
    move.w	2(a6),24(a6)
    rts

mt_noset:
    tst.b	3(a6)
    bne.s	mt_noclr
    clr.w	24(a6)
mt_noclr:
    cmp.b	#3,d0
    beq.s	mt_modvol
    cmp.b	#6,d0
    beq.s	mt_modvol
    cmp.b	#9,d0
    beq.s	mt_modvol
    cmp.b	#4,d0
    beq.s	mt_modper
    cmp.b	#7,d0
    beq.s	mt_modper
    cmp.b	#10,d0
    beq.s	mt_modper
    cmp.b	#5,d0
    beq.s	mt_modvolper
    cmp.b	#8,d0
    beq.s	mt_modvolper
    cmp.b	#11,d0
    beq.s	mt_modvolper
    cmp.b	#12,d0
    bne.s	mt_nochnge
    move.b	3(a6),8(a5)
mt_nochnge:
    rts

mt_modvol:
    move.w	20(a6),d0
    bra.s	mt_push

mt_modper:
    move.w	20(a6),d0
    lsl.w	#4,d0
    bra.s	mt_push

mt_modvolper:
    move.w	20(a6),d0
    lsl.w	#4,d0
    add.w	20(a6),d0

mt_push:
    add.w	#$8000,d0
    move.w	d0,$dff09e
    rts

	incdir "amiga:work/players/tracker/common/"
	include	"mod_check.s"
	include	"ep_misc.s"



mt_aud1temp:	dc.w $0000,$0000,$0000,$0000,$0000,$0000,$0000,$0000
		dc.w $0000,$0000,$0001,$0000,$0000
mt_aud2temp:	dc.w $0000,$0000,$0000,$0000,$0000,$0000,$0000,$0000
		dc.w $0000,$0000,$0002,$0000,$0000
mt_aud3temp:	dc.w $0000,$0000,$0000,$0000,$0000,$0000,$0000,$0000
		dc.w $0000,$0000,$0004,$0000,$0000
mt_aud4temp:	dc.w $0000,$0000,$0000,$0000,$0000,$0000,$0000,$0000
		dc.w $0000,$0000,$0008,$0000,$0000
mt_partnote: 	dc.l 0
mt_partnrplay:	dc.l 0
mt_counter:	dc.l 0
mt_tempo:	dc.l 6
mt_partpoint:	dc.l 0
mt_samples:	dc.w $0000,$0000
mt_sample1:	blk.l 15,0
mt_maxpart:	dc.w $0000
mt_kn1:		dc.w $0000
mt_dmacon:	dc.w $0000

mt_modulate:
		dc.w $0c39,$0039,$00bf,$ec01,$6630,$0839,$0007,$00bf
		dc.w $e001,$6626,$2c79,$0000,$0004,$43fa,$0020,$4eae
		dc.w $fe68,$2c40,$4280,$41fa,$0026,$223c,$0000,$0032
		dc.w $4eae,$ffa6,$60ee,$0000,$0000,$4e75,$696e,$7475
		dc.w $6974,$696f,$6e2e,$6c69,$6272,$6172,$7900,$0104
		dc.w $1753,$6f75,$6e64,$5472,$6163,$6b65,$7220,$5632
		dc.w $0063,$00f0,$20a9,$2054,$6865,$204a,$756e,$676c
		dc.w $6520,$436f,$6d6d,$616e,$6400,$0000

mt_arpeggio:
		dc.w $0358,$0328,$02fa,$02d0,$02a6,$0280,$025c
		dc.w $023a,$021a,$01fc,$01e0,$01c5,$01ac,$0194,$017d
		dc.w $0168,$0153,$0140,$012e,$011d,$010d,$00fe,$00f0
		dc.w $00e2,$00d6,$00ca,$00be,$00b4,$00aa,$00a0,$0097
		dc.w $008f,$0087,$007f,$0078,$0071,$0000,$0000,$0000

mt_song		dc.l	0
