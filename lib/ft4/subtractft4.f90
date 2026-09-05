! This source code file was last time modified by Igor UA3DJY on 20190929
! All changes are shown in the patch file coming together with the full JTDX source code.

subroutine subtractft4(itone,f0,dt)

! Subtract an ft4 signal
!
! Measured signal  : dd(t)    = a(t)cos(2*pi*f0*t+theta(t))
! Reference signal : cref(t)  = exp( j*(2*pi*f0*t+phi(t)) )
! Complex amp      : cfilt(t) = LPF[ dd(t)*CONJG(cref(t)) ]
! Subtract         : dd(t)    = dd(t) - 2*REAL{cref*cfilt}

!  use timer_module, only: timer
  use ft4_mod1, only : dd4,cref4,camp4,cfilt4,xjunk4,cwsub4,cwsub4_ready

!  parameter (NMAX=21*3456,NSPS=576,NFFT=NMAX,NFILT=1400)
  parameter (NMAX=73728,NSPS=576,NFFT=NMAX,NFILT=1400)
  parameter (NFRAME=(103+2)*NSPS)
  real*4  window(-NFILT/2:NFILT/2)
  integer itone(103)
! CE3TSK: this routine's work space used to be common/heap8/, one copy for the whole program -
! two slices subtracting at once overwrote each other's waveform. It is now per-thread, but as
! module allocatables rather than a threadprivate common block: 2.5 MB per thread of static TLS
! comes out of the same mapping as the thread's stack and killed every worker at the default
! OMP_STACKSIZE. See ft4_mod1.f90. The filter cwsub4 is identical for all threads, so it stays
! shared and is built once.
  ! CE3TSK: zeroed like the common block they replaced - see ft4_downsample.f90
  if(.not.allocated(cref4)) then
     allocate(cref4(NFRAME),camp4(NMAX),cfilt4(NMAX),xjunk4(NFRAME))
     cref4=(0.,0.); camp4=(0.,0.); cfilt4=(0.,0.); xjunk4=0.
  endif

  nstart=dt*12000+1-NSPS
  nsym=103
  fs=12000.0
  icmplx=1
  bt=1.0 
  nss=NSPS
  call gen_ft4wave(itone,nsym,nss,fs,f0,cref4,xjunk4,icmplx,NFRAME)
  camp4=0.
  do i=1,nframe
    id=nstart-1+i 
    if(id.ge.1.and.id.le.NMAX) camp4(i)=dd4(id)*conjg(cref4(i))
  enddo

! Create and normalize the filter. The critical section is entered once per call, which is
! nothing against the two 73728-point transforms below, and it gives the flush that makes the
! finished filter visible to every thread.
!$omp critical(ft4_subtract_cw)
  if(.not.cwsub4_ready) then
     if(.not.allocated(cwsub4)) then; allocate(cwsub4(NMAX)); cwsub4=(0.,0.); endif
     pi=4.0*atan(1.0)
     fac=1.0/float(nfft)
     sum=0.0
     do j=-NFILT/2,NFILT/2
        window(j)=cos(pi*j/NFILT)**2
        sum=sum+window(j)
     enddo
     cwsub4=0.
     cwsub4(1:NFILT+1)=window/sum
     cwsub4=cshift(cwsub4,NFILT/2+1)
     call four2a(cwsub4,nfft,1,-1,1)
     cwsub4=cwsub4*fac
     cwsub4_ready=.true.
  endif
!$omp end critical(ft4_subtract_cw)

  cfilt4=0.0
  cfilt4(1:nframe)=camp4(1:nframe)
  call four2a(cfilt4,nfft,1,-1,1)
  cfilt4(1:nfft)=cfilt4(1:nfft)*cwsub4(1:nfft)
  call four2a(cfilt4,nfft,1,1,1)

! Subtract the reconstructed signal
  do i=1,nframe
     j=nstart+i-1
     if(j.ge.1 .and. j.le.NMAX) dd4(j)=dd4(j)-2*REAL(cfilt4(i)*cref4(i))
  enddo

  return
end subroutine subtractft4

