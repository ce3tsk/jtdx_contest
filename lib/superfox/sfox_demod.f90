subroutine sfox_demod(crcvd,f,t,isync,s2,s3)

! CE3TSK 2026-09-20: WSJT-X 3.0.2's, with sfox_pctile for pctile - the same two medians, selected
! instead of sorted for (sfox_pctile.f90 has the measurement) - and, the one step that is NOT
! WSJT-X's, the normalisation of tone bins that hold QRM (below). sfox_demod.f90.org is WSJT-X's.

  use sfox_mod
  complex crcvd(NMAX)                    !Signal as received
! CE3TSK 2026-09-20 (review): c was "complex c(0:NSPS-1)" - NSPS is a module VARIABLE, so that is an
! automatic array, and gfortran takes automatic arrays from the heap. four2a keys its FFTW plans on
! the array's ADDRESS and stops the program at 2100 of them: six Fox slots in one process planned
! the same 1024-point transform at five addresses. It settled in every pattern tried, but nothing
! made it settle. Allocated once and kept, as sfrx_sub does for its own.
  complex, allocatable, save :: c(:)     !Work array, one symbol long
  real s2(0:NQ-1,0:151)                  !Symbol spectra, including sync
  real s3(0:NQ-1,0:NN)                   !Synchronized symbol spectra
  integer isync(24)
  integer ipk(1)
  integer hist1(0:NQ-1),hist2(0:NQ-1)
  real rowmean
  real s2p(0:127,0:151)                  !CE3TSK: s2 without the normalisation, same blanking (for the SNR estimate)
  character envval*8
  integer, save :: nnorm=-1              !CE3TSK: 1 = bins that hold QRM are normalised (below); JTDX_SFOX_NORM=0: 0

  if(.not.allocated(c)) allocate(c(0:NSPS-1))
  j0=nint(12000.0*(t+0.5))
  df=12000.0/NSPS
  i0=nint(f/df)-NQ/2
  k2=0
! CE3TSK 2026-09-20: ALL of s2, not only the punctured symbol. The loop below skips a symbol that
! falls outside the 15 s buffer ("cycle") AFTER counting it, so its column of s2 was never set -
! and the median (pctile), the birdie histogram (maxloc over every column) and s3 then read it.
! qpc_decode2 keeps s2 on its stack, so what they read was whatever lay there. It takes a sync at
! an extreme time offset; valgrind found it in a band with its FT8 signals taken out. WSJT-X
! 3.0.2 has the same code. A symbol that was not received has no energy: zero.
  s2=0.
  s3(:,0)=0.                             !The punctured symbol

  do n=1,NDS                             !Loop over all symbols
     jb=n*NSPS + j0
     ja=jb-NSPS+1
     k2=k2+1
     if(ja.lt.1 .or. jb.gt.NMAX) cycle
     c=crcvd(ja:jb)
     call four2a(c,NSPS,1,-1,1)          !Compute symbol spectrum
     do i=0,NQ-1
        s2(i,k2)=real(c(i0+i))**2 + aimag(c(i0+i))**2
     enddo
  enddo


  call sfox_pctile(s2,NQ*151,50,base2)
  s2=s2/base2

! CE3TSK 2026-09-20: TONE BINS THAT HOLD QRM (SUPERFOX_DECODER_IDEAS.md section 4; the measurements
! are test/experiments/sfox_qrmbins/, where this step is "variant 5").
! The symbol likelihood (qpc_likelihoods2) is exp(power) with ONE noise level for all 128 bins,
! and the array has just been divided by its ONE median. An FT8 signal in the Fox's band breaks
! that twice. A caller at +10 dB is 2000 times the noise in its own bin against the Fox's 5 to
! 20, so that bin wins every symbol it is keyed in - and the rectangular symbol FFT leaks it over
! the whole band at 1/(pi k)^2, twice the noise ten bins away, so every OTHER bin's noise level
! is wrong as well, and differently wrong from bin to bin. WSJT-X's defence (below: the histogram
! of the loudest bin, ONE block of eight bins blanked) sees neither the leakage nor a second
! caller. The step: a bin whose MEAN power over the transmission is more than twice the noise's
! is divided by that mean. A caller's bin has a huge mean - its cells shrink to the order of 1 and
! its noise-only cells to nothing; a bin that only carries leakage is brought back to unit noise.
! The Fox's OWN bins: a strong Fox lifts the mean of every bin it uses, and those bins are then
! divided as well. That cannot hurt it. A bin the Fox uses k times has a mean of at least
! k x E / 151, so its Fox cells come out at no less than 151/k x 1.443 however strong the Fox is -
! and k is small, because the polar transform spreads even the barest message: a CQ-only
! transmission uses 82 different tones and its most frequent one 4 to 6 times. That leaves a Fox
! cell at 36 or more against about 8 for the loudest of the 127 noise cells beside it. Measured:
! CQ only, one RR73, one report and the full nine-Hound message, -16 to +20 dB, ten files each -
! the same files decode with the step as without.
! Measured, with the FT8 QRM remover: Hounds calling in the Fox's period 69 -> 79 of 80 (WSJT-X's
! receiver, which removes FT8 first: 67), the same with a fading Fox 71 -> 78, recorded FT8 bands
! 86 -> 89 of 114, the mean slot 0.87 -> 0.54 s (three callers: the Fox is printed after 0.06 s and
! the FT8 round never runs). Alone, with no FT8 decoding at all: 0 -> 31 and 67 -> 80.
! On a band WITHOUT QRM no bin qualifies and the receiver is WSJT-X's to the last decode: 360
! threshold files, the same ones decoded (-16.7 dB AWGN, -14.9 dB MM). Nothing printed on 78 bands
! without a Fox. The classic receiver never decodes a file this misses (0 of 274), so there is no
! second pass without it. 1.443 = 1/ln 2, the mean of noise power whose median is 1.
! JTDX_SFOX_NORM=0 switches the step off - for the comparison rows of the tests, and as a way out
! should it misbehave on the air.
! "Twice" was found on simulated callers and recorded FT8 bands; until there is more on-air
! material the factor is a setting, JTDX_SFOX_NORMX (sfox_mod.f90, read by sfox_config).
  if(nnorm.lt.0) then
     nnorm=1
     call get_environment_variable('JTDX_SFOX_NORM',envval,nenv,istat)
     if(istat.eq.0 .and. nenv.gt.0) then
        if(envval(1:1).eq.'0') nnorm=0
     endif
  endif
! THE SNR ESTIMATE must not see the Fox's own bins divided. qpc_snr adds up the power at the decoded
! symbols, and a strong Fox, whose every bin qualifies, read +6 dB where WSJT-X's receiver reads
! +14 (a 0 dB Fox -3 for -1; at -10 dB and below nothing qualifies and nothing changed, which is
! why no identity test saw it - the bare-and-strong-Fox row of superfox.sh did). So the spectra
! are kept WITHOUT the step as well (s2p, s3plain in sfox_mod - the same blanking decisions
! below), and qpc_decode2, which knows the Fox's symbols once it has decoded, tells the bins that
! hold QRM - those that stand out from the other bins even without the Fox's own cells - from the
! ones the Fox lifted itself, and takes its estimate from the plain spectra of the latter
! (qpc_decode2.f90 has the rule). The decoding itself is untouched by this.
  lnormed=.false.
  if(nnorm.eq.1) then
     s2p=s2
     do i=0,NQ-1
        rowmean=sum(s2(i,1:NDS))/NDS
        if(rowmean.gt.1.443*sfnormx) then          !twice the noise's, unless set otherwise: JTDX_SFOX_NORMX (sfox_mod)
           s2(i,:)=s2(i,:)*(1.443/rowmean); lnormed=.true.
        endif
     enddo
  endif

  hist1=0
  hist2=0
  do j=0,151
     ipk=maxloc(s2(1:NQ-1,j))       !Find the spectral peak
     i=ipk(1)-1
     hist1(i)=hist1(i)+1
  enddo

  hist1(0)=0                        !Don't treat sync tone as a birdie
  do i=0,123
     hist2(i)=sum(hist1(i:i+3))
  enddo

  ipk=maxloc(hist1)
  i1=ipk(1)-1
  m1=maxval(hist1)
  ipk=maxloc(hist2)
  i2=ipk(1)-1
  m2=maxval(hist2)
  if(m1.gt.12) then
     do i=0,127
        if(hist1(i).gt.12) then
             s2(i,:)=1.0
             if(lnormed) s2p(i,:)=1.0
        endif
     enddo
  endif

  if(m2.gt.20) then
     if(i2.ge.1) i2=i2-1
     if(i2.gt.120) i2=120
     s2(i2:i2+7,:)=1.0
     if(lnormed) s2p(i2:i2+7,:)=1.0
  endif

  k3=0
  do n=1,NDS                             !Copy symbol spectra from s2 into s3
     if(any(isync(1:NS).eq.n)) cycle     !Skip the sync symbols
     k3=k3+1
     s3(:,k3)=s2(:,n)
  enddo

  call sfox_pctile(s3,NQ*NN,50,base3)
  s3=s3/base3

  if(lnormed) then                       !CE3TSK: the same, from the spectra without the step
     k3=0
     s3plain(:,0)=0.
     do n=1,NDS
        if(any(isync(1:NS).eq.n)) cycle
        k3=k3+1
        s3plain(:,k3)=s2p(:,n)
     enddo
     call sfox_pctile(s3plain,NQ*NN,50,base3)
     s3plain=s3plain/base3
  endif

  return
end subroutine sfox_demod
