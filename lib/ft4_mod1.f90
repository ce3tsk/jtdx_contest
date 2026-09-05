module ft4_mod1

  real*4 dd4(73728)
!$omp threadprivate(dd4)
  ! CE3TSK: dd4 is the thread's working copy of the period - each slice subtracts into its own.
  ! The samples arrive on the master thread (jt9.f90 / jt9a.f90), so the pristine period is kept
  ! here, shared and read only, and every slice starts from it: the same shape as FT8's
  ! dd8/dd8orig/dd8delta.
  !
  ! The deltas are kept even though nothing in FT4 consumes the band today. An FT4 TX background
  ! is planned, and that is precisely what needs the merged band - FT8 builds it as
  ! dd8merged = dd8orig + sum of the slice deltas, summed **in slice order** so the result does
  ! not depend on which thread ran which slice. Collecting them now keeps the two decoders the
  ! same shape and saves retrofitting the merge later; the cost is one buffer per slice.
  real, allocatable :: dd4orig(:), dd4delta(:,:)
  ! CE3TSK: subtractft4's work space. It used to be common/heap8/ - one copy for the whole
  ! program, which two slices subtracting at once would overwrite. Making the common block
  ! threadprivate fixed the race but put 2.5 MB per thread into the static TLS block, and glibc
  ! carves that block out of the same 8 MB mapping as the thread's stack: every worker thread
  ! then died unless the caller raised OMP_STACKSIZE. As allocatables only the descriptors sit
  ! in TLS and the buffers live on the heap, allocated once per thread on first use.
  complex, allocatable :: cref4(:), camp4(:), cfilt4(:)
  real, allocatable :: xjunk4(:)
!$omp threadprivate(cref4,camp4,cfilt4,xjunk4)
  ! The low-pass filter is the same for every thread and is only read after it is built, so it
  ! stays shared - one copy, built once under a critical section.
  complex, allocatable :: cwsub4(:)
  logical :: cwsub4_ready=.false.
  logical :: lcollectdelta4=.false.   ! set when more than one slice runs
  ! CE3TSK: per-slice accumulation. Summed in slice order after the loop, never with atomics -
  ! avexdt is read back by the next period, and a float sum in thread-completion order would
  ! differ run to run (FT4_THREADING_PLAN.md, "the determinism contract").
  integer, parameter :: NFT4SLICEMAX=24
  ! CE3TSK: what this period has already printed. Each slice keeps its own dupe list inside
  ! decode(), which cannot see the other slices - and the offset pass decodes the whole band a
  ! second time on shifted boundaries, so without this every message would print twice. Keyed on
  ! the message text: it carries the callsigns, so two stations cannot collide.
  integer, parameter :: NPRINT4MAX=800
  character(len=37) :: msgseen4(NPRINT4MAX)
  integer :: nseen4=0
  real(8) :: sumxdt4(NFT4SLICEMAX)=0.d0
  integer :: ndecd4(NFT4SLICEMAX)=0, ncand4(NFT4SLICEMAX)=0
  real(8) :: t4sync_s(NFT4SLICEMAX)=0.d0, t4down_s(NFT4SLICEMAX)=0.d0, t4bp_s(NFT4SLICEMAX)=0.d0
  real(8) :: t4osd_s(NFT4SLICEMAX)=0.d0, t4sub_s(NFT4SLICEMAX)=0.d0, t4cand_s(NFT4SLICEMAX)=0.d0
  real(8) :: t4bits_s(NFT4SLICEMAX)=0.d0   ! CE3TSK: the bit-metrics timer, per slice like the rest
  integer :: n4sync_s(NFT4SLICEMAX)=0, n4bp_s(NFT4SLICEMAX)=0, n4osd_s(NFT4SLICEMAX)=0
  logical(1) llagcc,lfilter,lhidetest,lhidetelemetry
  integer nFT4decd,nfafilt,nfbfilt

! CE3TSK: the FT4 hint memory. JTDX's FT4 decoder had no hint pass at all (stophint there only
! trims the AP passes); FT8's hint decoder re-tries every message of the previous same-parity
! period on a candidate at the same frequency and DT, and its four-period memory (P11) was
! worth +6 % on the air. For FT4 the same idea is built on the decoder's own a-priori path: a
! stored message that matches a candidate within 3 Hz and 0.1 s becomes one extra pass with
! all 77 message bits fixed, exactly how the RR73 AP type 6 already works, so the acceptance
! criterion is the one JTDX ships. ft4cur holds this period's decodes; ft4even/ft4odd(:,k) the
! decodes of the k-th previous period of the same parity (a 7.5 s period's parity is its index
! in the minute: 0, 15, 30, 45 s even; 7.5, 22.5, 37.5, 52.5 s odd). decoder.f90 rotates them
! at the start of every period and empties them on a band or mode change.
! JTDX_FT4_HINT_DEPTH=0..8 overrides the depth (0 = no hint pass, JTDX's behaviour).
  integer, parameter :: NHINT4MAX=8, NHINT4LIST=130
  type ft4hint_struct
    real freq
    real dt
    logical lstate
    character*37 msg
  end type ft4hint_struct
  type(ft4hint_struct) :: ft4cur(NHINT4LIST)
  type(ft4hint_struct) :: ft4even(NHINT4LIST,NHINT4MAX),ft4odd(NHINT4LIST,NHINT4MAX)
  integer :: nft4cur=0,nft4parity=0,nft4hintdepth=4,nlastnutc4=-1
! CE3TSK: the hints this slice has already spent. Consumption used to mark the shared list
! straight away, so whether a slice found a hint depended on whether another slice had got
! there first. The mask is per thread; the real retirement happens in the merge, in slice order.
  logical :: ft4used(NHINT4LIST,NHINT4MAX)=.false.
!$omp threadprivate(ft4used)
! CE3TSK item 75: the QSO-side memory FT8 keeps beside its hint lists (ft8_mod1: calldteven/odd,
! lastrxmsg) and the virtual candidate they feed. Per parity, the DT at which each station was
! last heard (newest first, 150 deep); and the last message addressed to me by the DX call, with
! its DT. Once per period ft4qso_seed derives the DT to try at the QSO frequency - the partner's
! - and decode() appends one virtual candidate there when a QSO is in progress and no candidate
! of its own would carry the QSO's a-priori passes. The sensitivity (FT8's "QSO RX freq
! sensitivity"): 0 off, 1 the last RX message's DT only, 2 also the per-call list, 3 also a
! wider fine-sync window.
  type ft4calldt_struct
    real dt
    character*12 call2
  end type ft4calldt_struct
  type(ft4calldt_struct) :: calldt4even(150),calldt4odd(150)
  type ft4lastrx_struct
    real xdt
    logical lstate
    character*37 msg
  end type ft4lastrx_struct
  type(ft4lastrx_struct) :: lastrx4
  integer :: nft4rxfsens=2,nft4virtsrc=0   ! the level in force this phase; the DT's source (1 last message, 2 the call list)
  real :: xdtvirt=-99.0                    ! the virtual candidate's DT this period, -99 = none
! CE3TSK: a slice's decodes, held until the whole loop is done (FT4_THREADING_PLAN.md, "the
! emission order"). Emitting from inside the parallel region made the period's answer depend on
! which thread arrived first: a signal on a slice boundary is decoded by both neighbours at
! slightly different frequency and DT, the period-wide dedupe kept whichever came first, and
! that copy's coordinates seeded the hint memory - so a later period saw a different hint and
! the difference cascaded into whole messages. Measured on 2026-08-30: three runs of the
! 240-period hour at 12 threads with 3 members gave three different message sets. Collected per
! slice and merged in slice order, nothing depends on the schedule.
  integer, parameter :: NDEC4MAX=200
  type ft4res_struct
    character(len=37) :: msg=''
    character(len=26) :: m26a='', m26b=''
    character(len=1) :: srv=''
    integer :: nsnr=0, ih=0, kh=0
    real :: xdt=0., f1=0.
    logical :: lemit=.false., ltwo=.false.
  end type ft4res_struct
  type(ft4res_struct) :: ft4res(NDEC4MAX,NFT4SLICEMAX)
  integer :: nft4res(NFT4SLICEMAX)=0
! CE3TSK: the candidate cap. JTDX stopped getcandidates4 at its 100th peak - in frequency
! order, so once a band has more peaks the top of the passband is never decoded at all (the
! FT8 cap of 450 was raised to 2000 for the same reason). An FT4 signal is ~83 Hz wide against
! FT8's ~50 Hz, so a 3 kHz band holds fewer of them and fewer peaks: the WW Digi FT4 hour
! measured 41-97 candidates a period (median 70) against 180-300 for an FT8 hour - the cap
! of 100 was one busy period from binding. Default 400 (nothing to pay when it does not
! bind); NCAND4MAX sizes the arrays; JTDX_FT4_MAXCAND=1..NCAND4MAX overrides (100 = JTDX).
  integer, parameter :: NCAND4MAX=1000
  integer :: nft4maxcand=400
  integer :: nft4cand=0   ! CE3TSK: the period's candidate count (first pass), for <DecodeFinished><ncand>
! CE3TSK: wall-clock accumulators of the decode stages, printed by decoder.f90 under JTDX_FT4_TIMING=1
  real(8) :: t4sync=0.d0,t4bits=0.d0,t4bp=0.d0,t4osd=0.d0,t4sub=0.d0,t4cand=0.d0,t4down=0.d0
  integer :: n4sync=0,n4bp=0,n4osd=0
! CE3TSK: experiment hooks for the "more passes" question (decoder.f90 reads the env once per
! decode; the defaults are JTDX's values, except the pass count - item 66 made it 4): OSD depth, candidate sync threshold, the sync-quality
! gate, the number of subtraction passes, BP iterations, the coarse frequency step
  integer :: nft4osddeep=3,nft4syncqual=20,nft4nsp=4,nft4bpiter=40,nft4idfstp=3   ! CE3TSK item 66: nsp 3 -> 4 (crowded-band +5, recorded hours identical)
  integer :: nft4dt2=1   ! CE3TSK: the second near-DT sync peak attempt (item 57); JTDX_FT4_DT2=0 off
! CE3TSK: the FT4 TX background (item 59, the FT8 pipeline's shape carried over). nbg4run as
! FT8's nbgrun: 0 = the RX phase, 1 = background under the GUI's .lock/.bgabort rules,
! 2 = background in file mode with no clock. nft4ensfrom lets the background's single unit run
! the members the RX phase did not (imemb = isweep-2+nft4ensfrom-1 in ft4b).
  integer :: nbg4run=0, nft4ensfrom=1
  real, allocatable :: dd4prist(:)
  real :: ft4syncmin=1.2
  integer :: nft4osdbase=3,nft4altbase=0   ! CE3TSK item 69: the env hooks' values, before either phase's own switch is applied
  integer :: nft4syncqbase=20   ! CE3TSK item 73: the same for the sync-quality gate and the sync minimum
  real :: ft4syncminbase=1.2
  ! CE3TSK item 80: FT4's RX budget auto - FT8's P8 in FT4's shape. FT4's members are extra sweeps
  ! per candidate inside the slice loop, not separable units, so the count is decided BEFORE the
  ! decode from the learned cost of the whole RX phase at each member count: blend 50/50 on a run,
  ! 10 % decay on a skip, the unlearned counts seeded from a measured one by the built-in ratio
  ! (cost_learn/cost_decay's rules, decoder.f90). nft4rxrun is the count the RX phase actually ran,
  ! which the background phase continues from.
  real :: ft4rxcost(0:6)=0.
  integer :: nft4rxrun=0
  integer :: nft4alt=0,nft4ens=0   ! CE3TSK: the alternate-window residual pass, and the number of
                                   ! ensemble members (FT8 counts members too - FT8EnsembleEffort,
                                   ! actionFT8Ensemble1..5). Member 1 is the dither the FT4 side
                                   ! started with, 2..6 add the frequency and delay kinds.
  real :: ft4ensfreq=0.6   ! CE3TSK: how far the ensemble's frequency members shift, Hz. 0.6 was
                           ! taken from FT8, whose channel is ~50 Hz wide with 6.25 Hz tone spacing;
                           ! FT4 is ~90 Hz with 20.8 Hz spacing, so the right value here is its own
                           ! question - JTDX_FT4_ENSFREQ sweeps it (DECODER_IMPROVEMENTS item 49)
  real :: ft4falsegate=-17.5   ! CE3TSK: below this SNR a plain decode goes through chkfalse8 as FT8's do below -20.5 (JTDX_FT4_FALSEGATE)

  ! CE3TSK: the hint lists are written from inside the candidate loop (ft4hint_store) and an
  ! entry is retired by ft4hint_consume, so under the slice loop they are shared mutable state.
  ! Guarded by a named critical in the callers; ft4hint_find only reads, but it reads a list
  ! another slice may be appending to, so it takes the same critical.
contains

  subroutine ft4hint_clear()
    ft4cur%lstate=.false.; ft4even%lstate=.false.; ft4odd%lstate=.false.
    nft4cur=0; nlastnutc4=-1
    calldt4even%call2=''; calldt4even%dt=0.; calldt4odd%call2=''; calldt4odd%dt=0.; lastrx4%lstate=.false.   ! item 75
  end subroutine ft4hint_clear

! CE3TSK item 75: every printed decode feeds the per-call DT list of its parity (the sender is
! the second call), and a message addressed to me by the DX call becomes the last RX message
  subroutine ft4qso_store(xdt,msg,mycall,hiscall)
    real, intent(in) :: xdt
    character*37, intent(in) :: msg
    character*12, intent(in) :: mycall,hiscall
    character*12 call2
    call extract_call(msg,call2)
    if(len_trim(call2).gt.2) then
      if(nft4parity.eq.0) then
        calldt4even(150:2:-1)=calldt4even(149:1:-1); calldt4even(1)%call2=call2; calldt4even(1)%dt=xdt
      else
        calldt4odd(150:2:-1)=calldt4odd(149:1:-1); calldt4odd(1)%call2=call2; calldt4odd(1)%dt=xdt
      endif
    endif
    if(len_trim(hiscall).gt.2 .and. index(msg,trim(mycall)//' '//trim(hiscall)).eq.1) then
      lastrx4%msg=msg; lastrx4%xdt=xdt; lastrx4%lstate=.true.
    endif
  end subroutine ft4qso_store

! CE3TSK item 75: once per period, before the slice loop - the DT to try at the QSO frequency.
! FT8's rules (ft8_decode ~120-200, ft8b ~85-135): no DX call or a changed one invalidates the
! last RX message; a QSO in progress means the last TX was one of Tx1-Tx4 and the hint pass is
! not stopped; the DT comes from the last RX message, else (level 2 up) from the per-call list
! of this parity, else (level 2 up) from the hint lists' last message carrying the DX call.
  subroutine ft4qso_seed(mycall,hiscall,nlasttx,lstop)
    character*12, intent(in) :: mycall,hiscall
    integer, intent(in) :: nlasttx
    logical, intent(in) :: lstop
    integer i,k
    xdtvirt=-99.0; nft4virtsrc=0
    if(len_trim(hiscall).le.2) then; lastrx4%lstate=.false.; return; endif
    if(lastrx4%lstate .and. index(lastrx4%msg,trim(hiscall)).le.0) lastrx4%lstate=.false.
    if(nft4rxfsens.le.0 .or. lstop .or. nlasttx.lt.1 .or. nlasttx.gt.4) return
    if(lastrx4%lstate) then; xdtvirt=lastrx4%xdt; nft4virtsrc=1; return; endif
    if(nft4rxfsens.lt.2) return
    do i=1,150
      if(nft4parity.eq.0) then
        if(trim(calldt4even(i)%call2).eq.trim(hiscall)) then; xdtvirt=calldt4even(i)%dt; nft4virtsrc=2; return; endif
      else
        if(trim(calldt4odd(i)%call2).eq.trim(hiscall)) then; xdtvirt=calldt4odd(i)%dt; nft4virtsrc=2; return; endif
      endif
    enddo
    do k=1,min(nft4hintdepth,NHINT4MAX)   ! the hint lists: the last message carrying the DX call
      do i=1,NHINT4LIST
        if(nft4parity.eq.0) then
          if(ft4even(i,k)%lstate .and. index(ft4even(i,k)%msg,' '//trim(hiscall)//' ').gt.1) then
            xdtvirt=ft4even(i,k)%dt; nft4virtsrc=2; return
          endif
        else
          if(ft4odd(i,k)%lstate .and. index(ft4odd(i,k)%msg,' '//trim(hiscall)//' ').gt.1) then
            xdtvirt=ft4odd(i,k)%dt; nft4virtsrc=2; return
          endif
        endif
      enddo
    enddo
  end subroutine ft4qso_seed

! at the start of a period's decode: the finished period's decodes become list 1 of their
! parity, the older lists of that parity slide back one step; a second decode of the same
! period (the Decode button, nagain) rotates nothing
  subroutine ft4hint_rotate(nutc)
    integer, intent(in) :: nutc
    integer nsec,k
    if(nutc.eq.nlastnutc4) return
    if(nlastnutc4.lt.0) then   ! first period since start or a clear: every list explicitly empty
      ft4even%lstate=.false.; ft4odd%lstate=.false.
    else
      if(nft4parity.eq.0) then
        do k=NHINT4MAX,2,-1; ft4even(:,k)=ft4even(:,k-1); enddo
        ft4even(:,1)=ft4cur
      else
        do k=NHINT4MAX,2,-1; ft4odd(:,k)=ft4odd(:,k-1); enddo
        ft4odd(:,1)=ft4cur
      endif
    endif
    nsec=mod(nutc,100)
    nft4parity=mod(nint(real(nsec)/7.5),2)
    nft4cur=0; ft4cur%lstate=.false.; nlastnutc4=nutc
  end subroutine ft4hint_rotate

! every accepted decode of the period, once
  subroutine ft4hint_store(freq,dt,msg)
    real, intent(in) :: freq,dt
    character*37, intent(in) :: msg
    integer i
    do i=1,nft4cur
      if(ft4cur(i)%lstate .and. ft4cur(i)%msg.eq.msg) return
    enddo
    if(nft4cur.ge.NHINT4LIST) return
    nft4cur=nft4cur+1
    ft4cur(nft4cur)%freq=freq; ft4cur(nft4cur)%dt=dt; ft4cur(nft4cur)%msg=msg; ft4cur(nft4cur)%lstate=.true.
  end subroutine ft4hint_store

! the newest same-parity list first, the deeper ones only when the newer had nothing
  subroutine ft4hint_find(freq,dt,msg,ih,kh,lfound)
    real, intent(in) :: freq,dt
    character*37, intent(out) :: msg
    integer, intent(out) :: ih,kh
    logical, intent(out) :: lfound
    integer i,k
    lfound=.false.; ih=0; kh=0; msg=''
    do k=1,min(nft4hintdepth,NHINT4MAX)
      do i=1,NHINT4LIST
        if(nft4parity.eq.0) then
          if(.not.ft4even(i,k)%lstate) cycle
          if(ft4used(i,k)) cycle   ! CE3TSK: this slice has spent it already
          if(abs(ft4even(i,k)%freq-freq).lt.3.0 .and. abs(ft4even(i,k)%dt-dt).lt.0.1) then
            msg=ft4even(i,k)%msg; ih=i; kh=k; lfound=.true.; return
          endif
        else
          if(.not.ft4odd(i,k)%lstate) cycle
          if(ft4used(i,k)) cycle   ! CE3TSK: this slice has spent it already
          if(abs(ft4odd(i,k)%freq-freq).lt.3.0 .and. abs(ft4odd(i,k)%dt-dt).lt.0.1) then
            msg=ft4odd(i,k)%msg; ih=i; kh=k; lfound=.true.; return
          endif
        endif
      enddo
    enddo
  end subroutine ft4hint_find

! is any stored same-parity message within reach of this frequency? - the cheap gate on the
! hint sweep, so a candidate nothing was ever heard near does not pay for a second sync search
  logical function ft4hint_near(freq)
    real, intent(in) :: freq
    integer i,k
    ! CE3TSK: ft4used as well as lstate. Consumption used to clear lstate the moment a hint was
    ! spent, so this returned false straight away and the candidate was skipped; now that the
    ! retirement waits for the merge, the spent-hint mask has to be honoured here too, or the
    ! candidate re-enters the hint sweep only for ft4hint_find to come back empty.
    ft4hint_near=.false.
    do k=1,min(nft4hintdepth,NHINT4MAX)
      do i=1,NHINT4LIST
        if(ft4used(i,k)) cycle
        if(nft4parity.eq.0) then
          if(ft4even(i,k)%lstate .and. abs(ft4even(i,k)%freq-freq).lt.20.0) then; ft4hint_near=.true.; return; endif
        else
          if(ft4odd(i,k)%lstate .and. abs(ft4odd(i,k)%freq-freq).lt.20.0) then; ft4hint_near=.true.; return; endif
        endif
      enddo
    enddo
  end function ft4hint_near

! a hint that decoded is retired from its list; the decode itself is stored afresh
  subroutine ft4hint_consume(ih,kh)
    integer, intent(in) :: ih,kh
    if(ih.lt.1 .or. kh.lt.1) return
    if(nft4parity.eq.0) then; ft4even(ih,kh)%lstate=.false.; else; ft4odd(ih,kh)%lstate=.false.; endif
  end subroutine ft4hint_consume

end module ft4_mod1
