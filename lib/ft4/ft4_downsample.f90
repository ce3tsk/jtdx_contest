subroutine ft4_downsample(newdata,f0,c)

   use ft4_mod1, only : dd4 !,llagcc
   include 'ft4_params.f90'
   parameter (NFFT2=NMAX/NDOWN) ! 4096
   complex c(0:NMAX/NDOWN-1) ! 0:4095
   complex c1(0:NFFT2-1) ! 0:4095
   complex cx(0:NMAX/2)
   real x(NMAX), window(0:NFFT2-1)
   equivalence (x,cx)
   complex, allocatable :: cxx(:)   ! CE3TSK: allocatable so it lives on the heap, not in TLS
   logical first, newdata
   data first/.true./
   save first,window,cxx
! CE3TSK: the period's big FFT is state - filled when newdata is set, then read for every
! candidate after it, and refilled after each subtraction pass - so under the slice loop each
! thread needs its own. x and cx cannot be threadprivate: they are equivalenced, and OpenMP
! forbids the attribute on an equivalenced variable (naming a common block instead does not help,
! gfortran rejects that too). So the same shape as ft8_downsample.f90: x/cx stay ordinary
! per-call locals - private to a thread by virtue of being on its stack - and the result is kept
! in cxx, a plain complex buffer with no equivalence, which can be threadprivate.
!$omp threadprivate(first,window,cxx)

   ! CE3TSK: zeroed on allocation. cxx replaced a SAVEd array, which the loader zero-fills, and
   ! it is deliberately not refilled when newdata is false - so any path that reads it before the
   ! first big FFT used to see zeros, deterministically. From the heap it would see whatever was
   ! there, which differs on every run: exactly the rare marginal-decode flip that the
   ! repeatability runs chased (DECODER_IMPROVEMENTS item 52).
   if(.not.allocated(cxx)) then; allocate(cxx(0:NMAX/2)); cxx=(0.,0.); endif

   df=12000.0/NMAX
   baud=12000.0/NSPS
   if(first) then
      bw_transition = 0.5*baud
      bw_flat = 4*baud
      iwt = bw_transition / df
      iwf = bw_flat / df
      pi=4.0*atan(1.0)
      window(0:iwt-1) = 0.5*(1+cos(pi*(/(i,i=iwt-1,0,-1)/)/iwt))
      window(iwt:iwt+iwf-1)=1.0
      window(iwt+iwf:2*iwt+iwf-1) = 0.5*(1+cos(pi*(/(i,i=0,iwt-1)/)/iwt))
      window(2*iwt+iwf:)=0.0
      iws = baud / df
      window=cshift(window,iws)
      first=.false.
   endif

   if(newdata) then
      x=dd4
      call four2a(cx,NMAX,1,-1,0)             !r2c FFT to freq domain
      cxx=cx                                  ! CE3TSK: keep it per thread, see above
   endif
   i0=nint(f0/df)
   c1=0.
   if(i0.ge.0 .and. i0.le.NMAX/2) c1(0)=cxx(i0)
   do i=1,NFFT2/2
      if(i0+i.ge.0 .and. i0+i.le.NMAX/2) c1(i)=cxx(i0+i)
      if(i0-i.ge.0 .and. i0-i.le.NMAX/2) c1(NFFT2-i)=cxx(i0-i)
   enddo
   c1=c1*window/NFFT2
!   if(.not.llagcc) then; c1(0)=c1(0)*1.9; c1(4095)=c1(4095)*1.9; endif
   c1(0)=c1(0)*1.9; c1(4095)=c1(4095)*1.9
   call four2a(c1,NFFT2,1,1,1)            !c2c FFT back to time domain
   c=c1(0:NMAX/NDOWN-1)

   return
end subroutine ft4_downsample
