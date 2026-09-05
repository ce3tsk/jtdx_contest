module ft8_decode

  type :: ft8_decoder
     procedure(ft8_decode_callback), pointer :: callback
   contains
     procedure :: decode
     procedure :: emit => ft8emit   ! CE3TSK: the slice-order emission merge (FT8_EMISSION_ORDER.md)
  end type ft8_decoder

  abstract interface
     subroutine ft8_decode_callback (this,snr,dt,freq,decoded,servis8)
       import ft8_decoder
       implicit none
       class(ft8_decoder), intent(inout) :: this
       integer, intent(in) :: snr
       real, intent(in) :: dt,freq
       character(len=26), intent(in) :: decoded
       character(len=1), intent(in) :: servis8
     end subroutine ft8_decode_callback
  end interface

contains

  subroutine decode(this,callback,nQSOProgress,nfqso,nft8rxfsens,nftx,nutc,nfa,nfb,ncandthin,ndtcenter,nsec, &
                    napwid,swl,lmycallstd,lhiscallstd,filter,stophint,nthr,numthreads,nagainfil,lft8lowth,   &
                    lft8subpass,lhideft8dupes,lhidehash)
!use wavhdr
!    use timer_module, only: timer
 !$ use omp_lib
    use ft8_mod1, only : lcollectdelta,dd8orig,dd8delta,lsecondpass   ! CE3TSK
    use ft8ensemble, only : lretrymode,nretrydither,nbgrun,lbgabort,bg_lock_gone,ift8stage,ens_record_fail, &
                            ens_retry_candidates,bgsyncscale,NRETRY,nretrystage   ! CE3TSK: pipeline ensemble
    use ft8_mod1, only : oddcopyk,evencopyk,nhintdepth   ! CE3TSK experiment: deeper hint memory
    use ft8_mod1, only : ft8res,nft8res,NDEC8MAX,lft8buffered,ft8used,ihint8,khint8   ! CE3TSK: the emission merge buffer
    use ft8_mod1, only : ndecodes,allmessages,allsnrs,allfreq,odd,even,nmsg,lastrxmsg,lasthcall,calldteven,calldtodd,incall, &
                         oddcopy,evencopy,avexdt,mycall,hiscall,dd8,nft8cycles,nft8swlcycles,ncandallthr,nincallthr,evencq,  &
                         oddcq,numcqsig,numdeccq,evenmyc,oddmyc,nummycsig,numdecmyc,lapmyc,evenqso,oddqso,lqsomsgdcd,hisgrid4
    use ft4_mod1, only : lhidetest,lhidetelemetry
    include 'ft8_params.f90'
!type(hdr) h

    class(ft8_decoder), intent(inout) :: this
    procedure(ft8_decode_callback) :: callback
!    real sbase(NH1)
!integer*2 iwave(180000)
    real, DIMENSION(:), ALLOCATABLE :: dd8m
    real candidate(4,2000),freqsub(200)  ! CE3TSK: was 460, see sync8.f90
    real syncminx   ! CE3TSK: the pass's syncmin, scaled for the residual unit
    integer istage
    character(len=16) :: ctrace   ! CE3TSK: JTDX_CAND_TRACE diagnostic hook
    integer ltrace,itrace
    real ftrace
    integer nadd   ! CE3TSK
    integer ir8,nloc8,ick   ! CE3TSK: the emission merge buffer
    integer, intent(in) :: nQSOProgress,nfqso,nft8rxfsens,nftx,nfa,nfb,ncandthin,ndtcenter,nsec,napwid,nthr,numthreads
    logical, intent(in) :: nagainfil
    logical(1), intent(in) :: swl,filter,stophint,lft8lowth,lft8subpass,lhideft8dupes, &
                              lhidehash,lmycallstd,lhiscallstd
    logical newdat1,lsubtract,ldupe,lFreeText,lspecial,ldectrace,ldectrace2
    logical(1) lft8sdec,lft8s,lft8sd,lrepliedother,lhashmsg,lqsothread,lhidemsg,lhighsens,lcqcand,lsubtracted,levenint,loddint, &
               lnohiscall,lnomycall,lnohisgrid
    character msg37*37,msg37_2*37,msg26*26,servis8*1,datetime*13,call2*12
    character*37 msgsrcvd(130)

    type oddtmp_struct
      real freq
      real dt
      logical lstate
      character*37 msg
    end type oddtmp_struct
    type(oddtmp_struct) oddtmp(130)

    type eventmp_struct
      real freq
      real dt
      logical lstate
      character*37 msg
    end type eventmp_struct
    type(eventmp_struct) eventmp(130)

    type tmpcqdec_struct
      real freq
      real xdt
    end type tmpcqdec_struct
    type(tmpcqdec_struct) tmpcqdec(numdeccq) ! 40 sigs

    type tmpcqsig_struct
      real freq
      real xdt
      complex cs(0:7,79)
    end type tmpcqsig_struct
    type(tmpcqsig_struct) tmpcqsig(numcqsig) ! 20 sigs

    type tmpmyc_struct
      real freq
      real xdt
    end type tmpmyc_struct
    type(tmpmyc_struct) tmpmyc(numdecmyc) ! 25 sigs

    type tmpmycsig_struct
      real freq
      real xdt
      complex cs(0:7,79)
    end type tmpmycsig_struct
    type(tmpmycsig_struct) tmpmycsig(nummycsig) ! 5 sigs

    type tmpqsosig_struct
      real freq
      real xdt
      complex cs(0:7,79)
    end type tmpqsosig_struct
    type(tmpqsosig_struct) tmpqsosig(1)

    this%callback => callback
    ft8used=.false.   ! CE3TSK: this slice has spent no hint yet (retired in emit, as FT4 does)
! CE3TSK: the retry unit set the thread's dither seed (line ~252) and nothing ever cleared it,
! so an OS thread that had run a retry slice kept dithering every candidate of every later
! unit and period at 3 % - and which threads carried the taint was pure scheduling. Since the
! retry unit shipped this quietly perturbed random slices in every background configuration.
    if(.not.lretrymode) nretrydither=0

    oddtmp%lstate=.false.; eventmp%lstate=.false.; nmsgloc=0; ncandthr=0
    nmsgcq=0; tmpcqdec(:)%freq=6000.0; nmsgmyc=0; tmpmyc(:)%freq=6000.0
    tmpcqsig(:)%freq=6000.0; tmpmycsig(:)%freq=6000.0; tmpqsosig(1)%freq=6000.0
    if(hiscall.eq.'') then; lastrxmsg(1)%lstate=.false. 
    else if(lastrxmsg(1)%lstate .and. lasthcall.ne.hiscall .and. index(lastrxmsg(1)%lastmsg,trim(hiscall)).le.0) &
          then; lastrxmsg(1)%lstate=.false.
    endif

    levenint=.false.; loddint=.false.
    if(nsec.eq.0 .or. nsec.eq.30) then; levenint=.true.
    elseif(nsec.eq.15 .or. nsec.eq.45) then; loddint=.true.
    endif

    lrepliedother=.false.; lft8sdec=.false.; lqsothread=.false.; lsubtracted=.false.!; lthrdecd=.false.
    ncount=0; servis8=' '; mycalllen1=len_trim(mycall)+1; ncqsignal=0; nmycsignal=0
    call get_environment_variable('JTDX_CAND_TRACE',ctrace,ltrace,itrace)   ! CE3TSK diagnostic hook
    ftrace=0.; if(itrace.eq.0 .and. ltrace.gt.0) read(ctrace(1:ltrace),*,iostat=itrace) ftrace
    if(itrace.ne.0) ftrace=0.   ! an unreadable value switches the trace off
! CE3TSK diagnostic hook: JTDX_DEC_TRACE=1 - every CRC-clean decode with its pass, slice,
! frequency, DT and SNR, on stderr, whole band. The instrument that localises which decode
! diverged first between two runs (FT8_EMISSION_ORDER.md, measurement hygiene).
    call get_environment_variable('JTDX_DEC_TRACE',ctrace,ltrace,itrace)
    ldectrace=(itrace.eq.0 .and. ltrace.gt.0)
    ldectrace2=(ldectrace .and. ctrace(1:1).eq.'2')   ! =2: every attempt, not only the decodes
    if(ldectrace) write(0,3071) nthr,loc(dd8),mod(loc(dd8),64_8)
3071 format('SLICEMAP thr',i3,' dd8 ',i0,' mod64 ',i0)
    if(.not.lsecondpass) nincallthr(nthr)=0   ! CE3TSK: the second slicing pass adds to pass 1's incoming calls
!print *,lastrxmsg(1)%lstate,lastrxmsg(1)%xdt,lastrxmsg(1)%lastmsg
    write(datetime,1001) nutc        !### TEMPORARY ###
1001 format("000000_",i6.6)

    lnohiscall=.false.; if(len_trim(hiscall).lt.3) lnohiscall=.true.
    lnomycall=.false.; if(len_trim(mycall).lt.3) lnomycall=.true.
    lnohisgrid=.false.; if(len_trim(hisgrid4).ne.4) lnohisgrid=.true.

    if(nfqso.ge.nfa .and. nfqso.le.nfb) lqsothread=.true.

    if(lqsothread .and. .not.lastrxmsg(1)%lstate .and. .not.stophint .and. hiscall.ne.'') then
! got incoming call
      do i=1,30
        if(incall(i)%msg(1:1).eq." ") exit
        if(index(incall(i)%msg,(trim(mycall)//' '//trim(hiscall))).eq.1) then
          lastrxmsg(1)%lastmsg=incall(i)%msg; lastrxmsg(1)%xdt=incall(i)%xdt; lastrxmsg(1)%lstate=.true.; exit
        endif
      enddo

      if(.not.lastrxmsg(1)%lstate) then
! calling someone, lastrxmsg still not valid
        if(levenint) then
          do i=1,130
            if(.not.evencopy(i)%lstate) cycle
            if(index(evencopy(i)%msg,' '//trim(hiscall)//' ').gt.1) then
              lastrxmsg(1)%lastmsg=evencopy(i)%msg; lastrxmsg(1)%xdt=evencopy(i)%dt; lastrxmsg(1)%lstate=.true.; exit
            endif
          enddo
        elseif(loddint) then
          do i=1,130
            if(.not.oddcopy(i)%lstate) cycle
              if(index(oddcopy(i)%msg,' '//trim(hiscall)//' ').gt.1) then
                lastrxmsg(1)%lastmsg=oddcopy(i)%msg; lastrxmsg(1)%xdt=oddcopy(i)%dt; lastrxmsg(1)%lstate=.true.; exit
              endif
          enddo
        endif
        do k=2,nhintdepth   ! CE3TSK experiment: the older same-parity lists
          if(lastrxmsg(1)%lstate) exit
          do i=1,130
            if(levenint .and. evencopyk(i,k)%lstate .and. index(evencopyk(i,k)%msg,' '//trim(hiscall)//' ').gt.1) then
              lastrxmsg(1)%lastmsg=evencopyk(i,k)%msg; lastrxmsg(1)%xdt=evencopyk(i,k)%dt; lastrxmsg(1)%lstate=.true.; exit
            endif
            if(loddint .and. oddcopyk(i,k)%lstate .and. index(oddcopyk(i,k)%msg,' '//trim(hiscall)//' ').gt.1) then
              lastrxmsg(1)%lastmsg=oddcopyk(i,k)%msg; lastrxmsg(1)%xdt=oddcopyk(i,k)%dt; lastrxmsg(1)%lstate=.true.; exit
            endif
          enddo
        enddo
      endif
    endif

!print *,'in',lastrxmsg(1)%lstate
!print *,lastrxmsg(1)%lastmsg
!write(*,1018) nQSOProgress,'d'
!1018 format(i1,46x,a1)

! sliding search over +/- 2.5s relative to 0.5s TX start time
    jzb=-62 + avexdt*25.;  jzt=62 + avexdt*25.
! sliding search over +/- 3.5s relative to 0.5s TX start time
    if(swl) then; jzb=-86 + avexdt*25.;  jzt=86 + avexdt*25.; endif

    npass=3 ! fallback
    if(swl) then
      if(nft8swlcycles.ge.3 .and. nft8swlcycles.le.9) then; npass=nft8swlcycles
      else; npass=3
      endif
    else
      if(nft8cycles.ge.3 .and. nft8cycles.le.9) then; npass=nft8cycles
      else; npass=3
      endif
    endif
    syncmin=1.5
    if(lretrymode) npass=NRETRY   ! CE3TSK: the retry unit - one try per pass, a different dither each
    do ipass=1,npass
      newdat1=.true.; lsubtract=.true.; npos=0
      if(nbgrun.eq.1) then   ! CE3TSK: background phase - the GUI asked for the next decode?
        if(bg_lock_gone()) then; lbgabort=.true.; exit; endif
      endif
      if(ipass.eq.1 .or. ipass.eq.4 .or. ipass.eq.7) then
        if(lft8lowth .or. swl) syncmin=1.225
      elseif(ipass.eq.2 .or. ipass.eq.5 .or. ipass.eq.8) then
         if(lft8lowth .or. swl) syncmin=1.5
      elseif(ipass.eq.3 .or. ipass.eq.6 .or. ipass.eq.9) then
         if(lft8lowth .or. swl) syncmin=1.1
      endif
      if(ipass.gt.5 .or. (ipass.eq.3 .and. npass.eq.3 .and. .not.swl)) lsubtract=.false.
      if(lretrymode) lsubtract=.false.   ! CE3TSK: retries never touch the band
! CE3TSK: dd8 is now one buffer per thread (threadprivate, copied in at the start of the
! parallel region), so every thread averages and restores its own copy and no barrier or
! single section is needed. Before, all threads subtracted into one shared buffer with
! barriers only here, which made threaded results depend on timing.
      if(ipass.eq.4 .and. .not.lretrymode) then
          if(lcollectdelta) dd8delta(:,nthr)=dd8-dd8orig   ! CE3TSK: raw subtracted data, before averaging
          if(npass.eq.9) then ! 3 decoding cycles
            allocate(dd8m(180000), STAT = nAllocateStatus1)
            if(nAllocateStatus1.ne.0) STOP "Not enough memory"
            dd8m=dd8
          endif
          do i=1,179999; dd8(i)=(dd8(i)+dd8(i+1))/2; enddo
      else if(ipass.eq.7 .and. .not.lretrymode) then
        if(allocated(dd8m)) then
          dd8(1)=dd8m(1)
          do i=2,180000; dd8(i)=(dd8m(i-1)+dd8m(i))/2; enddo
          deallocate (dd8m, STAT = nDeAllocateStatus1)
          if (nDeAllocateStatus1.ne.0) print *, 'failed to release memory'
        endif
      endif
      !call timer('sync8   ',0)
      if(lretrymode) then   ! CE3TSK: the retry unit decodes this slice's LDPC/OSD failures again on dithered data
        call ens_retry_candidates(nthr,candidate,ncand); nretrydither=ipass
      else
        syncminx=syncmin*bgsyncscale   ! CE3TSK: the residual unit lowers the sync threshold
        call sync8(nfa,nfb,syncminx,nfqso,candidate,ncand,jzb,jzt,swl,ipass,lqsothread,ncandthin,filter,ndtcenter)
      endif
      !call timer('sync8   ',1)
      do icand=1,ncand
        sync=candidate(3,icand)
        f1=candidate(1,icand)
        xdt=candidate(2,icand)
        lcqcand=.false.; if(candidate(4,icand).gt.1.0) lcqcand=.true.
        lhighsens=.false.
        if(sync.lt.1.9 .or. ((ipass.eq.2 .or. ipass.eq.4 .or. ipass.eq.6).and. sync.lt.3.15)) lhighsens=.true.
        lspecial=.false.; lFreeText=.false.; i3bit=0; lft8s=.false.; lft8sd=.false.; lhashmsg=.false.; iaptype=0
        msg37='';i3=16;n3=16
        !call timer('ft8b    ',0)
!if(nthr.eq.1) print *,ipass,'nthr1',newdat1
!if(nthr.eq.2) print *,ipass,'nthr2',newdat1
!write (*,"(F5.2,1x,I1,1x,I4,1x,F4.2)") candidate(2,icand)-0.5,ipass,nint(candidate(1,icand)),candidate(3,icand)
        xsnr=-99.0   ! CE3TSK: ft8b leaves it unset on a failed candidate; nint() of it below was on garbage
        call ft8b(newdat1,nQSOProgress,nfqso,nftx,napwid,lsubtract,npos,freqsub,tmpcqdec,tmpmyc,              &
                  nagainfil,iaptype,f1,xdt,nbadcrc,lft8sdec,msg37,msg37_2,xsnr,swl,stophint,               &
                  nthr,lFreeText,ipass,lft8subpass,lspecial,lcqcand,ncqsignal,nmycsignal,npass,            &
                  i3bit,lhidehash,lft8s,lmycallstd,lhiscallstd,levenint,loddint,lft8sd,i3,n3,nft8rxfsens,  &
                  ncount,msgsrcvd,lrepliedother,lhashmsg,lqsothread,lft8lowth,lhighsens,lsubtracted,       &
                  tmpcqsig,tmpmycsig,tmpqsosig,lnohiscall,lnomycall,lnohisgrid)
        if(ldectrace2) write(0,3072) ipass,nthr,icand,candidate(1,icand),candidate(2,icand),sync,f1,nbadcrc,iaptype, &
                                     candidate(4,icand),lhighsens
3072    format('ATTTRACE pass',i2,' thr',i3,' cand',i4,' fc',f8.1,' dtc',f6.2,' sync',f7.2,' f1',f8.1,' crc',i2,' ap',i3, &
               ' c4',f4.1,' hs',l2)
        if(nbadcrc.ne.0 .and. ift8stage.eq.2 .and. .not.lretrymode) call ens_record_fail(nthr,f1,xdt,sync)   ! CE3TSK: for the retry unit
! CE3TSK diagnostic hook: JTDX_CAND_TRACE=<Hz> - every candidate within +-25 Hz of that
! frequency with the decoder's verdict (crc 0 = decoded; stage 1 = rejected before any
! LDPC/OSD attempt, 2 = attempted; ap = the AP type that succeeded)
        if(ftrace.gt.0. .and. abs(f1-ftrace).le.25.) write(*,3060) ipass,nthr,f1,xdt-0.5,sync,lhighsens,nbadcrc,   &
                                                                    iaptype,ift8stage,trim(msg37)
3060    format('CANDTRACE ft8b pass',i2,' thr',i3,' f',f8.1,' dt',f6.2,' sync',f7.2,' hs',l2,' crc',i2,   &
               ' ap',i3,' stage',i2,' msg ',a)
        if(lretrymode) then   ! CE3TSK: trace of the retry outcomes
          istage=ift8stage; if(nbadcrc.eq.0) istage=3
!$omp atomic
          nretrystage(istage)=nretrystage(istage)+1
        endif
        nsnr=nint(xsnr)
        xdt=xdt-0.5
        !call timer('ft8b    ',1)
        if(nbadcrc.eq.0) then
          if(ldectrace) write(0,3070) ipass,nthr,f1,xdt,nint(xsnr),trim(msg37)
3070      format('DECTRACE pass',i2,' thr',i3,' f',f8.1,' dt',f6.2,' snr',i4,' ',a)
          lhidemsg=.false.
          if(lhidetelemetry .and. i3.eq.0 .and. n3.eq.5) lhidemsg=.true.
          if(lhidetest) then
            if((i3.eq.0 .and. n3.gt.1 .and. n3.lt.5) .or. i3.eq.3 .or. i3.gt.4) then
              if(mycalllen1.lt.4 .or. msg37(1:mycalllen1).ne.trim(mycall)//' ') lhidemsg=.true.
            endif
            if(msg37(1:3).eq.'CQ ') then
              if(msg37(1:6).eq.'CQ RU ' .or. msg37(1:6).eq.'CQ FD ' .or. msg37(1:8).eq.'CQ TEST ') lhidemsg=.true.
            endif
          endif

          if(lspecial) then; nspecial=2; else; nspecial=1; endif
          do k=1,nspecial
            if(k.eq.2) msg37=msg37_2
            ldupe=.false.
            if(msg37(1:6).eq."      ") ldupe=.true. 
            if(.not.ldupe .and. ndecodes.gt.0) then
              do idec=1,ndecodes
                if(lhideft8dupes) then
                  if(msg37.eq.allmessages(idec) .and. (nsnr.le.allsnrs(idec) .or. &
                     (nsnr.gt.allsnrs(idec) .and. abs(allfreq(idec)-f1).lt.45.0))) then
                    ldupe=.true.; exit
                  endif
                else
! CE3TSK: a later pass, member or background unit never re-prints a message (lsecondpass): not
! for a better SNR - at a single thread the perturbed bands would repeat most of the band - and
! not at another frequency either, where identical text is the same transmitter seen through
! a harmonic or an image (the residual pass found one 2 kHz up)
                  if(msg37.eq.allmessages(idec) .and. (lsecondpass .or. (nsnr.le.allsnrs(idec) .and. &
                     abs(allfreq(idec)-f1).lt.45.0) .or. (nsnr.gt.allsnrs(idec) .and. &
                     abs(allfreq(idec)-f1).lt.45.0 .and. numthreads.ne.1))) then
                    ldupe=.true.; exit
                  endif
                endif
              enddo
            endif
! CE3TSK: with the emission buffered, allmessages holds only earlier passes' committed
! winners; this pass's own decodes sit in the slice's buffer - same rule as above, and the
! numthreads clause is true by construction (buffering is only on above one thread).
! Cross-slice copies are deliberately NOT checked here: emit() resolves them in slice order.
            if(.not.ldupe .and. lft8buffered .and. nft8res(nthr).gt.0) then
              do idec=1,nft8res(nthr)
                if(lhideft8dupes) then
                  if(msg37.eq.ft8res(idec,nthr)%msg .and. (nsnr.le.ft8res(idec,nthr)%nsnr .or. &
                     (nsnr.gt.ft8res(idec,nthr)%nsnr .and. abs(ft8res(idec,nthr)%f1-f1).lt.45.0))) then
                    ldupe=.true.; exit
                  endif
                else
                  if(msg37.eq.ft8res(idec,nthr)%msg .and. (lsecondpass .or. &
                     abs(ft8res(idec,nthr)%f1-f1).lt.45.0)) then
                    ldupe=.true.; exit
                  endif
                endif
              enddo
            endif
            if(.not.ldupe) then
              if(.not.lFreeText .and. k.eq.1) call extract_call(msg37,call2)
              if(lft8buffered) then
! CE3TSK: buffered emission - nothing shared is touched here. The marker and the even/odd
! store condition are decided now, with every local in scope; the duplicate check, the
! callback and all the bookkeeping happen in emit(), in slice order, after the parallel
! loop (FT8_EMISSION_ORDER.md). The marker logic below is the inline path's, verbatim.
                if(iaptype.eq.0) then
                  if(.not.lFreeText .or. lspecial) servis8=' '
                  if(.not.lspecial .and. lFreeText) then
                    if(abs(nfqso-nint(f1)).le.10) then; servis8=','; else; servis8='.'; endif
                  endif
                  if(lft8sd .or. lft8s) servis8='^'
                else
                  if(lft8sd .or. lft8s) then; servis8='^'; else; servis8='*'; endif
                endif
                if(i3bit.eq.1) servis8='1'
                if(nbgrun.gt.0) then
                  if(lft8sd .or. lft8s) then; servis8='#'; else; servis8='|'; endif
                endif
                nloc8=0
                if(i3.eq.4 .and. msg37(1:3).eq.'CQ ' .and. mod(nsec,15).eq.0) then
                  nloc8=1
                else if(.not.lFreeText) then
                  ispc1=index(msg37,' ')
                  if(.not.lhashmsg .and. mod(nsec,15).eq.0 .and. ((i3.eq.1 .and. .not.lft8sd) .or. lft8sd) .and. &
                     msg37(1:ispc1-1).ne.trim(mycall) .and. index(msg37,'<').le.0) then
                    if(index(msg37,'/').le.0 .or. msg37(1:3).eq.'CQ ') nloc8=2   ! compound not supported
                  endif
                endif
                if(nft8res(nthr).lt.NDEC8MAX) then
                  nft8res(nthr)=nft8res(nthr)+1; ir8=nft8res(nthr)
                  ft8res(ir8,nthr)%msg=msg37; ft8res(ir8,nthr)%call2=call2
                  ft8res(ir8,nthr)%srv=servis8; ft8res(ir8,nthr)%nsnr=nsnr; ft8res(ir8,nthr)%nloc=nloc8
                  ft8res(ir8,nthr)%xdt=xdt; ft8res(ir8,nthr)%f1=f1
                  ft8res(ir8,nthr)%lemit=.not.lhidemsg
                  ft8res(ir8,nthr)%ih=0; ft8res(ir8,nthr)%kh=0
                  if(k.eq.1 .and. ihint8.gt.0) then   ! the hint entry this decode used
                    ft8res(ir8,nthr)%ih=ihint8; ft8res(ir8,nthr)%kh=khint8
                  endif
                else if(.not.lhidemsg .and. associated(this%callback)) then
! the buffer holds 200 a slice; past that a decode still prints, outside the merge, as the
! allmessages cap has always let an unremembered decode print
                  msg26=msg37(1:26)
                  call this%callback(nsnr,xdt,f1,msg26,servis8)
                endif
                if(msg37(1:3).eq.'CQ ' .and. nmsgcq.lt.numdeccq) then
                  nmsgcq=nmsgcq+1; xdtr=xdt+0.5
                  tmpcqdec(nmsgcq)%freq=f1; tmpcqdec(nmsgcq)%xdt=xdtr
                endif
                if(lapmyc .and. lmycallstd) then
                  ispc1=index(msg37,' ')
                  if(msg37(1:ispc1-1).eq.trim(mycall) .and. nmsgmyc.lt.numdecmyc) then
                    nmsgmyc=nmsgmyc+1; xdtr=xdt+0.5
                    tmpmyc(nmsgmyc)%freq=f1; tmpmyc(nmsgmyc)%xdt=xdtr
                  endif
                endif
              else
!$omp critical(update_arrays)
! CE3TSK: the decode list holds 200; past that (a very busy band with ensemble members)
! a decode still prints but is no longer remembered for the duplicate check
              if(ndecodes.lt.size(allmessages)) then
                ndecodes=ndecodes+1; allmessages(ndecodes)=msg37; allsnrs(ndecodes)=nsnr; allfreq(ndecodes)=f1
              endif
              if(.not.lhidemsg) then
 ! simulated wav tests affected, structure contains data for at least previous and current even|odd intervals
                if(levenint) then
                  calldteven(150:2:-1)=calldteven(150-1:1:-1); calldteven(1)%call2=call2; calldteven(1)%dt=xdt
                else if(loddint) then
                  calldtodd(150:2:-1)=calldtodd(150-1:1:-1); calldtodd(1)%call2=call2; calldtodd(1)%dt=xdt
                endif
              endif
!$omp end critical(update_arrays)
              if(.not.lhidemsg) then
                if(iaptype.eq.0) then
                  if(.not.lFreeText .or. lspecial) servis8=' '
                  if(.not.lspecial .and. lFreeText) then
                    if(abs(nfqso-nint(f1)).le.10) then; servis8=','; else; servis8='.'; endif
                  endif
                  if(lft8sd .or. lft8s) servis8='^'
                else
                  if(lft8sd .or. lft8s) then; servis8='^'; else; servis8='*'; endif
                endif
                if(i3bit.eq.1) servis8='1'
! CE3TSK: a pipeline message - decoded in the TX background phase: '|', or '#' for a hint
! decode there, which the callback prints as the box-drawing cross (a pipe with the hint's
! caret through it; three UTF-8 bytes, hence the placeholder in this one-byte marker)
                if(nbgrun.gt.0) then
                  if(lft8sd .or. lft8s) then; servis8='#'; else; servis8='|'; endif
                endif
!write (*,"(F5.2,1x,I1,1x,I4,1x,F4.2)") candidate(2,icand)-0.5,ipass,nint(candidate(1,icand)),candidate(3,icand)
!write (*,"(I1,1x,F4.2)") ipass,candidate(3,icand)
!print *,candidate(2,icand)-0.5,msg37
!print *,msg37
                msg26=msg37(1:26)
                if(associated(this%callback)) call this%callback(nsnr,xdt,f1,msg26,servis8)
              endif

              if(msg37(1:3).eq.'CQ ' .and. nmsgcq.lt.numdeccq) then
                nmsgcq=nmsgcq+1; xdtr=xdt+0.5
                tmpcqdec(nmsgcq)%freq=f1; tmpcqdec(nmsgcq)%xdt=xdtr
              endif
              if(lapmyc .and. lmycallstd) then
                ispc1=index(msg37,' ')
                if(msg37(1:ispc1-1).eq.trim(mycall) .and. nmsgmyc.lt.numdecmyc) then
                  nmsgmyc=nmsgmyc+1; xdtr=xdt+0.5
                  tmpmyc(nmsgmyc)%freq=f1; tmpmyc(nmsgmyc)%xdt=xdtr
                endif
              endif

              if(i3.eq.4 .and. msg37(1:3).eq.'CQ ' .and. mod(nsec,15).eq.0 .and. nmsgloc.lt.130) then
                nmsgloc=nmsgloc+1
                if(levenint) then
                  eventmp(nmsgloc)%msg=msg37; eventmp(nmsgloc)%freq=f1
                  eventmp(nmsgloc)%dt=xdt; eventmp(nmsgloc)%lstate=.true.
                endif
                if(loddint) then
                  oddtmp(nmsgloc)%msg=msg37; oddtmp(nmsgloc)%freq=f1
                  oddtmp(nmsgloc)%dt=xdt; oddtmp(nmsgloc)%lstate=.true.
                endif
                go to 4 ! tmp filled in
              endif
              if(.not.lFreeText) then ! protection against any possible free txtmsg bit corruption
                ispc1=index(msg37,' ')
                if(.not.lhashmsg .and. mod(nsec,15).eq.0 .and. ((i3.eq.1 .and. .not.lft8sd) .or. lft8sd) .and. &
                   msg37(1:ispc1-1).ne.trim(mycall) .and. nmsgloc.lt.130 .and. index(msg37,'<').le.0) then
                  if(index(msg37,'/').gt.0 .and. msg37(1:3).ne.'CQ ') go to 4 ! compound not supported
                  nmsgloc=nmsgloc+1
                  if(levenint) then
                    eventmp(nmsgloc)%msg=msg37; eventmp(nmsgloc)%freq=f1
                    eventmp(nmsgloc)%dt=xdt; eventmp(nmsgloc)%lstate=.true.
                  endif
                  if(loddint) then
                    oddtmp(nmsgloc)%msg=msg37; oddtmp(nmsgloc)%freq=f1
                    oddtmp(nmsgloc)%dt=xdt; oddtmp(nmsgloc)%lstate=.true.
                  endif
                endif
              endif
              endif   ! lft8buffered
            endif
4           continue
          enddo
        endif
      enddo
      ncandthr=ncandthr+ncand
    enddo
    if(lcollectdelta .and. npass.lt.4) dd8delta(:,nthr)=dd8-dd8orig   ! CE3TSK: 3-pass run never averaged
! h=default_header(12000,NMAX)
! open(10,file='subtract.wav',status='unknown',access='stream')
! iwave(1:180000)=nint(dd8(1:180000))
! write(10) h,iwave
! close(10)
    if(ldectrace2) then   ! CE3TSK diagnostic: the invisible carried state this slice exports
      do ick=1,ncqsignal
        write(0,'(a,i3,a,i3,a,f8.1,a,f6.2,a,es16.8)') 'CQSIG thr',nthr,' n',ick,' f',tmpcqsig(ick)%freq, &
             ' dt',tmpcqsig(ick)%xdt,' ck ',sum(abs(tmpcqsig(ick)%cs))
      enddo
      do ick=1,nmycsignal
        write(0,'(a,i3,a,i3,a,f8.1,a,f6.2)') 'MYSIG thr',nthr,' n',ick,' f',tmpmycsig(ick)%freq,' dt',tmpmycsig(ick)%xdt
      enddo
      write(0,'(a,i3,a,i3,a,i3,a,i4,a,i4)') 'SLSTATE thr',nthr,' ncq',ncqsignal,' nmyc',nmycsignal, &
           ' nmsgloc',nmsgloc,' nincall',nincallthr(nthr)
    endif
    if(.not.lsecondpass) then   ! CE3TSK: pass 1 already exported the CQ/MyCall/QSO signals of this thread
    if(levenint) then
      evencq(1:ncqsignal,nthr)%freq=tmpcqsig(1:ncqsignal)%freq
      evencq(1:ncqsignal,nthr)%xdt=tmpcqsig(1:ncqsignal)%xdt
      do ik=1,ncqsignal; evencq(ik,nthr)%cs=tmpcqsig(ik)%cs; enddo
      if(lapmyc) then
        evenmyc(1:nmycsignal,nthr)%freq=tmpmycsig(1:nmycsignal)%freq
        evenmyc(1:nmycsignal,nthr)%xdt=tmpmycsig(1:nmycsignal)%xdt
        do ik=1,nmycsignal; evenmyc(ik,nthr)%cs=tmpmycsig(ik)%cs; enddo
        if(.not.lqsomsgdcd .and. tmpqsosig(1)%freq.lt.5001.) then
          evenqso(1,nthr)%freq=tmpqsosig(1)%freq; evenqso(1,nthr)%xdt=tmpqsosig(1)%xdt
          evenqso(1,nthr)%cs=tmpqsosig(1)%cs
        endif
      endif
    else if(loddint) then
      oddcq(1:ncqsignal,nthr)%freq=tmpcqsig(1:ncqsignal)%freq
      oddcq(1:ncqsignal,nthr)%xdt=tmpcqsig(1:ncqsignal)%xdt
      do ik=1,ncqsignal; oddcq(ik,nthr)%cs=tmpcqsig(ik)%cs; enddo
      if(lapmyc) then
        oddmyc(1:nmycsignal,nthr)%freq=tmpmycsig(1:nmycsignal)%freq
        oddmyc(1:nmycsignal,nthr)%xdt=tmpmycsig(1:nmycsignal)%xdt
        do ik=1,nmycsignal; oddmyc(ik,nthr)%cs=tmpmycsig(ik)%cs; enddo
        if(.not.lqsomsgdcd .and. tmpqsosig(1)%freq.lt.5001.) then
          oddqso(1,nthr)%freq=tmpqsosig(1)%freq; oddqso(1,nthr)%xdt=tmpqsosig(1)%xdt
          oddqso(1,nthr)%cs=tmpqsosig(1)%cs
        endif
      endif
    endif
    endif
    ncandthr=nint(float(ncandthr)/npass)
    ncandallthr(nthr)=ncandallthr(nthr)+ncandthr
    if(nmsgloc.gt.0) then
!$omp critical(update_structures)
      nadd=min(nmsgloc,130-nmsg)   ! CE3TSK: even/odd hold 130; decided inside the critical section
      if(nadd.gt.0) then
      if(levenint) then
        even(nmsg+1:nmsg+nadd)%msg=eventmp(1:nadd)%msg; even(nmsg+1:nmsg+nadd)%freq=eventmp(1:nadd)%freq
        even(nmsg+1:nmsg+nadd)%dt=eventmp(1:nadd)%dt; even(nmsg+1:nmsg+nadd)%lstate=eventmp(1:nadd)%lstate
        nmsg=nmsg+nadd
      else if(loddint) then
        odd(nmsg+1:nmsg+nadd)%msg=oddtmp(1:nadd)%msg; odd(nmsg+1:nmsg+nadd)%freq=oddtmp(1:nadd)%freq
        odd(nmsg+1:nmsg+nadd)%dt=oddtmp(1:nadd)%dt; odd(nmsg+1:nmsg+nadd)%lstate=oddtmp(1:nadd)%lstate
        nmsg=nmsg+nadd
      endif
      endif
!$omp end critical(update_structures)
    endif
!print *,'out',lastrxmsg(1)%lstate
!print *,lastrxmsg(1)%lastmsg
    return
  end subroutine decode

! CE3TSK: the pass's buffered decodes, merged in slice order and only now emitted
! (FT8_EMISSION_ORDER.md, TODO.md 2.2). Everything shared between slices - the duplicate
! arrays, the calldt lists, the even/odd hint-list store and the callback (which feeds
! my_ft8%xdtt and so next period's avexdt window) - is touched here and nowhere else, so
! the period's answer cannot depend on which thread finished first. Of two same-text copies
! the better SNR wins, slice order breaking ties: the winner's SNR, DT and frequency are
! what the operator, the log and the a-priori state see. Called once per slicing pass, from
! the serial region; inert when nothing was buffered. The one-thread path never buffers, so
! it never comes here and stays byte for byte what it was.
  subroutine ft8emit(this,nsl,nsec,lhideft8dupes)
    use ft8_mod1, only : ft8res,nft8res,NDEC8MAX,NSLICE8MAX,ndecodes,allmessages,allsnrs, &
                         allfreq,calldteven,calldtodd,even,odd,nmsg,lsecondpass,hint_consume
    class(ft8_decoder), intent(inout) :: this
    integer, intent(in) :: nsl,nsec
    logical(1), intent(in) :: lhideft8dupes
    integer iacc(2,NDEC8MAX*NSLICE8MAX),nacc,i,j,k,ka,ia
    logical ldup
    logical(1) leven,lodd
    character(len=26) msg26e
    leven=.false.; lodd=.false.
    if(nsec.eq.0 .or. nsec.eq.30) then; leven=.true.
    elseif(nsec.eq.15 .or. nsec.eq.45) then; lodd=.true.
    endif
! the hints the slices spent, retired now, in slice order - for every buffered decode,
! duplicates included, exactly as the inline path consumed on every hint decode
    do k=1,nsl
      do i=1,nft8res(k)
        if(ft8res(i,k)%ih.gt.0) call hint_consume(leven,lodd,ft8res(i,k)%ih,ft8res(i,k)%kh)
      enddo
    enddo
! winner selection, in slice order. Entries were already checked against allmessages (the
! earlier passes' committed winners) and against their own slice inside the loop; only the
! cross-slice copies are decided here, with the same rule the inline path applies - and a
! better-SNR copy replaces the accepted one in place, so its position in the emission order
! is the first slice that found the message.
    nacc=0
    do k=1,nsl
      do i=1,nft8res(k)
        ldup=.false.
        do j=1,nacc
          ka=iacc(1,j); ia=iacc(2,j)
          if(ft8res(i,k)%msg.eq.ft8res(ia,ka)%msg) then
            if(lhideft8dupes) then
              if(ft8res(i,k)%nsnr.le.ft8res(ia,ka)%nsnr .or. &
                 abs(ft8res(ia,ka)%f1-ft8res(i,k)%f1).lt.45.0) ldup=.true.
            else
              if(lsecondpass .or. abs(ft8res(ia,ka)%f1-ft8res(i,k)%f1).lt.45.0) ldup=.true.
            endif
            if(ldup) then
              if(ft8res(i,k)%nsnr.gt.ft8res(ia,ka)%nsnr) ft8res(ia,ka)=ft8res(i,k)
              exit
            endif
          endif
        enddo
        if(.not.ldup) then; nacc=nacc+1; iacc(1,nacc)=k; iacc(2,nacc)=i; endif
      enddo
    enddo
! commit, one winner at a time, in the order they were first accepted - the same per-decode
! sequence of bookkeeping the inline path performs
    do j=1,nacc
      ka=iacc(1,j); ia=iacc(2,j)
      if(ndecodes.lt.size(allmessages)) then
        ndecodes=ndecodes+1; allmessages(ndecodes)=ft8res(ia,ka)%msg
        allsnrs(ndecodes)=ft8res(ia,ka)%nsnr; allfreq(ndecodes)=ft8res(ia,ka)%f1
      endif
      if(ft8res(ia,ka)%lemit) then
        if(leven) then
          calldteven(150:2:-1)=calldteven(150-1:1:-1)
          calldteven(1)%call2=ft8res(ia,ka)%call2; calldteven(1)%dt=ft8res(ia,ka)%xdt
        else if(lodd) then
          calldtodd(150:2:-1)=calldtodd(150-1:1:-1)
          calldtodd(1)%call2=ft8res(ia,ka)%call2; calldtodd(1)%dt=ft8res(ia,ka)%xdt
        endif
        if(associated(this%callback)) then
          msg26e=ft8res(ia,ka)%msg(1:26)
          call this%callback(ft8res(ia,ka)%nsnr,ft8res(ia,ka)%xdt,ft8res(ia,ka)%f1,msg26e,ft8res(ia,ka)%srv)
        endif
      endif
      if(ft8res(ia,ka)%nloc.gt.0 .and. nmsg.lt.130) then   ! the even/odd lists hold 130, as before
        nmsg=nmsg+1
        if(leven) then
          even(nmsg)%msg=ft8res(ia,ka)%msg; even(nmsg)%freq=ft8res(ia,ka)%f1
          even(nmsg)%dt=ft8res(ia,ka)%xdt; even(nmsg)%lstate=.true.
        else if(lodd) then
          odd(nmsg)%msg=ft8res(ia,ka)%msg; odd(nmsg)%freq=ft8res(ia,ka)%f1
          odd(nmsg)%dt=ft8res(ia,ka)%xdt; odd(nmsg)%lstate=.true.
        endif
      endif
    enddo
    nft8res(1:nsl)=0
  end subroutine ft8emit
end module ft8_decode
