subroutine subtractft8(itone,f0,dt,swl)

! Subtract an ft8 signal
!
! Measured signal  : dd(t)    = a(t)cos(2*pi*f0*t+theta(t))
! Reference signal : cref(t)  = exp( j*(2*pi*f0*t+phi(t)) )
! Complex amp      : cfilt(t) = LPF[ dd(t)*CONJG(cref(t)) ]
! Subtract         : dd(t)    = dd(t) - 2*REAL{cref*cfilt}

  use ft8_mod1, only : dd8,cw,NFILT1,NFILT2,endcorr,endcorrswl
! NMAX=15*12000,NFFT=15*12000,NFRAME=1920*79
  parameter (NFFT=180000,NMAX=180000,NFRAME=151680)
  complex cref(nframe),cfilt(nmax)
  integer itone(79)
  logical(1), intent(in) :: swl
  save cfilt
  !$omp threadprivate(cfilt)

  block
    use ft8_mod1, only : ldectr2
    if(ldectr2) write(0,'(a,f14.7,f14.9,l2)') 'SUB ',f0,dt,swl
  end block
  nstart=dt*12000+1
  call gen_ft8wave(itone,79,1920,2.0,12000.0,f0,cref,xjunk,1,NFRAME)
  block
    use ft8_mod1, only : ldectr2
    real(8) ckr
    integer kcr
    if(ldectr2) then
      ckr=0.d0
      do kcr=1,nframe,97; ckr=ckr+dble(real(cref(kcr)))*dble(kcr); enddo
      write(0,'(a,es18.10)') 'SUBREF ',ckr
    endif
  end block
  do i=1,nframe
    id=nstart-1+i 
    if(id.ge.1.and.id.le.NMAX) then
      cfilt(i)=dd8(id)*conjg(cref(i))
    else
      cfilt(i)=0.0
    endif
  enddo
  cfilt(nframe+1:)=0.0
  call four2a(cfilt,nfft,1,-1,1)
  cfilt(1:nfft)=cfilt(1:nfft)*cw(1:nfft)
  call four2a(cfilt,nfft,1,1,1)
  if(.not.swl) then
    cfilt(1:NFILT1/2+1)=cfilt(1:NFILT1/2+1)*endcorr
    cfilt(nframe:nframe-NFILT1/2:-1)=cfilt(nframe:nframe-NFILT1/2:-1)*endcorr
  else
    cfilt(1:NFILT2/2+1)=cfilt(1:NFILT2/2+1)*endcorrswl
    cfilt(nframe:nframe-NFILT2/2:-1)=cfilt(nframe:nframe-NFILT2/2:-1)*endcorrswl
  endif
  do i=1,nframe
     j=nstart+i-1
     if(j.ge.1 .and. j.le.NMAX) dd8(j)=dd8(j)-2*REAL(cfilt(i)*cref(i))
  enddo
!!!$omp flush(dd8) ! makes no difference in number of decoded messages

  return
end subroutine subtractft8
