  use, intrinsic :: iso_c_binding, only: c_int, c_short, c_float, c_char, c_bool
  include 'constants.f90'

  !
  ! these structures must be kept in sync with ../commons.h
  !
  type, bind(C) :: params_block
!     character(kind=c_char) :: datetime(20)
     character(kind=c_char) :: mycall(12)
     character(kind=c_char) :: mybcall(12)
     character(kind=c_char) :: hiscall(12)
     character(kind=c_char) :: hisbcall(12)
!     character(kind=c_char) :: mygrid(6)
     character(kind=c_char) :: hisgrid(6)
     integer(c_int) :: listutc(10)
     integer(c_int) :: napwid
     integer(c_int) :: nQSOProgress
     integer(c_int) :: nftx
     integer(c_int) :: nutc
     integer(c_int) :: ntrperiod
     integer(c_int) :: nfqso
     integer(c_int) :: npts8
     integer(c_int) :: nfa
     integer(c_int) :: nfsplit
     integer(c_int) :: nfb
     integer(c_int) :: ntol
     integer(c_int) :: kin
     integer(c_int) :: nzhsym
     integer(c_int) :: ndepth
     integer(c_int) :: ncandthin
     integer(c_int) :: ndtcenter
     integer(c_int) :: nft8cycles
     integer(c_int) :: nft8swlcycles
     integer(c_int) :: ntxmode
     integer(c_int) :: nmode
     integer(c_int) :: nlist
     integer(c_int) :: nranera
     integer(c_int) :: ntrials10
     integer(c_int) :: ntrialsrxf10
     integer(c_int) :: naggressive
     integer(c_int) :: nharmonicsdepth
     integer(c_int) :: ntopfreq65
     integer(c_int) :: nprepass
     integer(c_int) :: nsdecatt
     integer(c_int) :: nlasttx
     integer(c_int) :: ndelay
     integer(c_int) :: nmt
     integer(c_int) :: nft8rxfsens
     integer(c_int) :: nft4depth
     integer(c_int) :: nsecbandchanged
     integer(c_int) :: nft8ensemble   ! CE3TSK: ensemble members 0-5 (perturbed re-decodes, ft8ensemble.f90)
     integer(c_int) :: nft8bgeffort   ! CE3TSK: background effort: -1 as the budget allows, 0 off, n units (pipeline ensemble)
     integer(c_int) :: nbgmargin      ! CE3TSK: the background phase stops this many tenths of a second before the next decode
     integer(c_int) :: nbgbudget      ! CE3TSK P7: the background's window from the decode's start, tenths of a second (0: the period)
     integer(c_int) :: nrxbudget      ! CE3TSK P8: the RX phase's budget for 'budget auto' members, tenths of a second (<=0: 27)
     integer(c_int) :: nft8bgcycles   ! CE3TSK: the TX background recipe (PIPELINED_DECODE_PLAN.md): cycles, SWL cycles,
     integer(c_int) :: nft8bgswlcycles !   ensemble members (-1 as the budget allows), RX frequency sensitivity
     integer(c_int) :: nft8bgensemble
     integer(c_int) :: nft8bgrxfsens
     logical(c_bool) :: ndiskdat
     logical(c_bool) :: newdat
     logical(c_bool) :: nagain
     logical(c_bool) :: nagainfil
     logical(c_bool) :: nswl
     logical(c_bool) :: nfilter
     logical(c_bool) :: nstophint
     logical(c_bool) :: nagcc
     logical(c_bool) :: nhint
     logical(c_bool) :: fmaskact
     logical(c_bool) :: showharmonics
     logical(c_bool) :: lft8lowth
     logical(c_bool) :: lft8subpass
     logical(c_bool) :: ltxing
     logical(c_bool) :: lhidetest
     logical(c_bool) :: lhidetelemetry
     logical(c_bool) :: lhideft8dupes
     logical(c_bool) :: lhound
     logical(c_bool) :: lhidehash
     logical(c_bool) :: lcommonft8b
     logical(c_bool) :: lmycallstd
     logical(c_bool) :: lhiscallstd
     logical(c_bool) :: lapmyc
     logical(c_bool) :: lmodechanged
     logical(c_bool) :: lbandchanged
     logical(c_bool) :: lenabledxcsearch
     logical(c_bool) :: lwidedxcsearch
     logical(c_bool) :: lmultinst
     logical(c_bool) :: lskiptx1
     logical(c_bool) :: lforcesync
     logical(c_bool) :: learlystart
     logical(c_bool) :: lft8deeposd   ! CE3TSK: OSD order 2 for every FT8 candidate (weak-signal mode)
     logical(c_bool) :: lft8twopass   ! CE3TSK: second slicing pass with the thread slices offset by half a slice
     logical(c_bool) :: lft8altpass   ! CE3TSK: alternate-approach pass (7 cycles + OSD order 2) on the subtracted band
     logical(c_bool) :: lbgswl        ! CE3TSK: the TX background recipe: SWL mode, OSD order 2, second slicing pass,
     logical(c_bool) :: lbgdeeposd    !   alternate-approach pass, low thresholds, subpass
     logical(c_bool) :: lbgtwopass
     logical(c_bool) :: lbgaltpass
     logical(c_bool) :: lbglowth
     logical(c_bool) :: lbgsubpass
     integer(c_int) :: nft8bgclassic  ! CE3TSK P9: the classic background unit - the plain non-SWL decode with this many cycles, 0 off
     logical(c_bool) :: lft4altpass   ! CE3TSK: FT4 alternate pass on the residual (the other DT search windows)
     logical(c_bool) :: lft4twopass   ! CE3TSK: FT4 second slicing pass
     integer(c_int) :: nft4ensemble   ! CE3TSK: FT4 ensemble members 0-6, as nft8ensemble counts FT8's
     logical(c_bool) :: lft4deeposd   ! CE3TSK: OSD order 2 for every FT4 candidate, as lft8deeposd (item 58)
     integer(c_int) :: nft4bgensemble ! CE3TSK: the FT4 TX background's target member count, 0 off (item 59)
     integer(c_int) :: nft4bgdepth    ! CE3TSK item 69: the FT4 TX background's effort 1-3, 0 = the RX phase's
     logical(c_bool) :: lft4bgdeeposd ! CE3TSK item 69: deep OSD in the FT4 TX background
     logical(c_bool) :: lft4bgaltpass ! CE3TSK item 69: alternate pass in the FT4 TX background
     logical(c_bool) :: lft4bgtwopass ! CE3TSK item 69: second slicing pass in the FT4 TX background
     logical(c_bool) :: lft4bgresidual ! CE3TSK item 72: the residual unit in the FT4 TX background
     integer(c_int) :: nft4sens       ! CE3TSK item 72: FT4 decoder sensitivity, 0 JTDX's thresholds, 1 sync minimum 1.0 + sync quality 16
     integer(c_int) :: nft4bgsens     ! CE3TSK item 73: the same for the FT4 TX background phase
     integer(c_int) :: nft4rxfsens    ! CE3TSK item 75: FT4 QSO RX frequency sensitivity 0-3
     integer(c_int) :: nft4bgrxfsens  ! CE3TSK item 75: the same for the FT4 TX background phase
     integer(c_int) :: ndecreq        ! CE3TSK: the decode request counter (commons.h has the why)
     integer(c_int) :: nft4bgeffort   ! CE3TSK item 78: the FT4 TX background switch, as nft8bgeffort - 0 off, 1 on
     integer(c_int) :: nsftol         ! CE3TSK: SuperFox receive - 0 off, N > 0 on with a sync search range of +/- N Hz (commons.h)
 end type params_block

  type, bind(C) :: dec_data
     real(c_float) :: ss(184,NSMAX)
     real(c_float) :: savg(NSMAX)
     integer(c_short) :: id2(NMAX)
     real(c_float) :: dd2(NMAX)
     type(params_block) :: params
  end type dec_data
  ! for unknown reason values of the variables at beginning of dec_data list are being
  ! not updated while Decode button is pushed manually, for decoding again keep variables at end
  ! of the list
