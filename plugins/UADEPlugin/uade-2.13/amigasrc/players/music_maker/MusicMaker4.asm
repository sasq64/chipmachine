;               T

* Copyright (C) 1986-2005 by Thomas Winischhofer, Vienna, Austria
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1) Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2) Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the istribution.
* 3) The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; MusicMaker 4 channel EaglePlayer header
; Version 4.0
; Written by Thomas Winischhofer
; First-pass improval by BUGGS of DEFECT
; Revised again by Thomas Winischhofer
;
; This file is a very special version of the "sysplayer"
; coming with the MusicMaker Developer Pack 2.0. It does
; in its original state not support many of the functions below.
;        ¯¯¯¯¯¯¯¯       ¯¯¯
; Further, MusicMaker's music system is not "just another Tracker"
; clone, but different to the mentioned in almost all features.
; Therefore, some of the functiones provided by EaglePlayer/DeliTracker
; such as "PrevPattern", do not match. MusicMaker does not have
; "modules" in common sense, instead, songs consist of more than
; one file. MusicMaker packs its sample-files better than any other
; program available currently, so don't worry if loading a song
; may take a bit longer than exspected.
; ************************************************************************
; ALWAYS select the ".sdata"-file to play a MM song from Deli/EaglePlayer!
; Song must have been saved in DATA/PACKED mode! (NO LIB-DISK SETS!)
; ************************************************************************
;
; INFO GIVEN HERE ALSO APPLIES TO MusicMaker8.

	incdir	include:
	include	misc/EaglePlayer.i
	include	LVO3.0/exec_lib.i

even MACRO
   cnop 0,2
   ENDM

   SECTION Player,Code

   PLAYERHEADER PlayerTagArray

   dc.b '$VER: MusicMaker 4-channel player module V4.0 (10 Nov 93)',0
   even

PlayerTagArray
   dc.l  DTP_RequestDTVersion,$1fffffff
   dc.l  DTP_PlayerVersion,4
   dc.l  DTP_PlayerName,PName
   dc.l  DTP_Creator,CName
   dc.l  EP_Check3,Chk
   dc.l  DTP_Check2,Chk
   dc.l  DTP_ExtLoad,Loadexternal
   dc.l  DTP_Config,Config
   dc.l  DTP_InitPlayer,InitPlay
   dc.l  DTP_EndPlayer,EndPlay
   dc.l  DTP_StartInt,StartSnd
   dc.l  DTP_StopInt,StopSnd
   dc.l  DTP_Volume,VolVoices
   dc.l  DTP_Balance,VolVoices
   dc.l  EP_SetSpeed,SetSpeedDirect
   dc.l  EP_StructInit,structinit
   dc.l  EP_Voices,setvoices
   dc.l  EP_Save,SaveMod
   dc.l  EP_Flags
   dc.l EPB_Songend+EPB_Packable+EPB_Balance+EPB_Volume+EPB_VolVoices+EPB_Voices+EPB_Restart+EPB_Analyzer+EPB_ModuleInfo+EPB_Save
   dc.l  TAG_DONE

*-----------------------------------------------------------------------*
;
; Player/Creatorname and local data

PName: dc.b 'MusicMaker4',0
CName: dc.b 'Thomas Winischhofer',0
uadename:	dc.b 'uade.library',0

 cnop 0,4

_DOSBase:   dc.l 0
dtbase:     dc.l 0
uadebase:	dc.l 0

MAX_NAME_LENGTH	equ	512

FileBuffer: ds.b 8

packedornot:dc.b 0
Playing:    dc.b 0
ownmem:     dc.b 0
 cnop 0,4
volvoice1:  dc.w 0
volvoice2:  dc.w 0
volvoice3:  dc.w 0
volvoice4:  dc.w 0

Modname: ds.b 20
         dc.b 0
    cnop 0,4

MM_InfoBuffer:
   dc.l  MI_Samples,0         ;4
   dc.l  MI_MaxSamples,0      ;12
   dc.l  MI_SamplesSize,0     ;20
   dc.l  MI_LoadSize,0        ;28
;  dc.l  MI_Songsize,0        ;36
   dc.l  MI_SongName,Modname
   dc.l  0
   cnop 0,4

;====================== Moduleinfo ==========================================

Getinfos:
   lea.l MM_InfoBuffer(pc),a0
   clr.l (a0)

   move.l basicinstrsdata,d0
   beq.s .rts
   move.l d0,a1
   cmp.l #'SEI1',(a1)
   bne.s .rts
   cmp.w #'XX',4(a1)
   bne.s .rts
   move.w 6(a1),12(a0)  ; maxsamples eintragen

   moveq.l #0,d1
   moveq.l #0,d2
   
   lea.l 8(a1),a1
   moveq #INSTNUM-1,d0
.chksam
     moveq.l #0,d3
     move.w (a1),d3
     beq.s .nosam
      add.l d3,d1
      addq.l #1,d2
.nosam
     addq.l #8,a1
   dbf d0,.chksam

   addi.l #8+(INSTNUM*8)+8,d1  ;8='SEI1XX'## + 8 zur Sicherheit
   move.l d1,20(A0)     ;Samplessize
   move.l d2,4(A0)      ;Sampleanzahl
   
   move.l basicmacrosdata,d0
   beq.s .rts

   move.l d0,a1

   cmp.w #'SE',(a1)
   bne.s .rts

   lea 2(a1),a2
   lea Modname(pc),a3
   moveq.l #19,d0
.copy: move.b (a2)+,(a3)+
   dbf d0,.copy

   move.l basicmacroslen,d0
   add.l basicinstrslen,d0
   move.l d0,28(a0)

   move.l #MI_Samples,(a0)
.rts
   rts

;======================== Save Funktion =====================================

SaveMod:
   move.l basicmacrosdata,d0
   beq .Nosave

;  move.l   dtg_PathArrayPtr(a5),a0
;  clr.b (a0)        ; clear Path
;  move.l   dtg_CopyDir(a5),a0   ; copy dir into patharray
;  jsr   (a0)
;  move.l   dtg_CopyFile(a5),a0  ; append filename
;  jsr   (a0)

   move.l dtg_PathArrayPtr(a5),a0
   move.l a0,a2
   move.w #300-1,d0
.loop: move.b (a0)+,(a2)+
   dbeq d0,.loop

   move.l a0,-(a7)
   bsr getfilename      ; cut .sdata/.i/.ip aso.
   move.l (a7)+,a0

.l1: tst.b (a0)+
   bne.s .l1
   move.b #'.',-1(a0)
   move.b #'s',(a0)+
   move.b #'d',(a0)+
   move.b #'a',(a0)+
   move.b #'t',(a0)+
   move.b #'a',(a0)+
   clr.b (a0)
.save:
   move.l basicmacrosdata,EPG_ARG1(a5)
   move.l basicmacroslen,EPG_ARG2(a5)
   move.l dtg_PathArrayPtr(a5),EPG_ARG3(a5)
   moveq.l #-1,d0
   move.l d0,EPG_ARG4(a5)
   clr.l EPG_ARG5(a5)
   moveq.l #5,d0
   move.l d0,EPG_ARGN(a5)

   move.l EPG_SaveMem(a5),a0
   jsr (a0)
   tst.l d0
   bne.s .error

   move.l dtg_PathArrayPtr(a5),a0
   move.w #300-1,d0
.loop2: tst.b (a0)+
   dbeq d0,.loop2

   subq.l #7,a0
   move.b #'.',(a0)+
   move.b #'i',(a0)+
   clr.b (a0)

   move.l basicinstrsdata,EPG_ARG1(a5)
   move.l basicinstrslen,EPG_ARG2(a5)
   move.l dtg_PathArrayPtr(a5),EPG_ARG3(a5)
   moveq.l #-1,d0
   move.l d0,EPG_ARG4(a5)
   clr.l EPG_ARG5(a5)
   moveq.l #5,d0
   move.l d0,EPG_ARGN(a5)

   move.l EPG_SaveMem(a5),a0
   jmp (a0)

.Nosave:
  moveq.l #EPR_NoModuleLoaded,d0
.error:
  rts


*-----------------------------------------------------------------------*
;
; Checkroutine

; Basically, it should be ID enough, if there's a ".sdata"-file that
; starts with "SE". But, due to bad ID handling of SoundTracker MOD's
; I implemented another - highly inofficial - check. Please DO NOT
; use this in own programs. Thank you.
; (What, if the MOD name is MMP0 or MMP1, like MED id-s his modules?)

Chk:
   move.l dtg_ChkData(a5),a0
   cmpi.w #'SE',(a0)
   bne ChkFail

   jsr _isstdsong
   beq ChkFail

   move.b 23(a0),d0     ; THIS IS HIGHLY !!! INOFFICIAL !!!
   cmpi.b #16,d0        ;
   bcs ChkFail          ; DO NOT USE THIS IN YOUR OWN PROGRAMS!!!
   cmpi.b #64,d0        ;
   bhi ChkFail          ; (Checks macrolength)

   move.l dtg_PathArrayPtr(a5),a0
   lea FileName,a2
   move.l #MAX_NAME_LENGTH-1,d0
.l4:  move.b (a0)+,(a2)+
   dbeq d0,.l4

   tst.w d0
   bmi ChkFail

   subq.l #7,a2
   cmpi.b #".",(a2)+
   bne ChkFail
   move.l a2,-(a7)
   lea.l .l3(PC),a0
.l1:   move.b (a2)+,d0
      beq.s .l2
      bclr #5,d0
      cmp.b (a0)+,d0
     beq.s .l1
   move.l (a7)+,a2
   bra ChkFail
.l3: dc.b 'SDATA',0
   cnop 0,4
.l2:
   move.l (a7)+,a2

ChkUnPkd:
   move.b #'i',(a2)+
   clr.b (a2)

   move.l #FileName,d1
   move.l #1005,d2
   move.l a6,-(a7)
   move.l _DOSBase(PC),a6
   jsr -30(a6)
   movem.l (a7)+,a6

   move.l d0,d4
   bne.s ChkOpen
ChkPacked:
   move.b #'p',(a2)+
   clr.b (a2)

   move.l #FileName,d1
   move.l #1005,d2
   move.l a6,-(a7)
   move.l _DOSBase(PC),a6
   jsr -30(a6)
   move.l (a7)+,a6
   move.l d0,d4
   beq.s ChkFail
ChkOpen:
   move.l d4,d1              ;
   move.l #FileBuffer,d2     ; Open/Read/Close is better than just Lock/Unlock
   moveq.l #8,d3             ;
   move.l a6,-(a7)           ; because of single-step r-flag testing!
   move.l _DOSBase(PC),a6    ;
   jsr -42(a6)               ; (If r-flag is clear, the loader won't read the file!)
   move.l (a7)+,a6           ;
   move.l   d0,d3

   move.l d4,d1
   move.l a6,-(a7)
   move.l _DOSBase(PC),a6
   jsr -36(a6)
   move.l (a7)+,a6
ChkTst:
   subq.l #8,d3
   bne.s ChkFail
   moveq.l #0,d0
   rts
ChkFail:
   moveq.l #-1,d0
   rts

;=================== Voices/Vol/Bal ========================================

VolVoices:
   lea.l myvoicedata+UPS_DMACon(PC),a0
   move.w EPG_Voices(a5),(a0)        ; copy volumes
   lea.l volvoice1(pc),a1
   move.l EPG_Voice1Vol(a5),(a1)
   move.l EPG_Voice3Vol(a5),4(a1)
   rts

;=============== externe Dateien laden & ggfalls entpacken  ================

Loadexternal:
   lea.l f.i(pc),a4     ; join '.i'
   bsr .getpath

   move.l dtg_DOSBase(a5),a6
   move.l dtg_PathArrayPtr(a5),d1
   moveq.l #-2,d2
   jsr -84(a6)
   move.l d0,d7
   move.l d0,d1
   jsr -90(a6)
   tst.l d7
   beq.s .trypacked     ;kein ".i" - File ? -> ".ip" versuchen

   clr.b packedornot    ;nicht gepacktes file

   move.l #1,EPG_ARGN(a5)
   move.l #$10003,EPG_ARG1(a5)      ; memory type: MusicMaker4: CHIP
   move.l EPG_NewLoadFile(a5),a0
   jmp (a0)

.trypacked:
   lea.l f.ip(pc),a4    ; join '.ip'
   bsr .getpath

   st packedornot       ;gepacktes file

   move.l #1,EPG_ARGN(a5)
   move.l #$10001,EPG_ARG1(a5)
   move.l EPG_NewLoadFile(a5),a0
   jmp (a0)

.getpath:
   movem.l d0-d7/a0-a5,-(a7)
   move.l dtg_PathArrayPtr(a5),a0
   clr.b (a0)                  ; clear Path
   move.l dtg_CopyDir(a5),a0   ; copy dir into patharray
   jsr (a0)
   move.l dtg_CopyFile(a5),a0  ; append filename
   jsr (a0)
   move.l dtg_CutSuffix(a5),a0 ; remove '.pp' suffix if necessary
   jsr (a0)
   move.l dtg_PathArrayPtr(A5),A0
   lea.l FileName,a2
   move #MAX_NAME_LENGTH-1,d0
.loop: move.b (a0)+,(a2)+
   dbeq d0,.loop
   subq.l #7,a0
   clr.b (a0)
   move.l a4,a0
   move.l dtg_CopyString(a5),a1
   jsr (a1)
   movem.l (a7)+,d0-d7/a0-a5
   rts
f.i:  dc.b  '.i',0
f.ip: dc.b  '.ip',0
   cnop 0,4

*-----------------------------------------------------------------------*
;
; Einmalige Initialisierung des Players

Config:
   move.l   dtg_DOSBase(a5),_DOSBase
   moveq #0,d0
   rts

*-----------------------------------------------------------------------*
;
; Init Player

InitPlay:
        move.l  4.w,a6
        lea     uadename(pc),a1
        moveq   #0,d0
        jsr     _LVOOpenLibrary(a6)
        move.l  d0,uadebase

        bsr     uade_time_critical_on

	
   move.l a5,dtbase

   moveq #0,d0
   move.l dtg_GetListData(a5),a0
   jsr (a0)
   tst.l d0
   beq EndPlay3

   move.l a0,basicmacrosdata
   move.l d0,basicmacroslen

   move.l dtg_PathArrayPtr(a5),a0
   moveq #0,d0
   jsr _loadandinit
   tst.l d0
   bne.s EndPlay3

   move.l dtg_SongEnd(a5),mm_songend   ; save songend function ptr

   move.l dtg_AudioAlloc(a5),a0
   jsr (a0)
   tst.l d0
   bne.s EndPlay2
   rts

uade_time_critical_on
	movem.l	d0-d7/a0-a6,-(a7)
        move.l  uadebase(pc),d0
        beq.b   no_uade_1
        move.l  d0,a6
        moveq   #-1,d0
        jsr     -6(a6)
no_uade_1
	movem.l	(a7)+,d0-d7/a0-a6
        rts

uade_time_critical_off
	movem.l	d0-d7/a0-a6,-(a7)
        move.l  uadebase(pc),d0
        beq.b   no_uade_2
        move.l  d0,a6
        moveq   #0,d0
        jsr     -6(a6)
no_uade_2
	movem.l	(a7)+,d0-d7/a0-a6
        rts

*-----------------------------------------------------------------------*
;
; End Player

EndPlay:
   move.l dtg_AudioFree(a5),a0    ; call Function
   jsr (a0)
EndPlay2:
   jsr _removeloaded
EndPlay3:
   moveq #-1,d0
   rts

*-----------------------------------------------------------------------*
;
; Start Sound

StartSnd
	bsr	uade_time_critical_off
   moveq.l #0,d0  ; LOOP-mode
   jsr _generalsndreset
   jsr _soundon
   st Playing
   rts

*-----------------------------------------------------------------------*
;
; Stop Sound

StopSnd
   sf Playing
   jsr _soundoff
   rts

*-----------------------------------------------------------------------*
;
; Set Volume            ... obsolete ....
;
;SetVolume:
;   tst.b Playing
;   beq.s SetVolEnd
;   moveq #0,d0
;   move.w dtg_SndVol(a5),d0
;   add.b d0,d0
;   bpl.s SetVolOk
;    moveq #127,d0
;SetVolOk
;   jsr   _setvolume
;SetVolEnd
;   rts

*-----------------------------------------------------------------------*
;
; EP_Speed handling

SetSpeedDirect:
 ext.l d0   ; is parameter .l ?
 neg.l d0
 addi.l #25,d0
 mulu #25,d0
 addi.l #300,d0
 jsr _setspeed
 rts

;*-----------------------------------------------------------------------*
;
; Increase Speed "a little" - whatsoever this should mean   [obs]
;
;SetSpeedPLUS:
;   tst.b Playing
;   beq.s .l1
;   moveq.l #-1,d0
;   jsr _setspeed     ; get speed
;   subq.w #8,d0
;   cmpi.w #300,d0    ; lower value -> higher speed, therefore this is lower limit
;   bcc.s 2$
;     move.w #300,d0
;.l2:
;   jsr _setspeed
;.l1:
;   rts
;
;*-----------------------------------------------------------------------*
;;
;; Decrease Speed "a little" - whatsoever this should mean, too    [obs]
;
;SetSpeedMINUS:
;   tst.b Playing
;   beq.s .l1
;   moveq.l #-1,d0
;   jsr _setspeed     ; get speed
;   addq.w #8,d0
;   cmpi.w #2800,d0   ; lower value -> higher speed, therefore this is higher limit
;   bls.s 2$
;    move.w #2800,d0
;.l2:
;   jsr _setspeed
;.l1:
;   rts
;

*-----------------------------------------------------------------------*
;
; EP_Voices handling

setvoices:
   not.b d0
   jsr _mutechannels
   rts


*-----------------------------------------------------------------------*
;
; Player User Prg Init Structure

structinit:
 lea.l myvoicedata(PC),a0
 move.w #$000f,UPS_DMACon(a0)
 clr.w UPS_Enabled(a0)
 move.w #UPSB_Adr!UPSB_Len!UPSB_Per!UPSB_Vol!UPSB_DMACon,UPS_Flags(a0)

 move.l a0,-(a7)
 moveq.l #(4*(2+7))-1,d0
.l1: clr.w (a0)+
 dbf d0,.l1
 move.l (a7)+,a0
 moveq.l #-1,d0       ; set repeat OFF
 move.b d0,16(a0)             ; DO NOT EXTEND TO WORD!
 move.b d0,18+16(a0)
 move.b d0,18+18+16(a0)
 move.b d0,18+18+18+16(a0)
 rts

myvoicedata:
 ds.b UPS_SizeOF

 cnop 0,4

mm_songend: dc.l 0

; ======================================================================
;
;
; REPLAYER for MusicMakerV8-created music, 4-Channel mode.
; [Linking module]
; Written by Dire Cracks Amiga® Software, Austria.
;
; SYSTEM-COOP-VERSION
;
;                === Special Version for EaglePlayer ===
;

INSTNUM equ 36 ; DON'T CHANGE !!!!!
CHANNELMAX equ 4

tablesize equ 6*4

_getsongname:    ; this is not implemented
_setupcachecontrol:     ;-
_newmaketables:         ;-
_setmixbuffers:         ;- these
_obtainmixbuflen:       ;-       are
_setalertreaction:      ;-           not required in 4-channel-player;
 moveq.l #0,d0          ;- This only for absolute identity to mplayer
 rts                    ;- in # and names of routines


_loadandinit:    ; a0=filename, d0=oneshot
 moveq.l #0,d1
 suba.l a1,a1
_newloadandinit: ; a0=filename, d0=oneshot, d1=songnumber(0-15), a1=subroutine
 movem.l d2-d7/a2-a6,-(a7)
 lea.l mystructure(PC),a5
 move.l a0,basicfilename-ms(a5)
 move.l d1,currsongnum-ms(a5)
 bsr geta4
 move.l d0,basicgivenoneshot-pf(a4)
 move.l a1,basicsubroutine-pf(a4)

 tst.l basicinstrsdata-pf(a4)
 beq.s noneinmem
    bsr freeinstruments
noneinmem:
 move.l $4,a6
 lea.l dosname(PC),a1
 jsr -408(a6)
 move.l d0,mydosbase-ms(a5)

 tst.b packedornot
 bne.s trypacked

  move.l a5,-(a7)
  move.l dtbase(PC),a5
  moveq.l #1,d0
  move.l dtg_GetListData(a5),a0
  jsr (a0)
  move.l (a7)+,a5
  move.l a0,basicinstrsdata-pf(a4)
  move.l d0,basicinstrslen-pf(a4)

  clr.b ownmem
  move.l basicinstrsdata-pf(a4),a0
  moveq.l #-1,d0
  cmp.l (a0)+,d0
  bne instrsloaded
  cmp.l (a0)+,d0
  bne instrsloaded
  bra instlibdiskerror

trypacked:
 clr.b ownmem
 move.l a5,-(a7)
 move.l dtbase(PC),a5
 moveq.l #1,d0                        ; HEY, BUGGS!  Isn't that a bit easier
 move.l dtg_GetListData(a5),a0        ;
 jsr (a0)                             ;
 move.l (a7)+,a5                      ; than you thought?
 move.l a0,samplefileadr-ms(a5)       ;
 move.l d0,samplefilesize-ms(a5)      ;
                                      ;
 bsr _getunpackedinstlen              ;
 move.l d0,basicinstrslen-pf(a4)      ;
 move.l #$10003,d1                    ;
 jsr -198(a6)                         ;
 move.l d0,basicinstrsdata-pf(a4)     ;
 beq instmemerror                     ;
 st ownmem                            ;
                                      ;
 move.l d0,a1                         ;
 move.l samplefileadr-ms(a5),a0       ;
 bsr _decrunchinstrs                  ;

instrsloaded:
 move.l basicmacrosdata-pf(a4),a0
 cmpi.w #'SE',(a0)    ; in library-loaded songs there MUST be this mark!
 bne fileformaterror
 jsr _isstdsong
 beq fileformaterror  ; if EXT-Song

 move.l basicinstrsdata-pf(a4),a1
 move.l basicmacrosdata-pf(a4),a2
 move.l basicsubroutine-pf(a4),a3
 move.l basicgivenoneshot-pf(a4),d0
 move.l currsongnum-ms(a5),d1
 bsr _newsndinit
 moveq.l #0,d0
exitloadandinit:
 movem.l (a7)+,d2-d7/a2-a6
 rts

instfileerror:
 moveq.l #101,d0
 bra.s exitloadandinit
instmemerror:
 moveq.l #102,d0
 bra.s exitloadandinit
instmemerror1:
 bsr freeinstruments
 moveq.l #103,d0
 bra.s exitloadandinit
instlibdiskerror:
 bsr freeinstruments
 moveq.l #104,d0
 bra.s exitloadandinit
melodyfileerror:
 bsr freeinstruments
 moveq.l #106,d0
 bra.s exitloadandinit
melodymemerror:
 bsr freeinstruments
 moveq.l #107,d0
 bra.s exitloadandinit
fileformaterror:
 bsr freesdata
 bsr freeinstruments
 moveq.l #111,d0
 bra.s exitloadandinit

getfilename:   ; result: d0=0=sndedfile , d1=filetype(1-7)
 movem.l a0-a6/d2-d7,-(a7)
 move.l a0,a1
findend:
  tst.b (a0)+
 bne.s findend
 subq.l #1,a0
 cmpa.l a0,a1
 beq.s notasndedfile
 move.l a0,a5
 lea.l appendixtable(PC),a2
 move.l a2,a4
searchthrutable:
 move.w (a2)+,d2
 bmi.s notasndedfile
 lea.l 0(a4,d2.w),a3
 move.l a5,a0
 moveq.l #0,d7
 move.b (a3)+,d7
appcompareloop:
   move.b -(a0),d6
   cmpi.b #'.',d6
   beq.s dontORdot
     ori.b #$20,d6
dontORdot:
   cmp.b (a3)+,d6
  dbne d7,appcompareloop
  beq.s isasndedfile
 bra searchthrutable
isasndedfile:
 subq.w #2,a2
 suba.l a4,a2
 move.l a2,d1
 lsr.l #1,d1
 addq.l #1,d1
 clr.b (a0)
 moveq.l #0,d0
 movem.l (a7)+,a0-a6/d2-d7
 rts
notasndedfile:
 moveq.l #-1,d0
 movem.l (a7)+,a0-a6/d2-d7
 rts

at:
appendixtable:
 dc.w a.i-at,a.i.n-at,a.i.lX-at,a.sdata-at,a.ip-at,a.ip.lX-at,a.ip.n-at
 dc.w -1
a.sdata: dc.b 5,'atads.',0
a.i:     dc.b 1,'i.',0
a.ip:    dc.b 2,'pi.',0
a.i.n:   dc.b 3,'n.i.',0
a.i.lX:  dc.b 3,'l.i.',0
a.ip.n:  dc.b 4,'n.pi.',0
a.ip.lX: dc.b 4,'l.pi.',0
 cnop 0,2

_removeallsongs:         ; <no parameter>
 movem.l d0-d7/a0-a6,-(a7)
 lea.l mystructure(PC),a5
 moveq.l #19,d7
.l1:  move.l d7,d0
     bsr.s _newremovesong
 dbra d7,.l1
 sf sounddatavalid-ms(a5)
 movem.l (a7)+,d0-d7/a0-a6
 rts

_removeloaded:           ; <no parameter>
 movem.l d2-d7/a2-a6,-(a7)
 bsr _soundoff
 bsr _generalsndremove
 lea.l mystructure(PC),a5
 clr.l currsongnum-ms(a5)
 bsr.s freesdata
 bsr.s freeinstruments
 movem.l (a7)+,d2-d7/a2-a6
 rts

_newremovesong:          ; d0=songnum
 movem.l d2-d7/a2-a6,-(a7)
 lea.l mystructure(PC),a5
 cmp.l lastinitedsong-ms(a5),d0
 bne.s .l1
  bsr _soundoff
.l1:
 move.l d0,currsongnum-ms(a5)
 bsr.s freesdata
 bsr.s freeinstruments
 movem.l (a7)+,d2-d7/a2-a6
 rts

freesdata:
 bsr geta4
 move.l basicmacrosdata-pf(a4),d0
 beq.s .l1
 move.l d0,a1
 move.l basicmacroslen-pf(a4),d0
 beq.s .l1
 clr.l basicmacrosdata-pf(a4)
.l1:
 clr.l basicmacroslen-pf(a4)
 rts

freeinstruments:
 tst.b ownmem
 beq.s .l1
  bsr geta4
  move.l basicinstrsdata-pf(a4),d0
  beq.s .l1
  move.l d0,a1
  move.l basicinstrslen-pf(a4),d0
  beq.s .l1
  move.l $4,a6
  jsr -210(a6)
  clr.l basicinstrsdata-pf(a4)
.l1:
  clr.l basicinstrslen-pf(a4)
 rts

geta4:
 move.l d0,-(a7)
 lea.l basicptrfield,a4
 move.l currsongnum(PC),d0
 mulu #tablesize,d0
 lea.l 0(a4,d0.l),a4
 move.l (a7)+,d0
 rts

_getunpackedinstlen:  ; a0=ip-file-start, result: d0=length
 movem.l d1-d4/a0-a1,-(a7)
 moveq.l #26,d3
 moveq.l #0,d2
 cmpi.l #'SEI1',(a0)
 bne.s .l1
   cmpi.w #'XX',4(a0)
   bne.s .l1
     move.w 6(a0),d3
     addq.l #1,d3
     moveq.l #8,d2
.l1:
 move.l a0,a1
 moveq.l #INSTNUM+1,d1  ; +1, weil *8=8=len('SEI1XX'##)
 lsl.w #3,d1            ; instinfo: INSTNUM*8
 addq.l #4,d1           ; defsnd
 lsl.w #3,d3
 move.w 0(a0,d3.w),d4
 adda.w d2,a0
 sub.w d3,d2
 neg.w d2
 lsr.w #3,d2
 move.w d2,d0
 subq.w #1,d0
calcinstlens:
   moveq.l #0,d2
   move.w (a0),d2    ; add the instrslen.
   beq.s .l3
    add.l d2,d1
    mulu d4,d2
    addq.l #8,d2
    lsr.l #3,d2
    add.l d2,d3
.l3:
  addq.l #8,a0
 dbra d0,calcinstlens
 addq.l #2,d3        ; fibbits:  WORD
 moveq.l #1,d0       ; fibtable: 1<<fibbits
 move.w (a0)+,d2
 lsl.l d2,d0
 add.l d0,d3
 lea.l 0(a1,d3.l),a0
 addi.l #15*2,d1
 moveq.l #15-1,d0
addtheLFOs:
    moveq.l #0,d2
    move.b (a0)+,d2
    lsl.w #8,d2
    move.b (a0)+,d2
    add.l d2,d1
 dbra d0,addtheLFOs
 addq.l #8,d1
 move.l d1,d0
 movem.l (a7)+,d1-d4/a0-a1
 rts

_isstdsong:  ; a0=sdatastart
 cmpi.w #'SE',(a0)
 beq.s .l1
   cmpi.b #$ff,(a0)
   bra.s .l2
.l1:
 cmpi.b #$ff,2+20(a0)
.l2:
 sne d0
 ext.w d0
 ext.l d0
 rts

_lockaudio:
 movem.l d1-d2/a0-a1/a5-a6,-(a7)
 lea.l mystructure(PC),a5
 tst.b audopen-ms(a5)
 bne.s .l1
  move.l $4,a6
  suba.l a1,a1
  jsr -294(a6)
  lea.l audreply,a1
  move.l d0,16(a1)
  lea.l audname-ms(a5),a0
  lea.l auddevio,a1
  move.l #audreply,14(a1)
  move.l #allocmap,$22(a1)
  move.l #1,$26(a1)
  move.b #127,9(a1)
  moveq.l #0,d0
  moveq.l #0,d1
  jsr -444(a6)
  tst.l d0
  seq audopen-ms(a5)
.l1:
 move.b audopen-ms(a5),d0
 ext.w d0
 ext.l d0
 movem.l (a7)+,d1-d2/a0-a1/a5-a6
 rts

_unlockaudio:
 movem.l d0-d1/a0-a1/a5-a6,-(a7)
 lea.l mystructure(PC),a5
 tst.b audopen-ms(a5)
 beq.s .l2
    sf audopen-ms(a5)
    move.l $4,a6
    lea.l auddevio,a1
    jsr -450(a6)
.l2:
 movem.l (a7)+,d0-d1/a0-a1/a5-a6
 rts

_soundon:
 movem.l a0/a5-a6/d0,-(a7)
 lea.l mystructure(PC),a5
 tst.b sounddatavalid-ms(a5)
 beq.s notvalid
  bsr hardwareinit
  moveq.l #1,d0
  move.w d0,curspeedfact1-ms(a5)
  move.w d0,curspeedfact2-ms(a5)
  move.w speed-ms(a5),d0
  mulu #23,d0
  move.w d0,playspeed-ms(a5)
  bsr settimer
  move.w #$e000,$09a(a6)
  sf soundenable-ms(a5)
  lea.l myvoicedata(PC),a5   ;
  moveq.l #4-1,d0            ;
.l1:  clr.l (a5)+             ;
     clr.l (a5)+             ; eagle
     clr.l (a5)+             ;
     clr.l (a5)+             ;
     st (a5)+                ;
     clr.b (a5)+             ;
  dbf d0,.l1                  ;
notvalid:
 movem.l (a7)+,a0/a5-a6/d0
 rts

_soundoff:
 movem.l a5-a6/d0,-(a7)
 lea.l mystructure(PC),a5
 sf sounddatavalid-ms(a5)
 st soundenable-ms(a5)
 bsr hardwareinit
 lea.l myvoicedata(PC),a5   ;
 moveq.l #4-1,d0            ;
.l1: clr.l (a5)+             ;
    clr.l (a5)+             ; eagle
    clr.l (a5)+             ;
    clr.l (a5)+             ;
    st (a5)+                ;
    clr.b (a5)+             ;
 dbf d0,.l1                  ;
 movem.l (a7)+,a5-a6/d0
 rts

_setvolume:    ; d0=state(0-127)
 movem.l d1-d2/a0-a2/a5,-(a7)
 andi.w #$007f,d0
  lea.l mystructure(PC),a5
  lea.l volumetable-ms+2(a5),a0
  lea.l strtvolus-ms+2(a5),a1
  moveq.l #(16-1)-1,d2
setvolloop:
    move.w (a1)+,d1
    mulu d0,d1
    lsr.w #7,d1
    move.w d1,(a0)+
  dbra d2,setvolloop
 moveq.l #4-1,d2
setvolloop1:
   move.w 8(a0),d1
   add.w d1,d1
   mulu d0,d1
   lsr.w #7,d1
   move.w d1,(a0)+
 dbra d2,setvolloop1
 movem.l (a7)+,d1-d2/a0-a2/a5
 rts

_mutechannels:
 andi.b #%00001111,d0
 move.b d0,_mutedchannels
 rts

_getmutedchannels:
 moveq.l #0,d0
 move.b _mutedchannels(PC),d0
 rts

_fadesnd:   ; speed = d0
 andi.b #%01111111,d0
 move.b d0,_fadeflag
 rts

_waitfade:  ; result: d0=0=fading , <>0=finished
 tst.b _fadefinished
 sne d0
 ext.w d0
 ext.l d0
 rts

_waitoneshotfin:  ; result: d0=0=playing , <>0=finished
 cmpi.b #$0f,_channelsfinished
 seq d0
 ext.w d0
 ext.l d0
 rts

_newsndreset:              ;  d0=songnum
 movem.l a4-a5,-(a7)
 lea.l mystructure(PC),a5
 move.l d0,currsongnum-ms(a5)
 bsr geta4
 move.l basicgivenoneshot-pf(a4),d0
 move.l currsongnum-ms(a5),d1
 bsr _newsndresetoneshot
 movem.l (a7)+,a4-a5
 rts

_generalsndreset:          ;  d0=oneshot
 moveq.l #0,d1
_newsndresetoneshot:       ;  d0=oneshot,d1=songnum
 movem.l d0-d7/a0-a6,-(a7)
 lea.l mystructure(PC),a5
 sf sounddatavalid-ms(a5)
 move.l d1,currsongnum-ms(a5)
 move.l d1,lastinitedsong-ms(a5)
 move.b d0,oneshot-ms(a5)
 bsr _soundoff
 bsr geta4
 move.l basicinstrsdata-pf(a4),d0
 beq noresettobedone
 move.l d0,a1
 move.l basicmacrosdata-pf(a4),a2
 move.l basicsubroutine-pf(a4),subroutine-ms(a5)
 sf internfinished-ms(a5)
 sf _channelsfinished-ms(a5)
 sf _dtchannelsfinished-ms(a5)
 sf _fadeflag-ms(a5)
 sf _fadefinished-ms(a5)
 sf channelshandle-ms(a5)
 sf doloudness-ms(a5)
 sf calltheroutine-ms(a5)
 sf dofadein-ms(a5)
 sf dofadeout-ms(a5)
 sf levelset-ms(a5)
 sf _mutedchannels-ms(a5)
 bsr.s setbacklaststuff
 clr.w called-ms(a5)
 clr.w whattodo-ms(a5)
 move.w #$8000,dmaon-ms(a5)
 move.w #$0000,dmaoff-ms(a5)
 clr.w pattlen-ms(a5)
 move.w (a2)+,d0
 cmpi.w #'SE',d0
 bne.s nosongname
   adda.w #20,a2
   move.w (a2)+,d0
nosongname:
 move.b d0,pattlen+1-ms(a5)
 lsr.w #8,d0
 move.b d0,newformat-ms(a5)
 clr.l extdatasize-ms(a5)
 cmpi.b #$fe,d0
 bne.s .l1
   move.w 6(a2),extdatasize+2-ms(a5)
.l1:
 move.w (a2)+,d0
 move.w d0,speed-ms(a5)
 move.w d0,calcspeed-ms(a5)
 mulu #23,d0
 move.w d0,playspeed-ms(a5)
 bsr ptrsinit
 bsr hardwareinit
 st sounddatavalid-ms(a5)
noresettobedone:
 movem.l (a7)+,d0-d7/a0-a6
 rts

setbacklaststuff:
 movem.l d0/a0,-(a7)
 moveq.l #4-1,d0
 lea.l lastperiods-ms(a5),a0
.l1: clr.l (a0)+
    move.w #$0100,(a0)+
    clr.w (a0)+
    clr.l (a0)+
    clr.l (a0)+
    clr.l (a0)+
    clr.l (a0)+
 dbra d0,.l1
 moveq.l #4-1,d0
 lea.l lastvolumes-ms(a5),a0
.l2: clr.l (a0)+
 dbra d0,.l2
 clr.l VIBRATO-ms(a5)
 clr.l VIBRATO+4-ms(a5)
 movem.l (a7)+,d0/a0
 rts

_generalsndremove:          ; DISPOSES THE PLAYER INTERRUPT ONLY !!!
 movem.l d0-d7/a0-a6,-(a7)  ;                               ----
 lea.l mystructure(PC),a5
 moveq.l #-1,d0
 move.l d0,lastinitedsong-ms(a5)
 tst.b intremoved-ms(a5)
 bne.s .l1
   sf sounddatavalid-ms(a5)
   move.l ciabase(PC),a6
   lea.l mysysint6(PC),a1
   moveq.l #1,d0
   jsr -12(a6)
   move.l oldinterrupt(PC),d0
   beq.s .l1
   move.l d0,a1
   moveq.l #1,d0
   jsr -6(a6)
   clr.l oldinterrupt-ms(a5)
.l1:
 st intremoved-ms(a5)
wasremsofar:
 movem.l (a7)+,d0-d7/a0-a6
 rts

_generalsndinit:            ; d0=oneshot,a1=instrs,a2=sdata,a3=subroutine
 moveq.l #0,d1
_newsndinit:                ; d0=oneshot,d1=song#,a1=instrs,a2=sdata,a3=subroutine
 movem.l d0-d7/a0-a6,-(a7)
 lea.l mystructure(PC),a5
 move.l d1,currsongnum-ms(a5)
 bsr geta4
 move.l a1,basicinstrsdata-pf(a4)
 move.l a2,basicmacrosdata-pf(a4)
 move.l a3,basicsubroutine-pf(a4)
 move.l d0,basicgivenoneshot-pf(a4)
 move.l currsongnum-ms(a5),d1
 bsr _newsndresetoneshot
 move.l $4,a6
 lea.l cianame-ms(a5),a1
 moveq.l #0,d0
 jsr -498(a6)
 move.l d0,ciabase-ms(a5)
allocint:
 move.l ciabase-ms(a5),a6
 lea.l mysysint6(PC),a1
 moveq.l #1,d0
 jsr -6(a6)
 tst.l d0
 beq.s intOK1
 cmpi.l #mysysint6,d0
 beq.s intOK
  move.l d0,oldinterrupt-ms(a5)
  move.l d0,a1
  moveq.l #1,d0
  jsr -12(a6)
  bra.s allocint
intOK1:
 clr.l oldinterrupt-ms(a5)
intOK:
 moveq.l #0,d0
 move.b #$82,d0
 jsr -18(a6)
 move.w #$a000,$dff09a
 sf intremoved-ms(a5)
 movem.l (a7)+,d0-d7/a0-a6
 rts

ptrsinit:
 lea.l myinstrvecs-ms(a5),a0
 lea.l myinstrlens-ms(a5),a4
 move.l a5,-(a7)
 moveq.l #26,d3
 cmpi.l #'SEI1',(a1)
 bne.s .l1
  cmpi.w #'XX',4(a1)
  bne.s .l1
    move.w 6(a1),d3
    addq.l #8,a1
.l1:
 move.l d3,d4
 lsl.l #3,d4
 lea.l 0(a1,d4.w),a5
 move.l a5,d4
 addq.l #4,a5
 move.l d3,d0
 subq.w #1,d0
 moveq.l #0,d3
setinstlens:
 move.l a5,(a0)+
  move.w (a1)+,d3
  move.w d3,(a4)+
  adda.l d3,a5
   move.w (a1)+,(a4)+
   move.w (a1)+,(a4)+
   move.w (a1)+,(a4)+
 dbra d0,setinstlens
 move.l a5,d0
 btst #0,d0
 beq.s iseven1
  addq.l #1,d0
iseven1:
 move.l d0,a1
 move.l (a7)+,a5
 lea.l lfoinstptrs+4-ms(a5),a0
 move.l a5,-(a7)
 lea.l (15*2)(a1),a5
 moveq.l #15-1,d0
setuplfodata:
  move.l a5,(a0)+
  move.w (a1)+,d3
  adda.l d3,a5
 dbra d0,setuplfodata
 move.l (a7)+,a5
 lea.l currlfotable-ms(a5),a0
 moveq.l #(INSTNUM+1)-1,d0
clrcurrlfo:
   clr.l (a0)+
 dbra d0,clrcurrlfo
 move.l d4,defsndptr-ms(a5)
 move.l a2,-(a7)
 lea.l 4(a2),a0
 adda.l extdatasize-ms(a5),a0
 bsr searchmelodystrt
 bsr searchmelodystrt
 bsr searchmelodystrt
 bsr searchmelodystrt
 move.l a0,a1
 move.l a0,a2
 lea.l mymacrovecs,a0   ; OPTIONAL !
 move.w #997+1,d0
lookontostart:
 cmpi.b #$ff,(a1)
 beq.s setpausemacro
 cmpi.b #$fe,(a1)
 beq.s endofmacros
 move.l a1,(a0)+
 subq.w #1,d0
searchnext:
 addq.l #3,a1
 cmpi.b #$ff,(a1)
 bne.s searchnext
 addq.l #1,a1
 bra.s lookontostart
setpausemacro:
 move.l a2,(a0)+
 subq.w #1,d0
 addq.l #1,a1
 bra.s lookontostart
endofmacros:
 tst.w d0
 beq.s realendofmacros
   move.l a2,(a0)+
   subq.w #1,d0
   bra.s endofmacros
realendofmacros:
 move.l (a7)+,a2
melodystrtup:
 moveq.l #0,d1
 move.w (a2)+,d1
 moveq.l #0,d2
 move.w (a2)+,d2
 move.l a2,a0
 adda.l extdatasize-ms(a5),a0
 lea.l COUNTERS-ms(a5),a2
 lea.l mymacrovecs,a3
 lea.l MODUL_0-ms(a5),a1
 bsr.s writeandsearchmelodystrt
 lea.l MODUL_1-ms(a5),a1
 bsr.s writeandsearchmelodystrt
 lea.l MODUL_2-ms(a5),a1
 bsr.s writeandsearchmelodystrt
 lea.l MODUL_3-ms(a5),a1
 bsr.s writeonly
 lea.l strtvolus-ms(a5),a0
 lea.l volumetable-ms(a5),a1
 moveq.l #15,d0
strtupvolume:
   move.w (a0)+,(a1)+
  dbra d0,strtupvolume
 clr.w (a1)+
 clr.w (a1)+
 clr.w (a1)+
 clr.w (a1)+
 rts
writeandsearchmelodystrt:
 bsr.s writeonly
searchmelodystrt:
 cmpi.w #999,(a0)+
 bne.s searchmelodystrt
 rts
writeonly:
 move.l a2,16(a1)
 move.l a2,8(a1)
 addi.l #32,8(a1)
 move.l a0,12(a1)
 add.l d2,12(a1)
 add.l d2,12(a1)
 move.l a0,(a2)
 add.l d1,(a2)
 add.l d1,(a2)
 move.l (a2),a4
 move.w (a4),d0
 add.w d0,d0
 add.w d0,d0
 move.l 0(a3,d0.w),16(a2)
 move.w #1,32(a2)
 clr.l 32+16(a2)
 addq.l #4,a2
 rts

hardwareinit:
 lea.l $dff000,a6
 move.w #$000f,$96(a6)
 moveq.l #0,d0
 move.w d0,$0a8(a6)
 move.w d0,$0b8(a6)
 move.w d0,$0c8(a6)
 move.w d0,$0d8(a6)
 rts

_setspeed:
 movem.l d1/a5,-(a7)
 lea.l mystructure(PC),a5
 move.w speed-ms(a5),d1
 tst.w d0
 bmi.s .l1
  move.w d0,speed-ms(a5)
  bsr.s getfacttruncedspeed
  move.w d0,calcspeed-ms(a5)
  mulu #23,d0
  move.w d0,playspeed-ms(a5)
.l1:
 move.w d1,d0
 movem.l (a7)+,d1/a5
 rts

getfacttruncedspeed:
 mulu curspeedfact1-ms(a5),d0
 divu curspeedfact2-ms(a5),d0
 cmpi.w #300,d0
 bge.s speedover300
  move.w #300,d0
speedover300:
 cmpi.w #2800,d0
 ble.s speedbelow2800
  move.w #2800,d0
speedbelow2800:
 rts

_decrunchinstrs:           ; a0=source , a1=dest
 movem.l d0-d7/a2-a6,-(a7)
 lea.l mystructure(PC),a5
 move.l a0,a3
 moveq.l #26,d7
 cmpi.l #'SEI1',(a0)
 bne.s .l1
  cmpi.w #'XX',4(a0)
  bne.s .l1
   move.w 6(a0),d7
   move.l (a0)+,(a1)+
   move.l (a0)+,(a1)+
   addq.l #8,a3
.l1:
 move.l d7,d0
 subq.w #1,d0
copyinstlens:
  move.l (a0)+,(a1)+
  move.l (a0)+,(a1)+
 dbra d0,copyinstlens
 clr.l (a1)+
 move.w (a0)+,fibbits
 move.l a0,fibtable
 move.w fibbits(PC),d0
 moveq.l #1,d1
 lsl.w d0,d1
 lea.l 0(a0,d1.w),a0
 subq.w #1,d7
packinstrsloop:
   moveq.l #0,d0
   move.w (a3),d0
    beq.s leniszero1
   moveq.l #0,d2
   move.w fibbits(PC),d2
   jsr decompress(PC)
leniszero1:
 addq.l #8,a3
 dbra d7,packinstrsloop
 move.l a1,d0
 btst #0,d0
 beq.s iseven
  addq.l #1,d0
iseven:
 move.l d0,a1
 moveq.l #(15*2)-1,d0
 move.l a1,a2
copylfodata:
  move.b (a0)+,(a1)+
 dbra d0,copylfodata
 moveq.l #15-1,d0
copylfodatalong:
  move.w (a2)+,d1
  beq.s lfoisunset
   subq.w #1,d1
copylfodatainner:
     move.b (a0)+,(a1)+
    dbra d1,copylfodatainner
lfoisunset:
 dbra d0,copylfodatalong
 movem.l (a7)+,d0-d7/a2-a6
 rts

decompress:
  movem.l d0-d7/a2-a3,-(a7)
  move.l a0,a3
  move.l fibtable-ms(a5),a0
  lea.l 0(a1,d0.l),a2
  move.l d2,d0
  moveq.l #0,d6
  moveq.l #0,d7
  moveq.l #0,d2
.l1:
   moveq.l #0,d1
   move.w d0,d3
.l2:
    subq.w #1,d7
    bpl.s .l3
     move.b (a3)+,d6
     moveq.l #7,d7
.l3:
    lsl.b #1,d6
    addx.b d1,d1
    subq.w #1,d3
   bne.s .l2
   add.b 0(a0,d1.w),d2
   move.b d2,(a1)+
 cmpa.l a1,a2
 bne.s .l1
 subq.w #1,d7
 bpl.s .l4
  addq.l #1,a3
.l4:
 move.l a3,a0
 movem.l (a7)+,d0-d7/a2-a3
 rts

systemint6:
 movem.l d0-d7/a0-a6,-(a7)
  lea.l $dff000,a6
  lea.l mystructure(PC),a5
  tst.b soundenable-ms(a5)
  bne.s nosndplay
  cmpi.b #$0f,_dtchannelsfinished-ms(a5)
  bne.s .l1
     move.l mm_songend,a0
     jsr (a0)
.l1:
  cmpi.b #$0f,_channelsfinished-ms(a5)
  beq.s nosndplay
    bsr.s playsnd
exitsysint6:
 movem.l (a7)+,d0-d7/a0-a6
 rts
nosndplay:
 move.w playspeed-ms(a5),d0
 bsr settimer
 bra.s exitsysint6

playsnd:          ; in a6=dff000, a5=mystructure
 move.w playspeed-ms(a5),d0
 tst.w whattodo-ms(a5)
 beq.s contsound
  clr.w whattodo-ms(a5)
  sub.w resttime-ms(a5),d0
  subi.w #$170,d0
  bsr settimer
  bra donthavetowait
contsound:
 bsr settimer
 clr.b ((dmaoff-ms)+1)(a5)
 clr.b ((dmaon-ms)+1)(a5)
 moveq.l #0,d2
 move.b _fadeflag-ms(a5),d2
 beq.s nooutfade
  not.b fadehalf-ms(a5)
  beq.s nooutfade
   lea.l volumetable-ms(a5),a0
   cmpi.w #3,2*15(a0)
   bgt.s fadenotfinished
    st _fadefinished-ms(a5)
    bra.s nooutfade
fadenotfinished:
    addq.l #2,a0
     moveq.l #(16-1)-1,d0
fadeloop:
      move.w (a0),d1
      mulu d2,d1
      lsr.w #7,d1
      move.w d1,(a0)+
     dbra d0,fadeloop
    moveq.l #4-1,d0
fadeloop1:
      move.w (a0),d1
      asr.w #1,d1
      mulu d2,d1
      lsr.w #6,d1
      move.w d1,8(a0)
      move.w d1,(a0)+
     dbra d0,fadeloop1
nooutfade:
 lea.l $d0(a6),a0
 lea.l ((lastvolumes-ms)+6)(a5),a1
 move.w volumetable+(15*2)-ms(a5),d7
 lea.l ((lastperiods-ms)+(24*3))(a5),a4
 lea.l modultable-ms(a5),a2
 lea.l SOFTMOD+(3*4)-ms(a5),a3
 sf channelshandle-ms(a5)
 moveq.l #3,d0
setallchanneldata:
 move.w (a2)+,d1
 movem.l a0-a4/d0/d7,-(a7)
 movem.l 0(a5,d1.w),d3/d7/a2-a4
 bsr handle_channel
 movem.l (a7)+,a0-a4/d0/d7
 movem.l a2,-(a7)
  movem.l d0/a0,-(a7)   ;
  lea.l myvoicedata,a0  ;
  mulu #(7+2)*2,d0      ; eagle
  clr.w 10(a0,d0.w)     ;
  movem.l (a7)+,d0/a0   ;
  tst.l d4
  bmi.s absnothing
  tst.l d5
  bmi.s pervolonly
   bset.b d0,channelshandle-ms(a5)
   move.w d4,4(a0)
   move.l d5,(a0)
    movem.l d0/a0,-(a7)   ;
    lea.l myvoicedata,a0  ;
    mulu #(7+2)*2,d0      ; eagle
    move.w d4,4(a0,d0.w)  ;
    move.l d5,(a0,d0.w)   ;
    st 10(a0,d0.w)        ;
    st 16(a0,d0.w)        ;
    movem.l (a7)+,d0/a0   ;
    swap d4
    move.w d4,d3
    swap d4
    lsr.w #8,d3
    lsl.w #2,d3
    clr.w 18(a4)
    lea.l currlfotable-ms(a5),a2
    move.l 0(a2,d3.w),8(a4)
     beq.s skiphull
    st 18(a4)
    clr.w 14(a4)
    move.w d4,16(a4)
    lsl.w 16(a4)
skiphull:
pervolonly:
  rol.w #8,d6
  move.b d6,9(a1)
  lsr.w #8,d6
  add.w d6,d6
  move.w d6,(a1)
  swap d6
  move.w d6,(a4)
  swap d4
  andi.w #$00ff,d4
  move.w d4,2(a4)
  moveq.l #-1,d4
absnothing:
  tst.w d4
  bmi.s nosetfade2
   move.w d4,4(a4)
nosetfade2:
  btst d0,dofadein-ms(a5)
  beq.s testiffadeout
    cmpi.w #4096,4(a4)
    bls.s donotsetbackto4096
      move.w #4096,4(a4)
donotsetbackto4096:
    cmpi.w #256,4(a4)
    beq.s nomorefade
    blt.s setbackto256
     move.w fadespeedin-ms(a5),d1
     sub.w d1,4(a4)
     bra.s nomorefade
setbackto256:
    move.w #256,4(a4)
    bra.s nomorefade
testiffadeout:
  btst d0,dofadeout-ms(a5)
  beq.s nomorefade
    cmpi.w #8192,4(a4)
    beq.s nomorefade
    bgt.s setbackto8192
     move.w fadespeedout-ms(a5),d1
     add.w d1,4(a4)
     bra.s nomorefade
setbackto8192:
    move.w #8192,4(a4)
nomorefade:
  move.w 8(a1),d6
  tst.b VIBRATO-SOFTMOD(a3)
  ble.s .l1
    not.b (VIBRATO+1)-SOFTMOD(a3)   ; tremolo
    bne.s .l1
      neg.w 2(a3)
.l1:
  move.w 2(a3),d1
  asr.w #1,d1
  add.w d1,d6
  moveq.l #64,d1
  exg d1,d7
  bsr getalegalvolume
  exg d1,d7
  move.w d6,8(a1)
  move.w (a1),d6
  add.w 2(a3),d6
  add.w d7,d7
  bsr getalegalvolume
  lsr.w #1,d7
  move.w d6,(a1)
  lsr.w #1,d6
  tst.b 18(a4)
  beq.s nuHULL
   moveq.l #0,d1
   move.w 14(a4),d1
   divu #40,d1
   swap d1
   clr.w d1
   swap d1
   add.w d1,d1
   add.l 8(a4),d1
   move.l d1,a2
   move.b 1(a2),d3
   move.b (a2)+,d1
   beq.s nuHULL
   ext.w d1
   muls d6,d1
   asr.w #5,d1
  add.w d1,d6
  bsr getalegalvolume
nuHULL:
  btst d0,doloudness-ms(a5)
  beq.s nodoloudness
     move.w 2(a4),d4
     moveq.l #0,d5
     lsl.w #8,d6
     move.w d6,d5
     lea.l loudnesstable-ms(a5),a2
     divu.w 0(a2,d4.w),d5
      cmp.w d6,d5
      bne.s writeloudnessvolume
        moveq.l #0,d5
writeloudnessvolume:
    move.w d5,d6
nodoloudness:
  cmpi.w #$0100,4(a4)
  beq.s nofadeatall
  lsl.w #8,d6
  moveq.l #0,d5
  move.w d6,d5
  divu 4(a4),d5
   cmp.w d6,d5
   bne.s checkvol
    moveq.l #0,d5
checkvol:
  move.w d5,d6
nofadeatall:
  bsr getalegalvolume
  btst d0,_mutedchannels-ms(a5)
  beq.s .l1
      clr.w d6
.l1:
    movem.l a0/d0,-(a7)        ;
    lea.l volvoice1(PC),a0     ;
    add.w d0,d0                ; eagleplayer
    mulu 0(a0,d0.w),d6         ;
    lsr.w #6,d6                ;
    movem.l (a7)+,a0/d0        ;
  move.w d6,8(a0)       ; - - - VOLUME READY !
   movem.l d0/a0,-(a7)      ;
   lea.l myvoicedata(PC),a0 ;
   mulu #(7+2)*2,d0         ; eagleplayer
   move.w d6,8(a0,d0.w)     ;
   movem.l (a7)+,d0/a0      ;
  move.w (a4),d6
  tst.b VIBRATO-SOFTMOD(a3)
  bge.s .l2
    not.b (VIBRATO+1)-SOFTMOD(a3)
    bne.s .l2                   ; vibrato
      neg.w (a3)
.l2:
  add.w (a3),d6
  bsr getalegalperiod
  move.w d6,(a4)
  tst.b 18(a4)
  beq.s nuHULLEND
   tst.b d3
   beq.s noLFOper
   ext.w d3
   move.w d6,d2
   muls.w d3,d2
   asr.l #8,d2
   add.w d2,d6
   bsr getalegalperiod  ; - - - PERIOD READY !
noLFOper:
   move.w calcspeed-ms(a5),d1
   mulu #115,d1
   divu d6,d1
   add.w d1,14(a4)
   bcs.s .l1
   move.w 14(a4),d1
   cmp.w 16(a4),d1
   bls.s nuHULLEND
.l1:
    tst.b 19(a4)
     beq.s switchoffHULL
      move.w 20(a4),d1
      sub.w d1,14(a4)
      bra.s nuHULLEND
switchoffHULL:
       sf 18(a4)
nuHULLEND:
  move.w d6,6(a0)
   movem.l d0/a0,-(a7)  ;
   lea.l myvoicedata,a0 ;
   mulu #(7+2)*2,d0     ; eagle
   clr.w 6(a0,d0.w)     ;
   tst.w 10(a0,d0.w)    ;
   beq.s .l1             ;
    move.w d6,6(a0,d0.w);
.l1:                     ;
   movem.l (a7)+,d0/a0  ;
 subq.l #2,a1
 subq.l #4,a3
 lea.l -24(a4),a4
 movem.l (a7)+,a2
 lea.l -$10(a0),a0
 dbra d0,setallchanneldata

 tst.b channelshandle-ms(a5)
 beq.s donthavetowait
 move.w dmaon-ms(a5),d0
 and.w dmaoff-ms(a5),d0
 beq.s donthavetowait
    st whattodo-ms(a5)
    lea.l $bfd000,a0
    andi.b #%10011110,$f00(a0)
     move.b $700(a0),d0
      lsl.w #8,d0
       move.b $600(a0),d0
        move.w playspeed-ms(a5),d1
        sub.w d0,d1
        move.w d1,resttime-ms(a5)
    move.w #$0170,d0
    bra settimer
donthavetowait:
 move.w dmaon-ms(a5),$096(a6)
 move.b _channelsfinished-ms(a5),d3
 bne.s waitatimee
  tst.b channelshandle-ms(a5)
  beq exitplay
waitatimee:
   move.l d0,-(a7)
   move.b $006(a6),d0
.l1: cmp.b $006(a6),d0
   beq.s .l1
.l2: btst #4,$007(a6)
   beq.s .l2
   move.l (a7)+,d0
     lea.l $d0(a6),a1
     lea.l nextdata3-ms(a5),a2
     lea.l lastperiods+(24*3)-ms(a5),a3
     moveq.l #3,d2
testifcontinsts:
      btst d2,d3
      bne.s pause_0
       btst d2,channelshandle-ms(a5)
       beq.s nopause_0
         move.l (a2),d0
         move.l d0,(a1)
         move.w 4(a2),4(a1)
         movem.l a0/d2,-(a7)                      ;
         lea.l myvoicedata(PC),a0                 ; eagleplayer
         mulu #(7+2)*2,d2                         ;
;        move.l (a2),0(a0,d2.w)                   ; MIT DEM FUNKTIONIERTS
;        move.w 4(a2),4(a0,d2.w)                  ; ÜBERHAUPT NICHT.
         cmp.l defsndptr-ms(a5),d0
         seq 16(a0,d2.w)                          ;
         movem.l (a7)+,a0/d2                      ;
         tst.b 18(a3)
         beq nopause_0
         cmp.l defsndptr-ms(a5),d0
         beq nopause_0
          st 19(a3)
          move.w 4(a2),20(a3)
          lsl.w 20(a3)
         bra.s nopause_0
pause_0:
         move.l defsndptr-ms(a5),(a1)
         move.w #1,4(a1)
         movem.l a0/d2,-(a7)                      ;
         lea.l myvoicedata(PC),a0                 ; eagleplayer
         mulu #(7+2)*2,d2                         ;
;        clr.l 0(a0,d2.w)                         ;
;        clr.w 4(a0,d2.w)                         ;
         st 16(a0,d2.w)                           ;
         movem.l (a7)+,a0/d2                      ;
nopause_0:
       lea.l -$10(a1),a1
       subq.l #8,a2
       lea.l -24(a3),a3
     dbra d2,testifcontinsts
exitplay:
 tst.b calltheroutine-ms(a5)
 beq.s donotcall
  movem.l d0-d7/a0-a6,-(a7)
  sf calltheroutine-ms(a5)
  move.l subroutine-ms(a5),d0
  beq.s endofcall
    move.l d0,a0
    addq.w #1,called-ms(a5)
    move.w called-ms(a5),d0
     jsr (a0)
endofcall:
    movem.l (a7)+,d0-d7/a0-a6
donotcall:
 rts

getalegalvolume:
  tst.w d6
  bpl.s noprbwithVOL
     clr.w d6
noprbwithVOL:
  cmp.w d7,d6
  ble.s noprbwithVOL1
    move.w d7,d6
noprbwithVOL1:
  rts

getalegalperiod:
  cmpi.w #113,d6
  bge.s noprbwithPER
   move.w #113,d6
noprbwithPER:
  cmpi.w #856,d6
  ble.s noprbwithPER1
   move.w #856,d6
noprbwithPER1:
  rts

settimer:
  lea.l $bfd000,a0
  move.b #%00000010,$d00(a0)
  andi.b #%10011110,$f00(a0)
   move.b d0,$600(a0)
    lsr.w #8,d0
      move.b d0,$700(a0)
  ori.b #%00011001,$f00(a0)
  move.b #%10000010,$d00(a0)
  rts

handle_channel:
 subq.w #1,(a2)
 beq.s countatzero
donothingwithDMA:
   moveq.l #-1,d4
   rts
countatzero:
 move.b internfinished-ms(a5),d0
 and.b d7,d0
 beq.s normalplay
  not.w d7
  and.b d7,dofadein-ms(a5)
  and.b d7,dofadeout-ms(a5)
  not.w d7
  move.w #0,8(a0)
  clr.w (a1)
  clr.w 8(a1)
  move.w d7,$096(a6)
  move.l 28(a7),a1   ; !!!
  clr.l (a1)+
  move.w #$0100,(a1)+
  clr.w (a1)+
  clr.l (a1)+
  clr.l (a1)+
  clr.l (a1)+
  clr.l (a1)+
  or.b d7,_dtchannelsfinished-ms(a5)  ; special
  tst.b oneshot-ms(a5)
  beq.s normalplay1
   or.b d7,_channelsfinished-ms(a5)
   bra.s donothingwithDMA
normalplay1:
 not.w d7
 and.b d7,internfinished-ms(a5)
 not.w d7
normalplay:
 move.l 16(a4),a0

 move.b 2(a0),d0
 cmpi.b #$f2,3(a0)
 beq.s takenextascount
 cmpi.b #$fb,3(a0)
 beq.s takenextascount
 cmpi.b #$f8,(a0)
 bne.s isbelow15
takenextascount:
   move.b 2+3(a0),d0
isbelow15:
 andi.w #$003f,d0
 addq.w #1,d0
 add.w d0,d0
 move.w d0,(a2)

 moveq.l #0,d5
 move.b (a0),d0
 move.b 1(a0),d1
 cmpi.b #$f2-1,d0
 bls nocontrolbyte
 cmpi.b #$f4,d0
 bne.s noloudness
  not.w d7
  and.b d7,doloudness-ms(a5)
  not.w d7
   tst.b d1
   beq.s OFFloudness
    or.b d7,doloudness-ms(a5)
OFFloudness:
jmptoaddcnt:
      moveq.l #-1,d4
      bra addcounthandle
noloudness:
 cmpi.b #$fd,d0
 beq.s jmptoaddcnt
 cmpi.b #$f5,d0
 bne.s nocustomprg
   st calltheroutine-ms(a5)
   bra.s jmptoaddcnt
nocustomprg:
 cmpi.b #$f6,d0
  bne.s nofade
   not.w d7
   and.b d7,dofadein-ms(a5)
   and.b d7,dofadeout-ms(a5)
   not.w d7
   tst.b d1
   beq.s jmptoaddcnt
   bmi.s switch2in
switch2out:
   move.b d1,fadespeedout+1-ms(a5)
   or.b d7,dofadeout-ms(a5)
   bra.s jmptoaddcnt
switch2in:
   neg.b d1
   move.b d1,fadespeedin+1-ms(a5)
   or.b d7,dofadein-ms(a5)
   bra.s jmptoaddcnt
nofade:
 cmpi.b #$f7,d0
 bne.s nosetfadestate
   moveq.l #-1,d4
   tst.b d1
   beq.s settoLO
settoHI:
   move.w #256,d4
    bra addcounthandle
settoLO:
   move.w #8192,d4
    bra addcounthandle
nosetfadestate:
 cmpi.b #$f8,d0
 bne.s isbelow151
  move.b d1,d5
  addq.l #3,a0
  move.b (a0),d0
  move.b 1(a0),d1
isbelow151:
 cmpi.b #$f9,d0
 bne.s noWARP2play
   ext.w d1
   btst #7,2(a0)
   beq.s aPERWARP
    btst #6,2(a0)
    bne.s aVIBORTRE
     move.w d1,((CHANNELMAX*8)+(CHANNELMAX*4))+2(a4)  ; vol slide
     tst.b VIBRATO-COUNTERS(a4)
     ble jmptoaddcnt
     clr.b VIBRATO-COUNTERS(a4)
     bra jmptoaddcnt
aVIBORTRE:
    tst.w d1
    bpl.s arealvib
     tst.b VIBRATO-COUNTERS(a4)
     bge.s .l1
      clr.w ((CHANNELMAX*8)+(CHANNELMAX*4))(a4)
.l1:
     neg.w d1
     move.w d1,((CHANNELMAX*8)+(CHANNELMAX*4))+2(a4)  ; tremolo
     move.w #$0100,VIBRATO-COUNTERS(A4)
     bra jmptoaddcnt
arealvib:
     tst.b VIBRATO-COUNTERS(a4)
     ble.s .l1
      clr.w ((CHANNELMAX*8)+(CHANNELMAX*4))+2(a4)
.l1:
     move.w d1,((CHANNELMAX*8)+(CHANNELMAX*4))(a4)
     move.w #$ff00,VIBRATO-COUNTERS(A4)               ; vibrato
     bra jmptoaddcnt
aPERWARP:
     move.w d1,((CHANNELMAX*8)+(CHANNELMAX*4))(a4)
     tst.b VIBRATO-COUNTERS(a4)                       ; per slide (warp)
     bge jmptoaddcnt
     clr.b VIBRATO-COUNTERS(a4)
     bra jmptoaddcnt
noWARP2play:
 cmpi.b #$fa,d0
 bne.s nosetuphull
   move.b d1,d2
   andi.w #$00f0,d1
   lsr.b #4,d1
   add.b d5,d1
   andi.w #$000f,d2
   lsl.b #2,d2
   lea.l lfoinstptrs-ms(a5),a1
   move.l 0(a1,d2.w),d2
    beq.s clrlfoptr
   move.l d2,a1
   addq.l #2,d2
   cmp.w (a1),d1
   beq.s proper
clrlfoptr:
     moveq.l #0,d2
proper:
   lsl.w #2,d1
   lea.l currlfotable-ms(a5),a1
   move.l d2,0(a1,d1.w)
   bra jmptoaddcnt
nosetuphull:
 cmpi.b #$fc,d0
 bne.s nospeedset
   moveq.l #0,d0
   move.b d1,d0
   lsr.w #4,d0
   andi.w #$000f,d1
   move.w d0,curspeedfact1-ms(a5)
   move.w d1,curspeedfact2-ms(a5)
   mulu.w speed-ms(a5),d0
   divu d1,d0
    cmpi.w #300,d0
    bge.s speedover300A
      move.w #300,d0
speedover300A:
    cmpi.w #2800,d0
    ble.s speedbelow2800A
      move.w #2800,d0
speedbelow2800A:
   move.w d0,calcspeed-ms(a5)
   mulu #23,d0
   move.w d0,playspeed-ms(a5)
   move.l a0,-(a7)
   bsr settimer
   move.l (a7)+,a0
   bra jmptoaddcnt
nospeedset:
nocontrolbyte:
 clr.l 32+16(a4)
 clr.w VIBRATO-COUNTERS(a4)
 btst #6,2(a0)
 beq.s nochangelpfplay
   move.b d1,d2
   andi.b #$40,d2
   lsr.b #5,d2
   andi.b #$fd,$bfe001
   or.b d2,$bfe001
nochangelpfplay:
 moveq.l #0,d4
 andi.w #$003f,d1
 add.w d1,d1
 move.w d1,d4
 swap d4
 move.w notetable-ms(a5,d1.w),d6
 swap d6

 cmpi.b #$f1,d0
 bne.s nonovolume
novolume:
      clr.w d6
      move.w d7,$096(a6)
      or.w d7,dmaoff-ms(a5)
      moveq.l #INSTNUM,d4
      ror.l #8,d4
      move.w #1,d4
      move.l defsndptr-ms(a5),d5
      move.l d5,0(a5,d3.w)
      move.w d4,4(a5,d3.w)
      bra addcounthandle
modulate:
      moveq.l #-1,d5
      moveq.l #0,d0
      cmpi.b #$f2,3(a0)
      beq.s losoftmod
      cmpi.b #$fb,3(a0)
      bne addcounthandle
      moveq.l #1,d0
losoftmod:
      move.b 3+1(a0),d1
      moveq.l #0,d2
      moveq.l #4,d3
      move.b d1,d2
      asr.b d3,d1
      ext.w d1
      lsl.b d3,d2
      asr.b d3,d2
      ext.w d2
      asl.w d0,d2
      move.w d2,32+16(a4)
      move.w d1,32+16+2(a4)
      addq.l #3,a0
      bra addcounthandle
nonovolume:
 andi.w #$000f,d0
  beq.s novolume
 add.w d0,d0
 lea.l strtvolus-ms(a5),a1
 move.w 0(a1,d0.w),d6
 lsl.w #8,d6
 move.b volumetable-ms+1(a5,d0.w),d6
 btst #7,2(a0)
  bne.s modulate

 move.b (a0),d2
 andi.w #$00f0,d2
 lsr.w #4,d2
 add.b d5,d2
 swap d4
 rol.w #8,d4
 move.b d2,d4
 rol.w #8,d4
 swap d4
 lsl.w #2,d2
 move.l myinstrvecs-ms(a5,d2.w),d5
 add.w d2,d2

 move.w d7,$096(a6)
 or.w d7,dmaoff-ms(a5)
 or.w d7,dmaon-ms(a5)

 lea.l myinstrlens-ms(a5),a1
 tst.w 0(a1,d2.w)
 beq novolume
 btst #7,1(a0)
 beq.s setoneshotdata
     move.w 2(a1,d2.w),d4
     beq.s setoneshotdata
     move.l d5,0(a5,d3.w)
     moveq.l #0,d0
     move.w 4(a1,d2.w),d0
     add.l d0,0(a5,d3.w)
     move.w 6(a1,d2.w),4(a5,d3.w)
     bra.s setrestdata
setoneshotdata:
     move.l defsndptr-ms(a5),0(a5,d3.w)
     move.w #1,4(a5,d3.w)
     move.w 0(a1,d2.w),d4
setrestdata:
     lsr.w #1,d4
addcounthandle:
 addq.l #3,a0
  cmpi.b #$f3,(a0)
  bne.s nosoftmodpseudo
   addq.l #6,a0
nosoftmodpseudo:
 cmpi.b #$ff,(a0)
 bne.s nonewmacro
   bsr.s getnewmacro
   tst.w d1          ; am ENDE kein ueberklingen !
   beq.s nonewmacro
    cmpi.b #$f0,(a0)
    bne.s nonewmacro
      move.b 2(a0),d0
      andi.w #$003f,d0
      addq.w #1,d0
      add.w d0,d0
      add.w d0,(a2)
      bra.s addcounthandle
nonewmacro:
 move.l a0,16(a4)
 rts

getnewmacro:
 moveq.l #1,d1
 addq.l #2,(a4)
 move.l (a4),a1
 tst.w (a1)
 bne.s nopause2play
  move.w pattlen-ms(a5),d1
  add.w d1,d1
  add.w d1,(a2)
  bra.s getnewmacro
nopause2play:
 cmpi.w #999,(a1)
  bne.s no_melody_carry
    move.l a3,(a4)
    move.l a3,a1
    moveq.l #0,d1
    or.b d7,internfinished-ms(a5)
no_melody_carry:
 move.w (a1),d0
 lsl.w #2,d0
 lea.l mymacrovecs,a1
 move.l 0(a1,d0.w),a0
 rts


; required data
; -----

 dc.b ' music by MusicMakerV8/4 CHANNEL VERSION - - Written by '
 dc.b 'DIRE CRACKS Inc.',$0d,$0a
 cnop 0,2
cianame:   dc.b 'ciab.resource',0
ciabase:   dc.l 0
dosname:   dc.b 'dos.library',0
audname:   dc.b 'audio.device',0
 cnop 0,2
mysysint6: dc.l 0,0
           dc.b 2,127
           dc.l int6name
           dc.l 0          ; data
           dc.l systemint6 ; code
int6name:  dc.b 'MMV8/Level 6',0
 cnop 0,2
oldinterrupt:  dc.l 0
 cnop 0,2

MACRO_0:
 dc.b $f0,$00,$3f,$ff
 cnop 0,2

notetable:         dc.w 856,832,808,784,760,740,720,700,680,660,640,622
                   dc.w 604,588,572,556,540,524,508,494,480,466,452,440
                   dc.w 428,416,404,392,380,370,360,350,340,330,320,311
                   dc.w 302,294,286,278,270,262,254,247,240,233,226,220
                   dc.w 214,208,202,196,190,185,180,175,170,160,151,143
                   dc.w 135,127,120,113
mystructure: ; -0-
ms:
volumetable:       dc.w 0,1,2,3,5,7,11,16,22,28,34,40,46,52,58,64
lastvolumes:       dc.w 0,0,0,0,0,0,0,0  ; NOTHING INBETWEEN
myinstrvecs:       ds.l INSTNUM
myinstrlens:       ds.w INSTNUM*8
strtvolus:         dc.w 0,1,2,3,5,7,11,16,22,28,34,40,46,52,58,64
speed:             dc.w 0
playspeed:         dc.w 0
curspeedfact1:     dc.w 1
curspeedfact2:     dc.w 1
pattlen:           dc.w 0
whattodo:          dc.w 0
resttime:          dc.w 0
dmaoff:            dc.w $0000
dmaon:             dc.w $8000
channelshandle:    dc.b 0
internfinished:    dc.b 0
oneshot:           dc.b 0
_channelsfinished: dc.b 0
_dtchannelsfinished: dc.b 0
soundenable:       dc.b -1
_fadeflag:         dc.b 0
_fadefinished:     dc.b 0
fadehalf:          dc.b 0
_mutedchannels:    dc.b 0
sounddatavalid:    dc.b 0
intremoved:        dc.b -1
doloudness:        dc.b 0
calltheroutine:    dc.b 0
dofadein:          dc.b 0
dofadeout:         dc.b 0
levelset:          dc.b 0
newformat:         dc.b 0
 cnop 0,2
extdatasize:       dc.l 0
fadespeedin:       dc.w 63
fadespeedout:      dc.w 21
defsndptr:         dc.l 0
subroutine:        dc.l 0
called:            dc.w 0
lfoinstptrs:       ds.l 16         ; N.T.P.I. !!!
currlfotable:      ds.l INSTNUM+1  ;
calcspeed:         dc.w 0
nextdata0:         dc.l 0,0
nextdata1:         dc.l 0,0
nextdata2:         dc.l 0,0
nextdata3:         dc.l 0,0
modultable:        dc.w MODUL_3-ms,MODUL_2-ms,MODUL_1-ms,MODUL_0-ms
MODUL_0:           dc.l nextdata0-ms,1,0,0,0
MODUL_1:           dc.l nextdata1-ms,2,0,0,0
MODUL_2:           dc.l nextdata2-ms,4,0,0,0
MODUL_3:           dc.l nextdata3-ms,8,0,0,0

COUNTERS:          dc.l 0,0,0,0                               ; NOTHING TO
MACRO_C:           dc.l 0,0,0,0                               ; BE PUT
SCOUNT:            dc.w 1,0,1,0,1,0,1,0                       ; INBETWEEN !
SOFTMOD:           dc.l 0,0,0,0                               ; !!!
VIBRATO:           ds.l CHANNELMAX                            ; !!!

lastperiods:       dc.l 0,0,0,0,0,0
                   dc.l 0,0,0,0,0,0
                   dc.l 0,0,0,0,0,0
                   dc.l 0,0,0,0,0,0
loudnesstable:
 dc.w 256,264,268,273,277,287,292,297,303,309,315,321,327,334,341
  dc.w 348,356,364,372,372,381,390,399,399
 dc.w 409,420,420,431,431,442,442,455,468,468,468,481,481,481,496
  dc.w 496,496,512,512,512,512,512,512,512
 dc.w 512,512,512,512,511,511,511,511,511,511,511,511,511,511,511
  dc.w 511

currsongnum:       dc.l 0
basicfilename:     dc.l 0
filenamebuffer:    ds.b 256
filenameendptr:    dc.l 0
mydosbase:         dc.l 0
filehandle:        dc.l 0
memhandle:         dc.l 0
memlen:            dc.l 0
xxxbuf:            dc.l 0
lastinitedsong:    dc.l -1
 cnop 0,2
fibbits:           dc.w 0
fibtable:          dc.l 0
samplefileadr:     dc.l 0
samplefilesize:    dc.l 0
ipfileptr:         dc.l 0
allocmap: dc.b 15
audopen:  dc.b 0
 cnop 0,4
rt_endskip:

 SECTION MacroVecs,BSS

FileName:   ds.b MAX_NAME_LENGTH

pf:
basicptrfield:
basicmacrosdata:  ds.l 1
basicmacroslen:    ds.l 1
basicinstrsdata:    ds.l 1
basicinstrslen:      ds.l 1
basicgivenoneshot:    ds.l 1
basicsubroutine:       ds.l 1
 ds.l 6*(20-1)
loadfibtable:
myinfoblock:       ds.b 270
mymacrovecs:       ds.l 999 ; optional

auddevio:      ds.w 34
audreply:      ds.w 17

 END

