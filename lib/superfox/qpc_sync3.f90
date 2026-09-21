subroutine qpc_sync3(crcvd0,fsample,isync,fsync,f2,t2,snrsync)

! CE3TSK 2026-09-21, for JTDX_CONTEST (Contact: jtdx_contest@ce3tsk.com): MSHV 2.76.6's THREE-WINDOW sync
! (DecoderSFox::qpc_sync, decodersfox.cpp) written on the program's qpc_sync.f90 (WSJT-X 3.0.2): the
! spectrum of the first 9.2 s (WSJT-X: 9.0), and three candidates - (1) the RX frequency +/- 60 Hz, the
! time lag measured over +/- 1 baud, snrsync + 0.55; (2) 700-800 Hz, +/- 2 baud; (3) 200-3200 Hz (MSHV's
! default decode range), +/- 1.5 baud, snrsync + 0.25. f2, t2, snrsync for each; the caller ranks them.
! Used ONLY by the last step of a Fox slot (decoder.f90 superfox_extra, qpc_decode2's nsfextra), switched
! on by JTDX_SFOX_SYNC3=1 - off by default: SUPERFOX_DECODER_IDEAS.md 4.14 has what it was measured on.

  parameter(N9SEC=110400,NMAX=15*12000,NDOWN=16,NZ=N9SEC/NDOWN)
  complex crcvd0(NMAX)
  complex c0(0:N9SEC-1)
  complex c1(0:NZ-1)
  complex c1sum(0:NZ-1)
  complex z
  real s(N9SEC/4)
  real p(-1125:1125)
  real f2(3),t2(3),snrsync(3)
  integer ipk(1)
  integer isync(24)

  baud=12000.0/1024.0
  df2=fsample/N9SEC
  fac=1.0/N9SEC
  c0=fac*crcvd0(1:N9SEC)
  call four2a(c0,N9SEC,1,-1,1)
  iz=N9SEC/4
  do i=1,iz
     s(i)=real(c0(i))**2 + aimag(c0(i))**2
  enddo
  do i=1,4
     call smo121(s,iz)
  enddo

  do icand=1,3
     if(icand.eq.1) then
        fa=fsync-60.0; fb=fsync+60.0; xknb=1.0
     else if(icand.eq.2) then
        fa=700.0; fb=800.0; xknb=2.0
     else
        fa=200.0; fb=3200.0; xknb=1.5
     endif
     ia=nint(fa/df2); ib=nint(fb/df2)
     if(ia.lt.1) ia=1
     if(ib.ge.27000) ib=27000
     ipk=maxloc(s(ia:ib))
     i0=ipk(1) + ia - 1
     ia=nint(i0-baud/df2); ib=nint(i0+baud/df2)
     if(ia.lt.1) ia=1
     if(ib.ge.27000) ib=27000
     s1=0.0; s0=0.0
     do i=ia,ib
        s0=s0+s(i); s1=s1+(i-i0)*s(i)
     enddo
     delta=0.
     if(s0.gt.0.0) delta=s1/s0
     i0=nint(i0+delta)
     f2(icand)=i0*df2-750.0

     c1=0.
     ia=nint(i0-xknb*baud/df2); ib=nint(i0+xknb*baud/df2)
     if(ia.lt.0) ia=0
     if(ib.ge.N9SEC-1) ib=N9SEC-1
     do i=ia,ib
        j=i-i0
        if(j.ge.0) c1(j)=c0(i)
        if(j.lt.0) c1(j+NZ)=c0(i)
     enddo
     call four2a(c1,NZ,1,1,1)
     c1sum(0)=c1(0)
     do i=1,NZ-1
        c1sum(i)=c1sum(i-1) + c1(i)
     enddo
     nspsd=1024/NDOWN
     dt=NDOWN/12000.0
     lagmax=1.5/dt
     i0=nint(0.5*fsample/NDOWN)
     pmax=0.; lagpk=0
     do lag=-lagmax,lagmax
        sp=0.
        do j=1,24
           i1=i0 + (isync(j)-1)*nspsd + lag
           i2=i1 + nspsd
           if(i1.lt.0 .or. i1.gt.NZ-1) cycle
           if(i2.lt.0 .or. i2.gt.NZ-1) cycle
           z=c1sum(i2)-c1sum(i1)
           sp=sp + real(z)**2 + aimag(z)**2
        enddo
        if(sp.gt.pmax) then
           pmax=sp; lagpk=lag
        endif
        p(lag)=sp
     enddo
     t2(icand)=lagpk*dt
     sp=0.; sq=0.; nsum=0
     tsym=1024/12000.0
     do lag=-lagmax,lagmax
        t=(lag-lagpk)*dt
        if(abs(t).lt.tsym) cycle
        nsum=nsum+1; sp=sp + p(lag); sq=sq + p(lag)*p(lag)
     enddo
     ave=sp/nsum
     rms=sqrt(max(1.e-30,sq/nsum-ave*ave))
     snrsync(icand)=(pmax-ave)/rms
     if(icand.eq.1) snrsync(icand)=snrsync(icand)+0.55
     if(icand.eq.3) snrsync(icand)=snrsync(icand)+0.25
  enddo

  return
end subroutine qpc_sync3
