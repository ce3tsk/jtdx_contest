subroutine getcandidates4(fa,fb,syncmin,nfqso,maxcand,candidate,ncand)

  use ft4_mod1, only : dd4
  include 'ft4_params.f90'
  real s(NH1,NHSYM)
  real savg(NH1),savsm(NH1)
  real sbase(NH1)
  real x(NFFT1)
  real window(256)
  complex cx(0:NH1)
  real candidate(2,maxcand),candidatet(2,maxcand)
  equivalence (x,cx)
  logical first
  data first/.true./
  save first,window
!$omp threadprivate(first,window)   ! CE3TSK: built on the first call, one copy per thread

  if(first) then
    first=.false.
    pi=4.0*atan(1.)
    do i=1,256
      window(i)=SQRT((1.0+cos(i*pi/256))/2)
    enddo
  endif

! Compute symbol spectra, stepping by NSTEP steps.  
  savg=0.
  df=12000.0/NFFT1
  fac=1.0/300.0
  do j=1,NHSYM
     ia=(j-1)*NSTEP + 1
     ib=ia+2303 ! ib=ia+NFFT1-1
     if(ib.gt.NMAX) exit
     x(129:2432)=fac*dd4(ia:ib)!; x(129)=x(129)*1.9; x(2432)=x(2432)*1.9
     if(j.ne.1) then; x(1:128)=fac*dd4(ia-128:ia-1); else; x(1:128)=0.; endif
     if(j.ne.NHSYM) then; x(2433:NFFT1)=fac*dd4(ib+1:ib+128); else; x(2433:NFFT1)=0.; endif
     x(1:128)=x(1:128)*window(1:128); x(2433:NFFT1)=x(2433:NFFT1)*window(129:256)
     call four2a(cx,NFFT1,1,-1,0)              !r2c FFT
     s(1:NH1,j)=abs(cx(1:NH1))**2
     savg=savg + s(1:NH1,j)                   !Average spectrum
  enddo
  savg=savg/NHSYM

  savsm=0.
  do i=9,NH1-8; savsm(i)=sum(savg(i-8:i+8))/17.; enddo
  do i=1,8; k=i-1; savsm(i)=sum(savg(i-k:i+k))/(2*k+1); enddo
!  do i=NH1-7,NH1; k=NH1-i; savsm(i)=sum(savg(i-k:i+k))/(2*k+1); enddo

  nfa=max(1,nint(fa/df)); nfb=min(nint(5000.0/df),nint(fb/df))   ! CE3TSK item 65: was 4910
  if(nfa.lt.43) then; nfaa=43; else; nfaa=nfa; endif
! CE3TSK: the noise baseline is fitted over the same fixed span whatever range is decoded -
! ft4_baseline's own limits, 200 Hz to 4910 Hz - as sync8 does for FT8 (nfawide/nfbwide), so
! a narrower decode range finds exactly the candidates the full band finds inside it; the
! peak search below still runs over nfa..nfb only
! CE3TSK: normalised across the baseline's own span, not the slice's - the shape sync8 uses for
! FT8, which scores the wide band (nfawide-nfbwide) and lets each slice select from it. Everything
! above this point - the symbol spectra, the smoothing, the baseline fit - was already
! slice-independent; the normalisation and the peak search were the only places the slice entered.
! Restricting the normalisation to nfa..nfb is what forced the peak loop to run nfa+1..nfb-1 (it
! needs savsm(i-1) and savsm(i+1), and a normalised bin cannot be compared against an
! un-normalised neighbour), and with slices that left bins nfb and nfb+1 searched by NEITHER of
! two adjacent slices - a 9.4 Hz blind spot at every seam, 2.1 % of the band
! (FT4_SLICE_COST_PLAN.md, idea 3). Normalising the whole span removes the special case instead of
! patching it, and leaves everything before the peak loop shareable between slices - which is the
! saving that plan calls idea 1.
! CE3TSK item 65: JTDX's FT4 candidate search stopped at 4910 Hz on the sync bin - a base
! frequency of ~4875 Hz, where FT8 reaches ~4960 (sync8 scores to nfbwide = 5000). The
! baseline is still FITTED over 200-4910 Hz (nwb), so every candidate below 4910 Hz is exactly
! what it was; it is EVALUATED to 5000 Hz (nwe) and the normalisation and the peak search run
! to there, so a signal up to a ~4969 Hz base gets its candidate as an FT8 one does.
  nwb=nint(4910.0/df); nwe=min(NH1-1,nint(5000.0/df))
  call ft4_baseline(savg,43,nwb,sbase,nwe+1); if(any(sbase(nfaa:min(nfb,nwe)).le.0)) return

  savsm(43:nwe+1)=savsm(43:nwe+1)/sbase(43:nwe+1)
! CE3TSK: `le`, not `lt`. Bins 1-42 were normalised only when the range began BELOW bin 43,
! which killed the lowest bin of every range that begins exactly ON it: with NFFT1=2560 the bin
! width is 4.6875 Hz, so a 200 Hz start gives nint(200/4.6875) = 43 exactly, the peak loop below
! starts at 43, and its first test compared the normalised savsm(43) against the raw power in
! savsm(42) - orders of magnitude larger, so the condition could never hold. Six of the fourteen
! selectable decode ranges start at 200 Hz. This is the normalised-against-un-normalised
! comparison the note above says the rewrite exists to remove.
! The condition stays rather than going away: nfaa is max(nfa,43), so sbase(43) is covered by the
! `any(sbase(nfaa:...).le.0)` guard above only while nfa is at or below 43. Dividing by it
! unconditionally would put a 0-divide outside that guard on every slice whose range starts
! higher - harmless for decodes, since those bins are then never read, but a trap under
! -ffpe-trap=zero and an Inf written into a real array.
  if(nfa.le.43) savsm(1:42)=savsm(1:42)/sbase(43)
  f_offset = -1.5*12000.0/NSPS
  ncand=0; candidatet=0
  do i=max(2,nfa),min(nfb,nwe)   ! CE3TSK: the slice's own range, end bins included - see above
     if(savsm(i).ge.savsm(i-1) .and. savsm(i).ge.savsm(i+1) .and.      &
          savsm(i).ge.syncmin) then
        den=savsm(i-1)-2*savsm(i)+savsm(i+1)
        del=0.
        if(den.ne.0.0)  del=0.5*(savsm(i-1)-savsm(i+1))/den
        fpeak=(i+del)*df+f_offset
!        if(fpeak.lt.200.0 .or. fpeak.gt.4910.0) cycle
        if(fpeak.gt.5000.0) cycle   ! CE3TSK item 65: was 4910
        speak=savsm(i) - 0.25*(savsm(i-1)-savsm(i+1))*del
        ncand=ncand+1
        candidatet(1,ncand)=fpeak
        candidatet(2,ncand)=speak
        if(ncand.eq.maxcand) exit
     endif
  enddo
  candidate=0
  nq=count(abs(candidatet(1,1:ncand)-nfqso).le.20.0)
  n1=1; n2=nq+1 
  do i=1,ncand
     if(abs(candidatet(1,i)-nfqso).le.20.0) then
        candidate(1:2,n1)=candidatet(1:2,i)
        n1=n1+1
     else
        candidate(1:2,n2)=candidatet(1:2,i)
        n2=n2+1
     endif
  enddo 
return
end subroutine getcandidates4