
mod_fail=-1
mod_DOC=1
mod_MST=2
mod_STIV=3
mod_UST=4

mod_PTK_comp=32
mod_PTK=33
mod_PTK_vblank=34
mod_NTK_2=35
mod_NTK_1=36
mod_NTK_AMP=37
mod_STK=38
mod_FLT4=39
mod_FLT8=40
mod_ADSC4=41
mod_ADSC8=42
mod_FTK=43

query_eagleopts:
	tst.l	uadebase
	beq.w	.no_uade_options

	move.l	uadebase,a6
	lea	eagleoptlist,a0
	jsr	-24(a6)
	beq.w	.no_uade_options
	moveq	#0,d0
	rts

.no_uade_options
	moveq	#-1,d0
	rts

eagleoptlist	dc.l	1,typename,typenamereceived,typestr	* string
		dc.l	3,vblankname,vblankflag,-1		* flag
		dc.l	0

typestr		dcb.b	32,0
typename	dc.b	'type',0
vblankname	dc.b	'vblank',0
vblankflag	dc.b	0
typenamereceived	dc.b	0
		even

******************************************************************************

mcheck_moduledata:	; Current implemation is a hack for uade only.
			movem.l	d1-d7/a0-a6,-(a7)
			move.l	song,a0
			move.l	size,d1

			move.w	#MyVarsEnd-MyVars-1,d0
			lea.l	MyVars(pc),a1
.cls_myvar:		clr.b	(a1)+
			dbra	d0,.cls_myvar


			move.l	#1084,d0
			bsr	mcheck_calc_modlen

.mod32:			bsr mcheck_which_mk
			bra	.mcheck_passed


.mcheck_passed:		move.l	modtag,d0
			lea.l	modtag,a0
.mcheck_end:		movem.l	(a7)+,d1-d7/a0-a6
			rts

mcheck_which_mk:
			move.l	song,a0
			bsr	ParseEffects
mcheck_is_ptk:
			; Check for vblank by playtime
			; in Protracker modules 
			moveq.l	#0,d0
			move.l	#25000/50,d1		; 50Hz
			moveq.l	#1,d2
			move.l	song,a0
			lea.l	Timer,a1
			bsr	PTCalcTime
			move.l #mod_PTK_vblank,modtag
			lea.l	Timer,a1
			move.l	(a1),d0
			cmp.l	#0,d0			; playtime in hours?
			bgt	.mcheck_end
			move.l	4(a1),d0
			cmp.l	#20,d0			; more than 20 minutes?
			bgt	.mcheck_maybe_vblank
			move.l #mod_PTK,modtag
			rts
.mcheck_maybe_vblank:
			moveq.l	#0,d0
			move.l	#25000/50,d1		; 50Hz
			moveq.l	#0,d2			; vblank
			move.l	song,a0

			lea.l	Timer2,a1
			bsr	PTCalcTime

			lea.l	Timer2,a1
			move.l	4(a1),d1		; vblank minutes
			mulu	#2,d1

			lea.l	Timer,a1
			move.l	4(a1),d0			; cia minutes
			
			cmp.l	d1,d0			; more than double?
			bgt	.mcheck_end
			move.l #mod_PTK,modtag
.mcheck_end		rts

;--------------------------------------------------------------------------
; Calculate Modlen
; Arguments:
;       a0.l = pointer to module data
;       d0.l = 1084(mod32) or 600 (mod16)
;	d1.l = file length
;
; returns:
;	d0 = status (-1 too short, 0 = len ok, 1 = too long)
;

mcheck_calc_modlen:	cmp.l	d0,d1
			bhi.b	.is_higher
			moveq	#-1,d0
			rts

.is_higher		movem.l	d1-d7/a0-a6,-(a7)

        		move.l	d0,header
			cmp.l	#1084,d0		;header size
			beq	.mcheck_32instr

			move.l	#472,d2			;d2 = plist
			moveq	#14,d3			;d3 no of Instruments 	
			bra	.mcheck_calc_start

.mcheck_32instr:	move.l	#952,d2
			moveq	#30,d3

			;--- Get Maxpattern ---
.mcheck_calc_start:
			lea.l	(a0,d2),a5		; plist
			moveq	#127,d4
			moveq	#0,d5
.mcheck_loop:		
			cmp.b	(a5)+,d5
			bge.s	.mcheck_loop2
			move.b	-1(a5),d5
.mcheck_loop2:
			dbra	d4,.mcheck_loop
			addq.b	#1,d5			; maxpattern
			move.w	d5,maxpattern
			
			;--- Calc Instruments ---

			move.l	a0,a5
			asl.l	#8,d5
			asl.l	#2,d5
			add.l	header,d5
.mcheck_loop3:		moveq	#0,d6
			move.w	42(a5),d6
			asl.l	#1,d6
			add.l	d6,d5
			add.l	#30,a5
			dbra	d3,.mcheck_loop3

			;--- Check file len ---
			cmp.l	d1,d5
			bgt.b	.mcheck_too_short	; file too short

			sub.l	d5,d1
			cmp.l	#1024,d1		; filesize +4KB is ok, kind of
			
			bgt	.mcheck_too_long

			; good length
			moveq	#0,d0
			bra	.mcheck_end			

			; being too short means a failure
.mcheck_too_short:	;lea tooshortmsg(pc),a0
			;bsr	uade_debug
			moveq	#-1,d0
			bra.b	.mcheck_end
	
	                ; being long is not a failure, but a reason for warning
			; this is debatable! -- mld
.mcheck_too_long:	;lea	toolongmsg(pc),a0
			;bsr	uade_debug
			moveq	#1,d0
.mcheck_end:		movem.l	(a7)+,d1-d7/a0-a6
	    		rts

;toolongmsg:		dc.b	'The module file is too long.',0
;tooshortmsg:		dc.b	'The module file is too short.',0
;			even

;------------------------------------------------------------------------------
;
;	$VER: PTCalcTime v1.1 - by Håvard "Howard" Pedersen
;	© 1994-96 Mental Diseases
;
;	A program for calculating the playtime of ProTracker modules.
;
;	I cannot be held responsible for any damage caused directly or in-
;	directly by this code. Still, every released version is thouroughly
;	tested with Mungwall and Enforcer, official Commodore debugging tools.
;	These programs traps writes to unallocated ram and reads/writes to/from
;	non-ram memory areas, which should cover most bugs.
;
;	HISTORY:
;
;v1.0	Simple thingy with bugs and quirks. Did calculating using IRQ/second.
;
;v1.1	Bugfix version. Finally usable.
;	* Pattern loop wasn't sensed properly. By some reason I used effect
;	  command E5 for set loop! (?)
;	* Entire pattern loop code was broken. Recoded, does now work correctly
;	  with "mod.couldbe".
;	* Uses 1/25000th second between each interrupt for timing. Much more
;	  accurate in CIA mode.
;	* Small speedups here and there.
;
;------------------------------------------------------------------------------
;
;	PUBLIC FUNCTIONS:
;
;Function:	PTCalcTime(SongPos,delay,CIA(BOOL),Module,TimerStruct)
;		(D0,D1,D2,A0,A1)
;Purpose:	Calculates exact playtime of a ProTracker module.
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;			PTCALCTIME
;------------------------------------------------------------------------------
; D0 - Local use
; D1 - 25000th of a second delay between each IRQ
; D2 - Local use
; D3 - Pattern delay count
; D4 - Local use
; D5 - 
; D6 - 
; D7 - Voiceloop count
; A0 - Module
; A1 - TimerStruct
; A2 - PatternPosition
; A3 - Local use
; A4 - 
; A5 - 
; A6 - 

* BUGGY !!! Does not work right with mod.ode2ptk *sigh*

PTCalcTime	move.l	#6,.Speed
		move.l	#0,.PattPos
		move.l	d0,.SongPos
		move.l	#0,.PattLoopPos
		move.l	#0,.PosJumpPos
		move.b	d2,.CIAFlag

.MainLoop	lea.l	952(a0),a2		; Get position
		add.l	.SongPos,a2
		moveq.l	#0,d0
		move.b	(a2),d0			; Get pattern at current pos
		lsl.l	#8,d0			; *1024
		lsl.l	#2,d0
		lea.l	1084(a0),a2
		add.l	d0,a2			; Address for pattern

.StepLoop	lea.l	952(a0),a2		; Get position
		add.l	.SongPos,a2
		moveq.l	#0,d0
		move.b	(a2),d0			; Get pattern at current pos
		lsl.l	#8,d0			; *1024
		lsl.l	#2,d0
		lea.l	1084(a0),a2
		add.l	d0,a2			; Address for pattern

		move.l	.PattPos,d0
		lsl.l	#4,d0
		add.l	d0,a2

		moveq.l	#4-1,d7			; Loop
.VoiceLoop	lea.l	.CmdsTab,a3
		move.l	(a2),d0			; Get stuff
		and.l	#$00000ff0,d0		; Get command

		move.l	d0,d2
		and.l	#$00000f00,d2
		cmp.l	#$00000e00,d2		; Misc cmds?
		beq.s	.TabLoop

		and.l	#$00000f00,d0

.TabLoop	cmp.l	#-1,(a3)
		beq.s	.NoneFound

		cmp.l	(a3),d0
		bne.s	.NoMatch

		move.l	4(a3),a3
		jsr	(a3)
		bra.s	.NoneFound

.NoMatch	addq.l	#8,a3
		bra.s	.TabLoop

.NoneFound	addq.l	#4,a2
		dbf	d7,.VoiceLoop

		bsr.w	.AddSpeed

		addq.l	#1,.PattPos

		tst.b	.BreakFlag
		bne.s	.NewPos

		cmp.l	#64,.PattPos
		bne.w	.StepLoop

.NewPos		move.b	#0,.BreakFlag

		move.l	.PattBreakPos,.PattPos
		move.l	#0,.PattBreakPos	; Default pattern break pos

		tst.l	.PosJumpPos
		beq.s	.NoPosJump

		move.l	.PosJumpPos,.SongPos
		move.l	#0,.PosJumpPos
		bra.s	.EndIt

.NoPosJump	add.l	#1,.SongPos
		move.l	.SongPos,d0
		lea.l	950(a0),a3
		cmp.b	(a3),d0
		blo.w	.MainLoop

.EndIt		move.l	12(a1),d0
		divu.w	#250,d0
		and.l	#$ffff,d0
		move.l	d0,12(a1)		; Convert to 100/s.

		rts

.AddSpeed	move.l	.Speed,d0
		subq.l	#1,d0
.SpeedLoop	add.l	d1,12(a1)
		dbf	d0,.SpeedLoop
.AddSpeedLoop	cmp.l	#25000,12(a1)
		blo.s	.OkIRQs
		sub.l	#25000,12(a1)
		add.l	#1,8(a1)
.OkIRQs		cmp.l	#60,8(a1)
		blo.s	.OkSecs
		sub.l	#60,8(a1)
		add.l	#1,4(a1)
.OkSecs		cmp.l	#60,4(a1)
		blo.s	.OkMins
		sub.l	#60,4(a1)
		add.l	#1,(a1)
.OkMins		rts

.CmdsTab	dc.l	$00000b00,._PosJump
		dc.l	$00000d00,._PattBreak
		dc.l	$00000f00,._SetSpeed
		dc.l	$00000e60,._PatLoop
		dc.l	$00000ee0,._PatDelay
		dc.l	-1,-1

._PosJump	move.l	(a2),d0			; Get stuff
		and.l	#$ff,d0
		move.l	d0,.PosJumpPos
		move.b	#-1,.BreakFlag
		rts

._PattBreak	move.l	(a2),d0			; Get stuff
		and.l	#$ff,d0
		move.l	d0,.PattBreakPos
		move.b	#-1,.BreakFlag
		rts

._SetSpeed	move.l	(a2),d0			; Get stuff
		and.l	#$ff,d0
		beq.s	.Halt

		tst.b	.CIAFlag
		beq.s	.VBL

		cmp.b	#$20,d0
		blo.s	.VBL

		; Do some CIA->Hz converting!
		move.l	#62500,d1
		divu.w	d0,d1
		and.l	#$ffff,d1

		rts

.VBL		move.l	d0,.Speed
		rts

.Halt		move.l	#-1,.PosJumpPos		; Halt module
		move.b	#-1,.BreakFlag
		rts

._PatLoop	rts				; temporarily disable pt_loop
						; because of ode2ptk...

		move.l	(a2),d0			; Get stuff
		and.l	#$f,d0
		tst.l	d0
		beq.s	.SetLoop

		tst.l	.PattLoopCnt
		beq.s	.SetLoopCnt


		subq.l	#1,.PattLoopCnt
		bne.s	.DoLoop

		rts

.SetLoop
		move.l	.PattPos,.PattLoopPos
		rts

.SetLoopCnt
		move.l	d0,.PattLoopCnt

.DoLoop		move.l	.PattLoopPos,.PattBreakPos; Force loop
		sub.l	#1,.SongPos
		move.b	#-1,.BreakFlag

		rts

.PattLoopIt	subq.l	#1,.PattLoopCnt
		tst.l	.PattLoopCnt
		beq.s	.Return

		move.l	.PattLoopPos,.PattPos
.Return		rts

._PatDelay	move.l	(a2),d0			; Get stuff
		and.l	#$f,d0
		tst.l	d0
		beq.s	.PatDelNo
		subq.l	#1,d0
		move.l	d0,d3
.PatDelLoop	bsr.w	.AddSpeed
		dbf	d3,.PatDelLoop
.PatDelNo	rts

.BreakFlag	dc.b	0
.CIAFlag	dc.b	0
	EVEN
.Speed		dc.l	0
.PattPos	dc.l	0
.SongPos	dc.l	0
.PattLoopPos	dc.l	0
.PattLoopCnt	dc.l	0
.PosJumpPos	dc.l	0
.PattBreakPos	dc.l	0

;--------------------------------------------------------------------------
; parse effects
;
; Input a0 = pointer to Module
; 	(d0 = no of instruments, to be done yet)
;

ParseEffects:
		movem.l	d1-d7/a0-a6,-(a7)
		lea.l	1084(a0),a0

		move.w	maxpattern,d0
		beq	.do_calc
		subq.w	#1,d0
.do_calc
		lea.l	pfx(pc),a1
.loop1:		move.w	#255,d1			; 1024 bytes
.loop2:		move.w	2(a0),d2
		and.w	#$0fff,d2
		beq	.next1			; no fx
		lsr.w	#8,d2
		cmp.w	#15,d2			; fxy?
		bne	.no_speedfx		;

		moveq	#0,d3
		move.b	3(a0),d3
		cmp.w	#$1f,d3			; Speed > $1f
		ble	.setSpeed

.setBPM		move.w	#14,d2			; Set Speed 	-> 0xf
		bra	.addpfx
.setSpeed
		move.w	#15,d2			; Set BPM 	-> 0xe
		bra	.addpfx

.no_speedfx:	cmp.w	#14,d2			; Exy ?
		bne	.addpfx			; nope, normal one 0 - 15
		moveq	#0,d2
		move.b	3(a0),d2
		lsr.w	#4,d2
		add.w	#16,d2			; Exy 		-> 16 to 32
		cmp.w	#16,d2			; Protracker cmds used?
		ble	.addpfx
		st	extended_fx_used

.addpfx		add.w	d2,d2			; word offset to pfx
		addq.w	#1,(a1,d2.w)
.next1:		add.l	#4,a0
		dbf	d1,.loop2
		dbf	d0,.loop1
				
.end		movem.l	(a7)+,d1-d7/a0-a6
		rts
;--------------------------------------------------------------------------
; mod_SubSongRange
;
mod_SubSongRange:
	        moveq	#1,d0
	        move.l	SubSongs,d1
	        rts

;--------------------------------------------------------------------------
; mod_probe_subsongs
; input:  a0=Songdata  
;
; FIXME: reports way too many Subsongs for songs playing backwards :(

mod_probe_subsongs:
		lea.l	pfx(pc),a1
		move.w	#127,d0
		cmp.w	$0b*2(a1),d0		; Hack for weird some mods
		bgt	.probe_subs		; using posjmp to go beserk :)
		move.l	#1,SubSongs
		rts

.probe_subs
		moveq	#0,D0
		lea	952(A0),A1
		suba.w	pt_seqadj(pc),a1	; adjust for 15instr
		lea	1084(A0),A2
		suba.w	pt_blkadj(pc),a2	; adjust for 15instr
		lea	950(a0),a3
		suba.w	pt_seqadj(pc),a3	; adjust for 15instr
		moveq	#0,d2
		move.b	(a3),d2
		subq	#1,d2

		lea	SongTable+1(PC),A6

		moveq	#0,D3
		moveq	#0,D4
		moveq	#0,D5
NextPos
		moveq	#0,D1
		move.b	(A1)+,D1
		lsl.l	#8,D1
		lsl.l	#2,D1
		lea	(A2,D1.L),A0
		lea	1024(A0),A3
		addq.l	#2,A0
NextPatPos
    		move.b	(A0),D1
		move.b	1(a0),d3
		and.b	#$0F,D1
		cmp.b	#$0e,d1
		bne	.chkposjmp
		
.chkposjmp
		cmp.b	#$0B,D1
		beq.b	SubFound
noSubfound:	addq.l	#4,A0
		cmp.l	A0,A3
		bgt.b	NextPatPos
		addq.l	#1,D4			; count pattern one up
back
		dbf	D2,NextPos

		tst.l	d0
		bne.b	LoopEnd
		moveq	#1,d0
LoopEnd:	    
		;move.w	maxpattern,d1
		;cmp.w	d0,d1		; another Hack for weird some mods
		;bgt	.sub_ok		; using posjmp to go beserk :)
		;move.l	#1,SubSongs
		;rts
.sub_ok		move.l	d0,SubSongs
		rts



SubFound	; d1 = fx
		; d3 = fxarg
		; d4 = position counter
		
		move.b	1(a0),d3	; fxarg

		move.w	d4,d5
		addq.w	#1,d5

		cmp.w	d5,d3		; jump ahead
		bgt	noSubfound

		move.w	d0,d3

		lea.l	SongTable+1(PC),a5
.loop		cmp.w	(a5,d3.w),d5	; no doubles
		beq	noSubfound
		dbra	d3,.loop
		
.subsong_found
		addq.l	#1,D4
		move.b	D4,(A6)+	; set starting position
		addq.l	#1,D0		; num of subsongs ++
		bra.b	back		; next one


;--------------------------------------------------------------------------
; mod_set_subsong
;
mod_set_subsong:

		    move.l	delibase(pc),a5
		    move.w	dtg_SndNum(a5),d0
		    tst.w	d0
		    beq.b	.s
		    subq.w	#1,d0	
.s
		    lea.l	SongTable(pc),a5
		    move.b	(a5,d0.w),d0
		    ext.l	d0
		    move.l	d0,pt_songposition
		    rts

open_uade_library:
		move.l	4.w,a6
		lea	uadename(pc),a1
		moveq	#0,d0
		jsr	-552(a6)
		move.l	d0,uadebase
		rts

uade_debug:	movem.l	d0-d7/a0-a6,-(a7)
		move.l	uadebase,d0
		beq.b	.nouadebase
		move.l	d0,a6
		* Message string in A0
		jsr	-12(a6)
.nouadebase:	movem.l (a7)+,d0-d7/a0-a6
		rts

;--------------------------------------------------------------------------
; Datas:
;Instrument flags

uadebase:		dc.l	0
uadename:		dc.b	"uade.library",0
			even
;--------
MyVars:
Subsongs:		dc.l	0
SongTable:		ds.b	128
			
maxpattern:		dc.w	0
Timer:			dc.l	0,0,0,0			; Hours, Minutes, secs
Timer2:			dc.l	0,0,0,0			; Hours, Minutes, secs
header:			dc.l	0
modtag:			dc.l	0
pfx			dcb.w	32,0
pfxarg			dcb.w	32,0

extended_fx_used	dc.b	0
repeat_in_bytes_used	dc.b	0	
finetune_used:		dc.b	0

;--------
MyVarsEnd:
			even
