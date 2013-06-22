	Section	Converter,Code


; v2.0: LAST DEBUGGING : 03-Apr-94.
; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;            Problems with ONE-PATTERN-MODULES in several formats.
;	     UNIC check-routine improved.
;	     UNIC improvement : $3b7 byte = Replay Position ! So, I have to
;                               insert a "Bxx" at the end of the last played
;                               pattern !  (UNIC.African_Dreams)
;	     P50A : Several problems fixed ! All seems to be OK now...


; v2.1: Since 18/09/94.....till 23/09/94 and some more in December ;-)
; ~~~~~~~~~~~~~~~~~~~~
;            NoisePacker 1 recognized from NoisePacker 2.
;            Check routines improved : Noise Runner, PP21/PP30,
;                                      ModProt, UNIC/LAX
;            Added all other formats recognized by Pro-Wizard 2.0 ! (16)
;            Added Tracker Packer 1-2-3 & The Player 4.xx (Jarno's routines
;                  were not so perfect !!! shit...  now I think I've fixed
;                  everything.... hope so !?)
;	     Added NoisePacker Compressed, PolkaPacker !
;            Added Power Music, SoundTracker Pro 3.0.
;            P60A modules with packed/delta samples are now recognized !
;
;       ===> 49 formats !


; v2.12 : 15-Jan-95          (Version numbers follow Pro-Wizard's ones :-))
; ~~~~~~~~~~~~~~~~~
;            PP21 check routine fixed.
;            Added ZenPacker by Dweezil/Stellar (Darkroom, Huivi...)
;
;       ===> 50 formats available !


; v2.15 : 05-Feb-95
; ~~~~~~~~~~~~~~~~~
;            Added  -Player v6.1a-  !! Whaouhh pattern packing method has
;                                      completely changed !
;                                      Stop this Jarnooooooooo ! ;-))
;       ===> 51 formats !!


; v2.20 : 10-Aug-95
; ~~~~~~~~~~~~~~~~~
;            Debugged : Soundtracker Pro 3.0, Player 4.xx, ST26, Pha Packer.
;            Added 'HornetPacker' (Alcatraz "Ilyad")

;       ===> 52 formats !




;	incdir	"Includes:"
	incdir	"Amiga:Includes/"
	include	"exec/exec.i"
;	include	"dos/dos.i"
;	include	"intuition/intuition.i"
;	include	"libraries/asl.i"
;	include	"libraries/gadtools.i"
;	include	"misc/DeliPlayer.i"
	include	"misc/DevpacMacros.i"             ; Thanx !

; Assembler: Genam 3.04
; Includes:  CBM V40.15
; Interesting code locations are marked with ';§'


 dc.b '$VER:Deli-Wizard genie module v2.20 by Gryzor (10-Aug-1995)',0
 even

*-----------------------------------------------------------------------*
;
; Genie/Creatorname/Description und lokale Daten

GName:	dc.b 'Deli-Wizard',0

GCName:	dc.b 'written by Nicolas FRANCK in 1994-95',10
	dc.b 'based on Pro-Wizard convert-routines.',0

GDName:	dc.b 'this genie converts various module',10
	dc.b 'formats to noise/protracker format.',0
	even



*-----------------------------------------------------------------------*
;
; Convert Module
;§
;This is the routine that does the Module-Conversion. DeliTracker supplies
;the pointer to the module in dtg_ChkData, the size in dtg_ChkSize (these
;values are read-only!).

Conv:	dc.w	0

ConvertMod:
	clr.l	FormName			; clear Format String

	moveq	#-1,d3				; default: module not converted
	clr.w	Conv

	lea	ConverterList,a3
	move.l	LH_HEAD(a3),a3
ConvertLoop:
	tst.l	LN_SUCC(a3)
	beq	ConvertEnd			; end of converter list !
	tst.b	LN_PRI(a3)
	beq	ConvertNext			; converter is disabled !

	move.l	song,a0
	move.l	size,d0

	move.l	LN_SIZE+0(a3),d2
	beq.s	ConvertNext			; no check routine available !
	move.l	d2,a2
	movem.l	d2-d4/a2-a4,-(sp)
	jsr	(a2)				; call check function
	movem.l	(sp)+,d2-d4/a2-a4
	tst.l	d0
	beq.s	ConvertNext			; unknown module format

	move.l	d0,d4
	move.l	d4,pt_pckdsize	
	move.l #MEMF_CHIP!MEMF_CLEAR,d1
	CALLEXEC AllocMem
	tst.l	d0
	beq.s	ConvertNext			; out of memory !
	move.l	d0,pt_pckddata
	
	
	move.l	d0,a4
	move.l	pt_pckdsize,d0
ClrMem:
	move.b	#0,(a4,d0.l)
	dbra	d0,ClrMem	


	move.l	song,a0
	move.l	size,d0

	move.l	pt_pckddata,a1
	move.l	pt_pckdsize,d1
	
	move.l	LN_SIZE+4(a3),d2
	beq.s	ConvertFail			; no convert routine available !
	move.l	d2,a2
	movem.l	d2-d4/a2-a4,-(sp)
	jsr	(a2)				; call convert function
	movem.l	(sp)+,d2-d4/a2-a4
	tst.l	d0
	bne.s	ConvertFail			; couldn't convert module

	move.l	LN_NAME(a3),d0			; get ^ListView name
	addq.l	#2,d0				; skip 2 spaces
	move.l	d0,FormName			; remember original module format

	moveq	#0,d3
	move.w	#1,Conv
	bra.s	ConvertEnd			; module successfully converted
ConvertFail:
	movem.l	a0-a6/d0-d6,-(sp)
	move.l	pt_pckddata,a1				; get module address
	move.l	pt_pckdsize,d0
	CALLEXEC FreeMem
	movem.l	(sp)+,a0-a6/d0-d6
ConvertNext:
	move.l	LN_SUCC(a3),a3
	bra	ConvertLoop			; try next format
ConvertEnd:
	move.l	#0,d0
	move	d3,d0				; set Result
	rts

*-----------------------------------------------------------------------*
;
; Routines executed for each converted module
; a5 = protrack module

Converted_by_Gryzor:
	lea	20(a5),a3		; pointe sur name 1er sample
	adda.l	#27*30,a3		; pointe sur le 28ème sample
	tst.b	(a3)			; 28eme sample vide ?
	bne.s	.argh
	tst.b	30(a3)			; 29eme sample vide ?
	bne.s	.argh
	tst.b	60(a3)			; 30eme sample vide ?
	beq	.do_it
.argh:
	suba.l	#27*30,a3		; revient au 1er sample_name
	moveq	#28,d0
  .tst:
	tst.b	(a3)
	bne.s	.next
	tst.b	30(a3)
	bne.s	.next
	tst.b	60(a3)
	beq	.do_it
  .next:
	lea	30(a3),a3
	dbf	d0,.tst
	bra	Put_PTK_Constante	; aucun incruste_msg ! snif..

.do_it:	lea	Gryzor_Msg,a4
	moveq	#2,d0			; 3 lignes
.loop:
	move.l	(a4)+,(a3)+		;  4 car	(Converted with)
	move.l	(a4)+,(a3)+		;  8 car	(Deli-Wizard)
	move.l	(a4)+,(a3)+		; 12 car	(By GRYZOR !)
	move.l	(a4)+,(a3)+		; 16 car
	move.l	(a4)+,(a3)+		; 20 car
	move.b	(a4)+,(a3)+		; 21 car
	clr.b	(a3)+
	addq	#1,a4
	lea	8(a3),a3		; jump sample_data (ouf!)
	dbf	d0,.loop

Put_PTK_Constante:
	move.b	#$7F,$3b7(a5)			; met la constante Protrack
	move.l	#'M.K.',$438(a5)

Force_00:
	lea	41(a5),a1			; pointe sur le dernier byte
	moveq	#30,d0				; de chaque sample_name
.loop:
	move.b	#$00,(a1)			; et force à 00 !
	lea	30(a1),a1			; (utile pour Unic...)
	dbf	d0,.loop
	rts

Gryzor_Msg: ;   '----------------------'
	dc.b	'_*  Converted with  *_'	; 22 caracteres
	dc.b	'_* Deli-Wizard 2.20 *_'
	dc.b	'_*    by Gryzor!    *_'
	even

*-----------------------------------------------------------------------*
;
; KRIS Check Routine

CheckKRIS:
	move.l	a0,a4			;copy data
	move.l	d0,d4			;copy size

	move.l	$3b8(a0),d0
	cmpi.l	#'KRIS',d0
	bne	.fail
	lea	44(a0),a0		; pointe sur sample_data
	moveq	#0,d2			; pour stocker SampLen
	moveq	#30,d0
.loop:
	move.l	(a0),d1			; take length + volume
	move.l	d1,d3
	andi.l	#$000000FF,d3
	cmpi.w	#$0040,d3
	bhi	.fail			; si volume > $40, fuck !!
	andi.l	#$FFFF0000,d1		; garde ke la length
	swap	d1			; dans le mot faible
	cmpi.w	#$8000,d1		; length must be < $8000
	bhi	.fail
	add.l	d1,d2			; cumul lengths
	lea	30(a0),a0
	dbf	d0,.loop

	lsl.l	#1,d2			; total length *2
	beq	.fail
	move.l	d2,SampLen		; longueur totale des samples

	lea	$3bc(a4),a0		; back to start
	moveq	#0,d0
	move.w	(a0)+,d0		; get nb_pos
	beq	.fail			; at least ONE pos
	lsr	#8,d0			; 1A00 = 001A
	move.w	d0,Nb_Pos
	lsl	#2,d0			; *4 mots par pos
	subq	#1,d0
	moveq	#0,d2
.pool:
	moveq	#0,d1
	move.w	(a0)+,d1
	ANDI.W	#$FF00,d1		; !!! vire les TRANSPOSE-NOTES
	cmp.w	d2,d1
	ble.s	.no
	move.w	d1,d2
  .no:
	dbf	d0,.pool
	addi.l	#$0100,d2
	move.l	d2,Bigest_Patt

	lea	$7c0(a4),a0
	add.l	d2,a0
	add.l	SampLen,a0
	sub.l	a4,a0			; = size_pack
	move.l	a0,d1
	sub.l	d1,d4
	cmpi.w	#-256,d4		; more than 256 bytes too short ??
	blt	.fail
;	cmpi.w	#+256,d4		; more than 256 bytes too long ??
;	bgt	.fail
			; too long is not a problem....

.KRIS_Get_PGP:
	lea	$3be(a4),a0
	move.l	a0,KR_Track_Patt
	adda.l	#$80*8+2,a0
	move.l	a0,KR_Track_Notes

	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; skip 1er pattern
	move.l	KR_Track_Patt(pc),a0
	lea	8(a0),a0		; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend les 2 longwords de chak patt
	move.l	(a0)+,d2
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	KR_Track_Patt(pc),a1	; table de comparaison
	lea	KR_PosTable,a3		; PTK Module
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	move.l	(a1)+,d4
	sub.l	d1,d3
	bne.s	.suivant
	sub.l	d2,d4
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos(pc),(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt

.KRIS_Check2:
	move.l	KR_Track_Notes(pc),a0
	move.l	#$ff,d2
	moveq	#0,d1			; compteur de $A8
.loop1:
	move.l	(a0)+,d3
	swap	d3
	cmpi.w	#$A800,d3
	bne.s	.et_non
	addq	#1,d1
.et_non:
	dbf	d2,.loop1
	tst.w	d1
	beq.s	.fail			; d1 still ZERO ? Ciao

	moveq	#0,d0			; Calc PTK Module Size
	move.w	Nb_Patt,d0
	lsl	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fuck
.okay:	rts

KR_Track_Patt:	dc.l	0
KR_Track_Notes:	dc.l	0
KR_BytePos:	dc.b	0
KR_Transpose:	dc.b	0
	even

*-----------------------------------------------------------------------*
;
; KRIS to Protracker Converter, Pro-Wizard routine

ConvertKRIS:
	move.l	a0,a4				; copy source addy
	move.l	a1,a5				; copy dest   addy

	lea	$3be(a0),a0
	move.l	a0,KR_Track_Patt
	adda.l	#$80*8+2,a0
	move.l	a0,KR_Track_Notes

	move.l	a4,a0
	move.l	(a0)+,(a1)+		; copy song_name
	move.l	(a0)+,(a1)+		; copy song_name
	move.l	(a0)+,(a1)+		; copy song_name
	move.l	(a0)+,(a1)+		; copy song_name
	move.l	(a0)+,(a1)+		; copy song_name
	addq	#2,a0			; pointe sur nom du 1er son
	move.l	#929,d0			; 930 octets (31 sons * 30o)
.loop:
	move.b	(a0)+,(a1)+
	dbf	d0,.loop

KRIS_DoIt:
	lea	$3bc(a4),a0
	moveq	#0,d0
	move.b	(a0),d0			; nb_pos
	move.w	d0,Nb_Pos
	move.b	d0,(a1)+		; write nb_pos
	move.b	#$7F,(a1)+		; constante

	lea	KR_PosTable,a0
	subq	#1,d0
.loop:
	move.b	(a0)+,(a1)+		; copy pattern_table
	dbf	d0,.loop

KRIS_Convert_Patterns:
	lea	$3b8(a5),a0		; PTK Module --> patt values
	moveq	#-1,d5
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	move.l	KR_Track_Patt(pc),a1
.loop:
	moveq	#0,d2
	move.b	(a0)+,d2		; take pattern_nbr
	cmp.b	d5,d2
	bgt.s	.okay
	lea	8(a1),a1
	bra.w	.fin
.okay:
	addq.l	#1,d5			; Plus Grand Pattern
	lsl.w	#2,d2			; mulu #4
	lsl.l	#8,d2			; 0004 = 0400 pour pointer ds patt.
	lea	$43c(a5),a3		; PTK_Module --> patterns
	add.l	d2,a3
	moveq	#3,d1			; 4 voies
.Vloop:
	moveq	#0,d2
	move.w	(a1)+,d2		; take track
	move.b	d2,KR_Transpose
	move.b	#$00,d2			; vire le transpose apres l'avoir pris
	move.l	KR_Track_Notes,a2
	add.l	d2,a2			; pointe sur la bonne track_notes
	moveq	#63,d2			; 64 notes	
.Nloop:
	move.l	(a2)+,d3		; take longword
	move.l	d3,d4
	andi.l	#$00FF0000,d4		; isole sample_nbr
	swap	d4			; 00080000 = 00000008
	moveq	#0,d7
	cmpi.w	#$0010,d4
	blt.s	.ok
	subi.w	#$0010,d4		; vire la dizaine
	move.l	#$10000000,d7		; prépare longword final (smpl>=$10)
.ok:
	bsr	KRIS_Get_Note
	move.l	d7,(a3)
	lea	16(a3),a3		; ligne suivante
	dbf	d2,.Nloop
	suba.l	#64*16,a3
	addq	#4,a3
	dbf	d1,.Vloop		; voie suivante
.fin:
	dbf	d0,.loop

KRIS_Copy_Samples:
	move.l	KR_Track_Notes(pc),a0
	add.l	Bigest_Patt,a0		; a0 = debut des samples
	move.l	a0,a1
	add.l	SampLen,a1		; a1 = fin des samples

	moveq	#0,d0
	move.w	Nb_Patt,d0		; $0F
	lsl.l	#8,d0
	lsl.l	#2,d0
	lea	$43c(a5),a2		; PTK jump header
	add.l	d0,a2			; jump patterns
.copy:					; donc pointe là où mettre les samples
	cmp.l	a1,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:

	; ----------------------------- ; Erase "01" bytes & check Repeat

	lea	20(a5),a3		; pointe sur name 1er sample
	moveq	#30,d0
.loop:
	cmp.b	#$01,(a3)
	bne.s	.next
	move.b	#$00,(a3)
.next:
	moveq	#0,d2
	move.w	26(a3),d2		; get Repeat
	beq.s	.no_need		; no repeat ?
	lsr	#1,d2			; divise par 2
	move.w	d2,26(a3)
.no_need:
	lea	30(a3),a3
	dbf	d0,.loop

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts


KRIS_Get_Note:
	ror.w	#4,d4			; 0008 = 8000
	add.w	d3,d4
	add.w	d4,d7
	andi.l	#$FF000000,d3		; isole la note packée
	rol.l	#8,d3			; 72000000 = 00000072
	cmpi.w	#$00A8,d3
	bne.s	.note_normale
	moveq	#0,d3
	bra.s	.fini
.note_normale:
	moveq	#0,d6
	move.b	KR_Transpose(pc),d6		; $02
	lsl	#1,d6			; mots
	add.b	d6,d3			; ajuste avec transpose
	subi.w	#$48,d3
	lea	mt_periodtable,a6
	move.w	(a6,d3.w),d3		; take real note
	andi.l	#$0000FFFF,d3
	swap	d3
.fini:
	add.l	d3,d7
	rts



NP2_Mark:	dc.l	'NP20'

*-----------------------------------------------------------------------*
;
; NoisePacker 2.0 Check Routine

CheckNP20
	move.l	a0,a4			;copy data
	move.l	d0,d4			;copy size

	move.l	(a0),d1
	swap	d1
	andi.l	#$0000F00F,d1		; Garde ce qui doit etre 0 et C
	cmpi.w	#$000C,d1
	bne	.fail
	move.l	(a0),d1
	swap	d1
	andi.l	#$00000FF0,d1		; Garde nb_samp
	beq	.fail			; no sample ? impossible !
	cmpi.w	#$01F0,d1		; no more than $1F samples
	bhi	.fail

	moveq	#0,d0
	move.w	(a0),d0
	lsr.w	#4,d0
	move.w	d0,Nb_Samp
	lsl	#4,d0
	addi.w	#8,d0
	move.w	(a0,d0.w),d0		; get nb_pos
	beq	.fail
	cmpi.w	#$FE,d0			; $7F pos (x2) = $FE, no more !
	bhi	.fail
	btst	#0,d0			; test if EVEN
	bne	.fail			; not 0 = fuck
	moveq	#0,d1
	move.w	2(a0),d1		; idem
	cmp.w	d1,d0
	bne	.fail
	moveq	#0,d1
	move.w	4(a0),d1		; nb_tracks
	beq	.fail
	btst	#0,d1			; test if EVEN
	bne	.fail
	moveq	#0,d1
	move.w	6(a0),d1		; samp_offset
	beq	.fail
	btst	#0,d1
	bne	.fail
	move.l	#'NP20',NP2_Mark
	cmpi.w	#$00C0,d1		; au moins $C0
	blo	.fail

	moveq	#0,d1
	move.w	Nb_Samp,d1
	subq	#1,d1			; nb_samples -1
	moveq	#0,d3			; pour stocker SampLen
	lea	12(a0),a0		; pointe sur length 1er son
.loop:
	move.l	-4(a0),d0
	cmpi.l	#$FE000,d0
	bhi	.fail
	move.l	4(a0),d0
	cmpi.l	#$FE000,d0
	bhi	.fail
	move.l	(a0),d5
	move.l	d5,d2
	andi.l	#$FFFF0000,d2		; garde ke length
	swap	d2
	cmpi.w	#$8000,d2		; pas de len > $7FFF
	bhi	.fail
	add.l	d2,d3			; cumul des longueurs

	tst.w	d2
	beq	.fail			; pas de length nulle !!!!
	moveq	#0,d0
	move.w	8(a0),d0		; get replen
	add.w	10(a0),d0		; ajoute repeat
	cmp.l	d2,d0			; si les 2 > length, fuck
	bls	.seems_ok
	moveq	#0,d0
	move.w	8(a0),d0
	moveq	#0,d6
	move.w	10(a0),d6
	lsr	#1,d6
	add.w	d6,d0
	cmp.l	d2,d0
	bhi	.fail
	move.l	#'NP10',NP2_Mark
.seems_ok:

	move.l	d5,d2
	andi.w	#$FF00,d5		; garde ke le finetune
	cmpi.w	#$0F00,d5
	bhi	.fail
	andi.w	#$00FF,d2		; garde que le volume
	cmpi.w	#$0040,d2		; qui doit etre < ou = $40
	bhi	.fail
	lea	16(a0),a0
	dbf	d1,.loop

	lsl.l	#1,d3
	beq	.fail
	move.l	d3,SampLen

	lea	8(a4),a0
	moveq	#0,d0
	move.w	Nb_Samp,d0
	lsl.w	#4,d0			; mulu	#16
	add.l	d0,a0

	move.l	a0,a2
	moveq	#0,d5
	moveq	#0,d1
	move.w	(a2),d1
	lsr	#1,d1
	subq	#1,d1
	addq	#4,a2
.last:
	moveq	#0,d2
	move.w	(a2)+,d2
	move.l	d2,d3
	lsr	#3,d3			; doit etre divisible par 8
	lsl	#3,d3
	cmp.w	d2,d3
	bne.s	.fail
	cmp.w	d5,d2
	bls.s	.no_need
	move.w	d2,d5
.no_need:
	dbf	d1,.last
	lsr	#3,d5
	addq	#1,d5
	move.w	d5,Nb_Patt

	addq	#4,a0			; pointe sur table_pos (passe le $3A)
	move.l	a0,a1
	move.l	a4,a0
	add.w	2(a0),a1		; add nb_pos
	add.w	4(a0),a1		; add nb_tracks
	add.w	6(a0),a1		; add offset_samples
	add.l	SampLen,a1		; fin du pack_mod
	sub.l	a4,a1			; size pack_mod
	move.l	a1,d0
	sub.l	d0,d4
	cmpi.l	#-256,d4		; more than 256 bytes too short ??
	blt	.fail

	moveq	#0,d0			; Calc PTK Module Size
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fuck
.okay:	rts

*-----------------------------------------------------------------------*
;
; NP2 to Protracker Converter, Pro-Wizard routine

ConvertNP20
	move.l	a0,a4				; copy source addy
	move.l	a1,a5				; copy dest   addy

NP2_Heading:
	lea	42(a1),a1
	moveq	#0,d0
	move.w	Nb_Samp,d0
	subq	#1,d0			; nb_samples -1
	lea	12(a0),a0		; pointe sur volume 1er son
.loop:
	move.l	(a0)+,(a1)		; copy length+vol
	addq	#4,a0
	move.w	(a0)+,6(a1)		; copy replen
	moveq	#0,d1
	move.w	(a0)+,d1
	cmpi.l	#'NP10',NP2_Mark
	bne.s	.nono
	lsr	#1,d1
.nono:
	move.w	d1,4(a1)		; copy repeat
	addq	#4,a0
	lea	30(a1),a1
	dbf	d0,.loop

	cmp.w	#$1F,Nb_Samp
	beq.s	NP2_DoIt
	moveq	#30,d0
	sub.w	Nb_Samp,d0
.loop1:
	move.l	#$0,(a1)
	move.l	#$1,4(a1)		; fill a vide le reste des samples...
	lea	30(a1),a1
	dbf	d0,.loop1

NP2_DoIt:
	lea	2(a4),a0
	move.w	(a0)+,Nb_Pos		; $3A
	move.w	(a0),Nb_Tracks		; $C0
	lea	8(a4),a0
	moveq	#0,d0
	move.w	Nb_Samp,d0
	lsl.w	#4,d0		; mulu	#16
	add.l	d0,a0
	addq	#4,a0		; pointe sur table_pos (passe le $3A)

	move.l	a0,NP2_Addy_Pos
	move.l	a0,a1
	add.w	Nb_Pos,a1
	move.l	a1,NP2_Addy_Tracks
	add.w	Nb_Tracks,a1
	move.l	a1,NP2_Addy_Notes

	moveq	#0,d0
	move.w	Nb_Pos,d0
	lsr.w	#1,d0		; divise par 2
	lea	$3B6(a5),a1
	move.b	d0,(a1)+	; max_pos
	move.b	#$7F,(a1)+
	subq	#1,d0
.loop2:
	move.w	(a0)+,d1	; copy les patt_pos
	lsr	#3,d1
	move.b	d1,(a1)+	; copy table_patt
	dbf	d0,.loop2

NP2_Depacker:
	sub.l	a6,a6			; stocker src_adr_samples
	move.l	NP2_Addy_Tracks(pc),a0	; pointe sur tracks_table

	move.l	NP2_Addy_Notes(pc),a3	; pointe au debut des patterns

	lea	$43c(a5),a1		; debut patterns mais commence par la
	lea	12(a1),a1		; 4eme voie...etc coz inversement NP2
	moveq	#0,d7
	move.w	Nb_Patt,d7
	lsl	#2,d7		; * 4 voies
	subq	#1,d7
  .Haha:
	move.l	a3,a2		; debut patterns
	moveq	#0,d0
	move.w	(a0)+,d0	; rajoute jump_track
	add.l	d0,a2
	moveq	#63,d6		; 64 notes (ou lignes)
  .note:
	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	move.b	(a2)+,d1		; note
	move.b	(a2)+,d2		; son + commande
	move.b	(a2)+,d3		; valeur de la commande
	bsr	NP2_Test_Pair	; Valeur reelle de la note dans d4
	bsr	NP2_Take_Son	; Valeur commande + son dans d1
	move.w	Val1(pc),(a1)
	move.w	Val2(pc),2(a1)
	lea	16(a1),a1	; pointe sur ligne suivante (Dest)
	dbf	d6,.note
	suba.l	#16*64,a1	; reviens
	add.l	#4,Saut_a1	; voie suivante (Dest)
	move.l	#4,Saut
	cmpi.l	#16,Saut_a1
	bge.s	.MAJ
	bra.s	.GoOn
  .MAJ:
	clr.l	Saut
	clr.l	Saut_a1
	adda.l	#12,a1		; ici on rajoute 12 au lieu de soustraire
	adda.l	#$400,a1	; toujours add 1024 pour patt suivant
  .GoOn:
	sub.l	Saut(pc),a1	; et ici on soustrait le SAUT
	cmp.l	a6,a2
	blt.s	.no_need
	move.l	a2,a6
.no_need:
	dbf	d7,.Haha

	lea	-12(a1),a1	; a la fin, re-enlever les 12 du debut !!

NP2_Copy_Samples:
	move.l	a1,a2			; start_samples_dest
	move.l	a6,a0			; start_samples_source
	move.l	a0,a3
	add.l	SampLen,a3
.copy:					; donc pointe là où mettre les samples
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts


NP2_Test_Pair:
	move.l	a6,-(sp)

	moveq	#0,d0		; Clear d0
	moveq	#0,d5		; Clear d5
	move.b	d1,d0		; Note dans d0
	move.b	d1,d5		; Note dans d5
	lsr	#1,d0		; Divise par 2
	lsl	#1,d0		; Remultiplie par 2
	cmp.w	d0,d1
	beq.s	.pair
	moveq	#0,d4
	move.w	#$1000,d4
	move.b	d0,d1		; note paire dans d1
	bra.s	.Jump
  .pair:
	moveq	#0,d4
  .Jump:
	subq	#2,d1			; -2
	lea	mt_periodtable,a6	; pointe sur C-1
	add.l	d1,a6
	moveq	#0,d1			; Clear d1
	cmp.w	#$01,d5			; Pas de note ?
	bls.s	.hop
  .SiSi:
	move.w	(a6),d1			; Mets vraie note dans d1
  .hop:
	add.w	d1,d4			; Concatene avec numson si + 16
	move.w	d4,Val1

	move.l	(sp)+,a6
	rts

NP2_Take_Son:
	moveq	#0,d1
	moveq	#0,d5
	move.b	d2,d1
	move.b	d2,d5
	andi.b	#$f0,d1		; Garde ke le numero du son
	lsl	#8,d1		; Transforme un 0010 en 1000
	andi.b	#$0f,d5		; Garde ke la commande
	cmpi.w	#$0B,d5
	beq.s	.Break
	cmpi.w	#$07,d5		; Effet 7 ? (A)
	beq.s	.Chge
	cmpi.w	#$08,d5		; Effet 8 = 0 arpeggio !!
	beq.s	.Chge1
	cmpi.w	#$06,d5		; Effet 6 ?
	beq.s	.Chge2
	cmpi.w	#$05,d5		; Effet 5 ?
	bne.s	.Good
	bra.s	.Chge2
  .Break:
	addi.b	#2,d3
	lsr	#1,d3
	bra.s	.Good
  .Chge:
	move.b	#$0A,d5		; Transforme en 000A
  .Chge2:
	cmpi.w	#$00F0,d3
	bgt.s	.do_neg
	lsl	#4,d3
	bra.s	.Good
  .Chge1:
  	moveq	#0,d5		; vire le 8
	bra.s	.Good
.do_neg:
	neg.b	d3		; Transforme un $f1 en $0f
  .Good:
	lsl	#8,d5		; Transforme en 0A00
	add.b	d3,d5		; + valeur de l'effet	(= 0A0F)
	add.w	d5,d1		; d1 = 1A0F
	move.w	d1,Val2
	move.w	d1,d5
	andi.w	#$0F00,d5
	cmpi.w	#$0E00,d5
	bne.s	.bye
	andi.w	#$FF00,d1
	move.b	#$01,d1		; force le filtre a 01 = OFF
	move.w	d1,Val2
.bye:
	rts

Saut:		dc.l	0
Saut_a1:	dc.l	0
NP2_Addy_Pos:	dc.l	0
NP2_Addy_Tracks:dc.l	0
NP2_Addy_Notes:	dc.l	0
Val1:		dc.w	0
Val2:		dc.w	0
Nb_Tracks:	dc.w	0


*-----------------------------------------------------------------------*
;
; NoisePacker 3.0 Check Routine

CheckNP30
	move.l	a0,a4			;copy data
	move.l	d0,d4			;copy size


	move.l	(a0),d1
	swap	d1
	andi.l	#$0000F00F,d1		; Garde ce qui doit etre 0 et C
	cmpi.w	#$000C,d1
	bne	.fail
	move.l	(a0),d1
	swap	d1
	andi.l	#$00000FF0,d1		; Garde nb_samp
	beq	.fail			; no sample ? impossible !
	cmpi.w	#$01F0,d1		; no more than $1F samples
	bhi	.fail

	moveq	#0,d0
	move.w	(a0),d0
	lsr.w	#4,d0
	move.w	d0,Nb_Samp
	lsl	#4,d0
	addi.w	#8,d0
	move.w	(a0,d0.w),d0		; get nb_pos
	beq	.fail
	cmpi.w	#$FE,d0			; $7F pos (x2) = $FE
	bhi	.fail
	btst	#0,d0
	bne	.fail
	moveq	#0,d1
	move.w	2(a0),d1		; idem
	cmp.w	d1,d0
	bne	.fail
	moveq	#0,d1
	move.w	4(a0),d1		; nb_tracks
	beq	.fail
	btst	#0,d1
	bne	.fail
	moveq	#0,d1
	move.w	6(a0),d1		; samp_offset
	beq	.fail
	btst	#0,d1
	bne	.fail

	moveq	#0,d1
	move.w	Nb_Samp,d1
	subq	#1,d1			; nb_samples -1
	moveq	#0,d3			; pour le cumul des longueurs_smpl
	lea	8(a0),a0		; pointe sur volume 1er son
.loop:
	move.l	2(a0),d0		; start addy
	cmpi.l	#$FE000,d0
	bhi	.fail
	move.l	8(a0),d0		; repeat addy
	cmpi.l	#$FE000,d0
	bhi	.fail
	moveq	#0,d0
	move.w	(a0),d0			; vol
	move.l	d0,d2
	andi.w	#$FF00,d0		; garde le finetune
	cmpi.w	#$0F00,d0		; ne doit pas depasser $0F
	bhi	.fail
	andi.w	#$00FF,d2		; garde que le volume
	cmpi.w	#$0040,d2		; qui doit etre inferieur ou egal a $40
	bhi	.fail
	moveq	#0,d0
	move.w	6(a0),d0
	beq	.fail			; pas de length NULLE !!!
	add.l	d0,d3

	moveq	#0,d2
	move.w	12(a0),d2		; get replen
	add.w	14(a0),d2		; ajoute repeat
	cmp.l	d0,d2			; si les 2 > length, fuck
	bhi	.fail
	lea	16(a0),a0		; sample suivant
	dbf	d1,.loop

	lsl.l	#1,d3
	beq	.fail
	move.l	d3,SampLen

	lea	8(a4),a0
	moveq	#0,d0
	move.w	Nb_Samp,d0
	lsl.w	#4,d0			; mulu	#16
	add.l	d0,a0

	move.l	a0,a2
	moveq	#0,d5
	moveq	#0,d1
	move.w	(a2),d1
	lsr	#1,d1
	subq	#1,d1
	addq	#4,a2
.last:
	moveq	#0,d2
	move.w	(a2)+,d2
	move.l	d2,d3
	lsr	#3,d3
	lsl	#3,d3
	cmp.w	d2,d3
	bne	.fail
	cmp.w	d5,d2
	bls.s	.no_need
	move.w	d2,d5
.no_need:
	dbf	d1,.last
	lsr	#3,d5
	addq	#1,d5
	move.w	d5,Nb_Patt

	addq	#4,a0			; pointe sur table_pos (passe le $3A)
	move.l	a0,a1
	move.l	a4,a0
	add.w	2(a0),a1		; add nb_pos
	add.w	4(a0),a1		; add nb_tracks
	add.w	6(a0),a1		; add offset_samples
	add.l	SampLen,a1		; fin du pack_mod
	sub.l	a4,a1			; size pack_mod
	sub.l	a1,d4
	cmpi.l	#-256,d4		; more than 256 bytes too short ??
	blt	.fail

	moveq	#0,d0			; Calc PTK Module Size
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts

*-----------------------------------------------------------------------*
;
; NoisePacker 3.0 to NoiseTracker Converter (© by Gryzor)
;

ConvertNP30
	move.l	a0,a4
	move.l	a1,a5

Heading:
	moveq	#0,d1
	move.w	Nb_Samp,d1
	subq	#1,d1			; nb_samples -1
	lea	42(a5),a1		; pointe sur long samp 1
	lea	8(a0),a0		; pointe sur volume 1er son
.loop:
	move.w	(a0)+,2(a1)		; swap volume
	addq	#4,a0			; jump octets_addresses
	move.w	(a0)+,(a1)		; swap length
	addq	#4,a0			; jump octets_addresses
	move.w	(a0)+,6(a1)		; swap replen (2 octets)
	move.w	(a0)+,4(a1)		; swap repeat (2 octets)
	lea	30(a1),a1		; sample suivant
	dbf	d1,.loop

	cmp.w	#$1F,Nb_Samp
	beq.s	NP3_DoIt
	moveq	#30,d0
	sub.w	Nb_Samp,d0
.loop1:
	move.l	#$0,(a1)
	move.l	#$1,4(a1)		; fill a vide le reste des samples...
	lea	30(a1),a1
	dbf	d0,.loop1

NP3_DoIt:
	moveq	#0,d0
	move.w	(a0)+,d0		; nb_positions_max
	lsr	#1,d0			; remet au format PTK (divise par 2)
	lea	$3b6(a5),a1
	move.b	d0,(a1)+		; write it
	move.b	#$7F,(a1)+		; constante

	move.w	d0,Nb_Pos		; nombre de patterns joués
	subq	#1,d0			; DBF
	addq	#2,a0			; jump 2 octets
.loop:
	moveq	#0,d1
	move.w	(a0)+,d1
	lsr	#3,d1			; divise par 8
	move.b	d1,(a1)+
	dbf	d0,.loop

	move.l	a0,Addy_Tracks		; table des adresses de chak track !
	lea	4(a4),a2
	add.w	(a2)+,a0
	move.l	a0,Addy_Patt		; adresse des patterns packés !!!
	add.w	(a2),a0
	move.l	a0,Addy_Samples		; debut des samples !!

NP3_Pattern_Generator:
	clr.l	Dest_Ptr
	clr.l	Dest_Ptr2
	clr.b	Nb_Notes_per_Track

	move.l	a4,a0			; Packed Mod
	moveq	#0,d0
	move.w	4(a0),d0		; take nb_tracks total
	lsr	#1,d0			; divise par 2 (traitement par mot)
	subq	#1,d0			; DBF
	move.l	Addy_Tracks,a0		; 1ère table : track_offset
NP3_The_Loop:
	move.l	Addy_Patt,a2		; 2ème table : patterns packés
	add.w	(a0)+,a2		; add track_offset !
	lea	$43c(a5),a1		; pointe debut patterns module PTK
	lea	12(a1),a1
	sub.l	Dest_Ptr,a1		; ajoute pointeur de tracks_réelles
	add.l	Dest_Ptr2,a1		; ajoute pointeur de patterns_réels
.In_track:
	clr.w	Real_Note
	moveq	#0,d1
	move.b	(a2)+,d1		; take first octet !
	cmpi.w	#$c1,d1			; teste si note ou saut (lignes vides)
	bge.s	.vide			; $c0 = 64 lignes vides : next_track
	cmpi.w	#$c0,d1			; teste si note ou saut (lignes vides)
	blt.s	.c_une_note		; < $c0 : c une note, traiter 3 octets
	bra.w	.next_track
.vide:
	neg.b	d1			; si $c1 ==> $3f = 63 notes vides !
	add.b	d1,Nb_Notes_per_Track
	moveq	#0,d7
	move.b	Nb_Notes_per_Track,d7
	cmpi.w	#64,d7			; 64 notes reconstituées ?? = 1 voie
	bge.w	.next_track
	lsl	#4,d1			; Mulu 16 : saute ligne
	add.l	d1,a1
	bra.s	.In_track		; note suivante de la meme voie
.c_une_note:
	moveq	#0,d7
	move.l	d1,d2
	lsr	#1,d1			; divise par 2 pour tester si PAIR
	lsl	#1,d1			; re-multiplie par 2
	cmp.w	d1,d2
	beq.s	.pair
	move.w	#$1000,d7		; prepare 1er mot pour Samp > $10
.pair:
	tst.b	d1
	beq.s	.no_note
	subq	#2,d1
	lea	mt_periodtable,a6
	move.w	(a6,d1.w),Real_Note
.no_note:
	add.w	d7,Real_Note		; Merge avec d7  ($1000 ou $0000)
	move.w	Real_Note(pc),d7	; d7 = 00 00 01 AC
	swap	d7			; d7 = 01 AC 00 00
	moveq	#0,d6
	move.b	(a2),d6			; take byte_cmd (1F ex...)
	andi.w	#$000F,d6		; isole commande
	cmpi.w	#$0008,d6
	bne.s	.pas_huit
	move.b	(a2)+,d6		; reprend l'octet original
	subi.w	#$0008,d6		; 8 = 0 (arpeggio)
	bra.s	.branche
.pas_huit:
	move.b	(a2)+,d6		; reprend l'octet original
.branche:
	lsl	#8,d6			; decale de 2 vers la gauche
	move.b	(a2)+,d6		; take byte_cmd (06 ex...)
	move.w	d6,d7

	move.l	d7,d6
	andi.l	#$00000F00,d6
	cmpi.w	#$0E00,d6		; test si cmd_filter
	bne.s	.desole
	move.b	#$01,d7			; si oui, forcer le filtre OFF
.desole:
	move.l	d7,(a1)			; write good_line
	lea	16(a1),a1		; ligne suivante pattern_réel
	addi.b	#1,Nb_Notes_per_Track	; 1 ligne de + 
	cmp.b	#64,Nb_Notes_per_Track
	bge.s	.next_track
	andi.w	#$0F00,d7		; garde que COMMANDE
	cmpi.w	#$0D00,d7		; Break Pattern ??
	beq.s	.next_track
	cmpi.w	#$0B00,d7		; Pattern JUMP ??
	beq.s	.next_track
	bra.w	.In_track
.next_track:
	add.l	#4,Dest_Ptr		; track suivante (4 octets)
	cmpi.l	#16,Dest_Ptr		; 4 tracks refaites = 1 pattern !!!
	blt.s	.not_yet
	clr.l	Dest_Ptr		; come back to start of zis pattern &
	add.l	#$400,Dest_Ptr2		; jump to Pattern suivant
.not_yet:
	clr.b	Nb_Notes_per_Track	; RAZ compteur de notes par voie...
	dbf	d0,NP3_The_Loop

; --------------------- Debug cmd 5xx, 6xx et (7xx = Axx) -------------------

NP3_Check_for_bugged_cmds:
	lea	$43c(a5),a1		; pointe sur debut patterns
	move.l	Dest_Ptr2(pc),d0	; ex : $800 pour 2 patterns refaits
	lsr.l	#2,d0			; divise par 4  (4 octets par note)
	subq	#1,d0			; pour le DBF
.loop:
	move.l	(a1),d1			; take long word (01 AC 1F 04)
	move.l	d1,d2			; save_it
	andi.l	#$00000F00,d2		; garde que la commande
	cmpi.w	#$0B00,d2
	beq.s	.arghl
	cmpi.w	#$0500,d2		; commande 5 ???
	beq.s	.trouve
	cmpi.w	#$0600,d2		; commande 6 ???
	beq.s	.trouve
	cmpi.w	#$0700,d2		; commande 7 ???
	bne.s	.next_note
	move.w	#$0A00,d2		; remet $0A00 pour $0700
.trouve:
	moveq	#0,d3
	move.w	d1,d3			; reprend le mod_cmd original
	andi.w	#$00FF,d3		; garde que la valeur de la cmd
	cmpi.w	#$00F0,d3		; de $F0 a $FF ??
	bge.s	.neg_it
	lsl	#4,d3			; si $01 : $10
	bra.s	.join_back
.arghl:
	moveq	#0,d5
	move.b	d1,d5
	addi.b	#4,d5			; add 4 aux commandes Bxx
	lsr	#1,d5			; et les diviser par 2 !!
	move.b	d5,d1
	move.l	d1,(a1)
	bra.s	.next_note
.neg_it:
	neg.b	d3			; retrouve valeur normale ($F1 = $0F)
.join_back:
	add.w	d3,d2			; $0A00 + $000F = $0A0F ex...
	andi.w	#$F000,d1		; vire cmd+valeur dans le lgwd
	add.w	d2,d1			; kon remet dans le long mot.
	move.l	d1,(a1)			; re-write it dans le pattern
.next_note:
	addq	#4,a1
	dbf	d0,.loop

; --------------------------   COPY SAMPLES   --------------------------

NP3_Copy_Samples:
	lea	$43c(a5),a2		; pointe sur debut patterns
	add.l	Dest_Ptr2(pc),a2	; pointe sur les sons_dest

	move.l	Addy_Samples(pc),a0	; Pointe sur les sons
	move.l	a0,a3
	add.l	SampLen,a3
.copy:					; donc pointe là où mettre les samples
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts


Addy_Tracks:	dc.l	0
Addy_Patt:	dc.l	0
Addy_Samples:	dc.l	0
Dest_Ptr:	dc.l	0
Dest_Ptr2:	dc.l	0
Real_Note:	dc.w	0
Nb_Notes_per_Track:
		dc.b	0
		even


*-----------------------------------------------------------------------*
;
; NoiseRunner Check Routine

CheckNRU:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	moveq	#0,d0
	moveq	#30,d1
.loop:
	move.l	2(a0),d2		; adresse start sample
	beq	.fail
	move.l	8(a0),d3		; adresse start repeat
	beq	.fail
	moveq	#0,d5
	move.w	6(a0),d5		; take length
	bne.s	.tst2
	cmp.l	d2,d3			; les 2 adresses doivent être égales
	bne	.fail			; sinon fuck
	bra.s	.tst3
.tst2:
	sub.l	d2,d3
	lsr.l	#1,d3
	add.w	12(a0),d3		; add replen
	cmp.l	d5,d3
	bhi	.fail
.tst3:
	add.l	d5,d0			; cumul lengths
	moveq	#0,d5
	move.w	(a0),d5			; take volume
	cmpi.w	#$0040,d5		; si vol>$40 : fuck
	bhi	.fail
	lea	16(a0),a0
	dbf	d1,.loop

	lsl.l	#1,d0
	beq	.fail
	move.l	d0,SampLen

	lea	$3b6(a4),a0
	move.l	(a0),d1
	andi.l	#$FF000000,d1		; garde nb_pos
	cmp.l	#$7F000000,d1		; si nb_pos > $7F, pas la peine
	bhi	.fail
	tst.l	d1			; ou aucune POS ? fuck
	beq	.fail
	move.l	(a0),d1
	andi.l	#$00FF0000,d1		; garde constante
	cmp.l	#$007F0000,d1		; si const = $7F, possible
	beq.s	.maybe
	tst.l	d1			; si const = 0, c possible
	bne	.fail			; ni 7F, ni 00, fuck
.maybe:
	move.l	(a0),d1
	andi.l	#$0000FF00,d1		; garde nb_patt 0
	cmpi.w	#$3F00,d1		; si patt > $3F, pas la peine
	bhi	.fail
	move.l	(a0),d1
	andi.l	#$000000FF,d1		; garde nb_patt 1
	cmpi.w	#$003F,d1		; si patt > $3F, pas la peine
	bhi	.fail


	lea	$3ae(a4),a0
	moveq	#0,d0
	move.w	(a0),d0
	beq.s	.mustbenul
	bmi	.fail
	moveq	#0,d1
	move.w	2(a0),d1
	cmp.b	#$40,d1			; volume
	bhi	.fail
	lsr	#8,d1
	cmp.b	#$0F,d1			; finetune
	bhi	.fail
	move.w	4(a0),d1
	add.w	6(a0),d1
	cmp.w	d0,d1
	bhi	.fail
	bra.s	.comptepatt
.mustbenul:
	move.w	2(a0),d0		; les vrais samples restants
	add.w	4(a0),d0		; si pas de sample, replen = 1
	add.w	6(a0),d0
	cmpi.w	#1,d0
	bne	.fail
.comptepatt:
	lea	$3b6(a4),a0
	moveq	#0,d0
	move.b	(a0),d0
	addq	#2,a0
	bsr	Get_PGP
	cmpi.w	#$40,d1
	bhi	.fail
	lsl	#8,d1
	lsl.l	#2,d1
	lea	$43c(a4),a0
	add.l	d1,a0
	add.l	SampLen,a0		; fin du pack_mod
	sub.l	a4,a0			; size pack_mod
	sub.l	a0,d4
	cmpi.l	#-256,d4
	blt.s	.fail

; Test_Note_Values

	lea	$43c(a4),a0
	move.l	#(64*4)-1,d0
.pool:
	move.l	(a0)+,d1
	andi.l	#$0000FF00,d1		; garde la note
	cmpi.w	#$4800,d1
	bhi	.fail
	dbf	d0,.pool

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts



*-----------------------------------------------------------------------*
;
; NoiseRunner-2-Protracker Convert routine, from Pro-Wizzy

ConvertNRU:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; Heading

	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.w	6(a0),(a1)+		; move length
	move.w	(a0),(a1)+		; move volume
; finetune ??
	moveq	#0,d3
	move.w	$e(a0),d3		; take word_finetune or crap
	beq.s	.next			; si nul, on fait rien
	move.l	d3,d4			; save_it
	andi.w	#$F000,d3		; garde que 1er car.
	cmpi.w	#$F000,d3		; si ca commence par $F : finetune !!
	bne.s	.next			; sinon c le reste du texte d'origine
; yes, finetune !
	move.w	#$1000,d3		; valeur de base
	andi.w	#$0FFF,d4		; garde que finetune (multiple de $48)
	sub.w	d4,d3			; substract it
	divu	#$48,d3			; get REAL finetune ! yeah
	move.b	d3,-2(a1)		; write it back !
.next:
	move.l	8(a0),d1		; take repeat address
	sub.l	2(a0),d1		; enleve start address
	lsr.w	#1,d1			; divise par 2
	move.w	d1,(a1)+		; move repeat
	move.w	12(a0),(a1)+		; move replen

	lea	$10(a0),a0		; next sample
	lea	22(a1),a1		; next sample
	dbf	d0,.loop

	lea	$3b6(a4),a0
	lea	$3b6(a5),a1
	move.l	#$81,d1
.nunchak:
	move.b	(a0)+,(a1)+
	dbf	d1,.nunchak

	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl.l	#8,d1
	subq.l	#1,d1
	move.l	d1,Bigest_Patt

NoiseRunner_Do_it:
	lea	$43c(a4),a0
	lea	$43c(a5),a1
.loop:
	move.l	(a0)+,d0
	moveq	#0,d2
	move.b	d0,d2			; traite le sample number
	lsr.w	#3,d2			; divise par 8
	moveq	#0,d3
	cmp.w	#$10,d2			; sample > $10 ?
	blt.s	.no_decoupe
	subi.w	#$10,d2
	move.l	#$10000000,d3		; prepare note finale
.no_decoupe:
	ror.w	#4,d2			; 0004 = 4000
	move.w	d2,d3			; add sample number ds note finale
	
	lsr.l	#8,d0			; traite la note
	moveq	#0,d2
	move.b	d0,d2			; take note
	moveq	#0,d4
	tst.w	d2			; pas de note ?
	beq.s	.no_pioche
	lea	mt_periodtable,a2	; trouve real_note
	subq	#2,d2
	move.w	(a2,d2.w),d4		; retrouve real note
.no_pioche:
	swap	d4
	add.l	d4,d3			; insere note ds note finale

	lsr.l	#8,d0			; traite la cmd_value
	move.b	d0,d3			; insere valeur dans note finale

	lsr.w	#8,d0			; traite la commande
	moveq	#0,d2
	move.b	d0,d2			; take cmd
	tst.w	d2			; cmd 0 = 3
	bne.s	.t2
	andi.w	#$F0FF,d3		; prepare l'emplacement pour la cmd
	add.w	#$0300,d3		; met cmd 3
	bra.s	.next
.t2:
	cmp.w	#$0C,d2
	bne.s	.t3
	andi.w	#$F0FF,d3		; efface l'emplacement de la cmd
	bra.s	.next
.t3:
	lsr.w	#2,d2			; sinon diviser par 4
	lsl.w	#8,d2			; et décaler de 2 a gauche
	add.w	d2,d3			; note FINALE !!
.next:
	move.l	d3,(a1)+
	dbf	d1,.loop

; NoiseRunner_Calc_SampLen:
	lea	$43c(a4),a0
	lea	$43c(a5),a1

	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl	#8,d1
	lsl.l	#2,d1			; mulu $400
	add.l	d1,a0
	add.l	d1,a1

	move.l	a0,a3			; debut des samples_source
	move.l	a1,a2			; debut des samples_dest
	add.l	SampLen,a3		; fin des sons
.copy:
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts


*-----------------------------------------------------------------------*
;
; Eureka-Packer Check Routine

CheckEureka
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	$3b6(a4),a0
	move.l	(a0),d1
	andi.l	#$FF000000,d1		; garde nb_pos
	cmp.l	#$7F000000,d1		; si nb_pos > $7F, pas la peine
	bhi	.fail
	tst.l	d1			; ou aucune POS ? fuck
	beq	.fail
	move.l	(a0),d1
	andi.l	#$00FF0000,d1		; garde constante
	cmp.l	#$007F0000,d1		; si const = $7F, possible
	beq.s	.maybe
	tst.l	d1			; si const = 0, c possible
	bne	.fail			; ni 7F, ni 00, fuck
.maybe:
	move.l	(a0),d1
	andi.l	#$0000FF00,d1		; garde nb_patt 0
	cmpi.w	#$3F00,d1		; si patt > $3F, pas la peine
	bhi	.fail
	move.l	(a0),d1
	andi.l	#$000000FF,d1		; garde nb_patt 1
	cmpi.w	#$003F,d1		; si patt > $3F, pas la peine
	bhi	.fail

	lea	$438(a4),a0
	tst.w	(a0)
	bne	.fail
	cmp.l	#'M.K.',(a0)		; no space for any 4-bytes MARK !
	beq	.fail
	cmp.l	#'SNT.',(a0)		; huhu !
	beq	.fail

	lea	42(a4),a0
	moveq	#0,d0
	moveq	#30,d1
.loop:
	moveq	#0,d3
	move.w	(a0),d3			; take length
	cmpi.w	#$8000,d3
	bhi	.fail
	add.l	d3,d0
	tst.w	d3
	beq.s	.next
	move.w	4(a0),d5
	add.w	6(a0),d5
	cmp.w	d3,d5
	bhi	.fail
.next:
	moveq	#0,d2
	move.w	2(a0),d2		; volume
	move.l	d2,d5
	andi.w	#$00FF,d5
	cmpi.w	#$0040,d5		; si vol>$40 : fuck
	bhi	.fail
	andi.w	#$FF00,d2
	cmpi.w	#$0F00,d2		; si fineT>$0F : fuck
	bhi	.fail
	lea	30(a0),a0
	dbf	d1,.loop

	lsl.l	#1,d0
	beq	.fail
	move.l	d0,SampLen

	move.l	a4,a0
	add.l	$438(a4),a0		; begin samples
	add.l	SampLen,a0		; end
	sub.l	a4,a0			; size mod
	sub.l	a0,d4			; difference des longueurs
	cmpi.l	#-256,d4
	blt	.fail			; 256_short allowed

	lea	$3b6(a4),a0
	moveq	#0,d0
	move.b	(a0),d0
	addq	#2,a0
	bsr	Get_PGP
	cmpi.w	#$40,d1
	bhi	.fail
	lea	$43c(a4),a0
	lsl	#2,d1
	subq	#2,d1
  .check:
	move.w	(a0)+,d2		; check if values are growing !
	move.w	(a0),d3
	cmp.w	d2,d3
	bls	.fail			; a shorter one ?? bad job !!
	dbf	d1,.check

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; EurekaPacker-2-Protracker Convert routine, from Pro-Wizzy

ConvertEureka
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; Heading
	move.l	#$437,d0
.loop:
	move.b	(a0)+,(a1)+
	dbf	d0,.loop

	move.l	(a0)+,Eureka_Smpl_Start
	move.l	a0,Eureka_Tracks_Table

Eureka_Conv:
	move.l	Eureka_Tracks_Table(pc),a0
	lea	$43c(a5),a1		; pointe sur les patterns_dest
	moveq	#0,d0
	move.w	Nb_Patt,d0
	subq	#1,d0
.loop:
	moveq	#3,d1			; 4 voies / pattern
.Vloop:
	move.l	a4,a2
	add.w	(a0)+,a2		; pointe sur la bonne track_source
	moveq	#0,d7			; compteur de notes (--> 64)
.64_notes:
	moveq	#0,d2
	move.b	(a2),d2			; pour checker si 0,4,8 ou C
	andi.w	#$00F0,d2
	tst.w	d2			; = 0 ??
	beq.w	.take_entire_note
	cmpi.w	#$10,d2
	beq.w	.take_entire_note
	cmpi.w	#$40,d2
	beq.w	.cmd_seule
	cmpi.w	#$80,d2
	beq.w	.note_seule
.espace_vide:
	moveq	#0,d2
	move.b	(a2)+,d2
	subi.w	#$00C0,d2
  .mini_loop:
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
  	dbf	d2,.mini_loop
	bra.w	.test_fin_track
.note_seule:
	addq	#1,d7
	moveq	#0,d6
	moveq	#0,d5
	move.b	(a2)+,d6		; take le $8x
	sub.w	#$0080,d6
	cmp.w	#$0010,d6
	blt.s	.minor
	sub.b	#$0010,d6
	move.l	#$10000000,d5
  .minor:
	ror.w	#4,d6			; 0003 = 3000
	add.l	d5,d6

	moveq	#0,d5
	move.b	(a2)+,d5
	lsl.w	#8,d5
	move.b	(a2)+,d5
	swap	d5
	add.l	d5,d6			; note finale
	move.l	d6,(a1)+
	add.l	#12,a1
	bra.w	.test_fin_track
.cmd_seule:
	addq	#1,d7
	clr.w	(a1)+
	move.b	(a2)+,(a1)
	sub.b	#$40,(a1)+
	move.b	(a2)+,(a1)+
	add.l	#12,a1
	bra.s	.test_fin_track
.take_entire_note:
	addq	#1,d7
	move.b	(a2)+,(a1)+
	move.b	(a2)+,(a1)+
	move.b	(a2)+,(a1)+
	move.b	(a2)+,(a1)+
	add.l	#12,a1
.test_fin_track:
	cmp.w	#64,d7
	blt.w	.64_notes
.fin_track:
	suba.l	#64*16,a1
	addq	#4,a1			; next track_dest
	dbf	d1,.Vloop
	adda.l	#$400,a1
	suba.l	#16,a1
	dbf	d0,.loop

Eureka_Copy_Samples:
	move.l	a4,a0
	add.l	Eureka_Smpl_Start(pc),a0
	move.l	a0,a3
	add.l	SampLen,a3
.copy:
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts

Eureka_Smpl_Start:
	dc.l	0
Eureka_Tracks_Table:
	dc.l	0
PTK_SampLen:
	dc.l	0


P50A_Sinfo_Adr:		dc.l	0
P50A_SampLen:		dc.l	0
P50A_Dest_SampLen:	dc.l	0
P60A_SampleType:	dc.l	0

P50A_InitSinfo:
	movem.l	a0/d0-d1,-(sp)
	lea	P50A_Sinfo,a0
	moveq.l	#30,d0
	moveq.l	#1,d1
.more	clr.l	(a0)+				; offset
	clr.l	(a0)+
	move.l	d1,(a0)+
	dbf	d0,.more
	movem.l	(sp)+,a0/d0-d1
	rts

*-----------------------------------------------------------------------*
;
; Player 50A/60A Check Routine

CheckP50A
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.w	#1,P50A_Sign
	move.l	#'P50A',P5ou6		; default is P5

	move.l	(a0)+,d2		; check for "P50A" or "P60A"
	cmpi.l	#'P50A',d2
	beq.s	.check_more
	cmpi.l	#'P60A',d2
	beq.s	.check_more
	cmpi.l	#'P61A',d2
	beq.s	.check_more
	subq	#4,a0			; no sign ? back to beginning
	clr.w	P50A_Sign
.check_more:
	move.l	a0,a4			; resauve !


	moveq	#0,d0
	move.w	(a0),d0
	btst	#0,d0			; le bit 0 = toujours 0 si pair
	bne	.fail			; test si offset_smpl PAIR !
	cmpi.w	#$0024,d0
	blt	.fail

	move.l	(a0),d0
	move.l	d0,d1
	andi.w	#$FF00,d1		; Garde nb_patt
	cmpi.w	#$4000,d1		; nb_patt ne depasse pas $40 ?
	bhi.w	.fail			; si, .fuck
	move.l	d0,d1			; sinon, test nb_samples
	andi.w	#$00FF,d1		; Garde nb_smpl
	cmpi.w	#$00DF,d1		; nb_smpl ne depasse pas $DF ?
	bhi.w	.fail			; plus petit ou egal, on teste !

	lea	8(a4),a0			; sample infos
	btst.b	#6,3(a4)			; p6 buffer?
	bne.s	.pk
	subq.l	#4,a0				; no buffer size (no packing)
	move.l	a0,P50A_Sinfo_Adr
	btst.b	#7,3(a4)			; delta ?
	bne.s	.dlt
	move.l	#'NORM',P60A_SampleType
	bra.s	.gooo

.pk	move.l	a0,P50A_Sinfo_Adr
	move.l	#'PACK',P60A_SampleType
	bra.s	.gooo

.dlt:
	move.l	#'DELT',P60A_SampleType

; ----- header info

.gooo:
	moveq	#0,d1
	move.b	2(a4),d1		; take nb_patt
	beq	.fail
	move.w	d1,Nb_Patt

	moveq	#$1f,d0
	and.b	3(a4),d0
	move.w	d0,Nb_Samp
	beq	.fail			;*bad

	mulu	#6,d0
	lea	0(a0,d0.w),a1			; end of sample infos
	lsl.w	#3,d1
	add.w	d1,a1
	movea.l	a4,a2
	add.w	(a4),a2				; debut des samples
	cmp.l	a1,a2
	bls	.fail			;*bad



; check sample infos

	bsr	P50A_InitSinfo			; init sinf table

	lea	P50A_Sinfo,a2

	moveq.l	#0,d2				; sample total
	moveq.l	#0,d3				; dest sample total
	move.l	a2,a1

	moveq	#0,d0
	move.w	Nb_Samp,d0
	bra.s	.smpl

.mn	moveq	#0,d1
	move.w	(a0)+,d1			; sample length
	bpl.s	.ok

	neg.w	d1				; or sample index
	cmpi.w	#31,d1
	bhi	.fail			;*bad coding!
	mulu	#12,d1
	move.l	-12(a2,d1.w),(a1)+		; offset is this
	move.w	-8(a2,d1.w),d1			; another sample len
	bra.s	.set

.ok	move.l	d2,(a1)+			; offset from 'smpbg'
	add.l	d1,d2				; this length
	tst.w	(a0)				; vol/ftu neg->fib packed
	bmi.s	.set
	add.l	d1,d2				; not fib packed
.set	add.l	d1,d3				; original length
	add.l	d1,d3
	move.w	d1,(a1)+			; sample length
	move.w	(a0)+,d1			; vol/ftu (check)
	move.w	d1,(a1)+
	cmp.b	#$40,d1
	bhi	.fail			;*bad
	and.w	#$7000,d1
	bne	.fail			;*bad
	move.w	(a0)+,d1			; wroffs
	bmi.s	.norep
	move.w	d1,(a1)
	move.w	-4(a1),d1			; real length
	sub.w	(a1),d1
	bcs	.fail			;*bad length
	move.w	d1,2(a1)			; wrlen
.norep	addq.l	#4,a1
.smpl	dbf	d0,.mn

	move.l	d2,SampLen
	move.l	d2,P50A_SampLen
	move.l	d3,P50A_Dest_SampLen


	tst.w	(a0)			; 1ere adresse_track
	bne	.fail			; doit etre 0000 !!

	move.l	a4,a1
	moveq	#0,d5
	move.w	(a1),d5
	moveq	#0,d0
	move.b	2(a1),d0
	subq	#1,d0
  .up:
	moveq	#2,d1
  .smp:
	move.w	(a0)+,d2
	move.w	(a0),d3
	subq	#3,d3			; diff doit etre > 4
	cmp.w	d2,d3
	bls	.fail
	cmp.w	d5,d2
	bhi	.fail
	cmp.w	d5,d3
	bhi	.fail
	dbf	d1,.smp
	addq	#2,a0
	dbf	d0,.up

	moveq	#0,d0
	moveq	#0,d7
.nico:
	move.b	(a0)+,d0		; take each patt_nb
	cmp.b	#$FF,d0			; $FF = fin table
	beq.s	.bye
	move.w	d0,d1
	lsr	#1,d1
	lsl	#1,d1
	cmp.w	d0,d1
	beq	.p50a			; sinon : Player 6.0a
.p60a:
	move.l	#'P60A',P5ou6
	cmpi.w	#$3F,d0			; et pas + de $3F patterns
	bhi	.fail
	bra.s	.pALL
.p50a:
	cmpi.w	#$7e,d1			; et pas + de $3F*2 = $7E patterns
	bhi	.fail
.pALL:
	addi.w	#1,d7
	cmpi.w	#$7F,d7			; si no $FF till $7F pos, fuck
	bhi	.fail
	bra.s	.nico
.bye:
	movea.l	a0,a2

	movea.l	a4,a0
	adda.w	(a0),a0
	adda.l	P50A_SampLen(pc),a0	; fin mod
	suba.l	a4,a0			; size
	sub.l	a0,d4			; difference
	cmpi.l	#-256,d4
	blt.s	.fail

; ----- Différencie P60 du P61

;	cmp.l	#'P60A',P5ou6
;	bne.s	.okay			; pas la peine, c'est un P50A ;)

	movea.l	a2,a0			; début des notes
	movea.l	a4,a1
	adda.w	(a1),a1			; fin des notes (debut des samples)
	subq.l	#4,a1			; on s'arrete un peu avant...
	moveq	#0,d2
.zLoop:
	cmp.l	a1,a0
	bhs	.zBye
	tst.b	(a0)+			; un 1er 00 ??
	bne.s	.zNext
	tst.b	(a0)			; un 2eme 00 ??
	bne.s	.zNext
	addq.l	#1,d2			; Inc d2 (0000 = P60A)
.zNext:	bra.s	.zLoop
.zBye:
	tst.l	d2
	beq	.Found_P61A		; aucun 0000 ===> P61A


; ----- 2eme série de tests

	movea.l	a2,a0			; début des notes
	movea.l	a4,a1
	adda.w	(a1),a1			; fin des notes (debut des samples)
	lea	-8(a1),a1		; on s'arrete un peu avant...
	moveq	#0,d2
.yLoop:
	cmp.l	a1,a0
	bhs	.yBye
	cmp.b	#$80,(a0)+		; un $80 ??
	bne.s	.yNext
	cmp.b	#$0F,(a0)+		; un $80 0F maximum ??
	bhi.s	.yNext
	moveq	#0,d5
	move.b	(a0)+,d5
	lsl	#8,d5			; prend l'offset
	move.b	(a0)+,d5
	move.l	a0,d7			; addy actuelle
	sub.l	d5,d7			; moins l'offset
	cmp.l	a2,d7
	bls.s	.yNext			; si plus petit ke le debut des notes, pas bon
	addq.l	#1,d2			; Inc d2 (0000 = P60A)
.yNext:	bra.s	.yLoop
.yBye:
	tst.l	d2
	bne.s	.okette			; aucun 0000, c un P61A !


.Found_P61A:
	move.l	#'P61A',P5ou6

.okette:

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#2,d0
	lsl.l	#8,d0
	adda.l	P50A_Dest_SampLen(pc),d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


P50A_Fin_Notes:	dc.l	0
P5ou6:		dc.l	'P50A'
P50A_Sign:	dc.w	1			; default = sign!


*-----------------------------------------------------------------------*
;
; Player 50A/60A to Protracker Convert routine, from Pro-Wizzy

ConvertP50A
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; Heading
	moveq	#0,d0
	move.w	P50A_Sign(pc),d0
	lsl	#2,d0			; *4 (longword)
	add.l	d0,a0

	move.l	a0,a4			; resauve!
	move.l	a0,a2
	adda.w	(a0),a2
	move.l	a2,P50A_Fin_Notes	; do not trepass !!

	move.l	P50A_Sinfo_Adr,a0

	lea	42(a1),a1
	moveq	#0,d0
	move.w	Nb_Samp,d0		; take nb_samples
	move.l	d0,d7
	subq	#1,d0
.entete:
	moveq	#0,d1
	move.w	(a0)+,d1		; take length
	bpl.s	.ok
	neg.w	d1			; sample IDENTIQUE !
	subq	#1,d1
	mulu	#6,d1
	move.l	P50A_Sinfo_Adr,a6
	move.w	(a6,d1.w),d1		; get length du VRAI sample
.ok:
	move.l	d1,d6
	move.w	d1,(a1)
	moveq	#0,d1
	move.w	(a0)+,d1
	andi.w	#$0FFF,d1		; vire le bit 7 if any...
	move.w	d1,2(a1)		; move finetune+volume
	moveq	#0,d1
	move.w	(a0)+,d1
	cmpi.w	#-1,d1			; $FFFF = no repeat, replen=1
	bne.s	.pas_si_facile
	move.l	#$00000001,4(a1)
	bra.s	.branch
.pas_si_facile:
	move.w	d1,4(a1)		; move REPEAT
	move.w	d6,d2			; reprend LENGTH
	sub.w	d1,d2
	move.w	d2,6(a1)		; move REPLEN
.branch:
	lea	30(a1),a1
	dbf	d0,.entete

	move.l	a0,P50A_Adr_Tracks

	cmpi.w	#$1F,d7
	beq.s	.no_need
	moveq	#30,d0
	sub.w	d7,d0
.loop1:
	move.l	#$0,(a1)
	move.l	#$1,4(a1)		; fill a vide le reste des samples...
	lea	30(a1),a1
	dbf	d0,.loop1

.no_need:
	moveq	#0,d0
	move.w	Nb_Patt,d0		; take nb_patt
	lea	$3b8(a5),a1
	move.l	P50A_Adr_Tracks(pc),a0
	lsl	#3,d0			; chak patt = 2 longwords
	add.l	d0,a0			; pointe sur table_patt, a /2
	moveq	#0,d7
.compte:
	moveq	#0,d1
	move.b	(a0)+,d1
	cmpi.w	#$FF,d1
	beq.s	.out
	cmp.w	#'P6',P5ou6		; P60 ou P61 ??
	beq.s	.saute
	lsr	#1,d1
  .saute:
	move.b	d1,(a1)+
	addq	#1,d7			; incremente nb_pos
	bra.s	.compte
.out:
	move.l	a0,P50A_Adr_Notes
	lea	$3b6(a5),a1
	move.b	d7,(a1)			; move nb_pos


P50A_Insert_Sample_Packing_Info:

	move.l	P60A_SampleType,d0
	cmpi.l	#'NORM',d0
	beq.s	P50A_Convert_Patterns	; normal samples : skip
	movea.l	a5,a1
	adda.l	#20+7*30,a1		; 8eme sample_name
	move.l	#'Samp',(a1)+
	move.l	#'les ',(a1)+
	move.l	#'were',(a1)+
	move.b	#' ',(a1)+
	cmpi.l	#'DELT',d0
	bne.s	.pk
	move.b	#'D',(a1)+
	move.l	#'ELTA',(a1)+
	move.b	#'!',(a1)+
	bra.s	P50A_Convert_Patterns
.pk:
	move.b	#'P',(a1)+
	move.l	#'ACKE',(a1)+
	move.w	#'D!',(a1)+

	movea.l	a4,a0
	moveq	#0,d0
	move.b	3(a0),d0		; pack byte
	cmpi.w	#$C0,d0
	blo.s	P50A_Convert_Patterns
	move.b	#'+',-1(a1)
	move.b	#'D',(a1)

P50A_Convert_Patterns:
	cmp.l	#'P61A',P5ou6
	beq	P61A_Convert_Patterns	; new P61A routines !!!

	clr.w	Track_Cpt
	lea	$43c(a5),a1
	movea.l	a4,a0
	moveq	#0,d0
	move.b	2(a0),d0		; take nb_patt
	lsl	#2,d0			; *4 voies
	subq	#1,d0
	move.l	P50A_Adr_Tracks(pc),a0
	move.l	P50A_Fin_Notes(pc),a6	; à ne pas dépasser !!
.Vloop:
	move.l	P50A_Adr_Notes(pc),a2
	add.w	(a0)+,a2
	moveq	#0,d7			; nb_notes.....till 64
.Nloop:
	cmp.l	a6,a2
	blo.s	.not_yet
	addq	#1,d7
	bra	.next_note
  .not_yet:
	moveq	#0,d1
	move.b	(a2)+,d1		; take 1er byte
	cmpi.w	#$80,d1
	beq.s	.copie_notes
	bgt.w	.multi_notes
.une_note:
	bsr	P50A_Get_Note		; d6 = 1er word pret
	moveq	#0,d1
	move.b	(a2)+,d1		; take 2eme byte
	lsl	#8,d1
	move.b	(a2)+,d1		; take 3eme byte
	bsr	P50A_Check_Cmd
	add.w	d1,d6			; longword final PRET !
	move.l	d6,(a1)
	lea	16(a1),a1		; note suivante (dest)
	addq	#1,d7			; Notes+1
	bra.w	.next_note
.copie_notes:
	moveq	#0,d3
	move.b	(a2)+,d3		; nb_notes a copier (deja DBFé)
	moveq	#0,d2
	move.b	(a2)+,d2		; take 3eme byte
	lsl	#8,d2
	move.b	(a2)+,d2		; take 4eme byte
	move.l	a2,a3			; copie adr
	sub.l	d2,a3			; revient sur les notes a copier
  .in_loop:
	moveq	#0,d1
	move.b	(a3)+,d1		; take 1er byte
	cmpi.w	#$80,d1
	bgt.w	.multi_notes2
	bsr	P50A_Get_Note		; d6 = 1er word PRET
	moveq	#0,d1
	move.b	(a3)+,d1		; take 2eme byte
	lsl.l	#8,d1
	move.b	(a3)+,d1		; take 3eme byte
	bsr	P50A_Check_Cmd
	add.w	d1,d6			; longword final PRET !
	move.l	d6,(a1)
	lea	16(a1),a1		; note suivante (dest)
	addq	#1,d7			; Notes+1
.come_back:
	dbf	d3,.in_loop
	bra.w	.next_note
.multi_notes:
	neg.b	d1
	andi.l	#$000000FF,d1
	subq	#1,d1
	bsr	P50A_Get_Note		; d6 = 1er word PRET
	moveq	#0,d1
	move.b	(a2)+,d1		; take 2eme byte
	lsl	#8,d1
	move.b	(a2)+,d1		; take 3eme byte
	bsr	P50A_Check_Cmd
	add.w	d1,d6
	move.l	d6,d5			; copie cmd pour plus bas
	move.l	d6,(a1)
	lea	16(a1),a1
	addq	#1,d7
	moveq	#0,d1			; Traitement de la copie de notes !
	move.b	(a2)+,d1		; take 4eme byte (nb_copies!)
	cmpi.w	#$40,d1
	bgt.s	.must_neg
	subq	#1,d1			; pour le dbf
  .cop_loop:				; copie 0000
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.cop_loop
	bra.w	.next_note
.must_neg:
	neg.b	d1
	andi.l	#$000000FF,d1
	subq	#1,d1			; pour le dbf
  .cmd_copy:
	move.l	d5,(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.cmd_copy
	bra.w	.next_note
.multi_notes2:
	neg.b	d1
	andi.l	#$000000FF,d1
	subq	#1,d1
	bsr	P50A_Get_Note		; d6 = 1er word PRET
	moveq	#0,d1
	move.b	(a3)+,d1		; take 2eme byte
	lsl	#8,d1
	move.b	(a3)+,d1		; take 3eme byte
	bsr	P50A_Check_Cmd
	add.w	d1,d6
	move.l	d6,d5			; copie cmd pour plus bas
	move.l	d6,(a1)
	lea	16(a1),a1
	addq	#1,d7
	moveq	#0,d1			; Traitement de la copie de notes !
	move.b	(a3)+,d1		; take 4eme byte (nb_copies!)
	cmpi.w	#$40,d1
	bgt.s	.must_neg2
	subq	#1,d1			; pour le dbf
  .cop_loop2:				; copie 0000
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.cop_loop2
	bra.w	.come_back
.must_neg2:
	neg.b	d1
	andi.l	#$000000FF,d1
	subq	#1,d1			; pour le dbf
  .cmd_copy2:
	move.l	d5,(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.cmd_copy2
	bra.w	.come_back
.next_note:
	cmpi.w	#$40,d7			; voie achevée ??
	blt.w	.Nloop			; non
	subi.w	#$40,d7
	mulu	#16,d7
	sub.l	d7,a1
	moveq	#0,d7			; oui!
	add.w	#1,Track_Cpt
	cmpi.w	#4,Track_Cpt
	blt.s	.voie_suiv
	clr.w	Track_Cpt
	suba.l	#12,a1			; pattern suivant !
	bra.s	.next_patt
.voie_suiv:
	suba.l	#64*16,a1
	addq	#4,a1
.next_patt:
	dbf	d0,.Vloop

P50A_Copy_Samples:
	movea.l	a4,a0
	add.w	(a0),a0			; pointe sur les samples source

	lea	$43c(a5),a2
	moveq	#0,d2
	move.w	Nb_Patt,d2
	lsl	#8,d2			; *$100
	lsl.l	#2,d2			; *$4
	add.l	d2,a2			; pointe sur les samples dest

; ----- sample conversion

	movea.l	a4,a1
	moveq	#0,d5
	move.b	3(a1),d5			; pack info

	move.l	a2,a1				; dest for sample
	lea	P50A_Sinfo,a2			; table
	move.l	P50A_Fin_Notes(pc),d6		; debut des samples
	moveq	#0,d7
	move.w	Nb_Samp,d7
	bra.s	.same

.sam1	move.l	(a2)+,a0			; source mem
	add.l	d6,a0
	move.w	(a2)+,d1			; length
	tst.w	(a2)+				; d-4 packed?
	addq.l	#4,a2
	bpl.s	.copy8

	moveq.l	#0,d2				; delta-4 coding...
	moveq.l	#0,d3
	bra.s	.fibe
.fibs	move.b	(a0)+,d3
	moveq.l	#$0f,d4
	and.w	d3,d4
	lsr.w	#4,d3
	sub.b	.d4tab(pc,d3.w),d2
	move.b	d2,(a1)+
	sub.b	.d4tab(pc,d4.w),d2
	move.b	d2,(a1)+
.fibe	dbf	d1,.fibs
	bra.s	.same

.cops	bsr	P50A_copysample
	bra.s	.same

.copy8	tst.b	d5
	bpl.s	.cops

	subq.w	#1,d1				; delta-8 coding...
	bcs.s	.same
	move.b	(a0)+,d2
	move.b	d2,(a1)+
	sub.b	(a0)+,d2
	move.b	d2,(a1)+
	bra.s	.dele
.delt	sub.b	(a0)+,d2
	move.b	d2,(a1)+
	sub.b	(a0)+,d2
	move.b	d2,(a1)+
.dele	dbf	d1,.delt

.same	dbf	d7,.sam1


	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts

.d4tab	dc.b	0,1,2,4,8,16,32,64,128,-64,-32,-16,-8,-4,-2,-1


P50A_Get_Note:
	moveq	#0,d6
	move.l	d1,d2
	lsr	#1,d2
	lsl	#1,d2
	cmp.w	d1,d2
	beq.s	.deja_pair
	move.l	#$00001000,d6		; prepare longword pour smpl>$10
	move.l	d2,d1
  .deja_pair:
	tst.w	d1			; no note ?
	beq.s	.no_note
	move.l	a6,-(sp)
	lea	mt_periodtable,a6
	subq	#2,d1
	move.w	(a6,d1.w),d1		; take real note
	move.l	(sp)+,a6
  .no_note:
	add.w	d1,d6
	swap	d6
	rts

P50A_Check_Cmd:
	move.l	d1,d2
	andi.l	#$00000FFF,d2
  .cmd_E00:
	cmpi.w	#$0E00,d2
	bne.s	.cmd_E02
	move.b	#$01,d1			; force $E01 (filtre off)
	rts
  .cmd_E02:
	cmpi.w	#$0E02,d2
	bne.s	.cmd_5_6_A
	move.b	#$01,d1			; force $E01 (filtre off)
	rts
  .cmd_5_6_A:
	move.l	d2,d4
  	andi.w	#$0F00,d4
  	cmpi.w	#$0500,d4
	beq.s	.test_byte
  	cmpi.w	#$0600,d4
	beq.s	.test_byte
  	cmpi.w	#$0A00,d4
	beq.s	.test_byte
  .cmd_8:
  	cmpi.w	#$0800,d4
  	beq.s	.arpeggio
	rts
  .arpeggio:
	andi.l	#$FFFFF0FF,d1		; vire cmd
	rts
.test_byte:
	andi.w	#$00FF,d2
	cmpi.w	#$F0,d2
	bgt.s	.must_neg
	rts
  .must_neg:
	neg.b	d2
	lsl	#4,d2			; decale
	move.b	d2,d1			; write_it
	rts


;------------------------------------------------------------------- P61A

P61A_Convert_Patterns:
	clr.w	Track_Cpt
	lea	$43c(a5),a1
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#2,d0			; *4 voies
	subq	#1,d0
	move.l	P50A_Adr_Tracks(pc),a0
	move.l	P50A_Fin_Notes(pc),a6	; à ne pas dépasser !!
.Vloop:
	move.l	P50A_Adr_Notes,a2
	add.w	(a0)+,a2
	moveq	#0,d7			; nb_notes.....till 64
.Nloop:
	cmp.l	a6,a2
	blo.s	.not_yet
	addq	#1,d7
	bra	.next_note
  .not_yet:
	moveq	#0,d1
	move.b	(a2)+,d1		; take 1er byte
	cmpi.w	#$7F,d1
	beq.s	.rien_du_tout
	cmpi.w	#$60,d1
	blo.s	.une_note_simple
	cmpi.w	#$80,d1
	blo.s	.test6ou7
	cmpi.w	#$D0,d1
	blo	.multi_notes
	cmpi.w	#$F0,d1
	blo	.multi_efx
	cmpi.w	#$FF,d1
	blo	.multi_notes_seules
	bra	.copie_notes
.test6ou7:
	move.l	d1,d2
	andi.b	#$F0,d2
	cmpi.w	#$60,d2
	beq.s	.only_efx
;	cmpi.w	#$70,d2
	bra.s	.note_sans_efx


.rien_du_tout:				; =====> $7F <===== (no note)
	moveq	#0,d6
	bra	.insere_note
.only_efx:				; =====> 6 310 <===== (6 + efx)
	moveq	#0,d6
	move.b	d1,d6			; reprend le 1er byte
	lsl	#8,d6			; décale de 2
	move.b	(a2)+,d6		; prend le second byte
	andi.l	#$00000FFF,d6		; garde que l'efx
	move.l	d6,d1
	bsr	P50A_Check_Cmd
	move.l	d1,d6
	bra	.insere_note
.note_sans_efx:				; =====> 7 32 8 <===== (7 + note + smp)
	lsl	#8,d1
	move.b	(a2)+,d1
	andi.w	#$0FFF,d1		; vire le 7
	move.l	d1,d3
	lsr	#4,d1
	bsr	P50A_Get_Note		; d6 = 1er word pret
	andi.w	#$000F,d3		; garde ke le sample
	ror.w	#4,d3
	add.w	d3,d6
	bra	.insere_note
.une_note_simple:			; ===> 1A 5F05 <=== (note + smp + efx)
	bsr	P50A_Get_Note
	moveq	#0,d1
	move.b	(a2)+,d1		; take 2eme byte
	lsl	#8,d1
	move.b	(a2)+,d1		; take 3eme byte
	bsr	P50A_Check_Cmd
	add.w	d1,d6			; longword final PRET !
.insere_note:
	move.l	d6,(a1)
	lea	16(a1),a1		; note suivante (dest)
	addq	#1,d7			; Notes+1
	bra.w	.next_note

.multi_notes:				; ===> A2 1602 84 <===
	subi.w	#$80,d1
	bsr	P50A_Get_Note		; d6 = 1er word PRET
	moveq	#0,d1
	move.b	(a2)+,d1		; take 2eme byte
	lsl	#8,d1
	move.b	(a2)+,d1		; take 3eme byte
	bsr	P50A_Check_Cmd
	add.w	d1,d6
	move.l	d6,d5			; copie cmd pour plus bas
	move.l	d6,(a1)
	lea	16(a1),a1
	addq	#1,d7

	moveq	#0,d1			; Traitement de la copie de notes !
	move.b	(a2)+,d1		; take 4eme byte (nb_copies!)
	bmi.s	.must_neg		; si >$80 : copie la note
	subq	#1,d1			; pour le dbf
  .copy_000:				; sinon, copie 00000000
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy_000
	bra.w	.next_note
.must_neg:
	subi.w	#$80,d1
	subq	#1,d1			; pour le dbf
  .copy_cmd:
	move.l	d5,(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy_cmd
	bra.w	.next_note

.multi_notes_seules:			; ===> F 1A 7 04 <=== (note+smp+repeat)
	lsl	#8,d1
	move.b	(a2)+,d1
	andi.w	#$0FFF,d1		; vire le F
	move.l	d1,d3
	lsr	#4,d1
	bsr	P50A_Get_Note		; d6 = 1er word pret
	andi.w	#$000F,d3		; garde ke le sample
	ror.w	#4,d3
	add.w	d3,d6
	move.l	d6,d5			; copie cmd pour plus bas
	move.l	d6,(a1)
	lea	16(a1),a1
	addq	#1,d7

	moveq	#0,d1			; Traitement de la copie de notes !
	move.b	(a2)+,d1		; take 4eme byte (nb_copies!)
	bmi.s	.must_neg8		; si >$80 : copie la note
	subq	#1,d1			; pour le dbf
  .copy8_000:				; sinon, copie 00000000
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy8_000
	bra.w	.next_note
.must_neg8:
	subi.w	#$80,d1
	subq	#1,d1			; pour le dbf
  .copy8_cmd:
	move.l	d5,(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy8_cmd
	bra.w	.next_note

.multi_efx:				; ===> E 602 84 <===
	lsl	#8,d1
	move.b	(a2)+,d1
	andi.w	#$0FFF,d1		; vire le E
	bsr	P50A_Check_Cmd
	moveq	#0,d6
	add.w	d1,d6
	move.l	d6,d5			; copie cmd pour plus bas
	move.l	d6,(a1)
	lea	16(a1),a1
	addq	#1,d7

	moveq	#0,d1			; Traitement de la copie de notes !
	move.b	(a2)+,d1		; take 4eme byte (nb_copies!)
	bmi.s	.must_neg9		; si >$80 : copie la note
	subq	#1,d1			; pour le dbf
  .copy9_000:				; sinon, copie 00000000
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy9_000
	bra.w	.next_note
.must_neg9:
	subi.w	#$80,d1
	subq	#1,d1			; pour le dbf
  .copy9_cmd:
	move.l	d5,(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy9_cmd
	bra.w	.next_note

.multi_zero:
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d5,.multi_zero
	bra	.next_note

.copie_notes:				; ===> FF 01 ou FF 41 09 ou FF CB 0153 <===
	moveq	#0,d5
	move.b	(a2)+,d5		; nb_notes a copier (deja DBFé)
	cmpi.w	#$40,d5
	blo.s	.multi_zero
	cmpi.w	#$80,d5
	blo.s	.cn1
	subi.w	#$C0,d5			; enlève $C0
	moveq	#0,d2
	move.b	(a2)+,d2		; take 3eme byte
	lsl	#8,d2
	move.b	(a2)+,d2		; take 4eme byte
	bra.s	.cnall
.cn1:
	subi.w	#$40,d5
	moveq	#0,d2
	move.b	(a2)+,d2		; take 3eme byte
.cnall:
	move.l	a2,a3			; copie adr
	sub.l	d2,a3			; revient sur les notes a copier

	movem.l	d0/a0/a2/a6,-(sp)
  .in_loop:
	move.l	d5,-(sp)

	moveq	#0,d1
	move.b	(a3)+,d1		; take 1er byte
	cmpi.w	#$7F,d1
	beq	.rien_du_tout_II
	cmpi.w	#$60,d1
	blo	.une_note_simple_II
	cmpi.w	#$80,d1
	blo.s	.test6ou7_II
	cmpi.w	#$D0,d1
	blo	.multi_notes_II
	cmpi.w	#$F0,d1
	blo	.multi_efx_II
	cmpi.w	#$FF,d1
	blo	.multi_notes_seules_II
	bra	.copie_notes_II
.test6ou7_II:
	move.l	d1,d2
	andi.b	#$F0,d2
	cmpi.w	#$60,d2
	beq.s	.only_efx_II
;	cmpi.w	#$70,d2
	bra.s	.note_sans_efx_II

.come_back:
	move.l	(sp)+,d5
	dbf	d5,.in_loop
;	bra.w	.next_note

	movem.l	(sp)+,d0/a0/a2/a6

.next_note:
	cmpi.w	#$40,d7			; voie achevée ??
	blt.w	.Nloop			; non
	subi.w	#$40,d7
	mulu	#16,d7
	sub.l	d7,a1
	moveq	#0,d7			; oui!
	add.w	#1,Track_Cpt
	cmpi.w	#4,Track_Cpt
	blt.s	.voie_suiv
	clr.w	Track_Cpt
	suba.l	#12,a1			; pattern suivant !
	bra.s	.next_patt
.voie_suiv:
	suba.l	#64*16,a1
	addq	#4,a1
.next_patt:
	dbf	d0,.Vloop

	bra	P61A_Finiiii

.rien_du_tout_II:			; =====> $7F <===== (no note)
	moveq	#0,d6
	bra	.insere_note_II
.only_efx_II:				; =====> 6 310 <===== (6 + efx)
	moveq	#0,d6
	move.b	d1,d6			; reprend le 1er byte
	lsl	#8,d6			; décale de 2
	move.b	(a3)+,d6		; prend le second byte
	andi.l	#$00000FFF,d6		; garde que l'efx
	move.l	d6,d1
	bsr	P50A_Check_Cmd
	move.l	d1,d6
	bra	.insere_note_II
.note_sans_efx_II:			; =====> 7 32 8 <===== (7 + note + smp)
	lsl	#8,d1
	move.b	(a3)+,d1
	andi.w	#$0FFF,d1		; vire le 7
	move.l	d1,d3
	lsr	#4,d1
	bsr	P50A_Get_Note		; d6 = 1er word pret
	andi.w	#$000F,d3		; garde ke le sample
	ror.w	#4,d3
	add.w	d3,d6
	bra	.insere_note_II
.une_note_simple_II:			; ===> 1A 5F05 <=== (note + smp + efx)
	bsr	P50A_Get_Note
	moveq	#0,d1
	move.b	(a3)+,d1		; take 2eme byte
	lsl	#8,d1
	move.b	(a3)+,d1		; take 3eme byte
	bsr	P50A_Check_Cmd
	add.w	d1,d6			; longword final PRET !
.insere_note_II:
	move.l	d6,(a1)
	lea	16(a1),a1		; note suivante (dest)
	addq	#1,d7			; Notes+1
	bra.w	.come_back

.multi_notes_II:			; ===> B2 1602 84 <===
	subi.w	#$80,d1
	bsr	P50A_Get_Note		; d6 = 1er word PRET
	moveq	#0,d1
	move.b	(a3)+,d1		; take 2eme byte
	lsl	#8,d1
	move.b	(a3)+,d1		; take 3eme byte
	bsr	P50A_Check_Cmd
	add.w	d1,d6
	move.l	d6,d5			; copie cmd pour plus bas
	move.l	d6,(a1)
	lea	16(a1),a1
	addq	#1,d7

	moveq	#0,d1			; Traitement de la copie de notes !
	move.b	(a3)+,d1		; take 4eme byte (nb_copies!)
	bmi.s	.must_neg_II		; si >$80 : copie la note
	subq	#1,d1			; pour le dbf
  .copy_000_II:				; sinon, copie 00000000
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy_000_II
	bra.w	.come_back
.must_neg_II:
	subi.w	#$80,d1
	subq	#1,d1			; pour le dbf
  .copy_cmd_II:
	move.l	d5,(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy_cmd_II
	bra.w	.come_back

.multi_notes_seules_II:			; ===> F 1A 7 04 <=== (note+smp+repeat)
	lsl	#8,d1
	move.b	(a3)+,d1
	andi.w	#$0FFF,d1		; vire le F
	move.l	d1,d3
	lsr	#4,d1
	bsr	P50A_Get_Note		; d6 = 1er word pret
	andi.w	#$000F,d3		; garde ke le sample
	ror.w	#4,d3
	add.w	d3,d6
	move.l	d6,d5			; copie cmd pour plus bas
	move.l	d6,(a1)
	lea	16(a1),a1
	addq	#1,d7

	moveq	#0,d1			; Traitement de la copie de notes !
	move.b	(a3)+,d1		; take 4eme byte (nb_copies!)
	bmi.s	.must_neg8_II		; si >$80 : copie la note
	subq	#1,d1			; pour le dbf
  .copy8_000_II:			; sinon, copie 00000000
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy8_000_II
	bra.w	.come_back
.must_neg8_II:
	subi.w	#$80,d1
	subq	#1,d1			; pour le dbf
  .copy8_cmd_II:
	move.l	d5,(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy8_cmd_II
	bra.w	.come_back

.multi_efx_II:				; ===> E 602 84 <===
	lsl	#8,d1
	move.b	(a3)+,d1
	andi.w	#$0FFF,d1		; vire le E
	bsr	P50A_Check_Cmd
	moveq	#0,d6
	add.w	d1,d6
	move.l	d6,d5			; copie cmd pour plus bas
	move.l	d6,(a1)
	lea	16(a1),a1
	addq	#1,d7

	moveq	#0,d1			; Traitement de la copie de notes !
	move.b	(a3)+,d1		; take 4eme byte (nb_copies!)
	bmi.s	.must_neg9_II		; si >$80 : copie la note
	subq	#1,d1			; pour le dbf
  .copy9_000_II:			; sinon, copie 00000000
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy9_000_II
	bra.w	.come_back
.must_neg9_II:
	subi.w	#$80,d1
	subq	#1,d1			; pour le dbf
  .copy9_cmd_II:
	move.l	d5,(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d1,.copy9_cmd_II
	bra.w	.come_back

.multi_zero_II:
	clr.l	(a1)
	lea	16(a1),a1
	addq	#1,d7
	dbf	d5,.multi_zero_II
	bra	.come_back

.copie_notes_II:			; ===> FF 01 ou FF 41 09 ou FF CB 0153 <===
	moveq	#0,d5
	move.b	(a3)+,d5		; nb_notes a copier (deja DBFé)
	cmpi.w	#$40,d5
	blo.s	.multi_zero_II
	bra.s	.multi_zero_II


P61A_Finiiii:
	bra	P50A_Copy_Samples



; copy a sample (a0->a1, length d1.w)
; a0 may be odd (p6 guy..)
; returns a0 and a1!

P50A_copysample
	and.l	#$ffff,d1			; word arg only

;copymem
	tst.l	d1
	beq.s	.none
	move.l	d0,-(sp)
	move.l	a0,d0
	and.w	#1,d0
	bne.s	.noalg				; source not aligned
	bra.s	.copy16
.movit	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
.copy16	subq.l	#8,d1
	bhs.s	.movit

;						; -8..-1
	addq.w	#7,d1				; -1..6
	bmi.s	.fini
.doit	move.w	(a0)+,(a1)+
	dbf	d1,.doit

.fini	move.l	(sp)+,d0
.none	rts

; will need some '020 'optimizations'...

.dona	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
.noalg	subq.l	#4,d1
	bhs.s	.dona
;						; -4..-1
	addq.w	#3,d1				; -1..2
	bmi.s	.fini
.doma	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	dbf	d1,.doma
	bra.s	.fini


P50A_Adr_Tracks:	dc.l	0
P50A_Adr_Notes:		dc.l	0
Save_Pack:		dc.l	0
Save_a0:		dc.l	0
Track_Cpt:		dc.w	0
Specopy:		dc.w	0


*-----------------------------------------------------------------------*
;
; ProRunner 1.0 Check Routine

CheckPRU1:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	42(a0),a0
	moveq	#0,d6
	moveq	#30,d0
.RTloop:
	moveq	#0,d1
	move.w	(a0),d1			; take length
	cmpi.w	#$8000,d1
	bhi	.fail
	add.l	d1,d6			; cumule
	moveq	#0,d3
	move.w	2(a0),d3
	move.l	d3,d2
	andi.w	#$00FF,d3		; isole volume !
	cmpi.w	#$0040,d3
	bhi	.fail			; volume > $40 ??? fuck !!
	andi.w	#$FF00,d2		; isole finetune !
	cmpi.w	#$0F00,d2
	bhi	.fail			; finetune > $0F ??? fuck !!
	tst.w	d1
	beq.s	.next2
	move.w	4(a0),d2
	add.w	6(a0),d2
	cmp.w	d1,d2
	bhi	.fail
.next2:
	lea	30(a0),a0
	dbf	d0,.RTloop

	lsl.l	#1,d6
	beq	.fail
	move.l	d6,SampLen

; How_Many_Patterns:
	lea	$3b6(a4),a0
	moveq	#0,d0
	move.b	(a0),d0			; nb de patterns
	move.w	d0,Nb_Pos
	addq	#2,a0			; pointe sur $3b8
	subq	#1,d0			; pour le DBF
	moveq	#0,d1
.Ploop:
	moveq	#0,d2
	move.b	(a0)+,d2
	cmpi.b	#$3f,d2
	bhi	.fail
	cmp.b	d1,d2
	ble.s	.next
	move.b	d2,d1
.next:
	dbf	d0,.Ploop
	addq	#1,d1
	move.w	d1,Nb_Patt
	lsl	#8,d1
	subq.l	#1,d1
	move.l	d1,Bigest_Patt

; Check_si_depasse:
	lea	$43c(a4),a0
	move.l	d1,d0
	addq.l	#1,d0
	lsl.l	#2,d0
	add.l	d0,a0
	add.l	SampLen,a0		; fin du pack_mod
	sub.l	a4,a0			; size
	sub.l	a0,d4
	cmpi.l	#-256,d4
	blt	.fail

; PRU1_Check1:
	lea	$43c(a4),a0
	moveq	#0,d3
	move.l	Bigest_Patt,d0		; scan TOUS les patterns !!
.loop:
	move.l	(a0)+,d1
	beq.s	.Nnext
	move.l	d1,d2
	move.l	d1,d5
	andi.l	#$0000F000,d5		; garde ce qui doit etre NUL
	bne	.fail
	andi.l	#$00FF0000,d1		; garde la note
	andi.l	#$FF000000,d2		; garde le smpl_nb
	swap	d1
	swap	d2
	cmpi.w	#$0024,d1		; ne doit pas depasser $24 !
	bhi	.fail			; $24 = 36 notes maxi
	cmpi.w	#$1F00,d2		; ne doit pas depasser $1F !
	bhi	.fail
	moveq	#-1,d3
.Nnext:
	dbf	d0,.loop
	tst.w	d3
	beq	.fail			; si d3 encore à 00, no notes : FUCK


; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; ProRunner 1.0 to Protracker Convert routine, from Pro-Wizzy

ConvertPRU1:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	movea.l	a0,a4
	movea.l	a1,a5

; Heading
	move.l	#$43b,d0
.rond:
	move.b	(a0)+,(a1)+
	dbf	d0,.rond

	cmpi.w	#$31,Format_nb		; Hornet ?
	bne.s	.zou1
	lea	38(a5),a1
	moveq	#30,d0
.arf:	clr.l	(a1)			; efface les adresses absolues
	lea	30(a1),a1
	dbf	d0,.arf

.zou1:
	lea	$43c(a5),a1
	move.l	Bigest_Patt,d0		;	   sample  note
.loop:					;		|  |
	move.l	(a0)+,d1		; take longwd ( 02 19 0C 18  par ex)
	move.l	d1,d2			;		       |
	swap	d2			;		       cmd

	cmpi.w	#$31,Format_nb		; Hornet ?
	bne.s	.zou2
	moveq	#0,d3
	move.b	d2,d3
	lsr	#1,d3			; divise par 2 pour Hornet !!
	move.b	d3,Note_Number
	andi.w	#$FF00,d2
	ror.w	#8,d2
	lsr	#1,d2			; divise par 2 pour Hornet !!
	move.b	d2,WN_Sample_number
	bra.s	.zou3

.zou2:	move.b	d2,Note_Number
	andi.w	#$FF00,d2
	ror.w	#8,d2
	move.b	d2,WN_Sample_number

.zou3:	cmp.b	#$10,WN_Sample_number	; sample > $10 ??
	bge.s	.decoupe
	moveq	#0,d4
	bra.s	.branch
.decoupe:
	subi.b	#$10,WN_Sample_number	; = $03 si yavait $13
	move.l	#$1000,d4
.branch:
	moveq	#0,d3
	move.b	Note_Number,d3
	bne.s	.suite			; pas de note ?
	move.w	#$0000,Real_Note
	bra.s	.suite1
.suite:
	subq	#1,d3
	lsl.w	#1,d3
	lea	mt_periodtable,a2	; trouve real_note
	move.w	(a2,d3),Real_Note
.suite1:
	add.w	d4,Real_Note
	moveq	#0,d3			; prepare real_sample_nb
	move.b	WN_Sample_number,d3
	ror.w	#4,d3			; $0001 devient $1000

	move.l	d1,d2
	swap	d2
	move.w	Real_Note,d2		; write real_note
	swap	d2
	add.w	d3,d2			; mix sample_nb avec cmd

	move.l	d2,(a1)+		; re-write real longwd
	dbf	d0,.loop

; PRU1_Calc_Length:
	move.l	a0,a3			; debut des samples_source
	move.l	a1,a2			; debut des samples_dest
	add.l	SampLen,a3		; fin des sons
.copy:
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts


*-----------------------------------------------------------------------*
;
; ProRunner 2.0 Check Routine

CheckPRU2:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	8(a0),a0		; pointe sur sample_data
	moveq	#0,d6
	moveq	#30,d0
.loop:
	moveq	#0,d1
	move.w	(a0),d1
	cmpi.w	#$8000,d1
	bhi	.fail
	add.l	d1,d6			; cumul lengths
	moveq	#0,d2
	move.w	2(a0),d2
	move.l	d2,d3
	andi.w	#$FF00,d2		; garde que le finetune
	andi.w	#$00FF,d3		; garde que le volume
	cmpi.w	#$0F00,d2		; finetune ne depasse pas $0F
	bhi	.fail
	cmpi.w	#$0040,d3		; volume ne depasse pas $40
	bhi	.fail
	tst.w	d1
	beq.s	.next
	move.w	4(a0),d3
	add.w	6(a0),d3
	cmp.w	d1,d3
	bhi	.fail
.next:
	lea	8(a0),a0
	dbf	d0,.loop

	lsl.l	#1,d6
	beq	.fail
	move.l	d6,SampLen

	move.l	a4,a0
	add.l	4(a0),a0
	add.l	d6,a0
	sub.l	a4,a0
	sub.l	a0,d4
	cmpi.l	#-256,d4
	blt	.fail

.Check2:
	lea	$100(a4),a0		; pointe sur patt_table
	moveq	#0,d1
	move.w	(a0)+,d1
	move.l	d1,d2
	andi.w	#$00FF,d1
	cmpi.w	#$007F,d1		; pas de constante ??
;	bne	.fail			; removed to get pru2.Infect-17 to work
	andi.w	#$FF00,d2		; garde nb_pos
	cmpi.w	#$7F00,d2
	bhi	.fail
	cmpi.w	#$0100,d2
	blo	.fail
	lsr	#8,d2
	move.w	d2,Nb_Pos
	subq	#1,d2
	moveq	#0,d1
.loop2:
	moveq	#0,d0
	move.b	(a0)+,d0
	cmpi.b	#$3F,d0			; patt_nbr ne depasse pas $3F (63)
	bhi	.fail
	cmp.w	d1,d0
	ble.s	.no
	move.w	d0,d1
.no:
	dbf	d2,.loop2
	addq	#1,d1
	move.w	d1,Nb_Patt

.Check3:
	lea	$282(a4),a0		; 1ere adr_track = 0000 !
	tst.w	(a0)
	bne	.fail

	cmpi.w	#2,d1
	blo.s	.mince
	move.l	4(a4),d4		; sample_offset
	subq	#2,d1
  .grd:
	moveq	#0,d2
	moveq	#0,d3
	move.w	(a0)+,d2		; le plus petit
	move.w	(a0),d3			; le plus grand
	cmp.w	d4,d3
	bhs	.fail			; ne pas depasser !
	cmp.w	d3,d2
	bhs	.fail
	dbf	d1,.grd
.mince:
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; ProRunner 2.0 to Protracker Convert routine, from Pro-Wizzy

ConvertPRU2:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; Heading
	lea	8(a0),a0
	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop

	lea	$100(a4),a0		; pointe sur nb_pos+cnste
	lea	$3B6(a5),a1
	move.w	(a0)+,(a1)+		; copie nb_pos + constante ($7F)
	moveq	#$1F,d0
.plop:
	move.l	(a0)+,(a1)+
	dbf	d0,.plop

; PRU2_Conv:
	lea	$282(a4),a0
	move.w	#$0000,(a0)		; Force le 1er pattern à 0
	lea	$43c(a5),a2
	moveq	#0,d5
	move.w	Nb_Patt,d5
	subq	#1,d5
.Ploop:
	lea	$302(a4),a1
	add.w	(a0)+,a1
	moveq	#63,d6
.Vloop:
	moveq	#3,d7
.Nloop:
	moveq	#0,d1
	move.b	(a1),d1
	cmp.w	#$80,d1
	beq.w	.rien
	cmp.w	#$c0,d1
	beq.w	.copy_prec
.note:
	move.b	(a1)+,d1
	rol.l	#8,d1
	move.b	(a1)+,d1
	rol.l	#8,d1
	move.b	(a1)+,d1

	move.l	d1,d2
	andi.w	#$0000,d2
	swap	d2
	move.l	d2,d3
	lsr.l	#1,d2
	lsl.l	#1,d2
	cmp.w	d2,d3
	beq.s	.ok
	move.l	#$1000,d3
	bra.s	.jp
.ok:
	moveq	#0,d3
.jp:
	subq	#2,d2
	lea	mt_periodtable,a6
	move.w	(a6,d2.w),d2

	swap	d2
	move.l	d1,d4
	andi.l	#$0000F000,d4		; isole sample_nbr
	rol.w	#4,d4			; 5000 = 0005
	mulu	#2,d4			; fois 2
	clr.l	Prepare
	cmpi.w	#$0010,d4		; superieur ou = à $10 ??
	blt.s	.bon
	subi.w	#$0010,d4
	move.l	#$10000000,Prepare
.bon:
	ror.w	#4,d4
	andi.l	#$00000FFF,d1
	add.w	d4,d1
	add.l	d3,d1
	add.l	d2,d1
	add.l	Prepare(pc),d1

	move.l	d1,(a2)+
	beq.s	.suiv
	lea	PrecTab,a3
	move.l	PrecNum(pc),d2
	move.l	d1,(a3,d2.w)
	bra.s	.suiv
.copy_prec:
	addq	#1,a1
	lea	PrecTab,a3
	move.l	PrecNum(pc),d2
	move.l	(a3,d2.w),(a2)+
	bra.s	.suiv
.rien:
	addq	#1,a1
	clr.l	(a2)+
.suiv:
	add.l	#4,PrecNum
	dbf	d7,.Nloop	
	clr.l	PrecNum
	dbf	d6,.Vloop
	dbf	d5,.Ploop

; PRU2_Copy_Sample:
	lea	$43c(a5),a1		; pointe sur les dest_patterns
	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl	#8,d1
	lsl.l	#2,d1			; mulu #$400
	add.l	d1,a1			; pointe sur les samples (dest)

	move.l	a4,a0
	add.l	4(a0),a0		; Pointe sur les sons (source)
	move.l	a0,a3
	add.l	SampLen,a3		; fin des sons
.copy:					; donc pointe là où mettre les samples
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts

Prepare:dc.l	0
PrecNum:dc.l	0
PrecTab:ds.l	8


*-----------------------------------------------------------------------*
;
; ProPacker 1.0 (SB) Check Routine

CheckPP10:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	$f8(a0),a0
	bsr	Crypto_PP_Same
	tst.l	d7
	beq	.fail

	moveq	#0,d0
	move.w	$17a(a4),d0
	beq	.fail

	lea	$fa(a4),a0
	moveq	#0,d2
	move.l	#$1ff,d0
  .loop1:
	moveq	#0,d1
	move.b	(a0)+,d1
	cmp.w	d2,d1
	ble.s	.non
	move.w	d1,d2
.non:
	dbf	d0,.loop1
	addq	#1,d2			; nb tracks !
	lsl.l	#6,d2			; *64 notes.l
	move.l	d2,Nb_PPTracks

	subq.l	#1,d2
.verify:
	move.l	(a0)+,d1		; take ce ki doit etre une note !
	andi.l	#$FFFF0000,d1		; isole la note
	swap	d1
	tst.w	d1
	beq.s	.nexta
	cmpi.w	#$0071,d1
	blo.w	.fail			; si une seule note est < $71, fuck!
	cmpi.w	#$1358,d1
	bhi.w	.fail
.nexta:
	dbf	d2,.verify

	lea	$2fa(a4),a0
	move.l	Nb_PPTracks,d0
	lsl.l	#2,d0
	add.l	d0,a0
	add.l	d5,a0			; fin du pack_mod
	sub.l	a4,a0
	sub.l	a0,d4
	cmpi.l	#-256,d4
	blt	.fail

; PP10_Copy_Trax:
	lea	PP21_Table,a0		; EFFACE les Save_Tables
	moveq	#127,d0
.efface:
	clr.l	(a0)+
	dbf	d0,.efface

	lea	$fa(a4),a6
	moveq	#0,d0
	move.b	-2(a6),d0
	move.w	d0,Nb_Pos
	lea	PP21_Table,a5
	moveq	#0,d2			; compteur
	subq	#1,d0
.lo:
	move.b	$000(a6),(a5)+
	move.b	$080(a6),(a5)+
	move.b	$100(a6),(a5)+
	move.b	$180(a6),(a5)+
	addq	#1,a6
	dbf	d0,.lo

; PP10_PTK_Table:
	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; skip 1er pattern
	lea	PP21_Table+4,a0
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend longword de chak patt
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	lea	PP21_Table,a1		; table de comparaison
	lea	KR_PosTable,a3		; PTK Table
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos(pc),(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	cmpi.w	#$40,d6
	bhi	.fail

; Calc PTK Module Size

	move.l	d6,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts

Nb_PPTracks:	dc.l	0
Nb_Trax:	dc.l	0
PP21_Type:	dc.w	1

*-----------------------------------------------------------------------*
;
; ProPacker 1.0 (SB) to Protracker Convert routine, from Pro-Wizzy

ConvertPP10:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; Heading
	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop
	lea	$3b6(a5),a1
	move.b	(a0),(a1)

PP10_Cree_PTK_Table:
	lea	KR_PosTable,a0
	lea	$3b8(a5),a1
	moveq	#$7f,d0
.loop:
	move.b	(a0)+,(a1)+
	dbf	d0,.loop

PP10_Convert_Patterns:
	lea	KR_PosTable,a6
	lea	PP21_Table,a1
	lea	$43c(a5),a2
	moveq	#-1,d5
	moveq	#0,d7
	moveq	#0,d2
	move.w	Nb_Pos,d7		; nb_pos
	subq	#1,d7
.Ploop:
	moveq	#0,d1
	move.b	(a6)+,d1
	cmp.w	d5,d1
	bgt.s	.ok
	addq	#4,a1
	bra.w	.next_patt
.ok:
	addq.l	#1,d5
	moveq	#3,d6			; 4 voies
.Vloop:
	moveq	#0,d1
	move.b	(a1)+,d1
	lsl	#8,d1
	lea	$2fa(a4),a0
	add.l	d1,a0
	moveq	#63,d0			; 64 notes par track
.loop:
	move.l	(a0)+,(a2)
	lea	16(a2),a2
	dbf	d0,.loop
	suba.l	#64*16,a2		; remonte au debut de la voie
	addq	#4,a2			; et voie suivante !
	dbf	d6,.Vloop
	suba.l	#16,a2			; pointe au debut pattern
	adda.l	#$400,a2		; pattern suivant !
.next_patt:
	dbf	d7,.Ploop

PP10_Copy_Samples:			; a2 = fin des patterns_dest
	lea	$2fa(a4),a0
	move.l	Nb_PPTracks(pc),d0
	lsl.l	#2,d0
	add.l	d0,a0
	move.l	a0,a3
	add.l	SampLen,a3		; fin des sons
.copy:					; donc pointe là où mettre les samples
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts



*-----------------------------------------------------------------------*
;
; ProPacker 2.1 (SB) Check Routine

CheckPP21:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	$2fa(a0),a0
	tst.w	(a0)
	bne	.fail

	lea	$f8(a4),a0
	bsr	Crypto_PP_Same
	tst.l	d7
	beq	.fail

; PP21_How_Many_Trax:
	lea	$fa(a4),a0
	moveq	#0,d2			; compteur
	move.l	#$1FF,d0		; $200 bytes a scanner
.loop1:
	moveq	#0,d1
	move.b	(a0)+,d1
	cmp.w	d2,d1
	ble.s	.jp
	move.w	d1,d2			; change'em
.jp:
	dbf	d0,.loop1
	addq	#1,d2
	lsl	#3,d2		; *$08
	lsl.l	#4,d2		; *$10
	move.l	d2,Nb_Trax

	lea	$2fa(a4),a0
	add.l	d2,a0
	move.l	a0,a1
	add.l	(a0),a0
	move.l	a0,d1
	addq.l	#4,d1
	add.l	d5,d1			; fin du pack_mod
	btst	#0,d1			; test si pair !!
	bne	.fail
	sub.l	a4,d1
	sub.l	d1,d4
	cmpi.l	#-256,d4
	blt	.fail


	move.l	(a1)+,d1		; pointe sur les vraies notes !
	beq	.fail
	moveq	#0,d7
	lsr	#2,d1
	subq.l	#1,d1
	cmpi.w	#64*2,d1
	blo.s	.okette
	move.l	#(64*2)-1,d1
.okette:
	move.l	(a1)+,d2		; get real notes
	andi.l	#$FFFF0000,d2
	swap	d2
	tst.w	d2
	beq.s	.nexta
	cmpi.w	#$0071,d2
	blo	.fail		; si une seule note est < $71, fuck!
	cmpi.w	#$1358,d2
	bhi	.fail
	addq	#1,d7
.nexta:
	dbf	d1,.okette
	tst.l	d7
	beq	.fail

.Kel_Type_de_PP21:
	lea	$2fa(a4),a0		; pointe sur les jump_val
	move.l	Nb_Trax,d0
	lsr.l	#4,d0
	lsr	#3,d0			; revient au nb de voies
	lsl.l	#6,d0			; * 64 notes
	cmpi.w	#64*4,d0
	blo.s	.greh
	move.l	#64*4,d0
.greh:
	subq	#1,d0
	moveq	#0,d3			; compteur d'erreurs
.loop2:
	moveq	#0,d1
	move.w	(a0)+,d1		; take value
	cmpi.w	#$1800,d1
	bhs	.fail
	move.l	d1,d2
	lsr	#2,d2			; divise par 4
	lsl	#2,d2			; remultiplie par 4
	cmp.w	d2,d1			; si different, fuck !
	beq.w	.next1
	addq	#1,d3
.next1:
	dbf	d0,.loop2
	tst.w	d3
	beq.s	.PP21_Type4		; si d3 tjrs à 0, c un type 4
	move.w	#1,PP21_Type
	bra.s	.go_on
.PP21_Type4:
	move.w	#4,PP21_Type		; pas de mulu #4 a faire donc...
.go_on:
	lea	PP21_Table,a0		; EFFACE les Save_Tables
	moveq	#127,d0
.efface:
	clr.l	(a0)+
	dbf	d0,.efface

	lea	$fa(a4),a6
	moveq	#0,d0
	move.b	-2(a6),d0
	move.w	d0,Nb_Pos
	lea	PP21_Table,a5
	moveq	#0,d2			; compteur
	subq	#1,d0
.lo:
	move.b	$000(a6),(a5)+
	move.b	$080(a6),(a5)+
	move.b	$100(a6),(a5)+
	move.b	$180(a6),(a5)+
	addq	#1,a6
	dbf	d0,.lo

; PP21_PTK_Table:
	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; skip 1er pattern
	lea	PP21_Table+4,a0
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend longword de chak patt
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	lea	PP21_Table,a1		; table de comparaison
	lea	KR_PosTable,a3		; PTK Table
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos(pc),(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	cmpi.w	#$40,d6
	bhi	.fail

; Calc PTK Module Size

	move.l	d6,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; ProPacker 2.1 (SB) to Protracker Convert routine, from Pro-Wizzy

ConvertPP21:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; Heading
	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop

	lea	$3b6(a5),a1
	move.b	(a0),(a1)

PP21_Cree_PTK_Table:
	lea	KR_PosTable,a0
	lea	$3b8(a5),a1
	moveq	#$7f,d0
.loop:
	move.b	(a0)+,(a1)+
	dbf	d0,.loop

PP21_Convert_Patterns:
	lea	$3b8(a5),a6		; patt_pos
	lea	$43c(a5),a2		; patterns_dest
	lea	PP21_Table,a1		; track_byte_pos
	lea	$2fa(a4),a3
	add.l	Nb_Trax,a3
	addq	#4,a3			; real notes source

	moveq	#-1,d5
	moveq	#0,d7
	move.w	Nb_Pos,d7		; nb_pos
	subq	#1,d7
.Ploop:
	moveq	#0,d1
	move.b	(a6)+,d1
	cmp.w	d5,d1
	bgt.s	.ok
	addq	#4,a1
	bra.s	.next_patt
.ok:
	addq.l	#1,d5
	moveq	#3,d6			; 4 voies
.Vloop:
	moveq	#0,d1
	move.b	(a1)+,d1
	mulu	#$80,d1
	lea	$2fa(a4),a0
	add.l	d1,a0
	moveq	#63,d0			; 64 notes par track
.loop:
	moveq	#0,d1
	move.w	(a0)+,d1		; take jump_address
	cmpi.w	#4,PP21_Type
	beq.s	.jp
	lsl.w	#2,d1			; mulu #4 pour longword
.jp:
	move.l	(a3,d1.w),d2		; take real note at zis address
.wr:
	move.l	d2,(a2)
	lea	16(a2),a2
	dbf	d0,.loop
	suba.l	#64*16,a2		; remonte au debut de la voie
	addq	#4,a2			; et voie suivante !
	dbf	d6,.Vloop
	suba.l	#16,a2			; pointe au debut pattern
	adda.l	#$400,a2		; pattern suivant !
.next_patt:
	dbf	d7,.Ploop

PP21_Copy_Samples:			; a2 = debut des samples_dest
	lea	$2fa(a4),a1
	add.l	Nb_Trax,a1
	move.l	(a1)+,d2
	move.l	a1,a3
	add.l	d2,a3			; debut des samples (source)

	move.l	a3,a0
	add.l	SampLen,a3		; fin des sons

	move.l	a2,a4
.copy:					; donc pointe là où mettre les samples
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts



*-----------------------------------------------------------------------*
;
; Digital Illusions Check Routine

CheckPinb
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	(a0),d2
	move.l	d2,d1
	andi.l	#$00FF0000,d1		; Garde ke le nb de samples
	swap	d1
	cmpi.w	#$001F,d1		; Supérieur a $1F ? fuck
	bhi	.fail
	move.l	d2,d1
	andi.l	#$FF00FFFF,d1		; Sinon reprend le longmot
	tst.l	d1			; sans nb_smpl = doit etre nul !
	bne	.fail
	move.w	6(a0),d1
	add.w	10(a0),d1		; Ajoute 2 mots qui DOIVENT etre nuls
	tst.w	d1
	bne	.fail			; Fuck

	tst.w	4(a0)
	beq	.fail
	tst.w	12(a0)
	beq	.fail
	moveq	#0,d0
	move.w	8(a0),d0		; valeur a comparer (158)
	beq	.fail
	moveq	#0,d1
	move.w	(a0),d1			; nb_smpl
	move.w	d1,Nb_Samp
	lsl	#3,d1			; *8
	lea	14(a0),a0		; +14
	moveq	#0,d2
	move.w	(a0,d1.w),d2		; d2 doit etre idem d0
	cmp.w	d0,d2
	bne	.fail

	moveq	#0,d3
	lea	14(a4),a0		; pointe sur 1er sample_data
	moveq	#0,d0
	move.w	Nb_Samp,d0
	subq	#1,d0
.loop:
	moveq	#0,d2
	move.w	(a0)+,d2		; take length
	cmpi.w	#$8000,d2
	bhi	.fail
	add.l	d2,d3			; cumule lengths
	moveq	#0,d1
	move.w	(a0)+,d1		; take volume
	move.l	d1,d5
	andi.w	#$00FF,d1
	cmpi.w	#$0040,d1
	bhi	.fail			; vol doit etre <= $40
	andi.w	#$FF00,d5
	cmpi.w	#$0F00,d5
	bhi	.fail			; vol doit etre <= $40
	move.w	(a0)+,d5
	add.w	(a0)+,d5		; repeat+replen
	tst.w	d2
	beq.s	.nop
	cmp.w	d2,d5
	bhi	.fail
.nop:
	dbf	d0,.loop

	lsl.l	#1,d3
	beq	.fail
	move.l	d3,SampLen
	
	move.l	a4,a0
	add.w	8(a0),a0
	cmpi.b	#$FF,-1(a0)
	bne	.fail

	move.l	a4,a0
	add.w	4(a0),a0		; pointe sur table_patt
	moveq	#0,d3
	moveq	#0,d2
.loop2:
	moveq	#0,d1
	move.b	(a0)+,d1		; take byte_pos
	cmpi.b	#$FF,d1			; fin de la table ?
	beq.s	.sortdela
	addq	#1,d2
	cmp.w	d3,d1
	ble.s	.ciao
	move.w	d1,d3
.ciao:
	bra.s	.loop2
.sortdela:
	move.w	d2,Nb_Pos
	addq	#1,d3
	move.w	d3,Nb_Patt

	cmpi.w	#2,d3			; Au moins 2 patterns
	blo.s	.tt_pis
	move.l	a4,a0
	add.w	4(a0),a0		; pointe sur table_patt
	subq	#2,a0
	subq	#2,d3
.gry:
	moveq	#0,d0
	moveq	#0,d1
	move.w	(a0),d0			; le + grand
	move.w	-(a0),d1		; le + petit
	cmp.w	d0,d1
	bhs	.fail			; d1 DOIT ETRE PLUS PETIT !!
	dbf	d3,.gry

.tt_pis:
	move.l	a4,a0
	add.w	12(a0),a0
	add.l	SampLen,a0		; end pack_mod
	sub.l	a4,a0			; size pack_mod
	move.l	a0,d1
	sub.l	d1,d4
	cmpi.l	#-256,d4
	blt.s	.fail

	moveq	#0,d0			; Calc PTK Module Size
	move.w	Nb_Patt,d0
	lsl	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; Pinball-2-Protrack routine, from Pro-Wizzy

ConvertPinb
	move.l	a0,a4
	move.l	a1,a5

					; Make header
	lea	42(a1),a1
	moveq	#0,d0
	move.w	(a0),d0			; nb_samples
	move.l	d0,d2
	subq	#1,d0
	lea	14(a0),a0
.loop:
	move.l	(a0)+,(a1)		; move length+vol
	move.l	(a0)+,4(a1)		; move repeat+replen
	lea	30(a1),a1
	dbf	d0,.loop

	cmpi.w	#$1F,d2
	beq.s	.No_Need
	moveq	#30,d0
	sub.w	d2,d0
.loop1:
	move.l	#$0,(a1)
	move.l	#$1,4(a1)		; fill a vide le reste des samples...
	lea	30(a1),a1
	dbf	d0,.loop1

.No_Need:
	move.l	a4,a0
	add.w	4(a0),a0		; pointe sur table_patt
	lea	$3b8(a5),a1
.loop2:
	moveq	#0,d1
	move.b	(a0)+,d1		; take byte_pos
	cmpi.b	#$FF,d1			; fin de la table ?
	beq.s	.sortdela
	move.b	d1,(a1)+		; move patt
	bra.s	.loop2
.sortdela:
	move.l	a5,a1
	moveq	#0,d2
	move.w	Nb_Pos,d2
	move.b	d2,$3b6(a1)		; Nb_Pos

Pinball_DoIt:
	move.l	a4,a0
	add.w	$c(a0),a0		; debut samples (source)
	move.l	a0,a6			; ne pas convertir si on depasse ca

	move.l	a4,a0
	add.w	4(a0),a0
	move.l	a0,a2			; limite de la conversion !
	lea	$43c(a5),a3
	move.l	a5,-(sp)

	move.l	a4,a0
	moveq	#0,d1
	move.w	(a0),d1			; nb_samples
	lsl	#3,d1			; $C * 8
	add.w	d1,a0
	lea	14(a0),a0		; debut de la table des patterns

	move.l	a4,a1
	move.l	a1,Save_Addy		; SAVE_IT
.Loop:
	add.w	(a0)+,a1		; pointe sur le 1er pattern...
	cmp.l	a2,a0
	bhi	Pinball_Copy_Samples
	move.l	#255,d0			; 64*4 notes, -1 pour le DBF
.Nloop:
	cmp.l	a6,a1
	blt.s	.its_ok
.vide:	
	clr.l	(a3)+
	dbf	d0,.vide
	move.l	Save_Addy(pc),a1	; RESTORE IT
	bra.s	.Loop
.its_ok:
	moveq	#0,d1			; note fait 2 ou 3 bytes
	move.b	(a1)+,d1		; take 1er byte de la note
	cmpi.w	#$00FF,d1		; $FF = Blank note.l !
	beq.w	.Blank_Note
	cmpi.w	#$0080,d1
	blo.s	.Note_Seule		; <$80 = Note sans commande, 2 bytes!
.Les_Deux:				; si >$80, les deux!
	lsl	#8,d1
	move.b	(a1)+,d1		; 2eme byte
	lsl.l	#8,d1
	move.b	(a1)+,d1		; 3eme byte, on a $00A4CC00 (ex)
	move.l	d1,d2
	andi.l	#$00000FFF,d2		; garde ke l'effet
	moveq	#0,d7
	move.w	d2,d7			; stock cmd+effect
	move.l	d1,d2
	lsr.l	#8,d2			; 00A4CC00 = 0000A4CC
	lsr	#4,d2			; 0000A4CC = 00000A4C
	subi.w	#$0800,d2		; A4C = 24C
	move.l	d2,d3
	divu	#$40,d3
	move.w	d3,d6			; stock sample_nb
	mulu	#$40,d3
	addq	#1,d3
	sub.w	d3,d2
	lsl	#1,d2
	lea	mt_periodtable,a5
	move.w	(a5,d2.w),d2
	cmpi.w	#$10,d6			; smpl >= $10
	blt.s	.pas_decoupe
.decoupe:
	subi.w	#$10,d6
	addi.w	#$1000,d2
.pas_decoupe:
	move.w	d2,(a3)+		; 1er word
	ror.w	#4,d6
	add.w	d7,d6			; 2eme word
	move.w	d6,(a3)+
	bra.w	.Next
.Note_Seule:
	lsl	#8,d1			; decale pour rajouter le 2eme byte
	move.b	(a1)+,d1		; c'est fait...
	moveq	#0,d5
	move.b	d1,d5
	andi.b	#$0F,d5			; garde ke la cmd
	lsl	#8,d5			; a la bonne place
	lsr	#4,d1			; $1CF0 = $01CF
	move.l	d1,d2
	divu	#$40,d2			; pour trouver le sample de reference
	move.w	d2,d7			; stock sample_nb
	mulu	#$40,d2			; re-multiplie pour trouver difference
	addq	#1,d2			; $1C0+1 = 1C1 = base (C-1 smpl 7)
	sub.w	d2,d1			; d1 = 1CF-1C1 = $E
	lsl	#1,d1			; *2 pour scanner mt_periodtable
	lea	mt_periodtable,a5
	move.w	(a5,d1.w),d1		; take real_note
	cmpi.w	#$10,d7			; smpl >= $10
	blt.s	.pas_decoupe2
.decoupe2:
	subi.w	#$10,d7
	addi.w	#$1000,d1
.pas_decoupe2:
	move.w	d1,(a3)+		; 1er word
	ror.w	#4,d7			; smpl 0007 = 7000
	add.w	d5,d7			; ajoute cmd
	move.w	d7,(a3)+		; 2eme word
	bra.s	.Next
.Blank_Note:
	move.l	#$00000000,(a3)+
.Next:
	dbf	d0,.Nloop
	move.l	Save_Addy(pc),a1		; RESTORE_IT
	bra.w	.Loop

Pinball_Copy_Samples:
	move.l	(sp)+,a5

	lea	$43c(a5),a3
	moveq	#0,d3
	move.w	Nb_Patt,d3
	lsl.l	#8,d3
	lsl.l	#2,d3			; +$400
	add.l	d3,a3
	move.l	a4,a0
	add.w	$c(a0),a0		; debut samples (source)
	move.l	a0,a2
	add.l	SampLen,a2		; fin des sons
.copy:
	cmp.l	a2,a0
	bge.s	.fini
	move.l	(a0)+,(a3)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts

Save_Addy:
	dc.l	0


*------------------------------------------------------------------------*
;
; Get highest pattern number in table

Get_PGP:				; d0 = nb_pos, a0 = patt_table
	moveq	#0,d1			; contiendra le plus grand pattern
	subq	#1,d0
  .check:
	moveq	#0,d2
	move.b	(a0)+,d2
	cmp.w	d1,d2			; si new < old, on saute
	ble.s	.no_need
	move.w	d2,d1			; sinon on stocke le new
  .no_need:
	dbf	d0,.check
	addq	#1,d1			; +1 car ca commence a 00
	move.w	d1,Nb_Patt
	rts


*-----------------------------------------------------------------------*
;
; Laxity Check Routine

CheckLax
	move.l	a0,a4
	move.l	d0,d4

	lea	$3a2(a0),a0
	move.l	(a0),d1
	andi.l	#$FF000000,d1		; garde nb_pos
	cmpi.l	#$7F000000,d1		; si nb_pos > $7F, pas la peine
	bhi	.fail
	tst.l	d1			; ou aucune POS ? fuck
	beq	.fail
	move.l	(a0),d1
	andi.l	#$00FF0000,d1		; garde constante
	cmpi.l	#$007F0000,d1		; si const = $7F, possible
	beq.s	.maybe
	cmpi.l	#$003F0000,d1		; sinon pas > $3F
	bhi	.fail
.maybe:
	move.l	(a0),d1
	andi.l	#$0000FF00,d1		; garde nb_patt 0
	cmpi.w	#$3F00,d1		; si patt > $3F, pas le peine
	bhi	.fail
	move.l	(a0),d1
	andi.l	#$000000FF,d1		; garde nb_patt 1
	cmpi.w	#$003F,d1		; si patt > $3F, pas le peine
	bhi	.fail

	cmpa.l	#'M.K.',$82(a0)		; no M.K. in a LAXITY !
	beq	.fail
	lea	22(a4),a0		; pointe sur length
	moveq	#0,d6
	moveq	#30,d0
.loop:
	moveq	#0,d1
	move.w	2(a0),d1
	move.l	d1,d2
	andi.w	#$00FF,d1		; volume > $40 : fuck
	cmpi.w	#$0040,d1
	bhi	.fail
	andi.w	#$FF00,d2
	tst.w	d2			; no finetune !! sinon : fuck
	bne	.fail
	moveq	#0,d1
	move.w	(a0),d1
	bmi	.fail
	bne.s	.next
	move.l	4(a0),d1
	andi.l	#$0000FFFF,d1		; si oui,
	cmp.w	#$0001,d1		; replen DOIT etre a 0001
	bne	.fail
	bra.s	.next2
.next:
	add.l	d1,d6			; cumul lengths
	moveq	#0,d2
	move.w	4(a0),d2
	add.w	6(a0),d2
	cmp.w	d1,d2
	bhi	.fail
.next2:
	lea	30(a0),a0
	dbf	d0,.loop

	lsl.l	#1,d6
	beq	.fail
	move.l	d6,SampLen

	move.l	a4,a0
	moveq	#0,d0
	move.b	$3a2(a0),d0
	lea	$3a4(a0),a0
	bsr	Get_PGP			; nb_patt dans d1
	cmpi.w	#$40,d1
	bhi	.fail
	lea	$424(a4),a0		; pointe sur les patterns
	lsl.l	#8,d1
	mulu	#3,d1
	cmpi.w	#$c000,d1		; $40 patt * $300 = $C000
	bhi	.fail
	adda.l	d1,a0			; debut des sons
	adda.l	d6,a0			; fin des sons
	suba.l	a4,a0
	sub.l	a0,d4
	cmpi.l	#-256,d4
	blt	.fail

	moveq	#0,d7
	lea	$424(a4),a0
	bsr	Unic_Test_PTK
	cmpi.l	#'OKAY',d7
	bne	.fail

.test_notes:
	lea	$424(a4),a0		; scan notes !! 3 octets (0D1F05)
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#8,d0
	subq	#1,d0
.scan:
	moveq	#0,d1
	move.b	(a0)+,d1		; prend le byte1 (note)
	cmpi.w	#$66,d1			; une note LAXITY ne depasse pas $66 !
	bhi	.fail
	addq	#2,a0
	dbf	d0,.scan

	move.w	#$424,Lax_begin		; convertir a partir de $424+

	moveq	#0,d0			; Calc PTK Module Size
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


Lax_begin:	dc.w	0
Replay_Pos:	dc.w	0
Note_Number:	dc.b	0
WN_Sample_number:dc.b	0
		even
Deci:
 dc.b	$00,$01,$02,$03,$04,$05,$06,$07,$08,$09,$10,$11,$12,$13,$14,$15
 dc.b	$16,$17,$18,$19,$20,$21,$22,$23,$24,$25,$26,$27,$28,$29,$30,$31
 dc.b	$32,$33,$34,$35,$36,$37,$38,$39,$40,$41,$42,$43,$44,$45,$46,$47
 dc.b	$48,$49,$50,$51,$52,$53,$54,$55,$56,$57,$58,$59,$60,$61,$62,$63
 dc.b	$64,$65,$66,$67,$68,$69,$70,$71,$72,$73,$74,$75,$76,$77,$78,$79
 dc.b	$80,$81,$82,$83,$84,$85,$86,$87,$88,$89,$90,$91,$92,$93,$94,$95
 dc.b	$96,$97,$98,$99
	even

Unic_Test_PTK:
	moveq	#0,d5
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#7,d0
	subq	#1,d0
.loop:	move.l	(a0)+,d1
	beq.s	.next
	addq	#1,d5
	swap	d1
	andi.w	#$0FFF,d1
	beq.s	.next
	cmpi.w	#$0071,d1
	blo	.okay
	cmpi.w	#$0358,d1
	bhi	.okay
.next:	dbf	d0,.loop
	tst.w	d5
	bne.s	.fuck
.okay:	move.l	#'OKAY',d7
	rts
.fuck:	move.l	#'FUCK',d7
	rts

*-----------------------------------------------------------------------*
;
; Unic Check Routine

CheckUnic
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	$3b6(a0),a0
	move.l	(a0),d1
	andi.l	#$FF000000,d1		; garde nb_pos
	cmp.l	#$7F000000,d1		; si nb_pos > $7F, pas la peine
	bhi	.fail
	tst.l	d1			; ou aucune POS ? fuck
	beq	.fail
	move.l	(a0),d1
	andi.l	#$00FF0000,d1		; garde constante
	cmp.l	#$007F0000,d1		; si const = $7F, possible
	beq.s	.maybe
	cmpi.l	#$003F0000,d1		; sinon pas > $3F
	bhi	.fail
.maybe:
	move.l	(a0),d1
	andi.l	#$0000FF00,d1		; garde nb_patt 0
	cmpi.w	#$3F00,d1		; si patt > $3F, pas le peine
	bhi	.fail
	move.l	(a0),d1
	andi.l	#$000000FF,d1		; garde nb_patt 1
	cmpi.w	#$003F,d1		; si patt > $3F, pas le peine
	bhi	.fail

	cmpa.l	#'SNT.',$82(a0)		; Hey Hey !!!
	beq	.fail

	lea	42(a4),a0		; pointe sur length
	moveq	#0,d6
	moveq	#30,d0
.loop:
	moveq	#0,d1
	move.w	2(a0),d1
	move.l	d1,d2
	andi.w	#$00FF,d1		; volume > $40 : fuck
	cmpi.w	#$0040,d1
	bhi	.fail
	andi.w	#$FF00,d2
	tst.w	d2			; no finetune !! sinon : fuck
	bne	.fail
	moveq	#0,d1
	move.w	(a0),d1
	bmi	.fail
	bne.s	.next
	move.l	4(a0),d1
	andi.l	#$0000FFFF,d1		; si oui,
	cmp.w	#$0001,d1		; replen DOIT etre a 0001
	bne	.fail
	bra.s	.next2
.next:
	add.l	d1,d6			; cumul lengths
	moveq	#0,d2
	move.w	4(a0),d2
	add.w	6(a0),d2
	cmp.w	d1,d2
	bhi	.fail
.next2:
	lea	30(a0),a0
	dbf	d0,.loop

	lsl.l	#1,d6
	beq	.fail
	move.l	d6,SampLen

	move.l	a4,a0
;	moveq	#0,d0
;	move.b	$3b6(a0),d0
	moveq	#$7e,d0
	lea	$3b8(a0),a0
	bsr	Get_PGP			; nb_patt dans d1
	cmpi.w	#$40,d1
	bhi	.fail
	lsl.l	#8,d1
	mulu	#3,d1
	cmpi.w	#$c000,d1		; $40 patt * $300 = $C000
	bhi	.fail
	lea	$43c(a4),a0		; pointe sur les patterns
	adda.l	d1,a0			; debut des sons
	adda.l	d6,a0			; fin des sons
	suba.l	a4,a0
	sub.l	a0,d4
	cmpi.l	#-256,d4
	blt	.fail

	moveq	#0,d7
	lea	$43c(a4),a0
	bsr	Unic_Test_PTK
	cmpi.l	#'OKAY',d7
	bne	.fail

.test_notes:
	lea	$43c(a4),a0		; scan notes !! 3 octets (0D1F05)
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#8,d0
	subq	#1,d0
.scan:
	moveq	#0,d1
	move.b	(a0)+,d1		; prend le byte1 (note)
	cmpi.w	#$66,d1			; une note UNIC ne depasse pas $66 !
	bhi	.fail
	addq	#2,a0
	dbf	d0,.scan

	move.w	#$43c,Lax_begin		; convertir a partir de $43c+

	moveq	#0,d0			; Calc PTK Module Size
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


*--------------------------------------------------------------------------*
;
; Laxity/Unic to Protracker converter, Pro-Wizard routine

ConvertLax:
	move.l	a0,a4
	move.l	a1,a5

	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl.l	#8,d1
	subq.l	#1,d1
	move.l	d1,Bigest_Patt

Lax_Copy_Header:
	lea	20(a1),a1
	move.l	#$423,d0
.loop:
	move.b	(a0)+,(a1)+			; copie les $424 1ers bytes
	dbf	d0,.loop

	bra.w	Branch_LaxUnic			; goto similar routine

; -------------------------------- U N I C ------------------------------

ConvertUnic:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl.l	#8,d1
	subq.l	#1,d1
	move.l	d1,Bigest_Patt

Unic_Copy_Header:
	move.l	#$437,d0
.loop:
	move.b	(a0)+,(a1)+			; copie les $438 1ers bytes
	dbf	d0,.loop

; ---------------------- conversion commune --------------------

Branch_LaxUnic:
	lea	-$82(a0),a0
	moveq	#0,d1
	move.b	(a0)+,d1
	move.w	d1,Nb_Pos
	move.b	(a0),d1
	move.w	d1,Replay_Pos

	lea	42(a5),a3
	moveq	#30,d0
.copy:
	moveq	#0,d3
	move.w	(a3),d3
	moveq	#0,d1
	move.w	4(a3),d1
	moveq	#0,d2
	move.w	6(a3),d2
	lsl	#1,d1				; double REPEAT, si necessaire
	add.w	d1,d2
	cmp.w	d3,d2
	bhi.s	.no
	move.w	d1,4(a3)
.no:	lea	30(a3),a3
	dbf	d0,.copy

Lax_Adjust_Finetune:				; le finetune est a la fin
	lea	40(a5),a1			; du nom de chak sample...
	moveq	#30,d0				; juste avant length.
.loop:
	moveq	#0,d1
	move.w	(a1),d1
	neg.b	d1
	andi.w	#$000F,d1
	move.b	d1,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop

Lax_Reconstitution:
	movea.l	a4,a0
	adda.w	Lax_begin(pc),a0	; soit $424, soit $43c
	lea	$43c(a5),a1
	move.l	Bigest_Patt,d0		;	     note  sample
.loop:					;		|  |
	moveq	#0,d1
	move.b	(a0)+,d1		; take longwd ( 0D 1F 05 XX par ex)
	rol.l	#8,d1
	move.b	(a0)+,d1
	rol.l	#8,d1
	move.b	(a0)+,d1
	move.l	d1,d2
	swap	d2
	move.b	d2,Note_Number

	cmp.b	#$40,Note_Number
	bge.s	.decoupe
	moveq	#0,d4
	bra.s	.branch
.decoupe:
	subi.b	#$40,Note_Number	; $40 = $01
	move.l	#$1000,d4
.branch:
	lea	mt_periodtable,a2	; trouve real_note
	moveq	#0,d3
	move.b	Note_Number(pc),d3
	tst.w	d3			; pas de note ??
	bne.s	.suite
	move.w	#$0000,Real_Note
	bra.s	.suite1
.suite:
	subq	#1,d3
	lsl.w	#1,d3
	move.w	(a2,d3.w),Real_Note
.suite1:
	add.w	d4,Real_Note

	move.l	d1,d2
	swap	d2
	move.w	Real_Note(pc),d2	; write real_note
	swap	d2

	move.w	d2,d1
	andi.w	#$0F00,d1
	cmpi.w	#$0D00,d1
	bne.s	.next
	lea	Deci(pc),a6
	moveq	#0,d5
	move.b	d2,d5
	move.b	(a6,d5),d5
	move.b	d5,d2			; remplace D20 par D32
.next:
	move.l	d2,(a1)+		; re-write real longwd
	dbf	d0,.loop

Lax_Copy_Samples:
	cmpi.w	#$424,Lax_begin
	beq.s	.non
	bsr	Insert_Bxx
.non:
	move.l	a0,a3			; debut des samples_source
	move.l	a1,a2			; debut des samples_dest
	add.l	SampLen,a3		; fin des sons
.copy:
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts

Insert_Bxx:
	movea.l	a4,a3
;	adda.w	Lax_begin,a3	; soit $3a3, soit $3b7
;	suba.w	#$85,a3
	moveq	#0,d3
	move.b	$3b7(a3),d3		; repeat_pos
	beq.s	.no_need
	cmpi.b	#$7F,d3
	beq.s	.no_need
	addi.w	#$0B00,d3		; forme la commande Bxx
	moveq	#3,d2			; test les 4 emplacements
	lea	$3b8(a5),a3
	movea.l	a3,a4
	moveq	#0,d4
	move.w	Nb_Pos,d4
	subq	#1,d4
	add.l	d4,a3			; pointe sur la derniere pos
	moveq	#0,d4
	move.b	(a3),d4
	addq	#1,d4
	lsl	#8,d4
	lsl.l	#2,d4
	lea	$84(a4),a4
	add.l	d4,a4			; last line of last played pattern
.loop:
	subq	#4,a4
	move.l	(a4),d4
	move.l	d4,d5
	andi.l	#$00000FFF,d5		; espace libre ??
	beq.s	.yeahman
	dbf	d2,.loop		; aucune des 4 ?? on ecrase la 1ere !!
	andi.l	#$FFFFF000,d4
.yeahman:
	add.w	d3,d4
	move.l	d4,(a4)
.no_need:
	rts



*-----------------------------------------------------------------------*
;
; WantonPacker Check Routine

CheckWanton
	move.l	a0,a4
	move.l	d0,d4

	move.l	$438(a4),d2
	move.l	d2,d1
	andi.l	#$FFFFFF00,d1		; Garde ce qui doit etre WN et 0
	cmp.l	#$574E0000,d1		; Un Wanton ??
	bne	.fail
	cmpi.b	#$40,d2			; pas + de $40 patterns
	bhi	.fail
	tst.b	d2
	beq	.fail
	move.w	d2,Nb_Patt

	lea	$43c(a4),a0
	moveq	#64,d0
	lsl	#2,d0
	subq	#1,d0
.loop:
	move.l	(a0)+,d1
	andi.l	#$00FF0000,d1		; garde le num_sample
	swap	d1
	cmpi.w	#$1F,d1
	bhi	.fail			; ne doit pas depasser $1F !
	dbf	d0,.loop
.Check2:
	lea	42(a4),a0
	moveq	#0,d0
	moveq	#30,d1
.Rloop:
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d5
	move.w	(a0),d2
	cmpi.w	#$8000,d2
	bhi	.fail
	add.l	d2,d0			; additionne les longueurs
	move.w	2(a0),d3
	andi.w	#$FF00,d3
	cmpi.w	#$0F00,d3
	bhi	.fail
	move.w	2(a0),d3
	andi.w	#$00FF,d3
	cmpi.w	#$0040,d3
	bhi	.fail
	tst.w	d2
	beq.s	.next
	move.w	4(a0),d5
	add.w	6(a0),d5		; add repeat+replen
	cmp.l	d2,d5
	bhi	.fail
.next:
	lea	30(a0),a0
	dbf	d1,.Rloop
	lsl.l	#1,d0			; mulu #2
	beq	.fail
	move.l	d0,SampLen

	lea	$43c(a4),a0
	moveq	#0,d0
	move.b	-1(a0),d0		; nb_patt
	lsl.l	#8,d0
	lsl.l	#2,d0
	add.l	d0,a0
	move.l	a0,d1
	add.l	SampLen,d1		; fin du pack_mod
	sub.l	a4,d1
	sub.l	d1,d4
	cmpi.l	#-256,d4
	blt.s	.fail

	moveq	#0,d0			; Calc PTK Module Size
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail	moveq	#0,d0				; check failed
.okay	rts



*-----------------------------------------------------------------------*
;
; WantonPacker (and Polka Packer) Converter, Pro-Wizzzzzzz

ConvertWanton:
	move.l	a0,a4				; copy Ptr's
	move.l	a1,a5

	move.l	#$437,d0
.copy:
	move.b	(a0)+,(a1)+
	dbf	d0,.copy

	moveq	#0,d0
	move.l	(a0)+,d0
	andi.l	#$000000FF,d0		; keep nb_patt
	lsl.l	#8,d0
	move.l	d0,Bigest_Patt
	bra.w	Convert_WN_PWR

ConvertPolka:
	move.l	a0,a4				; copy Ptr's
	move.l	a1,a5

	move.l	#$437,d0
.copy:
	move.b	(a0)+,(a1)+
	dbf	d0,.copy


	lea	46(a5),a0
	moveq	#0,d2
	moveq	#30,d0
.spec:
	move.w	(a0),d2
	beq.s	.next
	lsr	#1,d2			; divise par 2 les REPEAT
	move.w	d2,(a0)
.next:
	lea	30(a0),a0
	dbf	d0,.spec


	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#8,d0
	move.l	d0,Bigest_Patt
	lea	$43c(a4),a0

Convert_WN_PWR:
	addq	#4,a1
	move.l	Bigest_Patt,d0		;	      note  sample
	subq.l	#1,d0
.loop:					;	         |  |
	move.l	(a0)+,d1		; take longwd ( 14 08 0C 00  par ex)
	move.l	d1,d2
	swap	d2
	move.b	d2,WN_Sample_number
	andi.w	#$FF00,d2
	ror.w	#8,d2
	move.b	d2,Note_Number

	cmp.b	#$10,WN_Sample_number	; sample > $10 ??
	bge.s	.decoupe
	moveq	#0,d4
	bra.s	.branch
.decoupe:
	subi.b	#$10,WN_Sample_number	; = $03 si yavait $13
	move.l	#$1000,d4
.branch:
	lea	mt_periodtable,a2	; trouve real_note
	moveq	#0,d3
	move.b	Note_Number,d3
	tst.w	d3			; pas de note ??
	bne.s	.suite
	move.w	#$0000,Real_Note
	bra.s	.suite1
.suite:
	subq	#2,d3
	move.w	(a2,d3.w),Real_Note
.suite1:
	add.w	d4,Real_Note
	moveq	#0,d3			; prepare real_sample_nb
	move.b	WN_Sample_number,d3
	ror.w	#4,d3			; $0001 devient $1000

	move.l	d1,d2
	swap	d1
	move.w	Real_Note(pc),d1	; write real_note
	swap	d1
	add.w	d3,d1			; mix sample_nb avec cmd

	move.l	d1,(a1)+		; re-write real longwd
	dbf	d0,.loop

WN_Calc_SampLen:
	move.l	a0,a3			; debut des samples_source
	move.l	a1,a2			; debut des samples_dest
	add.l	SampLen,a3		; fin des sons
.copy:
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts


*-----------------------------------------------------------------------*
;
; Pha-Packer Check Routine

CheckPha:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	8(a0),d0
	cmpi.l	#$3C0,d0
	bne	.fail

	moveq	#0,d0
	move.w	$1b4(a0),d0
	beq	.fail
	cmpi.w	#$200,d0		; pas + de $80*4 = $200 pos
	bhi	.fail
	btst	#0,d0
	bne	.fail
	btst	#1,d0
	bne	.fail			; test si divisible par 4
	lsr.w	#2,d0
	move.w	d0,Nb_Pos

	moveq	#0,d2
	moveq	#30,d0
.loop:
	move.l	(a0),d1
	swap	d1
	moveq	#0,d3
	move.w	d1,d3
	add.l	d3,d2			; cumul longueurs
	tst.w	d3
	beq.s	.jump
	moveq	#0,d1
	move.w	4(a0),d1
	add.w	6(a0),d1
	beq	.fail			; jamais 0000
	cmp.l	d3,d1
	bhi	.fail
	bra.s	.jump2
.jump:
	moveq	#0,d1
	move.w	4(a0),d1
	add.w	6(a0),d1
	subq	#1,d1
	bne	.fail			; le 0001 de replen -1 = 0 ?
.jump2:
	moveq	#0,d1
	move.w	2(a0),d1
	andi.w	#$00FF,d1
	cmpi.w	#$0040,d1
	bhi	.fail
	moveq	#0,d1
	move.w	12(a0),d1
	divu	#$48,d1
	mulu	#$48,d1
	cmp.w	12(a0),d1
	bne	.fail
	lea	14(a0),a0
	dbf	d0,.loop

	lsl.l	#1,d2
	beq	.fail
	move.l	d2,SampLen

.Pha_Get_PGP:
	lea	$1c0(a4),a0
	move.l	a0,Table_Originale
	moveq	#$7F,d0
	moveq	#0,d1
.PGPloop:
	move.l	(a0)+,d2
	cmp.l	d1,d2
	ble.s	.PGPnext
	move.l	d2,d1
.PGPnext:
	dbf	d0,.PGPloop

	move.l	a4,a2
	add.l	d1,a2			; pointe sur debut du last pattern!
	move.l	#$FF,d0
	moveq	#$0,d5			; compte les notes !
.notes:
	addq.w	#$1,d5
	blt.s	.next_note
	addq	#$4,a2
	tst.w	(a2)
	bge.s	.next_note
	move.w	(a2)+,d5
	move.l	d5,d1
	andi.w	#$FF00,d1
	cmpi.w	#$FF00,d1
	beq.s	.next_note
	tst.w	d0
	bne	.fail
	subq	#2,a2
.next_note:
	dbf d0,.notes			; fin, a2 = fin du pack_mod !
	move.l	a2,Pha_End_Mod

	sub.l	a4,a2			; a2 = mod_size
	cmp.l	d4,a2			; d4 = mod_size (loaded)
	bhi	.fail			; PAS PLUS PETIT ! Sinon : byebye

; Pha_Make_PTK_Table:
	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; 1er patt = 0 d'office !
	move.l	Table_Originale,a0
	addq	#4,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Table_Originale,a1	; table de comparaison
	lea	KR_PosTable,a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,Pha_BytePos
	move.l	(a1)+,d3
	sub.w	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	Pha_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	cmpi.w	#$40,d6
	bhi	.fail

; Calc PTK Module Size

	move.l	d6,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts



Pha_a1:	dc.l	0

*-----------------------------------------------------------------------*
;
; Pha-Packer (Azatoth) to Protracker Convert routine, from Pro-Wizzy

ConvertPha:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; Heading
	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.l	(a0),d1
	andi.l	#$FFFF00FF,d1		; vire finetune pourrave
	moveq	#0,d2
	move.w	12(a0),d2		; take word_finetune
	divu	#$48,d2			; a diviser par $48
	lsl	#8,d2			; 000E = 0E00
	add.w	d2,d1
	move.l	d1,(a1)
	move.l	4(a0),4(a1)
	lea	14(a0),a0
	lea	30(a1),a1
	dbf	d0,.loop

	moveq	#0,d0
	move.w	2(a0),d0
	lsr.w	#2,d0
	lea	$3b6(a5),a1
	move.b	d0,(a1)+
	move.b	#$7F,(a1)+

Pha_Cree_PTK_Table:
	lea	KR_PosTable,a0
	moveq	#$7f,d0
.loop:
	move.b	(a0)+,(a1)+
	dbf	d0,.loop

PhaPacker_Morph:
	lea	$3b8(a5),a2		; Byte_tab
	lea	$43c(a5),a1		; Buffer-Destination
	move.l	Pha_End_Mod,a3		; fin des patterns, do not trepass !

	moveq	#-1,d5
	moveq	#0,d6
	move.w	Nb_Pos,d6		; nombre de patterns
	subq	#1,d6
	move.l	Table_Originale,a6
.boucle:
	clr.l	Pha_a1

	moveq	#0,d1
	move.b	(a2)+,d1
	cmp.w	d5,d1
	bgt.s	.okette
	addq	#4,a6
	bra.w	.next_patt
.okette:
	addq.l	#1,d5
	move.l	(a6)+,d3		; take pattern addy
	beq.w	.fini
	move.l	a4,a0
	add.l	d3,a0
	moveq	#0,d0			; compteur de positions (0 a 255)
.loop:
	cmp.l	a3,a0
	bhs	.next_patt		; fin des notes ? try next pattern

	moveq	#0,d1
	move.w	(a0),d1
	move.w	d1,d2
	andi.w	#$FF00,d2
	cmpi.w	#$FF00,d2		; test si Jump_Addr ou note normale
	bne.s	.normal
	move.w	d1,d2
	andi.w	#$00FF,d2		; garde le nombre de lignes a sauter
	neg.b	d2			; morph un $C0 en $40 (64 lignes vides)
	subq	#1,d2			; pour le DBF
	lea	-4(a1),a1		; refere a la voie precedente !!!!
	sub.l	#4,Pha_a1
	subq	#1,d0
	move.l	(a1),d7			; longmot a copier d2-fois !
	tst.l	d7			; si d7 = 0 alors d7 = $EEEEEEEE
	bne.s	.ouf
	move.l	#$EEEEEEEE,d7
.ouf:
	move.l	a1,-(sp)		; stocke a1
.fill_it:
	move.l	d7,(a1)
	lea	16(a1),a1
	dbf	d2,.fill_it
	move.l	(sp)+,a1		; remet a1 initial
	addq	#2,a0
	bra.s	.next_one
.normal:
	move.l	(a0),d1			; take longword (note+cmd)
	bsr	Pha_Morph_Note
.gosh:
	cmpi.l	#$EEEEEEEE,(a1)
	beq.s	.test2
.pas_ee:
	tst.l	(a1)
	beq.s	.ok
.test2:
	addq	#1,d0
	addq	#4,a1
	addi.l	#4,Pha_a1
	cmpi.l	#256,d0
	beq.s	.next_patt
	bra.s	.gosh
.ok:
	move.l	d1,(a1)
	addq	#4,a0
.next_one:
	addq	#1,d0
	addq	#4,a1
	addi.l	#4,Pha_a1
	cmpi.l	#256,d0
	bne.w	.loop
.next_patt:
	tst.l	Pha_a1
	beq.s	.zlot
	move.l	#1024,d1
	cmp.l	Pha_a1,d1
	beq.s	.zlot
	sub.l	Pha_a1,d1
	add.l	d1,a1			; rajoute l'oubli !! (! debug v2.20 !)
.zlot:
	dbf	d6,.boucle

.fini:
	lea	$43c(a5),a2
	moveq	#0,d3
	move.w	Nb_Patt,d3
	lsl	#8,d3
	subq	#1,d3
  .vire:
	cmp.l	#$EEEEEEEE,(a2)
	bne.s	.suiv
	move.l	#$00000000,(a2)
  .suiv:
	addq	#4,a2
	dbf	d3,.vire

Pha_Copy_Samples:
	lea	$3C0(a4),a0		; a0 = debut des samples
	move.l	a0,a1
	add.l	SampLen,a1		; a1 = fin des samples

	moveq	#0,d0
	move.w	Nb_Patt,d0		; $18
	lsl.l	#8,d0
	lsl.l	#2,d0
	lea	$43c(a5),a2		; jump header
	add.l	d0,a2			; jump patterns
.loop:					; donc pointe là où mettre les samples
	cmp.l	a1,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.loop
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts


Pha_Morph_Note:
	move.l	d1,d2
	moveq	#0,d3
	move.w	d2,d3			; prend le mot-cmd
	andi.l	#$FF000000,d2		; garde que le son
	rol.l	#8,d2			; 05000000 = 00000005
	moveq	#0,d7
	cmpi.w	#$10,d2			; + grand ke $10 ?
	blt.s	.note
	subi.w	#$0010,d2		; vire la dizaine (d2 = $00000005)
	move.l	#$00001000,d7		; prepare pour sample > $10
.note:
	ror.w	#4,d2			; devient $00005000
	add.w	d2,d3			; mot-cmd READY !

	move.l	d1,d2
	andi.l	#$00FF0000,d2		; garde que la note
	swap	d2
	tst.w	d2
	bne.s	.rhah
	moveq	#0,d4
	bra.s	.moby
.rhah:
	subq	#2,d2
	move.l	a2,-(sp)
	lea	mt_periodtable,a2
	moveq	#0,d4
	move.w	(a2,d2.w),d4		; real note dans d4
	move.l	(sp)+,a2
.moby:
	add.w	d4,d7			; mixée avec d7

	swap	d7
	add.w	d3,d7			; concatene les 2 mots
	move.l	d7,d1
Pha_Test_B00:
	andi.w	#$0F00,d7		; garde ke la commande
	cmpi.w	#$0B00,d7
	bne.s	.babye
	addq	#1,d1
.babye:
	rts

Table_Originale:dc.l	0
Pha_End_Mod:	dc.l	0
Pha_BytePos:	dc.b	0
	even


*-----------------------------------------------------------------------*
;
; Promizer 1.x Check Routine

CheckPM10:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	14(a0),d0
	cmpi.l	#$48e780c0,d0		; = PM10c ET PM18a
	bne	.fail
	move.l	$1164(a0),d0
	cmpi.l	#$000003DE,d0
	bne.s	.un_PM10c		; si ya ca c'est un 1.8a
.un_PM18a:
	adda.l	#$1170,a0
	move.l	#'PM18',PM1_Type
	bra.s	.all
.un_PM10c:
	adda.l	#$116c,a0
	move.l	#'PM10',PM1_Type
.all:
	moveq	#0,d6
	moveq	#30,d0
.loop:
	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d7
	move.w	(a0)+,d1		; take length
	move.w	(a0)+,d2		; take volume
	move.w	(a0)+,d3		; take repeat
	move.w	(a0)+,d7		; take replen
	move.l	d2,d5
	andi.w	#$FF00,d2		; garde que le finetune
	andi.w	#$00FF,d5		; garde que le volume
	cmpi.w	#$0F00,d2		; finetune ne depasse pas $0F
	bhi	.fail
	cmpi.w	#$0040,d5		; volume ne depasse pas $40 !
	bhi	.fail
	tst.w	d1			; length nulle ???
	beq.s	.nexty
	add.l	d1,d6			; cumul lengths
	add.w	d7,d3			; ajoute repeat+replen
	cmp.w	d1,d3			; compare avec length!
	bhi	.fail			; si plus grand, ca fuck !!
.nexty:
	dbf	d0,.loop

	lsl.l	#1,d6
	beq	.fail
	move.l	d6,SampLen
	moveq	#0,d0
	move.w	(a0)+,d0		; nb_positions_max
	lsr.l	#2,d0			; remet au format PTK (divise par 4)
	move.w	d0,Nb_Pos
	move.l	a0,Base

	lea	$1164(a4),a0
	cmp.l	#'PM18',PM1_Type
	bne.s	.nonon
	addq	#4,a0
.nonon:
	add.l	(a0),a0
	addq	#4,a0			; debut samples source
	add.l	d6,a0			; fin du pack_mod
	suba.l	a4,a0
	sub.l	a0,d4
	cmp.l	#-256,d4
	blt	.fail


; PM10_Make_PTK_Table:
	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; 1er patt = 0 d'office !
	move.l	Base(pc),a0
	addq	#4,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Base(pc),a1		; table de comparaison
	lea	KR_PosTable,a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,PM40_BytePos
	move.l	(a1)+,d3
	sub.w	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	PM40_BytePos,(a2)+
	bra.w	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare	;!!!
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	cmpi.w	#$40,d6
	bhi	.fail

; Calc PTK Module Size

	move.l	d6,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


PM_Offset1:	dc.l	0
PM_Offset2:	dc.l	0
PM_Offset3:	dc.l	0
PM1_Type:	dc.l	0

*-----------------------------------------------------------------------*
;
; Promizer 1.x to Protracker Convert routine, from Pro-Wizzy

ConvertPM10:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	cmp.l	#'PM18',PM1_Type
	beq.s	PM18a_Converter

; -------------------------- Promizer_v1.0c ----------------------------

PM10c_Converter:
	move.l	#$1168,PM_Offset1
	move.l	#$116c,PM_Offset2
	move.l	#$1164,PM_Offset3

	bra.s	PM_I_Converter		; routines communes aux deux PM

; -------------------------- Promizer_v1.8a ----------------------------

PM18a_Converter:
	move.l	#$116c,PM_Offset1
	move.l	#$1170,PM_Offset2
	move.l	#$1168,PM_Offset3

; ---------------------------- Promizer I ------------------------------

PM_I_Converter:
	move.l	a4,a0			; nb de notes total (4*64*3 patt * 2o)
	add.l	PM_Offset1(pc),a0	; add.l	#$116c,a0 (1.8a)
	move.l	(a0),Nb_Notes

	move.l	a4,a0			; pointe sur sample data
	add.l	PM_Offset2(pc),a0	; add.l	#$1170,a0
	lea	42(a1),a1		; pointe sur la 1ere sample_data
	moveq	#30,d0
.loop:
	move.l	(a0)+,(a1)+		; swap length (2 octets)
	move.l	(a0)+,(a1)+		; swap volume (2 octets)
	lea	22(a1),a1
	dbf	d0,.loop		; a la fin, a1 = $203b6

	lea	$3B6(a5),a1		; pointe sur la 1ere sample_data
	moveq	#0,d0
	move.w	(a0)+,d0		; nb_positions_max
	lsr.l	#2,d0			; remet au format PTK (divise par 4)
	move.b	d0,(a1)+		; write it
	move.b	#$7f,(a1)+		; constante

	subq	#1,d0			; nb_pos max-1
	move.l	d0,Nb_Loop		; nombre de patterns joués

PM10_Copy_Table:
	lea	KR_PosTable,a0
	moveq	#$7f,d0
.loop:
	move.b	(a0)+,(a1)+
	dbf	d0,.loop

PM_Combien_de_Notes_Differentes:
	move.l	Base(pc),a0
	adda.l	#512,a0			; note_numbers
	move.l	a0,PM1_Notes_Table
	move.l	a0,a2			; store it
	move.l	Nb_Notes(pc),d0
	lsr.l	#1,d0			; divise par 2
	moveq	#0,d2			; d2 contiendra la plus grande adresse
	subq	#1,d0
.loop:
	move.w	(a2)+,d1
	cmp.w	d2,d1			; si nouveau < d2, au suivant
	ble.s	.suivant
	move.w	d1,d2			; sinon on met le nouveau dans d2
.suivant:
	dbf	d0,.loop
	addq	#4,a2			; pour pointer sur les notes & cmds

PM_Pattern_Generator:
	move.l	a4,PM1_Save_a4
	move.l	a5,PM1_Save_a5

	move.l	Base(pc),a3
	moveq	#-1,d5
	lea	$43c(a5),a1
	lea	$3b8(a5),a4

	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
.loop:
	moveq	#0,d6
	move.b	(a4)+,d6
	cmp.w	d5,d6
	bgt.s	.okette
	addq	#4,a3
	bra.w	.non
.okette:
	addq.l	#1,d5
	move.l	PM1_Notes_Table(pc),a0
	move.l	(a3)+,d6
	add.l	d6,a0
	move.l	#(64*4)-1,d6
.loop1:
	cmp.l	a2,a0
	blt.s	.okay
	addq	#2,a0
	moveq	#0,d7
	bra.s	.ici
.okay:
	moveq	#0,d1
	move.w	(a0)+,d1		; read note_number
	subq	#1,d1
	lsl.l	#2,d1			; mulu #4  (4 octets par notes)
	move.l	(a2,d1),d7		; write notes & cmds
.ici:
	move.l	d7,(a1)+
	add.l	#4,Pos_Ptr
	andi.w	#$0F00,d7		; garde que cmd
	cmpi.w	#$0D00,d7		; COMMANDE Pattern_Break ????
	beq.s	.stop_it
	cmpi.w	#$0B00,d7		; COMMANDE Position_Jump ????
	bne.s	.normal
.stop_it:
	move.w	#1,Patt_Break
.normal:
	dbf	d6,.loop1
	cmpi.w	#1,Patt_Break
	bne.s	.continue
	clr.w	Patt_Break		; remet a 0 pour passage suivant
	cmpi.l	#1024,Pos_Ptr
	ble.s	.ok
	move.l	#1024,Pos_Ptr
.ok:	move.l	#1024,d7
	sub.l	Pos_Ptr,d7
	add.l	d7,a1
	clr.l	Pos_Ptr
	bra.s	.non
.continue:
	cmpi.l	#1024,Pos_Ptr		; un pattern refait ?
	blt.s	.non
	clr.l	Pos_Ptr
.non:	dbf	d0,.loop


PM_Copy_Samples:
	moveq	#0,d2
	move.w	Nb_Patt,d2

	move.l	PM1_Save_a5(pc),a1
	move.l	a1,a5
	adda.l	#$43c,a1		; pointe sur debut patterns
	lsl.l	#8,d2			; * $100
	lsl.l	#2,d2			; * 4
	add.l	d2,a1
	move.l	a1,a2			; addr dest_samples

	move.l	PM1_Save_a4(pc),a0
	add.l	PM_Offset3(pc),a0	; add.l	#$1164,a0
	add.l	(a0),a0			; Addr source_samples !
	addq	#4,a0

	move.l	a0,a3
	add.l	SampLen,a3		; adresse de fin des samples
.copy:
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:

	bsr	PM1_Adjust_Finetune
	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts				; FINISHED !!!!! OOOOOUUUUUUFFFFF !!!


; ------------------------   ADJUST FINETUNE   -------------------------

PM1_Adjust_Finetune:
	lea	44(a5),a0		; pointe sur emplacement finetune !
	move.l	#1,Sample_number
	moveq	#31-1,d0		; 31 samples maxi
.loop:
	cmp.b	#$00,(a0)		; pas de finetune utilisé ??
	beq.w	.fin_loop		; alors sample suivant
.taratata:				; sinon Aarrgghhhhhh
	moveq	#0,d1
	move.b	(a0),d1			; stocke finetune utilisé
	mulu.w	#36*2,d1		; 36 notes dans chak
	move.l	d1,FineTune_Ptr

	lea	$43c(a5),a1		; pointe sur debut patterns
	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl.l	#8,d1			; mulu #256 (64 lignes * 4 longwd)
	subq	#1,d1			; pour le DBF
	move.l	d1,Notes_Loop		; nombre d'iterations !
.find_sample:
	move.l	(a1),d7			; stocke longwd (11 AC 5F 06 ex...)
	move.l	d7,d6			; save_it
	andi.l	#$F000F000,d7		; morph ($10005000 ex...)
	moveq	#0,d5
	move.w	d7,d5			; dizaine dans d5 ($5000)
	swap	d7			; d7 = $50001000
	andi.l	#$0000F000,d7		; d7 = $00001000
	ror.w	#8,d7			; morph $1000 = $0010
	rol.w	#4,d5			; morph $5000 = $0005
	add.w	d5,d7			; mix'em...   = $0015
	cmp.l	Sample_number,d7
	bne.s	.fin_fs			; pas les memes samples...next one !
.sample_found:				; trouvé !!!
	move.l	d6,d7			; reprend longwd original !
	andi.l	#$0FFF0000,d7		; garde que la note ($013C000 ex...)
	swap	d7			; d7 = $0000013C : note à trouver !!
	lea	PM_PeriodTable,a3
	add.l	FineTune_Ptr,a3		; pointe sur la bonne table_notes
	moveq	#35,d2			; compare avec les 36 notes de la table
	clr.l	FineNote_Ptr
.compare:
	cmp.w	(a3),d7			; compare !
	bne.s	.next_note
	lea	PM_PeriodTable,a4
	add.l	FineNote_Ptr,a4
	move.w	(a4),d7			; read REAL_NOTE !!!! ($00000140 ex...)
	swap	d7			; d7 = $01400000 ex...
	move.l	d6,d5			; longwd original dans d5
	andi.l	#$F000FFFF,d5		; vire note pourrave !!! ($10005F06)
	or.l	d5,d7			; MIX'EM !! ($11 40 5F 06)
	move.l	d7,(a1)			; Paste Real_LongWord !!!!!!!!
	bra.s	.fin_fs			; note_suivante
.next_note:
	addq	#2,a3			; table en .w
	add.l	#2,FineNote_Ptr
	dbf	d2,.compare
.fin_fs:
	addq	#4,a1			; note suivante
	dbf	d1,.find_sample
.fin_loop:
	lea	30(a0),a0		; check finetune sample suivant...
	add.l	#1,Sample_number
	dbf	d0,.loop
	rts


Nb_Loop:	dc.l	0
Nb_Notes:	dc.l	0
Notes_Loop:	dc.l	0
FineTune_Ptr:	dc.l	0
FineNote_Ptr:	dc.l	0
Sample_number:	dc.l	0
Pos_Ptr:	dc.l	0
Patt_Break:	dc.w	0
PM1_Notes_Table:dc.l	0
PM1_Save_a4:	dc.l	0
PM1_Save_a5:	dc.l	0

PM_PeriodTable:
; Tuning 0, Normal
	dc.w	856,808,762,720,678,640,604,570,538,508,480,453
	dc.w	428,404,381,360,339,320,302,285,269,254,240,226
	dc.w	214,202,190,180,170,160,151,143,135,127,120,113
; Tuning 1
	dc.w	850,802,757,715,674,637,601,567,535,505,477,450
	dc.w	425,401,379,357,337,318,300,284,268,253,239,225
	dc.w	213,201,189,179,169,159,150,142,134,126,119,113
; Tuning 2
	dc.w	844,796,752,709,670,632,597,563,532,502,474,447
	dc.w	422,398,376,355,335,316,298,282,266,251,237,224
	dc.w	211,199,188,177,167,158,149,141,133,125,118,112
; Tuning 3
	dc.w	838,791,746,704,665,628,592,559,528,498,470,444
	dc.w	419,395,373,352,332,314,296,280,264,249,235,222
	dc.w	209,198,187,176,166,157,148,140,132,125,118,111
; Tuning 4
	dc.w	832,785,741,699,660,623,588,555,524,495,467,441
	dc.w	416,392,370,350,330,312,294,278,262,247,233,220
	dc.w	208,196,185,175,165,156,147,139,131,124,117,110
; Tuning 5
	dc.w	826,779,736,694,655,619,584,551,520,491,463,437
	dc.w	413,390,368,347,328,309,292,276,260,245,232,219
	dc.w	206,195,184,174,164,155,146,138,130,123,116,109
; Tuning 6
	dc.w	820,774,730,689,651,614,580,547,516,487,460,434
	dc.w	410,387,365,345,325,307,290,274,258,244,230,217
	dc.w	205,193,183,172,163,154,145,137,129,122,115,109
; Tuning 7
	dc.w	814,768,725,684,646,610,575,543,513,484,457,431
	dc.w	407,384,363,342,323,305,288,272,256,242,228,216
	dc.w	204,192,181,171,161,152,144,136,128,121,114,108
; Tuning -8
	dc.w	907,856,808,762,720,678,640,604,570,538,508,480
	dc.w	453,428,404,381,360,339,320,302,285,269,254,240
	dc.w	226,214,202,190,180,170,160,151,143,135,127,120
; Tuning -7
	dc.w	900,850,802,757,715,675,636,601,567,535,505,477
	dc.w	450,425,401,379,357,337,318,300,284,268,253,238
	dc.w	225,212,200,189,179,169,159,150,142,134,126,119
; Tuning -6
	dc.w	894,844,796,752,709,670,632,597,563,532,502,474
	dc.w	447,422,398,376,355,335,316,298,282,266,251,237
	dc.w	223,211,199,188,177,167,158,149,141,133,125,118
; Tuning -5
	dc.w	887,838,791,746,704,665,628,592,559,528,498,470
	dc.w	444,419,395,373,352,332,314,296,280,264,249,235
	dc.w	222,209,198,187,176,166,157,148,140,132,125,118
; Tuning -4
	dc.w	881,832,785,741,699,660,623,588,555,524,494,467
	dc.w	441,416,392,370,350,330,312,294,278,262,247,233
	dc.w	220,208,196,185,175,165,156,147,139,131,123,117
; Tuning -3
	dc.w	875,826,779,736,694,655,619,584,551,520,491,463
	dc.w	437,413,390,368,347,328,309,292,276,260,245,232
	dc.w	219,206,195,184,174,164,155,146,138,130,123,116
; Tuning -2
	dc.w	868,820,774,730,689,651,614,580,547,516,487,460
	dc.w	434,410,387,365,345,325,307,290,274,258,244,230
	dc.w	217,205,193,183,172,163,154,145,137,129,122,115
; Tuning -1
	dc.w	862,814,768,725,684,646,610,575,543,513,484,457
	dc.w	431,407,384,363,342,323,305,288,272,256,242,228
	dc.w	216,203,192,181,171,161,152,144,136,128,121,114



*-----------------------------------------------------------------------*
;
; Promizer 2.0 Check Routine

CheckPM20:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	(a0),d1
	cmpi.l	#$60000016,d1		; Un Promizer 2.0 ??
	bne	.fail
	move.l	4(a0),d1
	cmpi.l	#$60000140,d1		; Un Promizer 2.0 ??
	bne	.fail

	lea	$16(a0),a0
	moveq	#0,d1
	move.w	(a0)+,d1
	moveq	#0,d0
	move.w	#$4E75,d0
	cmp.w	d0,d1
	bne	.fail
	move.l	(a0),d1
	move.l	#$48e77ffe,d0
	cmp.l	d0,d1
	bne	.fail

.Check2:
	lea	$1552(a4),a0		; pointe sur sample_data
	moveq	#0,d6
	moveq	#30,d0
.loop:
	moveq	#0,d1
	move.w	(a0),d1
	add.l	d1,d6			; cumul lengths
	moveq	#0,d1
	move.w	2(a0),d1
	andi.w	#$00FF,d1
	cmpi.w	#$0040,d1
	bhi	.fail
	lea	8(a0),a0
	dbf	d0,.loop	

	lsl.l	#1,d6
	beq	.fail
	move.l	d6,SampLen

	lea	$1452(a4),a0
	move.l	a0,Base
	lea	$1552(a4),a2
	adda.l	#31*8,a2		; saute les 31 sample_data
	add.l	(a2),a0			; debut des samples !
	add.l	d6,a0			; fin du pack_mod
	suba.l	a4,a0
	sub.l	a0,d4
	cmp.l	#-256,d4
	blt	.fail

	lea	$1450(a4),a0		; pointe sur nb_pos
	moveq	#0,d0
	move.w	(a0),d0			; take nb_pos
	lsr	#1,d0
	move.w	d0,Nb_Pos

; PM20_Make_PTK_Table:
	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; 1er patt = 0 d'office !
	move.l	Base(pc),a0
	addq	#2,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	moveq	#0,d1
	move.w	(a0)+,d1
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Base(pc),a1		; table de comparaison
	lea	KR_PosTable,a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,PM40_BytePos
	moveq	#0,d3
	move.w	(a1)+,d3
	sub.w	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	PM40_BytePos(pc),(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare	;!!!
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	cmpi.w	#$40,d6
	bhi	.fail

; Calc PTK Module Size

	move.l	d6,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; Promizer 2.0 to Protracker Convert routine, from Pro-Wizzy

ConvertPM20:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; Heading
	lea	$1552(a4),a0		; pointe sur sample_data
	lea	42(a1),a1
	moveq	#30,d0
.copy:
	moveq	#0,d1
	move.w	(a0)+,(a1)		; copy LENGTH
	moveq	#0,d1
	move.w	(a0)+,d1		; take FINE+VOL
	move.l	d1,d2
	andi.w	#$FF00,d2		; vire le volume
	divu	#$200,d2		; divise par 2
	andi.w	#$00FF,d1		; vire le mauvais finetune
	lsl	#8,d2			; decale finetune
	add.w	d2,d1			; remet le bon finetune
	move.w	d1,2(a1)		; copy FINE+VOL

	move.l	(a0)+,4(a1)		; copy REPEAT+REPLEN
	lea	30(a1),a1
	dbf	d0,.copy

	lea	$1450(a4),a0		; pointe sur nb_pos
	moveq	#0,d0
	move.w	(a0)+,d0		; take nb_pos
	lsr	#1,d0
	lea	$3b6(a5),a1
	move.b	d0,(a1)+
	move.b	#$7F,(a1)+

PM20_Copy_Table:
	lea	KR_PosTable,a0
	moveq	#$7f,d0
.loop:
	move.b	(a0)+,(a1)+
	dbf	d0,.loop

PM2_Convert_Patterns:
	lea	$1552(a4),a0
	adda.l	#31*8,a0		; saute les 31 sample_data
	move.l	(a0)+,PM2_Add_Samples
	move.l	(a0)+,PM2_Add_Notes
	move.l	a0,PM2_Notes_Table	; debut des jump_notes
	move.l	Base(pc),a0
	adda.l	PM2_Add_Samples(pc),a0
	move.l	a0,PM2_Adr_Samples	; debut des samples !
	move.l	Base(pc),a0
	adda.l	PM2_Add_Notes(pc),a0
	move.l	a0,PM2_Adr_Notes	; debut des notes_réelles (packées) !
	subq	#4,a0
	movea.l	a0,a6			; adresse où arreter la conversion !!

	move.l	a5,PM2_Save_a5

	move.l	Base(pc),a3
	moveq	#-1,d5
	lea	$3b8(a5),a4

	clr.l	Compteur
	move.l	#-1,Compteur2
	clr.l	Patt_Nb

	moveq	#0,d7
	move.w	Nb_Pos,d7
	subq	#1,d7
.loop:
	moveq	#0,d6
	move.b	(a4)+,d6
	cmp.w	d5,d6
	bgt.s	.okette
	addq	#2,a3
	bra.w	.fini
.okette:
	addq.l	#1,d5
	move.l	PM2_Notes_Table(pc),a0
	moveq	#0,d6
	move.w	(a3)+,d6
	add.l	d6,a0

	move.l	PM2_Save_a5(pc),a5
	lea	$43c(a5),a1
	add.l	Patt_Nb(pc),a1
.Ploop:
	cmp.l	a6,a0
	bge.w	PM2_Copy_Samples
	add.l	#1,Compteur2
	cmp.l	#4,Compteur2
	blt.s	.not_yet
	clr.l	Compteur2
.not_yet:
	move.w	(a0)+,d1		; take valeur_jump
	tst.w	d1			; pas de note ??
	beq.s	.next_note
	move.l	PM2_Adr_Notes(pc),a2
	subq	#1,d1
	lsl	#2,d1
	move.l	(a2,d1.w),d2		; real_note (mais packée kan meme)

	bsr	PM2_Morph
	move.l	d2,(a1)
				; ###### TEST SI PATTERN BREAK !!!!
	move.l	d2,d3
	andi.l	#$00000F00,d3		; isole commande
	cmpi.w	#$0D00,d3
	beq.s	.ARGH
	cmpi.w	#$0B00,d3
	bne.s	.next_note		; non OUF, note suivante
.ARGH:
	bsr	PM2_SPECIAL
	bra.s	.next_patt
.next_note:
	addq	#4,a1			; note suivante
	add.l	#4,Compteur
	cmp.l	#$400,Compteur		; 1 pattern refait ??
	blt.w	.Ploop			; pasa encore : remonte
.next_patt:
	clr.l	Compteur
	move.l	#-1,Compteur2
	add.l	#$400,Patt_Nb		; pattern suivant
.fini:
	dbf	d7,.loop

PM2_Copy_Samples:
	move.l	PM2_Save_a5(pc),a1
	adda.l	#$43c,a1		; jump header
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#8,d0
	lsl.l	#2,d0
	add.l	d0,a1			; jump patterns

	move.l	PM2_Adr_Samples(pc),a0
	subq	#4,a0			; !!! evite le sifflement
	move.l	a0,a3
	add.l	SampLen,a3
.copy:
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:

	move.l	PM2_Save_a5(pc),a5
	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts



PM2_SPECIAL:
	cmpi.l	#3,Compteur2		; le $D00 est sur la voie 4 ??
	beq.s	.bye
	addq	#4,a1			; note suivante
	moveq	#2,d0			; 3 voies maxi à copier
	sub.l	Compteur2(pc),d0
.loop:
	move.w	(a0)+,d1
	tst.w	d1			; pas de note ??
	beq.s	.next
	move.l	PM2_Adr_Notes(pc),a2
	subq	#1,d1
	lsl	#2,d1
	move.l	(a2,d1.w),d2		; real_note (mais packée kan meme)

	bsr	PM2_Morph
	move.l	d2,(a1)+
.next:
	dbf	d0,.loop
.bye:
	rts

PM2_Morph:
	move.l	d7,-(sp)

	move.l	d2,d3			; note = $04 14 0F 03 par exemple...
	move.l	d2,d4			; note = $04 14 0F 03 par exemple...
	andi.l	#$FF000000,d3		; garde le sample
	andi.l	#$00FF0000,d4		; garde la note

	rol.l	#8,d3
	lsr	#2,d3			; divise par 4 pour avoir le bon smpl
	moveq	#0,d7
	cmpi.w	#$10,d3
	blt.s	.normal
	subi.w	#$0010,d3
	move.l	#$10000000,d7		; prepare longmot final
.normal:
	ror.w	#4,d3
	add.w	d3,d2			; mot faible OK

	swap	d4
	subq	#2,d4
	lea	mt_periodtable,a5
	move.w	(a5,d4.w),d4
	swap	d4
	add.l	d7,d4			; mot fort OK
	andi.l	#$0000FFFF,d2		; vire le mauvais mot fort
	add.l	d4,d2			; met le bon...

	move.l	(sp)+,d7
	rts



Compteur:	dc.l	0
Compteur2:	dc.l	-1
Patt_Nb:	dc.l	0
PM2_Save_a5:	dc.l	0
PM2_Add_Samples:dc.l	0
PM2_Add_Notes:	dc.l	0
PM2_Adr_Samples:dc.l	0
PM2_Adr_Notes:	dc.l	0
PM2_Notes_Table:dc.l	0
Base:		dc.l	0


*-----------------------------------------------------------------------*
;
; Promizer 4.0 Check Routine

CheckPM40:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	(a0)+,d0
	cmpi.l	#'PM40',d0
	bne	.fail

	move.l	(a0)+,d0
	cmpi.w	#$7F,d0
	bhi	.fail
	moveq	#$7F,d0
.patt:
	moveq	#0,d1
	move.w	(a0)+,d1
	btst	#0,d1
	bne	.fail
	dbf	d0,.patt

	moveq	#30,d0
	moveq	#0,d5
.loop:
	moveq	#0,d1
	move.w	(a0),d1			; take length
	bmi	.fail			; negatif ? fuck
	bne.s	.other			; non-nul ?
	add.w	2(a0),d1		; tout le reste doit etre nul!
	add.w	4(a0),d1
	add.w	6(a0),d1
	bne	.fail
.other:
	add.l	d1,d5			; cumule
	move.w	2(a0),d1		; take finetune+volume
	move.l	d1,d2
	andi.w	#$00FF,d1
	andi.w	#$FF00,d2
	cmpi.w	#$0040,d1
	bhi	.fail
	cmpi.w	#$0F00,d2
	bhi	.fail
	lea	8(a0),a0
	dbf	d0,.loop

	lsl.l	#1,d5
	beq	.fail
	move.l	d5,SampLen

	move.l	a4,a0
	adda.l	$200(a0),a0
	addq	#4,a0
	add.l	d5,a0
	suba.l	a4,a0
	sub.l	a0,d4
	cmpi.l	#-256,d4
	blt.w	.fail

; patt_pos
	lea	8(a4),a0
	move.l	a0,Table_Originale
	moveq	#0,d0
	move.b	-1(a0),d0
	move.w	d0,Nb_Pos

; PM40_Make_PTK_Table:
	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; 1er patt = 0 d'office !
	move.l	Table_Originale(pc),a0
	addq	#2,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	moveq	#0,d1
	move.w	(a0)+,d1
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Table_Originale(pc),a1	; table de comparaison
	lea	KR_PosTable,a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,PM40_BytePos
	moveq	#0,d3
	move.w	(a1)+,d3
	sub.w	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	PM40_BytePos(pc),(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	cmpi.w	#$40,d6
	bhi	.fail

; Calc PTK Module Size

	move.l	d6,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fail:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; Promizer 4.0 to Protracker Convert routine, from Pro-Wizzy

ConvertPM40:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; Heading
	lea	$108(a0),a0
	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop

	move.l	a4,a2
	move.l	a4,a3
	add.l	(a0)+,a3		; $890 = Adresse start samples
	addq	#4,a3
	move.l	a3,PM40_Adr_Samples
	add.l	(a0)+,a2		; $804 = Adresse start real notes
	addq	#4,a2
	move.l	a2,PM40_Adr_Real_Notes
	move.l	a0,PM40_Adr_Table_Notes

	lea	8(a4),a0
	lea	$3b6(a5),a1
	moveq	#0,d0
	move.b	-1(a0),d0
	move.b	d0,(a1)+
	move.b	#$7F,(a1)+

PM40_Copy_Table:
	lea	KR_PosTable,a0
	moveq	#$7f,d0
.loop:
	move.b	(a0)+,(a1)+
	dbf	d0,.loop

PM40_DoIt:
	move.l	a5,PM40_Save_a5

	lea	$43c(a5),a1
	lea	$3b8(a5),a5
	move.l	PM40_Adr_Real_Notes(pc),a2
	move.l	a2,a4			; SAVE_IT
	move.l	Table_Originale(pc),a3
	moveq	#-1,d5
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0			; nombre de notes
.Ploop:
	moveq	#0,d6
	move.b	(a5)+,d6
	cmp.w	d5,d6
	bgt.s	.okette
	addq	#2,a3
	bra.w	.fini
.okette:
	addq.l	#1,d5
	move.l	a1,a6			; SAVE_IT
	move.l	PM40_Adr_Table_Notes(pc),a0
	add.w	(a3)+,a0
	move.l	#$ff,d1
.Nloop:
	moveq	#0,d2
	move.w	(a0)+,d2
	lsl	#2,d2
	add.w	d2,a2			; pointe sur la VRAIE note
	move.l	(a2),d3			; take it (07 11 0 C20)
.Traite_Sample:
	moveq	#0,d7
	move.l	d3,d2
	andi.l	#$FF000000,d2		; Isole sample_nb
	rol.l	#8,d2
	cmpi.w	#$10,d2
	blt.s	.ok
	subi.w	#$10,d2
	move.l	#$10000000,d7		; prepare longword final
.ok:
	ror.w	#4,d2
	add.w	d2,d3			; mixe X000 avec word_cmd
.Traite_Note:
	move.l	d3,d2
	andi.l	#$00FF0000,d2		; Isole note
	swap	d2
	tst.w	d2
	beq.s	.yeah
	lsl	#1,d2			; note *2
	subq	#2,d2
	move.l	a0,-(sp)
	lea	mt_periodtable,a0
	move.w	(a0,d2.w),d2
	move.l	(sp)+,a0
.yeah:
	swap	d2
	add.l	d2,d7
	add.w	d3,d7
	move.l	d7,(a1)+
	move.l	a4,a2			; RESTORE_IT
	dbf	d1,.Nloop
	move.l	a6,a1			; RESTORE_IT
	lea	$400(a1),a1
.fini:
	dbf	d0,.Ploop

PM40_Copy_Samples:
	move.l	a1,a2

	move.l	PM40_Adr_Samples(pc),a0
	move.l	a0,a1
	add.l	SampLen,a1		; fin des sons
.copy:
	cmp.l	a1,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:

	move.l	PM40_Save_a5(pc),a5
	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts

PM40_Save_a5:		dc.l	0
PM40_Adr_Samples:	dc.l	0
PM40_Adr_Real_Notes:	dc.l	0
PM40_Adr_Table_Notes:	dc.l	0
PM40_BytePos:		dc.b	0
	even

*-----------------------------------------------------------------------*
;
; FC-M Check Routine

CheckFCM
	move.l	a0,a4
	move.l	a4,d4
	add.l	d0,d4

	cmpi.l	#'FC-M',(a4)			; ID
	bne.s	.Fail

	move.l	#"INST",d0
	bsr	.FindPart
	tst.l	d0
	beq.s	.Fail

	move.l	a0,a1
	moveq	#31-1,d0
	moveq	#0,d2
.InstLoop
	moveq	#0,d1
	move.w	(a1),d1
	add.l	d1,d2
	addq.w	#8,a1
	dbra	d0,.InstLoop

	move.l	#"LONG",d0
	bsr	.FindPart
	tst.l	d0
	beq.s	.Fail

	moveq	#0,d3
	move.b	(a0),d3
	subq.w	#1,d3

	move.l	#"PATT",d0
	bsr	.FindPart
	tst.l	d0
	beq.s	.Fail

	move.l	a0,a1
	moveq	#0,d0
	moveq	#0,d1
.PattLoop
	move.b	(a1)+,d1
	cmp.b	d1,d0
	bcc.s	.SkipPatt
	move.b	d1,d0
.SkipPatt
	dbra	d3,.PattLoop

	addq.w	#1,d0
	mulu	#1024,d0			; pattern * 1024

	add.l	d2,d2				; samplesize * 2
	add.l	d2,d0

	add.l	#$043C+4,d0			; Module-Header + 4 Bytes
	bra.s	.End
.Fail	moveq	#0,d0				; check failed
.End	rts


.FindPart
	lea	(a4),a0
.FindLoop
	cmp.l	(a0),d0
	beq.s	.FoundPart
	addq.l	#$02,a0
	cmp.l	d4,a0
	bcs.s	.FindLoop
	moveq	#$00,d0
.FoundPart
	addq.l	#$04,a0
	rts

*-----------------------------------------------------------------------*
;
; FC-M to ProTracker Converter by Delirium (another PT-Clone bites the dust :-))

ConvertFCM
	move.l	a0,a4				; copy Ptr's
	move.l	a1,a5

	move.l	a4,d4
	add.l	d0,d4

	move.l	#"NAME",d0
	bsr	.FindPart
	tst.l	d0
	beq	.Fail
	move.l	a0,.NamePtr

	move.l	#"INST",d0
	bsr	.FindPart
	tst.l	d0
	beq	.Fail
	move.l	a0,.InstPtr

	move.l	#"LONG",d0
	bsr	.FindPart
	tst.l	d0
	beq	.Fail
	move.l	a0,.LongPtr

	move.l	#"PATT",d0
	bsr	.FindPart
	tst.l	d0
	beq	.Fail
	move.l	a0,.PattPtr

	move.l	#"SONG",d0
	bsr	.FindPart
	tst.l	d0
	beq	.Fail
	move.l	a0,.SongPtr

	move.l	#"SAMP",d0
	bsr	.FindPart
	tst.l	d0
	beq	.Fail
	move.l	a0,.SampPtr

	move.l	.NamePtr(pc),a0
	lea	(a5),a1
	moveq	#20-1,d0
.CopyName
	move.b	(a0)+,(a1)+
	dbra	d0,.CopyName

	move.l	.InstPtr(pc),a0
	lea	20(a5),a1
	moveq	#31-1,d0
	moveq	#0,d2
.MakeHeader
	lea	22(a1),a1
	moveq	#0,d1
	move.w	(a0)+,d1
	add.l	d1,d2
	move.w	d1,(a1)+
	move.w	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	dbra	d0,.MakeHeader

	move.l	.LongPtr(pc),a0
	move.w	(a0),$3b6(a5)
	moveq	#0,d0
	move.b	(a0),d0
	move.l	.PattPtr(pc),a0
	lea	$3b8(a5),a1
	subq.w	#1,d0
	moveq	#0,d1
	moveq	#0,d3
.CopyPatt
	move.b	(a0)+,d1
	cmp.b	d1,d3
	bcc.s	.SkipPatt
	move.b	d1,d3
.SkipPatt
	move.b	d1,(a1)+
	dbra	d0,.CopyPatt

	move.l	#"M.K.",$438(a5)

	move.l	.SongPtr(pc),a0
	lea	$43C(a5),a1
	addq.w	#1,d3
	lsl.l	#8,d3
.CopySong
	move.l	(a0)+,(a1)+
	subq.l	#1,d3
	bne.s	.CopySong

	move.l	.SampPtr(pc),a0
.CopySamp
	move.w	(a0)+,(a1)+
	subq.l	#1,d2
	bne.s	.CopySamp
	clr.l	(a1)+

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0				; all right
	rts

.Fail	moveq	#-1,d0
	rts

.FindPart
	lea	(a4),a0
.FindLoop
	cmp.l	(a0),d0
	beq.s	.FoundPart
	addq.l	#$02,a0
	cmp.l	d4,a0
	bcs.s	.FindLoop
	moveq	#$00,d0
.FoundPart
	addq.l	#$04,a0
	rts


.NamePtr:	dc.l 0
.InstPtr:	dc.l 0
.LongPtr:	dc.l 0
.PattPtr:	dc.l 0
.SongPtr:	dc.l 0
.SampPtr:	dc.l 0



Crypto_PP_Same:				; Same checks for Hmc, PPxx, GV
	moveq	#0,d7			; at start : bad
	move.l	(a0),d3
	move.l	d3,d1
	andi.l	#$FF000000,d1		; garde nb_pos
	beq	.fuck
	bmi	.fuck
	move.l	d3,d1
	andi.l	#$0000FF00,d1		; garde nb_patt 0
	cmpi.w	#$3F00,d1		; si patt > $3F, pas le peine
	bhi	.fuck
	move.l	d3,d1
	andi.l	#$000000FF,d1		; garde nb_patt 1
	cmpi.w	#$003F,d1		; si patt > $3F, pas le peine
	bhi	.fuck
	move.l	d3,d1
	andi.l	#$00FF0000,d1		; garde constante
	cmpi.l	#$007F0000,d1		; si const <> $7F, pas le peine
	beq.s	.mb

;	cmpi.l	#$00780000,d1		; removed to make PP21.Earwig-08 work
;	bne	.fuck			; (mld)

.mb:
	move.l	$17a(a4),d0
	cmpi.l	#'M.K.',d0
	beq	.fuck
	cmpi.l	#'M&K!',d0
	beq	.fuck
	cmpi.l	#'FLT4',d0
	beq	.fuck
	cmpi.l	#'EXO4',d0
	beq	.fuck
	cmpi.l	#'UNIC',d0
	beq	.fuck
	cmpi.l	#'SNT.',d0
	beq	.fuck

	movea.l	a4,a0
	moveq	#0,d5			; longueur totale des samples
	moveq	#30,d0
.loop:
	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d7
	move.w	(a0)+,d1		; length
	move.w	(a0)+,d2		; vol
	move.w	(a0)+,d3		; repeat
	move.w	(a0)+,d7		; replen
	move.l	d2,d6
	andi.w	#$00FF,d2
	cmpi.w	#$0040,d2		; si volume superieur a $40 : .fuck
	bhi	.fuck
	andi.w	#$FF00,d6
	cmpi.w	#$0F00,d6		; si fineT superieur a $0F : .fuck
	bhi	.fuck
	tst.w	d1			; length=0 ? (no sample)
	beq.s	.next			; oui : au suivant
	add.l	d1,d5
	add.l	d3,d7			; sinon tester...
	cmp.l	d1,d7
	bhi	.fuck		; si repeat+replen > length : .fuck
.next:
	dbf	d0,.loop

	lsl.l	#1,d5			; longueur totale * 2
	beq	.fuck
	move.l	d5,SampLen

	move.l	#'OKAY',d7
.fuck:	rts


*-----------------------------------------------------------------------*
;
; CryptoPacker Check Routine (HeatSeeker mc1.0)

CheckCrypto:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	$f8(a0),a0
	bsr	Crypto_PP_Same
	tst.l	d7
	beq	.fuck			; si tjrs à 0 = bye bye

	lea	$f8(a4),a0
	moveq	#0,d0
	move.b	(a0),d0
	move.w	d0,Nb_Pos
	addq	#2,a0			; pointe sur la patt_table
	bsr	Get_PGP
	cmpi.w	#$40,d1
	bhi	.fuck

	moveq	#0,d6			; compteur
	lea	$17a(a4),a0		; les notes
	moveq	#0,d0
	move.w	#$200-1,d0
.loop1:
	move.l	(a0)+,d1
	swap	d1
	cmpi.w	#$8000,d1		; test valeur note_vide
	beq.s	.mouais
	tst.w	d1
	beq.s	.next1
	cmpi.w	#$0071,d1
	blo	.fuck
					; next check removed to detect
					; crb.4D_#6
;	cmpi.w	#$1358,d1		; si > $1358 : .fuck
;	bhi	.fuck
	bra.s	.next1
.mouais:
	addq	#1,d6			; trouvé ? incremente d6
.next1:
	dbf	d0,.loop1
	tst.l	d6			; si compteur toujours a 0 : .fuck
	beq	.fuck

	lea	$17a(a4),a1		; saute heading
	add.l	SampLen,a1		; saute samples_length
	movea.l	a4,a0
	suba.l	a0,a1
	cmp.l	#-256,a1
	blt	.fuck

	move.l	#$400*$40,d0		; SPECIAL POUR LE FORMAT CRYPTO !
	move.l	#$10001,d1		; (Public = $1, $2 = chip, $4 = fast)
	CALLEXEC AllocMem

	tst.l	d0
	beq.s	.fuck
	move.l	d0,Tracks		; adresse stockée !

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts

Tracks:	dc.l	0

*-----------------------------------------------------------------------*
;
; HeatSeeker to Protracker Convert routine, from Pro-Wizzy

ConvertCrypto:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop		; copy sample_data

	lea	$3b6(a5),a1
	move.w	(a0)+,(a1)+		; copy nb_pos + cnste
	moveq	#31,d0			; 32 * 4 = 128
.loop1:
	move.l	(a0)+,(a1)+
	dbf	d0,.loop1		; copy 128 patt_pos

	move.l	#'M.K.',(a1)+

Crypto_Make_Patterns:
	move.l	Tracks(pc),a3
	lea	$17a(a4),a0
	lea	$43c(a5),a1
	moveq	#0,d6
	move.w	Nb_Patt,d6
	subq	#1,d6
.Ploop:
	moveq	#3,d7
.Vloop:
	moveq	#63,d0
.loop:
	move.l	(a0)+,d1
	move.l	d1,d2
	swap	d2
	andi.l	#$0000FFFF,d2
	cmpi.l	#$c000,d2
	beq.s	.copy_track
.check:
	cmpi.l	#$8000,d2
	blt.s	.note_normale
	moveq	#0,d3
	move.b	d1,d3			; take jump_byte
.vide:
	clr.l	(a1)
	lea	16(a1),a1
	clr.l	(a3)+
	subq	#1,d0
	dbf	d3,.vide
	addq	#1,d0
	bra.s	.next
.copy_track:
	moveq	#0,d3
	move.w	d1,d3
	lsr.w	#2,d3			; divise par 4
	lsl.w	#8,d3			; mulu #$100
	move.l	Tracks(pc),a6
	add.l	d3,a6

.Crypto_Idem:
	moveq	#63,d4
.ID_loop:
	move.l	(a6),(a1)
	lea	16(a1),a1
	move.l	(a6)+,(a3)+
	dbf	d4,.ID_loop

	moveq	#0,d0			; force fin_loop
	bra.s	.next
.note_normale:
	move.l	d1,(a1)
	lea	16(a1),a1
	move.l	d1,(a3)+
.next:
	dbf	d0,.loop
	suba.l	#16*64,a1
	addq	#4,a1
	dbf	d7,.Vloop
	suba.l	#16,a1
	adda.l	#$400,a1
	dbf	d6,.Ploop

Crypto_Test_Filter:
	lea	$43c(a5),a1
	moveq	#0,d6
	move.w	Nb_Patt,d6
	lsl	#8,d6
	lsl.l	#2,d6
	subq.l	#1,d6
.loop:
	move.l	(a1),d0
	move.l	d0,d7
	andi.l	#$00000F00,d7		; isole commande
	cmpi.w	#$0E00,d7
	bne.s	.next
	addq	#1,d7			; force le filtre OFF ($E01)
	move.b	d7,d0			; copie filter_value
	move.l	d0,(a1)			; remet la note
.next:
	addq	#4,a1
	dbf	d6,.loop

Crypto_Calcul_Module_Size:		; compte les notes pour 
	moveq	#0,d7			; arriver aux samples !
	move.w	Nb_Patt,d7
	lsl	#8,d7			; $18 patt = $1800 notes !
	lea	$17a(a4),a0		; bcoz $100 (256) = 64 * 4
	moveq	#0,d0			;		 notes   voies
.loop:
	cmp.l	d7,d0			; $1800 notes reached ???
	bge.s	.stop			; yes : STOP_IT
	move.l	(a0)+,d1
	move.l	d1,d2
	swap	d2
	andi.l	#$0000FFFF,d2
	cmpi.l	#$8000,d2
	bne.s	.chk2
	andi.l	#$000000FF,d1		; isole byte-howmany
	add.l	d1,d0			; ajoute x-notes
	bra.s	.note_normale
.chk2:
	cmpi.l	#$C000,d2
	bne.s	.note_normale
	add.l	#63,d0			; ajoute 64 notes
.note_normale:
	addq.l	#1,d0
	bra.s	.loop
.stop:					; a0 = adresse des samples !
	move.l	a0,a1
	add.l	SampLen,a1		; a1 = adresse de fin des samples !

Crypto_Copy_Samples:
	moveq	#0,d0
	move.w	Nb_Patt,d0		; $18
	lsl	#8,d0
	lsl.l	#2,d0
	lea	$43c(a5),a2		; jump header
	add.l	d0,a2			; jump patterns
.loop:					; donc pointe là où mettre les samples
	cmp.l	a1,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.loop
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

;Do_FreeMem:				; mettre adresse dans a1
	move.l	#$400*$40,d0
	move.l	Tracks(pc),a1
	CALLEXEC FreeMem

	moveq	#0,d0				; all right
	rts



*-----------------------------------------------------------------------*
;
; Xann Player Check Routine

CheckXann:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	(a0),d3
	cmpi.l	#$000F8000,d3
	bhi	.fuck
	move.l	d3,d1
	move.l	4(a0),d2		; take longmot suivant
	beq	.fuck
	cmpi.l	#$000F8000,d2
	bhi	.fuck
	cmp.l	d1,d2
	ble.s	.sub_d2
	sub.l	d1,d2			; il faut trouver 2 valeurs dont la
	move.l	d2,d1			; difference est multiple de $400
	bra.s	.all
  .sub_d2:
	sub.l	d2,d1
  .all:
	move.l	d1,d2
	lsr	#8,d1
	lsr.l	#2,d1			; divu $400
	lsl	#8,d1
	lsl.l	#2,d1			; mulu $400
	cmp.l	d1,d2
	bne	.fuck

.Check:
	lea	$206(a0),a0
	moveq	#0,d7
	moveq	#30,d0
.loop:
	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d5
	move.w	(a0),d1			; take volume
	move.w	6(a0),d2		; take replen
	move.w	12(a0),d3		; take length
	move.w	d1,d5
	andi.w	#$00FF,d1
	cmpi.w	#$0040,d1
	bhi	.fuck		; si VOLUME depasse $40 : Pas_Xann !
	andi.w	#$FF00,d5
	cmpi.w	#$0F00,d5		; si Finetune > $0F : Pas_Xann !
	bhi	.fuck		; si VOLUME depasse $40 : Pas_Xann !
	tst.w	d3			; length = 0 ?
	beq.s	.yo
	cmpi.w	#$8000,d3
	bhi	.fuck		; pas de length > $8000
	cmp.w	d3,d2			; Replen > Length ? Pas_Xann !!
	bhi	.fuck
	add.l	d3,d7
	bra.s	.next
.yo:
	cmpi.w	#$0001,d2		; oui : replen DOIT ETRE EGAL a 0001 !
	bne	.fuck		; c bon, test suivant
	move.l	2(a0),d1		; take repeat_adr
	move.l	8(a0),d2		; take start_adr
	cmp.l	d1,d2
	bne	.fuck
.next:
	lea	$10(a0),a0		; next sample_line
	dbf	d0,.loop

	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	lea	$43c(a4),a0
	move.l	#255,d0
  .loop3:
	move.l	(a0)+,d1
	move.l	d1,d2
	andi.l	#$00FF0000,d2
	swap	d2
	cmpi.w	#$48,d2			; test note > $48
	bhi	.fuck
	move.l	d1,d2
	andi.l	#$0000FF00,d2
	cmpi.w	#$4800,d2		; test si commande volume
	bne.s	.next3
	cmpi.b	#$40,d1
	bhi	.fuck			; oui, alors pas + de $40 !
  .next3:
	dbf	d0,.loop3

.Check1:
	movea.l	a4,a0
	move.l	(a0)+,d3		; take 1ere valeur
	move.l	d3,d5
	moveq	#1,d2			; compteur à 1 au début
	moveq	#$7d,d0
.loop2:
	move.l	(a0)+,d1		; take patt_adr
	tst.l	d1			; rien ??
	beq.s	.next2			; n'incrémente pas d2
	cmp.l	d3,d1			; si new<old, intervertir
	bge.s	.jump			; sinon, passer
	move.l	d1,d3			; change_it
.jump:
	cmp.l	d5,d1			; si new>old, intervertir
	ble.s	.jump2
	move.l	d1,d5
.jump2:
	addq	#1,d2			; incrémente compteur
.next2:
	dbf	d0,.loop2
	tst.w	d2			; d2 = 0 ??
	beq	.fuck			; oui : FUUUUCCCCCKKKK !!
	move.w	d2,Nb_Pos		; stocke nombre de positions,
	move.l	d3,Plus_Petit_Patt	; plus petite adresse_patt,
	move.l	d5,Plus_Grand_Patt	; et plus grande adresse_patt
	sub.l	d3,d5			; ote +petit du +grand
	beq	.fuck		; plus petit = plus grand ?? Pas_Xann !
	move.l	d5,d3
	lsr	#8,d3
	lsr.l	#2,d3
	lsl	#8,d3
	lsl.l	#2,d3
	cmp.l	d3,d5
	bne	.fuck
	lsr.l	#8,d5
	lsr.l	#2,d5
	addq	#1,d5			; nombre de patterns différents
	move.w	d5,Nb_Patt		; stocke_it

	move.l	Plus_Petit_Patt,d3
	andi.w	#$F000,d3		; vire le "43C"
	move.l	d3,Xann_Origine
	move.l	a4,a0
	subq	#1,d2
.replace:
	sub.l	d3,(a0)+
	dbf	d2,.replace

	lea	$20e(a4),a0
	moveq	#29,d0
	move.l	Plus_Grand_Patt(pc),d7
	addi.l	#$400,d7		; fin des patt, deb des sons
	cmp.l	(a0),d7
	bne	.fuck
	move.l	(a0),d3			; take 1ere start adr
.up:
	move.l	(a0),d1			; take 1ere start adr
	beq	.fuck
	move.l	$10(a0),d2		; take 2eme start adr (smpl+1)
	cmp.l	d1,d2
	bcs	.fuck			; si 2eme < 1ere : FUUCCCCKKKK!
	lea	$10(a0),a0
	dbf	d0,.up
	cmp.l	d3,d2
	beq	.fuck

	lea	$208(a4),a0
	move.l	Xann_Origine,d3
	moveq	#30,d0
.repla:
	sub.l	d3,(a0)
	sub.l	d3,6(a0)
	lea	$10(a0),a0
	dbf	d0,.repla

.check3:
	move.l	a4,a0
	move.l	$20e(a0),d0
	add.l	d0,a0			; debut samples

	add.l	SampLen,a0
	movea.l	a4,a1
	add.l	d4,a1			; fin mod chargé
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts

Plus_Petit_Patt:	dc.l	0
Plus_Grand_Patt:	dc.l	0
Xann_Origine:		dc.l	0

*-----------------------------------------------------------------------*
;
; XannPlayer to Protracker Convert routine, from Pro-Wizzy

ConvertXann:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	lea	$206(a0),a0
	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.w	(a0),2(a1)		; move VOLUME
	move.w	12(a0),(a1)		; take LENGTH
	move.l	2(a0),d1		; take repeat_adr
	sub.l	8(a0),d1		; ote start_adr
	lsr.l	#1,d1			; divise par 2 avant de la stocker
	move.w	d1,4(a1)		; move REPEAT
	move.w	6(a0),6(a1)		; move REPLEN
	lea	$10(a0),a0
	lea	30(a1),a1
	dbf	d0,.loop

	moveq	#0,d4
	move.w	Nb_Pos,d4
	lea	$3b6(a5),a1
	move.b	d4,(a1)+
	move.b	#$7F,(a1)
	move.l	#'M.K.',$438(a5)

Xann_Patt_Pos:
	movea.l	a4,a0
	lea	$3B8(a5),a1
	move.l	Xann_Origine(pc),d3
	move.l	Plus_Petit_Patt(pc),d4
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
.loop:
	move.l	(a0)+,d1
	add.l	d3,d1
	sub.l	d4,d1
	lsr	#8,d1
	lsr.l	#2,d1
	move.b	d1,(a1)+
	dbf	d0,.loop

Xann_Notes:
	move.l	a4,a0
	move.l	Plus_Petit_Patt(pc),d1
	sub.l	Xann_Origine(pc),d1
	add.l	d1,a0			; réel 1er pattern
	lea	$43c(a5),a1
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#8,d0			; mulu 256   (64l * 4o)
	subq	#1,d0			; une de moins pour le DBF
.loop:
	move.l	(a0)+,d1
	move.l	d1,d2
	andi.l	#$FF000000,d2		; isole sample_number
	rol.l	#8,d2			; 58000000 = 00000058	
	lsr.w	#3,d2			; divise par 8
	moveq	#0,d7
	cmpi.w	#$10,d2
	blt.s	.plus_petit
	subi.w	#$0010,d2		; vire la dizaine
	move.l	#$10000000,d7		; prepare note finale (smpl >= $10)
.plus_petit:
	ror.w	#4,d2			; 0002 = 2000
	add.w	d2,d7

	move.l	d1,d2
	andi.l	#$00FF0000,d2		; isole la note packée
	swap	d2			; 00320000 = 00000032
	tst.w	d2			; pas de note ??
	bgt.s	.ok_note
	moveq	#0,d2
	bra.s	.zou
.ok_note:
	subq	#2,d2
	lea	mt_periodtable,a6
	move.w	(a6,d2.w),d2		; get real_note
	swap	d2			; 000001AC = 01AC0000
.zou:
	add.l	d2,d7			; note finale presk complete....

	move.l	d1,d2
	moveq	#0,d3
	move.b	d2,d3			; stocke VALEUR
	andi.l	#$0000FF00,d2		; isole la cmd !

; c'est parti pour les mega-tests !!!!!!!!!!!!!!

	tst.w	d2			; pas de cmd ? (tant mieux !!!!)
	bne.s	.test0
	move.w	#$0000,d2
	bra.w	.fini
.test0:
	cmpi.w	#$0800,d2		; cmd 1xx ????
	bne.s	.test1
	subi.w	#$0700,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test1:
	cmpi.w	#$0C00,d2		; cmd 2xx ????
	bne.s	.test2
	subi.w	#$0A00,d2		; 0C-0A = 02 = porta down
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test2:
	cmpi.w	#$1000,d2		; cmd 3 seule ????
	bne.s	.test3
	subi.w	#$0D00,d2
	bra.w	.fini
.test3:
	cmpi.w	#$1400,d2		; cmd 3xx ????
	bne.s	.test4
	subi.w	#$1100,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test4:
	cmpi.w	#$1800,d2		; cmd 4 seule ????
	bne.s	.test5
	subi.w	#$1400,d2
	bra.w	.fini
.test5:
	cmpi.w	#$1C00,d2		; cmd 4xx ????
	bne.s	.test5b
	subi.w	#$1800,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test5b:
	cmpi.w	#$2000,d2		; cmd 5x0 ????
	bne.s	.test6
	subi.w	#$1B00,d2
	rol.w	#4,d3			; 0006 = 0060
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test6:
	cmpi.w	#$2400,d2		; cmd 50x ????
	bne.s	.test6b
	subi.w	#$1F00,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test6b:
	cmpi.w	#$2800,d2		; cmd 6x0 ????
	bne.s	.test7
	subi.w	#$2200,d2
	rol.w	#4,d3			; 0006 = 0060
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test7:
	cmpi.w	#$2C00,d2		; cmd 60x ????
	bne.s	.test7b
	subi.w	#$2600,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test7b:
	cmpi.w	#$3800,d2		; cmd 9xx ????
	bne.s	.test8
	subi.w	#$2F00,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test8:
	cmpi.w	#$3C00,d2		; cmd Ax0 ????
	bne.s	.test9
	subi.w	#$3200,d2
	rol.w	#4,d3			; 0006 = 0060
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test9:
	cmpi.w	#$4000,d2		; cmd A0x ????
	bne.s	.test10
	subi.w	#$3600,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test10:
	cmpi.w	#$4400,d2		; cmd Bxx ????
	bne.s	.test10b
	subi.w	#$3900,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test10b:
	cmpi.w	#$4800,d2		; cmd Cxx ????
	bne.s	.test11
	subi.w	#$3C00,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test11:
	cmpi.w	#$4C00,d2		; cmd Dxx ????
	bne.s	.test12
	subi.w	#$3F00,d2
	bra.w	.fini
.test12:
	cmpi.w	#$5000,d2		; cmd Fxx ????
	bne.s	.test13
	subi.w	#$4100,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test13:
	cmpi.w	#$5800,d2		; cmd E01 ????
	bne.s	.test14
	move.w	#$0E01,d2
	bra.w	.fini
.test14:
	cmpi.w	#$5C00,d2		; cmd E1x ????
	bne.s	.test15
	move.w	#$0E10,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test15:
	cmpi.w	#$6000,d2		; cmd E2x ????
	bne.s	.test16
	move.w	#$0E20,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test16:
	cmpi.w	#$8400,d2		; cmd E9x ????
	bne.s	.test16b
	move.w	#$0E90,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test16b:
	cmpi.w	#$8800,d2		; cmd EAx ????
	bne.s	.test17
	move.w	#$0EA0,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.w	.fini
.test17:
	cmpi.w	#$8C00,d2		; cmd EBx ????
	bne.s	.test18
	move.w	#$0EB0,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.s	.fini
.test18:
	cmpi.w	#$9400,d2		; cmd EDx ????
	bne.s	.test19
	move.w	#$0ED0,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.s	.fini
.test19:
	cmpi.w	#$9800,d2		; cmd EEx 
	bne.s	.nada
	move.w	#$0EE0,d2
	add.b	d3,d2			; rajoute la valeur !
	bra.s	.fini
.nada:
	move.w	#$0000,d2
.fini:
	add.w	d2,d7
	move.l	d7,(a1)+
	dbf	d0,.loop

Xann_Copy_Samples:
	move.l	a4,a0
	move.l	$20e(a0),d0
	add.l	d0,a0			; a0 = début des samples
	move.l	a0,a1
	add.l	SampLen,a1		; a1 = fin des samples

	moveq	#0,d0
	move.w	Nb_Patt,d0		; $18
	lsl	#8,d0
	lsl.l	#2,d0
	lea	$43c(a5),a2		; jump header
	add.l	d0,a2			; jump patterns
.loop:					; donc pointe là où mettre les samples
	cmp.l	a1,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.loop
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts



*-----------------------------------------------------------------------*
;
; SKYT Packer Check Routine

CheckSKYT:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	$100(a0),d0
	cmpi.l	#'SKYT',d0
	bne	.fuck

	moveq	#0,d6
	moveq	#30,d0
.loop:
	moveq	#0,d1
	move.w	(a0),d1
	bmi	.fuck
	add.l	d1,d6			; cumul lengths
	moveq	#0,d3
	move.w	2(a0),d3
	move.l	d3,d2
	andi.w	#$00FF,d3		; isole le volume
	cmpi.w	#$0040,d3		; qui ne doit pas depasser $40
	bhi	.fuck
	andi.w	#$FF00,d2		; isole le finetune
	cmpi.w	#$0F00,d2		; qui ne doit pas depasser $0F
	bhi	.fuck
	tst.w	d1
	beq.s	.next
	move.w	4(a0),d2
	add.w	6(a0),d2
	cmp.w	d1,d2
	bhi	.fuck
.next:
	lea	8(a0),a0
	dbf	d0,.loop

	lsl.l	#1,d6
	beq	.fuck
	move.l	d6,SampLen

.Check3:
	lea	$104(a4),a0
	moveq	#0,d0
	move.w	(a0)+,d0
	lsr.w	#8,d0
	move.l	d0,d1
	addq	#1,d1
	move.w	d1,Nb_Patt
	lsr	#1,d0
	subq	#1,d0
	lea	6(a0),a0
.loop2:
	move.l	(a0)+,d1
	andi.l	#$00FF00FF,d1
	bne	.fuck
	lea	8(a0),a0
	dbf	d0,.loop2

	adda.l	d6,a0			; fin du pack_mod
	movea.l	a4,a1
	add.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts



*-----------------------------------------------------------------------*
;
; SKYT-Packer to Protracker Convert routine, from Pro-Wizzy

ConvertSKYT:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	move.w	6(a1),d1
	bne.s	.no_need
	move.w	#$0001,6(a1)
.no_need:
	lea	30(a1),a1
	dbf	d0,.loop		; copy sample_data

	lea	$104(a4),a0
	lea	$3b6(a5),a1
	moveq	#0,d0
	move.w	(a0)+,d0		; take nb_pos
	lsr	#8,d0			; decale de 2 vers la droite
	addq	#1,d0
	move.l	d0,d1			; save d0
	move.l	a0,a2			; save a0
	move.b	d0,(a1)+		; write nb_pos in PTK_mod
	move.b	#$7F,(a1)+		; write cnste
	move.l	#'M.K.',$438(a5)
	lsl	#3,d0			; ensuite : mulu #8
	add.l	d0,a0			; pour pointer sur les notes packées

SKYT_PTK_Table:
	lea	$3b8(a5),a1
	moveq	#0,d0
	moveq	#0,d1
	move.w	Nb_Patt,d1
	subq	#1,d1
.loop:
	move.b	d0,(a1)+
	addq	#1,d0
	dbf	d1,.loop

SKYT_Convert:
	clr.l	PGP
	moveq	#0,d3
	move.l	a0,a3			; debut notes packées, ajouter PGP
	moveq	#0,d1
	move.w	Nb_Patt,d1
	subq	#1,d1			; pour le dbf
.Ploop:
	moveq	#3,d6			; 4 tracks
.Vloop:
	moveq	#0,d0
	move.w	(a2)+,d0		; take track_nbr (26 00)
	cmp.l	PGP,d0
	ble.s	.non
	move.l	d0,PGP
.non:
	tst.w	d0
	beq.s	.jp
	subi.w	#$0100,d0		; = 25 00
.jp:
	move.l	a3,a0
	add.l	d0,a0			; pointe sur la bonne track
	lea	$43c(a5),a1
	add.l	d3,a1			; et aussi dans les PTK_Tracks
	moveq	#63,d7			; 64 notes a copier
.Tloop:
	move.l	(a0)+,d2		; take longmot-note
	bsr	SKYT_Morph		; convert_it
	move.l	d2,(a1)			; write_it in PTK_Data
	lea	16(a1),a1
	dbf	d7,.Tloop
	addq.l	#4,d3			; voie suivante
	dbf	d6,.Vloop
	subi.l	#16,d3			; revient au debut
	addi.l	#$400,d3		; pattern suivant
	dbf	d1,.Ploop

SKYT_Copy_Sample:
	lea	$43c(a5),a1		; pointe sur les dest_patterns
	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl	#8,d1
	lsl.l	#2,d1			; mulu #$400
	add.l	d1,a1			; pointe sur les samples (dest)
	move.l	a1,a4

	move.l	a3,a0
	add.l	PGP,a0			; Pointe sur les sons (source)
	move.l	a0,a3
	add.l	SampLen,a3		; fin des sons
.copy:
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts

PGP:	dc.l	0

SKYT_Morph:
	movem.l	d3-d7,-(sp)
	move.l	d2,d3
	andi.l	#$FFFF0000,d3		; vire cmd
	swap	d3
	moveq	#0,d4
	moveq	#0,d5
	move.b	d3,d4			; take sample_byte
	cmpi.w	#$0010,d4		; superieur a $10 ?
	blt.s	.non
	subi.w	#$0010,d4		; enleve dizaine
	move.l	#$1000,d5		; prepare avec smpl>$10
.non:
	ror.w	#4,d4			; 0001 = 1000
	add.w	d4,d2			; mixe avec cmd

	lsr.w	#8,d3			; traite note
	tst.w	d3			; pas de note ?
	beq.s	.no_note
	subq	#1,d3
	lsl	#1,d3
	lea	mt_periodtable,a6
	add.w	(a6,d3.w),d5
.no_note:
	swap	d5
	add.w	d2,d5
	move.l	d5,d2
	movem.l	(sp)+,d3-d7
	rts




*-----------------------------------------------------------------------*
;
; ModProt Check Routine

CheckGV:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	cmp.l	#"TRK1",(a4)		;TRK1 at the first 4 bytes???
	bne	.cgv1
	add.l	#4,a4			;add 4 bytes
	add.l	#4,a0
	sub.l	#4,d4			;length -4
	sub.l	#4,d0
	;bra 	.cgv2

.cgv1:	lea	$f8(a0),a0
	bsr	Crypto_PP_Same
	tst.l	d7
	beq	.fuck			; si tjrs à 0 = bye bye

.cgv2:	lea	$f8(a4),a0		; pointe sur max_pos
	moveq	#0,d0
	move.b	(a0),d0
	move.w	d0,Nb_Pos
	addq	#2,a0			; pointe sur la patt_table
	bsr	Get_PGP
	cmpi.w	#$40,d1
	bhi	.fuck

	movea.l a4,a0			; begin
	moveq	#$1e,d0
.rescan:
	moveq	#0,d1
	move.w	(a0),d1			; retake length
	bne.s	.nop
	moveq	#0,d2
	move.w	6(a0),d2		; retake replen
	cmpi.w	#$0001,d2
	bne	.fuck
.nop:
	lea	8(a0),a0
	dbf	d0,.rescan

	lea	mt_periodtable,a2
	movea.l	a2,a1
	moveq	#0,d6			; compteur
	lea	$17a(a4),a0		; les notes
	moveq	#0,d3
	moveq	#0,d0
	move.w	Nb_Patt,d0
	subq	#1,d0			; test sur tous les patterns
.pattloop:
	moveq	#(16*4)-1,d1		; 16 premieres notes de chak voie
.noteloop:
	move.l	(a0)+,d2
	andi.l	#$0FFF0000,d2
	swap	d2
	beq.s	.back
	moveq	#35,d5
  .test:
	cmp.w	(a2)+,d2
	beq.s	.found
	dbf	d5,.test
	bra	.fuck 
  .found:
	addq.l	#1,d3
  .back:
	movea.l	a1,a2			; debut mt_periodtable
	dbf	d1,.noteloop		; on a deja avancé de $100
	lea	$300(a0),a0		; pattern suivant
	dbf	d0,.pattloop
	tst.l	d3
	beq	.fuck			; aucune note ? Pas_ModProt

	lea	$17a(a4),a0
	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl.l	#2,d1
	lsl.l	#8,d1
	add.l	d1,a0
	add.l	SampLen,a0		; fin du pack_mod
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmp.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts



*-----------------------------------------------------------------------*
;
; ModProt to Protracker Convert routine, from Pro-Wizzy

ConvertGV:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	cmp.l	#"TRK1",(a0)		; same as above
	bne	.congv1
	add.l	#4,a0
	
.congv1:
	lea	42(a1),a1
	moveq	#$1e,d0
.loop:
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop

	lea	-22(a1),a1		; $3b6
	move.w	(a0)+,(a1)+
	moveq	#$7f,d0
.looop:
	move.b	(a0)+,(a1)+
	dbf	d0,.looop
	addq	#4,a1			; saute M.K.

	move.l	(a0)+,d0
	beq.s	.en_17e
.en_17a:
	subq	#4,a0			; revient en $17a
.en_17e:
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#8,d0
	subq.l	#1,d0
.pattcopy:
	move.l	(a0)+,(a1)+
	dbf	d0,.pattcopy
	bsr	GMC_Copy_Samples

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts



GMC_Copy_Samples:
	movea.l	a1,a2
	movea.l	a0,a1
	add.l	SampLen,a1		; fin des sons
.copy:
	cmp.l	a1,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:
	rts


*-----------------------------------------------------------------------*
;
; GMC Check Routine

CheckGMC:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	$f0(a0),a0
	move.l	(a0),d5
	cmpi.l	#$64,d5			; pas + de 100 pos (GMC)
	bhi	.fuck
	movem.l	4(a0),d0-d3
	or.l	d3,d0
	or.l	d2,d0
	or.l	d1,d0
	andi.l	#$03FF03FF,d0
	bne	.fuck

	movea.l	a4,a0
	moveq	#$e,d0
	moveq	#0,d7
.loop:
	move.l	(a0),d1				; start_addy
	cmpi.l	#$F8000,d1
	bhi	.fuck
	moveq	#0,d2
	move.w	4(a0),d2			; length
	beq.s	.no_len
	bmi	.fuck
	tst.l	(a0)
	beq	.fuck
	add.l	d2,d7
.no_len:
	moveq	#0,d2
	move.w	6(a0),d2			; vol
	move.l	d2,d3
	andi.w	#$FF00,d2
	bne	.fuck
	andi.w	#$00FF,d3
	cmpi.w	#$0040,d3
	bhi	.fuck
	move.l	8(a0),d1			; repeat_addy
	beq.s	.no_rep
	cmpi.l	#$F8000,d1
	bhi	.fuck
	tst.l	(a0)
	beq	.fuck
	tst.w	4(a0)
	beq	.fuck
.no_rep:
	lea	$10(a0),a0
	dbf	d0,.loop

	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	lea	$f2(a4),a0
	moveq	#0,d0
	move.w	(a0)+,d0
	move.w	d0,Nb_Pos
	subq	#1,d0
	moveq	#0,d7
.pos:
	moveq	#0,d1
	move.w	(a0)+,d1
	move.l	d1,d2
	lsr.l	#2,d2
	lsr	#8,d2
	lsl.l	#2,d2
	lsl	#8,d2
	cmp.w	d1,d2
	bne	.fuck
	cmp.w	d7,d1
	ble.s	.next
	move.w	d1,d7
.next:
	dbf	d0,.pos
	lsr.l	#2,d7
	lsr	#8,d7
	addq	#1,d7
	move.w	d7,Nb_Patt

	lea	$1bc(a4),a0
	moveq	#127,d0
	add	d0,d0
	moveq	#0,d3
.notes:
	move.l	(a0)+,d1
	andi.l	#$0FFF0000,d1
	swap	d1
	beq.s	.dont
	cmpi.w	#$0FFE,d1
	beq.s	.dont
	lea	mt_periodtable,a1
	moveq	#35,d2
  .per:
	cmp.w	(a1)+,d1
	beq.s	.found
	dbf	d2,.per
	bra	.fuck
  .found:
	addq	#1,d3
.dont:
	dbf	d0,.notes

	tst.w	d3
	beq	.fuck

	lea	$1bc(a4),a0
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#8,d0
	lsl.l	#2,d0
	adda.l	d0,a0
	adda.l	SampLen,a0
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts



*-----------------------------------------------------------------------*
;
; GMC to Protracker Convert routine, from Pro-Wizzy

ConvertGMC:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	lea	42(a1),a1
	moveq	#$e,d0
.smp:
	move.l	(a0),d1
	move.l	4(a0),(a1)
	moveq	#0,d2
	move.w	$c(a0),d2
	beq.s	.no_repeat
	cmpi.w	#2,d2
	beq.s	.no_repeat
	move.l	8(a0),d3
	sub.l	(a0),d3
	lsr.l	#1,d3
	move.w	d3,4(a1)
	move.w	d2,6(a1)
	bra.s	.jp
.no_repeat:
	move.l	#1,4(a1)
.jp:
	lea	16(a0),a0
	lea	30(a1),a1
	dbf	d0,.smp

	moveq	#15,d0
.loop1:
	move.l	#$0,(a1)
	move.l	#$1,4(a1)		; fill a vide le reste des samples...
	lea	30(a1),a1
	dbf	d0,.loop1

	lea	$f4(a4),a0
	lea	$3b8(a5),a1
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
.patt:
	moveq	#0,d1
	move.w	(a0)+,d1
	bpl.s	.right
	subi.w	#1,Nb_Pos
	bra.s	.ret
.right:
	divu	#$400,d1
	move.b	d1,(a1)+
.ret:
	dbf	d0,.patt

	lea	$3b6(a5),a1
	moveq	#0,d0
	move.w	Nb_Pos,d0
	move.b	d0,(a1)+
	move.b	#$7F,(a1)

GMC_Patterns:
	lea	$1bc(a4),a0
	lea	$43c(a5),a1
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#8,d0
	subq	#1,d0
.notes:
	move.l	(a0)+,d1
	move.l	d1,d2
	andi.l	#$00000F00,d2
	bne.s	.test
	andi.w	#$FF00,d1
	bra.s	.pas6
.test:
	cmpi.w	#$800,d2
	bne.s	.pas8
	andi.w	#$F0FF,d1
	addi.w	#$0F00,d1
	bra.s	.pas6
.pas8:
	cmpi.w	#$300,d2
	bne.s	.pas3
	andi.w	#$F0FF,d1
	addi.w	#$0C00,d1
	bra.s	.pas6
.pas3:
	cmpi.w	#$600,d2
	bne.s	.pas6
	andi.w	#$F000,d1
.pas6:
	move.l	d1,d2
	andi.l	#$FFFF0000,d2
	swap	d2
	cmpi.w	#$FFFE,d2
	bne.s	.bye
	move.l	#$0C00,d1
	bra.s	.okay
.bye:
	cmpi.w	#$358,d2
	bls.s	.okay
	andi.l	#$0000FFFF,d1
	addi.l	#$03580000,d1
.okay:
	move.l	d1,(a1)+
	dbf	d0,.notes

	bsr	GMC_Copy_Samples

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts



*-----------------------------------------------------------------------*
;
; Avalon Packer Check Routine

CheckAvalon:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	$f6(a0),a0
	move.l	(a0),d3
	cmpi.w	#$200,d3		; pas + de $200 pos (/4)
	bhi	.fuck
	btst	#0,d3
	bne	.fuck
	btst	#1,d3			; doit etre divisible par 4
	bne	.fuck
	movem.l	4(a0),d0-d3
	or.l	d3,d0
	or.l	d2,d0
	or.l	d1,d0
	andi.l	#$FFFF03FF,d0
	bne	.fuck

	movea.l	a4,a0
	moveq	#$1e,d0
	moveq	#0,d6
.loop:
	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d7
	move.w	(a0)+,d1
	move.w	(a0)+,d2
	move.w	(a0)+,d3
	move.w	(a0)+,d7
	tst.w	d1
	beq.s	.no_len
	bmi	.fuck		; pas negatif ! ($8000+)
	add.w	d3,d7
	cmp.w	d1,d7
	bhi	.fuck
	add.l	d1,d6			; cumul lengths
	move.l	d2,d3
	andi.w	#$FF00,d2
	andi.w	#$00FF,d3
	cmpi.w	#$0F00,d2
	bhi	.fuck
	cmpi.w	#$0040,d3
	bhi	.fuck
	bra.s	.next
.no_len:
	add.l	d2,d1
	add.l	d3,d1
	add.l	d7,d1
	subq	#1,d1			; on vire le 1 de Replen et
	bne	.fuck		; tout le reste doit etre nul
.next:
	dbf	d0,.loop

	lsl.l	#1,d6
	beq	.fuck
	move.l	d6,SampLen

	lea	$f8(a4),a0		; pointe sur nb_pos
	moveq	#0,d0
	move.w	(a0)+,d0
	lsr	#2,d0
	move.w	d0,Nb_Pos
	moveq	#$7F,d0
	moveq	#0,d7
.pos:
	move.l	(a0)+,d1
	move.l	d1,d2
	lsr.l	#2,d2
	lsr	#8,d2
	lsl.l	#2,d2
	lsl	#8,d2
	cmp.w	d1,d2
	bne	.fuck
	cmp.w	d7,d1
	ble.s	.next1
	move.w	d1,d7
.next1:
	dbf	d0,.pos
	move.l	d7,d6
	lsr.l	#2,d7
	lsr	#8,d7
	addq	#1,d7
	move.w	d7,Nb_Patt
	cmpi.w	#$40,d7
	bhi	.fuck

	addi.l	#$0400,d6
	lea	$2fa(a4),a0
	move.l	(a0)+,d1
	beq	.fuck
	btst	#0,d1
	bne	.fuck
	btst	#1,d1
	bne	.fuck
;	cmp.l	d1,d6	; removed to make prowiz detect pm01.tg95 (mld)
;	bne	.fuck

	move.l	#(64*4)-1,d0
	move.l	#-$10,d6
.notes:
	move.l	(a0)+,d1
	eor.l	d6,d1			; remet au format PTK
	andi.l	#$0FFF0000,d1
	swap	d1
	beq.s	.dont
	cmpi.w	#$006C,d1
	blo	.fuck
	cmpi.w	#$038B,d1		; valeurs finetunées !!
	bhi	.fuck
.dont:
	dbf	d0,.notes

	lea	$2fe(a4),a0
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#8,d0
	lsl.l	#2,d0
	adda.l	d0,a0
	adda.l	SampLen,a0
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts



*-----------------------------------------------------------------------*
;
; Avalon Packer to Protracker Convert routine, from Pro-Wizzy

ConvertAvalon:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	lea	42(a1),a1
	moveq	#$1e,d0
.loop:
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop
	addq	#2,a0

	lea	-22(a1),a1		; $3b6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	move.b	d0,(a1)+
	move.b	#$7F,(a1)+
	moveq	#$7f,d0
.looop:
	move.l	(a0)+,d1
	lsr.l	#2,d1
	lsr.l	#8,d1
	move.b	d1,(a1)+
	dbf	d0,.looop
	addq	#4,a0			; saute $7800
	addq	#4,a1			; saute M.K.

	move.l	#-$10,d6
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#8,d0
	subq.l	#1,d0
.pattcopy:
	move.l	(a0)+,d1
	eor.l	d6,d1
	move.l	d1,(a1)+
	dbf	d0,.pattcopy

	bsr	GMC_Copy_Samples

	bsr	PM1_Adjust_Finetune	; Remet les bonnes valeurs de notes !!

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts



*-----------------------------------------------------------------------*
;
; AC1D Packer Check Routine

CheckAC1D:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	(a0),d3
	move.l	d3,d1
	andi.l	#$0000FFFF,d1
	cmpi.l	#$0000AC1D,d1		; Un AC1D ?
	beq.s	.Test_AC1D
	cmpi.l	#$0000D1CA,d1		; Un AC1D ?
	bne	.fuck

.Test_AC1D:
	movea.l	a4,a0

	moveq	#0,d0
	move.b	(a0),d0			; get nb_pos
	beq	.fuck
	bmi	.fuck
	move.l	4(a0),d0		; get offset_smpl
	beq	.fuck
	btst	#0,d0
	bne	.fuck

	lea	8(a0),a0
	moveq	#0,d0
	moveq	#$40,d6
	move.l	#$0F00,d7
	moveq	#30,d1
.loop:
	moveq	#0,d3
	move.w	(a0),d3			; take length
	bmi	.fuck
	add.l	d3,d0
	moveq	#0,d2
	move.w	2(a0),d2		; volume
	move.l	d2,d5
	andi.w	#$00FF,d5
	cmp.w	d6,d5			; si vol>$40 : Pas_AC1D
	bhi	.fuck
	andi.w	#$FF00,d2
	cmp.w	d7,d2			; si fineT>$0F : Pas_AC1D
	bhi	.fuck
	lea	8(a0),a0
	dbf	d1,.loop

	lsl.l	#1,d0
	beq	.fuck
	move.l	d0,SampLen

	movea.l	a4,a0
	adda.l	4(a0),a0
	adda.l	d0,a0
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

	lea	$300(a4),a0
	moveq	#0,d2
	moveq	#127,d0
.loop2:
	moveq	#0,d1
	move.b	(a0)+,d1
	cmp.b	d2,d1
	blt	.next
	move.b	d1,d2			; si new>old, echange !
.next:
	dbf	d0,.loop2
	addq	#1,d2			; nb patt differents
	cmpi.w	#$40,d2
	bhi	.fuck
	move.w	d2,Nb_Patt

; Calc PTK Module Size

	lsl.l	#2,d2
	lsl.l	#8,d2
	add.l	SampLen,d2
	addi.l	#$43c,d2		; size for dtg_AllocListData
	move.l	d2,d0
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


AC1D_Offset:	dc.l	0

*-----------------------------------------------------------------------*
;
; AC1D Packer to Protracker Convert routine, from Pro-Wizzy

ConvertAC1D:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	moveq	#0,d0
	move.b	(a0),d0
	move.w	d0,Nb_Pos
	lea	8(a0),a0

AC1D_Make_Header:
	lea	42(a1),a1
	moveq	#30,d0
.loop:
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop

	lea	$300(a4),a0
	lea	$3b6(a5),a1
	move.w	Nb_Pos,d0
	move.b	d0,(a1)+
	move.b	#$7f,(a1)+
	subq	#1,d0
.Ploop:
	move.b	(a0)+,(a1)+
	dbf	d0,.Ploop

AC1D_Do_It:
	lea	$100(a4),a0
	lea	$43c(a5),a1

.choppe_L_offset:
	lea	$100(a4),a2
	move.l	(a2),AC1D_Offset	; $996A (-$996A + $380 !)

	moveq	#0,d0
	move.w	Nb_Patt,d0
	subq	#1,d0
.Ploop:
	movea.l	a4,a2
	add.l	(a0)+,a2		; pointe sur le patt 0
	sub.l	AC1D_Offset(pc),a2	; 996A - 996A
	adda.l	#$380,a2		; et + $380 !
	lea	$c(a2),a2

	moveq	#3,d1
.Vloop:
	moveq	#63,d2
.loop:
	moveq	#0,d3
	move.b	(a2),d3
	cmpi.w	#$81,d3			; teste si $81 ou +    : saut_lignes
	blt.s	.pas_saut
	addq	#1,a2
	subi.w	#$81,d3			; il reste le nb de lignes a sauter
.sloop:
	clr.l	(a1)
	lea	16(a1),a1
	subq	#1,d2			; -1 au nb de notes total.
	dbf	d3,.sloop
	tst.b	d2
	bge.s	.loop
	bra.w	.voie_finie
.pas_saut:
	cmpi.w	#$7F,d3			; no note, only sample ! (= AA0F)
	beq.s	.special_7F
	cmpi.w	#$4C,d3			; teste si note + sample $10+
	bge.s	.decoupe
	cmpi.w	#$3F,d3			; special : no note, no sample !
	blt.s	.normal_note
.special_3F:
	clr.w	(a1)
	move.b	1(a2),2(a1)
	move.b	2(a2),3(a1)
	addq	#3,a2
	lea	16(a1),a1
	bra.w	.next
.special_7F:
	move.w	#$1000,(a1)
	move.b	1(a2),2(a1)
	move.b	2(a2),3(a1)
	addq	#3,a2
	lea	16(a1),a1
	bra.s	.next
.normal_note:
	subi.w	#$0C,d3
	lsl	#1,d3
	moveq	#0,d7
	lea	mt_periodtable,a6
	move.w	(a6,d3.w),d3		; get real note
	swap	d3
	add.l	d3,d7			; octet fort de la note DONE.
	bra.s	.Octet2
.decoupe:
	subi.w	#$4C,d3			; ex: $64 --> $18 (x2 = $30)
	lsl	#1,d3			; mulu #2
	move.l	#$10000000,d7		; prepare note finale avec smpl $10+
	lea	mt_periodtable,a6
	move.w	(a6,d3.w),d3		; get real note
	swap	d3
	add.l	d3,d7			; octet fort de la note DONE.
.Octet2:
	addq	#1,a2
	moveq	#0,d3
	move.b	(a2),d3
	move.l	d3,d4
	andi.w	#$000F,d4		; garde que la commande, si 7 : break
	cmpi.w	#$07,d4
	beq.s	.pas_cmd
	lsl	#8,d3			; decale pour accueillir la valeur
	move.b	1(a2),d3		; take valeur
	add.l	d3,d7
	move.l	d7,(a1)			; real note !!
	lea	16(a1),a1
	addq	#2,a2
	bra.s	.next
.pas_cmd:
	andi.w	#$00F0,d3		; garde que le sample
	lsl	#8,d3			; 00C0 = C000
	add.l	d3,d7
	move.l	d7,(a1)			; real note !!
	lea	16(a1),a1
	addq	#1,a2
.next:
	dbf	d2,.loop
.voie_finie:
	suba.l	#64*16,a1
	addq	#4,a1
	dbf	d1,.Vloop

	lea	-16(a1),a1
	adda.l	#$400,a1
	dbf	d0,.Ploop

AC1D_Copy_Sample:
	lea	$43c(a5),a1		; pointe sur les dest_patterns
	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl	#8,d1
	lsl.l	#2,d1			; mulu #$400
	add.l	d1,a1			; pointe sur les samples (dest)

	movea.l	a4,a0
	add.l	4(a0),a0		; Pointe sur les sons (source)
	move.l	a0,a3
	add.l	SampLen,a3		; fin des sons
.copy:					; donc pointe là où mettre les samples
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:
	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts




*-----------------------------------------------------------------------*
;
; Pygmy Packer Check Routine

CheckPygmy:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	$2a0(a0),d3
	cmpi.l	#$00780071,d3		; Un Pygmy ? (en $2A0)

	movea.l	a4,a1
	moveq	#29,d0
	move.l	(a0),d3			; take 1ere start adr
.up:
	move.l	(a0),d1			; take 1ere start adr
	beq	.fuck
	move.l	$10(a0),d2		; take 2eme start adr (smpl+1)
	cmp.l	d1,d2
	blo	.fuck			; si 2eme < 1ere : FUUCCCCKKKK!
	lea	$10(a0),a0
	dbf	d0,.up
	cmp.l	d3,d2
	beq	.fuck

	movea.l	a4,a0
	moveq	#0,d0
	moveq	#30,d1
.loop:
	moveq	#0,d3
	move.w	4(a0),d3		; take length
	beq.s	.next
	bmi	.fuck
	add.l	d3,d0
	move.l	6(a0),d2
	sub.l	(a0),d2
	lsr.l	#1,d2
	add.w	10(a0),d2
	cmp.l	d3,d2
	bhi	.fuck
.next:
	moveq	#0,d2
	move.w	12(a0),d2		; volume
	move.l	d2,d5
	andi.w	#$00FF,d5
	cmpi.w	#$0040,d5		; si vol>$40 : Pas_Pygmy
	bhi	.fuck
	andi.w	#$FF00,d2
	cmpi.w	#$0F00,d2		; si fineT>$0F : Pas_Pygmy
	bhi	.fuck
	lea	$10(a0),a0
	dbf	d1,.loop

	lsl.l	#1,d0
	beq	.fuck
	move.l	d0,SampLen
	move.l	d0,d6

	move.l	$1f0(a4),d1		; 26192
	sub.l	d0,d1			; doit etre egal a SampLen
	bne	.fuck			; si diff <> 0, Pas_Pygmy !

	lea	$25c(a4),a0		; pointe sur period_table
	lea	mt_periodtable,a1
	moveq	#35,d0
.loop2:
	moveq	#0,d1
	move.w	(a0)+,d1
	cmp.w	(a1)+,d1
	bne	.fuck
	dbf	d0,.loop2

	movea.l	a4,a0
	move.l	$59c(a0),d0		; $12138
	move.l	(a0),d1			; $1c5b8 (adr du 1er sample)
	sub.l	d0,d1			; la diff divisée par $400 = nb_patt
	beq	.fuck
	btst	#0,d1
	bne	.fuck
	lsr.l	#8,d1			; /$100
	lsr.l	#2,d1			; /$4

	cmpi.w	#$40,d1
	bhi	.fuck
	move.w	d1,Nb_Patt

	lea	$636(a4),a0
	lsl.l	#8,d1
	lsl.l	#2,d1
	adda.l	d1,a0
	adda.l	d6,a0
	adda.l	#$80,a0			; table des patterns a la fin !
	move.l	a4,d1
	add.l	d4,d1
	sub.l	a0,d1
	blo	.fuck		; pas plus petit DU TOUT !!

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts



*-----------------------------------------------------------------------*
;
; Pygmy Packer to Protracker Convert routine, from Pro-Wizzy

ConvertPygmy:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	moveq	#0,d1			; compteur a oter de d0.loop apres
.testing:
	moveq	#0,d0
	move.w	4(a0),d0		; take length
	tst.w	d0			; nulle ?? pas de son ?
	bne.s	.ouf
	addq	#1,d1
	lea	$10(a0),a0
	bra.s	.testing
.ouf:
	lea	42(a1),a1
	moveq	#30,d0
	sub.w	d1,d0
.loop:
	move.w	4(a0),(a1)		; length
	move.w	12(a0),2(a1)		; volume
	move.w	10(a0),6(a1)		; replen
	move.l	6(a0),d1		; take repeat_addr
	sub.l	0(a0),d1		; sub start_addr
	lsr.l	#1,d1			; divise par 2
	move.w	d1,4(a1)		; repeat
	lea	16(a0),a0
	lea	30(a1),a1
	dbf	d0,.loop

	lea	$3b8(a5),a1

	lea	$636(a4),a0		; debut des patterns
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.w	#2,d0
	lsl.l	#8,d0			; * $400
	add.l	d0,a0			; debut des samples
	add.l	SampLen,a0		; fin des samples, debut patt_table
	moveq	#0,d1			; compteur
.loop1:
	moveq	#0,d0
	move.b	(a0)+,d0
	cmpi.w	#$00FF,d0		; fin de la table ?
	beq.s	.sortdela
	move.b	d0,(a1)+
	addq	#1,d1
	bra.s	.loop1
.sortdela:
	lea	$3b6(a5),a1
	move.b	d1,(a1)			; $3D

Pygmy_Convert_Patterns:
	lea	$636(a4),a0
	lea	$43c(a5),a1
	adda.l	#$400-$10,a1		; derniere ligne du 1er pattern
	move.w	Nb_Patt,d6		; coz on remplit en remontant !
	subq	#1,d6			; nb_patt-1
.Ploop:
	moveq	#3,d0
.loop:
	moveq	#63,d7
.Vloop:
	move.l	(a0)+,d1

	move.l	d1,d2
	andi.l	#$0000FF00,d2		; isole cmd
	lsr.w	#8,d2			; FF00 = 00FF

	bsr	Pygmy_Test_cmd

	move.l	d1,d3
	andi.l	#$00FF0000,d3		; isole smpl
	swap	d3
	tst.b	d3
	beq.s	.rien
	lsr.w	#3,d3
	cmp.w	#$10,d3
	blt.s	.rien
	add.l	#$10000000,d2
	sub.w	#$10,d3
.rien:
	ror.w	#4,d3
	add.l	d3,d2

	move.l	d1,d3
	andi.l	#$FF000000,d3		; isole note
	rol.l	#8,d3
	tst.b	d3
	beq.s	.no_note
	cmpi.w	#$b8,d3			; note avec sample = 0
	blt.s	.normal
	neg.b	d3
.normal:
	subq	#2,d3
	lea	mt_periodtable,a6
	move.w	(a6,d3.w),d3
	swap	d3
.no_note:
	add.l	d3,d2			; note finale !
	move.l	d2,(a1)
	lea	-$10(a1),a1
	dbf	d7,.Vloop
	adda.l	#64*16,a1
	addq	#4,a1
	dbf	d0,.loop
	adda.l	#$400-$10,a1		; (fin du) pattern suivant
	dbf	d6,.Ploop

Pygmy_Copy_Samples:
	movea.l	a5,a1
	adda.l	#$43c,a1		; pointe sur les dest_patterns
	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl	#8,d1
	lsl.l	#2,d1			; mulu #$400
	add.l	d1,a1			; pointe sur les samples (dest)

	lea	$636(a4),a0
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.w	#2,d0
	lsl.l	#8,d0			; * $400
	add.l	d0,a0			; Pointe sur les sons (source)
	move.l	a0,a3
	add.l	SampLen,a3		; fin des sons
.copy:
	cmp.l	a3,a0
	bge.s	.fini
	move.l	(a0)+,(a1)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts



Pygmy_Test_cmd:
	tst.b	d2			; cmd 000 et 0xx
	beq.s	.cmd0
	cmp.w	#$0010,d2
	bne.s	.test2
.cmd0:
	move.b	#00,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test2:
	cmp.w	#$0014,d2		; cmd 1xx
	bne.s	.test3
	move.b	#$01,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test3:
	cmp.w	#$0018,d2		; cmd 2xx
	bne.s	.test4
	move.b	#$02,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test4:
	cmp.w	#$0004,d2		; cmd 3xx
	bne.s	.test5
	move.b	#$03,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test5:
	cmp.w	#$0024,d2		; cmd 4xx
	bne.s	.test6
	move.b	#$04,d2
	lsl.w	#8,d2

	moveq	#0,d3			; inversion de l'octet-valeur
	moveq	#0,d4
	move.b	d1,d3
	move.b	d1,d4
	andi.w	#$000F,d3		; garde l'unite
	andi.w	#$00F0,d4		; garde la dizaine
	lsl.w	#4,d3
	lsr.w	#4,d4
	add.w	d3,d4
	move.b	d4,d2
	rts
.test6:
	cmp.w	#$000C,d2		; cmd 5xx
	bne.s	.test7
	move.b	#$05,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test7:
	cmp.w	#$002C,d2		; cmd 6xx
	bne.s	.test8
	move.b	#$06,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test8:
	cmp.w	#$001C,d2		; cmd Ax0 (volslide Up)
	bne.s	.test8b
	move.b	#$0A,d2
	lsl.w	#8,d2
	lsl.w	#4,d1			; decale valeur
	add.b	d1,d2			; ajoute valeur
	rts
.test8b:
	cmp.w	#$0020,d2		; cmd A0x (volslide Down)
	bne.s	.test9
	move.b	#$0A,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test9:
	cmp.w	#$0034,d2		; cmd Cxx
	bne.s	.test10
	move.b	#$0C,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test10:
	cmp.w	#$0038,d2		; cmd Dxx
	bne.s	.test11
	move.b	#$0D,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test11:
	cmp.w	#$003C,d2		; cmd Exx
	bne.s	.test12
	move.b	#$0E,d2
	lsl.w	#8,d2
	moveq	#0,d3
	move.b	d1,d3			; ajoute la valeur de la cmd
	cmpi.w	#$0002,d3
	bne.s	.bye
	move.b	#$01,d2			; force le filtre OFF
	rts
.bye:
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test12:
	cmp.w	#$0040,d2		; cmd Fxx
	bne.s	.test13
	move.b	#$0F,d2
	lsl.w	#8,d2
	add.b	d1,d2			; ajoute la valeur de la cmd
	rts
.test13:
	rts


Channel:	dc.l	0

*-----------------------------------------------------------------------*
;
; Channel Player Check Routine

CheckChan:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	(a0),d3
	move.l	d3,d1
	swap	d1
	move.l	d1,d2
	andi.w	#$F00F,d1		; Garde ce qui doit etre 0 et A
	cmpi.w	#$000A,d1		; Un Channel Player 1, 2 ou 3 ?
	bne	.fuck
	andi.w	#$0FF0,d2
	beq	.fuck
	cmpi.w	#$01F0,d2
	bhi	.fuck

	movea.l	a4,a0
	moveq	#0,d0
	move.w	(a0),d0
	lsr	#4,d0			; $01F0 = 001F
	subq	#1,d0
	addq	#6,a0			; pointe sur sample_length.l
	move.l	(a0)+,d3		; take it
	addq	#4,a0			; pointe sur length chak sample
	moveq	#$40,d6
	moveq	#$48,d7
	moveq	#0,d1
.loop:
	moveq	#0,d2
	move.w	(a0),d2			; take length
	bmi	.fuck
	add.l	d2,d1			; cumule lengths
	tst.w	d2
	bne.s	.other
	move.l	-4(a0),d5
	sub.l	4(a0),d5
	bne	.fuck
.other:
	move.l	-4(a0),d5
	cmpi.l	#$FE000,d5
	bhi	.fuck
	move.l	4(a0),d5
	cmpi.l	#$FE000,d5
	bhi	.fuck
	moveq	#0,d5
	move.w	2(a0),d5		; vol
	cmp.w	d6,d5
	bhi	.fuck
	move.w	10(a0),d5		; finetune *$48
	divu	d7,d5
	mulu	d7,d5
	cmp.w	10(a0),d5
	bne	.fuck
	lea	$10(a0),a0		; next sample
	dbf	d0,.loop

	lsl.l	#1,d1
	beq	.fuck			; longueur totale NULLE ?

	sub.l	d1,d3			; différence des 2 longueurs totales
	bne	.fuck			; doit etre nulle, par contre !
	move.l	d1,SampLen
	move.l	d1,d6

	movea.l	a4,a0
	moveq	#0,d0
	moveq	#0,d1
	moveq	#0,d2
	move.w	(a0),d0			; 00DA
	move.w	2(a0),d1		; 004C
	beq	.fuck
	move.w	4(a0),d2		; 2100
	beq	.fuck
	adda.l	d0,a0
	adda.l	d1,a0			; pointe sur les notes
	adda.l	d2,a0			; pointe sur les sons
	adda.l	d6,a0			; fin des sons
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

.Channel1_Note_Test:
	movea.l	a4,a0
	moveq	#0,d0
	moveq	#0,d1
	moveq	#0,d2
	move.w	(a0),d0			; 00DA
	move.w	2(a0),d1		; 004C
	move.w	4(a0),d2		; 2100
	adda.l	d0,a0
	adda.l	d1,a0			; pointe sur les notes !
	movea.l	a0,a1
	movea.l	a0,a2			; save pour test Channel 2
	adda.l	d2,a1			; pointe sur les sons, fin des notes !
	subq	#4,a1			; !! PRECAUTION !!
	moveq	#0,d6			; compteur !
.MegaLoop:
	cmpa.l	a1,a0
	bge.s	.StopIt
	moveq	#0,d0
	move.b	(a0)+,d0		; take byte
	cmpi.w	#$80,d0
	bne.s	.MegaLoop
	move.b	(a0)+,d0		; take 2eme byte
	cmpi.w	#$0F,d0			; ne doit pas depasser F (ya F cmds)
	bhi.s	.MegaLoop		; dommage
	addq	#1,d6			; OUI ! Un de trouvé...
	bra.s	.MegaLoop
.StopIt:
	tst.l	d6			; nb de $800x ???
	beq.s	.OkCh1			; yen a pas, c un Channel 1, GO !
	move.l	#'Cha2',Channel		; sinon c un Channel2 ou 3
	bra	.Test_Channel2
.OkCh1:
	move.l	#'Cha1',Channel
	bsr	Channel_GetPGP
	bsr	Chan1_Speco
	bra	.End_Test_Channel

.Test_Channel2:
	move.l	a2,a0
	moveq	#0,d6			; compteur !
.MegaLoop2:
	cmp.l	a1,a0
	bge.s	.StopIt2
	moveq	#0,d0
	move.l	(a0)+,d0		; take byte
	cmpi.l	#$80808080,d0		; Quatre $8080 qui se suivent ?
	bne.s	.MegaLoop2
	addq	#1,d6			; OUI ! Un de trouvé...
	bra.s	.MegaLoop2
.StopIt2:
	tst.l	d6			; nb de $8080 ???
	beq.s	.okay_c2		; yen a pas, c un Channel 2, GO !
	move.l	#'Cha3',Channel		; sinon c un Channel3
.okay_c2:
	bsr	Channel_GetPGP
	bsr	Chan2_Speco

.End_Test_Channel:

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts




Channel_GetPGP:
	movea.l a4,a0
	moveq	#0,d0
	move.w	(a0),d0
	andi.w	#$FFF0,d0
	lsr	#4,d0
	move.l	d0,d7			; stock sample_nb
	subq	#1,d0
	lea	$A(a0),a0
.loop:
	move.l	(a0)+,d1		; addr_start
	move.w	(a0)+,d2		; move length
	move.w	(a0)+,d1		; move volume
	move.l	(a0)+,d2		; addr_repeat
	move.w	(a0)+,d1		; move replen
	move.w	(a0)+,d2		; take finetune (multiple de $48)
	dbf	d0,.loop
	move.l	a0,Channel_Track_Table
	rts

Chan1_Speco:
	movea.l	a4,a0
	moveq	#0,d0
	move.w	2(a0),d0		; $4C
	lsr	#2,d0			; div /4
	move.w	d0,Nb_Pos

	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; 1er patt = 0 d'office !
	move.l	Channel_Track_Table,a0	; table d'origine
	addq	#4,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Channel_Track_Table,a1	; table de comparaison
	lea	KR_PosTable,a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,PM40_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	PM40_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	rts

Chan2_Speco:
	movea.l	a4,a0
	moveq	#0,d0
	move.w	2(a0),d0		; $4C
	lsr	#3,d0			; div /8
	move.w	d0,Nb_Pos

	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; 1er patt = 0 d'office !
	move.l	Channel_Track_Table,a0
	lea	8(a0),a0		; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend les 2 longwords de chak patt
	move.l	(a0)+,d2
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Channel_Track_Table,a1	; table de comparaison
	lea	KR_PosTable,a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,PM40_BytePos
	move.l	(a1)+,d3
	move.l	(a1)+,d4
	sub.l	d1,d3
	bne.s	.suivant
	sub.l	d2,d4
	bne.s	.suivant
  .le_meme:
	move.b	PM40_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	rts



*-----------------------------------------------------------------------*
;
; Channel Player to Protracker Convert routine, from Pro-Wizzy

ConvertChan:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	cmp.l	#'Cha1',Channel
	beq.s	Channel1_Converter
	cmp.l	#'Cha2',Channel
	beq	Channel2_Converter
	bra	Channel3_Converter

Channel1_Converter:
	bsr	Channel_Entete

	movea.l	a4,a0
	moveq	#0,d0
	move.w	2(a0),d0		; $4C
	lsr	#2,d0			; div /4
	move.w	d0,Nb_Pos

Channel1_Make_PTK_Table:
	lea	$3b9(a5),a2		; 1er patt = 0 d'office !
	move.l	Channel_Track_Table,a0	; table d'origine
	addq	#4,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Channel_Track_Table,a1	; table de comparaison
	lea	$3b8(a5),a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,PM40_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	PM40_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt

	movea.l	a5,a1
	move.w	Nb_Pos,d3
	move.b	d3,$3b6(a1)

Channel_Convert_Patterns:
	movea.l	a4,a1
	move.w	(a1),d0
	move.w	2(a1),d1
	add.w	d0,a1
	add.w	d1,a1			; debut des notes !
	move.l	a1,Chan_a4		; SAVE IT

	move.l	Channel_Track_Table,a0	; table de comparaison

	lea	$3b8(a5),a3		; patterns numbers
	lea	$43c(a5),a2		; patterns destination

	moveq	#-1,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
.loop:
	moveq	#0,d7
	move.b	(a3)+,d7
	cmp.w	d6,d7
	bgt.s	.okaayyy
	addq	#4,a0
	bra.w	.fin_patt
.okaayyy:
	addq.l	#1,d6
	move.l	(a0)+,d3		; take 1er pattern dans l'ordre
	beq.w	.finito
	move.l	d3,d4			; 00 01 02 03
	moveq	#3,d1			; convertir 4 voies
.Vloop:
	rol.l	#8,d4			; 01 02 03 00
	moveq	#0,d5
	move.b	d4,d5
	mulu	#$c0,d5
	add.l	d5,a1			; pointe sur les notes de la bonne trk
	moveq	#63,d2
.Nloop:
	moveq	#0,d3
	move.b	(a1)+,d3
	rol	#8,d3
	move.b	(a1)+,d3
	rol.l	#8,d3
	move.b	(a1)+,d3		; 3bytes-note. (32BAF1)
	move.w	d3,d5			; traitement cmd
	andi.w	#$0F00,d5
	cmpi.w	#$0D00,d5
	beq.s	.Convert_deci
	cmpi.w	#$0A00,d5
	beq.s	.Neg_or_decale
	cmpi.w	#$0500,d5
	beq.s	.Neg_or_decale
	cmpi.w	#$0600,d5
	beq.s	.Neg_or_decale
	bra.s	.ok_cmd
.Convert_deci:
	lea	Deci,a6
	moveq	#0,d5
	move.w	d3,d5
	andi.w	#$00FF,d5
	move.b	(a6,d5),d3
	bra.s	.ok_cmd
.Neg_or_decale:
	move.w	d3,d5			; reprend le mot
	andi.w	#$00FF,d5
	cmpi.w	#$F1,d5			; entre F1 et FF, faire un NEG
	bge.s	.Neg_It
	move.w	d3,d5
	andi.w	#$000F,d5
	lsl	#4,d5
	move.b	d5,d3
	bra.s	.ok_cmd
.Neg_It:
	neg.b	d3
.ok_cmd:
	moveq	#0,d5
	move.w	d3,2(a2)		; move 2eme word dans PTK_Pattern
	swap	d3
	tst.w	d3
	beq.s	.cool
	move.w	d3,d5
	lsr	#1,d5
	lsl	#1,d5
	cmp.w	d5,d3
	beq.s	.ok_deja_pair
	subq	#1,d3
	move.w	#$1000,d5		; prepare pour sample > $10
	bra.s	.prout
.ok_deja_pair:
	moveq	#0,d5
.prout:
	subq	#2,d3
	lea	mt_periodtable,a6
	move.w	(a6,d3.w),d3		; Get real note !
.cool:
	add.w	d5,d3
	moveq	#0,d5
	move.w	d3,(a2)			; move 1er word dans PTK_Pattern
	lea	$10(a2),a2		; note suivante
	dbf	d2,.Nloop
	suba.l	#16*64,a2		; remonte en haut de la voie
	addq	#4,a2			; voie suivante !
	move.l	Chan_a4,a1		; RESTORE IT
	dbf	d1,.Vloop
	suba.l	#16,a2
	adda.l	#$400,a2		; Pattern suivant
.fin_patt:
	dbf	d0,.loop
.finito:

Channel_Copy_Samples:
	lea	$43c(a5),a2
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#8,d0
	lsl.l	#2,d0
	add.l	d0,a2			; debut samples (dest)

	movea.l	a4,a0
	move.l	a0,a1
	add.w	(a1),a0
	add.w	2(a1),a0
	add.w	4(a1),a0		; debut samples (source)
	move.l	a0,a1
	add.l	SampLen,a1		; fin des sons
.copy:
	cmp.l	a1,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts

Chan_a4:		dc.l	0
Channel_Track_Table:	dc.l	0


; ------------------------- Channel Player 2 ------------------------

Channel2_Converter:
	bsr	Channel_Entete

	movea.l a4,a0
	moveq	#0,d0
	move.w	2(a0),d0		; $4C
	lsr	#3,d0			; div /8
	move.w	d0,Nb_Pos

	bsr	Channel2_Make_PTK_Table

	lea	$3b6(a5),a2
	moveq	#0,d0
	move.w	Nb_Pos,d0
	move.b	d0,(a2)+
	move.b	#$7F,(a2)+

Channel2_Convert_Patterns:
	movea.l	a4,a1
	moveq	#-1,d6
	moveq	#0,d0
	moveq	#0,d1
	move.w	(a1),d0
	move.w	2(a1),d1
	add.l	d0,a1
	add.l	d1,a1			; debut des notes !
	move.l	a1,Chan_a4		; SAVE IT

	move.l	Channel_Track_Table,a0	; table d'origine

	lea	$3b8(a5),a3		; patterns
	lea	$43c(a5),a2		; patterns destination

	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
.loop:
	moveq	#0,d7
	move.b	(a3)+,d7
	cmp.w	d6,d7
	bgt.s	.okaayyy
	lea	8(a0),a0
	bra.w	.fin_patt
.okaayyy:
	addq.l	#1,d6
	moveq	#3,d1
.Vloop:
	moveq	#0,d3
	move.w	(a0)+,d3		; take 1ere voie
	add.l	d3,a1			; pointe sur les notes de la bonne trk
	moveq	#63,d2
.Nloop:
	moveq	#0,d3
	move.b	(a1)+,d3
	cmpi.w	#$80,d3			; $80 + 0x = no note, mais cmd x
	beq.w	.No_Note
	bgt.w	.Note_2bytes
.Note_3bytes:
	moveq	#0,d5
	tst.w	d3
	beq.s	.Yipee
	move.w	d3,d5
	lsr	#1,d5
	lsl	#1,d5
	cmp.w	d5,d3
	beq.s	.ok_deja_pair
	subq	#1,d3
	move.w	#$1000,d5		; prepare pour sample > $10
	bra.s	.prout
.ok_deja_pair:
	moveq	#0,d5
.prout:
	subq	#2,d3
	lea	mt_periodtable,a6
	move.w	(a6,d3.w),d3		; Get real note !
.Yipee:
	add.w	d5,d3
	moveq	#0,d5
	move.w	d3,(a2)			; move 1er word dans PTK_Pattern
	moveq	#0,d3
	move.b	(a1)+,d3		; 6C
	lsl	#8,d3
	move.b	(a1)+,d3		; 20    = 6C20
	move.w	d3,d5			; traitement cmd
	andi.w	#$0F00,d5
	cmpi.w	#$0D00,d5
	beq.s	.Convert_deci
	cmpi.w	#$0A00,d5
	beq.s	.Neg_or_decale
	cmpi.w	#$0500,d5
	beq.s	.Neg_or_decale
	cmpi.w	#$0600,d5
	beq.s	.Neg_or_decale
	bra.s	.ok_cmd
.Convert_deci:
	lea	Deci,a6
	moveq	#0,d5
	move.w	d3,d5
	andi.w	#$00FF,d5
	move.b	(a6,d5),d3
	bra.s	.ok_cmd
.Neg_or_decale:
	move.w	d3,d5			; reprend le mot
	andi.w	#$00FF,d5
	cmpi.w	#$F1,d5			; entre F1 et FF, faire un NEG
	bge.s	.Neg_It
	move.w	d3,d5
	andi.w	#$000F,d5
	lsl	#4,d5
	move.b	d5,d3
	bra.s	.ok_cmd
.Neg_It:
	neg.b	d3
.ok_cmd:
	move.w	d3,2(a2)		; move 2eme word dans PTK_Pattern
	bra.s	.Next_Note
.Note_2bytes:
	moveq	#0,d5
	subi.w	#$0080,d3		; $C4 = $44
	move.w	d3,d5
	lsr	#1,d5
	lsl	#1,d5
	cmp.w	d5,d3
	beq.s	.ok_deja_pair2
	subq	#1,d3
	move.w	#$1000,d5		; prepare pour sample > $10
	bra.s	.prout2
.ok_deja_pair2:
	moveq	#0,d5
.prout2:
	subq	#2,d3
	lea	mt_periodtable,a6
	move.w	(a6,d3.w),d3		; Get real note !
	add.w	d5,d3
	moveq	#0,d5
	move.w	d3,(a2)			; move 1er word dans PTK_Pattern
	moveq	#0,d3
	move.b	(a1)+,d3		; take 2eme byte (sample)
	lsl.w	#8,d3
	move.w	d3,2(a2)		; 2eme word
	bra.s	.Next_Note
.No_Note:
	clr.w	(a2)			; 1er word
	move.b	(a1)+,d3		; take 2eme byte
	andi.w	#$000F,d3		; garde ke la cmd
	lsl.w	#8,d3			; 0004 = 0400
	move.w	d3,2(a2)		; 2eme word
.Next_Note:
	lea	$10(a2),a2		; note suivante
	dbf	d2,.Nloop
	suba.l	#16*64,a2		; remonte en haut de la voie
	addq	#4,a2			; voie suivante !
	move.l	Chan_a4,a1		; RESTORE IT
	dbf	d1,.Vloop
	suba.l	#16,a2
	adda.l	#$400,a2		; Pattern suivant
.fin_patt:
	dbf	d0,.loop

Channel2_Copy_Samples:
	lea	$43c(a5),a2
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#8,d0
	lsl.l	#2,d0
	add.l	d0,a2			; debut samples (dest)

	movea.l	a4,a0
	move.l	a0,a1
	add.w	(a1),a0
	add.w	2(a1),a0
	add.w	4(a1),a0		; debut samples (source)
	move.l	a0,a1
	add.l	SampLen,a1		; fin des sons
.copy:
	cmp.l	a1,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:
	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts


Channel2_Make_PTK_Table:
	lea	$3b9(a5),a2		; 1er patt = 0 d'office !
	move.l	Channel_Track_Table,a0
	lea	8(a0),a0		; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend les 2 longwords de chak patt
	move.l	(a0)+,d2
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Channel_Track_Table,a1	; table de comparaison
	lea	$3b8(a5),a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,PM40_BytePos
	move.l	(a1)+,d3
	move.l	(a1)+,d4
	sub.l	d1,d3
	bne.s	.suivant
	sub.l	d2,d4
	bne.s	.suivant
  .le_meme:
	move.b	PM40_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	rts


Channel_Entete:
	movea.l a4,a0
	movea.l	a5,a1
	lea	42(a1),a1
	moveq	#0,d0
	move.w	(a0),d0
	andi.w	#$FFF0,d0
	lsr	#4,d0
	move.l	d0,d7			; stock sample_nb
	subq	#1,d0
	lea	$A(a0),a0
.loop:
	move.l	(a0)+,d1		; addr_start
	move.w	(a0)+,(a1)		; move length
	move.w	(a0)+,2(a1)		; move volume
	move.l	(a0)+,d2		; addr_repeat
	sub.l	d1,d2			; difference...
	lsr.l	#1,d2			; divise par 2
	move.w	d2,4(a1)		; move repeat
	move.w	(a0)+,6(a1)		; move replen
	moveq	#0,d1
	move.w	(a0)+,d1		; take finetune (multiple de $48)
	divu	#$48,d1			; trouve REAL finetune
	move.b	d1,2(a1)		; mixe avec volume
	lea	30(a1),a1
	dbf	d0,.loop
	move.l	a0,Channel_Track_Table

	cmpi.w	#$1F,d7
	beq.s	.ret
	moveq	#30,d0
	sub.w	d7,d0
.loop1:
	move.l	#$0,(a1)
	move.l	#$1,4(a1)		; fill a vide le reste des samples...
	lea	30(a1),a1
	dbf	d0,.loop1
.ret:
	rts



; ------------------------- Channel Player 3 ------------------------

Channel3_Converter:
	bsr	Channel_Entete

	movea.l	a4,a0
	moveq	#0,d0
	move.w	2(a0),d0		; $A8
	lsr	#3,d0			; div /8
	move.w	d0,Nb_Pos

	bsr	Channel2_Make_PTK_Table

	lea	$3B6(a5),a2
	move.w	Nb_Pos,d0
	move.b	d0,(a2)+
	move.b	#$7F,(a2)+

Channel3_Convert_Patterns:
	movea.l	a4,a1
	moveq	#-1,d6
	moveq	#0,d0
	moveq	#0,d1
	move.w	(a1),d0
	move.w	2(a1),d1
	add.l	d0,a1
	add.l	d1,a1			; debut des notes !
	move.l	a1,Chan_a4		; SAVE IT

	move.l	Channel_Track_Table,a0	; table d'origine

	lea	$3b8(a5),a3		; patterns
	lea	$43c(a5),a2		; patterns destination

	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
.loop:
	moveq	#0,d7
	move.b	(a3)+,d7
	cmp.w	d6,d7
	bgt.s	.okaayyy
	lea	8(a0),a0
	bra.w	.fin_patt
.okaayyy:
	addq.l	#1,d6
	moveq	#3,d1
.Vloop:
	moveq	#0,d3
	move.w	(a0)+,d3		; take 1ere voie
	add.l	d3,a1			; pointe sur les notes de la bonne trk
	moveq	#63,d2
.Nloop:
	moveq	#0,d3
	move.b	(a1)+,d3
	cmpi.w	#$80,d3			; $80 = no note
	beq.w	.No_Note
	bgt.w	.Note_Only
	tst.w	d3			; $00 = take 3bytes no-note, but cmd!
	beq.w	.NoNote_3bytes
.Note_3bytes:
	moveq	#0,d5
	tst.w	d3
	beq.s	.Yipee
	move.w	d3,d5
	lsr	#1,d5
	lsl	#1,d5
	cmp.w	d5,d3
	beq.s	.ok_deja_pair
	subq	#1,d3
	move.w	#$1000,d5		; prepare pour sample > $10
	bra.s	.prout
.ok_deja_pair:
	moveq	#0,d5
.prout:
	subq	#2,d3
	lea	mt_periodtable,a6
	move.w	(a6,d3.w),d3		; Get real note !
.Yipee:
	add.w	d5,d3
	moveq	#0,d5
	move.w	d3,(a2)			; move 1er word dans PTK_Pattern
	moveq	#0,d3
	move.b	(a1)+,d3		; 6C
	lsl	#8,d3
	move.b	(a1)+,d3		; 20    = 6C20
	move.w	d3,d5			; traitement cmd
	andi.w	#$0F00,d5
	cmpi.w	#$0D00,d5
	beq.s	.Convert_deci
	cmpi.w	#$0A00,d5
	beq.s	.Neg_or_decale
	cmpi.w	#$0500,d5
	beq.s	.Neg_or_decale
	cmpi.w	#$0600,d5
	beq.s	.Neg_or_decale
	bra.s	.ok_cmd
.Convert_deci:
	lea	Deci,a6
	moveq	#0,d5
	move.w	d3,d5
	andi.w	#$00FF,d5
	move.b	(a6,d5),d3
	bra.s	.ok_cmd
.Neg_or_decale:
	move.w	d3,d5			; reprend le mot
	andi.w	#$00FF,d5
	cmpi.w	#$F1,d5			; entre F1 et FF, faire un NEG
	bge.s	.Neg_It
	move.w	d3,d5
	andi.w	#$000F,d5
	lsl	#4,d5
	move.b	d5,d3
	bra.s	.ok_cmd
.Neg_It:
	neg.b	d3
.ok_cmd:
	move.w	d3,2(a2)		; move 2eme word dans PTK_Pattern
	bra.w	.Next_Note
.NoNote_3bytes:
	clr.w	(a2)			; pas de note (1er word)
	moveq	#0,d3
	move.b	(a1)+,d3		; take 2eme byte
	lsl.w	#8,d3
	move.b	(a1)+,d3		; take 3eme byte
	move.w	d3,d5			; traitement cmd
	andi.w	#$0F00,d5
	cmpi.w	#$0D00,d5
	beq.s	.Convert_deci
	cmpi.w	#$0A00,d5
	beq.s	.Neg_or_decale
	cmpi.w	#$0500,d5
	beq.s	.Neg_or_decale
	cmpi.w	#$0600,d5
	beq.s	.Neg_or_decale
	bra.s	.ok_cmd
.Note_Only:
	moveq	#0,d5
	subi.w	#$80,d3
	tst.w	d3
	beq.s	.Yip
	move.w	d3,d5
	lsr	#1,d5
	lsl	#1,d5
	cmp.w	d5,d3
	beq.s	.ok_deja_pair2
	subq	#1,d3
	move.w	#$1000,d5		; prepare pour sample > $10
	bra.s	.prout2
.ok_deja_pair2:
	moveq	#0,d5
.prout2:
	subq	#2,d3
	lea	mt_periodtable,a6
	move.w	(a6,d3.w),d3		; Get real note !
.Yip:
	add.w	d5,d3
	moveq	#0,d5
	move.w	d3,(a2)			; move 1er word dans PTK_Pattern
	clr.w	2(a2)
	bra.s	.Next_Note
.No_Note:
	clr.l	(a2)			; all blank note
.Next_Note:
	lea	$10(a2),a2		; note suivante
	dbf	d2,.Nloop
	suba.l	#16*64,a2		; remonte en haut de la voie
	addq	#4,a2			; voie suivante !
	move.l	Chan_a4,a1		; RESTORE IT
	dbf	d1,.Vloop
	suba.l	#16,a2
	adda.l	#$400,a2		; Pattern suivant
.fin_patt:
	dbf	d0,.loop

Channel3_Copy_Samples:
	lea	$43c(a5),a2
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#8,d0
	lsl.l	#2,d0
	add.l	d0,a2			; debut samples (dest)

	movea.l	a4,a0
	move.l	a0,a1
	add.w	(a1),a0
	add.w	2(a1),a0
	add.w	4(a1),a0		; debut samples (source)
	move.l	a0,a1
	add.l	SampLen,a1		; fin des sons
.copy:
	cmp.l	a1,a0
	bge.s	.fini
	move.l	(a0)+,(a2)+
	bra.s	.copy
.fini:
	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts


*-----------------------------------------------------------------------*
;
; StarTrekker Packer Check Routine

CheckStar:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	lea	$100(a0),a0
	move.l	(a0),d3
	cmpi.l	#$1,d3			; $00000001
	bne	.fuck
	cmp.l	#$1,8(a0)		; $00000001
	bne	.fuck
	move.l	-4(a0),d1
	add.l	4(a0),d1
	bne	.fuck		; doit etre NUL
	move.w	12(a0),d1
	beq	.fuck		; ne doit pas etre NUL
	btst	#0,d1
	bne	.fuck
	btst	#1,d1
	bne	.fuck		; si c divisible par 4, on teste.

	movea.l	a4,a0
	moveq	#19,d0			; test les 20 octets du song-name
	moveq	#$20,d2			; ascii ne depasse pas $20 (32)
.name:
	moveq	#0,d1
	move.b	(a0)+,d1
	beq.s	.next_byte
	cmp	d2,d1
	blo	.fuck
.next_byte:
	dbf	d0,.name

	moveq	#0,d7
	moveq	#30,d0			; test sample_data * 31
.smpl:
	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d6
	move.w	(a0)+,d1		; length
	bmi	.fuck
	move.w	(a0)+,d2		; volume
	move.w	(a0)+,d3		; repeat
	move.w	(a0)+,d6		; replen
	move.l	d2,d5
	andi.w	#$00FF,d2
	andi.w	#$FF00,d5
	cmpi.w	#$0040,d2
	bhi	.fuck
	cmpi.w	#$0F00,d5
	bhi	.fuck
	tst.w	d1
	beq.s	.test_replen
	add.w	d3,d6
	cmp.w	d1,d6
	bhi	.fuck
	add.l	d1,d7
	bra.s	.next_smpl
.test_replen:
	add.w	d2,d6			; si length NULLE, vol+rpt+rpl = 0001
	add.w	d3,d6
	cmpi.w	#$0001,d6
	bne	.fuck
.next_smpl:
	dbf	d0,.smpl

	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	move.l	(a0)+,d0		; nb_pos*4
	swap	d0
	lsr	#2,d0
	move.w	d0,Nb_Pos

	lea	$310(a4),a0
	move.l	(a0)+,d0
	beq	.fuck
	movea.l	a0,a2

	adda.l	d0,a0			; fin des notes
	adda.l	d7,a0			; fin des samples
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

	moveq	#63,d0			; test 64 notes
.notes:
	moveq	#0,d1
	move.b	(a2)+,d1
	cmpi.b	#$80,d1
	beq.s	.next_note
	lsl	#8,d1
	move.b	(a2)+,d1
	andi.w	#$0FFF,d1		; garde la note PTK
	beq.s	.nul
	cmpi.w	#$0071,d1
	blo	.fuck
	cmpi.w	#$0358,d1
	bhi	.fuck
.nul:
	addq	#2,a2
.next_note:
	dbf	d0,.notes

; Calc PTK Module Size

	bsr	Star_Speco

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


Star_a4:	dc.l	0
Star_a5:	dc.l	0


Star_Speco:
	move.l	a4,a0

	move.l	(a0)+,d1
	move.l	(a0)+,d1
	move.l	(a0)+,d1
	move.l	(a0)+,d1
	move.l	(a0)+,d1

	moveq	#30,d0
.smp:
	move.l	(a0)+,d1
	move.l	(a0)+,d1
	dbf	d0,.smp

	addq	#8,a0				; pattern addys+1

; ----------------- Star_Make_PTK_Table

	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	-4(a0),a6
	lea	KR_PosTable+1,a2
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend le longword patt
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	a6,a1			; table de comparaison
	lea	KR_PosTable,a3		; PTK pos table
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	rts

*-----------------------------------------------------------------------*
;
; StarTrekker Packer to Protracker Convert routine, from Pro-Wizzy

ConvertStar:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+			; copy song name

	moveq	#30,d0
.smp:
	lea	22(a1),a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	dbf	d0,.smp

	move.w	Nb_Pos,d0
	move.b	d0,(a1)+
	move.b	#$7F,(a1)+

	addq	#8,a0				; pattern addys+1
	addq	#1,a1				; PTK pattpos+1

; ----------------- Star_Make_PTK_Table

	lea	-4(a0),a6
	move.l	a6,Star_a4
	lea	-1(a1),a6
	move.l	a1,a2
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend le longword patt
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Star_a4,a1		; table de comparaison
	movea.l	a6,a3			; PTK pos table
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt

; ---------------------- Star Convert Patterns

	movea.l	a4,a0
	adda.l	$310(a0),a0
	adda.l	#$314,a0
	movea.l	a0,a6			; samples ! ne pas dépasser !

	movea.l	a4,a0
	adda.l	#$314,a0
	lea	$43c(a5),a1		; PTK patterns
	lsl.l	#8,d6
	subq	#1,d6
.conv:
	cmp.l	a6,a0
	bge	.fini
	moveq	#0,d1
	move.b	(a0),d1
	cmpi.b	#$80,d1
	beq	.note_vide
	moveq	#0,d2
	move.b	2(a0),d2
	lsr.b	#4,d2
	andi.b	#$F0,d1
	add	d1,d2
	lsr	#2,d2
	move.b	d2,d1
	lsl.b	#4,d2
	andi.b	#$F0,d1
	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	subq	#4,a1
	andi.l	#$0FFF0FFF,(a1)
	or.b	d1,(a1)
	or.b	d2,2(a1)
	addq	#4,a1
	bra.s	.next_note
.note_vide:
	clr.l	(a1)+
	addq	#1,a0
.next_note:
	dbf	d6,.conv
.fini:
	move.l	a6,a0			; samples source
	lea	$43c(a5),a1
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#8,d0
	lsl.l	#2,d0
	add.l	d0,a1

	bsr	GMC_Copy_Samples

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts


*-----------------------------------------------------------------------*
;
; Fuzzac Packer Check Routine

CheckFuzzac:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	(a0),d3
	cmpi.l	#'M1.0',d3
	bne	.fuck

	movea.l	a4,a0
	lea	66(a0),a0
	moveq	#30,d0
	moveq	#0,d7
.smp:
	moveq	#0,d1
	move.w	(a0),d1
	bmi	.fuck
	beq.s	.nolen
	add.l	d1,d7
	moveq	#0,d2
	move.w	2(a0),d2
	add.w	4(a0),d2
	cmp.w	d1,d2
	bhi	.fuck
	move.w	6(a0),d2
	cmpi.w	#$0040,d2
	bhi	.fuck
.nolen:
	lea	68(a0),a0
	dbf	d0,.smp

	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	lea	$842(a4),a0
	moveq	#0,d0
	move.b	(a0),d0
	beq	.fuck
	cmpi.b	#$7F,d0
	bhi	.fuck
	move.w	d0,Nb_Pos
	move.l	(a0)+,d1
	lsr.l	#8,d1
	andi.l	#$0000FFFF,d1
	beq	.fuck
	lsl	#2,d0
	subq	#1,d0
	moveq	#0,d3
	moveq	#0,d2
.pos:
	move.w	(a0),d2
	cmp.l	d3,d2
	bls.s	.no
	move.l	d2,d3
  .no:	addq	#4,a0
	dbf	d0,.pos
	addi.l	#$100,d3
	cmp.l	d3,d1
	bne	.fuck

	movea.l	a0,a1
	lea	-$40(a1),a1
	move.l	a1,Fuzzac_Notes
	adda.l	d1,a1
	cmpi.l	#'SEnd',(a1)
	bne	.fuck

	moveq	#63,d0
.notes:
	move.l	(a0)+,d1
	andi.l	#$0FFF0000,d1
	swap	d1
	beq.s	.notes
	cmpi.w	#$0071,d1
	blo	.fuck
	cmpi.w	#$0358,d1
	bhi	.fuck
	dbf	d0,.notes

	lea	4(a1),a0
	move.l	a0,Fuzzac_Samples
	adda.l	d7,a0
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	move.l	#$7F*8,d0
	move.l	#$10001,d1
	CALLEXEC AllocMem
	move.l	d0,Fuzzac_Table
	beq.s	.fuck			; no mem   :-(

	bsr	Fuzzac_Speco

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts



Fuzzac_Speco:
	moveq	#0,d0
	move.w	Nb_Pos,d0
	move.l	d0,d7
	lsl	#2,d7
	subq	#1,d0
	lea	$846(a4),a0
	move.l	Fuzzac_Table(pc),a1
.copy:
	moveq	#0,d1
	move.w	(a0),d1
	move.w	d1,(a1)+
	add.l	d7,a0
	move.w	(a0),d1
	move.w	d1,(a1)+
	add.l	d7,a0
	move.w	(a0),d1
	move.w	d1,(a1)+
	add.l	d7,a0
	move.w	(a0),d1
	move.w	d1,(a1)+
	sub.l	d7,a0
	sub.l	d7,a0
	sub.l	d7,a0
	addq	#4,a0
	dbf	d0,.copy

; Make PTK Table

	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; 1er patt = 0 d'office !
	move.l	Fuzzac_Table(pc),a0
	lea	8(a0),a0		; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend les 2 longwords de chak patt
	move.l	(a0)+,d2
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Fuzzac_Table(pc),a1	; table de comparaison
	lea	KR_PosTable,a3
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	move.l	(a1)+,d4
	sub.l	d1,d3
	bne.s	.suivant
	sub.l	d2,d4
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	rts


Fuzzac_Table:	dc.l 0
Fuzzac_Notes:	dc.l 0
Fuzzac_Samples:	dc.l 0


*-----------------------------------------------------------------------*
;
; Fuzzac Packer to Protracker Convert routine, from Pro-Wizzy

ConvertFuzzac:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	addq	#6,a0
	lea	20(a1),a1
	moveq	#30,d0
.smp:
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.w	(a0)+,(a1)+
	lea	38(a0),a0
	move.w	(a0)+,(a1)
	move.w	(a0)+,4(a1)
	move.w	(a0)+,6(a1)
	bne.s	.full
	move.w	#$0001,6(a1)
.full:
	move.w	(a0)+,2(a1)
	lea	8(a1),a1
	dbf	d0,.smp

; Copy table

	moveq	#0,d0
	move.w	Nb_Pos,d0
	movea.l	a5,a0
	move.b	d0,$3b6(a0)

; Make PTK Table

	lea	$3b9(a5),a2		; 1er patt = 0 d'office !
	move.l	Fuzzac_Table(pc),a0
	lea	8(a0),a0		; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend les 2 longwords de chak patt
	move.l	(a0)+,d2
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Fuzzac_Table(pc),a1		; table de comparaison
	lea	$3b8(a5),a3
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	move.l	(a1)+,d4
	sub.l	d1,d3
	bne.s	.suivant
	sub.l	d2,d4
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt

; Convert Patterns

	lea	$3b8(a5),a0
	moveq	#0,d0
	moveq	#-1,d5
	move.w	Nb_Pos,d0
	subq	#1,d0
	move.l	Fuzzac_Table(pc),a1
.loop:
	moveq	#0,d2
	move.b	(a0)+,d2		; take pattern_nbr
	cmp.b	d5,d2
	bgt.s	.okay
	lea	8(a1),a1
	bra.w	.fin
.okay:
	addq	#1,d5			; Plus Grand Pattern
	lsl.w	#2,d2			; mulu #4
	lsl.l	#8,d2			; 0004 = 0400 pour pointer ds patt.
	lea	$43c(a5),a3
	add.l	d2,a3
	moveq	#3,d1			; 4 voies
.Vloop:
	moveq	#0,d2
	move.w	(a1)+,d2		; take track
	move.l	Fuzzac_Notes(pc),a2
	add.l	d2,a2			; pointe sur la bonne track_notes
	moveq	#63,d2			; 64 notes	
.Nloop:
	move.l	(a2)+,(a3)		; take longword
	lea	16(a3),a3		; ligne suivante
	dbf	d2,.Nloop
	suba.l	#64*16,a3
	addq	#4,a3
	dbf	d1,.Vloop		; voie suivante
.fin:
	dbf	d0,.loop

; Copy Samples

	move.l	Fuzzac_Samples(pc),a0
	lea	-$10(a3),a3
	lea	$400(a3),a3
	move.l	a3,a1

	bsr	GMC_Copy_Samples

	move.l	Fuzzac_Table(pc),d0
	beq.s	.ciao
	move.l	d0,a1
	move.l	#$7F*8,d0
	CALLEXEC FreeMem

.ciao:
	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts


*-----------------------------------------------------------------------*
;
; Old-Kefrens Packer Check Routine

CheckOldKef:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	(a0),d3
	move.l	d3,d1
	swap	d1
	cmpi.w	#'M.',d1
	bne	.fuck

	movea.l	a4,a0

	lea	$30(a0),a0
	moveq	#14,d0
	moveq	#0,d7
	moveq	#0,d6
.smp:
	move.l	(a0),d1
	beq	.fuck
	cmp.l	d6,d1
	bls	.fuck
	move.l	d1,d6
	moveq	#0,d1
	move.w	4(a0),d1
	beq	.fuck
	btst	#0,d1
	bne	.fuck
	add.l	d1,d7
	moveq	#0,d1
	move.b	6(a0),d1
	cmpi.b	#$40,d1
	bhi	.fuck
	lea	$20(a0),a0
	dbf	d0,.smp

	move.l	d7,SampLen

	lea	$600(a4),a6			; fin pos_table
	move.l	a4,a3
	adda.l	d4,a3
	cmp.l	a3,a6
	bhi	.fuck
	lea	-$10(a0),a0
	moveq	#0,d2
.pos:
	cmp.l	a6,a0
	bhi	.fuck
	move.l	(a0)+,d0
	move.l	d0,d1
	swap	d1
	andi.w	#$FF00,d1
	cmpi.w	#$FF00,d1
	beq.s	.fintable
	addq	#1,d2
	bra.s	.pos
.fintable:
	move.w	d2,Nb_Pos

	moveq	#$2F,d0
	moveq	#0,d1
.t:	add.l	(a6)+,d1
	dbf	d0,.t
	tst.l	d1
	bne	.fuck

	moveq	#$3F,d0
.notes:
	moveq	#0,d1
	move.b	(a6)+,d1
	cmpi.b	#$24,d1
	bhi	.fuck
	addq	#2,a6
	dbf	d0,.notes

	movea.l	a4,a0
	adda.l	$30(a0),a0
	adda.l	d7,a0
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	bsr	OldKef_Speco

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts



OldKef_Speco:
	movea.l	a4,a0

	lea	$34(a0),a0
	moveq	#14,d0
.smp:
	moveq	#0,d1
	move.w	(a0),d1
	move.w	4(a0),d2
	lea	$20(a0),a0
	dbf	d0,.smp

	lea	-$14(a0),a0
	movea.l	a0,a6

; PTK Table !

	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2
	addq	#4,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend longword
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	movea.l	a6,a1			; table de comparaison
	lea	KR_PosTable,a3
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	rts



*-----------------------------------------------------------------------*
;
;Old-Kefrens Packer to Protracker Convert routine, from Pro-Wizzy

ConvertOldKef:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	lea	$34(a0),a0
	lea	42(a1),a1
	moveq	#14,d0
.smp:
	moveq	#0,d1
	move.w	(a0),d1
	lsr	#1,d1
	move.w	d1,(a1)
	move.b	2(a0),3(a1)
	move.w	4(a0),d2
	beq.s	.noloop
	lsr	#1,d2
	sub.w	d2,d1
	move.w	d2,4(a1)
	move.w	d1,6(a1)
	bra.s	.nexts
.noloop:
	move.l	#1,4(a1)
.nexts:
	lea	$20(a0),a0
	lea	30(a1),a1
	dbf	d0,.smp

	moveq	#15,d0
.smp2:
	clr.l	(a1)
	move.l	#1,4(a1)
	lea	30(a1),a1
	dbf	d0,.smp2

	lea	-22(a1),a1
	move.w	Nb_Pos,d0
	move.b	d0,(a1)+
	move.b	#$7F,(a1)+
	lea	-$14(a0),a0
	movea.l	a0,a6

; PTK Table !

	lea	1(a1),a2
	addq	#4,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend longword
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	movea.l	a6,a1			; table de comparaison
	lea	$3b8(a5),a3
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt

; Convert !

	lea	$3b8(a5),a6
	lea	$43c(a5),a2
	lea	$200(a4),a1
	moveq	#-1,d5
	moveq	#0,d7
	move.w	Nb_Pos,d7		; nb_pos
	subq	#1,d7
.Ploop:
	moveq	#0,d1
	move.b	(a6)+,d1
	cmp.w	d5,d1
	bgt.s	.ok
	addq	#4,a1
	bra.s	.next_patt
.ok:
	addq.l	#1,d5
	moveq	#3,d6			; 4 voies
.Vloop:
	moveq	#0,d1
	move.b	(a1)+,d1
	mulu	#$C0,d1
	lea	$600(a4),a0
	add.l	d1,a0
	moveq	#63,d0			; 64 notes par track
.loop:
	moveq	#0,d4
	move.b	(a0)+,d4
	beq.s	.no_note
	subq	#1,d4
	lsl	#1,d4
	lea	mt_periodtable,a3
	move.w	(a3,d4.w),d4
.no_note:
	move.w	d4,(a2)
	move.b	(a0)+,2(a2)
	move.b	(a0)+,3(a2)
	move.w	2(a2),d4
	andi.w	#$0F00,d4
	cmpi.w	#$0D00,d4
	bne.s	.bon
	andi.w	#$F0FF,2(a2)
	adda.w	#$0A00,2(a2)
.bon:	lea	16(a2),a2
	dbf	d0,.loop
	suba.l	#64*16,a2		; remonte au debut de la voie
	addq	#4,a2			; et voie suivante !
	dbf	d6,.Vloop
	suba.l	#16,a2			; pointe au debut pattern
	adda.l	#$400,a2		; pattern suivant !
.next_patt:
	dbf	d7,.Ploop

; Copy Samples !

	movea.l	a4,a0
	adda.l	$30(a0),a0
	movea.l	a2,a1
	bsr	GMC_Copy_Samples

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts


*-----------------------------------------------------------------------*
;
; Soundtracker 2.6 Check Routine

CheckST26:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	$5b8(a0),d3
	bsr	ST26_IT10_Same
	tst.l	d7
	beq	.fuck

; Calc PTK Module Size

	bsr	ST26_Speco

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


ST26_IT10_Same:
	moveq	#0,d7
	move.l	d3,d1
	andi.b	#$00,d1
	cmpi.l	#$4D544E00,d1		; 'MTN' + 00
	beq.s	.ST26
	cmpi.l	#'IT10',d3
	bne	.fuck

.ST26:
	movea.l	a4,a0
	lea	42(a0),a0
	moveq	#0,d7
	moveq	#30,d0
.s1:
	moveq	#0,d1
	move.w	(a0),d1
	bmi	.fuck
	beq.s	.s2
	moveq	#0,d2
	move.w	4(a0),d2
	add.w	6(a0),d2
	cmp.w	d1,d2
	bhi	.fuck
	add.l	d1,d7
.s2:
	move.w	2(a0),d2
	move.l	d2,d3
	andi.w	#$FF00,d2
	andi.w	#$00FF,d3
	cmpi.w	#$0F00,d2
	bhi	.fuck
	cmpi.w	#$0040,d3
	bhi	.fuck
	lea	30(a0),a0
	dbf	d0,.s1

	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	lea	-22(a0),a0		; $3b6
	moveq	#0,d0
	move.b	(a0)+,d0
	beq	.fuck
	move.w	d0,Nb_Pos
	moveq	#0,d3
	moveq	#0,d1
	move.b	(a0)+,d1		; nb different tracks
	lsl	#2,d0
	subq	#1,d0
.pos:
	moveq	#0,d2
	move.b	(a0)+,d2
	cmp.w	d3,d2
	bls.s	.pend
	move.w	d2,d3
.pend:
	dbf	d0,.pos
	addq	#1,d3
	cmp.w	d1,d3
	bne	.fuck
;	move.w	d1,Nb_Patt		; = Nb_Tracks

	lea	$5bc(a4),a0
	lsl.l	#8,d1
	adda.l	d1,a0
	adda.l	d7,a0
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

	move.l	#'OKAY',d7
.fuck:
	rts


ST26_Speco:
	move.l	a4,a0

	move.l	#$3b5,d0
.loop:
	move.b	(a0)+,d1
	dbf	d0,.loop
	addq	#2,a0
	move.l	a0,ST26_Table

;ST26_Make_PTK_Table:
	addq	#4,a0			; skip 1er pattern
	lea	KR_PosTable+1,a2
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	ST26_Table(pc),a1	; table de comparaison
	lea	KR_PosTable,a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,ST26_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	ST26_BytePos(pc),(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	rts



*-----------------------------------------------------------------------*
;
; ST26 to Protracker Convert routine, from Pro-Wizzy

ConvertST26:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	move.l	#$3b5,d0
.loop:
	move.b	(a0)+,(a1)+
	dbf	d0,.loop
	move.b	(a0),(a1)
	addq	#2,a0
	addq	#3,a1
	move.l	a0,ST26_Table
	move.l	a1,a2

ST26_Make_PTK_Table:
	addq	#4,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	ST26_Table(pc),a1	; table de comparaison
	lea	$3b8(a5),a3		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a3)+,ST26_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	ST26_BytePos(pc),(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt

ST26_Morph:
	lea	$3b8(a5),a2		; Buffer-Destination
	lea	$43c(a5),a1		; Buffer-Destination
	suba.l	a3,a3

	moveq	#-1,d4
	moveq	#0,d6
	move.w	Nb_Pos,d6		; nombre de patterns
	subq	#1,d6
	move.l	ST26_Table(pc),a6
.boucle:
	moveq	#0,d1
	move.b	(a2)+,d1
	cmp.w	d4,d1
	bgt.s	.okette
	addq	#4,a6
	bra.w	.fini
.okette:
	addq.l	#1,d4
	move.l	(a6)+,d3
	moveq	#3,d5			; 4 voies
.Vloop:
	rol.l	#8,d3
	moveq	#0,d1
	move.b	d3,d1
	lea	$5bc(a4),a0
	lsl	#8,d1
	add.l	d1,a0
	moveq	#63,d1			; 64 notes
.Nloop:
	move.l	(a0)+,d7
	bsr	ST26_Check_Fxx
	move.l	d7,(a1)
	lea	16(a1),a1
	dbf	d1,.Nloop
	cmp.l	a3,a0
	ble.s	.zoi
	move.l	a0,a3			; stock + grande adresse
.zoi:
	suba.l	#64*16,a1
	addq	#4,a1
	dbf	d5,.Vloop
	lea	-16(a1),a1
	lea	$400(a1),a1
.fini:
	dbf	d6,.boucle

	move.l	a3,a0			; source samples

	bsr	GMC_Copy_Samples

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts

ST26_Table:	dc.l	0
ST26_Flag:	dc.w	0
ST26_BytePos:	dc.b	0
	even


ST26_Check_Fxx:
	tst.w	ST26_Flag
	beq.s	.ok
	clr.w	ST26_Flag
	rts
.ok:	move.l	d7,d0
	andi.l	#$00000F00,d0
	cmpi.w	#$0F00,d0
	bne	.bye

	move.l	d7,d0
	andi.l	#$000000FF,d0
	cmpi.w	#$001F,d0
	bls	.bye

	move.l	d1,-(sp)

	moveq	#0,d2
	moveq	#0,d1
	move.b	d0,d1
	andi.w	#$00F0,d0
	lsr.b	#4,d0
	andi.w	#$000F,d1

	cmp.w	d0,d1
	bcc.s	.skip
	exg.l	d0,d1
.skip:	move.w	d1,d2		; PT speed
	add.w	d1,d0
	mulu	#500,d1		; 2.5*50*2*2
	divu	d0,d1
	moveq	#0,d0		; round value
	lsr.w	#1,d1
	addx.w	d0,d1		; PT tempo

	andi.l	#$FFFFFF00,d7
	move.b	d2,d7		; Mets PT speed
	move.l	(a0),d0		; note suivante
	andi.l	#$00000FFF,d0
	bne.s	.arg
	addi.w	#$0F00,d0
	move.b	d1,d0		; Mets PT tempo
	andi.w	#$F000,2(a0)
	add.w	d0,2(a0)
	bra.s	.z
.arg:	move.l	4(a0),d0	; note suivante
	andi.l	#$00000FFF,d0
	bne.s	.arg2
	addi.w	#$0F00,d0
	move.b	d1,d0		; Mets PT tempo
	andi.w	#$F000,6(a0)
	add.w	d0,6(a0)
	bra.s	.z
.arg2:	move.l	8(a0),d0	; note suivante
	andi.l	#$00000FFF,d0
	bne.s	.z
	addi.w	#$0F00,d0
	move.b	d1,d0		; Mets PT tempo
	andi.w	#$F000,10(a0)
	add.w	d0,10(a0)

.z:	move.l	(sp)+,d1
	move.w	#1,ST26_Flag

.bye:	rts


*-----------------------------------------------------------------------*
;
; Tracker Packer 3 Check Routine

CheckTP3:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.w	#3,TP_Type
	move.l	(a0),d3
	cmpi.l	#'CPLX',d3
	beq.s	.mb
	cmpi.l	#'MEXX',d3
	bne	.fuck
	move.w	#2,TP_Type
.mb	move.l	4(a0),d3
	cmpi.l	#'_TP3',d3
	beq.s	.mb2
	cmpi.l	#'_TP2',d3
	bne	.fuck
.mb2
	lea	$1c(a0),a0

	moveq	#0,d0
	move.w	(a0)+,d0		; nb samp * 8
	beq	.fuck
	lsr	#3,d0
	subq	#1,d0
	moveq	#0,d7
.loop:
	moveq	#0,d6
	move.w	2(a0),d6		; length
	beq	.fuck
	add.l	d6,d7
	adda.l	#8,a0
	dbf	d0,.loop

	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	moveq	#0,d0
	moveq	#0,d2
	move.w	(a0)+,d0
	move.w	d0,Nb_Pos
	subq	#1,d0
.patt:
	moveq	#0,d1
	move.w	(a0)+,d1
	lsr	#3,d1
	cmp.w	d1,d2
	bhs	.nx
	move.w	d1,d2
.nx:	dbf	d0,.patt
	addq	#1,d2
	move.w	d2,Nb_Patt


	move.l	a0,TP3_Tracks
	lsl	#3,d2
	adda	d2,a0
	moveq	#0,d0
	move.w	(a0)+,d0		; samples_offset
	move.l	a0,TP3_Notes
	adda.l	d0,a0			; debut samples
	move.l	a0,TP3_Samples
	adda.l	d7,a0			; rajoute samplen
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


TP_Type:	dc.w	3

*-----------------------------------------------------------------------*
;
; Tracker Packer 3 to Protracker Convert routine, from Pro-Wizzy

ConvertTP3:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	lea	8(a0),a0
	moveq	#19,d0
.name:
	move.b	(a0)+,(a1)+
	dbf	d0,.name

	lea	$1c(a4),a0
	moveq	#0,d7
	move.w	(a0)+,d7
	lsr	#3,d7
	move.l	d7,d0
	subq	#1,d0
	lea	42(a5),a1
.loop:
	move.w	(a0)+,2(a1)
	move.w	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.loop

	cmpi.w	#$1F,d7
	beq.s	.no_need
	moveq	#30,d0
	sub.w	d7,d0
.loop1:
	move.l	#$0,(a1)
	move.l	#$1,4(a1)		; fill a vide le reste des samples...
	lea	30(a1),a1
	dbf	d0,.loop1

.no_need:
	lea	$3b6(a5),a1
	addq	#2,a0			; saute nb_pos
	moveq	#0,d0
	move.w	Nb_Pos,d0
	move.b	d0,(a1)+
	move.b	#$7F,(a1)+
	subq	#1,d0
.patt:
	moveq	#0,d1
	move.w	(a0)+,d1
	lsr	#3,d1
	move.b	d1,(a1)+
	dbf	d0,.patt

	lea	$43c(a5),a1
	move.l	TP3_Tracks(pc),a0
	moveq	#0,d0
	move.w	Nb_Patt,d0
	subq	#1,d0
.conv:
	moveq	#3,d1			; 4 voies
.tracks:
	move.l	TP3_Notes(pc),a2
	add.w	(a0)+,a2
	moveq	#64,d7			; 64 notes à convertir / voie
.notes:
	moveq	#0,d2
	move.b	(a2)+,d2		; on teste l'octet
	cmpi.w	#$c0,d2
	bhs	.notes_vides		; compteur de lignes vides (1 byte)
	cmpi.w	#$80,d2
	bhi	.cmd_seule		; only cmd (2 bytes)
	cmpi.w	#3,TP_Type
	bne.s	.tp2.1
	cmpi.w	#$5b,d2
	bhs	.fullnote10
	bra.s	.fullnote0
.tp2.1:
	btst	#0,d2
	bne.s	.fullnote10.tp2
.fullnote0:
	moveq	#0,d6
	bra.s	.fullnote
.fullnote10:
	move.l	#$10000000,d6		; prepare le long mot (smpl >=10)
	neg.b	d2
	subi.w	#$81,d2
	bra.s	.fullnote
.fullnote10.tp2:
	move.l	#$10000000,d6		; prepare le long mot (smpl >=10)
	subq	#1,d2
.fullnote:
	tst.w	d2
	beq.s	.no_note
	cmpi.w	#3,TP_Type
	bne.s	.tp2.2
	subq	#1,d2
	lsl	#1,d2
	bra.s	.getnote
.tp2.2:
	subq	#2,d2
.getnote:
	lea	mt_periodtable,a3
	move.w	(a3,d2.w),d2
.no_note:
	swap	d6
	add.w	d2,d6
	swap	d6
	moveq	#0,d2
	move.b	(a2)+,d2
	move.l	d2,d3
	andi.w	#$000F,d3
	beq	.no_cmd
	andi.w	#$FFF0,d2
	cmpi.w	#8,d3
	bne.s	.z
	moveq	#0,d3
  .z:	lsl	#8,d3
	cmpi.w	#$0500,d3
	beq.s	.tt
	cmpi.w	#$0600,d3
	beq.s	.tt
	cmpi.w	#$0A00,d3
	bne.s	.noneed
  .tt:
	moveq	#0,d4
	move.b	(a2)+,d4
	cmpi.w	#$00F0,d4
	bhi.s	.negg
	lsl	#4,d4
	move.b	d4,d3
	bra.s	.no_cmd
  .negg:neg.b	d4
	move.b	d4,d3
	bra.s	.no_cmd
 .noneed:
	move.b	(a2)+,d3
.no_cmd:
	lsl	#8,d2
	add.w	d3,d2
	move.w	d2,d6			; mot faible
	move.l	d6,(a1)
	lea	16(a1),a1
	subq	#1,d7
	bra	.end
.cmd_seule:
	subi.w	#$80,d2
	lsr	#1,d2			; get real cmd
	cmpi.w	#3,TP_Type
	beq.s	.gotit
	lsr	#1,d2
.gotit:
	cmpi.w	#8,d2
	bne.s	.y
	moveq	#0,d2			; arpeggio 8 = 0
  .y:	lsl	#8,d2			; decalage
	moveq	#0,d3
	move.b	(a2)+,d3
	cmpi.w	#$0500,d2
	beq.s	.test
	cmpi.w	#$0600,d2
	beq.s	.test
	cmpi.w	#$0A00,d2
	bne.s	.ok
  .test:
	cmpi.w	#$00F0,d3
	bhi.s	.neg
	lsl	#4,d3
	bra.s	.ok
  .neg:	neg.b	d3
  .ok:  move.b	d3,d2
	move.l	d2,(a1)
	lea	16(a1),a1
	subq	#1,d7
	bra.s	.end
.notes_vides:
	neg.b	d2			; $c0 = 64 notes vides
	subq	#1,d2
  .yo:	clr.l	(a1)
	lea	16(a1),a1		; note suivante, meme voie !
	subq	#1,d7			; decrementer le DBF en force
	dbf	d2,.yo
.end:
	tst.w	d7
	bhi	.notes

	lea	-16*64(a1),a1
	addq	#4,a1			; voie suivante
	dbf	d1,.tracks

	lea	$3f0(a1),a1		; pattern suivant
	dbf	d0,.conv

TP3_Copy_Samples:
	move.l	TP3_Samples(pc),a0	; source
;	move.l	a1,a1			; destination
	bsr	GMC_Copy_Samples

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts

TP3_Tracks:	dc.l	0
TP3_Notes:	dc.l	0
TP3_Samples:	dc.l	0

P4xx_Type:	dc.l	0
P4xx_Tracks:	dc.l	0


P4xx_Get_PGP:
	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; skip 1er pattern
	move.l	P4xx_Tracks(pc),a0
	lea	8(a0),a0		; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1		; prend les 2 longwords de chak patt
	move.l	(a0)+,d2
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	P4xx_Tracks(pc),a1	; table de comparaison
	lea	KR_PosTable,a3		; PTK Module
  .Gloop:
	move.b	(a3)+,KR_BytePos
	move.l	(a1)+,d3
	move.l	(a1)+,d4
	sub.l	d1,d3
	bne.s	.suivant
	sub.l	d2,d4
	bne.s	.suivant
  .le_meme:
	move.b	KR_BytePos(pc),(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:	addq	#1,d6
	move.w	d6,Nb_Patt
	rts


*-----------------------------------------------------------------------*
;
; P4xx Check Routine

CheckP4xx:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	(a0),d1
	move.l	#'P40A',d2
	cmp.l	d2,d1
	beq.s	.P40x
	move.l	#'P40B',d2
	cmp.l	d2,d1
	beq.s	.P40x
	move.l	#'P41A',d2
	cmp.l	d2,d1
	beq.s	.P41x
	clr.l	P4xx_Type
	bra	.fuck

.P40x:
	move.l	#'P40x',P4xx_Type
	moveq	#0,d5
	moveq	#0,d1
	move.b	6(a0),d1		; nb_samples
	subq	#1,d1
	lea	$20(a0),a0
	moveq	#-1,d6
  .a:
	moveq	#0,d2
	move.w	(a0),d2			; finetune
	move.l	d2,d7
	divu	#$4A,d2
	mulu	#$4A,d2
	cmp.w	d2,d7			; finetune multiple of $4a ??
	bne	.fuck
	cmpi.w	#$456,d7		; finetune >$F ??
	bls.s	.good
	clr.l	(a0)			; efface finetune + volume
	clr.w	-8(a0)			; efface length
	move.w	#1,-2(a0)		; mets 1 dans replen
	move.l	-12(a0),-6(a0)		; mets length_start dans repeat_start
.good:
	moveq	#0,d3
	move.w	2(a0),d3		; volume
	cmpi.w	#$40,d3
	bhi	.fuck
	moveq	#0,d3
	move.w	-8(a0),d3		; length
	bmi	.fuck			; never > $8000
	move.l	-12(a0),d7
	cmp.l	d6,d7			; ! debug v2.20 ! samples identiques
	ble.s	.oa
	move.l	d7,d6
	add.l	d3,d5			; adds
.oa:	lea	$10(a0),a0
	dbf	d1,.a
	bra.s	.P4_all

.P41x:
	move.l	#'P41x',P4xx_Type
	moveq	#0,d5
	moveq	#0,d1
	move.b	6(a0),d1		; nb_samples
	subq	#1,d1
	lea	$20(a0),a0
	moveq	#-1,d6
  .b:
	moveq	#0,d2
	move.w	2(a0),d2		; finetune
	move.l	d2,d7
	divu	#$4A,d2
	mulu	#$4A,d2
	cmp.w	d2,d7			; finetune multiple of $4a ??
	bne	.fuck
	cmpi.w	#$456,d7		; finetune >$F ??
	bls.s	.good2
	clr.l	(a0)			; efface finetune + volume
	clr.w	-8(a0)			; efface length
	move.w	#1,-2(a0)		; mets 1 dans replen
	move.l	-12(a0),-6(a0)		; mets length_start dans repeat_start
.good2:
	moveq	#0,d3
	move.w	(a0),d3			; volume
	cmpi.w	#$40,d3
	bhi	.fuck
	moveq	#0,d3
	move.w	-8(a0),d3		; length
	bmi	.fuck			; never > $8000
	move.l	-12(a0),d7
	cmp.l	d6,d7
	ble.s	.oa2
	move.l	d7,d6
	add.l	d3,d5			; adds
.oa2:	lea	$10(a0),a0
	dbf	d1,.b

.P4_all:
	lsl.l	#1,d5
	move.l	d5,SampLen

	moveq	#0,d1
	move.b	5(a4),d1
	move.w	d1,Nb_Pos
	lea	-12(a0),a0
	move.l	a0,P4xx_Tracks

	move.l	d4,-(sp)
	bsr	P4xx_Get_PGP		; Check # of patterns !!
	move.l	(sp)+,d4
	move.b	d6,4(a4)		; Writes back REAL # of patterns !!

	moveq	#0,d1
	move.w	Nb_Pos,d1
	lsl	#3,d1			; *8
	moveq	#0,d2
	move.b	6(a4),d2		; nb samples
	addq	#1,d2
	lsl	#4,d2
	add.l	d1,d2
	addq	#4,d2
	moveq	#0,d1
	move.w	(a4,d2.w),d1		; Here, we must find "FFFF"
	cmpi.w	#$FFFF,d1
	bne	.fuck

	bsr	P4xx_DeInit		; Put back relative addresses !
					; (If necessary...)
	lea	4(a4),a0
	move.l	12(a0),d0
	add.l	a0,d0			; samples
	add.l	SampLen,d0
	move.l	d0,P4_EndMod		; end of mod
	sub.l	a4,d0			; size of mod
	move.l	d4,d1			; loaded size
	sub.l	d0,d1
	cmp.l	#-256,d1
	bge	.maybe
	moveq	#0,d2			; SPECIAL ! Sometimes, there is a
	move.b	6(a4),d2		; useless sample at the end !!??
	lsl	#4,d2			; So, fuck it !! ;-)
	addq	#8,d2
	moveq	#0,d3
	move.w	(a4,d2.w),d3		; length du dernier sample
	lsl.l	#1,d3
	sub.l	d3,d0			; enleve-la
	move.l	d4,d1			; reommence
	sub.l	d0,d1
	cmp.l	#-256,d1
	blt	.fuck			; ce coup-ci, fuck !
	sub.l	d3,SampLen
	sub.l	d3,P4_EndMod		; re-adjust these values
	lea	(a4,d2),a0
	clr.w	(a0)			; efface length
	move.l	-4(a0),2(a0)		; meme length & repeat address
	clr.w	6(a0)		; efface replen
	clr.l	8(a0)		; efface finetune & volume
.maybe:
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


P4xx_DeInit			; Thanx to Marley/Infect for this ;-)
	lea	4(a4),a0	; jump "P40A"
	move.l	16(a0),d0	; pointer to first sample or zero!
	beq.s	.pure_mod	; not played yet!
	moveq	#0,d1
	move.b	2(a0),d1
	subq	#1,d1
	lea	$10(a0),a1
.resmpl	sub.l	d0,(a1)		; re-offset samplepointers
	sub.l	d0,6(a1)
	lea	$10(a1),a1
	dbra	d1,.resmpl
	moveq	#0,d0
	move.b	2(a0),d0
	addq.b	#1,d0
	asl.w	#4,d0
	move.l	8(a0),d1
	sub.l	d0,d1		; original module addresses
	move.l	d0,8(a0)
	sub.l	d1,4(a0)
	sub.l	d1,12(a0)
.pure_mod:
	rts

P4_a4:	dc.l	0
P4_a5:	dc.l	0

*-----------------------------------------------------------------------*
;
; P4xx to Protracker Convert routine, from Jarno Paananen
;                    But hardly debugged by Gryzor (!) :(

ConvertP4xx:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	move.l	a4,P4_a4		; save'em
	move.l	a5,P4_a5

	lea	cha0,a2
	moveq	#51,d2
  .clr:	clr.l	(a2)+
	dbf	d2,.clr

	cmp.l	#'P41x',P4xx_Type
	beq	ConvP41x

ConvP40x:
	clr.w	break
	lea	4(a4),a0

	move.b	1(a0),950(a1)
	move.b	#$7f,951(a1)

	move.l	12(a0),d0
	add.l	a0,d0
	move.l	d0,P4_samples

	lea	16(a0),a2
	lea	42(a1),a3
	moveq	#0,d7
	move.b	2(a0),d7
	move.w	d7,Nb_Samp
	subq	#1,d7
sloop	move.l	(a2)+,d0
	move	(a2)+,(a3)
	move.l	(a2)+,d1
	sub.l	d0,d1
	lsr	#1,d1			; !!!!! (added by Gryzor)
	move	d1,4(a3)
	move	(a2)+,6(a3)

	moveq	#0,d0
	move	(a2)+,d0
	divu	#74,d0
	move.b	d0,2(a3)

	move	(a2)+,d0
	move.b	d0,3(a3)

	lea	30(a3),a3

	dbf	d7,sloop

	cmp.w	#$1F,Nb_Samp
	beq.s	.noneed
	moveq	#30,d0
	sub.w	Nb_Samp,d0
.fier:
	move.l	#$0,(a3)
	move.l	#$1,4(a3)		; fill a vide le reste des samples...
	lea	30(a3),a3
	dbf	d0,.fier
.noneed:

	lea	P4_patterns(pc),a2
	move.l	a2,d7
	moveq	#-1,d0

	moveq	#0,d5
	move.b	(a0),d5
	subq	#1,d5

patloop	move.l	8(a0),a3
	add.l	a0,a3

	moveq	#0,d6
	move.b	1(a0),d6
	subq	#1,d6

	moveq	#0,d1
	move	#$7fff,d1

posloop	move	(a3),d2
	cmp	d0,d2
	ble	nextus
	cmp	d1,d2
	bge	nextus
	move	d2,d1
nextus	addq.l	#8,a3
	dbf	d6,posloop

	move	d1,(a2)+
	move	d1,d0
	dbf	d5,patloop


	lea	952(a1),a2
	move.l	8(a0),a4
	add.l	a0,a4

	moveq	#0,d6
	move.b	1(a0),d6
	subq	#1,d6
sch	lea	P4_patterns(pc),a3

	moveq	#0,d5
	move.b	(a0),d5
	subq	#1,d5

	move	(a4),d1
	addq.l	#8,a4

	moveq	#0,d0
onko	cmp	(a3)+,d1
	beq.b	lyi
	addq	#1,d0
	dbf	d5,onko

lyi	move.b	d0,(a2)+
	dbf	d6,sch


	move.l	4(a0),a2
	add.l	a0,a2

	moveq	#0,d7
	move.b	1(a0),d7
	subq	#1,d7

	move.l	8(a0),a5
	add.l	a0,a5

patconv	lea	P4_patterns(pc),a4

	moveq	#0,d6
	move.b	(a0),d6
	subq	#1,d6

	moveq	#0,d1			; !!!!!
	move	(a5),d1

	moveq	#0,d0
.onko	cmp	(a4)+,d1
	beq.b	.lyi
	addq	#1,d0
	dbf	d6,.onko

.lyi	muls	#1024,d0
	lea	(a1,d0.l),a3
	add.l	#1084,a3	

	move.l	d6,-(sp)

	move.l	a2,d0
;	add	(a5)+,d0
	moveq	#0,d6
	move.w	(a5)+,d6		; !!!!! (abG)
	add.l	d6,d0

	move.l	a2,d1
;	add	(a5)+,d1
	moveq	#0,d6
	move.w	(a5)+,d6
	add.l	d6,d1

	move.l	a2,d2
;	add	(a5)+,d2
	moveq	#0,d6
	move.w	(a5)+,d6
	add.l	d6,d2

	move.l	a2,d3
;	add	(a5)+,d3
	moveq	#0,d6
	move.w	(a5)+,d6
	add.l	d6,d3

	move.l	(sp)+,d6

	move	#64,count
pat	move.l	d0,a4
	lea	cha0(pc),a6
	bsr	convdata
	move.l	a4,d0
	move.l	d4,(a3)+

	move.l	d1,a4
	lea	cha1(pc),a6
	bsr	convdata
	move.l	a4,d1
	move.l	d4,(a3)+

	move.l	d2,a4
	lea	cha2(pc),a6
	bsr	convdata
	move.l	a4,d2
	move.l	d4,(a3)+

	move.l	d3,a4
	lea	cha3(pc),a6
	bsr	convdata
	move.l	a4,d3
	move.l	d4,(a3)+

	tst	break
	bne.b	net

	subq	#1,count
	bne.b	pat

net	clr	break

	dbf	d7,patconv

	move.l	P4_samples(pc),a2

	move.l	P4_a5,a3
	adda.l	#$43c,a3
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	adda.l	d0,a3

	moveq	#0,d0			; copie sample par sample !!
	move.w	Nb_Samp,d0		; en cas de samples similaires...
	subq	#1,d0			; ! debug v2.20 !
	movea.l	a2,a4
	move.l	P4_a4,a5
	lea	20(a5),a5		; 1ere sample_start_addy
.loop:
	movea.l	a4,a2
	add.l	(a5),a2
	moveq	#0,d1
	move.w	4(a5),d1		; sample_length/2
	beq.s	.pl			; no length ? rien a copier
	subq	#1,d1
.smp:
	move.w	(a2)+,(a3)+
	dbf	d1,.smp
.pl:	lea	16(a5),a5
	dbf	d0,.loop

	move.l	a3,P4_end

	move.l	P4_a4,a4
	move.l	P4_a5,a5
	movea.l	a4,a0
	movea.l	a5,a1

	bsr	P4xx_Check56A		; !!!!! (abG)

	move.l	P4_a4,a4
	move.l	P4_a5,a5
	movea.l	a4,a0
	movea.l	a5,a1

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	move.l	P4_end(pc),a6
	moveq	#0,d0
	rts

convdata
	tst.b	3(a6)
	beq.b	takeone
	bmi.b	keepsame

	subq.b	#1,3(a6)
	moveq	#0,d4
	rts

keepsame
	addq.b	#1,3(a6)
	move.l	12(a6),d4
	rts

takeone	tst.b	9(a6)
	beq.b	takenorm

	subq.b	#1,9(a6)

	move.l	a2,-(sp)

	move.l	4(a6),a2
	move.l	(a2)+,(a6)
	move.l	a2,4(a6)

	move.l	(sp)+,a2
	bra	jeah

takenorm
	tst.b	(a4)
	bmi.b	offs
	move.l	(a4)+,(a6)
	bra	jeah

offs	move	(a4)+,8(a6)
	movem.l	d0/a3,-(sp)

	moveq	#0,d0
	move	(a4)+,d0

	lea	(a2,d0.l),a3
	move.l	(a3)+,(a6)
	move.l	a3,4(a6)

	movem.l	(sp)+,d0/a3

jeah	movem.l	d0-d3,-(sp)

	moveq	#$7e,d4
	and.b	(a6),d4
	move	P4_periodtable(pc,d4),d4
	swap	d4

	moveq	#0,d1
	move	(a6),d1
	and	#$1f0,d1
	lsr	#4,d1

	moveq	#$f,d2
	and	d1,d2

	and	#$10,d1

	lsl	#8,d1
	swap	d1
	or.l	d1,d4

	ror	#4,d2
	or	d2,d4

	bsr	checkcomms

	move.l	d4,12(a6)
	
	movem.l	(sp)+,d0-d3
	rts

P4_periodtable
	dc	0
	dc	856,808,762,720,678,640,604,570,538,508,480,453
	dc	428,404,381,360,339,320,302,285,269,254,240,226
	dc	214,202,190,180,170,160,151,143,135,127,120,113
	dc	113

checkcomms
	moveq	#0,d1
	move.b	2(a6),d1

	moveq	#$f,d0
	and	(a6),d0

	cmp	#$b,d0
	bne.b	nobreak
	st	break
	bra	kud	

nobreak	cmp	#$d,d0
	bne.b	nojump
	st	break
	bra	kud	

nojump	cmp	#$e,d0
	bne	noe
	move	d1,d2
	and	#$f0,d2
	bne.b	nofilter
	lsr	#1,d2
	bra	kud

nofilter
	cmp	#$c0,d2
	bne.b	noe
	addq	#1,d1
	bra	kud

noe	cmp.b	#$a,d0
	beq	takas

	cmp.b	#5,d0
	beq	takas

	cmp.b	#6,d0
	bne	eislide
takas	tst.b	d1
	bpl	kud
	neg.b	d1
	lsl	#4,d1
	bra	kud

eislide	cmp	#8,d0
	bne	kud
	moveq	#0,d0

kud	lsl	#8,d0
	or	d0,d4
	move.b	d1,d4
	rts


P4xx_Check56A:
	lea	$43c(a1),a2
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl	#2,d0
	lsl.l	#6,d0
	subq	#1,d0			; check all lines
.patt:
	move.l	(a2),d1
	move.l	d1,d2
	andi.l	#$00000F00,d2
	cmpi.w	#$0E00,d2
	beq	.test2
	cmpi.w	#$0500,d2
	beq.s	.test
	cmpi.w	#$0600,d2
	beq.s	.test
	cmpi.w	#$0A00,d2
	bne.s	.next
.test:
	move.b	d1,d2
	tst.b	d2
	bpl.s	.next
	neg.b	d2
	move.l	d2,d1
	bra.s	.next
.test2:
	tst.b	d1
	beq.s	.change
	cmp.b	#$02,d1
	bne.s	.next
.change:
	move.b	#$01,d1			; force FILTER OFF
.next:
	move.l	d1,(a2)+
	dbf	d0,.patt
	rts


cha0	dc.l	0,0,0,0
cha1	dc.l	0,0,0,0
cha2	dc.l	0,0,0,0
cha3	dc.l	0,0,0,0
	
P4_end	dc.l	0

count	dc.l	0
break	dc	0

P4_patterns
	ds	64

P4_samples	dc.l	0
P4_EndMod	dc.l	0


;------------------------------------ P41A -------------------------------

ConvP41x:
	clr.w	break
	lea	4(a4),a0

	move.b	1(a0),950(a1)
	move.b	#$7f,951(a1)

	move.l	12(a0),d0
	add.l	a0,d0
	move.l	d0,P4_samples

	lea	16(a0),a2
	lea	42(a1),a3
	moveq	#0,d7
	move.b	2(a0),d7
	move.w	d7,Nb_Samp
	subq	#1,d7

	moveq	#0,d5
	moveq	#0,d6
.sloop	move.l	(a2)+,d0
	move	(a2)+,d6
	move	d6,(a3)
	add.l	d6,d5
	add.l	d6,d5
	move.l	(a2)+,d1
	sub.l	d0,d1
	lsr	#1,d1			; !!!!!
	move	d1,4(a3)
	move	(a2)+,6(a3)
	move	(a2)+,2(a3)
	moveq	#0,d0
	move	(a2)+,d0
	divu	#74,d0
	move.b	d0,2(a3)
	lea	30(a3),a3

	dbf	d7,.sloop

	moveq	#0,d6
	move.b	(a0),d6
;	mulu	#1024,d6
	lsl	#2,d6
	lsl.l	#8,d6
	add.l	d6,d5
	add.l	#1084,d5
	move.l	d5,P4_end

	cmp.w	#$1F,Nb_Samp
	beq.s	.noneed
	moveq	#30,d0
	sub.w	Nb_Samp,d0
.fier:
	move.l	#$0,(a3)
	move.l	#$1,4(a3)		; fill a vide le reste des samples...
	lea	30(a3),a3
	dbf	d0,.fier
.noneed:

	lea	P4_patterns(pc),a2
	move.l	a2,d7
	moveq	#-1,d0

	moveq	#0,d5
	move.b	(a0),d5
	subq	#1,d5

	moveq	#0,d4
.patloop
	move.l	8(a0),a3
	add.l	a0,a3

	moveq	#0,d6
	move.b	1(a0),d6
	subq	#1,d6

	move	#$7fff,d1
.posloop
	move	(a3),d2
	cmp	d0,d2
	ble.b	.nextus
	cmp	d1,d2
	bge.b	.nextus
	move	d2,d1
.nextus	addq.l	#8,a3
	dbf	d6,.posloop

	cmp	#$7fff,d1
	beq.b	.juskus
	addq	#1,d4
.juskus	move	d1,(a2)+
	move	d1,d0
	dbf	d5,.patloop

	move.b	d4,(a0)

	lea	952(a1),a2
	move.l	8(a0),a4
	add.l	a0,a4

	moveq	#0,d6
	move.b	1(a0),d6
	subq	#1,d6
.sch	lea	P4_patterns(pc),a3

	moveq	#0,d5
	move.b	(a0),d5
	subq	#1,d5

	move	(a4),d1
	addq.l	#8,a4

	moveq	#0,d0
.onko	cmp	(a3)+,d1
	beq.b	.lyi
	addq	#1,d0
	dbf	d5,.onko

.lyi	move.b	d0,(a2)+
	dbf	d6,.sch



	move.l	4(a0),a2
	add.l	a0,a2

	moveq	#0,d7
	move.b	1(a0),d7
	subq	#1,d7

	move.l	8(a0),a5
	add.l	a0,a5

.patconv
	lea	P4_patterns(pc),a4

	moveq	#0,d6
	move.b	(a0),d6
	subq	#1,d6

	moveq	#0,d1
	move	(a5),d1

	moveq	#0,d0
.onko2	cmp	(a4)+,d1
	beq.b	.lyi2
	addq	#1,d0
	dbf	d6,.onko2

.lyi2	muls	#1024,d0
	lea	(a1,d0.l),a3
	add.l	#1084,a3	

	move.l	d6,-(sp)

	move.l	a2,d0
;	add	(a5)+,d0
	moveq	#0,d6
	move.w	(a5)+,d6
	add.l	d6,d0

	move.l	a2,d1
;	add	(a5)+,d1
	moveq	#0,d6
	move.w	(a5)+,d6
	add.l	d6,d1

	move.l	a2,d2
;	add	(a5)+,d2
	moveq	#0,d6
	move.w	(a5)+,d6
	add.l	d6,d2

	move.l	a2,d3
;	add	(a5)+,d3
	moveq	#0,d6
	move.w	(a5)+,d6
	add.l	d6,d3

	move.l	(sp)+,d6

	move	#64,count
.pat	move.l	d0,a4
	lea	cha0(pc),a6
	bsr	.convdata
	move.l	a4,d0
	move.l	d4,(a3)+

	move.l	d1,a4
	lea	cha1(pc),a6
	bsr	.convdata
	move.l	a4,d1
	move.l	d4,(a3)+

	move.l	d2,a4
	lea	cha2(pc),a6
	bsr	.convdata
	move.l	a4,d2
	move.l	d4,(a3)+

	move.l	d3,a4
	lea	cha3(pc),a6
	bsr	.convdata
	move.l	a4,d3
	move.l	d4,(a3)+

	tst	break
	bne.b	.net

	subq	#1,count
	bne.b	.pat

.net	clr	break

	dbf	d7,.patconv

	move.l	P4_samples(pc),a2
	move.l	P4_a5,a3
	adda.l	#$43c,a3
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	adda.l	d0,a3

	moveq	#0,d0			; copie sample par sample !!
	move.w	Nb_Samp,d0		; en cas de samples similaires...
	subq	#1,d0			; ! debug v2.20 !
	movea.l	a2,a4
	move.l	P4_a4,a5
	lea	20(a5),a5		; 1ere sample_start_addy
.loop:
	movea.l	a4,a2
	add.l	(a5),a2
	moveq	#0,d1
	move.w	4(a5),d1		; sample_length/2
	beq.s	.pl
	subq	#1,d1
.smp:
	move.w	(a2)+,(a3)+
	dbf	d1,.smp
.pl:	lea	16(a5),a5
	dbf	d0,.loop

	move.l	a3,P4_end

	move.l	P4_a4,a4
	move.l	P4_a5,a5
	movea.l	a4,a0
	movea.l	a5,a1

	bsr	P4xx_Check56A

	move.l	P4_a4,a4
	move.l	P4_a5,a5
	movea.l	a4,a0
	movea.l	a5,a1

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	move.l	P4_end(pc),a6
	moveq	#0,d0
	rts

.convdata
	tst.b	3(a6)
	beq.b	.takeone
	bmi.b	.keepsame

	subq.b	#1,3(a6)
	moveq	#0,d4
	rts

.keepsame
	addq.b	#1,3(a6)
	move.l	12(a6),d4
	rts

.takeone
	tst.b	9(a6)
	beq.b	.takenorm

	subq.b	#1,9(a6)

	move.l	a2,-(sp)

	move.l	4(a6),a2
	move.l	(a2)+,(a6)
	move.l	a2,4(a6)

	move.l	(sp)+,a2
	bra.b	.jeah

.takenorm
	tst.b	(a4)
	bmi.b	.offs
	move.l	(a4)+,(a6)
	bra.b	.jeah

.offs	move	(a4)+,8(a6)
	movem.l	d0/a3,-(sp)

	moveq	#0,d0
	move	(a4)+,d0

	lea	(a2,d0.l),a3
	move.l	(a3)+,(a6)
	move.l	a3,4(a6)

	movem.l	(sp)+,d0/a3

.jeah	movem.l	d0-d3,-(sp)

	moveq	#$7e,d4
	and.b	(a6),d4
	move	P41_periodtable(pc,d4),d4
	swap	d4

	moveq	#0,d1
	move	(a6),d1
	and	#$1f0,d1
	lsr	#4,d1

	moveq	#$f,d2
	and	d1,d2

	and	#$10,d1

	lsl	#8,d1
	swap	d1
	or.l	d1,d4

	ror	#4,d2
	or	d2,d4

	bsr.b	P41_checkcomms

	move.l	d4,12(a6)
	
	movem.l	(sp)+,d0-d3
	rts

P41_periodtable
	dc	0
	dc	856,808,762,720,678,640,604,570,538,508,480,453
	dc	428,404,381,360,339,320,302,285,269,254,240,226
	dc	214,202,190,180,170,160,151,143,135,127,120,113
	dc	113

P41_checkcomms
	moveq	#0,d1
	move.b	2(a6),d1

	moveq	#$f,d0
	and	(a6),d0

	cmp	#$b,d0
	bne.b	.nobreak
	st	break
	bra.b	.kud	

.nobreak
	cmp	#$d,d0
	bne.b	.nojump
	st	break
	bra.b	.kud	

.nojump	cmp	#$e,d0
	bne.b	.noe
	move	d1,d2
	and	#$f0,d2
	bne.b	.nofilter
	lsr	#1,d2
	bra.b	.kud

.nofilter
	cmp	#$c0,d2
	bne.b	.noe
	addq	#1,d1
	bra.b	.kud

.noe	cmp.b	#$a,d0
	beq.b	.takas

	cmp.b	#5,d0
	beq.b	.takas

	cmp.b	#6,d0
	bne.b	.eislide
.takas	tst.b	d1
	bpl.b	.kud
	neg.b	d1
	lsl	#4,d1
	bra.b	.kud

.eislide
	cmp	#8,d0
	bne.b	.kud
	moveq	#0,d0

.kud	lsl	#8,d0
	or	d0,d4
	move.b	d1,d4
	rts




*------------------------------ NTPK -------------------------------*
;
; NTPK Packer Check Routine

CheckNTPK:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	$1F6(a0),d0
	cmpi.l	#'PATT',d0
	bne	.fuck

	lea	$174(a0),a0
	moveq	#0,d0
	move.b	(a0),d0			; nb_pos
	beq	.fuck
	bmi	.fuck
	move.w	d0,Nb_Pos
	addq	#2,a0
	bsr	Get_PGP
	cmpi.w	#$40,d1
	bhi	.fuck

	move.l	(a4),d0
	beq	.fuck
	btst	#0,d0
	bne	.fuck
	movea.l	a4,a1
	adda.l	(a4),a1			; debut samples

	subq	#1,d1
	lea	$1f6(a4),a0
.scan:
	move.l	(a0),d0
	addq	#2,a0
	cmpi.l	#'PATT',d0
	beq.s	.found
	cmpa.l	a1,a0
	bhs	.fuck
	bra.s	.scan
.found:
	dbf	d1,.scan
					; okay !

	movea.l	a4,a0
	moveq	#0,d7
	moveq	#30,d0
.smpl:
	move.l	(a0)+,d1
	btst	#0,d1
	bne	.fuck
	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d6
	move.w	(a0)+,d1		; length
	bmi	.fuck
	move.w	(a0)+,d2		; volume
	move.w	(a0)+,d3		; repeat
	move.w	(a0)+,d6		; replen
	move.l	d2,d5
	andi.w	#$00FF,d2
	andi.w	#$FF00,d5
	cmpi.w	#$0040,d2
	bhi	.fuck
	cmpi.w	#$0F00,d5
	bhi	.fuck
	tst.w	d1
	beq.s	.test_replen
	add.w	d3,d6
	cmp.w	d1,d6
	bhi	.fuck
	add.l	d1,d7
	bra.s	.next_smpl
.test_replen:
	add.w	d2,d6			; si length NULLE, vol+rpt+rpl = 0001
	add.w	d3,d6
	cmpi.w	#$0001,d6
	bne	.fuck
.next_smpl:
	dbf	d0,.smpl
	
	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	movea.l	a4,a0
	adda.l	(a4),a0
	adda.l	d7,a0
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; NTPK Packer to Protracker Convert routine, from Pro-Wizzy

ConvertNTPK:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	lea	42(a1),a1
	moveq	#30,d0
.head:
	move.l	(a0)+,d1
	move.l	(a0)+,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.head

	lea	-22(a1),a1
	moveq	#$7F,d0
	addq	#2,d0
.patt:
	move.b	(a0)+,(a1)+
	dbf	d0,.patt


NTPK_Reconst:
	addq	#4,a1
	moveq	#0,d0
	move.w	Nb_Patt,d0
	subq	#1,d0
.patt:
	move.b	(a0),d1
	bne.s	.go
	addq	#1,a0
	bra.s	.patt
.go:	lea	10(a0),a0		; jump 'PATT' + 3 words
	moveq	#3,d1
.track:
	moveq	#63,d2
.notes:
	moveq	#0,d3
	move.b	(a0),d3
	cmp.b	#$FF,d3
	beq	.notes_vides
	cmp.b	#$C0,d3
	bhs	.smpl10_nocmd
	cmp.b	#$80,d3
	bhs	.smpl10_andcmd
	cmp.b	#$40,d3
	bhs	.smpl0_nocmd
.smpl0_andcmd:
	moveq	#0,d3
	move.b	(a0)+,d3
	bsr	NTPK_Get_Note
	moveq	#0,d7
	swap	d7
	move.w	d3,d7
	swap	d7
	move.b	(a0)+,d7
	lsl.w	#8,d7
	move.b	(a0)+,d7
	move.l	d7,(a1)
	lea	16(a1),a1
	bra	.next
.smpl0_nocmd:
	moveq	#0,d3
	move.b	(a0)+,d3
	subi.w	#$40,d3
	bsr	NTPK_Get_Note
	moveq	#0,d7
	swap	d7
	move.w	d3,d7
	swap	d7
	move.b	(a0)+,d7
	lsl.w	#8,d7
	move.l	d7,(a1)
	lea	16(a1),a1
	bra	.next
.smpl10_andcmd:
	moveq	#0,d3
	move.b	(a0)+,d3
	subi.w	#$80,d3
	bsr	NTPK_Get_Note
	addi.w	#$1000,d3
	moveq	#0,d7
	swap	d7
	move.w	d3,d7
	swap	d7
	move.b	(a0)+,d7
	lsl.w	#8,d7
	move.b	(a0)+,d7
	move.l	d7,(a1)
	lea	16(a1),a1
	bra.s	.next
.smpl10_nocmd:
	moveq	#0,d3
	move.b	(a0)+,d3
	subi.w	#$C0,d3
	bsr	NTPK_Get_Note
	addi.w	#$1000,d3
	moveq	#0,d7
	swap	d7
	move.w	d3,d7
	swap	d7
	move.b	(a0)+,d7
	lsl.w	#8,d7
	move.l	d7,(a1)
	lea	16(a1),a1
	bra.s	.next
.notes_vides:
	addq	#1,a0
	move.b	(a0)+,d3
	andi.w	#$00FF,d3
	addq	#1,d2
	subq	#1,d3
  .0:
	clr.l	(a1)
	lea	16(a1),a1
	subq	#1,d2			; dec DBF
	dbf	d3,.0
.next:
	dbf	d2,.notes
	suba.l	#64*16,a1		; back to start
	addq	#4,a1			; next track
	dbf	d1,.track
	lea	$3f0(a1),a1		; next pattern
	dbf	d0,.patt

NTPK_Copy_Samples:
	movea.l	a4,a0
	adda.l	(a4),a0			; samples (source)

	lea	$43c(a5),a1
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	adda.l	d0,a1			; samples (dest)

	bsr	GMC_Copy_Samples

	bsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts


NTPK_Get_Note:
	tst.b	d3
	beq.s	.no_note
	lea	mt_periodtable,a6
	lsl	#1,d3
	subq	#2,d3
	move.w	(a6,d3.w),d3
.no_note:
	rts



*------------------------------ Polka Packer -------------------------------*
;
; Polka Packer Check Routine

CheckPolka:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	move.l	$438(a0),d0
	cmpi.l	#'PWR.',d0
	bne	.fuck

	lea	$43c(a0),a0
	move.l	#(64*4)-1,d0
.loop:
	move.l	(a0)+,d1
	andi.l	#$00FF0000,d1		; garde le num_sample
	swap	d1
	cmpi.w	#$1F,d1
	bhi	.fuck			; ne doit pas depasser $1F !
	dbf	d0,.loop

.Check2:
	lea	42(a4),a0
	moveq	#0,d0
	moveq	#30,d1
.Rloop:
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d5
	move.l	-4(a0),d7		; start addy
	beq	.fuck
	move.w	(a0),d2
	cmpi.w	#$8000,d2
	bhi	.fuck
	add.l	d2,d0			; additionne les longueurs
	move.w	2(a0),d3
	andi.w	#$FF00,d3
	cmpi.w	#$0F00,d3
	bhi	.fuck
	move.w	2(a0),d3
	andi.w	#$00FF,d3
	cmpi.w	#$0040,d3
	bhi	.fuck
	tst.w	d2
	beq.s	.next
	move.w	4(a0),d5
	lsr	#1,d5			; divise par 2
	add.w	6(a0),d5		; add repeat+replen
	cmp.l	d2,d5
	bhi	.fuck
.next:
	lea	30(a0),a0
	dbf	d1,.Rloop

	lsl.l	#1,d0			; mulu #2
	beq	.fuck
	move.l	d0,SampLen
	move.l	d0,d7

	lea	$3b6(a4),a0
	moveq	#0,d0
	move.b	(a0),d0
	addq	#2,a0
	bsr	Get_PGP
	cmpi.w	#$40,d1
	bhi	.fuck

	lea	$43c(a4),a0		; $43c
	lsl	#8,d1
	lsl.l	#2,d1
	adda.l	d1,a0
	adda.l	d7,a0
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts

;
; The convert routine is the same as ConvertWanton ! Go there...
;





*------------------------------ TP 1 -------------------------------*
;
;  Tracker Packer 1 Check Routine

CheckTP1:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	cmpi.l	#'MEXX',(a0)
	bne	.fuck
	move.l	4(a0),d0
	beq	.fuck
	btst	#0,d0
	bne	.fuck

	lea	$1c(a4),a0

	movea.l	a4,a1
	adda.l	4(a1),a1
	movea.l	a4,a2
	adda.l	d4,a2
	suba.l	a1,a2
	cmpa.l	#-256,a2
	blt	.fuck

	move.l	(a0)+,d0
	beq	.fuck
	btst	#0,d0
	bne	.fuck
	move.l	d0,TP1_Samples

	moveq	#30,d0
	moveq	#0,d7
.loop:
	moveq	#0,d6
	move.w	2(a0),d6		; length
	beq.s	.replen
	cmpa.w	#$40,(a0)
	bhi	.fuck
	add.l	d6,d7
	bra.s	.all
.replen:
	cmpa.w	#1,6(a0)
	bne	.fuck
.all:
	adda.l	#8,a0
	dbf	d0,.loop

	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	moveq	#0,d0
	moveq	#0,d2
	move.l	TP1_Samples,d7
	move.w	(a0),d0
	beq	.fuck
	addq	#4,a0
	move.l	a0,TP1_Patt
	addq	#1,d0
	move.w	d0,Nb_Pos
.patt:
	lea	KR_PosTable,a2
	moveq	#(128/4)-1,d0
.clear:
	clr.l	(a2)+
	dbf	d0,.clear

	lea	KR_PosTable+1,a2	; 1er patt = 0 d'office !
	move.l	TP1_Patt(pc),a0
	addq	#4,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:				; bug!
	moveq	#0,d1
	move.w	(a0)+,d1
	addq	#2,a0
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	TP1_Patt(pc),a1		; table de comparaison
	lea	KR_PosTable,a5		; 1er patt = 0 d'office !
  .Gloop:
	move.b	(a5)+,Pha_BytePos
	moveq	#0,d3
	move.w	(a1)+,d3
	addq	#2,a1
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	Pha_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:
	addq	#1,d6
	move.w	d6,Nb_Patt
	cmpi.w	#$40,d6
	bhi	.fuck

	movea.l	a4,a0
	adda.l	d7,a0
	move.l	a0,TP1_Samples
	adda.l	SampLen,a0			; rajoute samplen
	movea.l	a4,a1
	adda.l	d4,a1
	suba.l	a0,a1
	cmpa.l	#-256,a1
	blt	.fuck

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


TP1_Patt:	dc.l	0
TP1_Samples:	dc.l	0


*-----------------------------------------------------------------------*
;
; Tracker Packer 1 to Protracker Convert routine, from Pro-Wizzy

ConvertTP1:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	lea	8(a0),a0
	moveq	#19,d0
.name:
	move.b	(a0)+,(a1)+
	dbf	d0,.name

	lea	22(a1),a1

	lea	$20(a4),a0
	moveq	#30,d0
.smp:
	move.l	(a0)+,d1
	swap	d1
	move.l	d1,(a1)
	move.l	(a0)+,4(a1)
	lea	30(a1),a1
	dbf	d0,.smp

	lea	-22(a1),a1
	moveq	#0,d0
	move.w	Nb_Pos,d0
	move.b	d0,(a1)+
	move.b	#$7F,(a1)+
	lea	KR_PosTable,a2
	moveq	#$7F,d0
.pos:
	move.b	(a2)+,(a1)+
	dbf	d0,.pos

TP1_Convert:
	lea	$43c(a5),a1
	move.l	TP1_Patt,a0
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
.conv:
	movea.l	a4,a2
	moveq	#0,d2
	move.w	(a0),d2
	bne.s	.ok
	addq	#4,a0
	bra	.nextpatt
.ok:	adda.w	d2,a2
	bsr	TP1_SamePatt
	move.l	#(4*64)-1,d1			; 4 voies
.notes:
	moveq	#0,d2
	move.b	(a2)+,d2		; on teste l'octet
	cmpi.w	#$c0,d2
	beq	.note_vide		; compteur de lignes vides (1 byte)
	cmpi.w	#$80,d2
	bhs	.cmd_seule
	btst	#0,d2
	bne.s	.fullnote10.tp2
.fullnote0:
	moveq	#0,d6
	bra.s	.fullnote
.fullnote10.tp2:
	move.l	#$10000000,d6		; prepare le long mot (smpl >=10)
	subq	#1,d2
.fullnote:
	tst.w	d2
	beq.s	.no_note
	subq	#2,d2
.getnote:
	lea	mt_periodtable,a3
	move.w	(a3,d2.w),d2
.no_note:
	swap	d6
	add.w	d2,d6
	swap	d6
	moveq	#0,d2
	move.b	(a2)+,d2
	lsl	#8,d2
	move.b	(a2)+,d2
.no_cmd:
	move.w	d2,d6			; mot faible
	move.l	d6,(a1)+
	bra.s	.end
.cmd_seule:
	subi.w	#$80,d2
	lsr.w	#2,d2
	lsl	#8,d2
	move.l	d2,d6
	move.b	(a2)+,d6
	move.l	d6,(a1)+
	bra.s	.end
.note_vide:
	clr.l	(a1)+
.end:
	dbf	d1,.notes
.nextpatt:
	dbf	d0,.conv

TP1_Copy_Samples:
	move.l	TP1_Samples,a0		; source
	lea	$43c(a5),a1
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	d0,a1

	bsr	GMC_Copy_Samples

	jsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts


TP1_SamePatt:
	lea	$31a(a4),a3			; limite
	clr.l	(a0)+
	move.l	a0,-(sp)
.check:
	cmp.l	a3,a0
	bhs.s	.fini
	moveq	#0,d3
	move.w	(a0),d3
	addq	#4,a0
	cmp.w	d3,d2
	bne.s	.check
	clr.l	-4(a0)
	bra.s	.check
.fini:	move.l	(sp)+,a0
	rts




*------------------------------ Power Music -------------------------------*
;
;  Power Music Check Routine

CheckPowerMusic:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	cmpa.l	#'!PM!',$438(a0)
	bne	.fuck

	move.l	$3b6(a0),d0
	move.l	d0,d1
	andi.l	#$FF000000,d1		; garde nb_pos
	beq	.fuck
	bmi	.fuck
	move.l	d0,d1
	andi.l	#$0000FF00,d1		; garde nb_patt 0
	cmpi.w	#$3F00,d1		; si patt > $3F, pas la peine
	bhi	.fuck
	move.l	d0,d1
	andi.l	#$000000FF,d1		; garde nb_patt 1
	cmpi.w	#$003F,d1		; si patt > $3F, pas le peine
	bhi	.fuck
	move.l	d0,d1
	andi.l	#$00FF0000,d1		; garde constante
	cmpi.l	#$00FF0000,d1		; $FF = Apres OPTIMODisation
	bne	.fuck


; ----- check sample_infos

	lea	42(a0),a0		; pointe sur 1er sample_data
	moveq	#0,d6
	moveq	#$0040,d5
	move.w	#$0F00,d6
	moveq	#0,d7
	moveq	#30,d0
.loop:
	moveq	#0,d1
	move.w	(a0),d1			; take length
	bmi	.fuck
	add.l	d1,d7			; cumule
	moveq	#0,d3
	move.w	2(a0),d3
	move.l	d3,d2
	andi.w	#$00FF,d3		; isole volume !
	cmp.w	d5,d3
	bhi	.fuck			; volume > $40 ??? .fuck !!
	andi.w	#$FF00,d2		; isole finetune !
	cmp.w	d6,d2
	bhi	.fuck			; finetune > $0F ??? .fuck !!
	tst.w	d1
	beq.s	.next
	move.w	4(a0),d2
	add.w	6(a0),d2
	sub.l	d2,d1			; .l !!
	cmpi.l	#-4,d1
	blt	.fuck
	bra.s	.nn
.next:
	move.w	4(a0),d2		; si length nulle,
	add.w	6(a0),d2		; repeat+replen = pas plus de 2
	cmpi.w	#2,d2
	bhi	.fuck
.nn:
	lea	30(a0),a0
	dbf	d0,.loop

	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	moveq	#0,d0
	move.b	$3b6(a4),d0		; nb_pos
	lea	$3b8(a4),a0
	bsr	Get_PGP
	cmpi.w	#$40,d1
	bhi	.fuck

	lea	$43c(a4),a1		; saute heading
	lsl	#8,d1			; nb_patt
	lsl.l	#2,d1
	adda.l	d1,a1
	adda.l	SampLen,a1		; fin du mod calculé
	movea.l	a4,a0			; fin du mod chargé
	adda.l	d4,a0
	suba.l	a1,a0			; enleve le calculé du chargé
	cmpa.l	#-256,a0		; si POS, ok, MIN : pas + de 256
	blt	.fuck


; ----- check notes !

	lea	$43c(a4),a0		; pointe sur le 1er pattern
	moveq	#0,d5
	moveq	#0,d3
	move.w	Nb_Patt,d3
	lsl	#8,d3			; *256 (64*4)
	subq	#1,d3
	cmpi.l	#$5FF,d3
	bls.s	.Ploop
	move.l	#$5FF,d3		; Test sur 6 patterns, si possible
.Ploop:
	move.l	(a0)+,d1
	andi.l	#$0FFF0000,d1
	swap	d1
	tst.w	d1			; pas de note ????
	beq.s	.okay_next		; non, prend la suivante
.une_note:
	addq.l	#1,d5			; inc nb_notes
	lea	mt_periodtable,a2	; sinon compare avec mt_pt
	moveq	#35,d2			; 36 notes dans mt_periodtable
.test_note:
	cmp.w	(a2)+,d1
	beq.s	.okay_next
	dbf	d2,.test_note

	bra.s	.fuck			; note pas trouvée : fuck

  .okay_next:
	dbf	d3,.Ploop
	tst.w	d5
	beq.s	.fuck


; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; Power Music to Protracker Convert routine, from Pro-Wizzy

ConvertPowerMusic:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	move.l	#$43c/4-1,d0
.copy_header:
	move.l	(a0)+,(a1)+
	dbf	d0,.copy_header

	moveq	#0,d0
	move.w	Nb_Patt,d0		; 1
	lsl.l	#6,d0			; $40
	lsl.l	#2,d0			; $100
	subq	#1,d0
.copy_patt:
	move.l	(a0)+,(a1)+
	dbf	d0,.copy_patt

; ----- convert samples

	move.l	SampLen,d0

	moveq	#7,d1				; THANKS CHEXUM !!
	moveq	#0,d2				; (OH NO! NO MORE PASCAL ;)
	and.w	d0,d1				; len mod 7
	neg.w	d1				; 0..-7
	asl.w	#2,d1				; 0..-28
	jmp	.eolp(pc,d1.w)

.copy8	add.b	(a0)+,d2			; single byte
	move.b	d2,(a1)+
	add.b	(a0)+,d2			; next
	move.b	d2,(a1)+
	add.b	(a0)+,d2
	move.b	d2,(a1)+
	add.b	(a0)+,d2
	move.b	d2,(a1)+

	add.b	(a0)+,d2
	move.b	d2,(a1)+
	add.b	(a0)+,d2
	move.b	d2,(a1)+
	add.b	(a0)+,d2
	move.b	d2,(a1)+
	add.b	(a0)+,d2
	move.b	d2,(a1)+
.eolp:	subq.l	#8,d0
	bcc.s	.copy8


	jsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts


STP3_Patt_Adr:	dc.l	0
STP3_Samp_Adr:	dc.l	0

*-------------------------- SoundTracker Pro 3.0 --------------------------*
;
; STP3 Packer Check Routine

CheckSTP3:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4

	cmpa.l	#'STP3',(a0)
	bne	.fuck

	moveq	#0,d0
	move.b	6(a0),d0
	cmp.b	#$7F,d0
	bhi	.fuck
	move.w	d0,Nb_Pos
	cmp.b	#$40,7(a0)
	bne	.fuck
	cmp.w	#$0DE0,$8c(a0)
	bne	.fuck
	cmp.w	#$32,$94(a0)
	bne	.fuck
	cmp.w	#$1804,$a4(a0)
	bne	.fuck
	cmp.w	#$1808,$b4(a0)
	bne	.fuck
	cmp.w	#$000F,$c4(a0)
	bne	.fuck
	cmp.w	#$001F,$c8(a0)		; pas + de $1F samples
	bhi	.fuck
	move.w	$c8(a0),Nb_Samp

	lea	$cc(a4),a0
	moveq	#0,d2
	moveq	#0,d0
	move.w	Nb_Samp,d0
	subq	#1,d0
.loop:
	move.w	(a0),d1
	cmpi.w	#$1F,d1
	bhi	.fuck
	moveq	#0,d3
	move.w	$42(a0),d3
	beq	.fuck
	add.l	d3,d2			; cumul longueurs
	moveq	#0,d1
	move.b	$44(a0),d1		; volume
	cmpi.w	#$40,d1
	bhi	.fuck
	move.w	$48(a0),d1		;   repeat
	add.w	$4c(a0),d1		; + replen
	cmp.l	d3,d1
	bhi	.fuck
	lea	$56(a0),a0
	dbf	d0,.loop

	move.l	d2,SampLen
	move.l	a0,STP3_Patt_Adr


	movea.l	a4,a3
	adda.l	d4,a3			; fin du module chargé

	lea	8(a4),a1
	move.l	a0,a2
	moveq	#0,d7			; + grand pattern
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
.loop1:
	move.l	a2,a0
	moveq	#0,d1
	move.b	(a1)+,d1		; num patt à trouver
 .scan:
	cmpa.w	#$40,2(a0)
	bne	.fuck
	cmpa.w	#$4,4(a0)
	bne	.fuck
	cmp.w	(a0),d1
	beq.s	.next_patt
 .next:
	lea	$406(a0),a0
	cmpa.l	a3,a0
	bhi	.fuck
	bra.s	.scan
.next_patt:
	cmp.w	d7,d1
	bls.s	.no
	move.w	d1,d7
  .no:	cmpa.l	#$FFFFFFFF,$406(a0)
	bne.s	.eloop1
	lea	$42c(a0),a6
	move.l	a6,STP3_Samp_Adr
.eloop1:
	dbf	d0,.loop1

.rest:
	addq	#1,d7
	cmpi.w	#$40,d7
	bhi	.fuck
	move.w	d7,Nb_Patt

	movea.l	a6,a1
	adda.l	SampLen,a1
	movea.l	a3,a0			; fin du mod chargé
	suba.l	a1,a0			; enleve le calculé du chargé
	cmpa.l	#-256,a0		; si POS, ok, MIN : pas + de 256
	blt.s	.fuck


; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; STP3 Packer to Protracker Convert routine, from Pro-Wizzy

ConvertSTP3:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

; ----- header

	lea	42(a1),a1
	moveq	#30,d0
.raz:
	clr.l	(a1)
	move.l	#1,4(a1)
	lea	30(a1),a1
	dbf	d0,.raz

	lea	$cc(a0),a0
	moveq	#0,d0
	move.w	-4(a0),d0		; nb_samp
	subq	#1,d0
.samp:
	moveq	#0,d1
	move.w	(a0),d1
	subq	#1,d1
	mulu	#30,d1
	lea	20(a5),a1
	adda.l	d1,a1			; sample_name
	move.l	$22(a0),(a1)		; 4
	move.l	$26(a0),4(a1)		; 8
	move.l	$2a(a0),8(a1)		; 12
	move.l	$2e(a0),12(a1)		; 16
	move.l	$32(a0),16(a1)		; 20
	move.w	$36(a0),20(a1)		; 22
	moveq	#0,d1
	move.w	$42(a0),d1
	lsr	#1,d1
	move.w	d1,22(a1)		; length
	moveq	#0,d1
	move.b	$44(a0),d1
	move.w	d1,24(a1)		; volume
	moveq	#0,d1
	move.w	$48(a0),d1
	lsr	#1,d1
	move.w	d1,26(a1)		; repeat
	moveq	#0,d1
	move.w	$4c(a0),d1
	lsr	#1,d1
	or.w	#1,d1			; valeur ou 0001 !
	move.w	d1,28(a1)		; replen

	lea	$56(a0),a0
	dbf	d0,.samp


; ----- patt_table

	lea	6(a4),a0
	lea	$3b6(a5),a1
	move.b	(a0),(a1)+
	move.b	#$7F,(a1)+
	addq.l	#2,a0
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
.pos:
	move.b	(a0)+,(a1)+
	dbf	d0,.pos


; ----- patterns !

	move.l	STP3_Patt_Adr,a0
.patt_loop:
	cmpi.l	#$FFFFFFFF,(a0)
	beq.s	.patt_end
	moveq	#0,d0
	move.w	(a0),d0
	lsl.l	#8,d0
	lsl.l	#2,d0
	lea	$43c(a5),a1
	adda.l	d0,a1
	addq.l	#6,a0

	move.l	#$ff,d0			; 256 notes (lignes)
  .lines:
	move.l	(a0)+,d1
	bsr	STP3_ConvNote
	move.l	d1,(a1)+
	dbf	d0,.lines
	bra.s	.patt_loop

.patt_end:
	move.l	STP3_Samp_Adr,a0
;	movea.l	a1,a1			; sample_destination
	bsr	GMC_Copy_Samples

	jsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts


STP3_ConvNote:
	moveq	#0,d7
	move.l	d1,d2
	andi.l	#$FF000000,d2		; sample number
	rol.l	#8,d2
	move.l	d2,d3
	andi.b	#$F0,d2
	andi.b	#$0F,d3
	rol.w	#8,d2
	move.w	d2,d7
	swap	d7
	ror.w	#4,d3
	move.w	d3,d7

	move.l	d1,d2
	andi.l	#$00FF0000,d2		; la note
	swap	d2
	beq.s	.no_note
;	subq	#1,d2			; ! debug v2.20 !
	subi.w	#$c*2,d2		; soustraire 2 octaves !?!?
	lsl	#1,d2
	lea	mt_periodtable,a6
	move.w	(a6,d2.w),d2
.no_note:
	swap	d7
	add.w	d2,d7
	swap	d7

	move.l	d1,d2			; cmd
	andi.l	#$0000FFFF,d2
	move.l	d2,d3
	andi.w	#$FF00,d3
	cmpi.w	#$1400,d3
	bne.s	.efx1
	andi.w	#$00FF,d2
	addi.w	#$0B00,d2
	bra	.efxx
.efx1:
	move.l	d2,d3
	andi.w	#$FF00,d3
	cmpi.w	#$0D00,d3
	bne.s	.efx2
	move.w	#$0A00,d3
	moveq	#0,d4
	move.b	d2,d4
	lsr.b	#4,d4
	add.b	d4,d3
	move.w	d3,d2
.efx2:
	cmpi.w	#$0F00,d3
	bne.s	.efxx
	ror.b	#4,d2			; 70 => 07
.efxx:
	andi.w	#$0FFF,d2
	add.w	d2,d7

	move.l	d7,d1			; note finale
	rts





*------------------------------ Zen Packer -------------------------------*
;
; Zen Packer Check Routine

CheckZen:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	a0,a3
	move.l	d0,d4

	move.l	(a0),d0
	btst	#0,d0
	bne	.fuck
	cmpi.l	#$200000,d0
	bhs	.fuck

	moveq	#0,d0
	move.b	4(a3),d0		; nb_patt-1
	cmpi.w	#$3F,d0
	bhi	.fuck
	addq	#1,d0
	move.w	d0,Nb_Patt
	moveq	#0,d0
	move.b	5(a3),d0		; nb_pos
	cmpi.w	#$7F,d0
	bhi	.fuck
	move.w	d0,Nb_Pos

	lsl	#2,d0
	addq	#4,d0
	add.l	(a3),d0
	cmp.l	$e(a3),d0
	bne	.fuck

	lea	6(a3),a0
	moveq	#30,d0
	moveq	#0,d7
	moveq	#0,d6
.sampdata:
	moveq	#0,d1
	move.w	(a0),d1			; finetune
	beq.s	.vol
	move.l	d1,d2
	divu	#$48,d2
	mulu	#$48,d2
	cmp.l	d1,d2
	bne	.fuck
  .vol:	moveq	#0,d1
	move.w	2(a0),d1		; volume
	beq.s	.len
	cmpi.w	#$40,d1
	bhi	.fuck
  .len:	moveq	#0,d1
	move.w	4(a0),d1		; length
	bmi	.fuck
	beq.s	.test_replen
	add.l	d1,d6
	bra.s	.repeat
  .test_replen:
	cmp.w	#$0001,6(a0)
	bne	.fuck
  .repeat:
	move.l	8(a0),d1		; start addy
	move.l	12(a0),d2		; repeat addy
	cmp.l	d1,d2
	blo	.fuck
	cmp.l	d1,d7
	bhi	.fuck
	move.l	d1,d7

	lea	16(a0),a0
	dbf	d0,.sampdata

	lsl.l	#1,d6
	beq	.fuck
	move.l	d6,SampLen


; ----- Cherche le $FFFFFFFF

	movea.l	a4,a5
	adda.l	d4,a5
	lea	$1f6(a3),a0
.hunt:
	cmp.l	#-1,(a0)
	beq.s	.yep
	addq	#2,a0
	cmpa.l	a5,a0
	bhs	.fuck
	bra.s	.hunt
.yep:
	movea.l	a0,a1
	moveq	#0,d0
	move.w	Nb_Pos,d0
	lsl	#2,d0
	suba.l	d0,a1
	move.l	-4(a1),d1
	cmpi.l	#$FF000000,d1
	bne	.fuck

	movea.l	a1,a2
	move.l	a2,Table_Originale

	addq.l	#4,a0
	movea.l	a0,a1
	adda.l	d6,a1
	movea.l	a5,a0			; fin du mod chargé
	suba.l	a1,a0			; enleve le calculé du chargé
	cmpa.l	#-256,a0		; si POS, ok, MIN : pas + de 256
	blt	.fuck


; ----- Trouve le plus petit pattern (ou son adresse)

	movea.l	a2,a0
	move.l	(a0)+,d7
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.no_need
	subq	#1,d0
.h:
	move.l	(a0)+,d6
	cmp.l	d7,d6
	bhs.s	.n
	move.l	d6,d7
.n:	dbf	d0,.h

.no_need:
	move.l	d7,d0			; plus petit pattern
	subi.l	#$1F6,d0		; = adresse de base !


; ----- De-Initialise les adresses absolues

	movea.l	a2,a0
	moveq	#0,d1
	move.w	Nb_Pos,d1
	subq	#1,d1
.i:
	suba.l	d0,(a0)+
	dbf	d1,.i

	movea.l	a3,a0
	suba.l	d0,(a0)
	lea	$e(a0),a0
	moveq	#30,d1
.j:
	suba.l	d0,(a0)
	suba.l	d0,4(a0)
	lea	16(a0),a0
	dbf	d1,.j


; ----- Make PTK Table

	lea	PP21_Table,a0		; EFFACE les Save_Tables
	moveq	#0,d1
	moveq	#127,d0
.efface:
	move.l	d1,(a0)+
	dbf	d0,.efface


	lea	PP21_Table+1,a2
	move.l	Table_Originale,a0
	addq	#4,a0			; skip 1er pattern
	moveq	#0,d6
	moveq	#0,d0
	move.w	Nb_Pos,d0
	subq	#1,d0
	beq.s	.nn
.compare:
	move.l	(a0)+,d1
	moveq	#0,d5
	move.w	Nb_Pos,d5
	subq	#1,d5
	sub.w	d0,d5			; - loop boucle ppale
	move.l	Table_Originale,a1	; table de comparaison
	lea	PP21_Table,a6
  .Gloop:
	move.b	(a6)+,Pha_BytePos
	move.l	(a1)+,d3
	sub.l	d1,d3
	bne.s	.suivant
  .le_meme:
	move.b	Pha_BytePos,(a2)+
	bra.s	.next
.suivant:
	dbf	d5,.Gloop
.next2:
	addq	#1,d6
	move.b	d6,(a2)+
.next:
	subq	#1,d0
	bne.s	.compare
.nn:
	addq	#1,d6
	sub.w	Nb_Patt,d6
	bne	.fuck


; ----- Test des note_numbers sur le 1er pattern

	lea	$1f6(a3),a0
	moveq	#0,d0
	move.b	(a0),d0
	cmpi.w	#$00FF,d0
	beq.s	.okette
	move.l	d0,d1
	addq	#4,a0
.m:
	moveq	#0,d0
	move.b	(a0),d0
	cmpi.w	#$00FF,d0
	beq.s	.okette
	cmp.w	d1,d0
	ble	.fuck
	move.l	d0,d1
	addq	#4,a0
	bra.s	.m

.okette:

; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; Zen Packer to Protracker Convert routine, from Pro-Wizzy

ConvertZen:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	addq	#6,a0
	lea	42(a1),a1
	moveq	#30,d0
.Tloop:
	moveq	#0,d1
	move.w	(a0)+,d1		; finetune
	divu	#$48,d1
	move.b	d1,2(a1)

	moveq	#0,d1
	move.w	(a0)+,d1		; volume
	move.b	d1,3(a1)

	moveq	#0,d1
	move.w	(a0)+,d1		; length
	move.w	d1,(a1)

	moveq	#0,d1
	move.w	(a0)+,d1		; replen
	move.w	d1,6(a1)

	move.l	(a0)+,d2		; length_addy
	move.l	(a0)+,d3		; repeat_addy
	sub.l	d2,d3
	lsr	#1,d3
	move.w	d3,4(a1)

	lea	30(a1),a1
	dbf	d0,.Tloop


	lea	$3b6(a5),a1
	moveq	#0,d0
	move.w	Nb_Pos,d0
	move.b	d0,(a1)+
	move.b	#$7F,(a1)+
	lea	PP21_Table,a0
	subq	#1,d0
.z:
	move.b	(a0)+,(a1)+
	dbf	d0,.z

; ----- Convert Patterns

	lea	$3b8(a5),a6
	move.l	Table_Originale,a1
	lea	$43c(a5),a2
	moveq	#-1,d5
	moveq	#0,d7
	move.w	Nb_Pos,d7		; nb_pos
	subq	#1,d7
.Ploop:
	moveq	#0,d1
	move.b	(a6)+,d1
	cmp.w	d5,d1
	bgt.s	.ok
	addq	#4,a1
	bra.w	.next_patt
.ok:
	addq.l	#1,d5
	moveq	#0,d1
	move.l	(a1)+,d1
	movea.l	a4,a0
	adda.l	d1,a0
.loop:
	moveq	#0,d6
	moveq	#0,d1
	moveq	#0,d2			; note finale
	move.b	(a0)+,d1
	cmpi.w	#$FF,d1			; derniere note ?
	beq	.lastnote
	lsl	#2,d1			; * 4o
	add.l	d1,a2			; destination localisée
	move.l	d1,d0
	moveq	#0,d1
	move.b	(a0)+,d1		; note_number
	beq.s	.no_note
	btst	#0,d1
	beq.s	.pair
	move.l	#$10000000,d6
	subq	#1,d1
	beq.s	.no_note
.pair:
	subq	#2,d1			; - 2
	lea	mt_periodtable,a3
	moveq	#0,d3
	move.w	(a3,d1.w),d3		; take real note
	swap	d2
	move.w	d3,d2
	swap	d2
.no_note:
	move.w	(a0)+,d2
	add.l	d6,d2
	move.l	d2,(a2)
	suba.l	d0,a2
	bra	.loop
.lastnote:
	moveq	#0,d6
	lsl	#2,d1			; * 4o
	add.l	d1,a2			; destination localisée
	move.l	d1,d0
	moveq	#0,d1
	move.b	(a0)+,d1		; note_number
	beq.s	.no_note2
	btst	#0,d1
	beq.s	.pair2
	move.l	#$10000000,d6
	subq	#1,d1
	beq.s	.no_note2
.pair2:
	subq	#2,d1			; - 1
	lea	mt_periodtable,a3
	moveq	#0,d3
	move.w	(a3,d1.w),d3		; take real note
	swap	d2
	move.w	d3,d2
	swap	d2
.no_note2:
	move.w	(a0)+,d2
	add.l	d6,d2
	move.l	d2,(a2)
	suba.l	d0,a2
	adda.l	#$400,a2

	cmp.b	#$FF,(a0)
	bne.s	.next_patt
	addq	#4,a0

.next_patt:
	dbf	d7,.Ploop


; ----- Copy samples

	movea.l	a4,a0
	adda.l	$e(a4),a0		; Source

	lea	$43c(a5),a1
	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	adda.l	d0,a1			; Destination

	bsr	GMC_Copy_Samples

	jsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts




*------------------------------ Hornet Packer -------------------------------*
;
; Hornet Packer Check Routine

CheckHRT:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	movea.l	a0,a4
	movea.l	a0,a3
	move.l	d0,d4

	move.l	$438(a0),d6
	cmpi.l	#'HRT!',d6
	bne	.fuck

	lea	42(a0),a0		; pointe sur 1er sample_data
	moveq	#0,d6
	moveq	#$0040,d5
	move.w	#$0F00,d6
	moveq	#0,d7
	moveq	#30,d0
.loopz:
	moveq	#0,d1
	move.w	(a0),d1			; take length
	bmi	.fuck
	add.l	d1,d7			; cumule
	moveq	#0,d3
	move.w	2(a0),d3
	move.l	d3,d2
	andi.w	#$00FF,d3		; isole volume !
	cmp.w	d5,d3
	bhi	.fuck			; volume > $40 ??? .fuck !!
	andi.w	#$FF00,d2		; isole finetune !
	cmp.w	d6,d2
	bhi	.fuck			; finetune > $0F ??? .fuck !!
	tst.w	d1
	beq.s	.nextz
	move.w	4(a0),d2
	add.w	6(a0),d2
	sub.l	d2,d1			; .l !!
	cmpi.l	#-$10,d1
	blt	.fuck
	bra.s	.nn
.nextz:
	move.w	4(a0),d2		; si length nulle,
	add.w	6(a0),d2		; repeat+replen = pas plus de 2
	cmpi.w	#2,d2
	bhi	.fuck
.nn:
	lea	30(a0),a0
	dbf	d0,.loopz

	lsl.l	#1,d7
	beq	.fuck
	move.l	d7,SampLen

	move.l	$3b6(a3),d5
	move.l	d5,d1
	andi.l	#$FF000000,d1		; garde nb_pos
	beq	.fuck
	bmi	.fuck
	move.l	d5,d1
	andi.l	#$0000FF00,d1		; garde nb_patt 0
	cmpi.w	#$3F00,d1		; si patt > $3F, pas la peine
	bhi	.fuck
	move.l	d5,d1
	andi.l	#$000000FF,d1		; garde nb_patt 1
	cmpi.w	#$003F,d1		; si patt > $3F, pas la peine
	bhi	.fuck
	move.l	d5,d1
	andi.l	#$00FF0000,d1		; garde constante
	cmpi.l	#$007F0000,d1		; si const > $7F, pas la peine
	bhi	.fuck

	moveq	#$7f,d0
	lea	$3b8(a3),a0		; pointe sur la patt_table
	bsr	Get_PGP
	cmpi.w	#$40,d1
	bhi	.fuck

	lea	$43c(a3),a1		; saute "HRT!"
	lsl	#8,d1			; nb_patt
	lsl.l	#2,d1
	adda.l	d1,a1
	adda.l	SampLen,a1		; fin du mod calculé
	movea.l	a3,a0			; fin du mod chargé
	adda.l	d4,a0
	suba.l	a1,a0			; enleve le calculé du chargé
	cmpa.l	#-256,a0		; si +, ok, - : pas - de 256
	blt	.fuck

	moveq	#0,d1
	move.w	Nb_Patt,d1
	lsl	#8,d1
	subq.l	#1,d1
	move.l	d1,Bigest_Patt

	lea	$43c(a3),a0
	moveq	#0,d6
	moveq	#0,d7
	move.w	#$0048,d6
	move.w	#$3E00,d7
	moveq	#0,d3
	move.l	d1,d0
	cmpi.l	#$4FF,d0
	bls.w	.loop
	move.l	#$4FF,d0		; Test 5 patterns
.loop:
	move.l	(a0)+,d1
	beq.s	.Nnext
	move.l	d1,d2
	move.l	d1,d5
	andi.l	#$0000F000,d5		; toujours a 0 ici !
	bne	.fuck
	swap	d1
	swap	d2
	andi.w	#$00FF,d1		; garde la note
	andi.w	#$FF00,d2		; garde le smpl_nb
	cmp.w	d6,d1			; ne doit pas depasser $24*2 !
	bhi	.fuck			; $24 = 36 notes maxi
	cmp.w	d7,d2			; ne doit pas depasser $1F*2 !
	bhi	.fuck
	moveq	#-1,d3
.Nnext:
	dbf	d0,.loop
	tst.w	d3
	beq	.fuck		; si d3 encore à 00, no notes : .fuck


; Calc PTK Module Size

	move.w	#$31,Format_nb		; Hornet !

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
; Hornet Packer to Protracker Convert routine, from Pro-Wizzy
;
; Go to ConvertPRU1:




*------------------------------ Next one ??? -------------------------------*
;
;  Packer Check Routine

Check:
;	lea	Pack_Mod,a0
;	move.l	#Pack_End-Pack_Mod,d0

	move.l	a0,a4
	move.l	d0,d4



; Calc PTK Module Size

	moveq	#0,d0
	move.w	Nb_Patt,d0
	lsl.l	#2,d0
	lsl.l	#8,d0
	add.l	SampLen,d0
	addi.l	#$43c,d0		; size for dtg_AllocListData
	bra.s	.okay

.fuck:	moveq	#0,d0			; fail!
.okay:	rts


*-----------------------------------------------------------------------*
;
;  Packer to Protracker Convert routine, from Pro-Wizzy

Convert:
;	lea	Pack_Mod,a0		; Pour debugging
;	lea	PTK_Mod,a1

	move.l	a0,a4
	move.l	a1,a5

	jsr	Converted_by_Gryzor	; Incruste_message

	; **** Successful exit

	moveq	#0,d0
	rts




*-----------------------------------------------------------------------*

;
;
	SECTION GenieDatas,Data
;
;

Format_nb:	dc.w	0
SampLen:	dc.l 	0
Bigest_Patt:	dc.l 	0
Nb_Pos:		dc.w	0
Nb_Patt:	dc.w	0
Nb_Samp:	dc.w	0

		ds.l	4
mt_periodtable:	dc.w	$0358,$0328,$02fa,$02d0,$02a6,$0280,$025c,$023a,$021a
		dc.w	$01fc,$01e0,$01c5,$01ac,$0194,$017d,$0168,$0153,$0140
		dc.w	$012e,$011d,$010d,$00fe,$00f0,$00e2,$00d6,$00ca,$00be
		dc.w	$00b4,$00aa,$00a0,$0097,$008f,$0087,$007f,$0078,$0071
		dc.w	$0071,$0071,$0071,$0071,$0071,$0071,$0071,$0071,$0071

;delibase:	dc.l 0
DeliPort:	dc.l 0

FormName:	dc.l 0

;QuitFlag:	dc.w -1				; running
;ScreenCnt:	dc.w 0

;_GenieScreen:	dc.l 0
;_VisualInfo:	dc.l 0
;_IntuiMessage:	dc.l 0
;_FileReq:	dc.l 0
;_ModuleHandle:	dc.l 0

;_AslBase:	dc.l 0
;_DOSBase:	dc.l 0
;_GadToolsBase:	dc.l 0
;_IntuitionBase:	dc.l 0
;_UtilityBase:	dc.l 0


;§ *******************************************************************
;
; Always set LN_PRI to 1 or the specific comverter won't be called !
; (it controls the enable/disable status for this particular converter)
;
; Each node structure is followd by two pointers:
;
; The first is a pointer to a checkroutine.
;
; in:	d0.l contains the module size
;  	a0.l contains a pointer to the module start
; 
; out:	d0.l size of converted module or 0 if not recognized
; 
; Notes: There is no need to save any registers :-)
; This function may not change anything in the module!
;
;
; The second is a pointer to the real conversion routine
;
; in:	d0.l module size 
;	a0.l pointer to the module to convert
;	a1.l pointer to destination memory
;	
; out:	d0.l 0 if success, else -1
;
; Notes: There is no need to save any registers :-)
;


**********************************************************************
*                          Converter-Liste                           *
**********************************************************************

ConverterList:
		dc.l ConvNode00			; LH_HEAD
		dc.l 0				; LH_TAIL
		dc.l ConvNode38			; LH_TAILPRED
		dc.b 0				; LH_TYPE
		dc.b 0				; LH_pad
ConvNode00:
		dc.l ConvNode01			; LN_SUCC
		dc.l ConverterList		; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l KRISName			; LN_NAME
		dc.l CheckKRIS
		dc.l ConvertKRIS
ConvNode01:
		dc.l ConvNode02			; LN_SUCC
		dc.l ConvNode00			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l NoisePack2Name		; LN_NAME
		dc.l CheckNP20
		dc.l ConvertNP20
ConvNode02:
		dc.l ConvNode03			; LN_SUCC
		dc.l ConvNode01			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l NoisePack3Name		; LN_NAME
		dc.l CheckNP30
		dc.l ConvertNP30
ConvNode03:
		dc.l ConvNode04			; LN_SUCC
		dc.l ConvNode02			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l DigIllName			; LN_NAME
		dc.l CheckPinb
		dc.l ConvertPinb
ConvNode04:
		dc.l ConvNode05			; LN_SUCC
		dc.l ConvNode03			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PhaName			; LN_NAME
		dc.l CheckPha
		dc.l ConvertPha
ConvNode05:
		dc.l ConvNode06			; LN_SUCC
		dc.l ConvNode04			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l UnicName			; LN_NAME
		dc.l CheckUnic
		dc.l ConvertUnic

ConvNode06:
		dc.l ConvNode07			; LN_SUCC
		dc.l ConvNode05			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l LaxName			; LN_NAME
		dc.l CheckLax
		dc.l ConvertLax

ConvNode07:
		dc.l ConvNode08			; LN_SUCC
		dc.l ConvNode06			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l WantonName			; LN_NAME
		dc.l CheckWanton
		dc.l ConvertWanton

ConvNode08:
		dc.l ConvNode09			; LN_SUCC
		dc.l ConvNode07			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l NoiseRunName		; LN_NAME
		dc.l CheckNRU
		dc.l ConvertNRU

ConvNode09:
		dc.l ConvNode10			; LN_SUCC
		dc.l ConvNode08			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l EurekaName			; LN_NAME
		dc.l CheckEureka
		dc.l ConvertEureka

ConvNode10:
		dc.l ConvNode10b		; LN_SUCC
		dc.l ConvNode09			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l P4xxName			; LN_NAME
		dc.l CheckP4xx
		dc.l ConvertP4xx

ConvNode10b:
		dc.l ConvNode11			; LN_SUCC
		dc.l ConvNode10			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l P50AName			; LN_NAME
		dc.l CheckP50A
		dc.l ConvertP50A

ConvNode11:
		dc.l ConvNode12			; LN_SUCC
		dc.l ConvNode10			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PRU1Name			; LN_NAME
		dc.l CheckPRU1
		dc.l ConvertPRU1

ConvNode12:
		dc.l ConvNode13			; LN_SUCC
		dc.l ConvNode11			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PRU2Name			; LN_NAME
		dc.l CheckPRU2
		dc.l ConvertPRU2

ConvNode13:
		dc.l ConvNode14			; LN_SUCC
		dc.l ConvNode12			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PP10Name			; LN_NAME
		dc.l CheckPP10
		dc.l ConvertPP10

ConvNode14:
		dc.l ConvNode15			; LN_SUCC
		dc.l ConvNode13			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PP21Name			; LN_NAME
		dc.l CheckPP21
		dc.l ConvertPP21

ConvNode15:
		dc.l ConvNode16			; LN_SUCC
		dc.l ConvNode14			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PM10Name			; LN_NAME
		dc.l CheckPM10
		dc.l ConvertPM10

ConvNode16:
		dc.l ConvNode17			; LN_SUCC
		dc.l ConvNode15			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PM20Name			; LN_NAME
		dc.l CheckPM20
		dc.l ConvertPM20

ConvNode17:
		dc.l ConvNode18			; LN_SUCC
		dc.l ConvNode16			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PM40Name			; LN_NAME
		dc.l CheckPM40
		dc.l ConvertPM40

ConvNode18:
		dc.l ConvNode19			; LN_SUCC
		dc.l ConvNode17			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l FCMName			; LN_NAME
		dc.l CheckFCM
		dc.l ConvertFCM

;ConvNode19:
;		dc.l ConvNode20			; LN_SUCC
;		dc.l ConvNode18			; LN_PRED
;		dc.b 0				; LN_TYPE
;		dc.b 1				; LN_PRI
;		dc.l CryptoName			; LN_NAME
;		dc.l CheckCrypto
;		dc.l ConvertCrypto

ConvNode19:
		dc.l ConvNode20			; LN_SUCC
		dc.l ConvNode18			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l GMCName			; LN_NAME
		dc.l CheckGMC
		dc.l ConvertGMC

ConvNode20:
		dc.l ConvNode21			; LN_SUCC
		dc.l ConvNode19			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l XannName			; LN_NAME
		dc.l CheckXann
		dc.l ConvertXann

ConvNode21:
		dc.l ConvNode22			; LN_SUCC
		dc.l ConvNode20			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l SKYTName			; LN_NAME
		dc.l CheckSKYT
		dc.l ConvertSKYT

ConvNode22:
		dc.l ConvNode23			; LN_SUCC
		dc.l ConvNode21			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l GVName			; LN_NAME
		dc.l CheckGV
		dc.l ConvertGV

ConvNode23:
		dc.l ConvNode24			; LN_SUCC
		dc.l ConvNode22			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l CryptoName			; LN_NAME
		dc.l CheckCrypto
		dc.l ConvertCrypto


;ConvNode23:
;		dc.l ConvNode24			; LN_SUCC
;		dc.l ConvNode22			; LN_PRED
;		dc.b 0				; LN_TYPE
;		dc.b 1				; LN_PRI
;		dc.l GMCName			; LN_NAME
;		dc.l CheckGMC
;		dc.l ConvertGMC

ConvNode24:
		dc.l ConvNode25			; LN_SUCC
		dc.l ConvNode23			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l AvalonName			; LN_NAME
		dc.l CheckAvalon
		dc.l ConvertAvalon

ConvNode25:
		dc.l ConvNode26			; LN_SUCC
		dc.l ConvNode24			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l AC1DName			; LN_NAME
		dc.l CheckAC1D
		dc.l ConvertAC1D

ConvNode26:
		dc.l ConvNode27			; LN_SUCC
		dc.l ConvNode25			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PygmyName			; LN_NAME
		dc.l CheckPygmy
		dc.l ConvertPygmy

ConvNode27:
		dc.l ConvNode28			; LN_SUCC
		dc.l ConvNode26			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l ChanName			; LN_NAME
		dc.l CheckChan
		dc.l ConvertChan

ConvNode28:
		dc.l ConvNode29			; LN_SUCC
		dc.l ConvNode27			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l StarName			; LN_NAME
		dc.l CheckStar
		dc.l ConvertStar

ConvNode29:
		dc.l ConvNode30			; LN_SUCC
		dc.l ConvNode28			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l FuzzacName			; LN_NAME
		dc.l CheckFuzzac
		dc.l ConvertFuzzac

ConvNode30:
		dc.l ConvNode31			; LN_SUCC
		dc.l ConvNode29			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l OldKefName			; LN_NAME
		dc.l CheckOldKef
		dc.l ConvertOldKef

ConvNode31:
		dc.l ConvNode32			; LN_SUCC
		dc.l ConvNode30			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l ST26Name			; LN_NAME
		dc.l CheckST26
		dc.l ConvertST26

ConvNode32:
		dc.l ConvNode32b		; LN_SUCC
		dc.l ConvNode31			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l TP1Name			; LN_NAME
		dc.l CheckTP1
		dc.l ConvertTP1

ConvNode32b:
		dc.l ConvNode33			; LN_SUCC
		dc.l ConvNode32			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l TP3Name			; LN_NAME
		dc.l CheckTP3
		dc.l ConvertTP3

ConvNode33:
		dc.l ConvNode34			; LN_SUCC
		dc.l ConvNode32			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l NTPKName			; LN_NAME
		dc.l CheckNTPK
		dc.l ConvertNTPK

ConvNode34:
		dc.l ConvNode35			; LN_SUCC
		dc.l ConvNode33			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PolkaName			; LN_NAME
		dc.l CheckPolka
		dc.l ConvertPolka

ConvNode35:
		dc.l ConvNode36			; LN_SUCC
		dc.l ConvNode34			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l PowerMusicName		; LN_NAME
		dc.l CheckPowerMusic
		dc.l ConvertPowerMusic

ConvNode36:
		dc.l ConvNode37			; LN_SUCC
		dc.l ConvNode35			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l STP3Name			; LN_NAME
		dc.l CheckSTP3
		dc.l ConvertSTP3

ConvNode37:
		dc.l ConvNode38			; LN_SUCC
		dc.l ConvNode36			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l ZenName			; LN_NAME
		dc.l CheckZen
		dc.l ConvertZen

ConvNode38:
		dc.l ConverterList+4		; LN_SUCC
		dc.l ConvNode37			; LN_PRED
		dc.b 0				; LN_TYPE
		dc.b 1				; LN_PRI
		dc.l HRTName			; LN_NAME
		dc.l CheckHRT
		dc.l ConvertPRU1



**********************************************************************
*                               Font                                 *
**********************************************************************

;TOPAZ80:
;	dc.l	TOPAZname
;	dc.w	TOPAZ_EIGHTY
;	dc.b	$00,$01


**********************************************************************
*                   Table with all known Prefixes                    *
**********************************************************************

PrefixTable:
  dc.l "???","ac1d","aval","chan","cp","cplx","crb","di","eu","fcm","fp"
  dc.l "fuzz","gmc","gv","hmc","hrt","hrt!","ice","kef","kef7","kris","ksm"
  dc.l "lax","mexx","mod","mpro","np","np1","np2","np3","nr","nru","ntpk"
  dc.l "p40","p40a","p40b","p41","p41a","p50","p50a","p60","p60a","p61","p61a"
  dc.l "pin","pha","polk","pwr","prom","pm20","pm40","pm","!pm!"
  dc.l "pp","pp10","pp20","pp21","pp30","pr","pr1","pr2"
  dc.l "pru","pru1","pru2","pyg","pygm","skyt","snt","snt!"
  dc.l "st26","st30","star","stpk","stp3","tp","tp1","tp2","tp3"
  dc.l "unic","wn","wp","xann","zen","zik"
  dc.l 0						;terminator



**********************************************************************
*                        verschiedene Texte                          *
**********************************************************************

AboutText	dc.b "                  Deli-Wizard",10
		dc.b "          © 1994-95 by Nicolas FRANCK",10,0
;		dc.b 10
;		dc.b "     THIS PIECE OF SOFTWARE IS SHAREWARE!",10
;		dc.b "   If you are using this program frequently",10
;		dc.b "      please send US $15 / DM 20 / FF 50",10
;		dc.b "          to the following address :",10
;		dc.b 10
;		dc.b "                Nicolas FRANCK",10
;		dc.b "            157, Rue de Strasbourg",10
;		dc.b "            77350 Le Mée sur Seine",10
;		dc.b "            ------- FRANCE -------",10
;		dc.b 10
;		dc.b " Then, you'll become a registered user of both",10
;		dc.b "        Pro-Wizard-2 and Deli-Wizard !",0


DigIllName:	dc.b '  Digital Illusions',0
KRISName:	dc.b '  KRIS Tracker',0
UnicName:	dc.b '  Unic Tracker',0
LaxName:	dc.b '  Laxity Tracker',0
NoisePack2Name:	dc.b '  Noise Packer 1.0 - 2.0',0
NoisePack3Name:	dc.b '  Noise Packer 3.0',0
NoiseRunName:	dc.b '  Noise Runner',0
EurekaName:	dc.b '  Eureka Packer',0
P4xxName:	dc.b '  P40A - P40B - P41A',0
P50AName:	dc.b '  P50A - P60A - P61A',0
PM10Name:	dc.b '  Promizer 1.x',0
PM20Name:	dc.b '  Promizer 2.0',0
PM40Name:	dc.b '  Promizer 4.0',0
PP10Name:	dc.b '  ProPacker 1.0',0
PP21Name:	dc.b '  ProPacker 2.1 - 3.0',0
PRU1Name:	dc.b '  ProRunner 1.0',0
PRU2Name:	dc.b '  ProRunner 2.0',0
WantonName:	dc.b '  Wanton Packer',0
PhaName:	dc.b '  Pha Packer',0
FCMName:	dc.b '  FC-M Packer',0
CryptoName:	dc.b '  HeatSeeker mc1.0',0
XannName:	dc.b '  Xann Player',0
SKYTName:	dc.b '  SKYT Packer',0
GVName:		dc.b '  Module Protector',0
GMCName:	dc.b '  Game Music Creator',0
AvalonName:	dc.b '  Promizer 0.1',0
AC1DName:	dc.b '  AC1D Packer',0
PygmyName:	dc.b '  Pygmy Packer',0
ChanName:	dc.b '  Channel Players',0
StarName:	dc.b '  StarTrekker Packer',0
FuzzacName:	dc.b '  Fuzzac Packer',0
OldKefName:	dc.b '  Kefrens Sound Machine',0
ST26Name:	dc.b '  STK_2.6 - IceTracker',0
TP1Name:	dc.b '  Tracker Packer 1',0
TP3Name:	dc.b '  Tracker Packer 2 - 3',0
NTPKName:	dc.b '  NoiseTracker Pak',0
PolkaName:	dc.b '  Polka Packer',0
PowerMusicName: dc.b '  Power Music',0
STP3Name:	dc.b '  SoundTracker Pro 3.0',0
ZenName:	dc.b '  Zen Packer',0
HRTName:	dc.b '  Hornet Packer',0


;TOPAZname	dc.b 'topaz.font',0
;utilityname	dc.b 'utility.library',0


*-----------------------------------------------------------------------*

	SECTION Buffers,BSS

P50A_Sinfo:
	ds.l	3*31		; offset, len+vol, repeat+replen / each sample
	ds.l	1
KR_PosTable:
	ds.b	$80
	ds.l	1
PP21_Table:
	ds.b	$80*4		; copie les tracks
	ds.l	1



