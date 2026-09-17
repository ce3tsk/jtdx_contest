module ft4_decode

  type :: ft4_decoder
    procedure(ft4_decode_callback), pointer :: callback
  contains
    procedure :: decode
    procedure :: emit => ft4emit   ! CE3TSK: named as FT8's ft8emit, for symmetry
  end type ft4_decoder

  abstract interface
    subroutine ft4_decode_callback (this,snr,dt,freq,decoded,servis4)
      import ft4_decoder
      implicit none
      class(ft4_decoder), intent(inout) :: this
      integer, intent(in) :: snr
      real, intent(in) :: dt
      real, intent(in) :: freq
      character(len=26), intent(in) :: decoded
      character(len=1), intent(in) :: servis4
    end subroutine ft4_decode_callback
  end interface

contains

  subroutine decode(this,callback,nQSOProgress,nfqso,nfa,nfb,ndepth,stophint,swl,nthr,numthreads)
!    use timer_module, only: timer
    use packjt77
    use ft4_mod1, only : nFT4decd,nfafilt,nfbfilt,lfilter,lhidetest,lhidetelemetry
    ! CE3TSK: the slice's own accumulators and its copy of the band (FT4_THREADING_PLAN.md)
    use ft4_mod1, only : dd4,dd4orig,dd4delta,lcollectdelta4,sumxdt4,ndecd4,ncand4
    use ft4_mod1, only : msgseen4,nseen4,NPRINT4MAX   ! CE3TSK: this period's printed messages
    use ft4_mod1, only : ft4res,nft4res,NDEC4MAX,ft4used   ! CE3TSK: the slice's decodes, emitted after the loop
    use ft4_mod1, only : t4sync_s,t4down_s,t4bp_s,t4osd_s,t4sub_s,t4cand_s,t4bits_s,n4sync_s,n4bp_s,n4osd_s
    use ft8_mod1, only : sumxdtt,avexdt,mycall,hiscall
    use ft4_mod1, only : nft4hintdepth,ft4hint_find,ft4hint_store,ft4hint_consume,ft4hint_near   ! CE3TSK: the FT4 hint memory
    use ft4_mod1, only : NCAND4MAX,nft4maxcand,nft4cand   ! CE3TSK: the candidate cap and count
    use ft4_mod1, only : nft4osddeep,nft4syncqual,nft4nsp,nft4bpiter,nft4idfstp,ft4syncmin,nft4alt,nft4ens,ft4falsegate,ft4ensfreq   ! CE3TSK: hooks
    use ft4_mod1, only : xdtvirt   ! CE3TSK item 75
    use ft4_mod1, only : t4sync,t4bits,t4bp,t4osd,t4sub,t4cand,t4down,n4sync,n4bp,n4osd   ! CE3TSK: timing
    use omp_lib, only : omp_get_wtime
    use ft4_sync_mod, only : ft4_sync_twiddle   ! CE3TSK: the hoisted sync twiddle
    include 'ft4/ft4_params.f90'
    class(ft4_decoder), intent(inout) :: this
    procedure(ft4_decode_callback) :: callback
    integer, intent(in) :: nthr,numthreads   ! CE3TSK: this slice's index, and how many slices run
    logical lnewmsg   ! CE3TSK: not yet printed by another slice or by the other slicing pass
    integer iseen,ir
    parameter (NSS=NSPS/NDOWN,NDMAX=NMAX/NDOWN)
    character message*37,msg26*26,msgsent*37,msg37_2*37
    character msgd*37   ! CE3TSK: the stored message a candidate is hinted with
    integer hintbits(2*ND),ih,kh,npasst,isweep,ipass0
    integer*1 hint77(77)
    logical lhint,lhintok
    real xdtc
    real(8) tw0
    logical lviaosd,lft4timing,lswl,lmine
    logical(1) lhash1   ! CE3TSK: first call is a hash - chkfalse8 then checks the second call's grid, as ft8b
    integer nsweeps
    integer(8) :: sdit
    real udit,vdit,gdit
    real(8) sumdit
    integer :: imemb,ikind,imdt,ibm      ! CE3TSK: ensemble member, its kind, its sample delay, the aligned start
    real :: fmoff                        ! CE3TSK: the member's frequency offset, Hz
    character envv*8
    integer lenv,istatv
    character c77*77
    character*37 decodes(100)
    character*12 mycall0,hiscall0,call_a,call_b
    character*4 servis4
    complex cd2(0:NDMAX-1)                  !Complex waveform
    complex cb(0:NDMAX-1)
    complex cd(0:NN*NSS-1)                       !Complex waveform
    complex ctwk(2*NSS),ctwk2(2*NSS,-16:16)
    complex csa(2*NSS),csb(2*NSS),csc(2*NSS),csd(2*NSS)   ! CE3TSK: twiddled sync templates of one frequency step
    real a(5)
    real bitmetrics(2*NN,3)
    real llr(2*ND),llra(2*ND),llrb(2*ND),llrc(2*ND),llrd(2*ND)
    real candidate(2,NCAND4MAX)
    integer nvirt   ! CE3TSK item 75: the virtual candidate's index, 0 when none
    integer apbits(2*ND)
    integer*1 message77(77),rvec(77),apmask(2*ND),cw(2*ND)
    integer*1 hbits(2*NN)
    integer i4tone(103)
    integer nappasses(0:5)    ! # of decoding passes for QSO States 0-5
    integer naptypes(0:5,4)   ! nQSOProgress, decoding pass
    integer mcq(29),mrrr(19),m73(19),mrr73(19)
    logical nohiscall,unpk77_success,first,dobigfft,dosubtract,doosd,badsync,lFreeText,lhidemsg
    logical(1), intent(in) :: stophint,swl
    logical(1) falsedec

    this%callback => callback
    ft4used=.false.   ! CE3TSK: this slice has spent no hint yet
    mycalllen1=len_trim(mycall)+1
    smax=0.; smax1=0.; nd1=0 ! smax init value shall be increased to 1.+ ?

    maxcand=max(1,min(NCAND4MAX,nft4maxcand)); ndecodes=0; decodes=' '; fa=nfa; fb=nfb   ! CE3TSK: was 100
    call get_environment_variable('JTDX_FT4_TIMING',envv,lenv,istatv); lft4timing=(istatv.eq.0 .and. lenv.gt.0)
! ndepth=1: 1 pass, no subtraction
! ndepth=2: 3 passes, bp only
! ndepth=3: 3 passes, bp+osd
    max_iterations=nft4bpiter; syncmin=ft4syncmin; dosubtract=.true.; doosd=.true.; nsp=nft4nsp   ! CE3TSK: 40, 1.2, 3 by default
    if(ndepth.eq.2) doosd=.false.
    if(ndepth.eq.1) then; nsp=1; dosubtract=.false.; doosd=.false.; endif

    do isp = 1,nsp+nft4alt   ! CE3TSK: + one alternate-window pass on the residual when JTDX_FT4_ALT=1
      if(isp.eq.2) then; if(ndecodes.eq.0) exit; nd1=ndecodes
      elseif(isp.eq.3) then; nd2=ndecodes-nd1; if(nd2.eq.0 .and. nft4alt.eq.0) exit
      endif
      lswl=swl; if(isp.gt.nsp) lswl=.not.swl   ! the alternate pass: the other DT search windows

      candidate=0.0
      ncand=0
      tw0=omp_get_wtime()
      call getcandidates4(fa,fb,syncmin,nfqso,maxcand,candidate,ncand)
      t4cand_s(nthr)=t4cand_s(nthr)+omp_get_wtime()-tw0
      ! CE3TSK: accumulated, not assigned - decode() is called once per slice per slicing pass, and
      ! every other per-slice counter adds across both passes. Assigning made the period's <ncand>
      ! report only the offset pass. Zeroed once per period in decoder.f90.
      if(isp.eq.1) ncand4(nthr)=ncand4(nthr)+ncand
      ! CE3TSK item 75: FT8's virtual candidate (sync8 ~318) - one more candidate at the QSO
      ! frequency, in the slice that holds it, on the first subtraction pass, when ft4qso_seed
      ! found a DT to try; ft4b syncs it around that DT instead of searching the three segments
      nvirt=0
      if(isp.eq.1 .and. xdtvirt.gt.-90. .and. nfqso.ge.nfa .and. nfqso.le.nfb .and. ncand.lt.NCAND4MAX) then
        ncand=ncand+1; candidate(1,ncand)=real(nfqso); candidate(2,ncand)=0.; nvirt=ncand
      endif
      dobigfft=.true.
      do icand=1,ncand
        call ft4b(candidate(1,icand),candidate(2,icand),nQSOProgress,nfqso,ndepth,stophint, &
                  swl,lswl,nthr,isp,dobigfft,dosubtract,doosd,max_iterations,decodes,ndecodes, &
                  lft4timing,icand.eq.nvirt)
      enddo    !Candidate list
    enddo       !Subtraction loop

    ! CE3TSK: what this slice subtracted, for a future TX background to merge in slice order

    if(lcollectdelta4) dd4delta(:,nthr)=dd4-dd4orig

    return
  end subroutine decode

! CE3TSK: the period's decodes, merged in slice order and only now emitted. Everything that is
! shared between slices - the printed-message list, the callback, the hint memory - is touched
! here and nowhere else, so the answer cannot depend on which thread finished first. Called
! once per slicing pass; a single-slice period goes through it unchanged, which is why the
! one-thread output is still byte for byte what it was before the threading (1716 lines).
  subroutine ft4emit(this,nsl)
    use ft4_mod1, only : ft4res,nft4res,msgseen4,nseen4,NPRINT4MAX,ft4hint_store,ft4hint_consume
    use ft4_mod1, only : ft4qso_store   ! CE3TSK item 75
    use ft8_mod1, only : mycall,hiscall
    use ft4_mod1, only : NDEC4MAX,NFT4SLICEMAX
    use ft4_mod1, only : lft2   ! CE3TSK: FT2 decodes a stream stretched x2, so its frequencies are halved
    class(ft4_decoder), intent(inout) :: this
    integer, intent(in) :: nsl
    integer k,i,iseen,j,nacc,ka,ia
    integer iacc(2,NDEC4MAX*NFT4SLICEMAX)
    logical lnewmsg,ldup
    character(len=26) msg26
    character(len=1) servis4
    real f1out
! CE3TSK: the hints first, per entry in walk order exactly as before - duplicates included,
! and before any selection, so the hint lists' content and order cannot depend on which copy
! wins below (the store order steers ft4hint_find's first-match in later periods)
    do k=1,nsl
      do i=1,nft4res(k)
        call ft4hint_store(ft4res(i,k)%f1,ft4res(i,k)%xdt,ft4res(i,k)%msg)
        if(ft4res(i,k)%ih.gt.0) call ft4hint_consume(ft4res(i,k)%ih,ft4res(i,k)%kh)
      enddo
    enddo
! CE3TSK: winner selection, in slice order - FT8's ft8emit rule backported (FT4_FT8_PARITY.md):
! of two same-text copies in this batch the better SNR wins, slice order breaking ties, the
! replacement in place so the winner keeps the first copy's position in the emission order. A
! text already in msgseen4 (an earlier slicing pass this period) stays a plain dupe as before -
! it has printed and cannot be bettered. At one slice nothing can collide (the in-pass decodes
! list blocks same-text repeats within a slice), so the one-thread output is untouched.
    nacc=0
    do k=1,nsl
      do i=1,nft4res(k)
        lnewmsg=.true.
        do iseen=1,nseen4
          if(msgseen4(iseen).eq.ft4res(i,k)%msg) then; lnewmsg=.false.; exit; endif
        enddo
        if(.not.lnewmsg) cycle
        ldup=.false.
        do j=1,nacc
          ka=iacc(1,j); ia=iacc(2,j)
          if(ft4res(i,k)%msg.eq.ft4res(ia,ka)%msg) then
            ldup=.true.
            if(ft4res(i,k)%nsnr.gt.ft4res(ia,ka)%nsnr) ft4res(ia,ka)=ft4res(i,k)
            exit
          endif
        enddo
        if(.not.ldup) then; nacc=nacc+1; iacc(1,nacc)=k; iacc(2,nacc)=i; endif
      enddo
    enddo
    do j=1,nacc
      ka=iacc(1,j); ia=iacc(2,j)
      if(nseen4.lt.NPRINT4MAX) then; nseen4=nseen4+1; msgseen4(nseen4)=ft4res(ia,ka)%msg; endif
      if(ft4res(ia,ka)%lemit) then
        call ft4qso_store(ft4res(ia,ka)%xdt,ft4res(ia,ka)%msg,mycall,hiscall)   ! item 75: in emission order
        msg26=ft4res(ia,ka)%m26a; servis4=ft4res(ia,ka)%srv
        f1out=ft4res(ia,ka)%f1; if(lft2) f1out=2.0*f1out   ! CE3TSK: back to the frequency received
        call this%callback(ft4res(ia,ka)%nsnr,ft4res(ia,ka)%xdt,f1out,msg26,servis4)
        if(ft4res(ia,ka)%ltwo) then
          msg26=ft4res(ia,ka)%m26b
          call this%callback(ft4res(ia,ka)%nsnr,ft4res(ia,ka)%xdt,f1out,msg26,servis4)
        endif
      endif
    enddo
    nft4res(1:nsl)=0
  end subroutine ft4emit

end module ft4_decode
