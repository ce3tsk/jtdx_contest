! CE3TSK 2026-09-05, review note - READ BEFORE ADDING A CALLER.
!
! The contract is not what the argument names suggest:
!   - the loop is "do i=0,npts": it always starts at 0 whatever nbot is, and npts is the LAST
!     index written, not a count. cb(0:npts) is filled; cb(nbot:-1) and cb(npts+1:ntop) are not.
!   - ca and cb are explicit-shape dummies (nbot:ntop) sized by the caller's own arguments, so
!     no bounds check can ever see a call whose actual array is smaller than nbot:ntop claims.
!     -fcheck=bounds is on in every build of this fork and never caught the overrun below.
!   - w starts at 1.0 and advances before each store, so cb(k) = ca(k)*wstep**(k+1) with k
!     counted from 0 - the phase of every output element is tied to index 0.
!
! Callers (both in bounds today):
!   lib/ft4b.f90   twkfreq1(ctwk,0,2*NSS-1,2*NSS-1,...,ctwk2(:,idf))   64 elements, indices 0..63
!   lib/ft8b.f90   twkfreq1(cd0,-800,3199,4000,...,cd0)               fills 0..3199 of -800:4000
!
! Incident: until 2026-09-05 ft4b passed npts=ntop=2*NSS, i.e. 65 stores into a 64-element
! column of its threadprivate table ctwk2 - one element past the end of the last column. Static
! in JTDX, that landed in neighbouring data and was never noticed; threadprivate under MinGW
! gfortran's emulated TLS the table is an exactly sized heap block, and the overrun corrupted
! the next heap block's header (STATUS_HEAP_CORRUPTION on the first FT4 candidate of the first
! period, in ft4_downsample's allocate). Fixed at the call site; the trap here is unchanged.
!
! The ft8b call also passes cd0 as both the intent(in) ca and the intent(out) cb, which the
! standard forbids (F2018 15.5.2.13); it works only because each element is read and written at
! the same position, in order.
!
! Deeper fix, not applied (bit-identical for both callers, two files, ~5 lines): drop npts, loop
! "do i=nbot,ntop" so the routine can only touch what the dummies declare, and change the ft8b
! call to pass the range it fills, twkfreq1(cd0(0:3199),0,3199,fs2,a,cd0(0:3199)). Both calls
! still start at index 0, so every stored value is unchanged. The one trap: changing the loop
! bound WITHOUT changing the ft8b call would start the recurrence at -800 and rotate every FT8
! sample by wstep**800 - the same magnitudes in exact arithmetic, but different rounding in
! every sample, which the repeatability runs would show as marginal-decode flips.
subroutine twkfreq1(ca,nbot,npts,ntop,fsample,a,cb)

  use ft8_mod1, only : twopi
  complex, intent(in) :: ca(nbot:ntop)
  complex, intent(out) :: cb(nbot:ntop)
  complex w,wstep
  real a(5)

! Mix the complex signal
  w=1.0; wstep=1.0
!!  x0=0.5*(npts+1)
!!  s=2.0/npts
  do i=0,npts
!!     x=s*(i-x0) ! valid for 'do i=1,npts'
!!     p2=1.5*x*x - 0.5
!!     p3=2.5*(x**3) - 1.5*x
!!     p4=4.375*(x**4) - 3.75*(x**2) + 0.375
!!     dphi=(a(1) + x*a(2) + p2*a(3) + p3*a(4) + p4*a(5)) * (twopi/fsample)
     dphi=a(1) * (twopi/fsample)
     wstep=cmplx(cos(dphi),sin(dphi))
     w=w*wstep
     cb(i)=w*ca(i)
  enddo

  return
end subroutine twkfreq1
