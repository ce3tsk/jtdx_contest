! CE3TSK: the FT4 per-candidate machinery, split out of ft4_decode.f90 on 2026-09-01 so the
! two decoders share the file layout as well as the design (FT4_FT8_PARITY.md: ft8_decode.f90
! drives, ft8b.f90 decodes one candidate - now ft4_decode.f90 drives and this file decodes one
! candidate). Pure relocation: the body of the old candidate loop, the per-thread first-run
! tables and the a-priori bit patterns moved here verbatim; the driver keeps the slice loop,
! the passes, getcandidates4 and emit. One inherited knot was preserved through the split and
! untied on 2026-09-02 (item 62): the /R chkflscall rejection used to RETURN from the whole
! slice decode (skipping the remaining candidates, passes and the delta collect); it now
! rejects the candidate only - a RETURN from here - exactly as ft8b's chkflscall rejections do.
subroutine ft4b(f0,snrc,nQSOProgress,nfqso,ndepth,stophint,swl,lswl,nthr,isp,dobigfft, &
                dosubtract,doosd,max_iterations,decodes,ndecodes,lft4timing,lvirt)
  use packjt77
  use ft4_mod1, only : nFT4decd,nfafilt,nfbfilt,lfilter,lhidetest,lhidetelemetry
  use ft4_mod1, only : dd4,sumxdt4,ndecd4
  use ft4_mod1, only : ft4res,nft4res,NDEC4MAX,ft4used
  use ft4_mod1, only : t4sync_s,t4down_s,t4bp_s,t4osd_s,t4sub_s,t4bits_s,n4sync_s,n4bp_s,n4osd_s
  use ft8_mod1, only : avexdt,mycall,hiscall,mycalllen1
  use ft4_mod1, only : nft4hintdepth,ft4hint_find,ft4hint_near
  use ft4_mod1, only : nft4osddeep,nft4syncqual,nft4idfstp,nft4alt,nft4ens,ft4falsegate,ft4ensfreq,nft4dt2
  use ft4_mod1, only : nbg4run,nft4ensfrom   ! CE3TSK: the TX background (item 59)
  use ft4_mod1, only : xdtvirt,nft4rxfsens   ! CE3TSK item 75: the virtual candidate
  use omp_lib, only : omp_get_wtime
  use ft4_sync_mod, only : ft4_sync_twiddle
  include 'ft4/ft4_params.f90'
  real, intent(in) :: f0,snrc
  integer, intent(in) :: nQSOProgress,nfqso,ndepth,nthr,isp,max_iterations
  logical(1), intent(in) :: stophint,swl
  logical, intent(in) :: lswl,dosubtract,doosd,lft4timing
  logical, intent(in) :: lvirt   ! CE3TSK item 75: the virtual candidate at the QSO frequency - sync around xdtvirt
  integer ibvirt,nwvirt
  logical, intent(inout) :: dobigfft
  character*37 decodes(100)
  integer, intent(inout) :: ndecodes
  parameter (NSS=NSPS/NDOWN,NDMAX=NMAX/NDOWN)
  character message*37,msgsent*37,msg37_2*37
  character msgd*37
  integer hintbits(2*ND),ih,kh,npasst,isweep,ipass0
  integer*1 hint77(77)
  logical lhint,lhintok
  real xdtc
  real(8) tw0
  logical lviaosd,lmine
  logical(1) lhash1
  integer nsweeps
  integer(8) :: sdit
  real udit,vdit,gdit
  real(8) sumdit
  integer :: imemb,ikind,imdt,ibm
  real smax2g
  integer ib2g,idf2g   ! CE3TSK: the coarse scan's runner-up DT peak (item 57)
  integer :: nsyncloc   ! CE3TSK: this candidate's sync4d_tw calls, added to the shared counter once
  real :: fmoff
  character c77*77
  character*12 mycall0,hiscall0,call_a,call_b
  character*4 servis4
  complex cd2(0:NDMAX-1)
  complex cb(0:NDMAX-1)
  complex cd(0:NN*NSS-1)
  complex ctwk(2*NSS),ctwk2(2*NSS,-16:16)
  complex csa(2*NSS),csb(2*NSS),csc(2*NSS),csd(2*NSS)
  real a(5)
  real bitmetrics(2*NN,3)
  real llr(2*ND),llra(2*ND),llrb(2*ND),llrc(2*ND),llrd(2*ND)
  integer apbits(2*ND)
  integer*1 message77(77),rvec(77),apmask(2*ND),cw(2*ND)
  integer*1 hbits(2*NN)
  integer i4tone(103)
  integer nappasses(0:5)
  integer naptypes(0:5,4)
  integer mcq(29),mrrr(19),m73(19),mrr73(19)
  logical nohiscall,unpk77_success,first,badsync,lFreeText,lhidemsg
  logical(1) falsedec

  data first/.true./
  data     mcq/0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0/
  data    mrrr/0,1,1,1,1,1,1,0,1,0,0,1,0,0,1,0,0,0,1/
  data     m73/0,1,1,1,1,1,1,0,1,0,0,1,0,1,0,0,0,0,1/
  data   mrr73/0,1,1,1,1,1,1,0,0,1,1,1,0,1,0,1,0,0,1/
  data rvec/0,1,0,0,1,0,1,0,0,1,0,1,1,1,1,0,1,0,0,0,1,0,0,1,1,0,1,1,0, &
    1,0,0,1,0,1,1,0,0,0,0,1,0,0,0,1,0,1,0,0,1,1,1,1,0,0,1,0,1, &
    0,1,0,1,0,1,1,0,1,1,1,1,1,0,0,0,1,0,1/
  save fs,dt1,tt,txt,twopi,h,first,apbits,nappasses,naptypes, &
    mycall0,hiscall0,ctwk2,mcq,mrrr,m73,mrr73
! CE3TSK: ft4b runs once per candidate under the parallel slice loop, so everything it keeps
! between calls - the a-priori bit patterns rebuilt when the QSO state changes, the twiddle
! table, the constants - has to be one set per thread (the mcq-in-place-transform race of
! DECODER_IMPROVEMENTS item 52).
!$omp threadprivate(fs,dt1,tt,txt,twopi,h,first,apbits,nappasses,naptypes,mycall0,hiscall0,ctwk2, &
!$omp              mcq,mrrr,m73,mrr73)

    if(first) then
    fs=12000.0/NDOWN                !Sample rate after downsampling
    dt1=1/fs                         !Sample interval after downsample (s)
    tt=NSPS*dt1                      !Duration of "itone" symbols (s)
    txt=NZ*dt1                       !Transmission length (s) without ramp up/down
    twopi=8.0*atan(1.0)
    h=1.0

    do idf=-16,16
      a=0.
      a(1)=real(idf)
      ctwk=1.
      call twkfreq1(ctwk,0,2*NSS,2*NSS,fs/2.0,a,ctwk2(:,idf))
    enddo

    mcq=2*mod(mcq+rvec(1:29),2)-1
    mrrr=2*mod(mrrr+rvec(59:77),2)-1
    m73=2*mod(m73+rvec(59:77),2)-1
    mrr73=2*mod(mrr73+rvec(59:77),2)-1
    nappasses(0)=2; nappasses(1)=2; nappasses(2)=2; nappasses(3)=2; nappasses(4)=2; nappasses(5)=3

! iaptype
!   1        CQ     ???    ???           (29 ap bits)
!   2        MyCall ???    ???           (29 ap bits)
!   3        MyCall DxCall ???           (58 ap bits)
!   4        MyCall DxCall RRR           (77 ap bits)
!   5        MyCall DxCall 73            (77 ap bits)
!   6        MyCall DxCall RR73          (77 ap bits)

    naptypes(0,1:4)=(/1,2,0,0/) ! Tx6 selected (CQ)
    naptypes(1,1:4)=(/2,3,0,0/) ! Tx1
    naptypes(2,1:4)=(/2,3,0,0/) ! Tx2
    naptypes(3,1:4)=(/3,6,0,0/) ! Tx3
    naptypes(4,1:4)=(/3,6,0,0/) ! Tx4
    naptypes(5,1:4)=(/3,1,2,0/) ! Tx5

    mycall0=''; hiscall0=''; first=.false.
  endif

  l1=index(mycall,char(0)); if(l1.ne.0) mycall(l1:)=" "
  l1=index(hiscall,char(0)); if(l1.ne.0) hiscall(l1:)=" "
  if(mycall.ne.mycall0 .or. hiscall.ne.hiscall0) then
    apbits=0; apbits(1)=99; apbits(30)=99
    if(len(trim(mycall)) .lt. 3) go to 10
    nohiscall=.false.; hiscall0=hiscall
! use mycall for dummy hiscall - mycall won't be hashed
    if(len(trim(hiscall0)).lt.3) then; hiscall0=mycall; nohiscall=.true.; endif
    message=trim(mycall)//' '//trim(hiscall0)//' RR73'
    i3=-1; n3=-1
    call pack77(message,i3,n3,c77,0); call unpack77(c77,1,msgsent,unpk77_success,25)   ! CE3TSK: round trip only, save no hashes (as ft8apset.f90)
    if(i3.ne.1 .or. (message.ne.msgsent) .or. .not.unpk77_success) go to 10
    read(c77,'(77i1)') message77
    message77=mod(message77+rvec,2)
    call encode174_91(message77,cw)
    apbits=2*cw-1
    if(nohiscall) apbits(30)=99
10    continue
    mycall0=mycall; hiscall0=hiscall
  endif

    nf0=nint(f0); if(lfilter .and. (nf0.lt.nfafilt .or. nf0.gt.nfbfilt)) return
    snr=snrc-1.0
    tw0=omp_get_wtime(); call ft4_downsample(dobigfft,f0,cd2); t4down_s(nthr)=t4down_s(nthr)+omp_get_wtime()-tw0  !Downsample to 32 Sam/Sym
    if(dobigfft) dobigfft=.false.
    sum2=sum(cd2*conjg(cd2))/(real(NMAX)/real(NDOWN))
    if(sum2.gt.0.0) cd2=cd2/sqrt(sum2)
! Sample rate is now 12000/18 = 666.67 samples/second
! +/- 1.1 +/- 735 (1470); +/- 1.4 +/- 934.5 (1869); 0.5 sec 334 samples
    if(lswl) then; ibwindow=623; else; ibwindow=490; endif ! /3
    ibottom=(0.5+avexdt)*666.67-667
! CE3TSK: two sweeps over the DT segments - the ordinary passes first, the hint pass only in a
! second sweep once all three segments failed, so a hint can never pre-empt a decode a later
! segment would have made with better sync (measured: 8 decodes lost that way for 32 gained)
    nharderror=-1; smax2g=-99.; ib2g=-9999; idf2g=0
    nsweeps=2+nft4ens   ! CE3TSK: sweeps 3.. = the ensemble members (JTDX_FT4_DITHER=N, N=1 is the dither alone)
    do isweep=1,nsweeps
    if(isweep.ge.2 .and. nharderror.ge.0) exit
    if(isweep.eq.2 .and. (nft4hintdepth.le.0 .or. .not.ft4hint_near(f0))) cycle   ! no hint to try near this candidate
    do iseg=1,4                ! DT search: 3 segments, then the second near-DT peak (item 57)
      if(lvirt .and. iseg.ge.2) exit   ! item 75: the virtual candidate has one window, around the partner's DT
      if(iseg.eq.4) then
! CE3TSK: FT8's second-DT-peak-per-bin (DECODER_IMPROVEMENTS item 2) mapped to FT4's
! architecture. FT4 has no per-bin DT surface - the DT search lives here, per candidate - so
! the coarse scan's runner-up (32 samples / ~48 ms or more from the winner, kept by
! displacement) gets one fine-scan-and-decode attempt when all three segments failed: the
! co-channel station whose weaker DT peak sat in a stronger one's shadow. Ordinary sweep
! only; costs one fine scan per failed candidate. JTDX_FT4_DT2=0 switches it off.
        if(isweep.ne.1 .or. nft4dt2.eq.0 .or. smax2g.lt.1.2 .or. ib2g.lt.-900) exit
        idfbest=idf2g; ibest=ib2g; smax=smax2g
      endif
      do isync=1,2
        if(iseg.eq.4 .and. isync.eq.1) cycle   ! the coarse result is the runner-up itself
        if(isync.eq.1) then
          idfmin=-12
          idfmax=12
          idfstp=nft4idfstp   ! CE3TSK: 3 by default
!-1.0..+1.4; -1.2..+1.7 start window
          if(lvirt) then   ! item 75: xdt = ib/666.67 - 0.5, so ib = (xdt+0.5)*666.67; +-0.12 s, +-0.25 s at level 3
            ibvirt=nint((xdtvirt+0.5)*666.67); nwvirt=80; if(nft4rxfsens.ge.3) nwvirt=167
            ibmin=ibvirt-nwvirt; ibmax=ibvirt+nwvirt
          else if(abs(avexdt).lt.1.e-6) then
            if(iseg.eq.1) then
              if(lswl) then; ibmin=179; ibmax=823; else; ibmin=194; ibmax=730; endif
            elseif(iseg.eq.2) then
              smax1=smax
              if(lswl) then; ibmin=824; ibmax=1467; else; ibmin=731; ibmax=1266; endif
            elseif(iseg.eq.3) then
              if(lswl) then; ibmin=-467; ibmax=178; else; ibmin=-344; ibmax=193; endif
            endif
          else
            if(iseg.eq.1) then
              ibmin=ibottom+ibwindow+1; ibmax=ibottom+ibwindow*2
            elseif(iseg.eq.2) then
              smax1=smax
              ibmin=ibottom+ibwindow*2+1; ibmax=ibottom+ibwindow*3
            elseif(iseg.eq.3) then
              ibmin=ibottom; ibmax=ibottom+ibwindow
            endif
          endif
          ibstp=4
        else
          idfmin=idfbest-4
          idfmax=idfbest+4
          idfstp=1
          ibmin=ibest-5
          ibmax=ibest+5
          ibstp=1
        endif
        ibest=-1
        idfbest=0
        smax=-99.
        tw0=omp_get_wtime()
        ! CE3TSK: counted locally and added once below, as the sibling counters at the bit-metrics
        ! and OSD calls already are. n4sync_s is plain shared storage - the file's threadprivate
        ! directives cover only the sample and subtraction buffers - so all entries share a cache
        ! line, and incrementing it here put hundreds of thousands of contended read-modify-writes
        ! across twelve threads against work of a few hundred nanoseconds a call. The count, which
        ! only the JTDX_FT4_TIMING trace reads, is identical either way.
        nsyncloc=0
        do idf=idfmin,idfmax,idfstp
          call ft4_sync_twiddle(ctwk2(:,idf),csa,csb,csc,csd)   ! CE3TSK: once per frequency step, not per DT start
          do istart=ibmin,ibmax,ibstp
            nsyncloc=nsyncloc+1
            call sync4d_tw(cd2,istart,csa,csb,csc,csd,sync)  !Find sync power
            if(sync.gt.smax) then
              if(isync.eq.1 .and. isweep.eq.1 .and. abs(istart-ibest).ge.32 .and. smax.gt.smax2g) then
                smax2g=smax; ib2g=ibest; idf2g=idfbest   ! the displaced winner becomes the runner-up
              endif
              smax=sync
              ibest=istart
              idfbest=idf
            else if(isync.eq.1 .and. isweep.eq.1 .and. sync.gt.smax2g .and. abs(istart-ibest).ge.32) then
              smax2g=sync; ib2g=istart; idf2g=idf
            endif
          enddo
        enddo
        t4sync_s(nthr)=t4sync_s(nthr)+omp_get_wtime()-tw0
        n4sync_s(nthr)=n4sync_s(nthr)+nsyncloc
      enddo
      if(iseg.eq.1) smax1=smax
      if(smax.lt.1.2 .and. .not.lvirt) cycle   ! item 75: the virtual candidate trusts the partner's DT, as FT8's does, not a sync score
      if(iseg.gt.1 .and. iseg.ne.4 .and. smax.lt.smax1) cycle   ! CE3TSK: the runner-up is weaker by definition 
      ! CE3TSK: the ensemble members (JTDX_FT4_DITHER=N). FT8's ensemble perturbs a failed candidate
! three ways - a delay, additive dither and a frequency shift (ft8ensemble.f90, ens_perturb) -
! and FT4 only ever got the dither one. Member 1 is that same dither, so N=1 is unchanged; the
! rest walk the other kinds. A delay moves the alignment, so the reported DT follows it; a
! frequency shift moves the candidate onto f1, so the signal really sat at f1 + the offset and
! the reported frequency follows that.
      ikind=0; imdt=0; fmoff=0.
      if(isweep.ge.3) then
        imemb=isweep-2+nft4ensfrom-1   ! CE3TSK: the background continues where the RX members stopped (item 59)
        ikind=mod(imemb-1,6)+1
        select case(ikind)
          case(2); fmoff=ft4ensfreq
          case(3); fmoff=-ft4ensfreq
          case(4); imdt=1
          case(5); imdt=-1
        end select
      endif
      f1=f0+real(idfbest)+fmoff   ! CE3TSK: the frequency members move the final downsample
                                  ! centre, so the candidate is demodulated as if it sat
                                  ! there - and f1, which the report and the subtraction
                                  ! both use, already carries the offset
      if( f1.le.10.0 .or. f1.ge.4990.0 ) cycle
      call ft4_downsample(dobigfft,f1,cb) !Final downsample, corrected f0
      sum2=sum(abs(cb)**2)/(real(NSS)*NN)
      if(sum2.gt.0.0) cb=cb/sqrt(sum2)
      cd=0.
      ibm=ibest+imdt   ! CE3TSK: the delay members shift the alignment by a sample
      if(ibm.ge.0) then
        it=min(NDMAX-1,ibm+NN*NSS-1)
        np=it-ibm+1
        cd(0:np-1)=cb(ibm:it)
      else
        cd(-ibm:ibm+NN*NSS-1)=cb(0:NN*NSS+2*ibm-1)
      endif
      if(ikind.eq.1 .or. ikind.eq.6) then   ! CE3TSK: the dither members - ens_dither_cd0 on the aligned candidate
        sumdit=0.d0
        do i=0,NN*NSS-1; sumdit=sumdit+dble(real(cd(i)))**2+dble(aimag(cd(i)))**2; enddo
        gdit=0.03*real(sqrt(sumdit/dble(NN*NSS)/2.d0))
        sdit=int(nint(f1*10.),8)*40503_8+int(ibest,8)*2654435761_8+88172645463325252_8
        if(ikind.eq.6) sdit=sdit+7919_8*int(imemb,8)   ! the second dither draws a different noise
        do i=0,NN*NSS-1
          udit=0.; vdit=0.
          do j=1,12
            sdit=ieor(sdit,ishft(sdit,13)); sdit=ieor(sdit,ishft(sdit,-7)); sdit=ieor(sdit,ishft(sdit,17))
            udit=udit+real(ishft(sdit,-40))/16777216.
            sdit=ieor(sdit,ishft(sdit,13)); sdit=ieor(sdit,ishft(sdit,-7)); sdit=ieor(sdit,ishft(sdit,17))
            vdit=vdit+real(ishft(sdit,-40))/16777216.
          enddo
          cd(i)=cd(i)+cmplx((udit-6.)*gdit,(vdit-6.)*gdit)
        enddo
      endif
      tw0=omp_get_wtime(); call get_ft4_bitmetrics(cd,bitmetrics,badsync); t4bits_s(nthr)=t4bits_s(nthr)+omp_get_wtime()-tw0
      if(badsync) cycle
      hbits=0
      where(bitmetrics(:,1).ge.0) hbits=1
      ns1=count(hbits(  1:  8).eq.(/0,0,0,1,1,0,1,1/))
      ns2=count(hbits( 67: 74).eq.(/0,1,0,0,1,1,1,0/))
      ns3=count(hbits(133:140).eq.(/1,1,1,0,0,1,0,0/))
      ns4=count(hbits(199:206).eq.(/1,0,1,1,0,0,0,1/))
      nsync_qual=ns1+ns2+ns3+ns4
      if(nsync_qual.lt.nft4syncqual .and. .not.lvirt) cycle   ! CE3TSK: 20 by default; item 75: not for the virtual candidate

      scalefac=2.83
      llra(  1: 58)=bitmetrics(  9: 66, 1)
      llra( 59:116)=bitmetrics( 75:132, 1)
      llra(117:174)=bitmetrics(141:198, 1)
      llra=scalefac*llra
      llrb(  1: 58)=bitmetrics(  9: 66, 2)
      llrb( 59:116)=bitmetrics( 75:132, 2)
      llrb(117:174)=bitmetrics(141:198, 2)
      llrb=scalefac*llrb
      llrc(  1: 58)=bitmetrics(  9: 66, 3)
      llrc( 59:116)=bitmetrics( 75:132, 3)
      llrc(117:174)=bitmetrics(141:198, 3)
      llrc=scalefac*llrc

      apmag=maxval(abs(llra))*1.1
      npasses=3+nappasses(nQSOProgress)
      if(stophint) npasses=4
      if(ndepth.eq.1) npasses=3
! CE3TSK: the hint pass - a message decoded in one of the last nft4hintdepth same-parity
! periods at this frequency and DT is tried once more with all 77 bits fixed (the RR73 AP
! type 6 mechanism), after every ordinary pass has failed on this candidate
      lhint=.false.; npasst=npasses; ih=0; kh=0
      if(isweep.eq.2) then
        xdtc=ibest/666.67 - 0.5
        ! CE3TSK: no lock needed any more - the previous periods' lists are read only while
        ! the slices run (they are retired in emit(), after the loop) and the "already spent"
        ! mask ft4used is per thread.
        call ft4hint_find(f1,xdtc,msgd,ih,kh,lhint)
        if(lhint) then
          i3=-1; n3=-1; call pack77(msgd,i3,n3,c77,0); call unpack77(c77,1,msgsent,lhintok,25)   ! CE3TSK: round trip only, save no hashes
          if(lhintok .and. msgsent.eq.msgd) then
            read(c77,'(77i1)') hint77; hint77=mod(hint77+rvec,2)
            call encode174_91(hint77,cw); hintbits=2*cw-1; npasst=npasses+1
          else
            lhint=.false.
          endif
        endif
      endif
      ipass0=1; if(isweep.eq.2) then; if(.not.lhint) cycle; ipass0=npasst; endif   ! sweep 2: the hint pass alone
      if(isweep.ge.3) then; lhint=.false.; npasst=npasses; endif   ! members: the ordinary passes on the perturbed candidate
      do ipass=ipass0,npasst
        if(ipass.eq.1) llr=llra; if(ipass.eq.2) llr=llrb; if(ipass.eq.3) llr=llrc
        if(ipass.le.3) then; apmask=0; iaptype=0; endif
        if(lhint .and. ipass.eq.npasst) then   ! CE3TSK: the hint pass
          llrd=llra; iaptype=7; apmask=0; apmask(1:77)=1; llrd(1:77)=apmag*hintbits(1:77); llr=llrd
        else if(ipass.gt.3) then
          llrd=llra
          iaptype=naptypes(nQSOProgress,ipass-3)
          if(stophint) iaptype=1
! Conditions that cause us to bail out of AP decoding
          napwid=50
          if(iaptype.ge.3 .and. (abs(f1-nfqso).gt.napwid)) cycle
          if(iaptype.ge.2 .and. apbits(1).gt.1) cycle  ! No, or nonstandard, mycall
          if(iaptype.ge.3 .and. apbits(30).gt.1) cycle ! No, or nonstandard, dxcall

          if(iaptype.eq.1) then; apmask=0; apmask(1:29)=1; llrd(1:29)=apmag*mcq(1:29); endif ! CQ
          if(iaptype.eq.2) then; apmask=0; apmask(1:29)=1; llrd(1:29)=apmag*apbits(1:29); endif ! MyCall,???,???
          if(iaptype.eq.3) then; apmask=0; apmask(1:58)=1; llrd(1:58)=apmag*apbits(1:58); endif ! MyCall,DxCall,???
          if(iaptype.eq.4 .or. iaptype.eq.5 .or. iaptype.eq.6) then ! mycall, hiscall, RRR|73|RR73
            apmask=0; apmask(1:77)=1; if(iaptype.eq.6) llrd(1:77)=apmag*apbits(1:77)
          endif
          llr=llrd
        endif
        message77=0; dmin=0.0
        tw0=omp_get_wtime(); n4bp_s(nthr)=n4bp_s(nthr)+1
        call bpdecode174_91(llr,apmask,max_iterations,message77,cw,nharderror,niterations)
        t4bp_s(nthr)=t4bp_s(nthr)+omp_get_wtime()-tw0
        lviaosd=.false.
        if(doosd .and. nharderror.lt.0) then
          lviaosd=.true.
          ndeep=nft4osddeep   ! CE3TSK: 3 by default
!              if(abs(nfqso-f1).le.napwid) ndeep=4
          if(lvirt) ndeep=max(ndeep,merge(5,4,nft4rxfsens.ge.3))   ! item 75: the virtual candidate gets the deep OSD JTDX left commented out here - one candidate, a QSO's reply
          tw0=omp_get_wtime(); n4osd_s(nthr)=n4osd_s(nthr)+1
! CE3TSK: the FT8 OSD - the same (174,91) code and the same search, but GF(2) rows packed in
! three 64-bit words (DECODER_IMPROVEMENTS.md item 18); osd4_174_91 was a stock copy differing
! only in the thread plumbing. Thread slot 1: FT4 decodes on one thread.
          ! CE3TSK: this slice's index, as ft8b.f90 passes nthr. osd174_91's fetchit91p keeps
          ! lastpat/inext per slice; FT4 passed a literal 1 from its single-threaded days, so
          ! every slice shared one enumeration state and OSD returned different index pairs
          ! depending on the interleaving - the same candidate decoded via OSD in one run and
          ! not in the next (DECODER_IMPROVEMENTS item 52).
          call osd174_91(llr,apmask,ndeep,message77,cw,nharderror,dmin,nthr)
          t4osd_s(nthr)=t4osd_s(nthr)+omp_get_wtime()-tw0
        endif

        if(lft4timing .and. lvirt) write(0,9015) ipass,iaptype,isweep,idfbest,ibest,smax,nharderror,dmin   ! item 75 trace
9015    format('ft4 virt pass ',i2,' ap ',i1,' sweep ',i1,' idf ',i3,' ib ',i5,' smax ',f6.2,' nhard ',i4,' dmin ',f7.2)
        if(sum(message77).eq.0) cycle
        if(nharderror.ge.0) then
          message77=mod(message77+rvec,2) ! remove rvec scrambling
          write(c77,'(77i1)') message77(1:77); read(c77(72:74),'(b3)') n3; read(c77(75:77),'(b3)') i3
          call unpack77(c77,1,message,unpk77_success,nthr)   ! CE3TSK: this slice's own hash window, as ft8b.f90 passes nthr
          if(message.eq."") cycle ! being treated as false decode
! CE3TSK: honour unpack77's verdict as the FT8 path does - it is where the report-range gate
! and the RTTY-Roundup / telemetry switches live (packjt77.f90); FT4 accepted any non-empty
! text, so "TU; F09NJF WZ8DWC 559 7995" at -20 dB slipped through on the WSJT-X FT4 sample
          if(.not.unpk77_success) cycle
          if(unpk77_success.and.dosubtract) then
            call get_ft4_tones_from_77bits(message77,i4tone)
            dt=real(ibest)/666.67
            tw0=omp_get_wtime(); call subtractft4(i4tone,f1,dt); t4sub_s(nthr)=t4sub_s(nthr)+omp_get_wtime()-tw0
          endif

          lhidemsg=.false.
          if(lhidetelemetry .and. i3.eq.0 .and. n3.eq.5) lhidemsg=.true.
          if(lhidetest) then
            if((i3.eq.0 .and. n3.gt.1 .and. n3.lt.5) .or. i3.eq.3 .or. i3.gt.4) then
              if(mycalllen1.lt.4 .or. message(1:mycalllen1).ne.trim(mycall)//' ') lhidemsg=.true.
            endif
            if(message(1:3).eq.'CQ ') then
              if(message(1:6).eq.'CQ RU ' .or. message(1:6).eq.'CQ FD ' .or. message(1:8).eq.'CQ TEST ') &
                lhidemsg=.true.
            endif
          endif

          lFreeText=.false.; if(i3.eq.0 .and. n3.eq.0) lFreeText=.true.
! delete braces
          if(.not.lFreeText .and. index(message,'<').gt.0) then ! DXpedition being not supported in FT4
            ispc1=index(message,' '); ispc2=index(message((ispc1+1):),' ')+ispc1
            ispc3=index(message((ispc2+1):),' ')+ispc2
            ieoc1=ispc1-1; iboc2=ispc1+1; ieoc2=ispc2-1
            if(message(1:1).eq.'<' .and. message(2:2).ne.'.') then
              message(ieoc1:37)=message(ieoc1+1:37)//' '; message(1:37)=message(2:37)//' '
            else if(message(iboc2:iboc2).eq.'<' .and. message(iboc2+1:iboc2+1).ne.'.') then
              message(ieoc2:37)=message(ieoc2+1:37)//' '; message(iboc2:37)=message(iboc2+1:37)//' '
            else
              iboc3=ispc2+1; ieoc3=ispc3-1
              if(message(iboc3:iboc3).eq.'<' .and. message(iboc3+1:iboc3+1).ne.'.') then
                message(ieoc3:37)=message(ieoc3+1:37)//' '; message(iboc3:37)=message(iboc3+1:37)//' '
              endif
            endif
          endif

          idupe=0
          do i=1,ndecodes; if(decodes(i).eq.message) idupe=1; enddo
          if(idupe.eq.1) exit
          ndecodes=ndecodes+1; decodes(ndecodes)=message
          if(snr.gt.0.0) then; xsnr=10*log10(snr)-14.8; else; xsnr=-21.0; endif
          nsnr=nint(max(-21.0,xsnr))
          xdt=ibm/666.67 - 0.5   ! CE3TSK: ibm carries the member's delay, ibest for every other sweep
! check for false decodes
! i3=3 n3=4  TU; B69FWJ 8Z6IB 559 580  
! i3=3 n3=3  TU; FD9GRU HT1HHY R 529 11
          if(message(1:3).eq.'TU;' .and. nsnr.lt.-15 .and. i3.eq.3 .and. (n3.eq.3 .or. n3.eq.4)) then
            ispc1=index(message,' '); ispc2=index(message((ispc1+1):),' ')+ispc1 
            ispc3=index(message((ispc2+1):),' ')+ispc2
            call_a=''; call_b=''; call_a=message(ispc1+1:ispc2-1); call_b=message(ispc2+1:ispc3-1)
            falsedec=.false.; call chkflscall(call_a,call_b,falsedec)
            if(falsedec) then; message=''; cycle; endif
          endif
! CE3TSK: FT8's gating of chkfalse8 (ft8b.f90): every doubtful decode - weak, or from an
! a-priori type 1..3 - goes through the message-level checks (the callsign prefix against the
! grid, the two-unknown-calls test), unless it carries my own or the DX base call and is not an
! a-priori guess about them. JTDX's FT4 ran it only on CQ a-priori decodes below -15 dB, so a
! plain-decoded ghost with an impossible prefix/grid pair passed. The full-AP types 4-6 and the
! hint pass (7) are known messages, nothing to check. SNR gate: -17.5 on FT4's scale, as -20.5
! sits 3.5 dB above FT8's floor (JTDX_FT4_FALSEGATE overrides).
          if(iaptype.lt.4 .and. (xsnr.lt.ft4falsegate .or. (iaptype.ge.1 .and. iaptype.le.3))) then
            lmine=.false.
            if(iaptype.ne.2 .and. iaptype.ne.3) then
              if(len_trim(mycall).gt.2) then; if(index(message,trim(mycall)).gt.0) lmine=.true.; endif
              if(len_trim(hiscall).gt.2) then; if(index(message,trim(hiscall)).gt.0) lmine=.true.; endif
            endif
            if(.not.lmine) then
              nbadcrc=0; msg37_2=message; lhash1=message(1:1).eq.'<'
              call chkfalse8(message,i3,n3,nbadcrc,iaptype,lhash1)
              if(nbadcrc.eq.1) then
                if(lft4timing) write(0,9020) i3,n3,iaptype,nsnr,msg37_2(1:26)
9020                format('ft4 reject i3 ',i2,' n3 ',i2,' iaptype ',i2,' snr ',i4,' ',a)
                message=''; cycle
              endif
            endif
          endif
! EA1AHY M83WN/R R QA79   *
! MS8QQS UX3QBS/P R NG63  i3=2 n3=7
! 3B4NDC/R C40AUZ/R R IR83  i3=1 n3=7
! EA1AHY PW1BSL R GR47 i3=1 n3=3 mycall
! EA1AHY PW1BSL R GR47 *  i3=1 n3=1 mycall
          if((i3.eq.1 .or. i3.eq.2) .and. index(message,' R ').gt.0) then
            ispc1=index(message,' '); ispc2=index(message((ispc1+1):),' ')+ispc1
            ispc3=index(message((ispc2+1):),' ')+ispc2
            if(message(ispc2:ispc3).eq.' R ') then 
              call_a='            '; call_b='            '
              if(message(1:ispc1-1).eq.trim(mycall)) then
                call_a='CQ          '
              else
                if((i3.eq.1 .and. message(ispc1-2:ispc1-1).eq.'/R') .or. &
                   (i3.eq.2 .and. message(ispc1-2:ispc1-1).eq.'/R')) then
                  call_a=message(1:ispc1-3)
                else
                  call_a=message(1:ispc1-1)
                endif
              endif
              if((i3.eq.1 .and. message(ispc2-2:ispc2-1).eq.'/R') .or. &
                 (i3.eq.2 .and. message(ispc2-2:ispc2-1).eq.'/P')) then
                call_b=message(ispc1+1:ispc2-3)
              else
                call_b=message(ispc1+1:ispc2-1)
              endif
              falsedec=.false.; call chkflscall(call_a,call_b,falsedec)
              if(falsedec) then   ! CE3TSK item 62: this candidate is rejected, the slice goes on (as ft8b)
                if(lft4timing) write(0,9021) i3,n3,iaptype,nsnr,call_a,call_b
9021            format('ft4 reject /R i3 ',i2,' n3 ',i2,' iaptype ',i2,' snr ',i4,' ',a,' ',a)
                nbadcrc=1; message=''; return
              endif
            endif
          endif
!write(21,'(i6.6,i5,2x,f4.1,i6,2x,a37,2x,f4.1,3i3,f5.1,i4,i4,i4)') &
!  nutc,nsnr,xdt,nint(f1),message,smax,iaptype,ipass,isp,dmin,nsync_qual,nharderror,iseg
          ! CE3TSK: buffered, not emitted. The dedupe, the callback and the hint memory
          ! all happen in emit() once the slice loop is over, in slice order - see ft4_mod1.
          if(nft4res(nthr).lt.NDEC4MAX) then
            nft4res(nthr)=nft4res(nthr)+1; ir=nft4res(nthr)
            ft4res(ir,nthr)%msg=message; ft4res(ir,nthr)%nsnr=nsnr
            ft4res(ir,nthr)%xdt=xdt; ft4res(ir,nthr)%f1=f1
            ft4res(ir,nthr)%ih=0; ft4res(ir,nthr)%kh=0
            if(iaptype.eq.7) then; ft4res(ir,nthr)%ih=ih; ft4res(ir,nthr)%kh=kh; endif
            if(i3.eq.0 .and. n3.eq.1) then ! special DXpedition msg
              call msgparser(message,msg37_2)
              ft4res(ir,nthr)%srv="1"; ft4res(ir,nthr)%ltwo=.true.; ft4res(ir,nthr)%lemit=.true.
              if(nbg4run.gt.0) ft4res(ir,nthr)%srv='|'   ! CE3TSK: the background overrides '1' as FT8's does
              ft4res(ir,nthr)%m26a=message(1:26); ft4res(ir,nthr)%m26b=msg37_2(1:26)
            else
              ! CE3TSK: the markers, FT8's rules verbatim (ft8_decode.f90 ~368): a plain decode is
              ! blank, or ',' / '.' for free text at / off the QSO frequency; a hint decode '^'
              ! (iaptype 7, FT8's lft8sd/lft8s); any other AP decode '*' (types 1-6, unmarked until
              ! item 61); the hint and AP markers take precedence over the free-text ones as in FT8
              servis4=""
              if(iaptype.eq.0) then
                if(lFreeText) then; if(abs(nfqso-nint(f1)).le.10) then; servis4=','; else; servis4='.'; endif; endif
              else if(iaptype.eq.7) then; servis4='^'
              else; servis4='*'
              endif
              if(nbg4run.gt.0) then   ! CE3TSK: a background decode - '|', '#' for a hint (item 59)
                if(servis4.eq.'^') then; servis4='#'; else; servis4='|'; endif
              endif
              ft4res(ir,nthr)%srv=servis4; ft4res(ir,nthr)%ltwo=.false.
              ft4res(ir,nthr)%m26a=message(1:26); ft4res(ir,nthr)%lemit=.not.lhidemsg
            endif
          endif
          ! this slice must not spend the same hint twice; the shared list is retired in emit()
          if(iaptype.eq.7 .and. ih.gt.0) ft4used(ih,kh)=.true.
          ndecd4(nthr)=ndecd4(nthr)+1; sumxdt4(nthr)=sumxdt4(nthr)+dble(xdt)   ! CE3TSK: per slice, summed in order
          if(lft4timing) write(0,9010) ipass,lviaosd,nsync_qual,smax,iseg,nsnr,isp,nthr,message(1:26)   ! CE3TSK: provenance
9010          format('ft4 decode pass ',i2,' osd ',l1,' nsync ',i3,' smax ',f6.2,' iseg ',i2,' snr ',i3,' isp ',i2,' thr ',i3,' ',a)
          exit
        endif
      enddo !Sequence estimation
      if(nharderror.ge.0) exit
    enddo !DT segments + the second peak (item 57)
    enddo !two sweeps

  return
end subroutine ft4b
