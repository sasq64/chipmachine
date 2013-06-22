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

; MusicMaker 8 channel EaglePlayer header
; Version 4.0
;
; EPPlayer8.o is based on "mplayer.o" supplied in MM's Developer Pack 2.0
; For more info, see "EP4.a"

; This player is capable of playing NEW v2.4 song files.

	incdir	Include:
	include	misc/EaglePlayer.i
	include	LVO3.0/exec_lib.i

even MACRO
   cnop 0,2
   ENDM


   SECTION Player,Code

   PLAYERHEADER PlayerTagArray

   dc.b '$VER: MusicMaker 8-channel player module V4.1 (16 Dec 93)',0
   even

PlayerTagArray
   dc.l  DTP_RequestDTVersion,$1fffffff
   dc.l  DTP_PlayerVersion,4
   dc.l  DTP_PlayerName,PName
   dc.l  DTP_Creator,CName
   dc.l  DTP_Check2,Chk
   dc.l  EP_Check3,Chk
   dc.l  DTP_Config,Config

   dc.l  DTP_ExtLoad,Loadexternal

   dc.l  DTP_InitPlayer,InitPlay
   dc.l  DTP_EndPlayer,EndPlay
   dc.l  DTP_StartInt,StartSnd
   dc.l  DTP_StopInt,StopSnd

   dc.l  EP_Voices,setvoices
   dc.l  DTP_Volume,VolVoices  ;SetVolume
   dc.l  DTP_Balance,VolVoices   ; ???

   dc.l  EP_StructInit,structinit
   dc.l  EP_Flags
;   dc.l  EPB_Songend+EPB_Packable+EPB_Volume+EPB_VolVoices+EPB_Voices+EPB_Restart+EPB_Analyzer+EPB_ModuleInfo+EPB_Save
   dc.l  EPB_Songend+EPB_Packable+EPB_Balance+EPB_Volume+EPB_VolVoices+EPB_Voices+EPB_Restart+EPB_Analyzer+EPB_ModuleInfo+EPB_Save

   dc.l  EP_Save
   dc.l  SaveMod

   dc.l  EP_Get_ModuleInfo,Getinfos
   dc.l  TAG_DONE

*-----------------------------------------------------------------------*
;
; Player/Creatorname und lokale Daten

PName: dc.b 'MusicMaker8',0
CName: dc.b 'Thomas Winischhofer',0
uadename:	dc.b 'uade.library',0

	   even

_DOSBase:  dc.l 0
dtbase: dc.l 0
uadebase:	dc.l 0

MAX_NAME_LENGTH	equ	64

FileBuffer:ds.b 8

 cnop 0,4
Playing      dc.b 0
packedornot: dc.b 0
ownmem:      dc.b 0
 cnop 0,4
volvoice1: dc.w 0
volvoice2: dc.w 0
volvoice3: dc.w 0
volvoice4: dc.w 0

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
   lea.l MM_InfoBuffer(PC),a0
   clr.l (a0)

   move.l basicinstrsdata(PC),d0
   beq.s .rts
   move.l d0,a1
   cmpi.l #'SEI1',(a1)
   bne.s .rts
   cmpi.w #'XX',4(a1)
   bne.s .rts
   move.w 6(a1),12(a0)  ; maxsamples eintragen

   moveq.l #0,d1
   moveq.l #0,d2
   
   lea.l 8(a1),a1
   moveq #INSTNUM-1,d0
.chksam:
    moveq.l #0,d3
    move.w (a1),d3
    beq.s .nosam
     add.l d3,d1
     addq.l   #1,d2
.nosam:
    addq.l   #8,a1
   dbf d0,.chksam

   addi.l #8+(INSTNUM*8)+8,d1  ;8='SEI1XX'## + 8 zur Sicherheit
   move.l d1,20(A0)      ;Samplessize
   move.l d2,4(A0)       ;Sampleanzahl
   
   move.l basicmacrosdata(PC),d0
   beq.s .rts
   move.l d0,a1
   cmpi.w #'SE',(a1)
   bne.s .rts

   lea 2(a1),a2
   lea Modname(pc),a3
   moveq.l #19,d0
.copy: move.b (a2)+,(a3)+
   dbf d0,.copy

   move.l basicmacroslen(PC),d0
   add.l basicinstrslen(PC),d0
   move.l d0,28(a0)

   move.l #MI_Samples,(a0)
.rts:
   rts

;======================== Save Funktion =====================================

SaveMod:
   move.l basicmacrosdata(PC),d0
   beq .Nosave

;  move.l   dtg_PathArrayPtr(a5),a0
;  clr.b (a0)        ; clear Path
;  move.l   dtg_CopyDir(a5),a0   ; copy dir into patharray
;  jsr   (a0)
;  move.l   dtg_CopyFile(a5),a0  ; append filename
;  jsr   (a0)

   move.l dtg_PathArrayPtr(A5),A0
   move.l a0,a2
   move.w #300-1,d0
.loop move.b (a0)+,(a2)+
   dbeq d0,.loop

   move.l a0,-(a7)
   jsr getfilename
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

   move.l basicmacrosdata(PC),EPG_ARG1(a5)
   move.l basicmacroslen(PC),EPG_ARG2(a5)
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

   move.l basicinstrsdata(PC),EPG_ARG1(a5)
   move.l basicinstrslen(PC),EPG_ARG2(a5)
   move.l dtg_PathArrayPtr(a5),EPG_ARG3(a5)
   moveq.l #-1,d0
   move.l d0,EPG_ARG4(a5)
   clr.l EPG_ARG5(a5)
   moveq.l #5,d0
   move.l d0,EPG_ARGN(a5)

   move.l EPG_SaveMem(a5),a0
   jmp (A0)

.Nosave:
  moveq #EPR_NoModuleLoaded,d0
.error:
  rts

*-----------------------------------------------------------------------*
;
; Testet auf Modul

Chk                  ; MusicMaker EXT-song ?
   move.l   dtg_ChkData(a5),a0
   cmpi.w   #'SE',(a0)
   bne ChkFail

   jsr _isstdsong
   bne ChkFail

   move.b 25(a0),d0
   cmpi.b #123,d0
   bcs ChkFail
   cmpi.b #224,d0
   bhi ChkFail

   move.l dtg_PathArrayPtr(a5),a0
   lea.l FileName,a2
   move #MAX_NAME_LENGTH-1,d0
.l4: move.b  (a0)+,(a2)+
   dbeq d0,.l4

   tst.w d0
   bmi ChkFail

   subq.w #6,a2
   cmpi.b #".",-1(a2)
   bne ChkFail
   move.l a2,-(a7)
   lea.l .l3(pc),a0
.l1:   move.b (a2)+,d0
      beq.s .l2
      bclr #5,d0
      cmp.b (a0)+,d0
     beq.s .l1
   move.l (a7)+,a2
   bra ChkFail
.l3: dc.b 'SDATA',0
   cnop 0,2
.l2:
   move.l (a7)+,a2

ChkUnPkd
   move.b   #'i',(a2)+
   clr.b (a2)

   move.l #FileName,d1
   move.l #1005,d2
   move.l a6,-(a7)
   move.l _DOSBase(PC),a6
   jsr -30(a6)
   movem.l (a7)+,a6
   move.l d0,d4
   bne.s ChkOpen
ChkPacked
   move.b #'p',(a2)+
   clr.b (a2)

   move.l  #FileName,d1
   move.l #1005,d2
   move.l a6,-(a7)
   move.l _DOSBase(PC),a6
   jsr -30(a6)
   movem.l (a7)+,a6
   move.l d0,d4
   beq.s ChkFail
ChkOpen
   move.l d4,d1
   move.l #FileBuffer,d2    ;
   moveq.l #8,d3            ; Open/Read/Close is better than Lock/UnLock
   move.l a6,-(a7)          ;
   move.l _DOSBase(PC),a6   ; (What, if r-flag is clear ???)
   jsr -42(a6)              ;
   movem.l (a7)+,a6         ;
   move.l d0,d3

   move.l d4,d1
   move.l a6,-(a7)
   move.l _DOSBase(PC),a6
   jsr -36(a6)
   movem.l (a7)+,a6
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
   move.w EPG_Voices(a5),(a0)
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
   moveq #-2,d2
   jsr -84(a6)
   move.l d0,d7
   move.l d0,d1
   jsr -90(a6)
   tst.l d7
   beq.s .trypacked     ;kein ".i" - File ? -> ".ip" versuchen

   clr.b packedornot    ;nicht gepacktes file

   move.l #1,EPG_ARGN(a5)
   move.l #$10001,EPG_ARG1(a5)    ; kein mensch braucht CHIP RAM!
   move.l EPG_NewLoadFile(a5),a0
   jmp (a0)

.trypacked:
   lea.l f.ip(pc),a4    ; join '.ip'
   bsr .getpath

   st packedornot       ; gepacktes file

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
.loop move.b (a0)+,(a2)+
   dbeq d0,.loop
   subq.l #7,a0
   clr.b (a0)
   move.l a4,a0
   move.l dtg_CopyString(a5),a1
   jsr (a1)
   movem.l  (a7)+,d0-d7/a0-a5
   rts
f.i:  dc.b  '.i',0
f.ip: dc.b  '.ip',0
   cnop  0,4

*-----------------------------------------------------------------------*
;
; Einmalige Initialisierung des Players

Config:
   move.l dtg_DOSBase(a5),_DOSBase
   moveq.l #0,d0
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

   moveq.l #0,d0
   move.l dtg_GetListData(a5),a0
   jsr (a0)
   tst.l d0
   beq.s EndPlay3

   lea.l mystructure(PC),a1
   move.l a0,basicmacrosdata-ms(a1)
   move.l d0,basicmacroslen-ms(a1)

   move.l dtg_PathArrayPtr(a5),a0
   moveq #0,d0
   jsr _loadandinit
   tst.l d0
   bne.s EndPlay3

   move.l dtg_SongEnd(a5),mm_songend

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
   move.l   dtg_AudioFree(a5),a0
   jsr   (a0)
EndPlay2:
   jsr   _removeloaded
EndPlay3:
   moveq.l #-1,d0            ; set Error
   rts

*-----------------------------------------------------------------------*
;
; Start Sound

StartSnd:
	bsr	uade_time_critical_off

;*************************************************************************
; The following is REALLY important:
; Starting version 3, the mplayer features SetupCacheControl() to handle
; 68020,30,40 caches and 68040 CachePreDMA(). By default, the caches are
; ignored. On machines equipped with 68040, especially, this can be
; troubleful in case COPYBACK mode is ON. Arguments to this functions
; are: d0: Kickstart version, because ClearCache() and CachePreDMA() are
;          supported from 37, yet.
;      d1: System-Attn Flags from Execbase for sensing the CPU type.
;
; This function MUST be called AFTER a song has been initialized.
;
; NOTE: The mplayer module requires a CORRECTLY INITIALIZED
; ¯¯¯¯¯ ExecBase area due to VBR handling routines inside  !!!!

   move.l $4,a0
   move.w 20(a0),d0    ; execbase->Version
   move.w 296(a0),d1   ; execbase-> AttnFlags
   jsr _setupcachecontrol      ; SetupCacheControl()

;*************************************************************************

   moveq #0,d0
   jsr _generalsndreset
   jsr _soundon
   st Playing
   rts

*-----------------------------------------------------------------------*
;
; Stop Sound

StopSnd:
   sf Playing
   jsr   _soundoff
   rts

*-----------------------------------------------------------------------*
;
; Set Volume               ... obsolete ...
;
;SetVolume
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
; SetVoices:

setvoices:     ; create 8-bit mask from 4-bit (1010 -> 11001100)
 move.w d0,myvoicedata+UPS_DMACon
 movem.l d1-d2,-(a7)
 not.b d0
 lsl.b #4,d0
 moveq.l #0,d1
 moveq.l #0,d2
 roxl.b #1,d0
 roxl.b #1,d1
 add.b d1,d1
 roxl.b #1,d0
 roxl.b #1,d1
 add.b d1,d1
 roxl.b #1,d0
 roxl.b #1,d1
 add.b d1,d1
 roxl.b #1,d0
 roxl.b #1,d1
 add.b d1,d1
 move.b d1,d2
 lsr.b #1,d2
 or.b d2,d1
 move.b d1,d0
 movem.l (a7)+,d1-d2
 jsr _mutechannels
 rts

mm_songend: dc.l 0

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
 rts

myvoicedata:
 ds.b UPS_SizeOF

; ds.w 4*(2+7)
; dc.w 15
; dc.w 4+8  ; no sample-start provided. "Quadroscope" -> "Oktoscope"????
; dc.w 0
; dc.w 0

; ===================================================================================

;
; MULTI-CHANNEL-PLAYER V1.9   PCS 1.2  FAST-VOL-MIX
; (C) DIRE CRACKS Inc.
; All rights reserved.
; Linking module - not executable !
;

; ==== SPECIAL EaglePlayer Version ====

INSTNUM    equ 36
CHANNELMAX equ 8

_getsongname:   ; not implemented
_newsndinit:
_newremovesong:
_newsndreset:
_newsndresetoneshot:
_removeallsongs:
 moveq.l #0,d0
 rts

_setspeed:
 moveq.l #0,d0
 move.w speed(PC),d0
 rts

_newloadandinit:; a0=filename, d0=oneshot, a1=subroutine
 movem.l d2-d7/a2-a6,-(a7)
 lea.l mystructure(PC),a5
 move.l a1,loadandinitrout-ms(a5)
 bra.s intlabel

_loadandinit:   ; a0=filename, d0=oneshot
 movem.l d2-d7/a2-a6,-(a7)
 lea.l mystructure(PC),a5
 clr.l loadandinitrout-ms(a5)
intlabel:
 move.l a0,basicfilename-ms(a5)
 move.l d0,basicgivenoneshot-ms(a5)

 tst.l basicinstrsdata-ms(a5)
 beq.s noneinmem
  bsr _soundoff
  bsr _generalsndremove
  bsr freeinstruments
  bsr freevoltable
  bsr freemixbuf
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
  move.l a0,basicinstrsdata-ms(a5)
  move.l d0,basicinstrslen-ms(a5)

  clr.b ownmem
  move.l basicinstrsdata-ms(a5),a0
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
 moveq.l #1,d0
 move.l dtg_GetListData(a5),a0
 jsr (a0)
 move.l (a7)+,a5
 move.l a0,samplefileadr-ms(a5)
 move.l d0,samplefilesize-ms(a5)

 bsr _getunpackedinstlen
 move.l d0,basicinstrslen-ms(a5)
 move.l #$10001,d1    ; NO CHIP RAM
 jsr -198(a6)
 move.l d0,basicinstrsdata-ms(a5)
 beq instmemerror
 st ownmem

 move.l d0,a1
 move.l samplefileadr-ms(a5),a0
 bsr _decrunchinstrs

; lea.l myinfoblock-ms(a5),a2
;
; move.l samplefileadr-ms(a5),a1
; move.l a2,-(a7)
; move.b (a1)+,(a2)+
; move.b (a1)+,(a2)+
; move.b (a1)+,(a2)+
; move.b (a1)+,(a2)+
; move.b (a1)+,(a2)+
; move.b (a1)+,(a2)+
; move.b (a1)+,(a2)+
; move.b (a1)+,(a2)+
; move.l (a7)+,a2
;
; moveq.l #0,d6
; moveq.l #26,d7
; cmpi.l #'SEI1',(a2)
; bne.s nonewinsts
;  cmpi.w #'XX',4(a2)
;  bne.s nonewinsts
;    moveq.l #8,d6
;    move.w 6(a2),d7
;nonewinsts:
; lea.l myinstrlens-ms(a5),a0
; moveq.l #INSTNUM-1,d0
.l;1: clr.l (a0)+
;    clr.l (a0)+
;  dbra d0,.l1
; move.l samplefileadr-ms(a5),a0
; lea.l 0(a0,d6.l),a0
; lea.l myinstrlens-ms(a5),a1
; move.l d7,d0
; lsl.w #3,d0
;.l2: move.b (a0)+,(a1)+
;  subq.w #1,d0
; bne.s .l2
;
; move.w (a0)+,fibbits-ms(a5)
;
; lea.l myinstrlens-ms(a5),a0
; moveq.l #INSTNUM-1,d5
; moveq.l #0,d0
; moveq.l #0,d2
;findinstlenspacked:
;  moveq.l #0,d1
;  move.w (a0),d1
;  beq.s .l1
;  add.l d1,d0
;  mulu fibbits-ms(a5),d1
;  addq.l #8,d1
;  lsr.l #3,d1
;  add.l d1,d2
;.l1:
;  addq.l #8,a0
; dbra d5,findinstlenspacked
; addi.l #8+(INSTNUM*8),d0  ; 8='SEI1XX'##
; addq.l #8,d0
; move.l d0,basicinstrslen-ms(a5)
; lsl.l #3,d7
; add.l d7,d2
; addq.l #2,d2
; moveq.l #1,d3
; move.w fibbits-ms(a5),d4
; lsl.w d4,d3
; add.l d3,d2
; add.l d6,d2
;
; move.l samplefileadr-ms(a5),a0
; add.l d2,a0
;
; lea.l myinfoblock-ms(a5),a1
; moveq.l #15*2,d0
;.l3: move.b (a0)+,(a1)+
;  subq.w #1,d0
; bne.s 3$
;
; move.l samplefileadr-ms(a5),a0
; move.l d7,d2
; add.l d6,d2
; addq.l #2,d2
; add.l d2,a0
; move.l a0,ipfileptr-ms(a5)
;
; lea.l myinfoblock-ms(a5),a0
; moveq.l #15-1,d7
; moveq.l #0,d0
;findlfolenspacked:
;  moveq.l #0,d1
;  move.w (a0)+,d1
;  add.l d1,d0
; dbra d7,findlfolenspacked
; addi.l #15*2,d0
; addq.l #8,d0  ; for sure, babe !
; add.l d0,basicinstrslen-ms(a5)
;
; move.l basicinstrslen-ms(a5),d0
; move.l #$10001,d1
; move.l 4,a6
; jsr -198(a6)
; move.l d0,basicinstrsdata-ms(a5)
; beq instmemerror
; st ownmem
;
; move.l d0,a1
; move.l #'SEI1',(a1)+
; move.w #'XX',(a1)+
; move.w #INSTNUM,(a1)+
;  lea.l myinstrlens-ms(a5),a0
;  moveq.l #INSTNUM-1,d0
;copyinstrlensaaa:
;   move.l (a0)+,(a1)+
;   move.l (a0)+,(a1)+
;  dbra d0,copyinstrlensaaa
; clr.l (a1)+
; move.l a1,a4
; lea.l loadfibtable-ms(a5),a2
; move.l a2,fibtable-ms(a5)
;
; moveq.l #1,d3
; move.w fibbits-ms(a5),d4
; lsl.w d4,d3
; move.l ipfileptr-ms(a5),a0
;.l4: move.b (a0)+,(a2)+
;  subq.l #1,d3
; bne.s 4$
; move.l a0,ipfileptr-ms(a5)
;
; movem.l a0-a1/d1-d2,-(a7)
; lea.l myinstrlens-ms(a5),a0
; moveq.l #INSTNUM-1,d1
; moveq.l #0,d0
;findpackbuflenloop:
;  cmp.w (a0),d0
;  bhi.s notlonger
;    move.w (a0),d0
;notlonger:
;  addq.l #8,a0
; dbra d1,findpackbuflenloop
; tst.w d0
; beq.s lenis000
;   mulu fibbits,d0
;   lsr.l #3,d0
;   addi.l #1000,d0
;lenis000:
; movem.l (a7)+,a0-a1/d1-d2
; move.l d0,memlen-ms(a5)
; move.l #$10001,d1
; move.l $4,a6
; jsr -198(a6)
; move.l d0,memhandle-ms(a5)
; beq instmemerror1
;
; lea.l myinstrlens-ms(a5),a3
; moveq.l #INSTNUM-1,d7
;packinstrsloop1:
;  moveq.l #0,d0
;  move.w (a3),d0
;  beq.s leniszero3
;   mulu fibbits-ms(a5),d0
;   addq.l #8,d0
;   lsr.l #3,d0
;
;   movem.l d0-d3/a0-a2/a6,-(a7)
;   move.l memhandle-ms(a5),a0
;   move.l ipfileptr-ms(a5),a1
;.l5:  move.b (a1)+,(a0)+
;    subq.l #1,d0
;   bne.s 5$
;   move.l a1,ipfileptr-ms(a5)
;   movem.l (a7)+,d0-d3/a0-a2/a6
;
;   moveq.l #0,d0
;   move.w (a3),d0
;   moveq.l #0,d2
;   move.w fibbits-ms(a5),d2
;   move.l memhandle-ms(a5),a0
;   move.l a4,a1
;   add.l d0,a4
;   move.l a4,-(a7)
;   bsr decompress
;   move.l (a7)+,a4
;leniszero3:
;  addq.l #8,a3
; dbra d7,packinstrsloop1
; move.l a4,d0
; btst #0,d0
; beq.s itsOK1
;  addq.l #1,a4
;itsOK1:
; move.l ipfileptr-ms(a5),a0
; move.l samplefilesize-ms(a5),d0
; add.l samplefileadr-ms(a5),d0
; sub.l a0,d0
;.l1: move.b (a0)+,(a4)+
;  subq.l #1,d0
; bne.s .l1
; 
; move.l $4,a6
; move.l memhandle-ms(a5),a1
; move.l memlen-ms(a5),d0
; jsr -210(a6)

instrsloaded:
 move.l basicmacrosdata-ms(a5),a0
 cmpi.w #'SE',(a0)    ; in library-loaded songs there MUST be this mark!
 bne fileformaterror
 jsr _isstdsong
 bne fileformaterror  ; if STD-Song

 move.l $4,a6
 move.l #$51c0,d0
 move.l #$10001,d1
 jsr -198(a6)
 move.l d0,mytable-ms(a5)
 beq tablememerror

 move.l basicmacrosdata-ms(a5),a0
 bsr _obtainmixbuflen
 move.l d0,-(a7)
 lsl.l #3,d0
 move.l d0,mymixbuflen-ms(a5)
 move.l #$10003,d1
 move.l $4,a6
 jsr -198(a6)
 move.l d0,mymixbufptr-ms(a5)
 movem.l (a7)+,d1
 beq mixbufmemerror
 move.l d0,a0
 moveq.l #8-1,d0
.l1:  move.l a0,-(a7)
     adda.l d1,a0
 dbra d0,.l1
 move.l a7,a0
 bsr _setmixbuffers
 lea.l 8*4(a7),a7

 move.l mytable-ms(a5),a0
 moveq.l #64,d0
 moveq.l #-1,d1
 bsr _maketables

 move.l basicinstrsdata-ms(a5),a1
 move.l basicmacrosdata-ms(a5),a2
 move.l loadandinitrout-ms(a5),a3
 move.l basicgivenoneshot-ms(a5),d0
 bsr _generalsndinit
 moveq.l #0,d0
exitloadandinit:
 movem.l (a7)+,d2-d7/a2-a6
 rts

instfileerror:
 moveq.l #101,d0
 bra.s exitloadandinit
instmemerror:
 bsr.s internalclosefile
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
 bsr.s internalclosefile
 bsr freeinstruments
 moveq.l #107,d0
 bra.s exitloadandinit
fileformaterror:
 bsr freesdata
 bsr.s internalclosefile
 bsr freeinstruments
 moveq.l #111,d0
 bra.s exitloadandinit
tablememerror:
 bsr freesdata
 bsr freeinstruments
 moveq.l #108,d0
 bra.s exitloadandinit
mixbufmemerror:
 bsr freesdata
 bsr freeinstruments
 bsr freevoltable
 moveq.l #109,d0
 bra.s exitloadandinit

internalclosefile:
 rts

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

_removeloaded:
 movem.l d2-d7/a2-a6,-(a7)
 bsr _soundoff
 bsr _generalsndremove
 lea.l mystructure(PC),a5
 bsr.s freesdata
 bsr.s freeinstruments
 bsr.s freevoltable
 bsr.s freemixbuf
 movem.l (a7)+,d2-d7/a2-a6
 rts

freesdata:
 move.l basicmacrosdata-ms(a5),d0
 beq.s .l1
 clr.l basicmacrosdata-ms(a5)
.l1:
 clr.l basicmacroslen-ms(a5)
 rts

freeinstruments:
 tst.b ownmem
 beq .l1
  move.l basicinstrsdata-ms(a5),d0
  beq.s .l1
  move.l d0,a1
  move.l basicinstrslen-ms(a5),d0
  move.l $4,a6
  jsr -210(a6)
  clr.l basicinstrsdata-ms(a5)
.l1:
  clr.l basicinstrslen-ms(a5)
 rts

freevoltable:
 move.l mytable-ms(a5),d0
 beq.s .l1
 move.l d0,a1
 move.l #$51c0,d0
 move.l $4,a6
 jsr -210(a6)
 clr.l mytable-ms(a5)
.l1:
 rts

freemixbuf:
 move.l mymixbufptr-ms(a5),d0
 beq.s .l1
 move.l d0,a1
 move.l mymixbuflen-ms(a5),d0
 move.l $4,a6
 jsr -210(a6)
 clr.l mymixbufptr-ms(a5)
.l1:
 rts

_generalsndremove:
 movem.l a5/a0/d0,-(a7)
 lea.l mystructure(PC),a5
 tst.l oldintvector-ms(a5)
 beq.s .l1
  bsr getvbr
  move.l d0,a0
  move.l oldintvector-ms(a5),$70(a0)
  clr.l oldintvector-ms(a5)
.l1:
 movem.l (a7)+,a5/a0/d0
 rts

* FROM HERE THERE ARE ALMOST NO EaglePlayer Adaptions contained.
* execept: SongEnd, Analyzer functions.
* They are all marked !


_mutechannels:     ; d0=bit0-3=1=channel muted
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
 cmpi.b #$ff,_channelsfinished
 seq d0
 ext.w d0
 ext.l d0
 rts

getvbr:
 movem.l d1-d2/a0-a1/a6,-(a7)
 move.l $4,a6
 moveq.l #0,d2
 move.w 296(a6),d1
 andi.w #$0003,d1
 beq.s .l1
   jsr -150(a6)
;   mc68030
   movec VBR,d2
;   mc68000
   jsr -156(a6)
.l1:
 move.l d2,d0
 movem.l (a7)+,d1-d2/a0-a1/a6
 rts

_soundon:
 movem.l a0/a5-a6/d0,-(a7)
 bsr hardwareinit
 bsr initmixloops
 lea.l mystructure(PC),a5
 move.w #$0780,$09a(a6)
 sf soundenable-ms(a5)
 move.w speed-ms(a5),d0
 lsr.w #2,d0
 move.w d0,$0a4(a6)
 move.w d0,$0b4(a6)
 move.w d0,$0c4(a6)
 move.w d0,$0d4(a6)
 move.w d0,hwchannel0+36-ms(a5)
 move.w d0,hwchannel1+36-ms(a5)
 move.w d0,hwchannel2+36-ms(a5)
 move.w d0,hwchannel3+36-ms(a5)
 moveq.l #0,d0
 move.w #230,d0
 swap d0
 move.l d0,$0a6(a6)
 move.l d0,$0b6(a6)
 move.l d0,$0c6(a6)
 move.l d0,$0d6(a6)
 move.l d0,hwchannel0+32-ms(a5)
 move.l d0,hwchannel1+32-ms(a5)
 move.l d0,hwchannel2+32-ms(a5)
 move.l d0,hwchannel3+32-ms(a5)
 move.w #$0780,$09c(a6)
 move.w #$800f,$096(a6)
 move.w #$8780,$09a(a6)
 movem.l (a7)+,a0/a5-a6/d0
 rts

_soundoff:
 movem.l a5-a6/d0,-(a7)
 lea.l mystructure(PC),a5
 lea.l $dff000,a6
 st soundenable-ms(a5)
 move.w #$0780,$09a(a6)
 move.w #$0780,$09c(a6)
 bsr hardwareinit
 movem.l (a7)+,a5-a6/d0
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

_obtainmixbuflen: ; a0=sdata-start, result: d0=mixbuflen
 movem.l d1/a0-a1,-(a7)
 cmpi.w #'SE',(a0)
 bne.s .l1
   adda.w #22,a0
.l1:
 tst.b 2(a0)      ; all on SLOW-setting?
 beq.s nonewmixer
   move.w #113,d0
   bra.s haveit
nonewmixer:
 moveq.l #0,d1
 move.b 3(a0),d1
 lea.l 8-226+2(a0),a1
 cmpi.b #$fe,4(a0)  ; filemarkv2.4 ?
 bne.s .l1
   adda.w 6(a0),a1
.l1:
 adda.w d1,a1
 adda.w d1,a1
 move.w d1,d0
 addq.w #1,d0
 subi.w #113,d1
 bmi.s haveit
findhighest:
  cmp.w -(a1),d0
  bls.s isntit
   move.w (a1),d0
isntit:
 dbf d1,findhighest
haveit:
 move.l #3579545,d1
 divu d0,d1
 cmpi.b #$fe,4(a0)
 bne.s .l1
   adda.w 6(a0),a0
.l1:
 mulu 236(a0),d1
 divu #31250,d1
 moveq.l #0,d0
 move.w d1,d0
 move.l d0,d1
 lsr.w #2,d1
 add.l d1,d0
 bclr #0,d0
 movem.l (a7)+,d1/a0-a1
 rts

_setmixbuffers:   ; a0=vectorfield
 movem.l a0/a5,-(a7)
 lea.l hwchannel0(PC),a5
 move.l (a0)+,(a5)+
 move.l (a0)+,(a5)+
 lea.l hwchannel1(PC),a5
 move.l (a0)+,(a5)+
 move.l (a0)+,(a5)+
 lea.l hwchannel2(PC),a5
 move.l (a0)+,(a5)+
 move.l (a0)+,(a5)+
 lea.l hwchannel3(PC),a5
 move.l (a0)+,(a5)+
 move.l (a0)+,(a5)+
 movem.l (a7)+,a0/a5
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
 addq.l #4,d1           ; defsnd/eoredflag
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


_generalsndreset:    ; d0=oneshot
 movem.l d0-d7/a0-a6,-(a7)
 lea.l mystructure(PC),a5
 move.l myinstrsdata-ms(a5),a1
 move.l mymacrosdata-ms(a5),a2
internsndreset:
 sf internfinished-ms(a5)
 sf _fadeflag-ms(a5)
 sf _fadefinished-ms(a5)
 sf channelshandle-ms(a5)
 sf called-ms(a5)
 sf doloudness-ms(a5)
 sf dofadein-ms(a5)
 sf dofadeout-ms(a5)
 sf speedtoupdate-ms(a5)
 sf _mutedchannels-ms(a5)
 move.b d0,oneshot-ms(a5)
 bsr setbacklaststuff
 move.w (a2)+,d0
 cmpi.w #'SE',d0
 bne.s nosongname
   adda.w #20,a2
   move.w (a2)+,d0
nosongname:
 move.b d0,channelsenable-ms(a5)
 not.b d0
 move.b d0,_channelsfinished-ms(a5)
 move.b d0,_dtchannelsfinished-ms(a5)
 move.b (a2)+,mixertype-ms(a5)
 clr.w hi-ms(a5)
 move.b (a2)+,hi+1-ms(a5)
 cmpi.b #$fe,(a2)
 bne.s .l1
   adda.w 2(a2),a2
.l1:
 addq.w #4,a2 ; skip mixplayer,packed
 move.l a2,a0
 suba.w #226,a0
 move.l a0,hightable-ms(a5)
 adda.w #226,a2
 move.w (a2)+,pattlen-ms(a5)
 moveq.l #0,d0
 move.w (a2)+,d0
 move.w d0,speed-ms(a5)
 move.w d0,origspeed-ms(a5)
 move.w d0,calcspeed-ms(a5)
 move.w d0,d1
 mulu #5*23,d1
 move.l d1,mixcalcspeed-ms(a5)
 bsr ptrsinit
 bsr hardwareinit
 lea.l hwchannel3-ms(a5),a0
inithwchannels:
   move.l #$10000,42(a0)
   clr.w 8(a0)
 suba.w #hwchannel3-hwchannel2,a0
 cmpa.l #hwchannel0,a0
 bge.s inithwchannels
 move.l insteoredflag-ms(a5),a0
 tst.l (a0)
 bne.s .l1
  not.l (a0)
  lea.l myinstrvecs-ms(a5),a0
  lea.l myinstrlens-ms(a5),a1
  bsr calculateinstruments
.l1:
 movem.l (a7)+,d0-d7/a0-a6
 rts
setbacklaststuff:
 movem.l d0/a0,-(a7)
 moveq.l #CHANNELMAX-1,d0
 lea.l lastperiods-ms(a5),a0
setbacklaststuffto0:
   clr.l (a0)+
   move.w #$0100,(a0)+
   clr.w (a0)+
   clr.l (a0)+
   clr.l (a0)+
   clr.l (a0)+
   clr.l (a0)+
 dbra d0,setbacklaststuffto0
 moveq.l #CHANNELMAX-1,d0
 lea.l lastvolumes-ms(a5),a0
setbacklastvolumes:
   clr.l (a0)+
  dbra d0,setbacklastvolumes
 clr.l VIBRATO-ms(a5)
 clr.l VIBRATO+4-ms(a5)
 clr.l VIBRATO+8-ms(a5)
 clr.l VIBRATO+12-ms(a5)
 movem.l (a7)+,d0/a0
 rts

_generalsndinit:               ; in d0 = oneshot(=<>0)
 movem.l d0-d7/a0-a6,-(a7)     ;    a1 = ptr to instr data
 lea.l mystructure(PC),a5      ;    a2 =        macros
 move.l a1,myinstrsdata-ms(a5) ;    a3 =        subroutine
 move.l a2,mymacrosdata-ms(a5)
 move.l a3,subroutine-ms(a5)
 movem.l d0/a0,-(a7)
 bsr getvbr
 move.l d0,a0
 move.l $70(a0),oldintvector-ms(a5)
 move.l #_generalplay,$70(a0)
 movem.l (a7)+,a0/d0
 bra internsndreset

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
 moveq.l #INSTNUM-1,d1
setinstlens:
 move.l a5,(a0)+
  move.w (a1)+,d3
  move.w d3,(a4)+
  adda.l d3,a5
   move.w (a1)+,(a4)+
   move.w (a1)+,(a4)+
   move.w (a1)+,(a4)+
   subq.w #1,d1
 dbra d0,setinstlens
 blt.s .l1
.l2: clr.l (a4)+
    clr.l (a4)+
   dbra d1,.l2
.l1:
 move.l a5,d0
 btst #0,d0
 beq.s iseven1
  addq.l #1,d0
iseven1:
 move.l d0,a1
 move.l (a7)+,a5
 move.l d4,insteoredflag-ms(a5)
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
 moveq.l #(INSTNUM+1)-1,d0
 lea.l currlfotable-ms(a5),a0
clrcurrlfo:
   clr.l (a0)+
 dbra d0,clrcurrlfo
 move.l a2,-(a7)
 lea.l 4(a2),a0
 moveq.l #7,d0
searchmacrodata:
  btst d0,channelsenable-ms(a5)
  beq.s channelisdisabled
   bsr searchmelodystrt
channelisdisabled:
 dbra d0,searchmacrodata
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
getnextmacro:
 addq.l #1,a1
 bra.s lookontostart
endofmacros:
 tst.w d0
 beq.s reallyendofmacros
   move.l a2,(a0)+
   subq.w #1,d0
  bra.s endofmacros
reallyendofmacros:
 move.l (a7)+,a2
melodystrtup:
 move.l a6,-(a7)
 moveq.l #0,d1
 move.w (a2)+,d1
 moveq.l #0,d2
 move.w (a2)+,d2
 move.l a2,a0
 lea.l COUNTERS-ms(a5),a2
 lea.l mymacrovecs,a3  ; OPTIONAL
 lea.l modultableend-ms(a5),a4
 lea.l channel0struc-ms(a5),a6
 moveq.l #0,d0
buildmelodies:
 clr.l (nextdata0-channel0struc)(a6)
 clr.l 4+(nextdata0-channel0struc)(a6)
  clr.l (olddata0-channel0struc)(a6)   ; NEW for "("
  clr.w 4+(olddata0-channel0struc)(a6) ;
 move.w -(a4),d3
 lea.l 0(a5,d3.w),a1
 btst d0,channelsenable-ms(a5)
 beq.s channelsdisabled
  movem.l d0/a4,-(a7)
  bsr.s writeandsearchmelodystrt
  movem.l (a7)+,d0/a4
  bra.s channelinit
channelsdisabled:
 movem.l d0-d3/a0/a4,-(a7)
 moveq.l #0,d1
 moveq.l #0,d2
 lea.l mychanneloffmelody-ms(a5),a0
 bsr.s writeonly
 movem.l (a7)+,d0-d3/a0/a4
channelinit:
 suba.l #channel0struc-channel1struc,a6
 addq.w #1,d0
 cmpi.w #7,d0
 bne.s buildmelodies
 move.w -(a4),d3
 lea.l 0(a5,d3.w),a1
 btst d0,channelsenable-ms(a5)
 beq.s lastchannelsdisabled
  bsr.s writeonly
  bra.s getthosevols
lastchannelsdisabled:
 movem.l d0-d3/a0,-(a7)
 moveq.l #0,d1
 moveq.l #0,d2
 lea.l mychanneloffmelody-ms(a5),a0
 bsr.s writeonly
 movem.l (a7)+,d0-d3/a0
getthosevols:
 move.l (a7)+,a6
 lea.l strtvolus-ms(a5),a0
 lea.l origvolumetable-ms(a5),a1
 moveq.l #15,d0
strtupvolume:
   move.w (a0)+,(a1)+
  dbra d0,strtupvolume
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
 addi.l #CHANNELMAX*8,8(a1)
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
 move.l 0(a3,d0.w),(CHANNELMAX*4)(a2)
 move.w #1,(CHANNELMAX*8)(a2)
 clr.l ((CHANNELMAX*8)+(CHANNELMAX*4))(a2)
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

_setalertreaction:  ; d0=ignorealert=-1
 movem.l a5,-(a7)
 lea.l mystructure(PC),a5
 move.b d0,ignorealert-ms(a5)
 movem.l (a7)+,a5
 rts

_setvolume:    ; d0=state(0-127)
 movem.l d1-d2/a0-a2/a5,-(a7)
 andi.w #$007f,d0
  lea.l mystructure(PC),a5
  lea.l origvolumetable-ms+2(a5),a0
  lea.l strtvolus-ms+2(a5),a1
  moveq.l #(16-1)-1,d2
setvolloop:
    move.w (a1)+,d1
    mulu d0,d1
    lsr.w #7,d1
    move.w d1,(a0)+
  dbra d2,setvolloop
 moveq.l #CHANNELMAX-1,d2
setvolloop1:
   move.w CHANNELMAX*2(a0),d1
   add.w d1,d1
   mulu d0,d1
   lsr.w #7,d1
   move.w d1,(a0)+
 dbra d2,setvolloop1
 movem.l (a7)+,d1-d2/a0-a2/a5
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
 clr.l (a1)+  ; HERE: instset eored/asr-ed ??? (-1=YES)
 move.w (a0)+,fibbits-ms(a5)
 move.l a0,fibtable-ms(a5)
 move.w fibbits-ms(a5),d0
 moveq.l #1,d1
 lsl.w d0,d1
 lea.l 0(a0,d1.w),a0
 subq.w #1,d7
packinstrsloop:
   moveq.l #0,d0
   move.w (a3),d0
    beq.s leniszero1
   moveq.l #0,d2
   move.w fibbits-ms(a5),d2
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

_generalplay:
 movem.l d0-d7/a0-a6,-(a7)
 lea.l mystructure(PC),a5
 lea.l $dff000,a6
 lea.l myvoicedata,a1
 moveq.l #0,d7
 move.w #$0780,d1
 bra.s nowait1
waitforallchannels:
   move.b $006(a6),d0
wait111:
   cmp.b $006(a6),d0
   beq.s wait111
wait222:
   btst #4,$007(a6)
   beq.s wait222
nowait1:
  move.w $01e(a6),d0
  btst #7,d0
  beq.s noch0
    lea.l $a0(a6),a0
    move.l hwchannel0-ms(a5),(a0)+
    move.w hwchannel0pervol+4-ms(a5),(a0)+
    move.l hwchannel0-ms(a5),UPS_Voice1Adr(a1)           ; ep
    move.w hwchannel0pervol+4-ms(a5),UPS_Voice1Len(a1)   ; ep
    move.w hwchannel0pervol-ms(a5),(a0)+     ; sligthly changed
    move.w hwchannel0pervol+2-ms(a5),d6      ;
    mulu volvoice1(PC),d6           ;
    lsr.w #6,d6                     ;
    move.w d6,(a0)                  ;
    move.w channel0struc+8(PC),d6   ;
    add.w channel1struc+8(PC),d6    ;  ep
    lsr.w #1,d6                     ;
    move.w d6,8(a1)                 ;
    clr.l 4(a1)                     ;
    move.l d7,d2
    andi.w #$ff7f,d1
    move.w #$0080,$09c(a6)
noch0:
  btst #8,d0
  beq.s noch1
    lea.l $b0(a6),a0
    move.l hwchannel1-ms(a5),(a0)+
    move.w hwchannel1pervol+4-ms(a5),(a0)+
    move.l hwchannel1-ms(a5),UPS_Voice2Adr(a1)
    move.w hwchannel1pervol+4-ms(a5),UPS_Voice2Len(a1)
    move.w hwchannel1pervol-ms(a5),(a0)+
    move.w hwchannel1pervol+2-ms(a5),d6
    mulu volvoice2(PC),d6
    lsr.w #6,d6
    move.w d6,(a0)
    move.w channel2struc+8(PC),d6   ;
    add.w channel3struc+8(PC),d6    ;
    lsr.w #1,d6                     ;
    move.w d6,18+8(a1)                 ;
    clr.l 18+4(a1)                     ;
    move.l d7,d3
    andi.w #$feff,d1
    move.w #$0100,$09c(a6)
noch1:
  btst #9,d0
  beq.s noch2
    lea.l $c0(a6),a0
    move.l hwchannel2-ms(a5),(a0)+
    move.w hwchannel2pervol+4-ms(a5),(a0)+
    move.l hwchannel2-ms(a5),UPS_Voice3Adr(a1)
    move.w hwchannel2pervol+4-ms(a5),UPS_Voice3Len(a1)
    move.w hwchannel2pervol-ms(a5),(a0)+
    move.w hwchannel2pervol+2-ms(a5),d6
    mulu volvoice3(PC),d6
    lsr.w #6,d6
    move.w d6,(a0)
    move.w channel4struc+8(PC),d6   ;
    add.w channel5struc+8(PC),d6    ;
    lsr.w #1,d6                     ;
    move.w d6,36+8(a1)              ;
    clr.l 36+4(a1)                  ;
    move.l d7,d4
    andi.w #$fdff,d1
    move.w #$0200,$09c(a6)
noch2:
  btst #10,d0
  beq.s noch3
    lea.l $d0(a6),a0
    move.l hwchannel3-ms(a5),(a0)+
    move.w hwchannel3pervol+4-ms(a5),(a0)+
    move.l hwchannel3-ms(a5),UPS_Voice4Adr(a1)
    move.w hwchannel3pervol+4-ms(a5),UPS_Voice4Len(a1)
    move.w hwchannel3pervol-ms(a5),(a0)+
    move.w hwchannel3pervol+2-ms(a5),d6
    mulu volvoice4(PC),d6
    lsr.w #6,d6
    move.w d6,(a0)
    move.w channel6struc+8(PC),d6   ;
    add.w channel7struc+8(PC),d6    ;
    lsr.w #1,d6                     ;
    move.w d6,54+8(a1)              ;
    clr.l 54+4(a1)                  ;
    move.l d7,d5
    andi.w #$fbff,d1
    move.w #$0400,$09c(a6)
noch3:
   addq.l #1,d7
  tst.w d1
 bne waitforallchannels

 lea.l hwchannel0-ms(a5),a0
 movem.l d2-d5,-(a7)
 moveq.l #3,d1
calccorr:
  moveq.l #0,d7
  move.l (a7)+,d6
  subq.l #4,d6
  bmi.s calccorrready
   addq.w #1,d7
   subq.l #6,d6
   bmi.s calccorrready
    addq.w #1,d7
     subq.l #5,d6
     bmi.s calccorrready
       addq.w #3,d7
calccorrready:
  sub.w d7,10(a0)
  adda.w #hwchannel1-hwchannel0,a0
 dbra d1,calccorr

 tst.b speedtoupdate-ms(a5)
 beq.s .l1
   move.w newspeed-ms(a5),d0
   move.w d0,speed-ms(a5)
   move.w d0,calcspeed-ms(a5)
   move.w d0,d1
   mulu #5*23,d1
   move.l d1,mixcalcspeed-ms(a5)
   sf speedtoupdate-ms(a5)
.l1:

 tst.b soundenable-ms(a5)
 bne.s noplay
 cmpi.b #$ff,_dtchannelsfinished-ms(a5)   ; DeliTracker special
 bne.s .l55                                ;
      move.l mm_songend,a0                ;
      jsr (a0)                            ;
.l55:                                      ;
 move.b _channelsfinished-ms(a5),d0
 and.b channelsenable-ms(a5),d0
 cmp.b channelsenable-ms(a5),d0
 beq channelsallfinished
 jsr playsnd(PC)
 lea.l mystructure(PC),a5
  tst.b ignorealert-ms(a5)
  bne.s ignoreit
   lea.l $dff000,a6
   move.w $01e(a6),d0
   andi.w #$0780,d0
   bne.s playalert
ignoreit:
cache6:
   bsr.w clearcache2
noplay:
 movem.l (a7)+,d0-d7/a0-a6
 rte
playalert:
 bsr _soundoff
 st playeralert-ms(a5)
 st _fadefinished-ms(a5)
 bra.s noplay
channelsallfinished:
 move.w #$0780,$09a(a6)
 move.w #$0780,$09c(a6)
 move.w #$000f,$096(a6)
 st _fadefinished-ms(a5)
 bra.s noplay

playsnd:          ; in a6=dff000, a5=mystructure
 moveq.l #0,d2
 move.b _fadeflag-ms(a5),d2
 beq.s nooutfade
  not.b fadehalf-ms(a5)
  beq.s nooutfade
   lea.l origvolumetable-ms(a5),a0
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
    moveq.l #CHANNELMAX-1,d0
fadeloop1:
      move.w (a0),d1
      asr.w #1,d1
      mulu d2,d1
      lsr.w #6,d1
      move.w d1,(a0)+
     dbra d0,fadeloop1
nooutfade:
 lea.l channel7struc-ms(a5),a0
 lea.l ((lastvolumes-ms)+((CHANNELMAX-1)*2))(a5),a1
 move.w origvolumetable+(15*2)-ms(a5),d7
 lea.l ((lastperiods-ms)+(24*(CHANNELMAX-1)))(a5),a4
 lea.l modultable-ms(a5),a2
 lea.l SOFTMOD+((CHANNELMAX-1)*4)-ms(a5),a3
 sf channelshandle-ms(a5)
 moveq.l #CHANNELMAX-1,d0
setallchanneldata:
 move.w (a2)+,d1
 movem.l a0-a4/d0/d7,-(a7) ; NICHT AENDERN !!!
 movem.l 0(a5,d1.w),d3/d7/a2-a4
 bsr handle_channel
 movem.l (a7)+,a0-a4/d0/d7
 movem.l a2,-(a7)
  tst.l d4
  bmi.s absnothing
  tst.l d5
  bmi.s pervolonly
   bset.b d0,channelshandle-ms(a5)
   move.w d4,4(a0)
   move.l d5,(a0)
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
skiphull:
pervolonly:
  rol.w #8,d6
  move.b d6,(CHANNELMAX*2)+1(a1)
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
      bra.s nowrealfade
donotsetbackto4096:
    cmpi.w #256,4(a4)
    beq.s nomorefadein
    blt.s setbackto256
nowrealfade:
     move.w fadespeedin-ms(a5),d1
     sub.w d1,4(a4)
     bra.s fadedone
setbackto256:
    move.w #256,4(a4)
nomorefadein:
    bclr d0,dofadein-ms(a5)
    bra.s fadedone
testiffadeout:
  btst d0,dofadeout-ms(a5)
  beq.s fadedone
    cmpi.w #8192,4(a4)
    beq.s nomorefade
    bgt.s setbackto8192
     move.w fadespeedout-ms(a5),d1
     add.w d1,4(a4)
     bra.s fadedone
setbackto8192:
    move.w #8192,4(a4)
nomorefade:
  bclr d0,dofadeout-ms(a5)
fadedone:
  move.w (CHANNELMAX*2)(a1),d6
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
  move.w d6,(CHANNELMAX*2)(a1)
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
  move.w d6,8(a0)       ; - - - VOLUME READY !
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
  movem.l d0/a0/a1,-(a7)       ;
  move.l a0,a1                 ;
  lea.l myvoicedata,a0         ;
  lsr.w #1,d0                  ; EAGLEPLAYER SPECIAL
  mulu #18,D0                  ;
  tst.w 4(A1)                  ;
  beq.s .l1                     ;
    move.w D6,6(A0,D0.W)       ;
.l1:                            ;
  movem.l  (a7)+,d0/a0/a1      ;

  btst d0,_channelsfinished-ms(a5)
  bne.s pause_0
   btst d0,channelshandle-ms(a5)
   beq.s nopause_0
      tst.b 18(a4)
      beq.s nopause_0
      tst.l 16(a0)
      beq.s nopause_0
       st 19(a4)
       move.w 20(a0),20(a4)
      bra.s nopause_0
pause_0:
   clr.l 16(a0)
   clr.w 20(a0)
nopause_0:
 subq.l #2,a1
 subq.l #4,a3
 lea.l -24(a4),a4
 movem.l (a7)+,a2
 lea.l (channel6struc-channel7struc)(a0),a0
 dbra d0,setallchanneldata
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
 jmp MIXER(PC)

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

handle_channel:
 subq.w #1,(a2)
 beq.s countatzero
donothingwithDMA:
   moveq.l #-1,d4
   rts
countatzero:
 move.b internfinished-ms(a5),d1
 and.b d7,d1
 beq.s normalplay
  not.w d7
  and.b d7,dofadein-ms(a5)
  and.b d7,dofadeout-ms(a5)
  not.w d7
  bset.b d0,channelshandle-ms(a5)
  clr.w (a1)
  clr.w (CHANNELMAX*2)(a1)
  move.l 12(a7),a1
  clr.l (a1)
  clr.w 4(a1)
  move.l 24(a7),a1
  clr.l (a1)
  move.l 28(a7),a1
  clr.l (a1)+
  move.w #$0100,(a1)+
  clr.w (a1)+
  clr.l (a1)+
  clr.l (a1)+
  clr.l (a1)+
  clr.l (a1)+
  clr.w d6
  moveq.l #0,d5
  clr.l 0(a5,d3.w)
  clr.w 4(a5,d3.w)
  or.b d7,_dtchannelsfinished-ms(a5)      ; eagleplayer
  tst.b oneshot-ms(a5)
  beq.s normalplay1
   or.b d7,_channelsfinished-ms(a5)
   bra.s donothingwithDMA
normalplay1:
 not.w d7
 and.b d7,internfinished-ms(a5)
 not.w d7
normalplay:
 move.l (CHANNELMAX*4)(a4),a0

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
 cmpi.b #$fc,d0      ; speed set NOW supported !
; beq.s jmptoaddcnt
 bne.s nospeedset
   moveq.l #0,d0
   move.b d1,d0
   lsr.w #4,d0
   andi.w #$000f,d1
   mulu.w origspeed-ms(a5),d0
   divu d1,d0
    cmpi.w #600,d0
    bge.s .l1
      move.w #600,d0
.l1: cmp.w origspeed-ms(a5),d0
    ble.s .l2
      move.w origspeed-ms(a5),d0
.l2: move.w d0,newspeed-ms(a5)
    st speedtoupdate-ms(a5)
    bra.s jmptoaddcnt
nospeedset:
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
; cmpi.b #$f9,d0
; bne.s noWARP
;   ext.w d1
;   btst #7,2(a0)
;   beq.s PERWARP
;    move.w d1,((CHANNELMAX*8)+(CHANNELMAX*4))+2(a4)
;    bra jmptoaddcnt
;PERWARP:
;   move.w d1,((CHANNELMAX*8)+(CHANNELMAX*4))(a4)
;   bra jmptoaddcnt
;noWARP:
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
nocontrolbyte:
 clr.l ((CHANNELMAX*8)+(CHANNELMAX*4))(a4)
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
   tst.w d1
   beq.s novolume
      moveq.l #-1,d4
      move.l 8(a5,d3.w),8+6(a5,d3.w)     ; NEW for "(" !
      move.w 8+4(a5,d3.w),8+6+4(a5,d3.w) ; backup current playptrs
      bra addcounthandle
novolume:
      move.l 8(a5,d3.w),8+6(a5,d3.w)     ; NEW for "(" !
      move.w 8+4(a5,d3.w),8+6+4(a5,d3.w) ; backup current playptrs
novolumemod:
      clr.w d6
      moveq.l #INSTNUM,d4
      ror.l #8,d4
      moveq.l #0,d5
      clr.w d4
      clr.l 0(a5,d3.w)
      clr.w 4(a5,d3.w)
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
    move.w d2,((CHANNELMAX*8)+(CHANNELMAX*4))(a4)
    move.w d1,((CHANNELMAX*8)+(CHANNELMAX*4)+2)(a4)
    addq.l #3,a0
    bra addcounthandle
nonovolume:
 andi.w #$000f,d0
  beq.s novolumemod
 add.w d0,d0
 lea.l strtvolus-ms(a5),a1
 move.w 0(a1,d0.w),d6
 lsl.w #8,d6
 move.b origvolumetable-ms+1(a5,d0.w),d6
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
 cmpi.w #INSTNUM,d2
 beq.s setCONTFROMBREAK
 lsl.w #2,d2
 move.l myinstrvecs-ms(a5,d2.w),d5
 add.w d2,d2

 lea.l myinstrlens-ms(a5),a1
 btst #7,1(a0)
 beq.s setoneshotdata
   move.w 6(a1,d2.w),d4
   beq.s setoneshotdata
   move.l d5,0(a5,d3.w)
   moveq.l #0,d0
   move.w 4(a1,d2.w),d0
   add.l d0,0(a5,d3.w)
   add.w d4,d4
   move.w d4,4(a5,d3.w)
   move.w 2(a1,d2.w),d4
   bra.s addcounthandle
setCONTFROMBREAK:                 ;
    clr.l 0(a5,d3.w)              ;
    clr.w 4(a5,d3.w)              ;   NEW for "("
    move.l 8+6(a5,d3.w),d5        ;
    move.w 8+6+4(a5,d3.w),d4      ;
    bra.s addcounthandle          ;
setoneshotdata:
     clr.l 0(a5,d3.w)
     clr.w 4(a5,d3.w)
     move.w 0(a1,d2.w),d4
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
 move.l a0,(CHANNELMAX*4)(a4)
 rts

getnewmacro:
 moveq.l #1,d1
 addq.l #2,(a4)
 move.l (a4),a1
 tst.w (a1)
 bne.s nopausemacro
  move.w pattlen-ms(a5),d1
  add.w d1,d1
  add.w d1,(a2)
  bra.s getnewmacro
nopausemacro: 
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

; definitely no eagleplayer adaptions from here

MIXER:
 lea.l channel7struc(PC),a3
 lea.l hwchannel3(PC),a4
 moveq.l #7,d7
mixmainloop:   ; ----------------
 movem.w 6(a3),d0/d2   ; 6:period A, 8:volume A
 movem.w (channel6struc-channel7struc)+6(a3),d1/d3  ; " " "

 move.w d7,d5
 lsr.w #1,d5
 btst d5,mixertype
 sne mixer

 moveq.l #0,d5
 moveq.l #0,d6
 move.b channelshandle(PC),d4
 btst d7,d4
 beq.s donthandlec1
   move.l (a3),12(a4)  ; instptr A
   move.w 4(a3),d5     ; length A
   clr.w 4(a3)         ;
   move.w d5,20(a4)    ; 
   clr.w 82(a4)
   bra.s handlec1
donthandlec1:
 move.w 20(a4),d5
 cmpi.w #3,d5
 bhi.s handlec1
   moveq.l #0,d5
   clr.w 20(a4)
   move.l 16(a3),12(a4)
   beq.s handlec1
    move.w 20(a3),d5
    move.w d5,20(a4)
handlec1:

 subq.w #1,d7
 btst d7,d4
 beq.s donthandlec2
   move.l (channel6struc-channel7struc)(a3),22(a4) ; instptr B
   move.w (channel6struc-channel7struc)+4(a3),d6   ; length B
   clr.w (channel6struc-channel7struc)+4(a3)       ;
   move.w d6,30(a4)     ;
   clr.w 82(a4)
   bra.s handlec2
donthandlec2:
 move.w 30(a4),d6
 cmpi.w #3,d6
 bhi.s handlec2
   moveq.l #0,d6
   clr.w 30(a4)
   move.l (channel6struc-channel7struc)+16(a3),22(a4)
   beq.s handlec2
    move.w (channel6struc-channel7struc)+20(a3),d6
    move.w d6,30(a4)
handlec2:
 subq.w #1,d7
 move.l d7,-(a7)

 moveq.l #-1,d4
 clr.w d4
 tst.w d5
 bne.s onechannelused1
  move.l d4,16(a4)
  moveq.l #-1,d0
  moveq.l #0,d2
  tst.w d6
  bne.s onechannelused
   move.l d4,26(a4)
   moveq.l #-1,d1
   moveq.l #0,d3
nochannelused:
   move.w #5*23*2,32(a4)
   clr.w 34(a4)
   move.w speed(PC),d4
   lsr.w #2,d4
   add.w 10(a4),d4
   clr.w 10(a4)
   move.w d4,36(a4)
   movem.w d0/d2,16(a4)   ; per A, per B
   movem.w d1/d3,26(a4)   ; vol A, vol B
   move.l #$10000,42(a4)
   clr.w 8(a4)
   bra absmixloopsend

onechannelused1:
 tst.w d6
 bne.s onechannelused
  move.l d4,26(a4)
  moveq.l #-1,d1
  moveq.l #0,d3
onechannelused:

 move.l (a4),a6         ; a6=mixbuffer
 movem.w d0/d2,16(a4)   ; per A,per B
 movem.w d1/d3,26(a4)   ; vol A,vol B
 sf 22(a3)  ; first call to mixagain
mixagain:
 movem.l d0-d4/a3/a6,-(a7)
 move.b mixer(PC),d4
 beq.s old
new:
 bsr returndbramixer
 move.l a2,78(a4)
 bra.s qmixergot
old:
 movea.w hi(PC),a2
 movea.l hightable(PC),a3
 bsr returnmixer
qmixergot:
 swap d7
 move.w d0,d7
 swap d7
 movem.l (a7)+,d0-d4/a3/a6

 move.l 46(a4),d5 ; neu, for FAST vol-mix
 clr.b d5         ;
 move.l d5,a5     ;
 move.w d0,d5
 move.w d1,d6
 movem.l d0-d6/a0/a3-a6,-(a7)
 movem.l d7,-(a7)
 move.w d3,d4
 move.w d2,d3
 move.w d1,d2
 move.w d0,d1
 swap d7
 move.w d7,d0
 movea.w hi(PC),a3
 bsr tabledepack
 move.l a5,a2
 movem.l (a7)+,d7
 swap d7
 move.w a6,d7
 swap d7
 movem.l (a7)+,d0-d6/a0/a3-a6
 move.l a2,38(a4)

 tst.w d7
 beq.s noexchangeaddqs
   move.w -10(a0),a2
   move.w -8(a0),-10(a0)
   move.w a2,-8(a0)
noexchangeaddqs:

 cmp.w d2,d3               ; sort vol's
 bls.s novolumeexchange
  exg.l d2,d3
novolumeexchange: 
 move.w d2,34(a4)          ; write vol

 move.b mixer(PC),d2
 bne.s nohi2
 movea.l hightable(PC),a2    ; hightable->a2
 cmp.w hi(PC),d0
 bhi.s nohi1
  add.w d0,d0
  move.w 0(a2,d0.w),d0
nohi1:
 cmp.w hi(PC),d1
 bhi.s nohi2
  add.w d1,d1
  move.w 0(a2,d1.w),d1
nohi2:

 tst.b 22(a3)
 bne itssecondcall  

 move.l mixcalcspeed(PC),d2
 cmp.w d1,d0
 bls itsd0todo
   tst.w 8(a4)
   beq.s nod1recalc
   cmp.w 8(a4),d6
   beq.s nod1recalc
    move.l 42(a4),d4
    divu.w d6,d4
    mulu.w 8(a4),d4
    move.l d4,42(a4)
nod1recalc:
   move.w d1,32(a4)
   divu d1,d2
   move.w d6,8(a4)
   move.w 10(a4),d4
   beq.s nod1corr
    clr.w 10(a4)
    add.w d4,d2
nod1corr:
   move.l d2,d4
   clr.w d4
   move.l d4,d3
   divu.w d1,d3
   moveq.l #0,d4
   move.w d3,d4
   btst #0,d2
   beq.s itsad1todoeven
    addq.w #1,d2
    not.w d4
    sub.l d4,42(a4)
    bra.s itsad1todoodd
itsad1todoeven:
   add.l d4,42(a4)
itsad1todoodd:
   move.l 42(a4),d4
   bpl.s itsad1todocont
    addq.w #2,42(a4)
    subq.w #2,10(a4)
    bra.s itsad1todook
itsad1todocont:
   cmpi.l #$1ffff,d4
   bls.s itsad1todook
    subq.w #2,42(a4)
    addq.w #2,10(a4)
itsad1todook:
 move.w d2,d4
 move.w d2,d3
 tst.b mixer
 bne.s itsd1tododbra
  cmp.w hi(PC),d6
  bhi.s nohi4
   mulu -8(a0),d2
   lsr.l #6+2,d2        ; !!!!!!
   addq.w #1,d2
nohi4:
  mulu -10(a0),d3
  lsr.l #6+2,d3         ; !!!!!!
  addq.w #1,d3
  exg.l d2,d3
  bra bytescalced
itsd1tododbra:
 tst.w d0
 bmi bytescalced
 move.l d1,-(a7)
 swap d1
 clr.w d1
 divu d0,d1
 mulu d1,d3
 swap d3
 swap d1
 addq.w #1,d3
 exg.l d2,d3
 move.l (a7)+,d1
 bra bytescalced
itsd0todo:     ; -----
   tst.w 8(a4)
   beq.s nod0recalc
   cmp.w 8(a4),d5
   beq.s nod0recalc
    move.l 42(a4),d4
    divu.w d5,d4
    mulu.w 8(a4),d4
    move.l d4,42(a4)
nod0recalc:
  move.w d0,32(a4)
  divu d0,d2
  move.w d5,8(a4)
  move.w 10(a4),d4
  beq.s nod0corr
   clr.w 10(a4)
   add.w d4,d2
nod0corr:
   move.l d2,d4
   clr.w d4
   move.l d4,d3
   divu.w d0,d3
   moveq.l #0,d4
   move.w d3,d4
   btst #0,d2
   beq.s itsad0todoeven
    addq.w #1,d2
    not.w d4
    sub.l d4,42(a4)
    bra.s itsad0todoodd
itsad0todoeven:
   add.l d4,42(a4)
itsad0todoodd:
   move.l 42(a4),d4
   bpl.s itsad0todocont
    addq.w #2,42(a4)
    subq.w #2,10(a4)
    bra.s itsad0todook
itsad0todocont:
   cmpi.l #$1ffff,d4
   bls.s itsad0todook
    subq.w #2,42(a4)
    addq.w #2,10(a4)
itsad0todook:
 move.w d2,d4
 move.w d2,d3
 tst.b mixer
 bne.s itsd0tododbra
  cmp.w hi(PC),d5
  bhi.s nohi33
   mulu -10(a0),d2
   lsr.l #6+2,d2     ; !!!!!!
   addq.w #1,d2
nohi33:
  cmp.w d5,d6
  beq.s periodsareequal
   mulu -8(a0),d3
   lsr.l #6+2,d3     ; !!!!!!
   addq.w #1,d3
   bra.s bytescalced
itsd0tododbra:
 cmp.w d5,d6
 beq.s periodsareequalnohi
 tst.w d1
 bmi.s bytescalced
 move.l d0,-(a7)
 swap d0
 clr.w d0
 divu d1,d0
 mulu d0,d3
 swap d3
 swap d0
 addq.w #1,d3
 move.l (a7)+,d0
 bra.s bytescalced
periodsareequal:
  cmp.w hi(PC),d5
  bhi.s nohi3333
periodsareequalnohi:
   move.w d2,d3
nohi3333:
bytescalced:   ; d2=bytes needed of INST A
               ; d3=bytes needed of INST B ; !!
               ; d4=length(BYTES!!!) to be written into DMA-Regs
 move.w d4,36(a4)
 lsr.w 36(a4)

 move.l a0,a2
 move.l 12(a4),a0 ; get instdataptr (REST)
 move.l 22(a4),a1
 bra.s wasfirstcall

itssecondcall:
 move.l a0,a2
 movem.l mya0buffer-base(a4),a0-a1 ; 8+4*2
 movem.w myd2buffer-base(a4),d2-d4 ; 4+4*3

wasfirstcall:
 exg.l d0,a5            ; neu for FAST-vol-mix
 move.l 38(a4),d0 ; volumebuffer
 clr.b d0
 exg.l d0,a5

 movem.w d0-d1,myd0buffer-base(a4)

startofenoughstuff:
 move.w 20(a4),d5
 beq enoughofA
 cmp.w d5,d2
 bls enoughofA
NOTenoughofA:
   move.w 30(a4),d5
   beq neoAenoughofB
   cmp.w d5,d3
   bls neoAenoughofB
NOTenoughofAandB:  ; A and B need more
 cmp.w 32(a4),d0
  beq.s neofABperA
neofABperB:          ; per B
 movem.l d2-d3,-(a7)
 movem.l d7/d4/a4,-(a7)
 moveq.l #0,d4
 move.w 20(a4),d4
 lsl.l #6+2,d4       ; !!!!!!
 divu -10(a2),d4
 moveq.l #0,d0
 move.w 30(a4),d0
 move.w -8(a2),d1
 cmpi.w #64*4,d1
 beq.s norecalcB
  lsl.l #6+2,d0      ; !!!!!!
  divu d1,d0
norecalcB:
 cmp.w d0,d4
 bls ohlord_badAA
 move.w d0,d4
 bra yeahsimpleBB

neofABperA:          ; per A
 movem.l d2-d3,-(a7)
 movem.l d7/d4/a4,-(a7)
 moveq.l #0,d4
 move.w 30(a4),d4
 lsl.l #6+2,d4       ; !!!!!!
 divu -8(a2),d4
 moveq.l #0,d0
 move.w 20(a4),d0
 move.w -10(a2),d1
 cmpi.w #64*4,d1
 beq.s norecalc
   lsl.l #6+2,d0    ; !!!!!!!
   divu d1,d0
norecalc:
 cmp.w d0,d4
 bls ohlord_badBB
 move.w d0,d4
 bra yeahsimpleAA

neoAenoughofB:      ; B ok, A needs more
 cmp.w 32(a4),d0    ; per period of A played ?
  beq yeahsimpleA
ohlord_badA:        ; B ok, A needs more, per B played
 movem.l d2-d3,-(a7)
 movem.l d7/d4/a4,-(a7)
 moveq.l #0,d4
 move.w 20(a4),d4
 lsl.l #6+2,d4       ; !!!!!!!!
 divu -10(a2),d4
ohlord_badAA:
 tst.w d7
 beq.s noexchange22
  move.l -10(a2),d1
  swap d1
  move.l d1,-10(a2)
  exg.l a1,a0
noexchange22:
 move.b mixer(PC),d1
 bne.s ohlord_badAAdbra
 move.w d4,d1
 lsr.w #6,d4
  move.w d4,d0
 lsl.w #6,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #6,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
  addq.w #1,d0
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu: for FAST vol-mix
 move.l a6,a4
 moveq.l #0,d4     ; PCS
 move.w -18(a2),d6 ;
cache1:
 bsr.w clearcache1
 jsr 0(a2,d1.w)
 bra.s eof_ohlord_badAA
ohlord_badAAdbra:
 move.w d4,d1
 lsr.w #3,d4
 move.w d4,d0
 lsl.w #3,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #3,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu: for FAST vol-mix
 move.w 82(a4),d4
 move.w -18(a2),d6
 move.l 78(a4),a5
 jsr (a5)
 move.l a6,-(a7)
 jsr 0(a2,d1.w)
 move.w d4,82(a4)
 move.l 78(a4),a5
 jsr -4(a5)
 move.l (a7)+,a4
eof_ohlord_badAA:
 clr.b d5          ; neu for FAST vol-mix
 move.l d5,a5      ;
 sub.l a0,d2
 sub.l a1,d3
 sub.l a6,a4
 move.l a4,d5
 movem.l (a7)+,d7/d4/a4
 add.w d5,d4
 tst.w d7
 beq.s noexchange2aa
  exg.l a1,a0
  exg.l d2,d3
noexchange2aa:
 add.w d3,30(a4)
 movem.l (a7)+,d0-d1
 add.w d2,d0
 add.w d3,d1
 bra temporaryA

yeahsimpleA:       ; B ok, A needs more (per A played)
 movem.l d2-d3,-(a7)
 movem.l d7/d4/a4,-(a7)
 moveq.l #0,d4
 move.w 20(a4),d4
 move.w -10(a2),d1
 cmpi.w #64*4,d1
 beq.s yeahsimpleAA
  lsl.l #6+2,d4      ; !!!!!!!!
  divu.w d1,d4
yeahsimpleAA:
 tst.w d7
 beq.s noexchange2
  move.l -10(a2),d1
  swap d1
  move.l d1,-10(a2)
  exg.l a1,a0
noexchange2:
 move.b mixer(PC),d1
 bne.s yeahsimpleAAdbra
 move.w d4,d1
 lsr.w #6,d4
  move.w d4,d0
 lsl.w #6,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #6,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
  addq.w #1,d0
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu for FAST vol-mix
 move.l a6,a4
 moveq.l #0,d4     ; PCS
 move.w -18(a2),d6 ;
cache2:
 bsr.w clearcache1
 jsr 0(a2,d1.w)
 bra.s eof_yeahsimpleAA
yeahsimpleAAdbra:
 move.w d4,d1
 lsr.w #3,d4
 move.w d4,d0
 lsl.w #3,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #3,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu for FAST vol-mix
 move.w 82(a4),d4
 move.w -18(a2),d6 ; PCS
 move.l 78(a4),a5
 jsr (a5)
 move.l a6,-(a7)
 jsr 0(a2,d1.w)
 move.w d4,82(a4)
 move.l 78(a4),a5
 jsr -4(a5)
 move.l (a7)+,a4
eof_yeahsimpleAA:
 clr.b d5          ; neu for FAST vol-mix
 move.l d5,a5      ;
 sub.l a0,d2
 sub.l a1,d3
 sub.l a6,a4
 move.l a4,d5
 movem.l (a7)+,d7/d4/a4
 add.w d5,d4
 tst.w d7
 beq.s noexchange2a
  exg.l a1,a0
  exg.l d2,d3
noexchange2a:
 add.w d3,30(a4)
 movem.l (a7)+,d0-d1
 add.w d2,d0
 add.w d3,d1

temporaryA:
 movem.w d0-d1/d4,myd2buffer-base(a4)
 moveq.l #0,d5
  move.l 16(a3),a0
  move.l a0,12(a4)
  beq.s Aisoneshot
   move.w 20(a3),d5
   move.w d5,20(a4)
   beq.s Aisoneshot
Aiscont:
    move.w 30(a4),d6
    cmpi.w #3,d6
    bhi.s Bhasstillmore
      moveq.l #0,d6
      move.l (channel6struc-channel7struc)+16(a3),a1
      move.l a1,22(a4)
      beq Bisoneshot1
      move.w (channel6struc-channel7struc)+20(a3),d6
      move.w d6,30(a4)
Bhasstillmore:
   tst.w d7
   beq.s noexchangeaddqs1
     move.l -10(a2),d1
     swap d1
     move.l d1,-10(a2)
noexchangeaddqs1:
    movem.w myd0buffer-base(a4),d0-d4
    bra startofenoughstuff
Aisoneshot:
 clr.w 20(a4)
 move.w 30(a4),d6
 cmpi.w #3,d6
 bhi.s Bhasstillmore2
   moveq.l #0,d6
   move.l (channel6struc-channel7struc)+16(a3),a1
   move.l a1,22(a4)
   beq.s Bhasstillmore3
    move.w (channel6struc-channel7struc)+20(a3),d6
Bhasstillmore3:
    move.w d6,30(a4)
Bhasstillmore2:
 movem.w 16(a4),d0/d2    ; per A, per B
 movem.w 26(a4),d1/d3    ; vol A, vol B
 st 22(a3)              ; second call to mixagain !!!
 movem.l a0-a1,mya0buffer-base(a4)
 bra mixagain

enoughofA:
 move.w 30(a4),d5
 beq enoughofAandB
 cmp.w d5,d3
 bls enoughofAandB
NOTenoughofB:      ; A ok, B needs more
 cmp.w 32(a4),d1
  beq yeahsimpleB
ohlord_badB:       ; A ok, B needs more, per A played
 movem.l d2-d3,-(a7)
 movem.l d4/d7/a4,-(a7)
 moveq.l #0,d4
 move.w 30(a4),d4
 lsl.l #6+2,d4    ; !!!!!!!!
 divu -8(a2),d4
ohlord_badBB:
 tst.w d7
 beq.s noexchange44
  move.l -10(a2),d1
  swap d1
  move.l d1,-10(a2)
  exg.l a1,a0
noexchange44:
 move.b mixer(PC),d1
 bne.s ohlord_badBBdbra
 move.w d4,d1
 lsr.w #6,d4
  move.w d4,d0
 lsl.w #6,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #6,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
  addq.w #1,d0
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu for FAST vol-mix
 move.l a6,a4
 moveq.l #0,d4     ; PCS
 move.w -18(a2),d6 ;
cache3:
 bsr.w clearcache1
 jsr 0(a2,d1.w)
 bra.s eof_ohlord_badBB
ohlord_badBBdbra:
 move.w d4,d1
 lsr.w #3,d4
 move.w d4,d0
 lsl.w #3,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #3,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu for FAST vol-mix
 move.w 82(a4),d4
 move.w -18(a2),d6 ;
 move.l 78(a4),a5
 jsr (a5)
 move.l a6,-(a7)
 jsr 0(a2,d1.w)
 move.w d4,82(a4)
 move.l 78(a4),a5
 jsr -4(a5)
 move.l (a7)+,a4
eof_ohlord_badBB:
 clr.b d5          ; neu for FAST vol-mix
 move.l d5,a5      ;
 sub.l a0,d2
 sub.l a1,d3
 sub.l a6,a4
 move.l a4,d5
 movem.l (a7)+,d4/d7/a4
 add.w d5,d4
 tst.w d7
 beq.s noexchange4aa
  exg.l a1,a0
  exg.l d2,d3
noexchange4aa:
 add.w d2,20(a4)
 movem.l (a7)+,d0-d1
 add.w d2,d0
 add.w d3,d1
 bra temporaryB

yeahsimpleB:        ; A ok, B needs more, per B is played
 movem.l d2-d3,-(a7)
 movem.l d4/d7/a4,-(a7)
 moveq.l #0,d4
 move.w 30(a4),d4
 move.w -8(a2),d1
 cmpi.w #64*4,d1
 beq.s yeahsimpleBB
   lsl.l #6+2,d4     ; !!!!!!
   divu.w d1,d4
yeahsimpleBB:
 tst.w d7
 beq.s noexchange4
  move.l -10(a2),d1
  swap d1
  move.l d1,-10(a2)
  exg.l a1,a0
noexchange4:
 move.b mixer(PC),d1
 bne.s yeahsimpleBBdbra
 move.w d4,d1
 lsr.w #6,d4
  move.w d4,d0
 lsl.w #6,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #6,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
  addq.w #1,d0
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu for FAST vol-mix
 move.l a6,a4
 moveq.l #0,d4     ; PCS
 move.w -18(a2),d6 ;
cache4:
 bsr.w clearcache1
 jsr 0(a2,d1.w)
 bra.s eof_yeahsimpleBB
yeahsimpleBBdbra:
 move.w d4,d1
 lsr.w #3,d4
 move.w d4,d0
 lsl.w #3,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #3,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu for FAST vol-mix
 move.w 82(a4),d4
 move.w -18(a2),d6 ;
 move.l 78(a4),a5
 jsr (a5)
 move.l a6,-(a7)
 jsr 0(a2,d1.w)
 move.w d4,82(a4)
 move.l 78(a4),a5
 jsr -4(a5)
 move.l (a7)+,a4
eof_yeahsimpleBB:
 clr.b d5          ; neu for FAST vol-mix
 move.l d5,a5      ;
 sub.l a0,d2
 sub.l a1,d3
 sub.l a6,a4
 move.l a4,d5
 movem.l (a7)+,d4/d7/a4
 add.w d5,d4
 tst.w d7
 beq.s noexchange4a
  exg.l a1,a0
  exg.l d2,d3
noexchange4a:
 add.w d2,20(a4)
 movem.l (a7)+,d0-d1
 add.w d2,d0
 add.w d3,d1

temporaryB:
 movem.w d0-d1/d4,myd2buffer-base(a4)
 moveq.l #0,d6
  move.l (channel6struc-channel7struc)+16(a3),a1
  move.l a1,22(a4)
  beq.s Bisoneshot1
   move.w (channel6struc-channel7struc)+20(a3),d6
   move.w d6,30(a4)
   beq.s Bisoneshot1
Biscont:
    move.w 20(a4),d5
    cmpi.w #3,d5
    bhi.s Ahasstillmore
      moveq.l #0,d5
      move.l 16(a3),a0
      move.l a0,12(a4)
      beq Aisoneshot
      move.w 20(a3),d5
      move.w d5,20(a4)
Ahasstillmore:
    tst.w d7
    beq.s noexchangeaddqs2
     move.l -10(a2),d1
     swap d1
     move.l d1,-10(a2)
noexchangeaddqs2:
    movem.w myd0buffer-base(a4),d0-d4
    bra startofenoughstuff
Bisoneshot1:
 clr.w 30(a4)
 move.w 20(a4),d5
 cmpi.w #3,d5
 bhi.s Ahasstillmore1
   moveq.l #0,d5
   move.l 16(a3),a0
   move.l a0,12(a4)
   beq.s Ahasstillmore2
    move.w 20(a3),d5
Ahasstillmore2:
    move.w d5,20(a4)
Ahasstillmore1:
 movem.w 16(a4),d0/d2   ; per A, per B
 movem.w 26(a4),d1/d3   ; vol A, vol B
 st 22(a3)              ; second call to mixagain !!!
 movem.l a0-a1,mya0buffer-base(a4)
 bra mixagain

enoughofAandB:     ; A and B ok.
 move.l d7,-(a7)
 tst.w d7
 beq.s noexchange
  move.l -10(a2),d1
  swap d1
  move.l d1,-10(a2)
  exg.l a1,a0
noexchange:
 move.b mixer(PC),d1
 bne.s enoughofAandBdbra
 move.w d4,d1
 lsr.w #6,d4
  move.w d4,d0
 lsl.w #6,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #6,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
 addq.w #1,d0
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu for FAST vol-mix
 moveq.l #0,d4     ; PCS
 move.w -18(a2),d6 ;
cache5:
 bsr.w clearcache1
 jsr 0(a2,d1.w)
 bra.s eof_enoughofAandB
enoughofAandBdbra:
 move.w d4,d1
 lsr.w #3,d4
 move.w d4,d0
 lsl.w #3,d4
 sub.w d4,d1
 swap d7
 moveq.l #0,d6
 move.w d7,d6
 lsl.w #3,d6
 mulu d1,d7
 sub.l d7,d6
 move.w d6,d1
 move.l a0,d2
 move.l a1,d3
 moveq.l #0,d7
 move.l a5,d5      ; neu for FAST vol-mix
 move.w 82(a4),d4
 move.w -18(a2),d6 ;
 move.l 78(a4),a5
 jsr (a5)
 jsr 0(a2,d1.w)
 move.w d4,82(a4)
 move.l 78(a4),a5
 jsr -4(a5)
eof_enoughofAandB:
 clr.b d5          ; neu for FAST vol-mix
 move.l d5,a5      ;
 sub.l a0,d2
 sub.l a1,d3
 move.l (a7)+,d7
 tst.w d7
 beq.s noexchange56
  exg.l d2,d3
  exg.l a0,a1
noexchange56:
 add.w d2,20(a4)
 add.w d3,30(a4)

mixloopsend:
 move.l a0,12(a4)
 move.l a1,22(a4)
absmixloopsend:
 move.l (a4),d0      ; exchange buffer pointers
 move.l 4(a4),(a4)
 move.l d0,4(a4)
 move.l (a7)+,d7
absend:
  move.l 12(a4),olddata1-channel1struc(a3)        ;
  move.w 20(a4),olddata1+4-channel1struc(a3)      ; NEW for "("
  move.l 22(a4),olddata0-channel1struc(a3)        ;
  move.w 30(a4),olddata0+4-channel1struc(a3)      ;
 adda.w #channel5struc-channel7struc,a3
 suba.w #hwchannel3-hwchannel2,a4
 cmpa.l #hwchannel0,a4
 bge mixmainloop
 lea.l $dff000,a6
 move.w hwchannel0pervol+4(PC),$a4(a6)
 move.w hwchannel1pervol+4(PC),$b4(a6)
 move.w hwchannel2pervol+4(PC),$c4(a6)
 move.w hwchannel3pervol+4(PC),$d4(a6)
 rts

tabledepack:  ; a0=mixer,d0=routinelength,a1=pointer(addq,end),a5=vb
 move.l a5,-(a7)
 swap d7
depackperiod: ; d1,d2=per d3,d4=vol  ; RESULT: a6=realroutinelen
 tst.w d0     ; a3=hi-limit-1
 beq depackvolumePLUS
 tst.b mixer
 bne make_dbra_mixer
searchforit:
 move.l a1,a6
 suba.l a0,a6
 addq.l #2,a6
 move.w d1,d6
 swap.w d6
 move.w d2,d6
 move.w d2,d5
 swap.w d5
 move.w d1,d5
  cmp.l -4(a0),d6       ; look into myself !
  beq immyself          ;  PREPARED FOR MYSELF ?
  cmp.l -4(a0),d5       ;
  beq wrongregs         ;  wrong reg-order !
  tst.w -14(a0)         ; Am I a 2-addq ?
  beq calcperiodtable   ;  YES ? So calculate it !
 lea.l mixlooptable(PC),a2
 move.l (a2)+,a4
 cmp.l -(a4),d6
 beq.s copyperiodtable
 cmp.l (a4),d5
 beq.s copyperiodtable
 move.l (a2)+,a4
 cmp.l -(a4),d6
 beq.s copyperiodtable
 cmp.l (a4),d5
 beq.s copyperiodtable
 move.l (a2)+,a4
 cmp.l -(a4),d6
 beq.s copyperiodtable
 cmp.l (a4),d5
 beq.s copyperiodtable
 move.l (a2)+,a4
 cmp.l -(a4),d6
 beq.s copyperiodtable
 cmp.l (a4),d5
 beq.s copyperiodtable
 move.l (a2)+,a4
 cmp.l -(a4),d6
 beq.s copyperiodtable
 cmp.l (a4),d5
 beq.s copyperiodtable
   tst.w -14(a0)
   bpl calcperiodtable
 move.l (a2)+,a4  ; ----- 2 addqs:
 cmp.l -(a4),d6
 beq.s copyperiodtable
 cmp.l (a4),d5
 beq.s copyperiodtable
 move.l (a2)+,a4
 cmp.l -(a4),d6
 beq.s copyperiodtable
 cmp.l (a4),d5
  bne calcperiodtable
copyperiodtable:
 move.l d6,-(a0)
 move.w d7,-12(a0)
 move.w -14(a4),-14(a0)       ; neu: for PCS V1.2
 tst.w -10(a4)
 bne.s he_is_no_2addq
  tst.w -10(a0)
  bmi.s copyfrom2addq2zero  ; NEW !
gotocalcperiodtable:
     addq.l #4,a0
     bra calcperiodtable
copyfrom2addq2zero:
 cmp.l (a4),d6
 bne.s gotocalcperiodtable
sameorder:
 movem.l d3-d4,-(a7)
 move.w -6(a4),-6(a0)
 move.w -2(a4),d5
 cmp.w -12(a4),d7
 beq.s leaveitlikethis
   move.w -4(a4),-6(a0)
leaveitlikethis:
 clr.w -4(a0)
 move.b #$88,d3
 tst.w d0
 bpl.s copyafterlabel
copybeforelabel:
 neg.w d0
 subq.w #4,a4
  bra copyperiodtablenew
copyafterlabel:
 subq.w #2,a4
 bra copyperiodtablenew

he_is_no_2addq:  ; ------
 tst.w -10(a0)
 bmi.s copy2zero
 tst.w -10(a4)
 bpl.s fromnorm2norm
copyfromzero:
   move.w -6(a4),-6(a0)
   move.w #64*4,-4(a0)
   bra.s fromzero2norm
copy2zero:
 tst.w -10(a4)
 bpl.s fromnorm2zero
copyfromzero2zero:
 cmp.w -12(a4),d7
 bne.s testd1d2
testd2d1:
  cmp.l (a4),d6
  bne.s gotocalcperiodtable
  bra.s copylongword
testd1d2:
  cmp.l (a4),d5
  bne.s gotocalcperiodtable
fromnorm2zero:
fromnorm2norm:
copylongword:
 move.l -6(a4),-6(a0)
fromzero2norm:
startcopytable:
 move.w -(a4),d5
 move.w d5,d7
 tst.w -10(a0)
 bpl.s copyperiodtableokneu
   tst.w d0
   bpl.s copyperiodtableokneu
    neg.w d0
copyperiodtableokneu:
 eor.w d0,d7
 bmi.s copyperiodtablechange
copyperiodtableok:
 tst.w -10(a0)
 bpl.s nozero12345
    clr.w -4(a0)
nozero12345:
 tst.w d0
 bpl.s copyperiodtableoknoneg
  neg.w d5
  neg.w d0
copyperiodtableoknoneg:
 addq.l #4+2,a4
 moveq.l #(64/8)-1,d6
copyperiodtableokloop:
  adda.w d5,a4
  move.w (a4)+,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,(a1)+
  adda.w d0,a1
 dbra d6,copyperiodtableokloop
 suba.w d0,a1
 move.w (a4),(a1)
 moveq.l #14,d6
 adda.w d6,a4
 adda.w d6,a1
 move.w (a4),(a1)
 bra depackvolume ; --------
copyperiodtablechange:
 movem.l d3-d4,-(a7)
 move.w -6(a0),d3
 move.w -4(a0),-6(a0)
 move.w d3,-4(a0)
 tst.w -10(a0)
 bpl.s nozero123456
    clr.w -4(a0)
nozero123456:
 move.b #$88,d3
 tst.w d0
 bpl.s copyperiodtablechangeneg
  addq.b #1,d3
  neg.w d0
  neg.w d5
copyperiodtablechangeneg:
 neg.w d5
copyperiodtablenew:
 addq.l #4+2,a4
 move.w #$4e71,d4
 moveq.l #(64/8)-1,d6
copyperiodtablechangeloop:
  adda.w d5,a4
  move.w (a4)+,d7
  cmp.w d4,d7
  beq.s copyperioditsanop1
   move.b d3,d7
copyperioditsanop1:
  move.w d7,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,d7
  cmp.w d4,d7
  beq.s copyperioditsanop2
   move.b d3,d7
copyperioditsanop2:
  move.w d7,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,d7
  cmp.w d4,d7
  beq.s copyperioditsanop3
   move.b d3,d7
copyperioditsanop3:
  move.w d7,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,d7
  cmp.w d4,d7
  beq.s copyperioditsanop4
   move.b d3,d7
copyperioditsanop4:
  move.w d7,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,d7
  cmp.w d4,d7
  beq.s copyperioditsanop5
   move.b d3,d7
copyperioditsanop5:
  move.w d7,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,d7
  cmp.w d4,d7
  beq.s copyperioditsanop6
   move.b d3,d7
copyperioditsanop6:
  move.w d7,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,d7
  cmp.w d4,d7
  beq.s copyperioditsanop7
   move.b d3,d7
copyperioditsanop7:
  move.w d7,(a1)+
  adda.w d0,a1
  adda.w d5,a4
  move.w (a4)+,d7
  cmp.w d4,d7
  beq.s copyperioditsanop8
   move.b d3,d7
copyperioditsanop8:
  move.w d7,(a1)+
  adda.w d0,a1
 dbra d6,copyperiodtablechangeloop
 suba.w d0,a1
 moveq.l #14,d6
 adda.w d6,a4
 adda.w d6,a1
 move.w (a4),d7
 cmp.w d4,d7
 beq.s copyperioditsanop9
   move.b d3,d7
copyperioditsanop9:
 move.w d7,(a1)
 movem.l (a7)+,d3-d4
 bra depackvolume ; ---------

immyself:
 tst.w -14(a0)
  bgt depackvolume
 bmi.s gettestforzero
 cmp.w -16(a0),d7
 bne.s exchangeaddqs
 bra depackvolume
gettestforzero:
 cmp.w -16(a0),d7
 beq depackvolume
 bra calcperiodtable

wrongregs:
 tst.w -14(a0)
  bne calcperiodtable  ; "bne"=RICHTIG !!!
 cmp.w -16(a0),d7
 beq.s exchangeaddqs
   move.l d6,-(a0)
   move.w d7,-12(a0)
   bra depackvolume
exchangeaddqs:
 move.l d6,-(a0)
 movem.l d3-d4,-(a7)
 move.w d7,-12(a0)
 move.w -6(a0),d3
 move.w -4(a0),-6(a0)
 move.w d3,-4(a0)
 tst.w d0
 bpl.s exchangeaddqsnoneg
  neg.w d0
exchangeaddqsnoneg:
 subq.w #2,d0
 subq.w #2,a1
 moveq.l #1,d4
 move.w #$4e71,d3
 moveq.l #(64/8)-1,d6
exchangeaddqsloop:
  move.l (a1),d1
  cmp.w d3,d1
  beq.s exaddnonop1
    eor.w d4,d1
exaddnonop1:
  swap d1
  cmp.w d3,d1
  beq.s exaddnonop1a
    eor.w d4,d1
exaddnonop1a:
  swap d1
  move.l d1,(a1)+
  adda.w d0,a1
  move.l (a1),d1
  cmp.w d3,d1
  beq.s exaddnonop2
    eor.w d4,d1
exaddnonop2:
  swap d1
  cmp.w d3,d1
  beq.s exaddnonop2a
    eor.w d4,d1
exaddnonop2a:
  swap d1
  move.l d1,(a1)+
  adda.w d0,a1
  move.l (a1),d1
  cmp.w d3,d1
  beq.s exaddnonop3
    eor.w d4,d1
exaddnonop3:
  swap d1
  cmp.w d3,d1
  beq.s exaddnonop3a
    eor.w d4,d1
exaddnonop3a:
  swap d1
  move.l d1,(a1)+
  adda.w d0,a1
  move.l (a1),d1
  cmp.w d3,d1
  beq.s exaddnonop4
    eor.w d4,d1
exaddnonop4:
  swap d1
  cmp.w d3,d1
  beq.s exaddnonop4a
    eor.w d4,d1
exaddnonop4a:
  swap d1
  move.l d1,(a1)+
  adda.w d0,a1
  move.l (a1),d1
  cmp.w d3,d1
  beq.s exaddnonop5
    eor.w d4,d1
exaddnonop5:
  swap d1
  cmp.w d3,d1
  beq.s exaddnonop5a
    eor.w d4,d1
exaddnonop5a:
  swap d1
  move.l d1,(a1)+
  adda.w d0,a1
  move.l (a1),d1
  cmp.w d3,d1
  beq.s exaddnonop6
    eor.w d4,d1
exaddnonop6:
  swap d1
  cmp.w d3,d1
  beq.s exaddnonop6a
    eor.w d4,d1
exaddnonop6a:
  swap d1
  move.l d1,(a1)+
  adda.w d0,a1
  move.l (a1),d1
  cmp.w d3,d1
  beq.s exaddnonop7
    eor.w d4,d1
exaddnonop7:
  swap d1
  cmp.w d3,d1
  beq.s exaddnonop7a
    eor.w d4,d1
exaddnonop7a:
  swap d1
  move.l d1,(a1)+
  adda.w d0,a1
  move.l (a1),d1
  cmp.w d3,d1
  beq.s exaddnonop8
    eor.w d4,d1
exaddnonop8:
  swap d1
  cmp.w d3,d1
  beq.s exaddnonop8a
    eor.w d4,d1
exaddnonop8a:
  swap d1
  move.l d1,(a1)+
  adda.w d0,a1
 dbra d6,exchangeaddqsloop
 suba.w d0,a1
 adda.w #12,a1
 move.l (a1),d1
 cmp.w d3,d1
 beq.s exaddnonop9
   eor.w d4,d1
exaddnonop9:
 swap d1
 cmp.w d3,d1
 beq.s exaddnonop9a
    eor.w d4,d1
exaddnonop9a:
 swap d1
 move.l d1,(a1)
 movem.l (a7)+,d3-d4
 bra depackvolume

calcperiodtable:
 movem.l d1-d4,-(a7)
 move.w d1,d3
 move.w d2,d4
 move.w d2,-(a0)
 move.w d1,-(a0)
 move.w d7,-12(a0)
 clr.w -14(a0)    ; neu   PCS V1.2
 swap d7
 clr.w d7
 move.w a3,d5
 move.l hightable(PC),a3      ; made: (PC) 20.8.1900
 cmp.w d5,d1
 bhi.s calcperiodtabled1nothigh
calcperiodtabled1high:
  add.w d1,d1
  move.w 0(a3,d1.w),d1
  not.w d7
  cmp.w d5,d2
  bhi.s calcperiodtabled1cont
   add.w d2,d2
   move.w 0(a3,d2.w),d2
   addq.w #2,d7
   bra.s calcperiodtabled2nothigh
calcperiodtabled1cont:
  cmp.w d1,d2
  bhi.s calcperiodtabled2nothigh
   not.w d7
   bra.s calcperiodtabled2nothigh
calcperiodtabled1nothigh:
 cmp.w d5,d2
 bhi.s calcperiodtabled2nothigh
calcperiodtabled2high:
  add.w d2,d2
  move.w 0(a3,d2.w),d2
  not.w d7
  cmp.w d2,d1
  bhi.s calcperiodtabled2nothigh
   not.w d7
calcperiodtabled2nothigh:
 cmp.w d1,d2
 bhi.s calcperiodtablenoswap
  exg.l d1,d2
  exg.l d3,d4
calcperiodtablenoswap:  ; ----------------
 moveq.l #1,d6
 tst.w d7
 beq calcperiodtablenormal
calcperiodtablehigh:    ; BEIDE HI
  bmi calcperiodtablechange
 cmp.w d1,d2
 bne.s notcalced2same
   cmp.w d3,d4
   bhi.s isinrightorder
     exg.l d1,d2
     exg.l d3,d4
isinrightorder:
notcalced2same:
 tst.w -10(a0)
 bmi azeroloopgiven
 moveq.l #0,d5
 move.w d1,d5
 swap.w d5
 add.w d3,d3
 divu.w d3,d5
 swap.w d5
 clr.w d5
 swap.w d5
 add.l d5,d5
 moveq.l #0,d7
 move.l d7,a5
 move.w d2,d7
 swap.w d7
 add.w d4,d4
 divu.w d4,d7
 cmp.w d1,d2
 beq.s persame1
   mulu.w d1,d7
   divu.w d2,d7
persame1:
 swap.w d7
 clr.w d7
 swap.w d7
 add.l d7,d7
 move.l d7,a2 ; --------
 move.w #$5288,d4
 move.w #$5488,d3
 moveq.l #-6,d7
 move.l d7,a4
 tst.w d0
 bpl.s nochange1
   eor.w d6,d3
   eor.w d6,d4
   neg.w d0
   addq.l #2,a4
nochange1:
 swap.w d6
 move.l a1,a3
 subq.w #2,a1
  bsr calcloop02
  move.w d1,0(a0,a4.l)
 move.l a4,d7
 eori.w #$0006,d7
 move.l d7,a4
 move.l a3,a1
 move.l a2,d5
 cmp.l d5,d6
 bhi.s doc1
   moveq.l #1,d7
   eor.w d7,d3
   eor.w d7,d4
   bsr calcloop02
   move.w d1,0(a0,a4.l)
    bra calcperiodtablecont
doc1:
  move.w d4,d3
  eori.w #$0001,d3
  move.w #$4e71,d4
  bsr calcloop01
  move.w d1,0(a0,a4.l)
  move.w d5,-14(a0)   ; neu  PCS V1.2
   bra calcperiodtablecont
azeroloopgiven: ; --- beide hi, eines aus
 tst.w d0
 bpl.s zeroloop_per
zeroloop_hi:
   neg.w d0
   moveq.l #0,d5
   move.w d1,d5
   swap.w d5
   add.w d3,d3
   divu.w d3,d5
   swap.w d5
   clr.w d5
   swap.w d5
   add.l d5,d5
   move.w #$5288,d4
   move.w #$5488,d3
   swap.w d6
   bsr calcloop02
    move.w d1,-6(a0)
    clr.w -4(a0)
    bra calcperiodtablecont
zeroloop_per:
 moveq.l #0,d5
 move.l d5,a5
 move.w d2,d5
 swap.w d5
 add.w d4,d4
 divu.w d4,d5
 cmp.w d1,d2
 beq.s persame1b
   mulu.w d1,d5
   divu.w d2,d5
persame1b:
 swap.w d5
 clr.w d5
 swap.w d5
 add.l d5,d5
 swap.w d6
 cmp.l d5,d6
 bhi.s doc1a
 move.w #$5288,d4
 move.w #$5488,d3
 bsr calcloop02
 move.w d1,-6(a0)
 clr.w -4(a0)
  bra calcperiodtablecont
doc1a:
 move.w #$5288,d3
 move.w #$4e71,d4
 bsr calcloop01
 move.w d1,-6(a0)
 clr.w -4(a0)
 move.w d5,-14(a0)  ; neu  PCS V1.2
  bra calcperiodtablecont
; --------------------------------
calcperiodtablechange:      ; ---- EINES HI, d. andere perioden angep.
 cmp.w d5,d3
 bls.s calcperiodtabledo
    exg.l d3,d4
    exg.l d1,d2
calcperiodtabledo:
  tst.w -10(a0)
  bmi anotherzeroloopgiven
;   moveq.l #0,d5
;   move.w d4,d5
;   lsl.l #7,d5
;   divu d1,d5
;   move.w d5,a5
   moveq.l #0,d5     ;
   move.w d1,d5      ; neu  PCS V1.2
   lsl.l #8,d5       ;
   lsl.l #2,d5       ;
   divu d4,d5        ;
   move.w d5,a5      ;
  moveq.l #0,d5
  move.w d1,d5
  swap d5
  divu.w d4,d5
  swap d5
  clr.w d5
  swap d5
  moveq.l #0,d7
  move.w d1,d7
  swap d7
  add.w d3,d3
  divu.w d3,d7
  swap d7
  clr.w d7
  swap d7
  add.l d7,d7
  move.l d7,a2
  move.w #$4e71,d4
  move.w #$5288,d3
  moveq.l #-6,d7
  move.l d7,a4
  tst.w d0
  bpl.s nochange2
    eor.w d6,d3
    neg.w d0
    addq.l #2,a4
nochange2:
  swap d6
  move.l a1,a3
  bsr calcloop01
  move.w d1,0(a0,a4.l)
  move.w d5,-14(a0)   ; neu  PCS V1.2
  move.l a4,d7
  eori.w #$0006,d7
  move.l d7,a4
  lea.l -2(a3),a1
  move.l a2,d5
  move.w d3,d4
  eori.w #$0001,d4
  eori.w #$0601,d3
  bsr calcloop02
  move.w d1,0(a0,a4.l)
   bra calcperiodtablecont
anotherzeroloopgiven:
  tst.w d0     ; NEU: d0=neg, wenn hi-anpassung des remain, per=ausgegang.
  bmi anotherzeroloop_hi
anotherzeroloop_per:
;   moveq.l #0,d5
;   move.w d4,d5
;   lsl.l #7,d5
;   divu d1,d5
;   move.w d5,a5
   moveq.l #0,d5  ;
   move.w d1,d5   ;
   lsl.l #8,d5    ;
   lsl.l #2,d5    ;  neu  PCS V1.2
   divu d4,d5     ;
   move.w d5,a5   ;
  moveq.l #0,d5
  move.w d1,d5
  swap d5
  divu.w d4,d5
  swap d5
  clr.w d5
  swap d5
  move.w #$4e71,d4
  move.w #$5288,d3
  swap d6
  bsr calcloop01
  move.w d1,-6(a0)
  clr.w -4(a0)
  move.w d5,-14(a0)  ; neu  PCS V1.2
   bra calcperiodtablecont
anotherzeroloop_hi:
  neg.w d0
  moveq.l #0,d5
  move.w d1,d5
  swap.w d5
  add.w d3,d3
  divu.w d3,d5
  swap d5
  clr.w d5
  swap d5
  add.l d5,d5
  move.w #$5488,d3
  move.w #$5288,d4
  swap d6
  bsr calcloop02
  move.w d1,-6(a0)
  clr.w -4(a0)
   bra calcperiodtablecont
calcperiodtablenormal:  ; ---  keines hi ODER eines hi+perioden angep.
 cmp.w d5,d4
 bls.s cptd
  cmp.w d5,d3           ;
  bhi.s reallynormal    ;
    exg.l d1,d2         ; NEU 19.8.90
    exg.l d3,d4         ;
    bra.s cptd          ;
reallynormal:           ;
;   moveq.l #0,d5
;   move.w d4,d5
;   lsl.l #7,d5
;   divu d3,d5
;   move.w d5,a5
   moveq.l #0,d5      ;
   move.w d3,d5       ;
   lsl.l #8,d5        ; neu  PCS V1.2
   lsl.l #2,d5        ;
   divu d4,d5         ;
   move.w d5,a5       ;
  moveq.l #0,d5
  move.w d3,d5
  swap.w d5
  divu.w d4,d5
  swap d5
  clr.w d5
  swap d5
  move.w #$4e71,d4
  move.w #$5288,d3
  moveq.l #-6,d7
  move.l d7,a3
  tst.w d0
  bpl.s nochange3
   neg.w d0
   tst.w -10(a0)
   bmi.s nochange3
    eor.w d6,d3
    addq.l #2,a3
nochange3:
  swap.w d6
  bsr calcloop01
  move.w d1,0(a0,a3.l)
  move.w d5,-14(a0)     ; neu  PCS V1.2
   bra.s calcperiodtablecont
cptd:
 moveq.l #0,d5
 move.l d5,a5
 move.w d2,d5
 swap.w d5
 add.w d4,d4
 divu.w d4,d5
 cmp.w d1,d2
 beq.s persame2
   mulu.w d1,d5
   divu.w d2,d5
persame2:
 swap d5
 clr.w d5
 swap d5
 add.l d5,d5
 move.w #$5288,d4
 move.w #$5488,d3
 moveq.l #-6,d7
 move.l d7,a3
 tst.w d0
 bpl.s nochange4
   neg.w d0
   tst.w -10(a0)
   bmi.s nochange4
    addq.l #2,a3
    eor.w d6,d3
    eor.w d6,d4
nochange4:
 swap d6
 cmp.l d5,d6
 bhi.s doc1b
 bsr calcloop02
 move.w d1,0(a0,a3.l)
  bra.s calcperiodtablecont
doc1b:
 move.w d4,d3
 move.w #$4e71,d4
 bsr calcloop01
 move.w d1,0(a0,a3.l)
 move.w d5,-14(a0)     ; neu  PCS V1.2
calcperiodtablecont:
 movem.l (a7)+,d1-d4
 bra.s depackvolume
make_dbra_mixer:   ; dbramixer!
 movem.l d1-d2,-(a7)
 cmp.w d1,d2
 beq.s .l2
 bcs.s .l1
   exg d1,d2
.l1:
 swap d2
 clr.w d2
 divu d1,d2
 move.w d2,-18(a0)
 swap d2
 clr.w d2
 lsr.l #8,d2
 tst.w d2
 bne.s .l99
   swap d2
   bra.s .l98
.l99:
 swap d2
 addq.w #4,d2
.l98:
 tst.w d0
 bpl.s .l3
   move.w d2,-8(a0)
   bra.s .l2
.l3:
 move.w d2,-10(a0)
.l2:
 movem.l (a7)+,d1-d2
depackvolumePLUS: ; ---------------
 move.l a1,a6
 suba.l a0,a6
depackvolume:
 move.l (a7)+,a5
 cmp.w d3,d4
 beq tabledepackready
 tst.l packed
 beq.s lookforit
justgetvolumepointer:   ; not packed
   cmp.w d4,d3
   bhi.s getvolumetablenoswap1
    exg.l d3,d4
getvolumetablenoswap1:
   move.l tablepercenttable(PC),a2   ; ONE PARTED
   moveq.l #64,d5
   sub.w d3,d5
   add.w d5,d5
   move.w 0(a2,d5.w),d5
   move.l percenttable(PC),a2        ; ONE PARTED
   adda.w d5,a2
   move.w d3,d5
   sub.w d4,d5
   add.w d5,d5
   move.w -2(a2,d5.w),d5
   move.l mixvolumetable(PC),a2      ; ONE PARTED
   lea.l 0(a2,d5.w),a5
   rts

lookforit:
 move.w d3,d6
 swap.w d6
 move.w d4,d6
 move.w d4,d7
 swap.w d7
 move.w d3,d7
 lea.l volumebuffertable(PC),a2
 moveq.l #((volumebuffertableend-volumebuffertable)/8)-1,d5
lookforitloop:
  move.l (a2)+,d0    ; neu, for FAST volume-mixing
  clr.b d0           ;
  cmpa.l d0,a5
  beq.s .l1
  addq.l #4,a2
 dbra d5,lookforitloop
.l1:
 cmp.l (a2),d6
 beq.s samevolumefound
 cmp.l (a2),d7
 beq.s samevolumefound
depackvolumetable:
 move.w d3,(a2)+
 move.w d4,(a2)+
 cmp.w d4,d3
 bhi.s getvolumetablenoswap
  exg.l d3,d4
getvolumetablenoswap:
 move.l tablepercenttable(PC),a2 ; ONE PARTED
 moveq.l #64,d5
 sub.w d3,d5
 add.w d5,d5
 move.w 0(a2,d5.w),d5
 move.l percenttable(PC),a2      ; ONE PARTED
 adda.w d5,a2
 move.w d3,d5
 sub.w d4,d5
 add.w d5,d5
 move.w -2(a2,d5.w),d5
 move.l mixvolumetable(PC),a2    ; ONE PARTED
 lea.l 0(a2,d5.w),a2
 moveq.l #4*9,d6
 move.l a5,a0
 movem.l (a2)+,d0-d5/a1/a3-a4
 movem.l d0-d5/a1/a3-a4,(a0)
 adda.w d6,a0
 movem.l (a2)+,d0-d4/a1/a3
 movem.l d0-d4/a1/a3,(a0)
 lea.l $c0(a5),a0
 movem.l (a2)+,d0-d5/a1/a3-a4
 movem.l d0-d5/a1/a3-a4,(a0)
 adda.w d6,a0
 movem.l (a2)+,d0-d4/a1/a3
 movem.l d0-d4/a1/a3,(a0)
samevolumefound:
tabledepackready:
 rts

calcloop02:   ; [addq.w #1 (d4)]  oder   [addq.w #2 (d3)]
 moveq.l #0,d7
 move.l d5,d7
 lsr.l #2,d5
 moveq.l #0,d1
 moveq.l #(64/8)-1,d2
periodcalctableloop:
  sub.l d7,d5
  add.l d6,d5
  bmi.s periodcalctableaddq21
periodcalctableaddq11:
   move.w d4,(a1)+              ; 8    2.fall 26
   bra.s periodcalctablecont1   ;10
periodcalctableaddq21:
  add.l d6,d5
  addq.w #4,d1                  ; 4    1.fall: 22
  move.w d3,(a1)+               ; 8
periodcalctablecont1:
  adda.w d0,a1
  sub.l d7,d5
  add.l d6,d5
  bmi.s periodcalctableaddq22
periodcalctableaddq12:
   move.w d4,(a1)+
   bra.s periodcalctablecont2
periodcalctableaddq22:
  add.l d6,d5
  addq.w #4,d1
  move.w d3,(a1)+
periodcalctablecont2:
  adda.w d0,a1
  sub.l d7,d5
  add.l d6,d5
  bmi.s periodcalctableaddq23
periodcalctableaddq13:
   move.w d4,(a1)+
   bra.s periodcalctablecont3
periodcalctableaddq23:
  add.l d6,d5
  addq.w #4,d1
  move.w d3,(a1)+
periodcalctablecont3:
  adda.w d0,a1
  sub.l d7,d5
  add.l d6,d5
  bmi.s periodcalctableaddq24
periodcalctableaddq14:
   move.w d4,(a1)+
   bra.s periodcalctablecont4
periodcalctableaddq24:
  add.l d6,d5
  addq.w #4,d1
  move.w d3,(a1)+
periodcalctablecont4:
  adda.w d0,a1
  sub.l d7,d5
  add.l d6,d5
  bmi.s periodcalctableaddq25
periodcalctableaddq15:
   move.w d4,(a1)+
   bra.s periodcalctablecont5
periodcalctableaddq25:
  add.l d6,d5
  addq.w #4,d1
  move.w d3,(a1)+
periodcalctablecont5:
  adda.w d0,a1
  sub.l d7,d5
  add.l d6,d5
  bmi.s periodcalctableaddq26
periodcalctableaddq16:
   move.w d4,(a1)+
   bra.s periodcalctablecont6
periodcalctableaddq26:
  add.l d6,d5
  addq.w #4,d1
  move.w d3,(a1)+
periodcalctablecont6:
  adda.w d0,a1
  sub.l d7,d5
  add.l d6,d5
  bmi.s periodcalctableaddq27
periodcalctableaddq17:
   move.w d4,(a1)+
   bra.s periodcalctablecont7
periodcalctableaddq27:
  add.l d6,d5
  addq.w #4,d1
  move.w d3,(a1)+
periodcalctablecont7:
  adda.w d0,a1
  sub.l d7,d5
  add.l d6,d5
  bmi.s periodcalctableaddq28
periodcalctableaddq18:
   move.w d4,(a1)+
   bra.s periodcalctablecont8
periodcalctableaddq28:
  add.l d6,d5
  addq.w #4,d1
  move.w d3,(a1)+
periodcalctablecont8:
  adda.w d0,a1
 dbra d2,periodcalctableloop
 suba.w d0,a1
 move.w #$4e71,14(a1)
 addi.w #64*4,d1
 rts

calcloop01:   ;    [addq.w #1 (d3)]   oder  [nop (d4)]
  move.l d5,d7
  lsr.l #8,d7
  lsr.w #1,d7
  sub.l d7,d5
 move.l d5,d7
 lsr.l #2,d5
 sub.l d7,d5
 add.l d6,d5
 moveq.l #0,d1
 moveq.l #(64/8)-1,d2
calcperiodtableloop:
  sub.l d7,d5
  bmi.s calcperiodtableloopaddq1
calcperiodtableloopnop1:
   move.w d4,(a1)+
   bra.s calcperiodtableloopcont1
calcperiodtableloopaddq1:
  move.w d3,(a1)+
  add.l d6,d5
  addq.w #4,d1
calcperiodtableloopcont1:
  adda.w d0,a1
  sub.l d7,d5
  bmi.s calcperiodtableloopaddq2
calcperiodtableloopnop2:
   move.w d4,(a1)+
   bra.s calcperiodtableloopcont2
calcperiodtableloopaddq2:
  move.w d3,(a1)+
  add.l d6,d5
  addq.w #4,d1
calcperiodtableloopcont2:
  adda.w d0,a1
  sub.l d7,d5
  bmi.s calcperiodtableloopaddq3
calcperiodtableloopnop3:
   move.w d4,(a1)+
   bra.s calcperiodtableloopcont3
calcperiodtableloopaddq3:
  move.w d3,(a1)+
  add.l d6,d5
  addq.w #4,d1
calcperiodtableloopcont3:
  adda.w d0,a1
  sub.l d7,d5
  bmi.s calcperiodtableloopaddq4
calcperiodtableloopnop4:
   move.w d4,(a1)+
   bra.s calcperiodtableloopcont4
calcperiodtableloopaddq4:
  move.w d3,(a1)+
  add.l d6,d5
  addq.w #4,d1
calcperiodtableloopcont4:
  adda.w d0,a1
  sub.l d7,d5
  bmi.s calcperiodtableloopaddq5
calcperiodtableloopnop5:
   move.w d4,(a1)+
   bra.s calcperiodtableloopcont5
calcperiodtableloopaddq5:
  move.w d3,(a1)+
  add.l d6,d5
  addq.w #4,d1
calcperiodtableloopcont5:
  adda.w d0,a1
  sub.l d7,d5
  bmi.s calcperiodtableloopaddq6
calcperiodtableloopnop6:
   move.w d4,(a1)+
   bra.s calcperiodtableloopcont6
calcperiodtableloopaddq6:
  move.w d3,(a1)+
  add.l d6,d5
  addq.w #4,d1
calcperiodtableloopcont6:
  adda.w d0,a1
  sub.l d7,d5
  bmi.s calcperiodtableloopaddq7
calcperiodtableloopnop7:
   move.w d4,(a1)+
   bra.s calcperiodtableloopcont7
calcperiodtableloopaddq7:
  move.w d3,(a1)+
  add.l d6,d5
  addq.w #4,d1
calcperiodtableloopcont7:
  adda.w d0,a1
  sub.l d7,d5
  bmi.s calcperiodtableloopaddq8
calcperiodtableloopnop8:
   move.w d4,(a1)+
   bra.s calcperiodtableloopcont8
calcperiodtableloopaddq8:
  move.w d3,(a1)+
  add.l d6,d5
  addq.w #4,d1
calcperiodtableloopcont8:
  adda.w d0,a1
 dbra d2,calcperiodtableloop
 suba.w d0,a1
 move.w a5,d5
 bne.s calcperiodtableloopcont9
 move.w d4,14(a1)
 rts
calcperiodtableloopcont9:
 moveq.l #0,d2
 move.w d1,d2
 add.l d2,d2
 add.l d2,d2
 lsl.l #8,d2
 divu d5,d2
 mulu d1,d2
 move.w d1,d5
 lsl.w #8,d5
 sub.w d2,d5
 bcs.s .l2
 beq.s .l2
 lsl.w #6,d5
 move.w d5,d2
 lsr.w #8,d2
 lsr.w #6,d2
 beq.s .l1
 addq.w #1,d2
 add.w d2,d1
.l1:
 move.w d3,14(a1)
 rts
.l2:
 moveq.l #0,d5
 move.w d4,14(a1)
 rts


; required data
; -------------

 dc.b ' MusicMaker V8(tm) 8 CHANNEL VERSION '
 dc.b ' - - Written in 1989 by DIRE CRACKS Inc.',$0d,$0a
 cnop 0,2
dosname: dc.b 'dos.library',0
audname: dc.b 'audio.device',0
 cnop 0,2
mychanneloffmelody:
 dc.w 0,999
 cnop 0,2

notetable: dc.w 856,832,808,784,760,740,720,700,680,660,640,622
           dc.w 604,588,572,556,540,524,508,494,480,466,452,440
           dc.w 428,416,404,392,380,370,360,350,340,330,320,311
           dc.w 302,294,286,278,270,262,254,247,240,233,226,220
           dc.w 214,208,202,196,190,185,180,175,170,160,151,143
           dc.w 135,127,120,113
mystructure: ; -0-
ms:
origvolumetable:   dc.w 0,1,2,3,5,7,11,16,22,28,34,40,46,52,58,64
lastvolumes:       ds.w CHANNELMAX*2    ;(nichts dazwischen !)
myinstrvecs:       ds.l INSTNUM
myinstrlens:       ds.w INSTNUM*8
strtvolus:         dc.w 0,1,2,3,5,7,11,16,22,28,34,40,46,52,58,64
speed:             dc.w 0
origspeed:         dc.w 0
newspeed:          dc.w 0
pattlen:           dc.w 0
fadespeedin:       dc.w 63
fadespeedout:      dc.w 21
channelshandle:    dc.b 0
internfinished:    dc.b 0
oneshot:           dc.b 0
_channelsfinished: dc.b 0
_dtchannelsfinished: dc.b 0
soundenable:       dc.b -1
_fadeflag:         dc.b 0
_fadefinished:     dc.b 0
_mutedchannels:    dc.b 0
fadehalf:          dc.b 0
doloudness:        dc.b 0
calltheroutine:    dc.b 0
dofadein:          dc.b 0
dofadeout:         dc.b 0
playeralert:       dc.b 0
ignorealert:       dc.b 0
mixertype:         dc.b 0
mixer:             dc.b 0
speedtoupdate:     dc.b 0
 cnop 0,2
insteoredflag:     dc.l 0
channelsenable:    dc.w 0
subroutine:        dc.l 0
called:            dc.w 0
lfoinstptrs:       ds.l 16         ; N.T.P.I. !!!
currlfotable:      ds.l INSTNUM+1  ;
calcspeed:         dc.w 0
mixcalcspeed:      dc.l 0
hi:                dc.w 179
hightable:         dc.l 0
modultable:        dc.w MODUL_7-ms,MODUL_6-ms,MODUL_5-ms,MODUL_4-ms
                   dc.w MODUL_3-ms,MODUL_2-ms,MODUL_1-ms,MODUL_0-ms
modultableend:
MODUL_0:           dc.l nextdata0-ms,1,0,0,0
MODUL_1:           dc.l nextdata1-ms,2,0,0,0
MODUL_2:           dc.l nextdata2-ms,4,0,0,0
MODUL_3:           dc.l nextdata3-ms,8,0,0,0
MODUL_4:           dc.l nextdata4-ms,16,0,0,0
MODUL_5:           dc.l nextdata5-ms,32,0,0,0
MODUL_6:           dc.l nextdata6-ms,64,0,0,0
MODUL_7:           dc.l nextdata7-ms,128,0,0,0

COUNTERS:          ds.l CHANNELMAX                            ; NOTHING TO
MACRO_C:           ds.l CHANNELMAX                            ; BE PUT
SCOUNT:            dc.w 1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0       ; INBETWEEN !
SOFTMOD:           ds.l CHANNELMAX                            ; !!!
VIBRATO:           ds.l CHANNELMAX                            ; !!!

lastperiods:       ds.l 6*CHANNELMAX
myinstrsdata:      dc.l 0
mymacrosdata:      dc.l 0
mymelodydata:      dc.l 0
loudnesstable:
 dc.w 256,264,268,273,277,287,292,297,303,309,315,321,327,334,341
  dc.w 348,356,364,372,372,381,390,399,399
 dc.w 409,420,420,431,431,442,442,455,468,468,468,481,481,481,496
  dc.w 496,496,512,512,512,512,512,512,512
 dc.w 512,512,512,512,511,511,511,511,511,511,511,511,511,511,511
  dc.w 511
 cnop 0,2

fibbits:           dc.w 0
fibtable:          dc.l 0
mymixbufptr:       dc.l 0
mymixbuflen:       dc.l 0
mytable:           dc.l 0
basicgivenoneshot: dc.l 0
basicmacrosdata:   dc.l 0
basicmacroslen:    dc.l 0
basicinstrsdata:   dc.l 0
basicinstrslen:    dc.l 0
basicfilename:     dc.l 0
filenamebuffer:    ds.b 256  ;40    ;1754(a5)
filenameendptr:    dc.l 0
mydosbase:         dc.l 0
filehandle:        dc.l 0
memhandle:         dc.l 0
memlen:            dc.l 0
loadandinitrout:   dc.l 0
ipfileptr:         dc.l 0
samplefileadr:     dc.l 0
samplefilesize:    dc.l 0
allocmap: dc.b 15
audopen:  dc.b 0
 cnop 0,4
loadfibtable:
myinfoblock:       ds.b 270
oldintvector:      dc.l 0

; MIXER DATA:
; SOFTWARE CHANNEL STRUCTURES
channel7struc:
           ds.b 16
nextdata7: ds.b 8
olddata7: ds.w 3        ; NEW for "("
olddata7backup: ds.w 3  ;
channel6struc:
           ds.b 16
nextdata6: ds.b 8
olddata6: ds.w 3        ; NEW for "("
olddata6backup: ds.w 3  ;
channel5struc:
           ds.b 16
nextdata5: ds.b 8
olddata5: ds.w 3        ; NEW for "("
olddata5backup: ds.w 3  ;
channel4struc:
           ds.b 16
nextdata4: ds.b 8
olddata4: ds.w 3        ; NEW for "("
olddata4backup: ds.w 3  ;
channel3struc:
           ds.b 16
nextdata3: ds.b 8
olddata3: ds.w 3        ; NEW for "("
olddata3backup: ds.w 3  ;
channel2struc:
           ds.b 16
nextdata2: ds.b 8
olddata2: ds.w 3        ; NEW for "("
olddata2backup: ds.w 3  ;
channel1struc:
           ds.b 16
nextdata1: ds.b 8
olddata1: ds.w 3        ; NEW for "("
olddata1backup: ds.w 3  ;
channel0struc:
           ds.b 16
nextdata0: ds.b 8
olddata0: ds.w 3        ; NEW for "("
olddata0backup: ds.w 3  ;

; HARDWARE CHANNELS
hwchannel0:
 dc.l 0,0     ; buffer1,buffer2 (allocated)
 dc.w 0,0     ;
 dc.l 0       ; instptr A (ptr to REST)
 dc.w 0,0,0   ; per A, vol A, len A (REST/Bytes)
 dc.l 0       ; instptr B (ptr to REST)
 dc.w 0,0,0   ; per B, vol B, len B (REST/Bytes)
hwchannel0pervol:
 dc.w 0,0     ; per,vol (written to DMA-Regs)
 dc.w 0       ; instlen ( " " " )
 dc.l volumebuffer0
 dc.l $10000
 dc.l volumebuffer0
 ds.w 14
 dc.l 0,0,0

hwchannel1:
 dc.l 0,0     ; buffer1,buffer2 (allocated)
 dc.w 0,0     ;
 dc.l 0       ; instptr A (ptr to REST)
 dc.w 0,0,0   ; per A, vol A, len A (REST/Bytes)
 dc.l 0       ; instptr B (ptr to REST)
 dc.w 0,0,0   ; per B, vol B, len B (REST/Bytes)
hwchannel1pervol:
 dc.w 0,0     ; per,vol (written to DMA-Regs)
 dc.w 0       ; instlen ( " " " )
 dc.l volumebuffer1
 dc.l $10000
 dc.l volumebuffer1
 ds.w 14
 dc.l 0,0,0 ; dbramixer !
 
hwchannel2:
 dc.l 0,0     ; buffer1,buffer2 (allocated)
 dc.w 0,0     ;
 dc.l 0       ; instptr A (ptr to REST)
 dc.w 0,0,0   ; per A, vol A, len A (REST/Bytes)
 dc.l 0       ; instptr B (ptr to REST)
 dc.w 0,0,0   ; per B, vol B, len B (REST/Bytes)
hwchannel2pervol:
 dc.w 0,0     ; per,vol (written to DMA-Regs)
 dc.w 0       ; instlen ( " " " )
 dc.l volumebuffer2
 dc.l $10000
 dc.l volumebuffer2
 ds.w 14
 dc.l 0,0,0

base: 
hwchannel3:
 dc.l 0,0     ; buffer1,buffer2 (allocated)
 dc.w 0,0     ; used for sync
 dc.l 0       ; instptr A (ptr to REST)
 dc.w 0,0,0   ; per A, vol A, len A (REST/Bytes)
 dc.l 0       ; instptr B (ptr to REST)
 dc.w 0,0,0   ; per B, vol B, len B (REST/Bytes)
hwchannel3pervol:
 dc.w 0,0     ; per,vol (written to DMA-Regs)
 dc.w 0       ; instlen ( " " " )
 dc.l volumebuffer3
 dc.l $10000
 dc.l volumebuffer3
mya0buffer: dc.l 0
mya1buffer: dc.l 0
myd0buffer: dc.w 0
myd1buffer: dc.w 0
myd2buffer: dc.w 0
myd3buffer: dc.w 0
myd4buffer: dc.w 0
 ds.w 5  ; unused !
 dc.l 0,0,0

;
; MULTI-CHANNEL PLAYER V1.2
; TABLES HANDLING
; VERSION with asr-d instrs
; (C) DC 1990-93

 xdef _maketables  ;,packed

************
*_maketables
************
* INPUT:   a0= free space for table, (64*2)+(65*32*2)+((d0)*packed) Bytes
*          d0= percent resolution (16-100)
*          d1= packed (128=0/256<>0)
****

; ---------
_newmaketables:
 moveq.l #64,d0
_maketables:
 movem.l a0-a6/d0-d7,-(a7)
 lea.l tablespointer(PC),a5
 move.l a0,(a5)+          ; tablepercenttable    64*2 Bytes    !
 lea.l 64*2(a0),a0
 move.l a0,(a5)+          ; percenttable         65*32*2 Bytes !
 lea.l 255+(65*32*2)(a0),a0   ; "255+" neu for FAST vol-mix
 move.l a0,(a5)+          ; mixvolumetable       d0*d1 Bytes   !
 clr.b -1(a5)     ; neu for FAST vol-mix
 move.l d0,(a5)+
 move.l d1,(a5)+

 move.l tablepercenttable(PC),a4
 move.l percenttable(PC),a5
 move.l percentresolution(PC),d2
 move.l packed(PC),d3
 moveq.l #8,d5
 tst.w d3
 bne.s calcpercentpercentasrnopack
  moveq.l #7,d5
calcpercentpercentasrnopack:
 moveq.l #0,d7
 moveq.l #64,d0
calcpercentpercentasrtablesout:
  move.w d7,(a4)+
  move.l d0,d1
  subq.w #1,d1
calcpercentpercentasrtablesin:
   move.l d1,d6
   swap.w d6
   divu.w d0,d6
   mulu.w d2,d6
   swap.w d6
   lsl.w d5,d6
   move.w d6,(a5)+
   addq.w #2,d7
  dbra d1,calcpercentpercentasrtablesin
  subq.w #1,d0
 bne.s calcpercentpercentasrtablesout

 move.l percentresolution(PC),d2
 move.l packed(PC),d3
 move.l mixvolumetable(PC),a4
 lea.l $80(a4),a5
 tst.w d3
 beq.s calcpercentvolumeasrnopack1
  lea.l $80(a5),a5
calcpercentvolumeasrnopack1:
 moveq.l #0,d0
calcpercentvolumeasrtablesout:
  moveq.l #0,d1
  moveq.l #0,d5
  moveq.l #64-1,d6
calcpercentvolumeasrtablesin:
   move.w d1,d7
   mulu.w d0,d7
   divu.w d2,d7
   move.b d7,(a4)+
   move.b d5,-(a5)
   sub.b d7,(a5)
   addq.b #1,d1
  dbra d6,calcpercentvolumeasrtablesin
  adda.w #64,a4
  adda.w #128+64,a5
  tst.w d3
  beq.s calcpercentvolumeasrnopack2
   adda.w #128,a4
   adda.w #128,a5
calcpercentvolumeasrnopack2:
  addq.w #1,d0
  cmp.w d0,d2
 bne.s calcpercentvolumeasrtablesout
 movem.l (a7)+,a0-a6/d0-d7
 rts

_setupcachecontrol:     ; d0=system version, d1=attflags
 lea.l clearcache1(PC),a0
 move.w #$4e71,(a0)
 move.w #$4e71,clearcache2-clearcache1(a0)
 cmpi.w #37,d0
 bcs.s .l1
 btst #1,d1
 beq.s .l1
 btst #3,d1
 beq.s .l2
 bra.s .l3
.l1:
 move.w #$4e75,(a0)
.l2:
 move.w #$4e75,clearcache2-clearcache1(a0)
.l3:
 cmpi.w #37,d0
 bcs.s .l4
 btst #1,d1
 beq.s .l4
    movem.l d0-d1/a0-a1/a6,-(a7)
    move.l $4,a6
    jsr -636(a6)
    movem.l (a7)+,d0-d1/a0-a1/a6
.l4:
 rts

clearcache1:
 rts
 movem.l d0-d1/a0-a1/a6,-(a7)
 move.l $4,a6
 cmpi.w #37,20(a6)
 bcs.s .l1
   jsr -636(a6)
.l1:
 movem.l (a7)+,d0-d1/a0-a1/a6
 rts

clearcache2:
 rts  ; Writing into CHIP RAM requires no clearing of cache!
 movem.l d0-d1/a0-a1/a6,-(a7)
 move.l $4,a6
 cmpi.w #37,20(a6)
 bcs.s .l1
 move.l hwchannel0+4(PC),a0
 lea.l cachelength(PC),a1
 moveq.l #0,d0
 move.w hwchannel0+36(PC),d0
 add.w d0,d0
 move.l d0,(a1)
 moveq.l #0,d0
 jsr -762(a6)
 move.l hwchannel1+4(PC),a0
 lea.l cachelength(PC),a1
 moveq.l #0,d0
 move.w hwchannel1+36(PC),d0
 add.w d0,d0
 move.l d0,(a1)
 moveq.l #0,d0
 jsr -762(a6)
 move.l hwchannel2+4(PC),a0
 lea.l cachelength(PC),a1
 moveq.l #0,d0
 move.w hwchannel2+36(PC),d0
 add.w d0,d0
 move.l d0,(a1)
 moveq.l #0,d0
 jsr -762(a6)
 move.l hwchannel3+4(PC),a0
 lea.l cachelength(PC),a1
 moveq.l #0,d0
 move.w hwchannel3+36(PC),d0
 add.w d0,d0
 move.l d0,(a1)
 moveq.l #0,d0
 jsr -762(a6)
.l1:
 movem.l (a7)+,d0-d1/a0-a1/a6
 rts
cachelength: dc.l 0

; data

tablespointer:
tablepercenttable: dc.l 0  ;
percenttable:      dc.l 0  ;
mixvolumetable:    dc.l 0  ; NICHTS DAZW. !!!
percentresolution: dc.l 0  ;
packed:            dc.l 0  ;

volumebuffertable:
  dc.l volumebuffer0,0,volumebuffer1,0
  dc.l volumebuffer2,0,volumebuffer3,0
volumebuffertableend:

               ds.b 256    ; WICHTIG!!!
volumebuffer0: ds.b 256
volumebuffer1: ds.b 256
volumebuffer2: ds.b 256
volumebuffer3: ds.b 256


mixlooptable:
 dc.l perper,pervol,volper
 dc.l Lzeropervol,Lzeroper
 dc.l Sperper,Spervol   ; 2 addqs !!!
mixlooptableend:

; MULTI-CHANNEL PLAYER V1.2   PC
; (C) DC 1990, 1991
; VERSION FAST & BAD
; RETURNMIXER-ROUTINE + MIXLOOPS + CALCULATEINSTRUMENTS

; xdef returnmixer,calculateinstruments,initmixloops

;INSTNUM equ 36

calculateinstruments:
 moveq.l #INSTNUM-1,d0
calculateinstrsloop:
  move.l (a0)+,a2
  move.w (a1),d1
  beq.s iunset
calcinstloop:
   move.b (a2),d2
   asr.b #1,d2
   move.b d2,(a2)+
  subq.w #1,d1
 bne.s calcinstloop
iunset:
 addq.l #8,a1
 dbra d0,calculateinstrsloop
 rts

initmixloops:
 lea.l simple-4(PC),a0
 clr.l (a0)
 clr.l perper-simple(a0)
 clr.l volvol-simple(a0)
 clr.l pervol-simple(a0)
 clr.l volper-simple(a0)
 clr.l Lzeropervol-simple(a0)
 clr.l Lzeroper-simple(a0)
 clr.l Lzerovol-simple(a0)
 clr.l Lzerosimple-simple(a0)
 clr.l Sperper-simple(a0)
 clr.l Spervol-simple(a0)
 rts

returnmixer: ; d0=per1,d1=per2,d2=vol1,d3=vol2 | a0=mixer,d7.w=0|-1
 clr.w d7    ; d5,d6=on(<>0),off(0),a2=hi-limit-1 ; a1=mixerl,d0=l.l
 tst.w d6    ;                      a3=hightable-226
 beq lookforzeromixer2
 tst.w d5
 beq lookforzeromixer1
 cmp.w a2,d0
 bls lookforspecialmixer1
 cmp.w a2,d1
 bls lookforspecialmixer2
lookfornormalmixer:
 cmp.w d0,d1
 beq.s lfnmvol
 cmp.w d2,d3
 beq.s lfnmper
lfnmpervol:
 cmp.w d1,d0
 bls.s lfnmpervol2
lfnmpervol1:
  cmp.w d2,d3
  bhi.s lfnmpervol1ok
lfnmpervol1xok:
   not.w d7
   lea.l volper(PC),a0
   lea.l volperl(PC),a1
   moveq.l #-(volperl-volper),d0
   rts
lfnmpervol1ok:
  lea.l pervol(PC),a0
  lea.l pervoll(PC),a1
  moveq.l #pervoll-pervol,d0
  rts
lfnmpervol2:
  cmp.w d2,d3
  bhi.s lfnmpervol2ok
lfnmpervol2xok:
   not.w d7
   lea.l pervol(PC),a0
   lea.l pervoll(PC),a1
   moveq.l #pervoll-pervol,d0
   rts
lfnmpervol2ok:
  lea.l volper(PC),a0
  lea.l volperl(PC),a1
  moveq.l #-(volperl-volper),d0
  rts
lfnmvol:
 cmp.w d2,d3
 beq.s lfnmsimpleok
 bhi.s lfnmvolok
  not.w d7
lfnmvolok:
 lea.l volvol(PC),a0
 lea.l volvoll(PC),a1
 moveq.l #0,d0
 rts
lfnmper:
 cmp.w d1,d0
 bhi.s lfnmperok
  not.w d7
lfnmperok:
 lea.l perper(PC),a0
 lea.l perperl(PC),a1
 moveq.l #perperl-perper,d0
 rts
lfnmsimpleok:
 lea.l simple(PC),a0
 lea.l simplel(PC),a1
 moveq.l #0,d0
 rts

lookforspecialmixer1:
 cmp.w a2,d1
 bls lookforspecialspecialmixer
 add.w d0,d0
 move.w 0(a3,d0.w),d0
 cmp.w d0,d1
 beq.s lfsm1vol
 cmp.w d2,d3
 beq lfsm1per
lfsm1pervol:
 cmp.w d1,d0
 bls.s lfsm1pervol2
lfsm1pervol1:
  cmp.w d2,d3
  bhi.s lfsm1pervol1ok
lfsm1pervol1xok:
   not.w d7
   lea.l volper(PC),a0
   lea.l volperl(PC),a1
   moveq.l #-(volperl-volper),d0
   rts
lfsm1pervol1ok:
  lea.l pervol(PC),a0
  lea.l pervoll(PC),a1
  moveq.l #pervoll-pervol,d0
  rts
lfsm1pervol2:
  cmp.w d2,d3
  bhi.s lfsm1pervol2ok
lfsm1pervol2xok:
   not.w d7
   lea.l Spervol(PC),a0   ; !!! hi !!!
   lea.l Spervoll(PC),a1
   moveq.l #Spervoll-Spervol,d0  ; NEU
   rts
lfsm1pervol2ok:
  lea.l Spervol(PC),a0
  lea.l Spervoll(PC),a1
  moveq.l #-(Spervoll-Spervol),d0   ; NEU
  rts
lfsm1vol:
 cmp.w d2,d3
 beq.s lfsm1simpleok
 bhi.s lfsm1volok
  not.w d7
  lea.l volper(PC),a0  ; !!! hi !!!
  lea.l volperl(PC),a1
  moveq.l #-(volperl-volper),d0
  rts
lfsm1volok:
 lea.l pervol(PC),a0
 lea.l pervoll(PC),a1
 moveq.l #pervoll-pervol,d0
 rts
lfsm1per:
 cmp.w d1,d0
 bhi.s lfsm1perok
  lea.l Sperper(PC),a0
  lea.l Sperperl(PC),a1
  moveq.l #-(Sperperl-Sperper),d0
  rts
lfsm1perok:
 lea.l perper(PC),a0
 lea.l perperl(PC),a1
 moveq.l #perperl-perper,d0
 rts
lfsm1simpleok:
 lea.l perper(PC),a0
 lea.l perperl(PC),a1
 moveq.l #perperl-perper,d0
 rts

lookforspecialmixer2:
 add.w d1,d1
 move.w 0(a3,d1.w),d1
 cmp.w d0,d1
 beq.s lfsm2vol
 cmp.w d2,d3
 beq lfsm2per
; beq.s lfsm2per
lfsm2pervol:
 cmp.w d1,d0
 bls.s lfsm2pervol2
lfsm2pervol1:
  cmp.w d2,d3
  bhi.s lfsm2pervol1ok
lfsm2pervol1xok:
   not.w d7
   lea.l Spervol(PC),a0
   lea.l Spervoll(PC),a1
   moveq.l #-(Spervoll-Spervol),d0
   rts
lfsm2pervol1ok:
  lea.l Spervol(PC),a0
  lea.l Spervoll(PC),a1
  moveq.l #Spervoll-Spervol,d0
  rts
lfsm2pervol2:
  cmp.w d2,d3
  bhi.s lfsm2pervol2ok
lfsm2pervol2xok:
   not.w d7
   lea.l pervol(PC),a0 ; !!! hi !!!
   lea.l pervoll(PC),a1
   moveq.l #pervoll-pervol,d0
   rts
lfsm2pervol2ok:
  lea.l volper(PC),a0
  lea.l volperl(PC),a1
  moveq.l #-(volperl-volper),d0
  rts
lfsm2vol:
 cmp.w d2,d3
 beq.s lfsm2simpleok
 bhi.s lfsm2volok
  not.w d7
  lea.l pervol(PC),a0 ; !!! hi !!!
  lea.l pervoll(PC),a1
  moveq.l #pervoll-pervol,d0
  rts
lfsm2volok:
 lea.l volper(PC),a0
 lea.l volperl(PC),a1
 moveq.l #-(volperl-volper),d0
 rts
lfsm2per:
 cmp.w d1,d0
 bhi.s lfsm2perok
  not.w d7
  lea.l perper(PC),a0 ; !!! hi !!!
  lea.l perperl(PC),a1
  moveq.l #perperl-perper,d0
  rts
lfsm2perok:
 lea.l Sperper(PC),a0
 lea.l Sperperl(PC),a1
 moveq.l #Sperperl-Sperper,d0
 rts
lfsm2simpleok:
 not.w d7
 lea.l perper(PC),a0
 lea.l perperl(PC),a1
 moveq.l #perperl-perper,d0
 rts

lookforspecialspecialmixer:
 move.w d0,d5
 move.w d1,d6
 add.w d0,d0
 add.w d1,d1
 move.w 0(a3,d0.w),d0
 move.w 0(a3,d1.w),d1
 cmp.w d0,d1
 beq.s lfssmvol
 cmp.w d2,d3
 beq lfssmper
lfssmpervol:
 cmp.w d1,d0
 bls.s lfssmpervol2
lfssmpervol1:
  cmp.w d2,d3
  bhi.s lfssmpervol1ok
lfssmpervol1xok:
   not.w d7
   lea.l Spervol(PC),a0
   lea.l Spervoll(PC),a1
   moveq.l #Spervoll-Spervol,d0
   rts
lfssmpervol1ok:
  lea.l Spervol(PC),a0
  lea.l Spervoll(PC),a1
  moveq.l #-(Spervoll-Spervol),d0
  rts
lfssmpervol2:
  cmp.w d2,d3
  bhi.s lfssmpervol2ok
lfssmpervol2xok:
   not.w d7
   lea.l Spervol(PC),a0
   lea.l Spervoll(PC),a1
   moveq.l #-(Spervoll-Spervol),d0
   rts
lfssmpervol2ok:
  lea.l Spervol(PC),a0
  lea.l Spervoll(PC),a1
  moveq.l #Spervoll-Spervol,d0
  rts
lfssmvol:
 cmp.w d5,d6  ; ?????????
 bhi.s lfssmper2
 cmp.w d2,d3
 beq.s lfssmSSsimpleok
 bhi.s lfssmvolok
  not.w d7
  lea.l Spervol(PC),a0
  lea.l Spervoll(PC),a1
  moveq.l #Spervoll-Spervol,d0
  rts
lfssmvolok:
 lea.l Spervol(PC),a0
 lea.l Spervoll(PC),a1
 moveq.l #-(Spervoll-Spervol),d0
 rts
lfssmper2:
 cmp.w d2,d3
 beq.s lfssmSSsimpleok2
 bhi.s lfssmvol2ok
  not.w d7
  lea.l Spervol(PC),a0
  lea.l Spervoll(PC),a1
  moveq.l #-(Spervoll-Spervol),d0
  rts
lfssmvol2ok:
 lea.l Spervol(PC),a0
 lea.l Spervoll(PC),a1
 moveq.l #Spervoll-Spervol,d0
 rts

lfssmper:
 cmp.w d1,d0
 bhi.s lfssmperok
  not.w d7
  lea.l Sperper(PC),a0
  lea.l Sperperl(PC),a1
  moveq.l #-(Sperperl-Sperper),d0
  rts
lfssmperok:
 lea.l Sperper(PC),a0
 lea.l Sperperl(PC),a1
 moveq.l #-(Sperperl-Sperper),d0
 rts
lfssmSSsimpleok:
 lea.l Sperper(PC),a0
 lea.l Sperperl(PC),a1
 moveq.l #-(Sperperl-Sperper),d0
 rts
lfssmSSsimpleok2:
 lea.l Sperper(PC),a0
 lea.l Sperperl(PC),a1
 moveq.l #Sperperl-Sperper,d0
 rts

lookforzeromixer2:  ; s2=0
 tst.w d5
 beq simpleclearmixer
checkfzm:
 cmp.w a2,d0
 bls lookforzerospecialmixer
 cmp.w a2,d1
 bhi.s lfzmnohi2
  add.w d1,d1
  move.w 0(a3,d1.w),d1
lfzmnohi2:
 cmp.w d1,d0
 bhi.s lfzmper1
lfzmvol1:
  cmp.w d2,d3
  bls.s lfzmsimpleok
lfzmvolok:
   lea.l Lzerovol(PC),a0
   lea.l Lzerovoll(PC),a1
   moveq.l #0,d0
   rts
lfzmper1:
  cmp.w d2,d3
  bls.s lfzmperok
lfzmpervolok:
   lea.l Lzeropervol(PC),a0
   lea.l Lzeropervoll(PC),a1
   moveq.l #Lzeropervoll-Lzeropervol,d0
   rts
lfzmperok:
  lea.l Lzeroper(PC),a0
  lea.l Lzeroperl(PC),a1
  moveq.l #Lzeroperl-Lzeroper,d0
  rts
lfzmsimpleok:
 lea.l Lzerosimple(PC),a0
 lea.l Lzerosimplel(PC),a1
 moveq.l #0,d0
 rts
lookforzeromixer1:
 not.w d7
 exg d0,d1
 exg d2,d3
 bra checkfzm

lookforzerospecialmixer:
 move.w d0,d5        ;
 move.w d1,d6        ;
 cmp.w a2,d1
 bhi.s lfzsmnohi2
  add.w d1,d1
  move.w 0(a3,d1.w),d1
lfzsmnohi2:
 add.w d0,d0
 move.w 0(a3,d0.w),d0
 cmp.w d1,d0
 bhi.s lfzsmper1
 bne.s lfzsmvol1     ;
  cmp.w d6,d5        ; NEU !?!
  bhi.s lfzsmper1    ;
lfzsmvol1:
  cmp.w d2,d3
  bls.s lfzsmsimpleok
lfzsmvolok:
   lea.l Lzeropervol(PC),a0            ; NUR hi-anpassung
   lea.l Lzeropervoll(PC),a1
   moveq.l #-(Lzeropervoll-Lzeropervol),d0   ; NEU: -()
   rts
lfzsmper1:
  cmp.w d2,d3
  bls.s lfzsmperok
lfzsmpervolok:
   lea.l Lzeropervol(PC),a0
   lea.l Lzeropervoll(PC),a1
   moveq.l #Lzeropervoll-Lzeropervol,d0
   rts
lfzsmperok:
  lea.l Lzeroper(PC),a0
  lea.l Lzeroperl(PC),a1
  moveq.l #Lzeroperl-Lzeroper,d0
  rts
lfzsmsimpleok: ; nur, wenn hi kleiner als ausgegangenes
 lea.l Lzeroper(PC),a0
 lea.l Lzeroperl(PC),a1
 moveq.l #-(Lzeroperl-Lzeroper),d0  ; wg. hi !
 rts

simpleclearmixer:
 lea.l simpleclear(PC),a0
 lea.l simpleclearl(PC),a1
 moveq.l #0,d0
 rts


; MULTI-CHANNEL PLAYER V1.2  PCS V1.2
; MIXLOOPS

o  equ  0      ; offset volumetable

c  equr d0     ; counter 
s1 equr a0     ; source inst 1
s2 equr a1     ; source inst 2
t1 equr a6     ; mixbuffer
vt equr a5     ; volumetable , now: TRASH
u  equr d7     ; TRASH
;u1 equr d6     ; TRASH
vtx equr d5    ; volumetable
; d4=0 PCS !!!
; d6=1

; xdef simple,perper,volvol,pervol,volper,simpleclear
; xdef Lzeropervol,Lzeroper,Lzerovol,Lzerosimple
; xdef Sperper
; xdef Spervol

 dc.w 0,0,1,1
 dc.w 64*4,64*4,0
 dc.l 0
simple:   ; per=per, vol=vol
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
simplel:
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
 subq.w #1,c
 bne simple
 rts

 dc.w 0,0,1,2
 dc.w 0,64*4,perperl-perper
 dc.l 0
perper:      ; per<>per, vol=vol        s1=per
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
perperl:
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
 add.w d6,d4
 bcs.s perperadd
 subq.w #1,c
 bne perper
 rts
 nop
perperadd:
  addq.w #1,s1
 subq.w #1,c
 bne perper
 rts

 dc.w 0,0,1,3
 dc.w 64*4,64*4,0
 dc.l 0
volvol:      ; per=per, vol<>vol        s1=vol
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
volvoll:
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
 subq.w #1,c
 bne volvol
 rts

 dc.w 0,0,1,4
 dc.w 0,64*4,pervoll-pervol
 dc.l 0
pervol:      ; per<>per, vol<>vol       s1=vol+per
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
pervoll:
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  addq.l #1,s1
 add.w d6,d4
 bcs.s pervoladd
 subq.w #1,c
 bne pervol
 rts
 nop
pervoladd:
  addq.w #1,s1
 subq.w #1,c
 bne pervol
 rts

 dc.w 0,0,1,5
 dc.w 64*4,0,-(volperl-volper)
 dc.l 0
volper:      ; per<>per, vol<>vol  s1=vol s2=per
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
volperl:
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
 add.w d6,d4
 bcs.s volperadd
 subq.w #1,c
 bne volper
 rts
 nop
volperadd:
  addq.l #1,s2
 subq.w #1,c
 bne volper
 rts

 dc.w 0,0,-1,6
 dc.w 0,0,Lzeropervoll-Lzeropervol
 dc.l 0
Lzeropervol:    ; one=zero!             s1=per+vol s2=0, s1=low
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
Lzeropervoll:
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
  move.b (s1),vtx    ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  addq.l #1,s1
 add.w d6,d4
 bcs.s Lzeropervoladd
 subq.w #1,c
 bne Lzeropervol
 rts
 nop
Lzeropervoladd:
  addq.l #1,s1
 subq.w #1,c
 bne Lzeropervol
 rts

 dc.w 0,0,-1,7
 dc.w 0,0,Lzeroperl-Lzeroper
 dc.l 0
Lzeroper:    ; one=zero!   per          s1=per   s2=0     s1=low
  move.b (s1),(t1)+  ; 1
Lzeroperl:
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
  move.b (s1),(t1)+  ; 1
  addq.l #1,s1
 add.w d6,d4
 bcs.s Lzeroperadd
 subq.w #1,c
 bne Lzeroper
 rts
 nop
Lzeroperadd:
  addq.l #1,s1
 subq.w #1,c
 bne Lzeroper
 rts

 dc.w 0,0,-1,8
 dc.w 64*4,0,0
 dc.l 0
Lzerovol:    ; one=zero!   vol          s1=vol   s2=0  s1=low
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
Lzerovoll:
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
 subq.w #1,c
 bne Lzerovol
 rts

 dc.w 0,0,-1,10
 dc.w 64*4,0,0
 dc.l 0
Lzerosimple:   ;           s2=zero, s1=low
  move.b (s1)+,(t1)+ ; 1
Lzerosimplel:
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
 subq.w #1,c
 bne Lzerosimple
 rts


; specials

 dc.w 0,0,0,12
 dc.w 0,0,Sperperl-Sperper
 dc.l 0
Sperper:      ; per<>per, vol=vol        s1=per   s2=hi
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
Sperperl:
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),u ; 1
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
 add.w d6,d4
 bcs.s Sperperadd
 subq.w #1,c
 bne Sperper
 rts
Sperperadd:
  addq.l #1,s2
  addq.l #1,s1
 subq.w #1,c
 bne Sperper
 rts

 dc.w 0,0,0,13
 dc.w 0,0,Spervoll-Spervol
 dc.l 0
Spervol:      ; per<>per, vol<>vol  s1=vol+per       s2=hi
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
Spervoll:
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
  move.b (s1),vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2),u
  move.b u,(t1)+
  addq.l #1,s2
  addq.l #1,s1
 add.w d6,d4
 bcs.s Spervoladd
 subq.w #1,c
 bne Spervol
 rts
Spervoladd:
  addq.l #1,s2
  addq.l #1,s1
 subq.w #1,c
 bne Spervol
 rts

 dc.w 0,0,1,14
 dc.w 0,0,0
 dc.l 0
simpleclear:
  move.b u,(t1)+ ; 1
simpleclearl:
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
 subq.w #1,c
 bne simpleclear
 rts 


; xdef returndbramixer

returndbramixer:
;returnmixer: ; d0=per1,d1=per2,d2=vol1,d3=vol2 | a0=mixer,d7.w=0|-1
 clr.w d7    ; d5,d6=on(<>0),off(0),           ; a1=mixerl,d0=l.l
 tst.w d6    ;
 beq alookforzeromixer2
 tst.w d5
 beq alookforzeromixer1
alookfornormalmixer:
 cmp.w d0,d1
 beq.s alfnmvol
 cmp.w d2,d3
 beq.s alfnmper
alfnmpervol:
 cmp.w d1,d0
 bls.s alfnmpervol2
alfnmpervol1:
  cmp.w d2,d3
  bhi.s alfnmpervol1ok
alfnmpervol1xok:
   not.w d7
   lea.l avolper(PC),a0
   lea.l avolperl(PC),a1
   lea.l avolpergetfirst(PC),a2
   moveq.l #-(avolperl-avolper),d0
   rts
alfnmpervol1ok:
  lea.l apervol(PC),a0
  lea.l apervoll(PC),a1
  lea.l apervolgetfirst(PC),a2
  moveq.l #apervoll-apervol,d0
  rts
alfnmpervol2:
  cmp.w d2,d3
  bhi.s alfnmpervol2ok
alfnmpervol2xok:
   not.w d7
   lea.l apervol(PC),a0
   lea.l apervoll(PC),a1
   lea.l apervolgetfirst(PC),a2
   moveq.l #apervoll-apervol,d0
   rts
alfnmpervol2ok:
  lea.l avolper(PC),a0
  lea.l avolperl(PC),a1
  lea.l avolpergetfirst(PC),a2
  moveq.l #-(avolperl-avolper),d0
  rts
alfnmvol:
 cmp.w d2,d3
 beq.s alfnmsimpleok
 bhi.s alfnmvolok
  not.w d7
alfnmvolok:
 lea.l avolvol(PC),a0
 lea.l avolvoll(PC),a1
 lea.l avolvolgetfirst(PC),a2
 moveq.l #0,d0
 rts
alfnmper:
 cmp.w d1,d0
 bhi.s alfnmperok
  not.w d7
alfnmperok:
 lea.l aperper(PC),a0
 lea.l aperperl(PC),a1
 lea.l aperpergetfirst(PC),a2
 moveq.l #aperperl-aperper,d0
 rts
alfnmsimpleok:
 lea.l asimple(PC),a0
 lea.l asimplel(PC),a1
 lea.l asimplegetfirst(PC),a2
 moveq.l #0,d0
 rts

alookforzeromixer2:  ; s2=0
 tst.w d5
 beq asimpleclearmixer
acheckfzm:
 cmp.w d1,d0
 bhi.s alfzmper1
alfzmvol1:
  cmp.w d2,d3
  bls.s alfzmsimpleok
alfzmvolok:
   lea.l aLzerovol(PC),a0
   lea.l aLzerovoll(PC),a1
   lea.l aLzerovolgetfirst(PC),a2
   moveq.l #0,d0
   rts
alfzmper1:
  cmp.w d2,d3
  bls.s alfzmperok
alfzmpervolok:
   lea.l aLzeropervol(PC),a0
   lea.l aLzeropervoll(PC),a1
   lea.l aLzeropervolgetfirst(PC),a2
   moveq.l #aLzeropervoll-aLzeropervol,d0
   rts
alfzmperok:
  lea.l aLzeroper(PC),a0
  lea.l aLzeroperl(PC),a1
  lea.l aLzeropergetfirst(PC),a2
  moveq.l #aLzeroperl-aLzeroper,d0
  rts
alfzmsimpleok:
 lea.l aLzerosimple(PC),a0
 lea.l aLzerosimplel(PC),a1
 lea.l aLzerosimplegetfirst(PC),a2
 moveq.l #0,d0
 rts
alookforzeromixer1:
 not.w d7
 exg d0,d1
 exg d2,d3
 bra acheckfzm

asimpleclearmixer:
 lea.l asimpleclear(PC),a0
 lea.l asimpleclearl(PC),a1
 lea.l asimplecleargetfirst(PC),a2
 moveq.l #0,d0
 rts


;o  equ  0      ; offset volumetable

;c  equr d0     ; counter 
;s1 equr a0     ; source inst 1
;s2 equr a1     ; source inst 2
;t1 equr a6     ; mixbuffer
;vt equr a5     ; volumetable , neu:TRASH
;u  equr d7     ; TRASH (zero !)
u1 equr d1
;vtx equr d5     ; volumetable
rca equr d6    ;-> Mix-calculation registers
rc equr d4     ; -/

 rts
 nop
asimplegetfirst:
 rts
 dc.w 0,0,1,1
 dc.w 64*4,64*4,0
 dc.l 0
asimple:   ; per=per, vol=vol
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
asimplel:
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,u ; 1
  add.b (s2)+,u
  move.b u,(t1)+
 dbra c,asimple
 rts

 subq.w #1,s1
 rts
aperpergetfirst:
 move.b (s1)+,u
 rts
 dc.w 0,0,1,2
 dc.w 0,64*4,aperperl-aperper
 dc.l 0
aperper:      ; per<>per, vol=vol        s1=per
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l1
    move.b (s1)+,u
.l1:
aperperl:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l2
    move.b (s1)+,u
.l2:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l3
    move.b (s1)+,u
.l3:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l4
    move.b (s1)+,u
.l4:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l5
    move.b (s1)+,u
.l5:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l6
    move.b (s1)+,u
.l6:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l7
    move.b (s1)+,u
.l7:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l8
    move.b (s1)+,u
.l8:
 dbra c,aperper
 rts

 rts
 nop
avolvolgetfirst:
 rts
 dc.w 0,0,1,3
 dc.w 64*4,64*4,0
 dc.l 0
avolvol:      ; per=per, vol<>vol        s1=vol
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
avolvoll:
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),u
  add.b (s2)+,u
  move.b u,(t1)+
 dbra c,avolvol
 rts

 subq.w #1,s1
 rts
apervolgetfirst:
 move.b (s1)+,vtx
 move.l vtx,vt
 move.b (vt),u
 rts
 dc.w 0,0,1,4
 dc.w 0,64*4,apervoll-apervol
 dc.l 0
apervol:      ; per<>per, vol<>vol       s1=vol+per
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l1
     move.b (s1)+,vtx
     move.l vtx,vt
     move.b (vt),u
.l1:
apervoll:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l2
     move.b (s1)+,vtx
     move.l vtx,vt
     move.b (vt),u
.l2:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l3
     move.b (s1)+,vtx
     move.l vtx,vt
     move.b (vt),u
.l3:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l4
     move.b (s1)+,vtx
     move.l vtx,vt
     move.b (vt),u
.l4:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l5
     move.b (s1)+,vtx
     move.l vtx,vt
     move.b (vt),u
.l5:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l6
     move.b (s1)+,vtx
     move.l vtx,vt
     move.b (vt),u
.l6:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l7
     move.b (s1)+,vtx
     move.l vtx,vt
     move.b (vt),u
.l7:
  move.b (s2)+,u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l8
     move.b (s1)+,vtx
     move.l vtx,vt
     move.b (vt),u
.l8:
 dbra c,apervol
 rts

 subq.w #1,s2
 rts
avolpergetfirst:
 move.b (s2)+,u
 rts
 dc.w 0,0,1,5
 dc.w 64*4,0,-(avolperl-avolper)
 dc.l 0
avolper:      ; per<>per, vol<>vol  s1=vol s2=per
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l1
     move.b (s2)+,u
.l1:
avolperl:
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l2
     move.b (s2)+,u
.l2:
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l3
     move.b (s2)+,u
.l3:
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l4
     move.b (s2)+,u
.l4:
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l5
     move.b (s2)+,u
.l5:
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l6
     move.b (s2)+,u
.l6:
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l7
     move.b (s2)+,u
.l7:
  move.b (s1)+,vtx  ; 1
  move.l vtx,vt
  move.b (vt),u1
  add.b u,u1
  move.b u1,(t1)+
  add.w rca,rc
  bcc.s .l8
     move.b (s2)+,u
.l8:
 dbra c,avolper
 rts

 subq.w #1,s1
 rts
aLzeropervolgetfirst:
 move.b (s1)+,vtx
 move.l vtx,vt
 move.b (vt),u
 rts
 dc.w 0,0,-1,6
 dc.w 0,0,aLzeropervoll-aLzeropervol
 dc.l 0
aLzeropervol:    ; one=zero!             s1=per+vol s2=0, s1=low
  move.b u,(t1)+
  add.w rca,rc
  bcc.s .l1
    move.b (s1)+,vtx
    move.l vtx,vt
    move.b (vt),u
.l1:
aLzeropervoll:
  move.b u,(t1)+
  add.w rca,rc
  bcc.s .l2
    move.b (s1)+,vtx
    move.l vtx,vt
    move.b (vt),u
.l2:
  move.b u,(t1)+
  add.w rca,rc
  bcc.s .l3
    move.b (s1)+,vtx
    move.l vtx,vt
    move.b (vt),u
.l3:
  move.b u,(t1)+
  add.w rca,rc
  bcc.s .l4
    move.b (s1)+,vtx
    move.l vtx,vt
    move.b (vt),u
.l4:
  move.b u,(t1)+
  add.w rca,rc
  bcc.s .l5
    move.b (s1)+,vtx
    move.l vtx,vt
    move.b (vt),u
.l5:
  move.b u,(t1)+
  add.w rca,rc
  bcc.s .l6
    move.b (s1)+,vtx
    move.l vtx,vt
    move.b (vt),u
.l6:
  move.b u,(t1)+
  add.w rca,rc
  bcc.s .l7
    move.b (s1)+,vtx
    move.l vtx,vt
    move.b (vt),u
.l7:
  move.b u,(t1)+
  add.w rca,rc
  bcc.s .l8
    move.b (s1)+,vtx
    move.l vtx,vt
    move.b (vt),u
.l8:
 dbra c,aLzeropervol
 rts

 subq.w #1,s1
 rts
aLzeropergetfirst:
 move.b (s1)+,u
 rts
 dc.w 0,0,-1,7
 dc.w 0,0,aLzeroperl-aLzeroper
 dc.l 0
aLzeroper:    ; one=zero!   per          s1=per   s2=0     s1=low
  move.b u,(t1)+  ; 1
  add.w rca,rc
  bcc.s .l1
    move.b (s1)+,u
.l1:
aLzeroperl:
  move.b u,(t1)+  ; 1
  add.w rca,rc
  bcc.s .l2
    move.b (s1)+,u
.l2:
  move.b u,(t1)+  ; 1
  add.w rca,rc
  bcc.s .l3
    move.b (s1)+,u
.l3:
  move.b u,(t1)+  ; 1
  add.w rca,rc
  bcc.s .l4
    move.b (s1)+,u
.l4:
  move.b u,(t1)+  ; 1
  add.w rca,rc
  bcc.s .l5
    move.b (s1)+,u
.l5:
  move.b u,(t1)+  ; 1
  add.w rca,rc
  bcc.s .l6
    move.b (s1)+,u
.l6:
  move.b u,(t1)+  ; 1
  add.w rca,rc
  bcc.s .l7
    move.b (s1)+,u
.l7:
  move.b u,(t1)+  ; 1
  add.w rca,rc
  bcc.s .l8
    move.b (s1)+,u
.l8:
 dbra c,aLzeroper
 rts

 rts
 nop
aLzerovolgetfirst:
 rts
 dc.w 0,0,-1,8
 dc.w 64*4,0,0
 dc.l 0
aLzerovol:    ; one=zero!   vol          s1=vol   s2=0  s1=low
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
aLzerovoll:
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
  move.b (s1)+,vtx   ; 1
  move.l vtx,vt
  move.b (vt),(t1)+
 dbra c,aLzerovol
 rts

 rts
 nop
aLzerosimplegetfirst:
 rts
 dc.w 0,0,1,15
 dc.w 64*4,0,0
 dc.l 0
aLzerosimple:   ;           s2=zero, s1=low
  move.b (s1)+,(t1)+ ; 1
aLzerosimplel:
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
  move.b (s1)+,(t1)+ ; 1
 dbra c,aLzerosimple
 rts

 rts
 nop
asimplecleargetfirst:
 moveq.l #0,u
 rts
 dc.w 0,0,1,14
 dc.w 0,0,0
 dc.l 0
asimpleclear:
  move.b u,(t1)+ ; 1
asimpleclearl:
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
  move.b u,(t1)+ ; 1
 dbra c,asimpleclear
 rts

rt_endskip:

 SECTION vectors,BSS

FileName:  ds.b MAX_NAME_LENGTH

auddevio:      ds.w 34
audreply:      ds.w 17
mymacrovecs: ds.l 999 ; optional !

* These 999 long's are just my macro-vectors. they can also be placed
* somewhere in memory, whereas you must replace all the "lea.l mymacro..."
* by "move.l mymac..." and set the pointer to the vecs in here.
*

 END
