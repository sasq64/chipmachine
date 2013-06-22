;               T
*
* UADE sound core
* Copyright: Heikki Orsila <heikki.orsila@iki.fi>
* License: GNU LGPL
* (relicensing is very possible for open source development reasons)
*
	include	custom.i
	include	exec_lib.i
	include	graphics_lib.i
	include	dos_lib.i
	include	rmacros.i

	include	np.i

	incdir	include:
	include misc/deliplayer.i
	include misc/eagleplayer.i

	include	resources/cia.i
	include	lvo3.0/cia_lib.i
	include	devices/timer.i


UADE_SETSUBSONG	equ	1
UADE_SONG_END	equ	2
UADE_PLAYERNAME	equ	3
UADE_MODULENAME	equ	4
UADE_SUBSINFO	equ	5
UADE_CHECKERROR	equ	6
UADE_SCORECRASH	equ	7
UADE_SCOREDEAD	equ	8
UADE_GENERALMSG	equ	9
UADE_NTSC	equ	10
UADE_FORMATNAME	equ	11
UADE_LOADFILE	equ	12
UADE_READ	equ	13
UADE_FILESIZE	equ	14
UADE_TIME_CRITICAL	equ	15
UADE_GET_INFO	equ	16
UADE_START_OUTPUT	equ	17

EXECBASE	equ	$0D00
EXECENTRIES	equ	210

TRAP_VECTOR_0	equ	$80
TRAP_VECTOR_3	equ	$8c	* exec_cause uses this for software interrupt
TRAP_VECTOR_4	equ	$90	* play loop uses this for vbi sync
TRAP_VECTOR_5	equ	$94	* output message trap
TRAP_VECTOR_6	equ	$98	* bin trap

* uade.library interface
UadeTimeCritical	equ	-6
UadePutString		equ	-12
UadeGetInfo		equ	-18
UadeNewGetInfo		equ	-24

EP_OPT_SIZE		equ	256
	
* $100	mod address
* $104	mod length
* $108	player address
* $10c	reloc address (the player)
* $110	user stack
* $114	super stack
* $118	force by default
* $11C	setsubsong flag (only meaningful on amiga reboot)
* $120	subsong
* $124	ntsc flag	0 = pal, 1 = ntsc
* $128	loaded module name ptr
* $12C	song end bit
* $180	postpause flag
* $184	prepause flag
* $188	delimon flag
* $18C	Exec debug flag (enabled if $18c == 0x12345678)
* $190	volume test flag (enabled if $190 == 0x12345678)
* $194	dma wait constant (number of raster lines + 0x12340000)
* $198	disable EP_ModuleChange

* $200	output message flag + output message

* $300	input message flag + input message

* $400	module (player) name path

* $0800 -
* $0D00	exec base (-6*EXECENTRIES == -6 * 210)

	section	uadesoundcore,code_c
	illegal

start	* set super stack and user stack
	move.l	$114,a7
	move.l	$110,a0
	move.l	a0,usp

	move	#$7fff,intena+custom
	move	#$7fff,intreq+custom
	move	#$7fff,dmacon+custom
	move.b	#3,$bfe201
	move.b	#2,$bfe001		* filter off

	move.b	#$1f,$bfdd00		* ciab ICR (disable all timers)
	move.b	#$00,$bfef01		* TOD counter rolling on vsync
	move.b	#$00,$bfe801		* order of these is important
	move.b	#$00,$bfdf00		* TOD counter rolling on hsync
	move.b	#$00,$bfd800		* order of these is important

	move	#$200,bplcon0+custom
	move	#$00ff,adkcon+custom

	* patch exception vectors
	move	#8,a0
	lea	excep(pc),a1
exloop	move.l	a1,(a0)+
	cmp.l	#$100,a0
	bne.b	exloop

	move	#0,sr			* switch to user level

	lea	zero_sample(pc),a0	* set zero sample
	lea	custom,a2
	moveq	#4-1,d7
zero_sample_l	clr	aud0vol(a2)
	move.l	a0,aud0lch(a2)
	move	#1,aud0len(a2)
	move	#150,aud0per(a2)
	add	#$10,a2
	dbf	d7,zero_sample_l

	lea	test_copperlist(pc),a0	* set blank copper list
	move.l	a0,cop1lch+custom
	move	d0,copjmp1+custom
	move	#$8280,dmacon+custom

	move	#$c000,intena+custom

	* initialize exec base with failures
	lea	EXECBASE,a0
	move.l	a0,4.w
	lea	exec_exception(pc),a2
	move	#EXECENTRIES-1,d7
dfdf	subq.l	#6,a0
	move	jmpcom(pc),(a0)
	move.l	a2,2(a0)
	dbf	d7,dfdf

	* initialize dosbase with failures
	lea	dos_lib_base(pc),a0
	lea	dosexception(pc),a2
	move	#200-1,d7
dosdfdf	subq.l	#6,a0
	move	jsrcom(pc),(a0)
	move.l	a2,2(a0)
	dbf	d7,dosdfdf

	move.l	4.w,a6
	lea	exec_set_int_vector(pc),a0
	move.l	a0,SetIntVector+2(a6)
	lea	exec_add_int_server(pc),a0
	move.l	a0,AddIntServer+2(a6)
	lea	exec_allocmem(pc),a0
	move.l	a0,AllocMem+2(a6)
	lea	myfreemem(pc),a0
	move.l	a0,FreeMem+2(a6)
	lea	exec_open_resource(pc),a0
	move.l	a0,OpenResource+2(a6)
	lea	exec_open_device(pc),a0
	move.l	a0,OpenDevice+2(a6)
	lea	exec_doio(pc),a0
	move.l	a0,DoIO+2(a6)
	lea	exec_sendio(pc),a0
	move.l	a0,SendIO+2(a6)
	lea	exec_waitio(pc),a0
	move.l	a0,WaitIO+2(a6)
	lea	myabortio(pc),a0
	move.l	a0,AbortIO+2(a6)
	lea	mygetmsg(pc),a0
	move.l	a0,GetMsg+2(a6)
	lea	exec_cause(pc),a0
	move.l	a0,Cause+2(a6)
	lea	exec_old_open_library(pc),a0
	move.l	a0,OldOpenLibrary+2(a6)
	lea	exec_open_library(pc),a0
	move.l	a0,OpenLibrary+2(a6)
	lea	exec_typeofmem(pc),a0
	move.l	a0,TypeOfMem+2(a6)
	lea	exec_allocsignal(pc),a0
	move.l	a0,AllocSignal+2(a6)
	lea	exec_signal(pc),a0
	move.l	a0,Signal+2(a6)
	lea	exec_supervisor(pc),a0
	move.l	a0,Supervisor+2(a6)
	lea	exec_superstate(pc),a0
	move.l	a0,SuperState+2(a6)
	lea	exec_userstate(pc),a0
	move.l	a0,UserState+2(a6)
	lea	exec_findtask(pc),a0
	move.l	a0,FindTask+2(a6)

	move.b	#$ff,$126(a6)	* fuck med player
	move	#$0003,$128(a6)	* execbase processor flags to 68020+

	lea	dos_lib_base(pc),a6
	lea	dos_loadseg(pc),a0
	move	jmpcom(pc),LoadSeg(a6)
	move.l	a0,LoadSeg+2(a6)
	lea	dos_open(pc),a0
	move	jmpcom(pc),Open(a6)
	move.l	a0,Open+2(a6)
	lea	dos_seek(pc),a0
	move	jmpcom(pc),Seek(a6)
	move.l	a0,Seek+2(a6)
	lea	dos_read(pc),a0
	move	jmpcom(pc),Read(a6)
	move.l	a0,Read+2(a6)
	lea	dos_close(pc),a0
	move	jmpcom(pc),Close(a6)
	move.l	a0,Close+2(a6)
	lea	dos_currentdir(pc),a0
	move	jmpcom(pc),CurrentDir(a6)
	move.l	a0,CurrentDir+2(a6)
	lea	dos_lock(pc),a0
	move	jmpcom(pc),Lock(a6)
	move.l	a0,Lock+2(a6)

	lea	uade_lib_base(pc),a6
	lea	uade_time_critical(pc),a0
	move	jmpcom(pc),UadeTimeCritical(a6)
	move.l	a0,UadeTimeCritical+2(a6)
	lea	put_string(pc),a0
	move	jmpcom(pc),UadePutString(a6)
	move.l	a0,UadePutString+2(a6)
	lea	uade_get_info(pc),a0
	move	jmpcom(pc),UadeGetInfo(a6)
	move.l	a0,UadeGetInfo+2(a6)
	lea	uade_new_get_info(pc),a0
	move	jmpcom(pc),UadeNewGetInfo(a6)
	move.l	a0,UadeNewGetInfo+2(a6)

	move.l	4.w,a6
	lea	rtsprog(pc),a1
	lea	harmlesslist(pc),a0
harmlessloop	move	(a0)+,d0
	beq.b	endharmlessloop
	move	jmpcom(pc),(a6,d0)
	move.l	a1,2(a6,d0)
	bra.b	harmlessloop
endharmlessloop
	lea	dos_lib_base(pc),a6
	lea	rtsnonzeroprog(pc),a1
	lea	harmlessdoslist(pc),a0
harmlessdosloop	move	(a0)+,d0
	beq.b	endharmlessdosloop
	move	jmpcom(pc),(a6,d0)
	move.l	a1,2(a6,d0)
	bra.b	harmlessdosloop
endharmlessdosloop
	bra	contplayer

rtsprog	rts
rtsnonzeroprog	moveq	#-1,d0
	rts

* exec library harmless function list (just do rts)
harmlesslist	dc	CacheClearU,Forbid,Permit,Enable,Disable
	dc	CloseLibrary,0

harmlessdoslist	dc	UnLock,UnLoadSeg,0

zero_sample	dc	0

test_copperlist	dc.l	$01000200,-2

exec_error_msg	dc.b	'unimplemented exec function: return address:',0
	even

exec_exception	push	all
	lea	exec_error_msg(pc),a0
	bsr	put_string
	move.l	60(a7),d0
	bsr	put_value
	pull	all
	illegal

excep	movem.l	d0-d7/a0-a7,$100
	move	#$7fff,intena+custom
	move	#$7fff,intreq+custom
	move	#$7fff,dmacon+custom
	lea	$100.w,a7
	bsr	set_message_traps
	moveq	#UADE_SCORECRASH,d0
	bsr	put_message_by_value
	movem.l	$100,d0-d7/a0-a7

exceploop	move	d0,$dff180
	not	d0
	bra.b	exceploop


contplayer	* initialize messaging trap *
	bsr	set_message_traps

	lea	moduleptr(pc),a0
	lea	modulesize(pc),a1
	move.l	$100.w,(a0)
	move.l	$104.w,(a1)

	move.l	$108.w,a0		* player address
	lea	chippoint(pc),a2
	move.l	$10c.w,(a2)		* reloc address
	bsr	relocator
	tst.l	d0
	beq.b	reloc_success
	lea	reloc_error_msg(pc),a0
	bsr	put_string
	moveq	#UADE_SCOREDEAD,d0
	bsr	put_message_by_value
reloc_wait_forever	bra	reloc_wait_forever

reloc_error_msg	dc.b	'reloc error',0
	even

reloc_success	lea	binbase(pc),a1
	move.l	a0,(a1)		* a0 = player code start relocated

	* allocate space for dynamic memory operations (allocmem,
	* loadseq, ...)
	lea	chippoint(pc),a0
	move.l	moduleptr(pc),d0
	add.l	modulesize(pc),d0
	add.l	#1024,d0
	clr.b	d0
	move.l	d0,(a0)

	* volume test (debug)
	lea	voltestbit(pc),a0
	moveq	#0,d0
	cmp.l	#$12345678,$190.w
	bne.b	novoltest
	moveq	#-1,d0
novoltest	move.l	d0,(a0)
	* dma wait (debug)
	cmp	#$1234,$194.w
	bne.b	nospecialdmawait
	move.l	$194.w,d0
	ext.l	d0
	lea	dmawaitconstant(pc),a0
	move.l	d0,(a0)
nospecialdmawait
	* EP_ModuleChange
	move.l	$198.w,d0
	beq.b	noepmc
	moveq	#-1,d0
	lea	modulechange_disabled(pc),a0
	move.l	d0,(a0)
noepmc
	* initialize intuitionbase with failures
	lea	intuition_lib_base(pc),a0
	lea	intuiwarn(pc),a1
	move.l	#$400,d0
	bsr	exec_initlibbase
	* initialize intuitionbase functions
	lea	intuition_lib_base(pc),a6
	lea	intui_allocremember(pc),a0
	move	jmpcom(pc),-$18C(a6)
	move.l	a0,-$18C+2(a6)

	* ntsc/pal checking
	moveq	#$20,d1		* default beamcon0 = $20 for PAL
	moveq	#50,d2		* default 50Hz for PAL
	* 709379 / 50
	move	#$376b,d3	* PAL CIA timer value
	tst.l	$124.w
	beq.b	is_pal
	moveq	#0,d1		* NTSC beamcon0 = 0
	moveq	#60,d2		* NTSC 60 Hz
	move	#$2e9c,d3	* NTSC CIA timer value
is_pal	move	d1,beamcon0+custom
	lea	vbi_hz(pc),a0
	move	d2,(a0)
	move.l	4.w,a6
	move.b	d2,$212(a6)	* set execbase vblank frequency
	lea	cia_timer_base_value(pc),a0
	move	d3,(a0)

	* check deliplayer's header tags
	bsr	parse_player_tags

	* initialize delitracker api
	bsr	init_base

	bsr	split_module_name

	bsr	call_config

	* make sure dtg_PathArrayPtr has module name stored
	bsr	copy_module_name

	* check if module corresponds to deliplayer
	bsr	call_check_module

	bsr	call_extload

	* initialize interrupt vectors
	bsr	init_interrupts

	* initialize noteplayer (after configfunc)
	bsr	np_init

	* Set default subsong to zero
	lea	eaglebase(pc),a5
	move	#0,dtg_SndNum(a5)

	* Set default timer value
	move	cia_timer_base_value(pc),dtg_Timer(a5)

	bsr	call_init_player

	* Initialize amplifier
	bsr	call_amplifier_init

	bsr	get_player_info

	bsr	call_check_subsongs

	* set subsong
	move.l	$11c.w,d1
	beq.b	nospecialsubs
	move.l	$120.w,d0
	cmp	#2,d1
	bne.b	notrelsubs
	move	minsubsong(pc),d1
	add	d1,d0
notrelsubs	bsr	SetSubSong
	bra.b	dontsetsubsong
nospecialsubs
	lea	eaglebase(pc),a5
	move	dtg_SndNum(a5),d0
	bne.b	dosetsubsong
	move	minsubsong(pc),d0
dosetsubsong	* takes subsong in D0
	bsr	SetSubSong
dontsetsubsong
	bsr	ReportSubSongs

	* filter off
	bset	#1,$bfe001

call_init_volume
	move.l	volumefunc(pc),d0
	beq.b	novolfunc
	lea	eaglebase(pc),a5
	move.l	d0,a0
	jsr	(a0)
novolfunc
	* call initsound
	bsr	call_init_sound

	* tell the simulator that audio output should start now
	move.l	#UADE_START_OUTPUT,d0
	bsr	put_message_by_value

	* CIA/VBI is initialized here, or start_int is called, if
	* necessary
	bsr	set_player_interrupt

playloop	* this is for debugging only
	bsr	volumetest

	bsr	waittrap		* wait for next frame

	* check input message
	tst.l 	$300.w
	beq.b	noinputmsgs
	bsr	inputmessagehandler
noinputmsgs
	lea	changesubsongbit(pc),a0
	tst	(a0)
	beq	dontchangesubs
	clr	(a0)

	move	#$4000,intena+custom
	* kill timer device *
	lea	vblanktimerstatusbit(pc),a0
	clr.l	(a0)
	* kill vbi list *
	lea	lev3serverlist(pc),a0
	clr.l	(a0)
	move	#$f,dmacon+custom
	move	#0,aud0vol+custom
	move	#0,aud1vol+custom
	move	#0,aud2vol+custom
	move	#0,aud3vol+custom
	move	#$00ff,adkcon+custom
	bsr	wait_audio_dma
	move	#$800f,dmacon+custom

	lea	songendbit(pc),a0	* clear for short modules
	clr.l	(a0)
	lea	virginaudioints(pc),a0	* audio ints are virgins again
	clr.l	(a0)

change_subsong	* call nextsongfunc or prevsongfunc if necessary
	move.l	adjacentsubfunc(pc),d0
	beq.b	notadjacentsub
	lea	eaglebase(pc),a5
	move.l	d0,a0
	jsr	(a0)
	lea	cursubsong(pc),a0
	lea	eaglebase(pc),a5
	move	dtg_SndNum(a5),(a0)
	bra.b	adjacentsub
notadjacentsub
	bsr	call_stop_int
	bsr	call_end_sound
	bsr	call_init_sound
	bsr	set_player_interrupt
adjacentsub	move	#$c000,intena+custom
dontchangesubs

	btst	#6,$bfe001
	beq.b	end_song

	* check if song has ended, ignore if songendbit ($12C) = 0
	tst.l	$12C.w
	beq.b	nosongendcheck
	move	songendbit(pc),d0
	bne.b	end_song
nosongendcheck

	* call dtp_interrupt if dtp_startint function hasn't been inited
	move.l	useciatimer(pc),d0
	bne.b	dontcallintfunc

	move.l	intfunc(pc),d0
	bne.b	dont_check_start_int
	move.l	startintfunc(pc),d1
	beq	dontplay
	bsr	HandleTimerDevice
	bra.b	dontcallintfunc
dont_check_start_int
	* call dtp_interrupt with trap (some require superstate)
	move.l	d0,a0
	move	#$2000,d0
	lea	trapcall(pc),a1
	move.l	a1,TRAP_VECTOR_3
	lea	eaglebase(pc),a5
	trap	#3
dontcallintfunc	bra	playloop			* loop back

end_song	* FIRST: report that song has ended
	bsr	report_song_end

	* THEN do the deinit stuff...
	bsr	call_stop_int
	bsr	call_end_sound

	* wait for late change of subsong
	lea	songendbit(pc),a0
	clr.l	(a0)
subsongseqloop
	bsr	waittrap		* wait for next frame

	* check input message
	tst.l 	$300.w
	beq.b	subsongseqloop
	bsr	inputmessagehandler
	lea	changesubsongbit(pc),a0
	tst	(a0)
	beq.b	subsongseqloop
	bsr	call_init_sound
	bsr	set_player_interrupt
	bra	playloop

dontplay	* report that score is dead
	bsr	set_message_traps
	move	songendbit(pc),d0
	bne.b	noscoredeadmsg
	moveq	#UADE_SCOREDEAD,d0
	bsr	put_message_by_value
noscoredeadmsg
	* check if this is delimon
	cmp.l	#'MONI',$188.w
	bne.b	flashloop
	move	#$7fff,intena+custom
	move	#$7fff,intreq+custom
	lea	smallint(pc),a0
	move.l	a0,$6c.w
	move	#$c020,intena+custom
	bra.b	flashloop
smallint	move	#$0020,intreq+custom
	rte

flashloop	move	$dff006,$dff180
	bra.b	flashloop


jsrcom	jsr	0
jmpcom	jmp	0

waittrap	lea	waittrapvector(pc),a0
	move.l	a0,TRAP_VECTOR_4
	trap	#4
	rts

* wait for the next vertical blanking frame and return
waittrapvector	lea	framecount(pc),a0
	move.l	(a0),d0
newstop	stop	#$2000
	move.l	(a0),d1
	cmp.l	d0,d1
	beq.b	newstop
	rte

* dumps memory to output trap
bintrap	push	all
	cmp.l	#32,d0
	ble.b	nottoobig
	moveq	#32,d0
nottoobig	and.l	#-4,d0
	push	all
	move.l	a0,d0
	lea	binaddr(pc),a0
	bsr	genhexstring
	pull	all
	lea	bindump(pc),a1
bindumploop	tst.l	d0
	beq.b	endbindumploop
	move.l	(a0)+,d1
	push	all
	move.l	d1,d0
	move.l	a1,a0
	bsr	genhexstring
	pull	all
	subq.l	#4,d0
	addq.l	#8,a1
	move.b	#' ',(a1)+
	bra.b	bindumploop
endbindumploop	clr.b	(a1)+
	lea	binmsg(pc),a0
	sub.l	a0,a1
	move.l	a1,d0
	bsr	put_message
	pull	all
	rte
binmsg	dc.l	UADE_GENERALMSG
	dc.b	'MEM '
binaddr	dcb.b	8,0
	dc.b	': '
bindump	dcb.b	100,0
	even

* outputs constant d0 to output trap
put_value	push	all
	* d0 has input
	lea	pollmsgcode(pc),a0
	bsr	genhexstring
	lea	pollmsg(pc),a0
	bsr	put_string
	pull	all
	rts
pollmsg	dc.b	'debug value '
pollmsgcode	dcb.b	9,0
pollmsge	even

* inputs: d0 = number   a0 = string pointer
* function: generates ascii string from d0 in hexadecimal representation to a0
genhexstring	push	all
	moveq	#8-1,d7
ghsl	rol.l	#4,d0
	moveq	#$f,d1
	and.l	d0,d1
	cmp	#10,d1
	blt.b	notalfa
	add.b	#'A'-10-$30,d1
notalfa	add.b	#$30,d1
	move.b	d1,(a0)+
	dbf	d7,ghsl
	pull	all
	rts

dosexception	lea	dos_lib_base(pc),a0
	move.l	(a7),a1
	subq.l	#6,a1
	sub.l	a1,a0
	move.l	a0,d0
	lea	doserrorcode(pc),a0
	bsr	genhexstring	
	lea	doserrormsg(pc),a0
	bsr	put_string
	illegal
doserrormsg	dc.b	'non-implemented dos.library function called:'
	dc.b	' -$'
doserrorcode	dcb.b	8,0
	dc.b	' => CRASH',0
doserrormsge	even

split_module_name
	move.l	$128.w,d0
	beq.b	nomodulename
	move.l	d0,a0
	lea	loadedmodname(pc),a1
	move.l	#255,d0
	bsr	strlcpy

	lea	loadedmodname(pc),a0
	bsr	strendptr
	move.l	a0,a1
	lea	loadedmodname(pc),a0
separloop1	cmp.b	#'/',-(a1)
	beq.b	endseparloop1
	cmp.l	a0,a1
	bne.b	separloop1
endseparloop1	cmp.b	#'/',(a1)
	bne.b	noslash1
	addq.l	#1,a1
noslash1
	lea	dirarray(pc),a3
patharrayloop1	cmp.l	a0,a1
	beq.b	endpatharrayloop1
	move.b	(a0)+,(a3)+
	bra.b	patharrayloop1
endpatharrayloop1
	clr.b	(a3)+
	move.l	a1,a0
	lea	filearray(pc),a1
	move.l	#255,d0
	bsr	strlcpy

nomodulename	rts

copy_module_name
	lea	eaglebase(pc),a5
	move.l	dtg_PathArrayPtr(a5),a0
	clr.b	(a0)
	bsr	copydir
	bsr	copyfile
	rts

timeventerrmsg1	dc.b	'soundcore: handletimerdevice(): timer function is '
	dc.b	'NULL pointer ...',0
	even
HandleTimerDevice
	move.l	vblanktimerstatusbit(pc),d0
	beq.b	notimerdevcount
	move.l	vblanktimercount(pc),d0
	bne.b	notimerdevevent
	lea	vblanktimerstatusbit(pc),a0
	clr.l	(a0)
	move.l	vblanktimerfunc(pc),d0
	bne.b	timerdevfuncok
	lea	timeventerrmsg1(pc),a0
	bsr	put_string
	bra	dontplay
timerdevfuncok	move.l	timerioptr(pc),a1
	push	all
	move.l	d0,a0
	jsr	(a0)
	pull	all
	bra.b	notimerdevcount
notimerdevevent	lea	vblanktimercount(pc),a1
	tst.l	(a1)
	beq.b	notimerdevcount
	subq.l	#1,(a1)
notimerdevcount	rts


* this function calls startint function if it exists and intfunc doesnt exist
call_start_int	push	all
	move.l	intfunc(pc),d0
	bne.b	intfuncexists
	move.l	startintfunc(pc),d0
	beq	dontplay
	move.l	d0,a0
	lea	eaglebase(pc),a5
	jsr	(a0)
intfuncexists	pull	all
	rts


call_stop_int	push	all
	move.l	stopintfunc(pc),d0
	beq.b	nostopintfunc
	move.l	d0,a0
	lea	eaglebase(pc),a5
	jsr	(a0)
nostopintfunc
	* Disable player interrupt, if any.
	move.l	intfunc(pc),d0
	beq.b	do_not_disable_player_interrupt
	lea	rem_ciaa_interrupt(pc),a0
	move.l	cia_chip_sel(pc),d0
	beq.b	its_ciaa_2
	lea	rem_ciab_interrupt(pc),a0
its_ciaa_2	move.l	cia_timer_sel(pc),d0
	jsr	(a0)
do_not_disable_player_interrupt
	pull	all
	rts


messagetrap	rte


inputmessagehandler
	push	all
	lea	$300.w,a0
	cmp.l	#UADE_SETSUBSONG,(a0)
	bne.b	nnsubs
	move.l	$120.w,d0
	* call SetSubSong if nextsubsong func is not used
	push	all
	move	d0,d1
	sub	cursubsong(pc),d1
	lea	adjacentsubfunc(pc),a1
	clr.l	(a1)
	cmp	#1,d1
	bne.b	notnextsub
	move.l	nextsongfunc(pc),d2
	beq.b	notnextsub
	move.l	d2,(a1)
	bra.b	dontcallsetss
notnextsub	cmp	#-1,d1
	bne.b	notprevsub
	move.l	prevsongfunc(pc),d2
	beq.b	notprevsub
	move.l	d2,(a1)
	bra.b	dontcallsetss
notprevsub	bsr	SetSubSong
dontcallsetss	pull	all
	lea	changesubsongbit(pc),a0
	st	(a0)
	bra	inputmessagehandled
nnsubs
	cmp.l	#UADE_NTSC,(A0)
	bne.b	no_ntsc_pal
	move.l	$124.w,d0
	and.l	#1,d0
	move.l	d0,d1
	eor.b	#1,d0
	lsl	#5,d0
	move	d0,beamcon0+custom
	lea	ntsc_report_bit(pc),a0
	add.b	#$30,d1
	move.b	d1,(a0)
	lea	ntsc_report_msg(pc),a0
	bsr	put_string
	bra	inputmessagehandled
ntsc_report_msg	dc.b	'ntsc bit set: '
ntsc_report_bit	dc.b	'0', 0

no_ntsc_pal	move.l	$300.w,d0
	lea	inputmsgcode(pc),a0
	bsr	genhexstring
	lea	input_msg_error(pc),a0
	bsr	put_string

inputmessagehandled
	clr.l	$300.w
	pull	all
	rts

input_msg_error	dc.b	'sound core got unknown input message 0x'
inputmsgcode	dcb.b	9,0
	even

put_message	move.l	msgptr(pc),a1
msgloop	tst.l	d0
	beq.b	endmsgloop
	move.b	(a0)+,(a1)+
	subq.l	#1,d0
	bra.b	msgloop
endmsgloop	trap	#5
	rts

put_message_by_value
	move.l	msgptr(pc),a0
	move.l	d0,(a0)
	trap	#5
	rts

put_string	push	all
	bsr	strlen
	addq.l	#1,d0
	move.l	msgptr(pc),a1
	move.l	#UADE_GENERALMSG,(a1)+
stringmsgloop	tst.l	d0
	beq.b	endstringmsgloop
	move.b	(a0)+,(a1)+
	subq.l	#1,d0
	bra.b	stringmsgloop
endstringmsgloop	trap	#5
	pull	all
	rts

strlen	moveq	#-1,d0
.strlenloop
	addq.l	#1,d0
	tst.b	(a0,d0.w)
	bne.b	.strlenloop
	rts

set_message_traps
	lea	messagetrap(pc),a0
	move.l	a0,TRAP_VECTOR_5
	lea	bintrap(pc),a0
	move.l	a0,TRAP_VECTOR_6
	rts


paska	PLAYERHEADER	0
paskaend

parse_player_tags
	move.l	binbase(pc),a0
	move.l	paskaend-paska-4(a0),a0
	
tagloop	move.l	(a0),d0
	tst.l	d0
	beq	endtagloop

	cmp.l	#DTP_Check2,d0
	bne.b	nono1
	lea	dtp_check(pc),a1
	move.l	4(a0),(a1)
nono1	cmp.l	#EP_Check3,d0
	bne.b	nono1_2
	lea	ep_check3(pc),a1
	move.l	4(a0),(a1)
nono1_2	cmp.l	#EP_Check5,d0
	bne.b	nono1_3
	lea	ep_check5(pc),a1
	move.l	4(a0),(a1)
nono1_3	cmp.l	#DTP_Interrupt,d0
	bne.b	nono2
	lea	intfunc(pc),a1
	move.l	4(a0),(a1)
nono2	cmp.l	#DTP_InitPlayer,d0
	bne.b	nono3
	lea	initfunc(pc),a1
	move.l	4(a0),(a1)
nono3	cmp.l	#DTP_SubSongRange,d0
	bne.b	nono4
	lea	subsongfunc(pc),a1
	move.l	4(a0),(a1)
nono4	cmp.l	#DTP_EndSound,d0
	bne.b	nono5
	lea	endfunc(pc),a1
	move.l	4(a0),(a1)
nono5	cmp.l	#DTP_InitSound,d0
	bne.b	nono6
	lea	initsoundfunc(pc),a1
	move.l	4(a0),(a1)
nono6	cmp.l	#DTP_CustomPlayer,d0
	bne.b	nono7
	lea	moduleptr(pc),a1
	clr.l	(a1)
	lea	modulesize(pc),a1
	clr.l	(a1)
nono7	cmp.l	#DTP_Volume,d0
	bne.b	nono9
	lea	volumefunc(pc),a1
	move.l	4(a0),(a1)
nono9	cmp.l	#DTP_NextSong,d0
	bne.b	nono10
	lea	nextsongfunc(pc),a1
	move.l	4(a0),(a1)
nono10	cmp.l	#DTP_PrevSong,d0
	bne.b	nono11
	lea	prevsongfunc(pc),a1
	move.l	4(a0),(a1)
nono11	cmp.l	#DTP_DeliBase,d0
	bne.b	nono12
	lea	eaglebase(pc),a1
	move.l	4(a0),a2
	move.l	a1,(a2)
nono12	cmp.l	#DTP_StartInt,d0
	bne.b	nono13
	lea	startintfunc(pc),a1
	move.l	4(a0),(a1)
nono13	cmp.l	#DTP_StopInt,d0
	bne.b	nono14
	lea	stopintfunc(pc),a1
	move.l	4(a0),(a1)
nono14	cmp.l	#DTP_Config,d0
	bne.b	nono15
	lea	configfunc(pc),a1
	move.l	4(a0),(a1)
nono15	cmp.l	#DTP_ExtLoad,d0
	bne.b	nono16
	lea	extloadfunc(pc),a1
	move.l	4(a0),(a1)
nono16	cmp.l	#DTP_NewSubSongRange,d0
	bne.b	nono17
	lea	newsubsongarray(pc),a1
	move.l	4(a0),(a1)
nono17	cmp.l	#DTP_NoteStruct,d0
	bne.b	nono18
	lea	noteplayerptr(pc),a1
	move.l	4(a0),(a1)
nono18	cmp.l	#$80004486,d0		* DTP_Process+9
	bne.b	nono19
	lea	noteplayersetupfunc(pc),a1
	move.l	4(a0),(a1)
nono19	cmp.l	#EP_InitAmplifier,d0
	bne.b	nono20
	lea	amplifier_init_func(pc),a1
	move.l	4(a0),(a1)
nono20	cmp.l	#EP_EagleBase,d0
	bne.b	nono21
	lea	eaglebase(pc),a1
	move.l	4(a0),a2
	move.l	a1,(a2)
nono21
nexttag	addq.l	#8,a0
	bra	tagloop
endtagloop
	rts


get_player_info	move.l	binbase(pc),a0
	move.l	paskaend-paska-4(a0),a0
infoloop	move.l	(a0),d0
	tst.l	d0
	beq	endinfoloop
	cmp.l	#DTP_ModuleName,d0
	bne.b	nodtpmodulename
	move.l	4(a0),a1
	move.l	(a1),d1
	beq.b	nodtpmodulename
	push	all
	move.l	d1,a0
	lea	modulename+4(pc),a1
	move.l	#250,d0
	bsr	strlcpy
	lea	modulename(pc),a0
	move.l	#UADE_MODULENAME,(a0)
	move.l	#256,d0
	bsr	put_message
	pull	all
	bra	nextinfoiter
nodtpmodulename	cmp.l	#DTP_PlayerName,d0
	bne.b	nodtpplayername
	push	all
	move.l	4(a0),a0
	lea	playername+4(pc),a1
	move.l	#25,d0
	bsr	strlcpy
	lea	playername(pc),a0
	move.l	#UADE_PLAYERNAME,(a0)
	move.l	#32,d0
	bsr	put_message
	pull	all
	bra	nextinfoiter
nodtpplayername	cmp.l	#DTP_FormatName,d0
	bne.b	nodtpformatname
	move.l	4(a0),a1
	move.l	(a1),d1
	beq.b	nodtpformatname
	push	all
	move.l	d1,a0
	lea	formatname+4(pc),a1
	move.l	#250,d0
	bsr	strlcpy
	lea	formatname(pc),a0
	move.l	#UADE_FORMATNAME,(a0)
	move.l	#256,d0
	bsr	put_message
	pull	all
	bra	nextinfoiter
nodtpformatname
nextinfoiter	addq.l	#8,a0
	bra	infoloop
endinfoloop	rts

* a0 src1 a1 src2
* returns d0: zero => same, non-zero => not same (not usable for sorting)
strcmp	push	a0-a1
strcmp_loop	move.b	(a0)+,d0
	cmp.b	(a1)+,d0
	bne.b	strcmp_notsame
	tst.b	d0
	bne.b	strcmp_loop
	pull	a0-a1
	moveq	#0,d0
	rts
strcmp_notsame	pull	a0-a1
	moveq	#-1,d0
	rts

* a0 src1, a1 src2, d0 max length to compare
* returns d0: zero => same, non-zero => not same (not usable for sorting)
strncmp	movem.l	d1/a0-a1,-(a7)
	move.l	d0,d1
	beq.b	.strncmp_notsame
.strncmp_loop
	move.b	(a0)+,d0
	cmp.b	(a1)+,d0
	bne.b	.strncmp_notsame
	subq.l	#1,d1
	beq.b	.strncmp_same
	tst.b	d0
	bne.b	.strncmp_loop
.strncmp_same
	movem.l	(a7)+,d1/a0-a1
	moveq	#0,d0
	rts
.strncmp_notsame
	movem.l	(a7)+,d1/a0-a1
	moveq	#-1,d0
	rts

* a0 src a1 dst d0 max bytes
strlcpy	subq	#1,d0
	bmi.b	endstrlcpyloop
strlcpyloop	move.b	(a0)+,d1
	move.b	d1,(a1)+
	tst.b	d1
	beq.b	endstrlcpyloop
	dbf	d0,strlcpyloop
endstrlcpyloop	rts

* a0 string returns the zero byte in a0
strendptr	tst.b	(a0)+
	bne.b	strendptr
	subq.l	#1,a0
	rts


hex2int	moveq	#0,d0
	push	d1/a0
hexloop	move.b	(a0)+,d1
	beq.b	endhexloop
	lsl	#4,d0
	cmp.b	#$61,d1
	blt.b	notsmall
	sub.b	#$61-10,d1
	and	#$f,d1
	or	d1,d0
	bra.b	hexloop
notsmall	cmp.b	#$41,d1
	blt.b	notbig
	sub.b	#$41-10,d1
	and	#$f,d1
	or	d1,d0
	bra.b	hexloop
notbig	sub.b	#$30,d1
	and	#$f,d1
	or	d1,d0
	bra.b	hexloop
endhexloop	pull	d1/a0
	rts


safetybaseroutine
	push	all
	lea	safetymsg(pc),a0
	bsr	put_string
	move.l	60(a7),d0		* return address
	subq.l	#6,d0
	lea	eaglebase(pc),a0
	sub.l	a0,d0
	bsr	put_value
	pull	all
	illegal

safetymsg	dc.b	'fatal error: non-implemented eagleplayer function',0
	even

init_base	lea	eaglebase(pc),a5

	* set unimplemented ENPP function calls to safetybaseroutine()
	move.l	a5,a0
	lea	safetybaseroutine(pc),a1
	moveq	#(eaglebase-eaglesafetybase)/6-1,d0
dsbl	subq.l	#6,a0
	move	jsrcom(pc),(a0)
	move.l	a1,2(a0)
	dbf	d0,dsbl

	move.l	moduleptr(pc),d0
	move.l	modulesize(pc),d1
	move.l	d0,dtg_ChkData(a5)
	move.l	d1,dtg_ChkSize(a5)

	move	cursubsong(pc),dtg_SndNum(a5)
	moveq	#64,d0
	move	d0,dtg_SndVol(a5)
	move	d0,dtg_SndLBal(a5)
	move	d0,dtg_SndRBal(a5)
	move	#15,EPG_Voices(a5)
	move	d0,EPG_Voice1Vol(a5)
	move	d0,EPG_Voice2Vol(a5)
	move	d0,EPG_Voice3Vol(a5)
	move	d0,EPG_Voice4Vol(a5)

	lea	okprog(pc),a0
	move.l	a0,dtg_AudioAlloc(a5)
	move.l	a0,dtg_AudioFree(a5)
	lea	ENPP_AllocAudio(a5),a0
	lea	enpp_allocaudio(pc),a1
	move	jmpcom(pc),(a0)+
	move.l	a1,(a0)+

	lea	get_list_data(pc),a0
	move.l	a0,dtg_GetListData(a5)
	move	jmpcom(pc),ENPP_GetListData(a5)
	move.l	a0,ENPP_GetListData+2(a5)

	lea	endsongfunc(pc),a0
	move.l	a0,dtg_SongEnd(a5)
	move	jmpcom(pc),ENPP_SongEnd(a5)
	move.l	a0,ENPP_SongEnd+2(a5)

	lea	defaultstartintfunc(pc),a0
	move.l	startintfunc(pc),d0
	beq.b	nostartintfunc
	move.l	d0,a0
nostartintfunc	move.l	a0,dtg_StartInt(a5)
	lea	defaultstopintfunc(pc),a0
	move.l	stopintfunc(pc),d0
	beq.b	nostopintfunc2
	move.l	d0,a0
nostopintfunc2	move.l	a0,dtg_StopInt(a5)

	lea	settimer(pc),a0
	move.l	a0,dtg_SetTimer(a5)
	clr	dtg_Timer(a5)
	lea	ENPP_SetTimer(a5),a1
	move	jmpcom(pc),(a1)+
	move.l	a0,(a1)+

	lea	np_int(pc),a0
	move.l	a0,dtg_NotePlayer(a5)

	lea	epg_findauthor(pc),a0
	move.l	a0,EPG_FindAuthor(a5)
	lea	epg_modulechange(pc),a0
	move.l	a0,EPG_ModuleChange(a5)

	lea	wait_audio_dma(pc),a0
	move.l	a0,dtg_WaitAudioDMA(a5)

	lea	patharray(pc),a0
	move.l	a0,dtg_PathArrayPtr(a5)
	lea	filearray(pc),a0
	move.l	a0,dtg_FileArrayPtr(a5)
	lea	dirarray(pc),a0
	move.l	a0,dtg_DirArrayPtr(a5)

	lea	loadfile(pc),a0
	move.l	a0,dtg_LoadFile(a5)
	lea	ENPP_LoadFile(a5),a1
	move	jmpcom(pc),(a1)+
	move.l	a0,(a1)+
	lea	ENPP_NewLoadFile(a5),a1
	move	jmpcom(pc),(a1)+
	move.l	a0,(a1)+
	lea	EPG_NewLoadFile(a5),a1
	move.l	a0,(a1)+

	lea	copydir(pc),a0
	move.l	a0,dtg_CopyDir(a5)
	lea	copyfile(pc),a0
	move.l	a0,dtg_CopyFile(a5)
	lea	cutsuffix(pc),a0
	move.l	a0,dtg_CutSuffix(a5)
	lea	copystring(pc),a0
	move.l	a0,dtg_CopyString(a5)

	lea	dos_lib_base(pc),a0
	move.l	a0,dtg_DOSBase(a5)

	lea	intuition_lib_base(pc),a0
	move.l	a0,dtg_IntuitionBase(a5)

	lea	amplifier_int(pc),a0
	move	jmpcom(pc),ENPP_Amplifier(a5)
	move.l	a0,ENPP_Amplifier+2(a5)
	lea	amplifier_dma(pc),a0
	move	jmpcom(pc),ENPP_DMAMask(a5)
	move.l	a0,ENPP_DMAMask+2(a5)
	lea	amplifier_adr(pc),a0
	move	jmpcom(pc),ENPP_PokeAdr(a5)
	move.l	a0,ENPP_PokeAdr+2(a5)
	lea	amplifier_len(pc),a0
	move	jmpcom(pc),ENPP_PokeLen(a5)
	move.l	a0,ENPP_PokeLen+2(a5)
	lea	amplifier_per(pc),a0
	move	jmpcom(pc),ENPP_PokePer(a5)
	move.l	a0,ENPP_PokePer+2(a5)
	lea	amplifier_vol(pc),a0
	move	jmpcom(pc),ENPP_PokeVol(a5)
	move.l	a0,ENPP_PokeVol+2(a5)
	lea	amplifier_command(pc),a0
	move	jmpcom(pc),ENPP_PokeCommand(a5)
	move.l	a0,ENPP_PokeCommand+2(a5)

	lea	ENPP_FindAuthor(a5),a0
	lea	enpp_find_author(pc),a1
	move	jmpcom(pc),(a0)+
	move.l	a1,(a0)+
	rts


enpp_allocaudio	tst.l	d0
	rts

fawarn	dc.b	'ENPP_FindAuthor not implemented',0
	even
enpp_find_author
	push	all
	lea	fawarn(pc),a0
	bsr	put_string
	pull	all
	rts

wait_audio_dma	push	d0-d1
	move.l	dmawaitconstant(pc),d0
	subq	#1,d0
	bmi.b	wad_nowait
wad_loop	move.b	$dff006,d1
wad_loop_1	cmp.b	$dff006,d1
	beq.b	wad_loop_1
	dbf	d0,wad_loop
wad_nowait	pull	d0-d1
	rts


settimer	push	all
	move.l	cia_chip_sel(pc),d0
	move.l	cia_timer_sel(pc),d1
	bsr	set_cia_timer_value
	pull	all
	rts


set_player_interrupt
	push	all
	move.l	intfunc(pc),d0
	beq.b	try_start_int
	lea	useciatimer(pc),a0
	st	(a0)
	lea	add_ciaa_interrupt(pc),a0
	move.l	cia_chip_sel(pc),d0
	beq.b	its_ciaa
	lea	add_ciab_interrupt(pc),a0
its_ciaa	lea	tempciabtimerstruct(pc),a1
	move.l	intfunc(pc),$12(a1)
	move.l	cia_timer_sel(pc),d0
	jsr	(a0)
	pull	all
	rts
try_start_int	move.l	startintfunc(pc),d0
	beq	dontplay
	* call startint (player initializes own interrupts here)
	bsr	call_start_int
	pull	all
	rts

tempciabtimerstruct	dcb.b	$20,0

* D0 CIA A or CIA B: 0 = CIA A, 1 = CIA B
* D1 Timer A or timer B: 0 = Timer A, 1 = Timer B
set_cia_timer_value
	push	d0-d1/a0-a1
	btst	#0,d0
	bne.b	set_ciab_timer
	lea	$bfe001,a0
	bra.b	set_ciaa_timer
set_ciab_timer	lea	$bfd000,a0
set_ciaa_timer	and	#1,d1
	lsl	#8,d1
	add	d1,d1
	add	d1,a0
	lea	eaglebase(pc),a1
	move	dtg_Timer(a1),d0
	move.b	d0,$400(a0)
	ror	#8,d0
	move.b	d0,$500(a0)
	pull	d0-d1/a0-a1
	rts


modulechangemsg	dc.b	'epg_modulechange: patched the player', 0
findauthormsg	dc.b	'epg_findauthor notice', 0
	even

epg_modulechange	push	all
	move.l	modulechange_disabled(pc),d0
	bne	modulechange_not_enabled
	cmp.l	#1,EPG_ARG4(a5)
	bne	modulechange_not_enabled
	cmp.l	#-2,EPG_ARG5(a5)
	bne.b	modulechange_not_enabled
	cmp.l	#5,EPG_ARGN(a5)
	bne.b	modulechange_not_enabled
	moveq	#0,d7			* number of patches applied
	move.l	EPG_ARG3(a5),a1		* patch table
mc_pl_1	move	(a1)+,d2		* pattern offset
	beq.b	end_mc_pl_1
	move	(a1)+,d3		* (pattern_len / 2) - 1
	move	(a1)+,d4		* patch offset
	move.l	EPG_ARG1(a5),a0		* dst address
	move.l	EPG_ARG2(a5),d0		* dst len
mc_pl_2	tst.l	d0
	ble.b	end_mc_pl_2
	move	d3,d5			* patch dbf len
	move.l	EPG_ARG3(a5),a2		* patch table
	add	d2,a2			* + pattern offset
	move.l	a0,a3			* dst
mc_pl_3	cmpm	(a2)+,(a3)+
	bne.b	mc_pl_3_no
	dbf	d5,mc_pl_3
	* a pattern match => patch it
	move.l	a0,a2			* dst address
	move	d3,d5			* patch dbf len
mc_pl_3_nop	move	#$4e71,(a2)+		* put nops to old code
	dbf	d5,mc_pl_3_nop
	move.l	EPG_ARG3(a5),a2		* patch table
	add	d4,a2			* patch code address
	move	jsrcom(pc),(a0)		* put jump to patch code
	move.l	a2,2(a0)
	addq.l	#1,d7			* number of patches applied
	move	d3,d5
	ext.l	d5
	add.l	d5,d5
	sub.l	d5,d0
	add.l	d5,a0			* skip dbf_len*2+2-2
mc_pl_3_no	addq.l	#2,a0
	subq.l	#2,d0
	bra.b	mc_pl_2
end_mc_pl_2	bra.b	mc_pl_1
end_mc_pl_1	tst.l	d7
	beq.b	modulechange_not_enabled
	lea	modulechangemsg(pc),a0
	bsr	put_string
modulechange_not_enabled
	pull	all
	rts
epg_findauthor	push	all
	lea	findauthormsg(pc),a0
	bsr	put_string
	pull	all
	rts

defaultstartintfunc	lea	startintwarning(pc),a0
	bsr	put_string
	rts
defaultstopintfunc	lea	stopintwarning(pc),a0
	bsr	put_string
	rts
startintwarning	dc.b	'warning: default start int func called', 0
stopintwarning	dc.b	'warning: default stop int func called', 0
	even

noinitfuncwarn	dc.b	'warning: the player is unorthodox (no InitPlayer())',0

call_init_player
	lea	eaglebase(pc),a5
	move.l	initfunc(pc),d0
	bne.b	hasinitfunction
	lea	noinitfuncwarn(pc),a0
	bsr	put_string
	rts
hasinitfunction	move.l	d0,a0
	jsr	(a0)
	tst.l	d0
	beq.b	initwasok
	lea	initerrmsg(pc),a0
	bsr	put_string
initwasok	rts
initerrmsg	dc.b	'InitPlayer function returned fail',0
	even

checkwarn	dc.b	'warning: both DTP_Check2 and EP_Check5 available: '
	dc.b	'using DTP_Check2',0
epcheck3warn	dc.b	'warning: using EP_Check3',0
	even

* call check module: Priority for checks (first is highest):
*	     DTP_Check2, EP_Check5, EP_Check3
call_check_module
	lea	eaglebase(pc),a5
	move.l	dtp_check(pc),d0
	beq.b	no_dtp_ep_conflict
	move.l	ep_check5(pc),d0
	beq.b	no_dtp_ep_conflict
	lea	checkwarn(pc),a0
	bsr	put_string
no_dtp_ep_conflict
	move.l	dtp_check(pc),d0
	bne.b	do_ep_check
	move.l	ep_check5(pc),d0
	bne.b	do_ep_check
	move.l	ep_check3(pc),d0
	beq.b	do_no_check
	lea	epcheck3warn(pc),a0
	bsr	put_string
do_ep_check	move.l	d0,a0
	* must be called even when force_by_default is enabled
	jsr	(a0)
	tst.l	d0
	beq.b	song_ok
	tst.l	$118.w
	bne.b	song_ok
	moveq	#UADE_CHECKERROR,d0
	bsr	put_message_by_value
	bra	dontplay
do_no_check
song_ok	rts

call_config	lea	eaglebase(pc),a5
	move.l	configfunc(pc),d0
	beq.b	noconfig
	move.l	d0,a0
	jsr	(a0)
	tst.l	d0
	bne	dontplay
noconfig	rts

call_extload	* load external data if requested
	move.l	extloadfunc(pc),d0
	beq.b	noextloadfunc
	lea	eaglebase(pc),a5
	move.l	d0,a0
	jsr	(a0)
	tst.l	d0
	beq.b	noextloadfunc
	lea	extloaderrmsg(pc),a0
	bsr	put_string
	bra	dontplay
extloaderrmsg	dc.b	'ExtLoad failed',0
	even
noextloadfunc	rts

report_song_end	moveq	#UADE_SONG_END,d0
	bsr	put_message_by_value
	rts

call_check_subsongs
	push	all
	move.l	subsongfunc(pc),d0
	beq.b	NoSubSongFunc
	move.l	d0,a0
	lea	eaglebase(pc),a5
	jsr	(a0)
	lea	subsongrange(pc),a0
	movem	d0-d1,(a0)
NoSubSongFunc	move.l	newsubsongarray(pc),d0
	beq.b	nonewsubsongarr
	move.l	d0,a0
	movem	(a0),d0-d2   * d0 = default, d1 = min, d2 = max subsong
	lea	eaglebase(pc),a5
	move	d0,dtg_SndNum(a5)
	lea	subsongrange(pc),a1
	movem	d1-d2,(a1)
nonewsubsongarr	pull	all
	rts

ReportSubSongs	move	minsubsong(pc),d0
	move	maxsubsong(pc),d1
	move	cursubsong(pc),d2
	ext.l	d0
	ext.l	d1
	ext.l	d2
	lea	subsonginfo(pc),a0
	move.l	d0,4(a0)
	move.l	d1,8(a0)
	move.l	d2,12(a0)
	moveq	#16,d0
	bsr	put_message
	rts
subsonginfo	dc.l	UADE_SUBSINFO
	dc.l	0,0,0


SetSubSong	push	d0-d1/a0-a2/a5
	lea	cursubsong(pc),a0
	lea	eaglebase(pc),a5
	tst	d0
	bpl.b	notnegative
	moveq	#0,d0
notnegative	move	d0,(a0)
	move	d0,dtg_SndNum(a5)
nonewsubsong	pull	d0-d1/a0-a2/a5
	rts


call_init_sound	lea	eaglebase(pc),a5
	move.l	initsoundfunc(pc),d0
	beq.b	initsoundintena
	move.l	d0,a0
	move	intenar+custom,d1
	pushr	d1
	jsr	(a0)
	pullr	d1
initsoundintena	* this hack overcomes fatality in SynTracker deliplayer
	* SynTracker does move #$4000,intena+custom in InitSound
	* function and does not re-enable interrupts
	move	intenar+custom,d2
	and	#$4000,d1
	beq.b	nointenaproblem
	and	#$4000,d2
	bne.b	nointenaproblem
	lea	intenamsg(pc),a0
	bsr	put_string
	move	#$c000,intena+custom	* re-enable intena
nointenaproblem	tst.l	d0
	rts
intenamsg	dc.b	'Stupid deliplayer: disables interrupts',0
	even


call_end_sound	push	all
	move.l	endfunc(pc),d0
	beq.b	noendsound
	move.l	d0,a0
	lea	eaglebase(pc),a5
	jsr	(a0)
	pull	all
	rts
noendsound	move	#15,dmacon+custom
	move	#0,aud0vol+custom
	move	#0,aud1vol+custom
	move	#0,aud2vol+custom
	move	#0,aud3vol+custom
	pull	all
	rts


endsongfunc	pushr	a0
	lea	songendbit(pc),a0
	st	(a0)
	pullr	a0
	rts


okprog	moveq	#0,d0
	rts


*** MIX MODULE SIZE LATER ****

get_list_data	tst.l	d0
	bne.b	dontreturnmodule
	move.l	moduleptr(pc),a0
	move.l	modulesize(pc),d0
	rts
dontreturnmodule
	push	d1/a1
	lea	load_file_list(pc),a1
datalistloop1	move.l	(a1),d1
	bmi.b	endloopillegal
	cmp.l	d0,d1
	bne.b	notthisdata
	move.l	4(a1),a0
	move.l	8(a1),d0
	bra.b	enddatalistloop1
notthisdata	add	#12,a1
	bra.b	datalistloop1
enddatalistloop1
	pull	d1/a1
	rts
endloopillegal	add.b	#$30,d0
	lea	errorloadindex(pc),a1
	move.b	d0,(a1)
	lea	getlistdataerror(pc),a0
	bsr	put_string
	pull	d1/a1
	rts
getlistdataerror	dc.b	'Tried to get list data with index number '
errorloadindex	dc.b	'0, but it does not exist!',0
getlistdataerrore	even

copydir	push	d0/a0-a1/a5
	lea	eaglebase(pc),a5
	move.l	dtg_PathArrayPtr(a5),a0
	bsr	strendptr
	move.l	a0,a1
	lea	dirarray(pc),a0
	move.l	#255,d0
	bsr	strlcpy
	pull	d0/a0-a1/a5
	rts

copyfile	push	d0/a0-a1/a5
	lea	eaglebase(pc),a5
	move.l	dtg_PathArrayPtr(a5),a0
	bsr	strendptr
	move.l	a0,a1
	lea	filearray(pc),a0
	move.l	#128,d0
	bsr	strlcpy
	pull	d0/a0-a1/a5
	rts

cutsuffix	rts

copystring	pushr	a5
	pushr	a0
	move.l	dtg_PathArrayPtr(a5),a0
	bsr	strendptr
	move.l	a0,a1
	pullr	a0
	move.l	#128,d0
	bsr	strlcpy
	pullr	a5
	rts



loadfilemsg	dc.l	UADE_LOADFILE
	dc.l	0,0,0,0	* name ptr, dest ptr, size in msgptr(pc)+12
loadfilemsge
load_file_overflow_msg	dc.b	'load file list overflow!',0
	even

loadfile	push	d1-d7/a0-a6
	lea	loadfilemsg(pc),a0
	move.l	dtg_PathArrayPtr(a5),4(a0)
	move.l	chippoint(pc),d2
	move.l	d2,8(a0)
	pushr	d2
	clr.l	12(a0)
	move.l	#loadfilemsge-loadfilemsg,d0
	bsr	put_message
	move.l	msgptr(pc),a0
	move.l	12(a0),d3
	pullr	d2
	tst.l	d3
	beq.b	loadfileerror
	moveq	#1,d1
	lea	load_file_list(pc),a1
	lea	load_file_list_end(pc),a2
datalistloop2	cmp.l	a1,a2
	bgt.b	no_load_overflow
	lea	load_file_overflow_msg(pc),a0
	bsr	put_string
	bra	dontplay
no_load_overflow
	tst.l	(a1)
	bmi.b	enddatalistloop2
	move.l	(a1),d1
	addq.l	#1,d1
	add	#12,a1
	bra.b	datalistloop2
enddatalistloop2
	move.l	d1,(a1)		* index
	move.l	d2,4(a1)	* ptr
	move.l	d3,8(a1)	* size
	move.l	#-1,12(a1)	* mark end

	lea	chippoint(pc),a2
	add.l	d3,(a2)
	and.l	#-16,(a2)
	add.l	#16,(a2)

loadfileerror	moveq	#0,d0
	tst.l	d3
	seq	d0
	pull	d1-d7/a0-a6
	tst.l	d0
	rts

relocator	cmp.l	#$000003f3,(a0)+
	bne	hunkerror
	tst.l	(a0)+
	bne	hunkerror
	lea	nhunks(pc),a1
	move.l	(a0)+,(a1)		* take number of hunks
	* we could clear upper word of number of hunks, because original
	* implementation only uses 16 bits. it's an undocumented feature.
	* however, i'm sporty and want to see a player/custom that abuses
	* this feature. bring it on. the next cmp command will catch the
	* error.
	cmp.l	#100,(a1)
	bhi	hunkerror
	addq.l	#8,a0			* skip hunk load infos

	lea	hunks(pc),a1
	lea	chippoint(pc),a2
	move.l	nhunks(pc),d7
	subq	#1,d7
hunkcheckloop	move.l	(a0)+,d1
	move.l	d1,d2
	and.l	#$3fffffff,d1
	lsl.l	#2,d1
	move.l	d1,(a1)+		* save hunk size (in bytes)
	* Harry Sintonen pointed out there is a possibility of extra long
	* memattr here (another long word) if MEMF_CHIP and MEMF_FAST flags
	* are both set, but i'm not testing for that work-around unless
	* it really happens with some player/custom we know of.
	and.l	#$40000000,d2
	move.l	d2,(a1)+		* save hunk mem type
	move.l	(a2),d0			* take relocpoint
	and.b	#-8,d0			* align by 8
	addq.l	#8,d0
	move.l	d0,(a1)+		* save reloc addr for hunk
	add.l	d1,d0
	move.l	d0,(a2)			* put new relocpoint
	dbf	d7,hunkcheckloop

	lea	hunks(pc),a1
	move.l	nhunks(pc),d7
	subq	#1,d7
	bmi.b	nomorehunks

HunkLoop	push	d7/a1
HunkLoopTakeNext
	move.l	(a0)+,d1
	and.l	#$ffff,d1
	cmp.l	#$000003f1,d1
	bne.b	NotDebugHunk
	pushr	a0
	lea	debughunkwarn(pc),a0
	bsr	put_string
	pullr	a0
	move.l	(a0)+,d0
	lsl.l	#2,d0
	add.l	d0,a0
	bra.b	HunkLoopTakeNext
NotDebugHunk
	cmp.l	#$000003ea,d1
	beq	DataCodeHunk
	cmp.l	#$000003e9,d1
	beq	DataCodeHunk
	cmp.l	#$000003eb,d1
	beq	BSSHunk
hunklooperror	pull	d7/a1
	moveq	#-1,d0
	rts
conthunkloop	cmp.l	#$000003f2,(a0)+
	bne.b	hunklooperror
	pull	d7/a1
	add	#12,a1
	dbf	d7,HunkLoop
nomorehunks	move.l	hunks+8(pc),a0
	moveq	#0,d0
	rts
hunkerror	moveq	#-1,d0
	rts

hunksizewarn	dc.b	'hunk size warn',0
bsshunksizewarn	dc.b	'bss hunk size warn',0
hunksizeerr	dc.b	'hunk size error',0
symbolhunkwarn	dc.b	'hunk relocator: symbol hunk warning!',0
illegalhunkwarn	dc.b	'illegal hunk',0
debughunkwarn	dc.b	'hunk relocator: debug hunk warning!', 0
	even

DataCodeHunk	move.l	(a0)+,d0	* take hunk length (in long words)
	lsl.l	#2,d0

	* copy hunk data
	pushr	a1
	move.l	8(a1),a1
	bsr	memcopy
	add.l	d0,a0		* skip hunk data
	pullr	a1

	* if reported hunk size is bigger than copied hunk data,
	* fill the rest with zeros (piru said this is a must)
	move.l	(a1),d1
	sub.l	d0,d1
	beq.b	no_extra_in_hunk
	bpl.b	hunk_size_not_fucked_1
	push	all
	lea	hunksizeerr(pc),a0
	bsr	put_string
	pull	all
	bra.b	no_extra_in_hunk
hunk_size_not_fucked_1
	push	all
	lea	hunksizewarn(pc),a0
	bsr	put_string
	move.l	8(a1),a0		*   destination start address
	add.l	d0,a0			* + actual data size
	move.l	d1,d0			* extra size
	bsr	clearmem
	pull	all
no_extra_in_hunk

hunktailloop	cmp.l	#$000003ec,(a0)
	bne.b	noprogreloc
	addq.l	#4,a0
	pushr	a1
	move.l	8(a1),a1
	bsr	handlerelochunk
	pullr	a1
	bra.b	hunktailloop
noprogreloc	cmp.l	#$000003f7,(a0)
	bne.b	noprogreloc_3f7
	addq.l	#4,a0
	pushr	a1
	move.l	8(a1),a1
	bsr	handlerelochunk_3f7
	pullr	a1
	bra.b	hunktailloop
noprogreloc_3f7	cmp.l	#$000003f0,(a0)
	bne.b	nosymbolhunk
	push	all
	lea	symbolhunkwarn(pc),a0
	bsr	put_string
	pull	all
	addq.l	#4,a0
symbolhunkloop	move.l	(a0)+,d0
	beq.b	hunktailloop
	lsl.l	#2,d0
	add.l	d0,a0
	addq.l	#4,a0
	bra.b	symbolhunkloop
nosymbolhunk
	cmp.l	#$000003f1,(a0)
	bne.b	not_debug_hunk
	addq.l	#4,a0
	move.l	(a0)+,d0
	lsl.l	#2,d0
	add.l	d0,a0
	bra.b	hunktailloop
not_debug_hunk
	cmp.l	#$000003f2,(a0)
	beq	conthunkloop
	move.l	(a0),d0
	bsr	put_value
	lea	illegalhunkwarn(pc),a0
	bsr	put_string
	illegal

* Clear BSS hunk memory with zeros
BSSHunk	move.l	(a0)+,d0	* take hunk length (in long words)
	lsl.l	#2,d0
	cmp.l	(a1),d0
	beq.b	r_size_match_2
	push	all
	lea	bsshunksizewarn(pc),a0
	bsr	put_string
	pull	all
r_size_match_2	pushr	a0
	move.l	8(a1),a0	* get hunk address
	bsr	clearmem
	pullr	a0
	bra	conthunkloop

handlerelochunk	move.l	(a0)+,d0	* take number of reloc entries
	tst.l	d0
	bne.b	morereloentries
	rts
morereloentries	move.l	(a0)+,d1	* take index of associated hunk for
	lea	hunks(pc),a3	* following reloc entries
	mulu	#12,d1
	move.l	8(a3,d1),d2	* take reloced address for hunk
relochunkloop	move.l	(a0)+,d1	* take reloc entry (offset)
	add.l	d2,(a1,d1.l)	* add reloc base address
	subq.l	#1,d0
	bne.b	relochunkloop
	bra.b	handlerelochunk

* same as handle reloc hunk but takes 16 bit data word inputs
handlerelochunk_3f7
	moveq	#0,d0
	move	(a0)+,d0	* take number of reloc entries
	tst.l	d0
	bne.b	morereloentries_3f7
	rts
morereloentries_3f7
	moveq	#0,d1
	move	(a0)+,d1	* take index of associated hunk for
	lea	hunks(pc),a3	* following reloc entries
	mulu	#12,d1
	move.l	8(a3,d1),d2	* take reloced address for hunk
	moveq	#0,d1
relochunkloop_3f7
	move	(a0)+,d1	* take reloc entry (offset)
	add.l	d2,(a1,d1.l)	* add reloc base address
	subq.l	#1,d0
	bne.b	relochunkloop_3f7
	bra.b	handlerelochunk_3f7


exec_allocmem	push	d1-d7/a0-a6
	lea	chippoint(pc),a0
	move.l	d0,d2
	move.l	(a0),d0
	move.l	d0,d3
	add.l	d2,d3
	and.b	#$f0,d3
	add.l	#16,d3
	move.l	d3,(a0)
	* test if MEMF_CLEAR is set
	btst	#16,d1
	beq.b	nomemclear
	move.l	d2,d1
	beq.b	nomemclear
	move.l	d0,a0
memclearloop	clr.b	(a0)+
	subq.l	#1,d1
	bne.b	memclearloop
nomemclear	pull	d1-d7/a0-a6
	rts

myfreemem	rts

clearmem	movem.l	d0-d2/a0,-(a7)
	move.l	d0,d1
	lsr.l	#2,d0
	beq.b	noltr1
ltr1	clr.l	(a0)+
	subq.l	#1,d0
	bne.b	ltr1
noltr1	and	#$3,d1
	subq	#1,d1
	bmi.b	nobs1
ybs1	clr.b	(a0)+
	dbf	d1,ybs1
nobs1	movem.l	(a7)+,d0-d2/a0
	rts

memcopy	push	d0-d1/a0-a1
	move.l	d0,d1
	lsr.l	#2,d0
	beq.b	noltr2
ltr2	move.l	(a0)+,(a1)+
	subq.l	#1,d0
	bne.b	ltr2
noltr2	and	#$3,d1
	subq	#1,d1
	bmi.b	nobs2
ybs2	move.b	(a0)+,(a1)+
	dbf	d1,ybs2
nobs2	pull	d0-d1/a0-a1
	rts


dos_lock	push	all
	pushr	d1
	move.l	d1,a0
	lea	lastlock(pc),a1
	move.l	#256,d0
	bsr	strlcpy
	pullr	d1
	* d1 = filename
	move.l	#1005,d2	* MODE_OLDFILE
	bsr	dos_open
	tst.l	d0
	beq.b	dos_lock_failure
	move.l	d0,d1
	bsr	dos_close
	pull	all
	move.l	#$f0000000,d0
	rts
dos_lock_failure
	pull	all
	moveq	#0,d0
	rts

dos_currentdir	push	all
	lea	lastlock(pc),a0
	lea	curdir(pc),a1
	move.l	#256,d0
	bsr	strlcpy
	lea	curdirwarning(pc),a0
	bsr	put_string
	pull	all
	rts
curdirwarning	dc.b	'warning: using dos.library/CurrentDir()',0
fixfilewarning	dc.b	'warning: fixfilename',0
	even

* puts CurrentDirectory in front of the name if there is no : character in
* the name. Allocates a new name pointer if necessary.
* filename in a0. returns a (new) name in a0.
dos_fixfilename	push	d0-d7/a1-a6
	move.l	a0,a4
dos_ffn_sloop	move.b	(a0)+,d0
	cmp.b	#':',d0
	beq.b	dos_ffn_nothing
	tst.b	d0
	bne.b	dos_ffn_sloop
	move.l	#256,d0
	bsr	exec_allocmem
	move.l	d0,a5
	lea	curdir(pc),a0
	move.l	a5,a1
	move.l	#256,d0
	bsr	strlcpy
	move.l	a5,a0
	bsr	strendptr
	move.l	a0,a1
	move.l	a4,a0
	move.l	#256,d0
	bsr	strlcpy
	move.l	a5,a4
	lea	curdir(pc),a0
	tst.b	(a0)
	beq.b	dos_ffn_nothing
	lea	fixfilewarning(pc),a0
	bsr	put_string
	move.l	a4,a0
	bsr	put_string
dos_ffn_nothing	move.l	a4,a0
	pull	d0-d7/a1-a6
	rts

* contrary to amigaos convention dos_open returns a positive index to the
* file that is opened (zero of failure)
dos_open	push	d1-d7/a0-a6
	move.l	d1,a0
	bsr	dos_fixfilename
	move.l	a0,d1
	lea	dosopenmsg(pc),a0
	move.l	d1,4(a0)
	clr.l	12(a0)
	moveq	#dosopenmsge-dosopenmsg,d0
	bsr	put_message
	move.l	msgptr(pc),a0
	tst.l	12(a0)
	bne.b	dos_open_not_fail
	pull	d1-d7/a0-a6
	moveq	#0,d0
	rts
dos_open_not_fail
	* get free file index
	lea	dos_file_list(pc),a2
filelistloop1	tst.l	(a2)
	beq.b	fileopenerror
	tst.l	8(a2)
	beq.b	usethisfileindex
	add	#16,a2
	bra.b	filelistloop1
usethisfileindex
	* save a2
	* alloc mem for name
	move.l	#128,d0
	moveq	#0,d1
	bsr	exec_allocmem
	move.l	msgptr(pc),a0
	move.l	4(a0),d2	* name
	move.l	d0,d3		* new name space
	move.l	8(a0),d4	* filesize
	* copy file name
	move.l	d2,a0
	move.l	d3,a1
	moveq	#127,d0
	bsr	strlcpy
	move.l	(a2),d0		* get free file index
	clr.l	4(a2)		* clear file offset
	move.l	d3,8(a2)	* put name space ptr
	move.l	d4,12(a2)
	pull	d1-d7/a0-a6
	rts
fileopenerror	lea	tablefullmsg(pc),a0
	bsr	put_string
	pull	all
	rts
tablefullmsg	dc.b	'error: file table full',0
	even
dosopenmsg	dc.l	UADE_FILESIZE
	dc.l	0	* file name ptr
	dc.l	0	* file length
	dc.l	0	* file exists (uae returns, see msgptr+12)
dosopenmsge
dos_file_list	dc.l	1,0,0,0		* index, filepos, filenameptr, filesize
	dc.l	2,0,0,0
	dc.l	3,0,0,0
	dc.l	4,0,0,0
	dc.l	5,0,0,0
	dc.l	0

dos_seek	push	d1-d7/a0-a6
	and	#15,d1
	subq	#1,d1
	lsl	#4,d1
	lea	dos_file_list(pc),a2
	add	d1,a2
	move.l	8(a2),d0
	bne.b	seek_is_opened
	move.l	#$3570,d0
	bsr	put_value
	pull	d1-d7/a0-a6
	moveq	#-1,d0
	rts
seek_is_opened	move.l	4(a2),d0
	cmp	#1,d3
	bne.b	seek_not_end
	add.l	12(a2),d2
	move.l	d2,4(a2)
	bra.b	seek_done
seek_not_end	cmp	#-1,d3
	bne.b	seek_not_start
	move.l	d2,4(a2)
	bra.b	seek_done
seek_not_start	tst	d3
	bne.b	seek_not_cur
	add.l	d2,4(a2)
	bra.b	seek_done
seek_not_cur	move.l	#$3578,d0
	bsr	put_value
	pull	d1-d7/a0-a6
	moveq	#-1,d0
	rts
seek_done	pull	d1-d7/a0-a6
	rts


dosreadmsg	dc.l	UADE_READ
	* name ptr, dest, offset, length, r. length
	dc.l	0,0,0,0,0
dosreadmsge

dos_read	push	d1-d7/a0-a6
	and	#15,d1
	subq	#1,d1
	lsl	#4,d1
	lea	dos_file_list(pc),a2
	add	d1,a2
	move.l	8(a2),d0
	bne.b	read_is_opened
	move.l	#$3680,d0
	bsr	put_value
	pull	d1-d7/a0-a6
	moveq	#0,d0
	rts
read_is_opened	lea	dosreadmsg(pc),a0
	move.l	8(a2),4(a0)		* name ptr
	move.l	d2,8(a0)		* dest
	move.l	4(a2),12(a0)		* offset
	move.l	d3,16(a0)		* length
	clr.l	20(a0)			* clear actually read len
	moveq	#dosreadmsge-dosreadmsg,d0
	bsr	put_message
	move.l	msgptr(pc),a0
	move.l	20(a0),d0
	add.l	d0,4(a2)		* udpate opened file offset
	pull	d1-d7/a0-a6
	rts

dos_close	push	all
	and	#15,d1
	subq	#1,d1
	lsl	#4,d1
	lea	dos_file_list(pc),a2
	add	d1,a2
	move.l	8(a2),d0
	bne.b	close_is_opened
	move.l	#$3790,d0
	bsr	put_value
	bra.b	close_is_not_opened
close_is_opened	clr.l	8(a2)
close_is_not_opened
	pull	all
	moveq	#0,d0
	rts


loadsegwarnmsg	dc.b	'warning: this deliplayer uses loadseg()/dos.library',0
loadsegerrmsg	dc.b	'loadseg relocation error',0
	even

dos_loadseg	push	d1-d7/a0-a6
	push	d0/a0
	lea	loadsegwarnmsg(pc),a0
	bsr	put_string
	pull	d0/a0
	move.l	d1,a0
	tst.b	(a0)
	bne.b	myloadseg_loadfile
	move.l	moduleptr(pc),a0
	bra.b	myloadseg_noloading
myloadseg_loadfile
	lea	loadfilemsg(pc),a0
	move.l	d1,4(a0)
	move.l	chippoint(pc),8(a0)
	move.l	#loadfilemsge-loadfilemsg,d0
	bsr	put_message
	move.l	chippoint(pc),d0
	move.l	d0,a0
	pushr	a0
	move.l	msgptr(pc),a0
	move.l	12(a0),d1
	add.l	d1,d0
	and.l	#-16,d0
	add.l	#16,d0
	lea	chippoint(pc),a0
	move.l	d0,(a0)
	pullr	a0
myloadseg_noloading
	bsr	relocator
	tst.l	d0
	beq.b	loadsegsuccess
	lea	loadsegerrmsg(pc),a0
	bsr	put_string
	pull	d1-d7/a0-a6
	moveq	#0,d0
	rts
loadsegsuccess	move.l	a0,d0
	subq.l	#4,d0
	lsr.l	#2,d0
	pull	d1-d7/a0-a6
	tst.l	d0
	rts


* volume test is for debugging only
volumetest	lea	voltestbit(pc),a0
	tst.l	(a0)
	beq.b	novoltest2
	move	#64,aud0vol+custom
	move	#64,aud1vol+custom
	move	#64,aud2vol+custom
	move	#64,aud3vol+custom
novoltest2	rts

exec_supervisor_msg	dc.b	'warning: supervisor called',0
exec_superstate_msg	dc.b	'warning: superstate called',0
	even
exec_supervisor	push	all
	lea	exec_supervisor_msg(pc),a0
	bsr	put_string
	pull	all
	move.l	a5,TRAP_VECTOR_0
	trap	#0
	rts

exec_superstate	push	all
	lea	exec_superstate_msg(pc),a0
	bsr	put_string
	lea	superstate_kick(pc),a0
	move.l	a0,TRAP_VECTOR_0
	pull	all
	trap	#0
	illegal

superstate_kick	move.l	a7,d0		* d0 = SSP
	move.l	usp,a7
	rts

exec_userstate	move.l	a7,usp
	move.l	d0,a7
	move	#0,sr
	rts

timername1	dc.b	'timer.device',0
audioname1	dc.b	'audio.device',0
tderrmsg1	dc.b	'OpenDevice: unknown device',0
tderrmsg2	dc.b	'OpenDevice: only timer.device UNIT_VBLANK supported',0
tdmsg1	dc.b	'a device was opened',0
	even

exec_open_device	push	all
	push	d0/a1
	lea	timername1(pc),a1	* timer.device support
	bsr	strcmp
	tst.l	d0
	beq.b	istimerdev
	lea	audioname1(pc),a1
	bsr	strcmp
	tst.l	d0
	beq.b	isaudiodev
	pull	d0/a1
	lea	tderrmsg1(pc),a0
	bsr	put_string
	bra	dontplay

isaudiodev	pull	d0/a1
	cmp	#32,IO_COMMAND(a1)	* ACMD_ALLOCATE ?
	bne.b	addone
	tst.l	IO_LENGTH(a1)
	beq.b	addone
	moveq	#0,d0			* cook up a valid result
	move.l	IO_DATA(a1),a0
	move.b	(a0),d0			* first chan mask succeeds ;)
	move.l	d0,IO_UNIT(a1)
	tst.l	32(a1)		* give it a "unique" IOA_ALLOCKEY if needed
	bne.b	addone
	move.l	#'uniq',32(a1)
addone	lea	tdmsg1(pc),a0
	bsr	put_string
	pull	all
	clr.b	IO_ERROR(a1)		* no error
	moveq	#0,d0
	rts

istimerdev	pull	d0/a1
	cmp.b	#UNIT_VBLANK,d0
	beq.b	novblerr
	lea	tderrmsg2(pc),a0
	bsr	put_string
	bra	dontplay
novblerr	move.l	d0,IO_UNIT(a1)
	lea	tdmsg1(pc),a0
	bsr	put_string
	pull	all
	push	all
	lea	timerioptr(pc),a0
	move.l	a1,(a0)
	lea	vblanktimerbit(pc),a0
	st	(a0)
	lea	vblanktimerstatusbit(pc),a0
	clr.l	(a0)
	pull	all
	clr.b	IO_ERROR(a1)		* no error
	moveq	#0,d0
	rts

doiowarnmsg	dc.b	'warning: doing fake exec.library/DoIO',0
	even

exec_doio	push	all
	lea	doiowarnmsg(pc),a0
	bsr	put_string
	pull	all
	clr.b	IO_ERROR(a1)		* no error
	moveq	#0,d0
	rts


sendioerrmsg1	dc.b	'SendIO(): Unknown IORequest pointer...',0
sendioerrmsg2	dc.b	'SendIO(): Unknown IORequest command',0
sendiomsg1	dc.b	'SendIO(): TR_ADDREQUEST',0
	even

exec_sendio	push	all
	lea	timerioptr(pc),a0
	move.l	(a0),a0
	cmp.l	a0,a1
	beq.b	itstimerdev_1
	lea	sendioerrmsg1(pc),a0
	bsr	put_string
	bra	dontplay
itstimerdev_1	cmp	#TR_ADDREQUEST,IO_COMMAND(a1)
	beq.b	itstraddreq
	lea	sendioerrmsg2(pc),a0
	bsr	put_string
	bra	dontplay
itstraddreq
	* check if sendio general msg has already been sent once
	lea	sendiomsgbit(pc),a0
	tst	(a0)
	bne.b	dontsendiomsg
	st	(a0)
	lea	sendiomsg1(pc),a0
	bsr	put_string
dontsendiomsg	pull	all
	push	all
	move.l	IOTV_TIME+TV_SECS(a1),d0
	move.l	IOTV_TIME+TV_MICRO(a1),d1
	mulu	vbi_hz(pc),d0
	move.l	#1000000,d2
	divu	vbi_hz(pc),d2	* frame time in microsecs
	divu	d2,d1
	ext.l	d1
	add.l	d1,d0
	lea	vblanktimercount(pc),a0
	move.l	d0,(a0)
	lea	vblanktimerstatusbit(pc),a0
	st	(a0)
	lea	vblanktimerfunc(pc),a2
	move.l	MN_REPLYPORT(a1),a0
	move.l	MP_SIGTASK(a0),a0
	move.l	$12(a0),(a2)
	pull	all
	clr.b	IO_ERROR(a1)		* no error
	rts


waitiomsg1	dc.b	'warning: WaitIO is not implemented!',0
abortiomsg1	dc.b	'AbortIO is not implemented!',0
getmsgmsg1	dc.b	'getmsg is not properly implemented!',0
	even

exec_waitio	push	all
	lea	waitiomsg1(pc),a0
	bsr	put_string
	pull	all
	clr.b	IO_ERROR(a1)		* no error
	moveq	#0,d0
	rts

myabortio	push	all
	lea	abortiomsg1(pc),a0
	bsr	put_string
	pull	all
	moveq	#0,d0
	rts

mygetmsg	push	all
	lea	getmsgbit(pc),a0
	tst	(a0)
	bne.b	nogetmsg_1
	lea	getmsgmsg1(pc),a0
	bsr	put_string
	lea	getmsgbit(pc),a0
	st	(a0)
nogetmsg_1	pull	all
	moveq	#0,d0
	rts

* exec.library: _LVOCause (a1 = struct Interrupt *)
exec_cause	push	all
	move.l	$12(a1),d0
	beq.b	not_soft_irq
	move.l	d0,a0
	move	#$2100,d0
	lea	trapcall(pc),a1
	move.l	a1,TRAP_VECTOR_3
	trap	#3
not_soft_irq	pull	all
	rts

trapcall	move	d0,sr
	jsr	(a0)
	move	#$2000,sr
	rte

dos_library_name	dc.b	'dos.library',0
uade_library_name	dc.b	'uade.library',0
	even

exec_old_open_library
exec_open_library
	push	d1-d7/a0-a6
	moveq	#0,d0
	lea	dos_library_name(pc),a0
	bsr	strcmp
	tst.l	d0
	bne.b	not_dos_lib
	bsr	send_open_lib_msg
	lea	dos_lib_base(pc),a0
	move.l	a0,d0
	bra.b	return_open_lib
not_dos_lib	lea	uade_library_name(pc),a0
	bsr	strcmp
	tst.l	d0
	bne.b	not_uade_lib
	bsr	send_open_lib_msg
	lea	uade_lib_base(pc),a0
	move.l	a0,d0
	bra.b	return_open_lib
not_uade_lib	move.l	a1,a0
	lea	openlibwarnname(pc),a1
	moveq	#31,d0
	bsr	strlcpy
	lea	openlibwarnmsg(pc),a0
	bsr	put_string
	moveq	#0,d0
return_open_lib	pull	d1-d7/a0-a6
	tst.l	d0
	rts

send_open_lib_msg	push	all
	lea	openlibname(pc),a1
	moveq	#31,d0
	bsr	strlcpy
	lea	openlibmsg(pc),a0
	bsr	put_string
	pull	all
	rts

openlibmsg	dc.b	'open library '
openlibname	dcb.b	32,0
openlibwarnmsg	dc.b	'warning: couldnt open library '
openlibwarnname	dcb.b	32,0
	even

liboffscheck	pushr	d0
	move.l	8(a7),d0
	push	all
	move.l	d0,a0
	subq.l	#4,a0
	move	(a0),d0
	and	#$4ea0,d0
	cmp	#$4ea0,d0
	bne.b	nolibjsr
	move	2(a0),d0
	ext.l	d0
	neg.l	d0
	bsr	put_value
nolibjsr	pull	all
	pullr	d0
	rts

intuiwarn	bsr	liboffscheck
	push	all
	lea	intuiwarnmsg(pc),a0
	bsr	put_string
	pull	all
	rts
intuiwarnmsg	dc.b	'warning: intuition library function not implemented',0
	even

* a0 base, a1 warn funct, d0 = abs(minimum offset)
exec_initlibbase
	push	all
	move.l	d0,d6
	moveq	#6,d0
	subq.l	#6,a0
initlibloop	cmp.l	d6,d0
	bgt.b	endlibloop
	move	jmpcom(pc),(a0)
	move.l	a1,2(a0)
	subq.l	#6,a0
	addq.l	#6,d0
	bra.b	initlibloop
endlibloop	pull	all
	rts


exec_typeofmem	move.l	a1,d0
	bmi.b	exec_typeofmem_fail
	cmp.l	#$200000,d0
	bge.b	exec_typeofmem_fail
	moveq	#3,d0			* MEMF_PUBLIC | MEMF_CHIP
	rts
exec_typeofmem_fail
	moveq	#0,d0
	rts

exec_findtask	pushr	a0
	lea	findtaskwarn(pc),a0
	bsr	put_string
	pullr	a0
	move.l	#$deadface,d0
	rts

findtaskwarn	dc.b	'exec.library/findtask() not good',0
allocsigwarn	dc.b	'exec.library/allocsignal() not good',0
signalwarn	dc.b	'exec.library/Signal() not good',0
	even
exec_sig_mask	dc.l	$0000ffff
exec_allocsignal	pushr	a0
	lea	allocsigwarn(pc),a0
	bsr	put_string
	pullr	a0
	push	d1/a0
	lea	exec_sig_mask(pc),a0
	move.l	(a0),d1
	cmp.l	#-1,d0
	bne.b	specificsignal
	moveq	#31,d0
allocsigloop	btst	d0,d1
	beq.b	allocsigret
	dbf	d0,allocsigloop
	moveq	#-1,d0
	bra.b	allocsigret

specificsignal	btst	d0,d1
	beq.b	signalok
	moveq	#-1,d0
	bra.b	allocsigret

signalok	bset	d0,d1
	move.l	d1,(a0)
allocsigret	pull	d1/a0
	rts

exec_signal	pushr	a0
	lea	signalwarn(pc),a0
	bsr	put_string
	pullr	a0
	rts

intui_allocremember
	move.l	#$666,(a0)	* mark success ;-)
	bra	exec_allocmem

uade_time_critical
	push	all
	lea	uade_tc_msg(pc),a0
	move.l	d0,4(a0)
	moveq	#8,d0
	bsr	put_message
	pull	all
	rts

uade_tc_msg	dc.l	UADE_TIME_CRITICAL,0

uade_get_info
	push	d1-d7/a0-a6
	lea	uade_gi_msg(pc),a2
	move.l	a0,4(a2)
	move.l	a1,8(a2)
	move.l	d0,12(a2)
	move.l	a2,a0
	moveq	#16,d0
	bsr	put_message
	move.l	msgptr(pc),a0
	move.l	12(a0),d0
	pull	d1-d7/a0-a6
	rts

* a0 = option array for returned values
* array format:
* dc.l	opttype,optname,optreceived,optvalue
* opttype = 0 (end of list), 1 (string), 2 hexadecimal value (at most 32 bits),
*           3 (flag)
*
* optname is a pointer to the name of the value
*
* optreceived is a pointer to a byte that indicates whether or not that
* option was found. 0 means not found, otherwise found.
*
* optvalue is a pointer to a place where the value should be stored.
* if option type is "flag", optvalue is not needed, because optreceived
* already indicates if that flag was received. "hexadecimal value" is
* returned as a 32 bit integer despite the length of hexadecimal string
* given by the user.
uade_new_get_info
	push	d1-d7/a0-a6
	move.l	a0,a6			* option array for eagleplayer
	lea	uade_gi_msg(pc),a2
	lea	ep_opt_request(pc),a0
	move.l	a0,4(a2)
	lea	uadelibmsg(pc),a1
	move.l	a1,8(a2)
	move.l	#EP_OPT_SIZE,d0
	move.l	d0,12(a2)
	move.l	a2,a0
	moveq	#16,d0
	bsr	put_message
	move.l	msgptr(pc),a0
	move.l	12(a0),d0
	tst.l	d0
	ble.b	no_info

	move.l	d0,d2

	lea	uadelibmsg(pc),a3
get_ep_info_loop
	move.l	a6,a2
get_ep_info_loop2
	move.l	(a2)+,d7	* opt type
	beq.b	end_get_ep_info_loop2

	; the eagleoption in eagleoptlist can be shorter than the
	; actual given option given from uade. this happens when
	; eagleoption gets a value, like ciatempo=150. to handle
	; this, eagleopt list would only contain "ciatempo" and
	; the following code only compares strlen("ciatempo")
	; amount of bytes from the data given from uade.
	move.l	(a2)+,a0	* opt name
	bsr	strlen

	move.l	(a2)+,a4	* opt received
	
	move.l	(a2)+,a5	* opt value

	; We now have eagleopt length in d0 and the eagle option name
	; pointer is still in a0 (strlen preservers registers).
	; Do a limited string comparison against the eagleoption
	; and data given from uade
	move.l	a3,a1		; uade lib message
	bsr	strncmp
	bne.b	get_ep_info_loop2

	; woah! equal! celebrate it by spamming the user:
	move.l	a3,a0
	bsr	put_string

	; string options may not be longer than 31 characters (32 bytes with
	; null byte)
	move.l	a3,a0
	bsr	strlen
	cmp.l	#31,d0
	bgt	invalid_ep_opt

	;  mark option as received
	st	(a4)

	cmp	#1,d7
	bne.b	ep_opt_not_a_string
	move.l	a3,a0
	move.l	a5,a1
	moveq	#32,d0
	bsr	strlcpy
	bra.b	end_get_ep_info_loop2
ep_opt_not_a_string
	cmp	#2,d7
	bne.b	end_get_ep_info_loop2
	; skip "foo=" string for a hexadecimal value, where foo is an option
	; name
	move.l	-12(a2),a0	;  get option name
	bsr	strlen
	addq.l	#1,d0		;  add one more to skip '='
	lea	(a3,d0),a0
	bsr	hex2int
	move.l	d0,(a5)

end_get_ep_info_loop2
	; get next option from response, or quit if no more eagleoptions.
	move.l	a3,a0
	bsr	strlen
	addq.l	#1,d0
	add.l	d0,a3
	sub.l	d0,d2
	bpl.b	get_ep_info_loop

	pull	d1-d7/a0-a6
	moveq	#1,d0
	rts
no_info	pull	d1-d7/a0-a6
	moveq	#0,d0
	rts

invalid_ep_opt
	lea	invalid_ep_opt_msg(pc),a0
	bsr	put_string
	bra	end_get_ep_info_loop2

uade_gi_msg	dc.l	UADE_GET_INFO,0,0,0
ep_opt_request	dc.b	'eagleoptions',0
invalid_ep_opt_msg	dc.b	'invalid ep option received',0

ciareswarnmsg	dc.b	'exec.library/OpenDevice: unknown resource',0
ciaawarnmsg	dc.b	'warning: ciaa resource opened',0
	even
exec_open_resource
	cmp.b	#'c',(a1)
	bne.b	nociabresource
	cmp.b	#'i',1(a1)
	bne.b	nociabresource
	cmp.b	#'a',2(a1)
	bne.b	nociabresource
	cmp.b	#'b',3(a1)
	beq.b	isciabresource
	cmp.b	#'a',3(a1)
	beq.b	isciaaresource
nociabresource	lea	ciareswarnmsg(pc),a0
	bsr	put_string
	moveq	#0,d0
	rts

isciaaresource	lea	ciaaresjmptab(pc),a0
	lea	illegalciaaresource(pc),a1
	moveq	#10-1,d0
ciaaresjmptabl	move.l	a1,2(a0)
	addq.l	#6,a0
	dbf	d0,ciaaresjmptabl
	lea	ciaaresource(pc),a0
	lea	sys_add_ciaa_interrupt(pc),a1
	move.l	a1,_LVOAddICRVector+2(a0)
	lea	rem_ciaa_interrupt(pc),a1
	move.l	a1,_LVORemICRVector+2(a0)
	lea	ciaaresource(pc),a0
	move.l	a0,d0
	rts

isciabresource	lea	ciabresjmptab(pc),a0
	lea	illegalciabresource(pc),a1
	moveq	#10-1,d0
ciabresjmptabl	move.l	a1,2(a0)
	addq.l	#6,a0
	dbf	d0,ciabresjmptabl
	lea	ciabresource(pc),a0
	lea	add_ciab_interrupt(pc),a1
	move.l	a1,_LVOAddICRVector+2(a0)
	lea	rem_ciab_interrupt(pc),a1
	move.l	a1,_LVORemICRVector+2(a0)
	lea	ciab_seticr(pc),a1
	move.l	a1,_LVOSetICR+2(a0)
	lea	ciab_ableicr(pc),a1
	move.l	a1,_LVOAbleICR+2(a0)
	lea	ciabresource(pc),a0
	move.l	a0,d0
	rts

ciab_ableicr
ciab_seticr	push	all
	lea	icrwarnmsg(pc),a0
	bsr	put_string
	pull	all
	moveq	#0,d0
	rts

icrwarnmsg	dc.b	'warning: not implemented ciab.resource/SetICR or '
	dc.b	'AbleICR was used',0
illciaamsg	dc.b	'ciaaresource: resource is not implemented!', 0
illciabmsg	dc.b	'ciabresource: resource is not implemented!', 0
	even
illegalciaaresource	push	all
	lea	illciaamsg(pc),a0
	bra.b	disillciamsg
illegalciabresource	push	all
	lea	illciabmsg(pc),a0
disillciamsg	bsr	put_string
	pull	all
	rts

ciaaresjmptab	rept	10
	jmp	0
	endr
ciaaresource
ciabresjmptab	rept	10
	jmp	0
	endr
ciabresource

* AddICRVector() for ciaa.resource and ciab.resource
* sets hw int vector, cia registers, and enables a ciab interrupt
*
* deliciabdata is passed in a1 to deliciabint function
*
* SHOULD WE READ deliciabdata from interrupt structure every time we do the
* interrupt?
add_ciab_interrupt
	push	all
	lea	ciabdatas(pc),a2
	lea	ciabints(pc),a3
	lea	$bfd000,a4
	move	#$2000,d2
	lea	ciab_interrupt(pc),a5
	move.l	a5,$78.w
	bra.b	add_cia_interrupt
sys_add_ciaa_interrupt
	moveq	#-1,d0
	rts
add_ciaa_interrupt
	push	all
	lea	ciaadatas(pc),a2
	lea	ciaaints(pc),a3
	lea	$bfe001,a4
	move	#$0008,d2
	lea	ciaa_interrupt(pc),a5
	move.l	a5,$68.w
add_cia_interrupt
	moveq	#1,d1
	and.l	d0,d1
	lsl	#2,d1
	move.l	$e(a1),(a2,d1)
	move.l	$12(a1),(a3,d1)

	lea	eaglebase(pc),a5
	move	dtg_Timer(a5),d1

	btst	#0,d0
	bne.b	bit0one
	move.b	d1,$400(a4)		* Set A Timer value
	rol	#8,d1
	move.b	d1,$500(a4)
	move.b	#$81,$d00(a4)		* set timer A ON
	move.b	#$11,$e00(a4)		* A timer on
	bra.b	bit0zero
bit0one	move.b	d1,$600(a4)		* Set B Timer value
	rol	#8,d1
	move.b	d1,$700(a4)
	move.b	#$82,$d00(a4)		* set timer B ON
	move.b	#$11,$f00(a4)		* B timer on
bit0zero
	or	#$8000,d2
	move	d2,intena+custom	* enable cia interrupt
	pull	all
	moveq	#0,d0
	rts


rem_ciaa_interrupt
	push	all
	lea	ciaadatas(pc),a2
	lea	ciaaints(pc),a3
	lea	$bfe001,a4
	bra.b	ciaremint

rem_ciab_interrupt
	push	all
	lea	ciabdatas(pc),a2
	lea	ciabints(pc),a3
	lea	$bfd000,a4
ciaremint	and	#1,d0
	moveq	#1,d1
	lsl	d0,d1
	move.b	d1,$d00(a4)		* Disable A or B timer in ICR
	lsl	#2,d0
	move.l	#0,(a2,d0)
	move.l	#0,(a3,d0)
	pull	all
	moveq	#0,d0
	rts


* register setup for calling ciab interrupt
* a1 = ciab interrupt data pointer
* a6 = exec base
* IS THIS RIGHT? Should we hit intreq+custom after the interrupt is executed?

ciaa_interrupt	push	all
	move	#$0008,intreq+custom * quit the int to be sure
	move.b	$bfed01,d0	* quit int (reading should do it)
	and	#3,d0
	btst	#0,d0
	beq.b	ciaa_not_timer_a
	move.l	ciaaints(pc),a0
	move.l	ciaadatas(pc),a1
	move.l	4.w,a6
	move.l	d0,-(a7)
	jsr	(a0)
	move.l	(a7)+,d0
ciaa_not_timer_a
	btst	#1,d0
	beq.b	ciaa_not_timer_b
	move.l	ciaaints+4(pc),a0
	move.l	ciaadatas+4(pc),a1
	move.l	4.w,a6
	move.l	d0,-(a7)
	jsr	(a0)
	move.l	(a7)+,d0
ciaa_not_timer_b
	pull	all
	rte

ciab_interrupt	push	all
	move	#$2000,intreq+custom * quit the int to be sure
	move.b	$bfdd00,d0	* quit int (reading should do it)
	and	#3,d0
	btst	#0,d0
	beq.b	ciab_not_timer_a
	move.l	ciabints(pc),a0
	move.l	ciabdatas(pc),a1
	move.l	4.w,a6
	move.l	d0,-(a7)
	jsr	(a0)
	move.l	(a7)+,d0
ciab_not_timer_a
	btst	#1,d0
	beq.b	ciab_not_timer_b
	move.l	ciabints+4(pc),a0
	move.l	ciabdatas+4(pc),a1
	move.l	4.w,a6
	move.l	d0,-(a7)
	jsr	(a0)
	move.l	(a7)+,d0
ciab_not_timer_b
	pull	all
	rte

AMP_ADR_CHANGED	equ	1
AMP_LEN_CHANGED	equ	2
AMP_PER_CHANGED	equ	4
AMP_VOL_CHANGED	equ	8

	rsreset
amp_adr	rs.l	1
amp_len	rs.l	1
amp_per	rs	1
amp_vol	rs	1
amp_changes	rs.l	1
amp_entry_size	rs.b	0

amp_dma	dc	0,0	* disable/enable masks
amp_struct	dcb.b	amp_entry_size*4,0

amplifierwarn	dc.b	'warning: amplifier used',0
ampinitfailure	dc.b	'error: amplifier init failed',0
	even

* amplifier_init must be called after DTP_InitPlayer. See Future Composer 1.4
call_amplifier_init
	move.l	amplifier_init_func(pc),d0
	bne.b	is_an_amp
	rts
is_an_amp	push	all
	lea	amplifierwarn(pc),a0
	bsr	put_string
	move.l	amplifier_init_func(pc),a0
	jsr	(a0)
	tst.l	d0
	beq.b	amp_init_ok
	lea	ampinitfailure(pc),a0
	bsr	put_string
amp_init_ok	pull	all
	rts

amp_is_valid_channel
	cmp	#4,d1
	bhs.b	amp_not_valid_channel
	rts
amp_chan_warning	dc.b	'warning: illegal chan number (amp)',0
	even
amp_not_valid_channel
	push	all
	lea	amp_chan_warning(pc),a0
	bsr	put_string
	pull	all
	rts

amplifier_dma	push	all
	lea	amp_dma(pc),a0
	and	#$000f,d1
	tst	d0
	bpl.b	amp_dma_not_neg
	or	#$8000,d1
amp_dma_not_neg	move	d1,dmacon+custom
	tst	d0
	bpl.b	amp_dma_not_neg_2
	bsr	wait_audio_dma
amp_dma_not_neg_2
	pull	all
	rts

amplifier_adr	push	all
	bsr	amp_is_valid_channel
	move.l	d1,d2
	lsl	#4,d2
	lea	aud0lch+custom,a0
	move.l	d0,(a0,d2)
	pull	all
	rts

amplifier_len	push	all
	bsr	amp_is_valid_channel
	move.l	d1,d2
	lsl	#4,d2
	lea	aud0len+custom,a0
	move	d0,(a0,d2)
	pull	all
	rts

amplifier_per	push	all
	bsr	amp_is_valid_channel
	move.l	d1,d2
	mulu	#$10,d2
	lea	aud0per+custom,a0
	move	d0,(a0,d2)
	pull	all
	rts

amplifier_vol	push	all
	bsr	amp_is_valid_channel
	move.l	d1,d2
	lsl	#4,d2
	lea	aud0vol+custom,a0
	move	d0,(a0,d2)
	pull	all
	rts

amp_com_unk_msg	dc.b	'unknown ENPP_PokeCommand() command',0
amp_com_filt_msg	dc.b	'unknown ENPP_PokeCommand() filter command',0
	even

amplifier_command
	push	all
	cmp.l	#1,d0
	bne.b	amp_com_unknown
	cmp.l	#0,d1
	bne.b	amp_com_not_filt_off
	bset	#1,$bfe001
	bra.b	amp_com_end
amp_com_not_filt_off
	cmp.l	#1,d1
	bne.b	amp_com_not_filt_on
	bclr	#1,$bfe001
	bra.b	amp_com_end
amp_com_not_filt_on
	cmp.l	#-1,d1
	bne.b	amp_com_not_filt_toggle
	bchg	#1,$bfe001
	bra.b	amp_com_end
amp_com_not_filt_toggle
	lea	amp_com_filt_msg(pc),a0
	bsr	put_string
	bra.b	amp_com_end
amp_com_unknown	lea	amp_com_unk_msg(pc),a0
	bsr	put_string
amp_com_end	pull	all
	rts

amplifier_int	push	all
	lea	amp_struct(pc),a0
	moveq	#4-1,d7
amp_clr_loop	clr.l	amp_changes(a0)
	add	#amp_entry_size,a0
	dbf	d7,amp_clr_loop
	lea	amp_dma(pc),a0
	clr.l	(a0)		* disable / enable
	pull	all
	rts

* Noteplayer initialization *
np_init	push	all
	move.l	noteplayerptr(pc),d0
	beq	np_not_an_np

	push	all
	lea	noteplayerwarn(pc),a0
	bsr	put_string
	move.l	noteplayersetupfunc(pc),d0
	beq.b	no_np_setup
	move.l	d0,a0
	jsr	(a0)
	tst.l	d0
	beq.b	no_np_setup
	move.l	#$06660666,d0
	bsr	put_value
no_np_setup	pull	all

	move.l	d0,a0
	move.l	(a0),a0
	lea	notestructptr(pc),a1
	move.l	a0,(a1)
	move.l	(a0),a1
	lea	np_chanlist(pc),a2
	move.l	a1,(a2)
	moveq	#0,d0
	move	6(a0),d0
	and	#$0020,d0
	lea	np_longsamples(pc),a2
	move.l	d0,(a2)
	move.l	#$00010000,d2			* short sample (1 word)
	tst	d0
	beq.b	np_not_long
	moveq	#2,d2				* long sample (2 bytes)
np_not_long	lea	np_zerosample(pc),a2
	moveq	#1,d1
np_count_channels
;	move.b	#2+8+$10,npc_modified(a1)	* set sample, per, vol
;	move.l	a2,npc_sampleptr(a1)
;	move.l	d2,npc_samplelen(a1)
;	clr.l	npc_srepeatptr(a1)
;	clr.l	npc_srepeatlen(a1)
;	move	#200,npc_period(a1)
;	move	#64,npc_volume(a1)
	move.l	(a1),d0
	beq.b	np_end_channel_count
	move.l	d0,a1
	addq.l	#1,d1
	bra.b	np_count_channels
np_end_channel_count
	lea	np_chans(pc),a2
	move.l	d1,(a2)
np_not_an_np	pull	all
	rts

noteplayerwarn	dc.b	'noteplayer warning',0
np_multichan_warning	dc.b	'noteplayer error: multichannel song',0
	even

np_zerosample	dc.l	0

* noteplayer interrupt routine *
np_int	push	all

	lea	custom,a6

	move.l	np_counter(pc),d0
	bne.b	np_counter_nz
	move	#$000f,dmacon(a6)
	bsr	wait_audio_dma
	lea	np_zerosample(pc),a1
	move.l	a6,a5
	moveq	#4-1,d7
np_zsloop	move.l	a1,aud0lch(a5)
	move	#1,aud0len(a5)
	move	#0,aud0vol(a5)
	move	#200,aud0per(a5)
	add	#$10,a5
	dbf	d7,np_zsloop
	move	#$800f,dmacon(a6)
	bsr	wait_audio_dma
np_counter_nz	lea	np_counter(pc),a0
	addq.l	#1,(a0)

	move.l	np_chanlist(pc),d0
	move.l	np_longsamples(pc),d5
	moveq	#0,d6		* dma on mask
	moveq	#0,d7		* audio channel bit number
	lea	np_chanset(pc),a5
np_int_loop	move.l	d0,a0
	cmp	#$8000,npc_chanpos(a0)
	beq.b	np_not_active
	move.b	npc_modified(a0),d0
	beq.b	np_no_changes

	btst	#1,d0
	beq.b	np_no_sample
	move.l	npc_sampleptr(a0),aud0lch(a6)
	tst.l	d5		* check if long samples
	bne.b	np_l_sample_1
	move	npc_samplelen(a0),aud0len(a6)
	bra.b	np_s_sample_1
np_l_sample_1	move.l	npc_samplelen(a0),d1
	lsr.l	#1,d1
	move	d1,aud0len(a6)
np_s_sample_1	st	(a5,d7)		* set sample repeat boolean
	bset	d7,d6		* set audio channel dma bit
np_no_sample
	btst	#2,d0
	beq.b	np_no_repeat
;	btst	#1,d0
;	bne.b	np_there_was_a_sample
;	move.l	npc_srepeatptr(a0),aud0lch(a6)
;	tst.l	d5		* check if long samples
;	bne.b	np_l_sample_2
;	move	npc_srepeatlen(a0),aud0len(a6)
;	bra.b	np_s_sample_2
;np_l_sample_2	move.l	npc_srepeatlen(a0),d1
;	lsr.l	#1,d1
;	move	d1,aud0len(a6)
;np_s_sample_2	bset	d7,d6		* set audio channel dma bit
;	bra.b	np_no_repeat
;np_there_was_a_sample
	st	(a5,d7)		* set sample repeat boolean
np_no_repeat
	btst	#3,d0
	beq.b	np_no_period
	move	npc_period(a0),aud0per(a6)
np_no_period
	btst	#4,d0
	beq.b	np_no_volume
	move	npc_volume(a0),aud0vol(a6)
np_no_volume
np_no_changes	clr.b	npc_modified(a0)
	add	#$10,a6
	addq	#1,d7
np_not_active	move.l	(a0),d0
	bne	np_int_loop

	cmp	#4,d7
	ble.b	np_4_chan
	lea	np_multichan_warning(pc),a0
	bsr	put_string
	and	#$000f,d6
np_4_chan
	tst.b	d6
	beq.b	np_no_dma_set
	move	d6,dmacon+custom
	bsr	wait_audio_dma
	or	#$8000,d6
	move	d6,dmacon+custom	* set relevant audio channels
np_no_dma_set
	move.l	np_longsamples(pc),d5
	lea	np_chanset(pc),a5
	move.l	np_chanlist(pc),d0
	lea	custom,a6
	moveq	#0,d7			* audio dma wait boolean
np_rloop	move.l	d0,a0
	cmp	#$8000,npc_chanpos(a0)
	beq.b	np_not_active_2
	tst.b	(a5)			* check if audxlch was set
	beq.b	np_no_repeat_2		* not set => dont set repeat
	move.l	npc_srepeatptr(a0),d0
	beq.b	np_no_repeat_2
	tst.l	d7		* check if audio dma has been waited
	bne.b	np_r_no_wait	* (audio dma should waited only once)
	st	d7
	bsr	wait_audio_dma
np_r_no_wait	move.l	d0,aud0lch(a6)
	tst.l	d5		* check if long samples
	bne.b	np_l_sample_3
	move	npc_srepeatlen(a0),aud0len(a6)
	bra.b	np_s_sample_3
np_l_sample_3	move.l	npc_srepeatlen(a0),d1
	lsr.l	#1,d1
	move	d1,aud0len(a6)
np_s_sample_3
np_no_repeat_2	clr.b	(a5)+			* clear channel sample repeat
	add	#$10,a6
np_not_active_2	move.l	(a0),d0
	bne.b	np_rloop
	pull	all
	rts


init_interrupts	push	all
	lea	mylevel2(pc),a0		* set CIAA int vector
	move.l	a0,$68.w
	lea	mylevel3(pc),a0		* set VBI vector
	move.l	a0,$6c.w
	lea	mylevel6(pc),a0		* set CIAB int vector
	move.l	a0,$78.w
	move	#$c000,d0
	or	#$0020,d0		* enable CIAA, CIAB and VBI
	move	d0,intena+custom

	lea	handlertab(pc),a0
	lea	mylevel1(pc),a1
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	lea	mylevel2(pc),a1
	move.l	a1,(a0)+
	lea	mylevel3(pc),a1
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	lea	mylevel4(pc),a1
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	lea	mylevel5(pc),a1
	move.l	a1,(a0)+
	move.l	a1,(a0)+
	lea	mylevel6(pc),a1
	move.l	a1,(a0)+
	pull	all
	rts

intvecmsg	dc.b	'setintvector(): Tried to set unauthorized interrupt '
	dc.b	'vector !',0
	even

exec_set_int_vector
	push	d1-d7/a0-a6
	cmp	#7,d0		* validity check
	blt.b	int_vec_error
	cmp	#10,d0
	ble.b	int_level_ok
int_vec_error	lea	intvecmsg(pc),a0
	bsr	put_string
	bra	dontplay
int_level_ok
	move	#$8000,d6
	bset	d0,d6		* enabling value for intena
	lea	vectab(pc),a0	* table containing vector addresses
	lea	irqlines(pc),a2	* table containing vectors
	lea	handlertab(pc),a3 * table containing my handlers
	lea	isdatapointers(pc),a4

	add	d0,d0
	move	(a0,d0),d2
	and.l	#$ff,d2		* get int vec address
	move.l	d2,a5		* a5 = hw interrupt pointer
				* eg. $6C == VBI interrupt pointer
	add	d0,d0
	move.l	(a3,d0),(a5)	* set my own int handler as hw int ptr
	move.l	(a2,d0),-(a7)	* get old int vector
	move.l	$0E(a1),(a4,d0)	* copy is_data pointer
	move.l	$12(a1),(a2,d0)	* put new vector into list

	lea	oldstructs(pc),a0
	add	d0,a0
	move.l	(a7)+,$12(a0)
	move.l	a0,d0		* return old int structure

	move	d6,intena+custom

	pull	d1-d7/a0-a6
	rts

addintmsg	dc.b	'addintserver(): Tried to add unauthorized interrupt '
	dc.b	'server !',0
	even

exec_add_int_server
	push	all
	cmp.b	#5,d0
	beq.b	servlevok
	lea	addintmsg(pc),a0
	bsr	put_string
	bra	dontplay
servlevok	lea	lev3serverlist(pc),a0
skipslistl	move.l	(a0),d0
	addq.l	#8,a0
	tst.l	d0
	bne.b	skipslistl
	move.l	$12(a1),-8(a0)	* interrupt vector pointer
	move.l	$e(a1),-4(a0)	* interrupt data pointer
	clr.l	(a0)
	pull	all
	moveq	#0,d0
	rts


mylevel1	move	#$0007,intreq+custom
	rte


mylevel2	move	#$0008,intreq+custom
	pushr	d0
	move.b	$bfed01,d0
	pullr	d0
	rte

mylevel3	btst	#5,intreqr+custom+1
	beq.b	is_not_vbi
	push	d0/a0/a5
	* add frame counter
	lea	framecount(pc),a0
	addq.l	#1,(a0)
	* interrupt server
	lea	lev3serverlist(pc),a0
server5loop	move.l	(a0),d0			* interrupt server pointer
	beq.b	endserver5list
	push	all
	move.l	4(a0),a1		* interrupt server data pointer
	move.l	d0,a5
	jsr	(a5)
	pull	all
	addq.l	#8,a0
	bra.b	server5loop
endserver5list	pull	d0/a0/a5
is_not_vbi	move	#$0070,intreq+custom
	rte

mylevel4	push	all
	lea	$dff000,a2
	move	intenar(a2),d2
	btst	#14,d2
	bne.b	mylevel4_ints_enabled
	lea	mylevel4_dismsg(pc),a0
	bsr	put_string
	pull	all
	rte
mylevel4_dismsg	dc.b	'audio interrupt taken but interrupts enabled.. hmm.. '
	dc.b	'please report this!',0
	even

mylevel4_ints_enabled
	lea	irqlines(pc),a3
	lea	isdatapointers(pc),a4
mylevel4_begin	moveq	#0,d2
	move	intenar(a2),d2
	and	intreqr(a2),d2
	moveq	#0,d7
mylevel4_loop	move	mylevel4_int_seq(pc,d7),d6	* aud0int bit
	bmi.b	mylevel4_end_loop
	addq	#2,d7
	btst	d6,d2
	beq.b	mylevel4_loop
	move.l	d6,d1
	lsl	#2,d1
	move.l	(a3,d1),d0
	beq.b	mylevel4_no_int_handler
	move.l	d0,a5
	move.l	(a4,d1),a1		* a1 = IS_DATA
	move.l	a2,a0			* a0 = $dff000
	move.l	d2,d1			* d1 = intena & intreq
	move.l	4.w,a6			* a6 = exec base
	jsr	(a5)
mylevel4_end_loop
	move	intenar(a2),d0
	and	intreqr(a2),d0
	and	#$0780,d0
	bne.b	mylevel4_begin
endmylevel4	pull	all
	rte

mylevel4_no_int_handler
	moveq	#0,d0
	bset	d6,d0
	move	d0,intreq(a2)
	lea	virginaudioints(pc),a0
	tst.l	(a0)
	bne.b	mylevel4_end_loop
	st	(a0)
	lea	mylevel4_msg(pc),a0
	bsr	put_string
	bra.b	mylevel4_end_loop

mylevel4_int_seq
	dc	8, 10, 7, 9, -1	* order of audio interrupt execution

mylevel4_msg	dc.b	'audio interrupt not handled',0
	even


mylevel5	move	#$1800,intreq+custom
	rts


mylevel6	move	#$2000,intreq+custom
	pushr	d0
	move.b	$bfdd00,d0	* quit int (reading should do it)
	pullr	d0
	rte


*		0 1 2 3 4 5 6 7 8 9 A B C D
irqtab	dc	1,1,1,2,3,3,3,4,4,4,4,5,5,6
vectab	dcb	3,$64
	dcb	1,$68
	dcb	3,$6c
	dcb	4,$70
	dcb	2,$74
	dcb	1,$78
* FOLLOWING LINES MUST BE SET TO MyLevel1,MyLevel2, ... (in init_interrupts())
handlertab	dcb.l	3,0
	dcb.l	1,0
	dcb.l	3,0
	dcb.l	4,0
	dcb.l	2,0
	dcb.l	1,0
irqlines	dcb.l	14,0
isdatapointers	dcb.l	14,0
oldstructs	dcb.b	$12+16*4,0

lev3serverlist	dcb.l	32,0

* hunk relocator variables
chippoint	dc.l	0
nhunks	dc.l	0
hunks	dcb.l	100*3,0

loadbase	dc.l	0
binbase	dc.l	0	* contains pointer to relocated player code
moduleptr	dc.l	0
modulesize	dc.l	0
* number of raster lines to wait for dma by default
dmawaitconstant	dc.l	10

framecount	dc.l	0
songendbit	dc.l	0

tagarray	dc.l	0
dtp_check	dc.l	0		* DTP_Check2
ep_check3	dc.l	0		* EP_Check3
ep_check5	dc.l	0		* EP_Check5
startintfunc	dc.l	0
stopintfunc	dc.l	0
intfunc	dc.l	0
initfunc	dc.l	0
initsoundfunc	dc.l	0
endfunc	dc.l	0
volumefunc	dc.l	0
cia_chip_sel	dc.l	0		* 0 = CIA A, 1 = CIA B
cia_timer_sel	dc.l	1		* 0 = Timer A, 1 = Timer B
ciabase	dc.l	0		* CIA base pointer: bfe001 or bfd000
ciatimerbase	dc.l	0		* Pointer to low byte of CIA timer
ciaadatas	dcb.l	2,0		* data pointers for CIA A timer A and B
ciaaints	dcb.l	2,0		* interrupt vectors for CIA A timers
ciabdatas	dcb.l	2,0		* data pointers for CIA B timer A and B
ciabints	dcb.l	2,0		* interrupt vectors for CIA B timers
configfunc	dc.l	0

* must be called before init player
extloadfunc	dc.l	0
* format is: dc.l index,pointer,len (last index is -1)
* maximum of 64 files can be loaded
load_file_list	dcb.l	64*3,-1
load_file_list_end
	dc.l	-1

msgptr	dc.l	$200
messagebit	dc.l	0

nextsongfunc	dc.l	0
prevsongfunc	dc.l	0
subsongfunc	dc.l	0
newsubsongarray	dc.l	0
cursubsong	dc	0
subsongrange
minsubsong	dc	0
maxsubsong	dc	0
changesubsongbit	dc.l	0

adjacentsubfunc	dc.l	0

vblanktimerstatusbit	dc.l	0
vblanktimercount	dc.l	0
vblanktimerbit	dc.l	0
vblanktimerfunc	dc.l	0
timerioptr	dc.l	0

vbi_hz	dc	50		* PAL 50Hz is default
cia_timer_base_value	dc	$376b	* PAL 50Hz

useciatimer	dc.l	0

voltestbit	dc.l	0
modulechange_disabled	dc.l	0

getmsgbit	dc.l	0
sendiomsgbit	dc.l	0

virginaudioints	dc.l	0

noteplayerptr	dc.l	0
noteplayersetupfunc	dc.l	0
notestructptr	dc.l	0
np_chanlist	dc.l	0
np_chans	dc.l	0
np_longsamples	dc.l	0
np_counter	dc.l	0
np_chanset	dcb.b	32,0

amplifier_init_func	dc.l	0

eaglesafetybase	dcb.b	$200,0	* see ENPP_SizeOf
eaglebase	dcb.b	$200,0

loadedmodname	dcb.b	256,0

playername	dcb.b	256,0
modulename	dcb.b	256,0
formatname	dcb.b	256,0

lastlock	dcb.b	256,0
curdir	dcb.b	256,0

dirarray	dcb.b	256,0
filearray	dcb.b	256,0
patharray	dcb.b	256,0

exec_dumpsignal	dcb.b	128,0

* WARNING a buggy asmone might not tolerate "\2 - debug_info" so spaces are
* removed from the macro
debuginfo	macro
	dc.b	\1, 0
	even
	dc.l	\2-debug_info
	endm

debug_info	debuginfo	'uade debug info', debug_info
	debuginfo	'config', call_config
	debuginfo	'extload', call_extload
	debuginfo	'check module', call_check_module
	debuginfo	'init player', call_init_player
	debuginfo	'check sub songs', call_check_subsongs
	debuginfo	'init volume', call_init_volume
	debuginfo	'init sound', call_init_sound
	debuginfo	'start int', call_start_int
	debuginfo	'interrupt', trapcall
	debuginfo	'amp init', call_amplifier_init
	debuginfo	'amp interrupt', amplifier_int
	debuginfo	'amp adr', amplifier_adr
	debuginfo	'amp per', amplifier_per
	debuginfo	'amp len', amplifier_len
	debuginfo	'amp vol', amplifier_vol
	debuginfo	'find author', enpp_find_author
	debuginfo	'relocator', relocator
	debuginfo	'load file', loadfile
	debuginfo	'audio int', mylevel4
	debuginfo	'ciaa int', ciaa_interrupt
	debuginfo	'ciab int', ciab_interrupt
	debuginfo	'set player interrupt', set_player_interrupt
	debuginfo	'superstate', exec_superstate
	debuginfo	'change subsong', change_subsong
	dc.l	0

* dos.library
	dcb.b	$800,0
dos_lib_base	dcb.b	$200,0

* uade.library
uadelibmsg	dcb.b	EP_OPT_SIZE,0
		dcb.b	24,0
uade_lib_base

* intuition.library
	dcb.b	$400,0
intuition_lib_base
	dcb.b	$200,0

end
