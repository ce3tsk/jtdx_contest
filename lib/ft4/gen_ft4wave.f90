subroutine gen_ft4wave(itone,nsym,nsps,fsample,f0,cwave,wave,icmplx,nwave)

  real wave(nwave)
  complex cwave(nwave)
  real pulse(6912)              !576*4*3
  real dphi(0:250000-1)
  integer itone(nsym)
  logical first
  integer nspsz
  real fsamplez
  data first/.true./,nspsz/-1/,fsamplez/-1.0/
  save pulse,first,twopi,dt,hmod,nspsz,fsamplez

! CE3TSK: the pulse table is shaped by nsps and dt is 1/fsample, so both have to be part of
! the "already computed" test. Caching on `first` alone was safe only while each process used
! a single pair - the decoder subtracts with (576, 12000) and the GUI transmits with
! (2304, 48000) - and would hand the second shape of a process the first one's table.
  if(first .or. nsps.ne.nspsz .or. fsample.ne.fsamplez) then
! CE3TSK: the table is shared and subtractft4 calls this from inside the slice parallel region, so
! the rebuild is serialised the way subtractft4 serialises its own filter init, and the test is made
! again inside. That protects rebuilders from each other, NOT a thread already reading pulse: it is
! safe today only because each process uses one shape (the decoder subtracts with 576/12000, the GUI
! transmits with 2304/48000). Mixing shapes inside one parallel region would need more than this.
!$omp critical(ft4_gen_pulse)
     if(first .or. nsps.ne.nspsz .or. fsample.ne.fsamplez) then
        twopi=8.0*atan(1.0)
        dt=1.0/fsample
        hmod=1.0
! Compute the smoothed frequency-deviation pulse
        do i=1,3*nsps
           tt=(i-1.5*nsps)/real(nsps)
           pulse(i)=gfsk_pulse(1.0,tt)
        enddo
        nspsz=nsps
        fsamplez=fsample
        first=.false.
     endif
!$omp end critical(ft4_gen_pulse)
  endif

! Compute the smoothed frequency waveform.
! Length = (nsym+2)*nsps samples, zero-padded
  dphi_peak=twopi*hmod/real(nsps)
  dphi=0.0 
  do j=1,nsym         
     ib=(j-1)*nsps
     ie=ib+3*nsps-1
     dphi(ib:ie) = dphi(ib:ie) + dphi_peak*pulse(1:3*nsps)*itone(j)
  enddo

! Calculate and insert the audio waveform
  phi=0.0
  dphi = dphi + twopi*f0*dt                          !Shift frequency up by f0
  wave=0.
  if(icmplx.eq.1) cwave=0.
  k=0
  do j=0,nwave-1
     k=k+1
     if(icmplx.eq.0) then
        wave(k)=sin(phi)
     else
        cwave(k)=cmplx(cos(phi),sin(phi))
     endif
     phi=mod(phi+dphi(j),twopi)
  enddo

! Compute the ramp-up and ramp-down symbols
  if(icmplx.eq.0) then
     wave(1:nsps)=wave(1:nsps) *                                          &
          (1.0-cos(twopi*(/(i,i=0,nsps-1)/)/(2.0*nsps)))/2.0
     k1=(nsym+1)*nsps+1
     wave(k1:k1+nsps-1)=wave(k1:k1+nsps-1) *                              &
          (1.0+cos(twopi*(/(i,i=0,nsps-1)/)/(2.0*nsps)))/2.0
  else
     cwave(1:nsps)=cwave(1:nsps) *                                          &
          (1.0-cos(twopi*(/(i,i=0,nsps-1)/)/(2.0*nsps)))/2.0
     k1=(nsym+1)*nsps+1
     cwave(k1:k1+nsps-1)=cwave(k1:k1+nsps-1) *                              &
          (1.0+cos(twopi*(/(i,i=0,nsps-1)/)/(2.0*nsps)))/2.0
  endif

  return
end subroutine gen_ft4wave
