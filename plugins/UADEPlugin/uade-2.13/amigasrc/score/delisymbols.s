	incdir include:
	include misc/deliplayer.i
	include	misc/eagleplayer.i


ByteSymbol     macro
               dc.b    '\1'
               dc.b    0
               dc.b    (\2)
               endm

WordSymbol     macro
               dc.b    '\1'
               dc.b    0
               dc.b    ((\2>>8)&$FF)
               dc.b    (\2&$FF)
               endm

WS		macro
             	dc.b    '\1'
              	dc.b    0
               	dc.b    ((\1>>8)&$FF)
               	dc.b    (\1&$FF)
               endm

LongSymbol     macro
               dc.b    '\1'
               dc.b    0
               dc.b    ((\2>>24)&$FF)
               dc.b    ((\2>>16)&$FF)
               dc.b    ((\2>>8)&$FF)
               dc.b    (\2&$FF)
               endm

LS	macro
               dc.b    '\1'
               dc.b    0
               dc.b    ((\1>>24)&$FF)
               dc.b    ((\1>>16)&$FF)
               dc.b    ((\1>>8)&$FF)
               dc.b    (\1&$FF)
	endm


ORedSymbol     macro
               dc.b    '\1'
               dc.b    0
               dc.b    ((\2>>24)&$FF)
               dc.b    ((\2>>16)&$FF)
               dc.b    ((\2>>8)&$FF)
               dc.b    (\2&$FF)
               dc.b    ((\3>>24)&$FF)
               dc.b    ((\3>>16)&$FF)
               dc.b    ((\3>>8)&$FF)
               dc.b    (\3&$FF)
               endm

OREDSYM        equ     1<<7            ;Symbol base using OR'd symbols
SIGNEDSYM      equ     1<<4            ;Symbol base using signed symbols
BYTESYM        equ     1<<0            ;Symbol base using byte symbols
WORDSYM        equ     1<<1            ;Symbol base using word symbols
LONGSYM        equ     BYTESYM!WORDSYM ;Symbol base using long symbols
ENDBASE        equ     $FF             ;Token to end symbol base

Start
	dc.l	name

***
*** SYMBOLS MUST BE IN ORDER OF GROWING IN THE LIST!
*** you must not have something like this:
*** LS SYM1
*** LS SYM2
*** where SYM2 value < SYM1 value
***
	dc.b	LONGSYM

	LS	dtg_AslBase		; librarybase don't CloseLibrary()

	LS	dtg_DOSBase		; librarybase -"-
	LS	dtg_IntuitionBase	; librarybase -"-
	LS	dtg_GfxBase		; librarybase -"-
	LS	dtg_GadToolsBase	; librarybase -"- (NULL for Kick 1.3 and below)
	LS	dtg_ReservedLibraryBase	; reserved for future use

	LS	dtg_DirArrayPtr
	LS	dtg_FileArrayPtr	; Ptr to the filename of the current module
	LS	dtg_PathArrayPtr	; Ptr to PathArray (e.g used in LoadFile())

	LS	dtg_ChkData
	LS	dtg_ChkSize

	LS	dtg_SndNum
	LS	dtg_SndVol
	LS	dtg_SndLBal
	LS	dtg_SndRBal
	LS	dtg_LED
	LS	dtg_Timer

	LS	dtg_GetListData
	LS	dtg_LoadFile
	LS	dtg_CopyDir
	LS	dtg_CopyFile		;
	LS	dtg_CopyString		;

	LS	dtg_AudioAlloc
	LS	dtg_AudioFree

	LS	dtg_StartInt
	LS	dtg_StopInt
	LS	dtg_SongEnd

	LS	dtg_CutSuffix		;

	LS	dtg_SetTimer

	LS	dtg_WaitAudioDMA

	LS	dtg_NotePlayer

	LS	dtg_AllocListData
	LS	dtg_FreeListData

	LS	EPG_SaveMem
	LS	EPG_FileRequest
	LS	EPG_TextRequest
	LS	EPG_LoadExecutable
	LS	EPG_NewLoadFile
	LS	EPG_ScrollText
	LS	EPG_LoadPlConfig
	LS	EPG_SavePlConfig
	LS	EPG_FindTag
	LS	EPG_FindAuthor
	LS	EPG_Hexdez
	LS	EPG_TypeText
	LS	EPG_ModuleChange
	LS	EPG_ModuleRestore
	LS	EPG_FTPRReserved8
	LS	EPG_XPKBase
	LS	EPG_LHBase
	LS	EPG_PPBase
	LS	EPG_DiskFontBase
	LS	EPG_ReqToolsBase
	LS	EPG_ReqBase
	LS	EPG_XFDMasterBase
	LS	EPG_WorkBenchBase
	LS	EPG_RexxSysBase
	LS	EPG_CommoditiesBase
	LS	EPG_IconBase
	LS	EPG_LocaleBase
	LS	EPG_WinHandle
	LS	EPG_TitlePuffer
	LS	EPG_SoundSystemname
	LS	EPG_Songname
	LS	EPG_Reserved2
	LS	EPG_Reserved3
	LS	EPG_PubScreen
	LS	EPG_CiaBBase
	LS	EPG_UPS_Structure
	LS	EPG_ModuleInfoTagList
	LS	EPG_Author
	LS	EPG_Identifier
	LS	EPG_EagleVersion
	LS	EPG_Speed
	LS	EPG_ARGN
	LS	EPG_ARG1
	LS	EPG_ARG2
	LS	EPG_ARG3
	LS	EPG_ARG4
	LS	EPG_ARG5
	LS	EPG_ARG6
	LS	EPG_ARG7
	LS	EPG_ARG8
	LS	EPG_Voices
	LS	EPG_Voice1Vol
	LS	EPG_Voice2Vol
	LS	EPG_Voice3Vol
	LS	EPG_Voice4Vol
	LS	EPG_VoiceTable
	LS	EPG_VoiceTableEntries
	LS	EPG_Unused1
	LS	EPG_SomePrefs
	LS	EPG_Timeout
	LS	EPG_FirstSnd
	LS	EPG_SubSongs
	LS	EPG_MODNr
	LS	EPG_MODS
	LS	EPG_PlayerTagList
	LS	EPG_TextFont
	LS	EPG_Volume
	LS	EPG_Balance
	LS	EPG_LeftBalance
	LS	EPG_RightBalance
	LS	EPG_UnUsed6
	LS	EPG_UnUsed7
	LS	EPG_UnUsed8
	LS	EPG_UnUsed9
	LS	EPG_DefTimer
	LS	EPG_CurrentPosition
	LS	EPG_WORDReserved3
	LS	EPG_WORDReserved4
	LS	EPG_WORDReserved5
	LS	EPG_WORDReserved6
	LS	EPG_WORDReserved7
	LS	EPG_WORDReserved8
	LS	EPG_Dirs
	LS	EPG_LoadedFiles
	LS	EPG_AppPort
	LS	EPG_SampleInfoStructure
	LS	EPG_MinTimeOut
	LS	EPG_CurrentTime
	LS	EPG_Duration
	LS	EPG_FirstUserStruct
	LS	EPG_FirstFileStruct
	LS	EPG_Entries
	LS	EPG_Modulesize
	LS	EPG_Playerlist
	LS	EPG_Enginelist
	LS	EPG_Moduleslist
	LS	EPG_AmplifierList
	LS	EPG_ActiveAmplifier
	LS	EPG_AudioStruct
	LS	EPG_AmplifierTagList
	LS	EPG_ConfigDirArrayPtr
	LS	EPG_PlayerDirArrayPtr
	LS	EPG_EngineDirArrayPtr
	LS	EPG_FirstPlayerStruct
	LS	EPG_ChkSegment
	LS	EPG_EagleplayerDirArrayPtr

	LS	DTP_CustomPlayer
	LS	DTP_RequestDTVersion
	LS	DTP_RequestKickVersion
	LS	DTP_PlayerVersion
	LS	DTP_PlayerName
	LS	DTP_Creator
	LS	DTP_Check1
	LS	DTP_Check2
	LS	DTP_ExtLoad
	LS	DTP_Interrupt
	LS	DTP_Stop
	LS	DTP_Config
	LS	DTP_UserConfig
	LS	DTP_SubSongRange
	LS	DTP_InitPlayer
	LS	DTP_EndPlayer
	LS	DTP_InitSound
	LS	DTP_EndSound
	LS	DTP_StartInt
	LS	DTP_StopInt
	LS	DTP_Volume
	LS	DTP_Balance
	LS	DTP_Faster
	LS	DTP_Slower

	LS	DTP_NextSong
	LS	DTP_PrevSong

	LS	DTP_SubSongTest

	LS	DTP_NewSubSongRange
	LS	DTP_DeliBase
	LS	DTP_Flags
	LS	DTP_CheckLen
	LS	DTP_Description
	LS	DTP_NotePlayer
	LS	DTP_NoteStruct
	LS	DTP_NoteInfo
	LS	DTP_NoteSignal
	LS	DTP_Process

	LS	DTP_ModuleName
	LS	DTP_FormatName
	LS	DTP_AuthorName

	LS	EP_Get_ModuleInfo
	LS	EP_Free_ModuleInfo
	LS	EP_Voices
	LS	EP_SampleInit
	LS	EP_SampleEnd
	LS	EP_Save
	LS	EP_ModuleChange
	LS	EP_ModuleRestore
	LS	EP_StructInit
	LS	EP_StructEnd
	LS	EP_LoadPlConfig
	LS	EP_SavePlConfig
	LS	EP_GetPositionNr
	LS	EP_SetSpeed
	LS	EP_Flags
	LS	EP_KickVersion
	LS	EP_PlayerVersion
	LS	EP_CheckModule
	LS	EP_EjectPlayer
	LS	EP_Date
	LS	EP_Check3
	LS	EP_SaveAsPT
	LS	EP_NewModuleInfo
	LS	EP_FreeExtLoad
	LS	EP_PlaySample
	LS	EP_PatternInit
	LS	EP_PatternEnd
	LS	EP_Check4
	LS	EP_Check5
	LS	EP_Check6
	LS	EP_CreatorLNr
	LS	EP_PlayerNameLNr
	LS	EP_PlayerInfo
	LS	EP_PlaySampleInit
	LS	EP_PlaySampleEnd
	LS	EP_InitAmplifier
	LS	EP_CheckSegment
	LS	EP_Show
	LS	EP_Hide
	LS	EP_LocaleTable
	LS	EP_Helpnodename
	LS	EP_AttnFlags
	LS	EP_EagleBase
	LS	EP_Check7
	LS	EP_Check8
	LS	EP_SetPlayFrequency
	LS	EP_SamplePlayer

	LS	MI_SongName
	LS	MI_AuthorName
	LS	MI_SubSongs
	LS	MI_Pattern
	LS	MI_MaxPattern
	LS	MI_Length
	LS	MI_MaxLength
	LS	MI_Steps
	LS	MI_MaxSteps
	LS	MI_Samples
	LS	MI_MaxSamples
	LS	MI_SynthSamples
	LS	MI_MaxSynthSamples
	LS	MI_Songsize
	LS	MI_SamplesSize
	LS	MI_ChipSize
	LS	MI_OtherSize
	LS	MI_Calcsize
	LS	MI_SpecialInfo
	LS	MI_LoadSize
	LS	MI_Unpacked
	LS	MI_UnPackedSystem
	LS	MI_Prefix
	LS	MI_About
	LS	MI_MaxSubSongs
	LS	MI_Voices
	LS	MI_MaxVoices
	LS	MI_UnPackedSongSize
	LS	MI_Duration
	LS	MI_Soundsystem
	LS	MI_PlayFrequency
	LS	MI_Volumeboost
	LS	MI_Playmode
	LS	MI_ExtraInfo
	LS	MI_InfoFlags

	dc.b	ENDBASE
	
name	dc.b	'Delisymbols',0
	cnop	0,2

end
