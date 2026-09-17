! last time modified by Igor UA3DJY on 20200105

subroutine partintft4(ndelay,nutc)

  use ft4_mod1, only : dd4,lft2   ! CE3TSK: lft2 - FT2's buffer holds two samples per real one
  real rnd

  if(ndelay.gt.50) ndelay=50
  numsamp=nint((float(ndelay))*1200) ! 12000 sample rate
! CE3TSK: FT2 holds two samples per real one, so a delay in real tenths costs twice as many
! slots here, and 50 tenths doubled is 120000 against dd4's 73728 - the whole buffer is gone
! long before that, so cap at its length instead of writing past the end of it
  if(lft2) numsamp=min(2*numsamp,73728)
  dd4(numsamp+1:73728)=dd4(1:(73728-numsamp))
  do i=1,numsamp
     call random_number(rnd)
     dd4(i)=10.0*rnd-5.
  enddo
  write(*,2) nutc,'partial loss of data','d'
2 format(i6.6,2x,a20,21x,a1)
  call flush(6)

  return
end subroutine partintft4