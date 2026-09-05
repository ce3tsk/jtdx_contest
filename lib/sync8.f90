subroutine sync8(nfa,nfb,syncmin,nfqso,candidate,ncand,jzb,jzt,swl,ipass,lqsothread,ncandthin,filter,ndtcenter)

  use ft8_mod1, only : dd8,windowx,facx,icos7,lagcc,lagccbail,nfawide,nfbwide
  use ft8_mod1, only : red_sh,red2_sh,jpeak_sh,jpeak2_sh,redcq_sh,base_sh,lsync8share   ! CE3TSK item 76
  use ft8_mod1, only : tsync8,tsync8s,tsync8l,tsync8p,nsync8p   ! CE3TSK timing
  use omp_lib
  real*8 :: t8a,t8b,t8c,t8d
  include 'ft8_params.f90'
  complex cx(0:NH1)
  real s(NH1,NHSYM),x(NFFT1),sync2d(NH1,jzb:jzt),red(NH1),candidate0(5,2000),candidate(4,2000),freq,rcandthin,dtcenter
  real, allocatable :: ssum(:,:)
  character(len=16) :: ctrace
  integer ltrace,itrace
  real ftrace
  equivalence (x,cx)
  integer jpeak(NH1),indx(NH1)
  integer jpeak2(NH1)   ! CE3TSK: second DT peak
  real red2(NH1),syncmin2
  integer, intent(in) :: nfa,nfb,nfqso,jzb,jzt,ipass,ncandthin,ndtcenter
  logical(1) syncq(NH1,jzb:jzt),redcq(NH1),lpass1,lpass2
  logical(1), intent(in) :: swl,lqsothread,filter

  tstep=0.04 ! NSTEP/12000.0                         
  df=3.125 ! 12000.0/NFFT1 , Hz
  candidate(4,:)=0.
  rcandthin=ncandthin/100.; if(filter) rcandthin=min(rcandthin*3.0,1.0)
  dtcenter=ndtcenter/100.
  t8a=omp_get_wtime()

! CE3TSK item 76: the wide-band part - shared across the slices for pass 1 when the driver
! computed it (lsync8share), otherwise computed here as before
  if(lsync8share .and. ipass.eq.1) then
    red=red_sh; red2=red2_sh; jpeak=jpeak_sh; jpeak2=jpeak2_sh; redcq=redcq_sh; base=base_sh
    t8c=omp_get_wtime(); t8d=t8c
  else
! the wide-band part inline, verbatim, for every other pass - measured: routing it through
! sync8_wide (an argument-passed band, host-associated arrays) cost ~20 % per call, 16 calls
! a pass, which ate the pass-1 saving; the shared copy in sync8_wide is the same text
  syncq=.false.; redcq=.false.
  if(ipass.eq.1 .or. ipass.eq.4 .or. ipass.eq.7) then
    do j=1,NHSYM
      ia=(j-1)*NSTEP + 1
      ib=ia+NSPS-1
      x(1:759)=0.
      if(j.ne.1) then; x(760:960)=dd8(ia-201:ia-1)*windowx(200:0:-1); else; x(760:960)=0.; endif
      x(961:2880)=facx*dd8(ia:ib); x(961)=x(961)*1.9; x(2880)=x(2880)*1.9
      if(j.ne.NHSYM) then; x(2881:3081)=dd8(ib+1:ib+201)*windowx; else; x(2881:3081)=0.; endif
      x(3082:)=0.
      call four2a(cx,NFFT1,1,-1,0)              !r2c FFT
      do i=1,NH1
        s(i,j)=SQRT(real(cx(i))**2 + aimag(cx(i))**2)
      enddo
    enddo
  endif
  if(ipass.eq.2 .or. ipass.eq.5 .or. ipass.eq.8) then
    do j=1,NHSYM
      ia=(j-1)*NSTEP + 1
      ib=ia+NSPS-1
      x(1:759)=0.
      if(j.ne.1) then; x(760:960)=dd8(ia-201:ia-1)*windowx(200:0:-1); else; x(760:960)=0.; endif
      x(961:2880)=facx*dd8(ia:ib); x(961)=x(961)*1.9; x(2880)=x(2880)*1.9
      if(j.ne.NHSYM) then; x(2881:3081)=dd8(ib+1:ib+201)*windowx; else; x(2881:3081)=0.; endif
      x(3082:)=0.
      call four2a(cx,NFFT1,1,-1,0)              !r2c FFT
      do i=1,NH1
        s(i,j)=real(cx(i))**2 + aimag(cx(i))**2
      enddo
    enddo
  endif
  if(ipass.eq.3 .or. ipass.eq.6 .or. ipass.eq.9) then
    do j=1,NHSYM
      ia=(j-1)*NSTEP + 1
      ib=ia+NSPS-1
      x(1:759)=0.
      if(j.ne.1) then; x(760:960)=dd8(ia-201:ia-1)*windowx(200:0:-1); else; x(760:960)=0.; endif
      x(961:2880)=facx*dd8(ia:ib); x(961)=x(961)*1.9; x(2880)=x(2880)*1.9
      if(j.ne.NHSYM) then; x(2881:3081)=dd8(ib+1:ib+201)*windowx; else; x(2881:3081)=0.; endif
      x(3082:)=0.
      call four2a(cx,NFFT1,1,-1,0)              !r2c FFT
      do i=1,NH1
        s(i,j)=abs(real(cx(i))) + abs(aimag(cx(i)))
      enddo
    enddo
  endif

  iaw=max(1,nint(nfawide/df)); ibw=max(1,nint(nfbwide/df))
  nssy=4 ! NSPS/NSTEP   ! # steps per symbol
  nssy36=144 ! nssy*36
  nssy72=288 ! nssy*72
  nfos=2 ! NFFT1/NSPS   ! # frequency bin oversampling factor
  jstrt=12.5 ! 0.5/tstep

  t8c=omp_get_wtime()
  if(lagcc .and. .not.lagccbail) then
    nfos6=12 ! nfos*6
! CE3TSK: sum(s(i:i+nfos6:nfos,k)) depends on the bin and the symbol only; it was recomputed
! for every DT lag. Tabulated once with the identical expression, so every value is bit for bit
! what the loop computed before. Same below for the plain metric.
    allocate(ssum(iaw:ibw,NHSYM))
    do k=1,NHSYM
      do i=iaw,ibw
        ssum(i,k)=sum(s(i:i+nfos6:nfos,k))
      enddo
    enddo
      do j=jzb,jzt; call lag_agc(j); enddo
  else
!    nfos6=15 ! 16i spec bw -1
    nfos6=16
    allocate(ssum(iaw:ibw,NHSYM))
    do k=1,NHSYM
      do i=iaw,ibw
        ssum(i,k)=sum(s(i:i+nfos6,k))
      enddo
    enddo
      do j=jzb,jzt; call lag_plain(j); enddo
  endif

  if(allocated(ssum)) deallocate(ssum)
  t8d=omp_get_wtime()
  red=0.; red2=0.; jpeak2=0; jpeak=0
    do i=iaw,ibw; call peak_bin(i); enddo

  iz=ibw-iaw+1
  call indexx(red(iaw:ibw),iz,indx)
  ibase=indx(max(1,nint(0.40*iz))) - 1 + iaw ! max is workaround to prevent indx getting out of bounds
  base=red(ibase)
  if(base.lt.1e-8) base=1.0 ! safe division
  red=red/base; red2=red2/base

! CE3TSK diagnostic hook: JTDX_CAND_TRACE=<Hz> prints the normalised sync metric of every bin
! within +-25 Hz of that frequency, each pass, before the threshold is applied (the candidates
! that result are traced in ft8_decode.f90 with the decoder's verdict on them).
  call get_environment_variable('JTDX_CAND_TRACE',ctrace,ltrace,itrace)
  ftrace=0.; if(itrace.eq.0 .and. ltrace.gt.0) read(ctrace(1:ltrace),*,iostat=itrace) ftrace
  if(itrace.ne.0) ftrace=0.   ! an unreadable value switches the trace off
  if(ftrace.gt.0.) then
    do i=max(iaw,nint((ftrace-25.)/df)),min(ibw,nint((ftrace+25.)/df))
      write(*,3050) ipass,i*df,red(i),(jpeak(i)-1)*tstep,red2(i),(jpeak2(i)-1)*tstep,syncmin,base
3050  format('CANDTRACE sync8 pass',i2,' f',f8.1,' red',f7.2,' dt',f6.2,' red2',f7.2,' dt2',f6.2,' syncmin',f5.2,' base',es10.2)
    enddo
  endif

  endif
  ia=max(1,nint(nfa/df)); ib=max(1,nint(nfb/df))

  candidate0=0.; k=0; iz=ib-ia+1; lpass1=.false.; lpass2=.false.
  if(rcandthin.lt.0.99) then
    if(ipass.eq.1 .or. ipass.eq.4 .or. ipass.eq.7) then; lpass1=.true.
    else if(ipass.eq.2 .or. ipass.eq.5 .or. ipass.eq.8) then; lpass2=.true.
    endif
  endif
  call indexx(red(ia:ib),iz,indx)
  do i=1,iz
    n=ia + indx(iz+1-i) - 1
    freq=n*df
    if(abs(freq-nfqso).gt.3.0) then
      if (red(n).lt.syncmin) cycle
    else
      if (red(n).lt.1.1) cycle
    endif
    if(swl) then
      if(jpeak(n).lt.-74 .or. jpeak(n).gt.101) cycle
    else
      if(jpeak(n).lt.-49 .or. jpeak(n).gt.76) cycle
    endif
    if(k.lt.2000) then; k=k+1; else; exit; endif  ! CE3TSK: was 450 - filled by adjacent-bin echoes of strong signals in a pileup
! being sorted by sync
    candidate0(1,k)=freq
    candidate0(2,k)=(jpeak(n)-1)*tstep
    candidate0(3,k)=red(n)
    if(rcandthin.lt.0.99) then ! candidate thinning option
      if(lpass2) then
        candidate0(5,k)=candidate0(3,k)/(abs(candidate0(2,k)-dtcenter)+1.0)**2
      else
        candidate0(5,k)=candidate0(3,k)/(abs(candidate0(2,k)-dtcenter)+1.0)
      endif
    endif
    if(redcq(n)) candidate0(4,k)=2.
    syncmin2=syncmin; if(abs(freq-nfqso).le.3.0) syncmin2=1.1   ! same threshold rule as the main peak
    if(jpeak2(n).ne.jpeak(n) .and. red2(n).ge.syncmin2 .and. k.lt.2000) then   ! CE3TSK: second DT peak
      k=k+1
      candidate0(1,k)=freq
      candidate0(2,k)=(jpeak2(n)-1)*tstep
      candidate0(3,k)=red2(n)
      if(rcandthin.lt.0.99) then   ! same DT weighting as the main peak
        if(lpass2) then
          candidate0(5,k)=candidate0(3,k)/(abs(candidate0(2,k)-dtcenter)+1.0)**2
        else
          candidate0(5,k)=candidate0(3,k)/(abs(candidate0(2,k)-dtcenter)+1.0)
        endif
      endif
    endif
  enddo
  ncand=k

  fdif0=4.0; if(swl) fdif0=3.0
  xdtdelta=0.0
! save sync only to the best of near-dupe freqs 
  do i=1,ncand
    if(i.ge.2) then
      do j=1,i-1
        fdiff=abs(candidate0(1,i)-candidate0(1,j))
        xdtdelta=abs(candidate0(2,i)-candidate0(2,j))
        if(fdiff.lt.fdif0 .and. abs(candidate0(1,i)-nfqso).gt.3.0) then
          if(xdtdelta.lt.0.1) then
            if(candidate0(3,i).ge.candidate0(3,j)) candidate0(3,j)=0.
            if(candidate0(3,i).lt.candidate0(3,j)) candidate0(3,i)=0.
          endif
        endif
      enddo
!        write(*,3001) i,candidate0(1,i-1),candidate0(1,i),candidate0(3,i-1),  &
!             candidate0(3,i)
!3001    format(i2,4f8.1)
    endif
  enddo

! Sort by sync
!  call indexx(candidate0(3,1:ncand),ncand,indx)
  if(rcandthin.gt.0.99) then; call indexx(candidate0(3,1:ncand),ncand,indx)
! sort by sync value with DT weight
  else; call indexx(candidate0(5,1:ncand),ncand,indx)
  endif
! Sort by frequency 
!  call indexx(candidate0(1,1:ncand),ncand,indx)

  k=1; ncandfqso=0
!Put nfqso at top of list and apply lowest sync threshold for nfqso
  fprev=5004.
  do i=ncand,1,-1
    j=indx(i)
    if(abs(candidate0(1,j)-nfqso).le.3.0 .and. candidate0(3,j).ge.1.1 .and. abs(candidate0(1,j)-fprev).gt.3.0) then
      candidate(1,k)=candidate0(1,j); candidate(2,k)=candidate0(2,j)
      candidate(3,k)=candidate0(3,j); candidate(4,k)=candidate0(4,j)
      fprev=candidate0(1,j)
      k=k+1; ncandfqso=ncandfqso+1
    endif
  enddo

!put virtual candidates for FT8S decoder
  if(lqsothread) then
    candidate(1,k)=float(nfqso)
    candidate(2,k)=5.0 ! xdt
    candidate(3,k)=0.0 ! sync
    k=k+1; ncandfqso=ncandfqso+1
    candidate(1,k)=float(nfqso)
    candidate(2,k)=-5.0
    candidate(3,k)=0.0
    k=k+1; ncandfqso=ncandfqso+1
  endif

  do i=ncand,1,-1
    j=indx(i)
    if(abs(candidate0(1,j)-nfqso).gt.3.0) then; syncmin1=syncmin; else; syncmin1=1.1; endif
    if(candidate0(3,j) .ge. syncmin1) then
      candidate(1,k)=candidate0(1,j); candidate(2,k)=candidate0(2,j)
      candidate(3,k)=candidate0(3,j); candidate(4,k)=candidate0(4,j)
      k=k+1
      if(k.gt.2000) exit  ! CE3TSK: was 460 
    endif
  enddo
  ncand=k-1
  if(ncand-ncandfqso.gt.1 .and. rcandthin.lt.0.99) then
! applying decoding pass weight factor, derived from number of candidates in each pass
    if(lpass1) then; rcandthin=min(rcandthin*1.27,1.0)
    else if(lpass2) then
      if(rcandthin.gt.0.79) then; rcandthin=rcandthin**2
      else; rcandthin=rcandthin*0.79
      endif
    else; rcandthin=min(rcandthin*5.0,1.0) ! ipass 3,6,9
    endif
    ncand=ncandfqso+nint((ncand-ncandfqso)*rcandthin)
  endif

  t8b=omp_get_wtime()
!$omp atomic
  tsync8=tsync8+(t8b-t8a)
!$omp atomic
  tsync8s=tsync8s+(t8c-t8a)
!$omp atomic
  tsync8l=tsync8l+(t8d-t8c)
  if(ipass.ge.0 .and. ipass.le.15) then   ! CE3TSK: per pass, for TODO.md item 2.1
!$omp atomic
     tsync8p(ipass)=tsync8p(ipass)+(t8b-t8a)
!$omp atomic
     nsync8p(ipass)=nsync8p(ipass)+1
  endif
  return

contains
  subroutine lag_agc(j)
    integer, intent(in) :: j
    integer i,n,k,k36,k72
    real ta,tb,tc,tcq,t0a,t0b,t0c,t0cq,t1,t01,t2,t02,sync01,sync02,syncf,syncs,sya,sycq,sybc,sy1,sy2,sync_abc,sync_bc,tall(30)
    logical(1) lcq,lcq2
      do i=iaw,ibw
        ta=0.; tb=0.; tc=0.
        do n=0,6
          k=j+jstrt+nssy*n
          if(k.gt.0) then
            ta=s(i+nfos*icos7(n),k)
            if(ta.gt.1e-9) then; tall(n+1)=ta*6.0/(ssum(i,k)-ta); else; tall(n+1)=0.; endif
          endif
          k36=k+nssy36
          if(k36.gt.0 .and. k36.le.NHSYM) then
            tb=s(i+nfos*icos7(n),k36)
            if(tb.gt.1e-9) then; tall(n+17)=tb*6.0/(ssum(i,k36)-tb); else; tall(n+17)=0.; endif
          endif
          k72=k+nssy72
          if(k72.le.NHSYM) then
            tc=s(i+nfos*icos7(n),k72)
            if(tc.gt.1e-9) then; tall(n+24)=tc*6.0/(ssum(i,k72)-tc); else; tall(n+24)=0.; endif
          endif
        enddo
        lcq=.false.
        if(ipass.gt.1) then
          do n=7,15
            k=j+jstrt+nssy*n
            if(k.gt.0) then
              if(n.lt.15) then
                tall(n+1)=s(i,k)*6.0/(ssum(i,k)-s(i,k))
              else
                tall(n+1)=s(i+2,k)*6.0/(ssum(i,k)-s(i+2,k))
              endif
            endif
          enddo
          sya=sum(tall(1:7)); sycq=sum(tall(8:16)); sybc=sum(tall(17:30))
          sy1=(sya+sycq+sybc)/30.; sy2=(sya+sybc)/21.; sync_abc=max(sy1,sy2)
          sy1=(sycq+sybc)/23.; sy2=(sybc)/14.; sync_bc=max(sy1,sy2); if(sy1.gt.sy2) lcq=.true.
        else
          sybc=sum(tall(17:30)); sync_abc=sum(tall(1:7))+sybc; sync_bc=sybc/14.; sync_abc=sync_abc/21.
        endif
        sync2d(i,j)=max(sync_abc,sync_bc); if(lcq) syncq(i,j)=.true.
      enddo
  end subroutine lag_agc

  subroutine lag_plain(j)
    integer, intent(in) :: j
    integer i,n,k,k36,k72
    real ta,tb,tc,tcq,t0a,t0b,t0c,t0cq,t1,t01,t2,t02,sync01,sync02,syncf,syncs,sya,sycq,sybc,sy1,sy2,sync_abc,sync_bc,tall(30)
    logical(1) lcq,lcq2
      do i=iaw,ibw
        ta=0.; tb=0.; tc=0.; tcq=0.; t0a=0.; t0b=0.; t0c=0.; t0cq=0.
        do n=0,6
          k=j+jstrt+nssy*n
          if(k.gt.0) then; ta=ta + s(i+nfos*icos7(n),k); t0a=t0a + ssum(i,k) - s(i+nfos*icos7(n)+1,k); endif
          k36=k+nssy36
          if(k36.gt.0 .and. k36.le.NHSYM) then
            tb=tb + s(i+nfos*icos7(n),k36); t0b=t0b + ssum(i,k36) - s(i+nfos*icos7(n)+1,k36)
          endif
          k72=k+nssy72
          if(k72.le.NHSYM) then
            tc=tc + s(i+nfos*icos7(n),k72)
            t0c=t0c + ssum(i,k72) - s(i+nfos*icos7(n)+1,k72)
          endif
        enddo
        do n=7,15
          k=j+jstrt+nssy*n
          if(k.ge.1) then
            if(n.lt.15) then; tcq=tcq + s(i,k); t0cq=t0cq + ssum(i,k) - s(i,k+1)
            else; tcq=tcq + s(i+2,k); t0cq=t0cq + ssum(i,k) - s(i,k+3)
            endif
          endif
        enddo
        t1=ta+tb+tc; t01=t0a+t0b+t0c; t2=t1+tcq; t02=t01+t0cq
        t01=(t01-t1*2)/42.0; if(t01.lt.1e-8) t01=1.0; t02=(t02-t2*2)/60.0; if(t02.lt.1e-8) t02=1.0 ! safe division
        sync01=t1/(7.0*t01); sync02=(t1/7.0 + tcq/9.0)/t02; syncf=max(sync01,sync02)
        lcq=.false.; if(sync02.gt.sync01) lcq=.true.
        t1=tb+tc; t01=t0b+t0c; t2=t1+tcq; t02=t01+t0cq
        t01=(t01-t1*2)/28.0; if(t01.lt.1e-8) t01=1.0; t02=(t02-t2*2)/46.0; if(t02.lt.1e-8) t02=1.0 ! safe division
        sync01=t1/(7.0*t01); sync02=(t1/7.0 + tcq/9.0)/t02; syncs=max(sync01,sync02)
        lcq2=.false.; if(sync02.gt.sync01) lcq2=.true.
        sync2d(i,j)=max(syncf,syncs)
        if(syncf.gt.syncs) then; if(lcq) syncq(i,j)=.true.; else; if(lcq2) syncq(i,j)=.true.; endif
      enddo
  end subroutine lag_plain

  subroutine peak_bin(i)
    integer, intent(in) :: i
    integer ii(1),j0,jlo,jhi
    ii=maxloc(sync2d(i,jzb:jzt)) - 1 + jzb
    j0=ii(1)
    jpeak(i)=j0
    red(i)=sync2d(i,j0); if(syncq(i,j0)) redcq(i)=.true.
! CE3TSK: a second sync peak per bin, restricted to |DT| <= 13 steps (0.52 s), as WSJT-X's
! classic sync8 does. One peak per bin loses a signal whose DT peak is hidden by a
! stronger co-channel signal at another DT; the near-DT peak becomes its own candidate.
    jlo=max(jzb,-13); jhi=min(jzt,13)
    if(jlo.le.jhi) then
      ii=maxloc(sync2d(i,jlo:jhi)) - 1 + jlo
      jpeak2(i)=ii(1); red2(i)=sync2d(i,jpeak2(i))
    else   ! the DT window has drifted past +-0.52 s (avexdt beyond +-3 s): no second peak
      jpeak2(i)=jpeak(i); red2(i)=0.
    endif
  end subroutine peak_bin

end subroutine sync8

! CE3TSK item 76: the wide-band part of sync8 - the symbol spectra, the sync surface over the
! wide span, the per-bin peaks and their baseline normalisation - which depends on the band,
! the DT window and the pass only, never on the slice. Called per slice (lpar false, serial:
! it runs inside the slice loop's parallel region) or once per slicing by the driver on the
! band every slice starts from (lpar true: its lag and peak loops run in parallel, so the
! hoist does not add back as serial time what it removes from every slice's chain).
subroutine sync8_wide(dd,jzb,jzt,ipass,lpar,syncmin,red,red2,jpeak,jpeak2,redcq,base,t8c,t8d)
  use ft8_mod1, only : windowx,facx,icos7,lagcc,lagccbail,nfawide,nfbwide
  use omp_lib
  include 'ft8_params.f90'
  real, intent(in) :: dd(*)
  integer, intent(in) :: jzb,jzt,ipass
  logical, intent(in) :: lpar
  real, intent(in) :: syncmin
  real, intent(out) :: red(NH1),red2(NH1),base
  integer, intent(out) :: jpeak(NH1),jpeak2(NH1)
  logical(1), intent(out) :: redcq(NH1)
  real*8, intent(out) :: t8c,t8d
  complex cx(0:NH1)
  real s(NH1,NHSYM),x(NFFT1),sync2d(NH1,jzb:jzt)
  integer indx(NH1)
  real, allocatable :: ssum(:,:)
  character(len=16) :: ctrace
  integer ltrace,itrace
  real ftrace
  logical(1) syncq(NH1,jzb:jzt)
  equivalence (x,cx)
  tstep=0.04; df=3.125
  syncq=.false.; redcq=.false.
  if(ipass.eq.1 .or. ipass.eq.4 .or. ipass.eq.7) then
    do j=1,NHSYM
      ia=(j-1)*NSTEP + 1
      ib=ia+NSPS-1
      x(1:759)=0.
      if(j.ne.1) then; x(760:960)=dd(ia-201:ia-1)*windowx(200:0:-1); else; x(760:960)=0.; endif
      x(961:2880)=facx*dd(ia:ib); x(961)=x(961)*1.9; x(2880)=x(2880)*1.9
      if(j.ne.NHSYM) then; x(2881:3081)=dd(ib+1:ib+201)*windowx; else; x(2881:3081)=0.; endif
      x(3082:)=0.
      call four2a(cx,NFFT1,1,-1,0)              !r2c FFT
      do i=1,NH1
        s(i,j)=SQRT(real(cx(i))**2 + aimag(cx(i))**2)
      enddo
    enddo
  endif
  if(ipass.eq.2 .or. ipass.eq.5 .or. ipass.eq.8) then
    do j=1,NHSYM
      ia=(j-1)*NSTEP + 1
      ib=ia+NSPS-1
      x(1:759)=0.
      if(j.ne.1) then; x(760:960)=dd(ia-201:ia-1)*windowx(200:0:-1); else; x(760:960)=0.; endif
      x(961:2880)=facx*dd(ia:ib); x(961)=x(961)*1.9; x(2880)=x(2880)*1.9
      if(j.ne.NHSYM) then; x(2881:3081)=dd(ib+1:ib+201)*windowx; else; x(2881:3081)=0.; endif
      x(3082:)=0.
      call four2a(cx,NFFT1,1,-1,0)              !r2c FFT
      do i=1,NH1
        s(i,j)=real(cx(i))**2 + aimag(cx(i))**2
      enddo
    enddo
  endif
  if(ipass.eq.3 .or. ipass.eq.6 .or. ipass.eq.9) then
    do j=1,NHSYM
      ia=(j-1)*NSTEP + 1
      ib=ia+NSPS-1
      x(1:759)=0.
      if(j.ne.1) then; x(760:960)=dd(ia-201:ia-1)*windowx(200:0:-1); else; x(760:960)=0.; endif
      x(961:2880)=facx*dd(ia:ib); x(961)=x(961)*1.9; x(2880)=x(2880)*1.9
      if(j.ne.NHSYM) then; x(2881:3081)=dd(ib+1:ib+201)*windowx; else; x(2881:3081)=0.; endif
      x(3082:)=0.
      call four2a(cx,NFFT1,1,-1,0)              !r2c FFT
      do i=1,NH1
        s(i,j)=abs(real(cx(i))) + abs(aimag(cx(i)))
      enddo
    enddo
  endif

  iaw=max(1,nint(nfawide/df)); ibw=max(1,nint(nfbwide/df))
  nssy=4 ! NSPS/NSTEP   ! # steps per symbol
  nssy36=144 ! nssy*36
  nssy72=288 ! nssy*72
  nfos=2 ! NFFT1/NSPS   ! # frequency bin oversampling factor
  jstrt=12.5 ! 0.5/tstep

  t8c=omp_get_wtime()
  if(lagcc .and. .not.lagccbail) then
    nfos6=12 ! nfos*6
! CE3TSK: sum(s(i:i+nfos6:nfos,k)) depends on the bin and the symbol only; it was recomputed
! for every DT lag. Tabulated once with the identical expression, so every value is bit for bit
! what the loop computed before. Same below for the plain metric.
    allocate(ssum(iaw:ibw,NHSYM))
    do k=1,NHSYM
      do i=iaw,ibw
        ssum(i,k)=sum(s(i:i+nfos6:nfos,k))
      enddo
    enddo
    if(lpar) then
!$omp parallel do default(shared) private(j)
      do j=jzb,jzt; call lag_agc(j); enddo
!$omp end parallel do
    else
      do j=jzb,jzt; call lag_agc(j); enddo
    endif
  else
!    nfos6=15 ! 16i spec bw -1
    nfos6=16
    allocate(ssum(iaw:ibw,NHSYM))
    do k=1,NHSYM
      do i=iaw,ibw
        ssum(i,k)=sum(s(i:i+nfos6,k))
      enddo
    enddo
    if(lpar) then
!$omp parallel do default(shared) private(j)
      do j=jzb,jzt; call lag_plain(j); enddo
!$omp end parallel do
    else
      do j=jzb,jzt; call lag_plain(j); enddo
    endif
  endif

  if(allocated(ssum)) deallocate(ssum)
  t8d=omp_get_wtime()
  red=0.; red2=0.; jpeak2=0; jpeak=0
  if(lpar) then
!$omp parallel do default(shared) private(i)
    do i=iaw,ibw; call peak_bin(i); enddo
!$omp end parallel do
  else
    do i=iaw,ibw; call peak_bin(i); enddo
  endif

  iz=ibw-iaw+1
  call indexx(red(iaw:ibw),iz,indx)
  ibase=indx(max(1,nint(0.40*iz))) - 1 + iaw ! max is workaround to prevent indx getting out of bounds
  base=red(ibase)
  if(base.lt.1e-8) base=1.0 ! safe division
  red=red/base; red2=red2/base

! CE3TSK diagnostic hook: JTDX_CAND_TRACE=<Hz> prints the normalised sync metric of every bin
! within +-25 Hz of that frequency, each pass, before the threshold is applied (the candidates
! that result are traced in ft8_decode.f90 with the decoder's verdict on them).
  call get_environment_variable('JTDX_CAND_TRACE',ctrace,ltrace,itrace)
  ftrace=0.; if(itrace.eq.0 .and. ltrace.gt.0) read(ctrace(1:ltrace),*,iostat=itrace) ftrace
  if(itrace.ne.0) ftrace=0.   ! an unreadable value switches the trace off
  if(ftrace.gt.0.) then
    do i=max(iaw,nint((ftrace-25.)/df)),min(ibw,nint((ftrace+25.)/df))
      write(*,3050) ipass,i*df,red(i),(jpeak(i)-1)*tstep,red2(i),(jpeak2(i)-1)*tstep,syncmin,base
3050  format('CANDTRACE sync8 pass',i2,' f',f8.1,' red',f7.2,' dt',f6.2,' red2',f7.2,' dt2',f6.2,' syncmin',f5.2,' base',es10.2)
    enddo
  endif

  return

contains
  subroutine lag_agc(j)
    integer, intent(in) :: j
    integer i,n,k,k36,k72
    real ta,tb,tc,tcq,t0a,t0b,t0c,t0cq,t1,t01,t2,t02,sync01,sync02,syncf,syncs,sya,sycq,sybc,sy1,sy2,sync_abc,sync_bc,tall(30)
    logical(1) lcq,lcq2
      do i=iaw,ibw
        ta=0.; tb=0.; tc=0.
        do n=0,6
          k=j+jstrt+nssy*n
          if(k.gt.0) then
            ta=s(i+nfos*icos7(n),k)
            if(ta.gt.1e-9) then; tall(n+1)=ta*6.0/(ssum(i,k)-ta); else; tall(n+1)=0.; endif
          endif
          k36=k+nssy36
          if(k36.gt.0 .and. k36.le.NHSYM) then
            tb=s(i+nfos*icos7(n),k36)
            if(tb.gt.1e-9) then; tall(n+17)=tb*6.0/(ssum(i,k36)-tb); else; tall(n+17)=0.; endif
          endif
          k72=k+nssy72
          if(k72.le.NHSYM) then
            tc=s(i+nfos*icos7(n),k72)
            if(tc.gt.1e-9) then; tall(n+24)=tc*6.0/(ssum(i,k72)-tc); else; tall(n+24)=0.; endif
          endif
        enddo
        lcq=.false.
        if(ipass.gt.1) then
          do n=7,15
            k=j+jstrt+nssy*n
            if(k.gt.0) then
              if(n.lt.15) then
                tall(n+1)=s(i,k)*6.0/(ssum(i,k)-s(i,k))
              else
                tall(n+1)=s(i+2,k)*6.0/(ssum(i,k)-s(i+2,k))
              endif
            endif
          enddo
          sya=sum(tall(1:7)); sycq=sum(tall(8:16)); sybc=sum(tall(17:30))
          sy1=(sya+sycq+sybc)/30.; sy2=(sya+sybc)/21.; sync_abc=max(sy1,sy2)
          sy1=(sycq+sybc)/23.; sy2=(sybc)/14.; sync_bc=max(sy1,sy2); if(sy1.gt.sy2) lcq=.true.
        else
          sybc=sum(tall(17:30)); sync_abc=sum(tall(1:7))+sybc; sync_bc=sybc/14.; sync_abc=sync_abc/21.
        endif
        sync2d(i,j)=max(sync_abc,sync_bc); if(lcq) syncq(i,j)=.true.
      enddo
  end subroutine lag_agc

  subroutine lag_plain(j)
    integer, intent(in) :: j
    integer i,n,k,k36,k72
    real ta,tb,tc,tcq,t0a,t0b,t0c,t0cq,t1,t01,t2,t02,sync01,sync02,syncf,syncs,sya,sycq,sybc,sy1,sy2,sync_abc,sync_bc,tall(30)
    logical(1) lcq,lcq2
      do i=iaw,ibw
        ta=0.; tb=0.; tc=0.; tcq=0.; t0a=0.; t0b=0.; t0c=0.; t0cq=0.
        do n=0,6
          k=j+jstrt+nssy*n
          if(k.gt.0) then; ta=ta + s(i+nfos*icos7(n),k); t0a=t0a + ssum(i,k) - s(i+nfos*icos7(n)+1,k); endif
          k36=k+nssy36
          if(k36.gt.0 .and. k36.le.NHSYM) then
            tb=tb + s(i+nfos*icos7(n),k36); t0b=t0b + ssum(i,k36) - s(i+nfos*icos7(n)+1,k36)
          endif
          k72=k+nssy72
          if(k72.le.NHSYM) then
            tc=tc + s(i+nfos*icos7(n),k72)
            t0c=t0c + ssum(i,k72) - s(i+nfos*icos7(n)+1,k72)
          endif
        enddo
        do n=7,15
          k=j+jstrt+nssy*n
          if(k.ge.1) then
            if(n.lt.15) then; tcq=tcq + s(i,k); t0cq=t0cq + ssum(i,k) - s(i,k+1)
            else; tcq=tcq + s(i+2,k); t0cq=t0cq + ssum(i,k) - s(i,k+3)
            endif
          endif
        enddo
        t1=ta+tb+tc; t01=t0a+t0b+t0c; t2=t1+tcq; t02=t01+t0cq
        t01=(t01-t1*2)/42.0; if(t01.lt.1e-8) t01=1.0; t02=(t02-t2*2)/60.0; if(t02.lt.1e-8) t02=1.0 ! safe division
        sync01=t1/(7.0*t01); sync02=(t1/7.0 + tcq/9.0)/t02; syncf=max(sync01,sync02)
        lcq=.false.; if(sync02.gt.sync01) lcq=.true.
        t1=tb+tc; t01=t0b+t0c; t2=t1+tcq; t02=t01+t0cq
        t01=(t01-t1*2)/28.0; if(t01.lt.1e-8) t01=1.0; t02=(t02-t2*2)/46.0; if(t02.lt.1e-8) t02=1.0 ! safe division
        sync01=t1/(7.0*t01); sync02=(t1/7.0 + tcq/9.0)/t02; syncs=max(sync01,sync02)
        lcq2=.false.; if(sync02.gt.sync01) lcq2=.true.
        sync2d(i,j)=max(syncf,syncs)
        if(syncf.gt.syncs) then; if(lcq) syncq(i,j)=.true.; else; if(lcq2) syncq(i,j)=.true.; endif
      enddo
  end subroutine lag_plain

  subroutine peak_bin(i)
    integer, intent(in) :: i
    integer ii(1),j0,jlo,jhi
    ii=maxloc(sync2d(i,jzb:jzt)) - 1 + jzb
    j0=ii(1)
    jpeak(i)=j0
    red(i)=sync2d(i,j0); if(syncq(i,j0)) redcq(i)=.true.
! CE3TSK: a second sync peak per bin, restricted to |DT| <= 13 steps (0.52 s), as WSJT-X's
! classic sync8 does. One peak per bin loses a signal whose DT peak is hidden by a
! stronger co-channel signal at another DT; the near-DT peak becomes its own candidate.
    jlo=max(jzb,-13); jhi=min(jzt,13)
    if(jlo.le.jhi) then
      ii=maxloc(sync2d(i,jlo:jhi)) - 1 + jlo
      jpeak2(i)=ii(1); red2(i)=sync2d(i,jpeak2(i))
    else   ! the DT window has drifted past +-0.52 s (avexdt beyond +-3 s): no second peak
      jpeak2(i)=jpeak(i); red2(i)=0.
    endif
  end subroutine peak_bin

end subroutine sync8_wide

! CE3TSK item 76: the driver's call - the pass-1 surface of the band every slice starts from,
! computed once with the loops in parallel, then read by every slice (lsync8share)
subroutine sync8_share(dd,jzb,jzt,swl,syncmin)
  use ft8_mod1, only : red_sh,red2_sh,jpeak_sh,jpeak2_sh,redcq_sh,base_sh,lsync8share
  use ft8_mod1, only : tsync8,tsync8s,tsync8l,tsync8p,nsync8p
  use omp_lib
  include 'ft8_params.f90'
  real, intent(in) :: dd(*),syncmin
  integer, intent(in) :: jzb,jzt
  logical(1), intent(in) :: swl
  real*8 :: t8a,t8b,t8c,t8d
  if(.not.allocated(red_sh)) allocate(red_sh(NH1),red2_sh(NH1),jpeak_sh(NH1),jpeak2_sh(NH1),redcq_sh(NH1))
  t8a=omp_get_wtime()
  call sync8_wide(dd,jzb,jzt,1,.true.,syncmin,red_sh,red2_sh,jpeak_sh,jpeak2_sh,redcq_sh,base_sh,t8c,t8d)
  lsync8share=.true.
  t8b=omp_get_wtime()
  tsync8=tsync8+(t8b-t8a); tsync8s=tsync8s+(t8c-t8a); tsync8l=tsync8l+(t8d-t8c)
  tsync8p(1)=tsync8p(1)+(t8b-t8a); nsync8p(1)=nsync8p(1)+1
  return
end subroutine sync8_share
