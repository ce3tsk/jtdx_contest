module ft8_mod1

  parameter (NPS=180000,NFR=151680,NFILT1=4000,NFILT2=3400,numcqsig=20,numdeccq=40,nummycsig=5,numdecmyc=25,nmaxthreads=48) !NFRAME=1920*79
  integer, parameter :: NSLICE8MAX=24, NDEC8MAX=200
  real*4 dd8(nps)
! CE3TSK: one audio buffer per decoder thread - decoder.f90 copies it in at each parallel region
!$omp threadprivate(dd8)
  real endcorr(NFILT1/2+1),endcorrswl(NFILT2/2+1)
  complex cw(nps),csync(0:6,32),csynce(0:18,32),csynccq(0:7,32),ctwkw(11,32), &
          ctwkn(11,32),ctwk256(256)
! CE3TSK: the FT8SD hint decoder's working tables are built by tonesd for the candidate's own
! stored message, per hint-matched candidate, INSIDE the parallel loop - as one shared set they
! were overwritten by whichever slice ran tonesd last, so a hint decode could score against
! another slice's message ("memory corruption?" in ft8mf1/ft8mfcq was this race). One set per
! slice; the serial tone8/cwfilter families (csynce, itone56, idtone25 rows 2+) stay shared.
  complex csyncsd(0:18,32,NSLICE8MAX),csyncsdcq(0:57,32,NSLICE8MAX)
  character*37 allmessages(200),msgsd76(76,NSLICE8MAX),msg(56),msgroot,msgincall(8*nmaxthreads)
  character lasthcall*12,mycall12_0*12,mycall12_00*12,hiscall12_0*12,hisgrid4*4
  character(len=12) :: mycall,hiscall,mybcall,hisbcall
  real allfreq(200),windowc1(0:54),windowx(0:200),pivalue,facx,twopi,facc1,dt,sumxdtt(24),avexdt, &
       xdtincall(8*nmaxthreads)
  integer itone76(76,79,NSLICE8MAX),idtone76(76,58,NSLICE8MAX)   ! CE3TSK: per slice, see above
  integer itone56(56,79),idtone56(56,58),idtone25(25,58),allsnrs(200),apsym(58),     &
          idtonemyc(58),mcq(29),mrrr(19),m73(19),mrr73(19),naptypes(0:5,27),icos7(0:6),graymap(0:7),nappasses(0:5), &
          nmsg,ndecodes,nlasttx,mycalllen1,msgrootlen,nfawide,nfbwide,nhaptypes(0:5,27),apsymsp(66),                &
          apsymdxns1(58),apsymdxnsrrr(77),ndxnsaptypes(0:5,27),apcqsym(77),apsymdxnsrr73(77),apsymdxns73(77),       &
          nft8cycles,nft8swlcycles,ncandallthr(nmaxthreads),nincallthr(nmaxthreads),idtonecqdxcns(58),              &
          apsymmyns1(29),apsymmyns2(58),apsymmynsrr73(77),apsymmyns73(77),nmycnsaptypes(0:5,27),apsymdxstd(58),     &
          apsymdxnsr73(77),apsymdxns732(77),apsymmynsrrr(77),idtonedxcns73(58),idtonefox73(58),idtonespec(58),nintcount
  integer*1 gen(91,174)
  logical :: lft8deeposd=.false.   ! CE3TSK: OSD order 2 (ndeep=4) for every candidate, from params%lft8deeposd
! CE3TSK: second slicing pass. In pass 1 every thread records what it subtracted from its
! private dd8 (before the pass-4 averaging), the master merges the deltas in thread order
! onto the original audio, and pass 2 runs on the band with everyone's subtractions applied.
  logical :: lcollectdelta=.false.
  logical :: lsecondpass=.false.   ! CE3TSK: set by the master while the second slicing pass runs
  real, allocatable :: dd8orig(:),dd8delta(:,:)
! CE3TSK item 76 (TODO.md 2.1): sync8's pass-1 wide-band surface - the per-bin sync peaks over
! 0-5000 Hz, the baseline they are normalised by - computed ONCE per slicing from the band every
! slice starts from (dd8orig), and read by every slice's pass-1 sync8 instead of each recomputing
! it. Identical by construction; lsync8share is true only inside the driver's slice loop.
  real, allocatable :: red_sh(:),red2_sh(:)
  integer, allocatable :: jpeak_sh(:),jpeak2_sh(:)
  logical(1), allocatable :: redcq_sh(:)
  real :: base_sh=1.0
  logical :: lsync8share=.false.
  ! CE3TSK: the ALLCALL7.TXT lookup is off in the source (chkflscall.f90 measured it throwing
  ! away ~45 real decodes an hour for one or two false ones). JTDX_ALLCALL7_FILTER=1 turns it
  ! on for tests of the rejection paths it gates - the FT4 /R rejection has no other way to
  ! fire (MERGE_ISSUES 5.2). Set once in decoder.f90's hook block, before any thread starts.
  logical :: lallcall7=.false.
  ! CE3TSK 2026-09-05: the source switch itself, here so that cwfilter (the load) and chkflscall
  ! (the lookup) agree on it; .true. restores JTDX's lookup and the load that feeds it.
  logical, parameter :: LALLCALL7_FILTER=.false.
  logical one(0:511,0:8),lqsomsgdcd,first_osd
  logical(1) lapmyc,lagcc,lagccbail,lhound,lenabledxcsearch,lwidedxcsearch,lmultinst,lskiptx1,ltxing
! CE3TSK: the decoder now works in up to nmaxthreads band slices (several per thread, see
! decoder.f90); everything that was per thread is per slice. Incoming-call slots: 8 per slice.
  integer :: maskincallthr(nmaxthreads+1)=(/ ((i-1)*8, i=1,nmaxthreads+1) /)
  integer :: nslicesft8=1
  real*8 :: tsync8=0.d0,tsync8s=0.d0,tsync8l=0.d0   ! CE3TSK: sync8 wall time: total, spectra, lag loops
  ! CE3TSK: the same split by decoding pass, and the call count per pass. Sharing the wide-band
  ! surface between slices is only possible in pass 1 (from pass 2 each slice has subtracted its
  ! own decodes), so the value of that optimisation is pass 1's share - TODO.md item 2.1.
  real*8 :: tsync8p(0:15)=0.d0
  integer :: nsync8p(0:15)=0
  data     mcq/0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0/
  data    mrrr/0,1,1,1,1,1,1,0,1,0,0,1,0,0,1,0,0,0,1/
  data     m73/0,1,1,1,1,1,1,0,1,0,0,1,0,1,0,0,0,0,1/
  data   mrr73/0,1,1,1,1,1,1,0,0,1,1,1,0,1,0,1,0,0,1/
  data   icos7/3,1,4,0,6,5,2/
  data graymap/0,1,3,2,5,6,4,7/
  data mycall12_0/'dummy'/
  data mycall12_00/'dummy'/
  data hiscall12_0/'dummy'/
! Hound OFF, MyCall is standard, DXCall is standard or empty
  data naptypes(0,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,1,1,1,31,31,31,36,36,36,35,35,35/ ! Tx6 CQ
  data naptypes(1,1:27)/3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,31,31,31,36,36,36,35,35,35/ ! Tx1 Grid
  data naptypes(2,1:27)/3,3,3,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,31,31,31,36,36,36,35,35,35/ ! Tx2 Report
  data naptypes(3,1:27)/3,3,3,6,6,6,5,5,5,4,4,4,0,0,0,0,0,0,31,31,31,36,36,36,35,35,35/ ! Tx3 RRreport
  data naptypes(4,1:27)/3,3,3,6,6,6,5,5,5,4,4,4,2,2,2,0,0,0,31,31,31,36,36,36,35,35,35/ ! Tx4 RRR,RR73
  data naptypes(5,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,1,1,1,31,31,31,36,36,36,35,35,35/ ! Tx5 73
! Hound OFF, MyCall is non-standard, DXCall is standard or empty
  data nmycnsaptypes(0,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,40,40,40,1,1,1,31,31,31,36,36,36,35,35,35/             ! Tx6 CQ
  data nmycnsaptypes(1,1:27)/41,41,41,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,31,31,31,36,36,36,35,35,35/             ! Tx1 DXcall MyCall
  data nmycnsaptypes(2,1:27)/41,41,41,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,31,31,31,36,36,36,35,35,35/             ! Tx2 Report
  data nmycnsaptypes(3,1:27)/41,41,41,44,44,44,43,43,43,42,42,42,0,0,0,0,0,0,31,31,31,36,36,36,35,35,35/    ! Tx3 RRreport
  data nmycnsaptypes(4,1:27)/41,41,41,44,44,44,43,43,43,42,42,42,40,40,40,0,0,0,31,31,31,36,36,36,35,35,35/ ! Tx4 RRR,RR73
  data nmycnsaptypes(5,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,40,40,40,1,1,1,31,31,31,36,36,36,35,35,35/             ! Tx5 73
! Hound mode
  data nhaptypes(0,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,111,111,111/       ! Tx6 CQ, possible in idle mode if DXCall is empty
  data nhaptypes(1,1:27)/21,21,21,22,22,22,0,0,0,0,0,0,0,0,0,31,31,31,0,0,0,36,36,36,0,0,0/ ! Tx1 Grid idle mode or transmitting
  data nhaptypes(2,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0/             ! Tx2 none
  data nhaptypes(3,1:27)/21,21,21,22,22,22,23,23,23,24,24,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0/ ! Tx3 RRreport QSO in progress or QSO is finished
  data nhaptypes(4,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0/             ! Tx4 none
  data nhaptypes(5,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0/             ! Tx5 none
!non-standard DXCall
  data ndxnsaptypes(0,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,1,1,1,31,31,31,36,36,36,35,35,35/             ! Tx6 CQ
  data ndxnsaptypes(1,1:27)/11,11,11,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,31,31,31,36,36,36,35,35,35/          ! Tx1 Grid
  data ndxnsaptypes(2,1:27)/11,11,11,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,31,31,31,36,36,36,35,35,35/          ! Tx2 Report
  data ndxnsaptypes(3,1:27)/11,11,11,14,14,14,13,13,13,12,12,12,0,0,0,0,0,0,31,31,31,36,36,36,35,35,35/ ! Tx3 RRreport
  data ndxnsaptypes(4,1:27)/11,11,11,14,14,14,13,13,13,12,12,12,2,2,2,0,0,0,31,31,31,36,36,36,35,35,35/ ! Tx4 RRR,RR73
  data ndxnsaptypes(5,1:27)/0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,1,1,1,31,31,31,36,36,36,35,35,35/             ! Tx5 73
  data nintcount/0/
  data avexdt/0.0/
  data first_osd/.true./
  
  type odd_struct
    real freq
    real dt
    logical lstate
    character*37 msg
  end type odd_struct
  type(odd_struct) odd(130)

  type even_struct
    real freq
    real dt
    logical lstate
    character*37 msg
  end type even_struct
  type(even_struct) even(130)

  type oddcopy_struct
    real freq
    real dt
    logical lstate
    character*37 msg
  end type oddcopy_struct
  type(oddcopy_struct) oddcopy(130)

  type evencopy_struct
    real freq
    real dt
    logical lstate
    character*37 msg
  end type evencopy_struct
  type(evencopy_struct) evencopy(130)
! CE3TSK P11: a deeper hint memory - the message lists of the same parity k periods back,
! k = 2..NHINTMAX, searched only when the newer ones hold nothing for a candidate. JTDX kept
! one (the previous same-parity period): one missed period broke a station's chain of hint
! decodes. Measured on the one-hour on-air set (DECODE_RECIPE_PLAN.md section 14): depth 2
! +3.6 %, 4 +6 %, 8 +7 % decodes, for 1-3 stale hint decodes an hour (the previous message
! on a re-used frequency when the new one shares most of its bits). Default 4 (two minutes);
! JTDX_HINT_DEPTH=1..8 overrides (1 = JTDX's behaviour).
  integer, parameter :: NHINTMAX=8
  type(evencopy_struct) evencopyk(130,2:NHINTMAX)
  type(oddcopy_struct) oddcopyk(130,2:NHINTMAX)
  integer, parameter :: NHINTDEFAULT=4
  integer :: nhintdepth=NHINTDEFAULT
  
  type lastrxmsg_struct
    real xdt
    logical lstate
    character*37 lastmsg
  end type lastrxmsg_struct
  type (lastrxmsg_struct) lastrxmsg(1)
  data lastrxmsg%lstate/.false./

  type callsigndtodd_struct
    real dt
    character*12 call2
  end type callsigndtodd_struct
  type (callsigndtodd_struct) calldtodd(150)

  type callsigndteven_struct
    real dt
    character*12 call2
  end type callsigndteven_struct
  type (callsigndteven_struct) calldteven(150)

! CE3TSK: the FT8 emission merge (FT8_EMISSION_ORDER.md, TODO.md 2.2). With more than one
! thread each slice appends its decodes here - its own column, no lock - and ft8_emit walks the
! slices in slice order after the parallel loop: the duplicate check, the callback, the
! calldt/even/odd bookkeeping and the avexdt feed all happen there, so the period's answer
! cannot depend on which thread finished first. Of two same-text copies the better SNR wins,
! slice order breaking ties. At one thread (lft8buffered false) the old inline path runs
! unchanged, byte for byte - there is no race to fix there.
  type ft8res_struct
    character(len=37) :: msg=''
    character(len=12) :: call2=''
    character(len=1)  :: srv=' '
    integer :: nsnr=0
    integer :: nloc=0      ! 0 none, 1/2 = which even/odd store condition the decode met
    real :: xdt=0., f1=0.
    integer :: ih=0, kh=0   ! the hint entry a ^ decode used - retired in ft8emit, in slice order
    logical(1) :: lemit=.true.   ! .not.lhidemsg - hidden decodes still feed the lists
  end type ft8res_struct
  type(ft8res_struct) :: ft8res(NDEC8MAX,NSLICE8MAX)
  integer :: nft8res(NSLICE8MAX)=0
  logical(1) :: lft8buffered=.false.
! CE3TSK: with the emission buffered a hint decode no longer retires its entry mid-pass -
! hint_consume from inside the parallel region cleared %lstate while other slices were still
! searching the same lists (and a boundary signal matches two slices' candidates within the
! 3 Hz window), so whether a slice saw the entry depended on thread arrival - the same race
! FT4's item 52 removed. The slice marks what it spent in its own mask (one slice must not
! spend the same hint twice) and the retirement happens in ft8emit, in slice order.
  logical(1) :: ft8used(130,NHINTMAX)=.false.
  integer :: ihint8=0, khint8=0   ! the entry the slice's current candidate consumed
!$omp threadprivate(ft8used,ihint8,khint8)
  logical(1) :: ldectr2=.false.   ! JTDX_DEC_TRACE=2, parsed once per period in decoder.f90

  type incall_struct
    real xdt
    character*37 msg
  end type incall_struct
  type (incall_struct) incall(30)

  type evencq_struct
    real freq
    real xdt
    complex cs(0:7,79)
  end type evencq_struct
  type(evencq_struct) evencq(numcqsig,nmaxthreads) ! 24 threads

  type oddcq_struct
    real freq
    real xdt
    complex cs(0:7,79)
  end type oddcq_struct
  type(oddcq_struct) oddcq(numcqsig,nmaxthreads)

  type evenmyc_struct
    real freq
    real xdt
    complex cs(0:7,79)
  end type evenmyc_struct
  type(evenmyc_struct) evenmyc(nummycsig,nmaxthreads)

  type oddmyc_struct
    real freq
    real xdt
    complex cs(0:7,79)
  end type oddmyc_struct
  type(oddmyc_struct) oddmyc(nummycsig,nmaxthreads)

  type evenqso_struct
    real freq
    real xdt
    complex cs(0:7,79)
  end type evenqso_struct
  type(evenqso_struct) evenqso(1,nmaxthreads)

  type oddqso_struct
    real freq
    real xdt
    complex cs(0:7,79)
  end type oddqso_struct
  type(oddqso_struct) oddqso(1,nmaxthreads)

contains
! CE3TSK: a hint decode retires its stored message (copy isdk: 1 = the last same-parity period, k = k periods back)
  subroutine hint_consume(leven,lodd,isd,isdk)
    logical(1), intent(in) :: leven,lodd
    integer, intent(in) :: isd,isdk
    if(leven) then
      if(isdk.eq.1) then; evencopy(isd)%lstate=.false.; else; evencopyk(isd,isdk)%lstate=.false.; endif
    else if(lodd) then
      if(isdk.eq.1) then; oddcopy(isd)%lstate=.false.; else; oddcopyk(isd,isdk)%lstate=.false.; endif
    endif
  end subroutine hint_consume
end module ft8_mod1