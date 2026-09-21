program ft8sim

! Generate simulated "type 2" ft8 files
! Output is saved to a *.wav file.

  use wavhdr
  use packjt77
  include 'ft8_params.f90'               !Set various constants
  parameter (NWAVE=NN*NSPS)
  type(hdr) h                            !Header for .wav file
  character arg*12,fname*17
  character msg37*37,msgsent37*37
  character c77*77
  complex c0(0:NMAX-1)
  complex c(0:NMAX-1)
  complex cwave(NWAVE)                   !CE3TSK: the GFSK waveform (JTDX_SIM_GFSK=1)
  real xjunk(NWAVE)                      !gen_ft8wave's real output, not written when icmplx=1
  real wave(NMAX)
  integer itone(NN)
  integer*1 msgbits(77)
  integer*4 iwave(NMAX)                  !Generated full-length waveform, CE3TSK: 32 bit for JTDX
  integer*2 i2wave(NMAX)                 !CE3TSK: the 16 bit default
  logical l32,lgfsk
  character envval*8
  integer envlen

! Get command-line argument(s)
  nargs=iargc()
  if(nargs.ne.8) then
     print*,'Usage:    ft8sim "message"                 f0     DT fdop del width nfiles snr'
     print*,'Examples: ft8sim "K1ABC W9XYZ EN37"       1500.0 0.0  0.1 1.0   0     10   -18'
     print*,'          ft8sim "WA9XYZ/R KA1ABC/R FN42" 1500.0 0.0  0.1 1.0   0     10   -18'
     print*,'          ft8sim "K1ABC RR73; W9XYZ <KH1/KH7Z> -11" 300 0 0 0 25 1 -10'
     print*,'Environment: JTDX_SIM_SEED=n   fixed noise seed'
     print*,'             JTDX_SIM_32BIT=1  32 bit files (default 16 bit)'
     print*,'             JTDX_SIM_GFSK=1   Gaussian-shaped FSK (BT 2.0), what WSJT-X, JTDX and MSHV'
     print*,'                               transmit; the default (0) is plain FSK'
     go to 999
  endif
  call getarg(1,msg37)                   !Message to be transmitted
  call getarg(2,arg)
  read(arg,*) f0                         !Frequency (only used for single-signal)
  call getarg(3,arg)
  read(arg,*) xdt                        !Time offset from nominal (s)
  call getarg(4,arg)
  read(arg,*) fspread                    !Watterson frequency spread (Hz)
  call getarg(5,arg)
  read(arg,*) delay                      !Watterson delay (ms)
  call getarg(6,arg)
  read(arg,*) width                      !Filter transition width (Hz)
  call getarg(7,arg)
  read(arg,*) nfiles                     !Number of files
  call getarg(8,arg)
  read(arg,*) snrdb                      !SNR_2500

  nsig=1
  if(f0.lt.100.0) then
     nsig=f0
     f0=1500
  endif

  nfiles=abs(nfiles)
! CE3TSK: the simulators write 16 bit wav files, the width JTDX records and the decoder's
! native format. JTDX_SIM_32BIT=1 restores the 32 bit files older expectation sets used.
  l32=.false.
  call get_environment_variable('JTDX_SIM_32BIT',envval,envlen)
  if(envlen.gt.0) l32=(envval(1:1).eq.'1')
  if(l32) write(*,*) 'JTDX_SIM_32BIT=1: writing 32 bit files'
! CE3TSK 2026-09-20: what this simulator has always written is PLAIN FSK - the phase advances at
! a constant rate through each symbol. No current program sends that: WSJT-X since 2.1, MSHV and
! this program (mainwindow.cpp, gen_ft8wave with bt=2.0) send GFSK, and GFSK is also the reference
! every decoder SUBTRACTS (subtractft8). For a single-signal decode test the difference does not
! matter - the symbol energies are detected non-coherently, and the measured threshold is the same
! (-21.7 against -21.8 dB). For anything that depends on how well a signal subtracts it matters a
! great deal: against a plain-FSK signal the reference is wrong at each of the 79 symbol edges.
! ONE signal at +20 dB is taken 19 dB down as plain FSK and 41 dB, to the noise, as GFSK; a -10 dB
! signal 12 Hz from it is never found in the first case and always in the second
! (FT8_IDEAS_FROM_SUPERFOX.md section 2). JTDX_SIM_GFSK=1 generates the signal with gen_ft8wave,
! the transmitter's own routine, ramps included. It is NOT the default, and must not become it
! unannounced: the expectations of memcheck.sh and ft8merge.sh are pinned on seeded files made
! with the plain waveform, every threshold on record was measured with it, and the default output
! is byte-identical to what it was (test/decode/ft8simgfsk.sh pins it).
! Any value but 0 and 1 stops the program: a harness that throws the output away would otherwise
! go on with the plain waveform while believing it had asked for the other one.
! lib/ft8sim_gfsk.f90 is the older route to the same waveform - WSJT-X 2.1's simulator as JTDX
! inherited it: 7 arguments (no filter width), 32 bit files only, and a signal that starts before
! the file wraps round to the file's end. Nothing uses it; the same test pins that the two agree.
  lgfsk=.false.
  call get_environment_variable('JTDX_SIM_GFSK',envval,envlen)
  if(envlen.gt.0) then
     if(envlen.ne.1 .or. (envval(1:1).ne.'0' .and. envval(1:1).ne.'1')) then
        write(*,*) 'JTDX_SIM_GFSK must be 1 (Gaussian-shaped FSK) or 0 (plain FSK, the default)'
        call exit(1)
     endif
     lgfsk=(envval(1:1).eq.'1')
  endif
  if(lgfsk) write(*,*) 'JTDX_SIM_GFSK=1: Gaussian-shaped FSK, BT 2.0'
  twopi=8.0*atan(1.0)
  fs=12000.0                             !Sample rate (Hz)
  dt=1.0/fs                              !Sample interval (s)
  tt=NSPS*dt                             !Duration of symbols (s)
  baud=1.0/tt                            !Keying rate (baud)
  bw=8*baud                              !Occupied bandwidth (Hz)
  txt=NZ*dt                              !Transmission length (s)
  bandwidth_ratio=2500.0/(fs/2.0)
  sig=sqrt(2*bandwidth_ratio) * 10.0**(0.05*snrdb)
  if(snrdb.gt.90.0) sig=1.0
  txt=NN*NSPS/12000.0

  ! Source-encode, then get itone()
  i3=-1
  n3=-1
  call pack77(msg37,i3,n3,c77,1)
  call genft8(msg37,i3,n3,1,msgsent37,msgbits,itone)

  write(*,*)  
  write(*,'(a23,a37,3x,a7,i1,a1,i1)') 'New Style FT8 Message: ',msgsent37,'i3.n3: ',i3,'.',n3
  write(*,1000) f0,xdt,txt,snrdb,bw
1000 format('f0:',f9.3,'   DT:',f6.2,'   TxT:',f6.1,'   SNR:',f6.1,    &
       '  BW:',f4.1)
  write(*,*)  
  if(i3.eq.1) then
    write(*,*) '         mycall                         hiscall                    hisgrid'
    write(*,'(28i1,1x,i1,1x,28i1,1x,i1,1x,i1,1x,15i1,1x,3i1)') msgbits(1:77) 
  else
    write(*,'(a14)') 'Message bits: '
    write(*,'(77i1)') msgbits
  endif
  write(*,*) 
  write(*,'(a17)') 'Channel symbols: '
  write(*,'(79i1)') itone
  write(*,*)  

  call sgran()

! CE3TSK 2026-09-20: a signal that lies WHOLLY outside the 15 s is not a test file. With noise it
! would be a file of noise that claims to hold a signal, and without noise the peak normalisation
! below divides by zero and writes NaN through nint() - both silently, exit code 0.
  k=nint((xdt+0.5)/dt)
  if(k+NWAVE.le.0 .or. k.ge.NMAX) then
     write(*,'(a,f8.2,a)') ' DT',xdt,' s puts the whole signal outside the 15 s file - nothing written'
     call exit(1)
  endif
  if(lgfsk) call gen_ft8wave(itone,NN,NSPS,2.0,fs,f0,cwave,xjunk,1,NWAVE)   !the transmitter's waveform

  msg0=msg
  do ifile=1,nfiles
     k=nint((xdt+0.5)/dt)
     ia=k
     phi=0.0 
     c0=0.0
     if(lgfsk) then                        !CE3TSK: cwave placed exactly as the plain signal below
        do i=1,NWAVE
           if(k.ge.0 .and. k.lt.NMAX) c0(k)=cwave(i)
           k=k+1
        enddo
     else
     do j=1,NN                             !Generate complex waveform
        dphi=twopi*(f0*dt+itone(j)/real(NSPS))
        do i=1,NSPS
           if(k.ge.0 .and. k.lt.NMAX) c0(k)=cmplx(cos(phi),sin(phi))
           k=k+1
           phi=mod(phi+dphi,twopi)
        enddo
     enddo
     endif
     if(fspread.ne.0.0 .or. delay.ne.0.0) call watterson(c0,NMAX,NWAVE,fs,delay,fspread)
     c=sig*c0
  
     ib=k
     wave=real(c)
! CE3TSK 2026-09-20: ia..ib is where the signal was placed and runs off the array when DT is
! -0.5 s or less (wave is 1-based, ia is then 0 or below) or above 1.86 s - the loop above guards
! exactly that case, this line did not, and the bounds-checked build stopped with "Index '-1200'
! of dimension 1". (peak itself is not used.)
     peak=maxval(abs(wave(max(1,ia):min(NMAX,ib))))
     nslots=1
     if(width.gt.0.0) call filt8(f0,nslots,width,wave)
   
     if(snrdb.lt.90) then
        do i=1,NMAX                   !Add gaussian noise at specified SNR
           xnoise=gran()
           wave(i)=wave(i) + xnoise
        enddo
     endif

! CE3TSK: gain puts the noise floor where JTDX records it, roughly 2.8e7 rms in 32 bit terms,
! so replayed files look like real received audio. A 16 bit file carries the same level: file
! mode multiplies 16 bit samples by 65536 (jt9.f90), so the gain is divided by it here.
     gain=2.7e7
     if(.not.l32) gain=gain/65536.0
     if(snrdb.lt.90.0) then
       wave=gain*wave
     else
       datpk=maxval(abs(wave))
       fac=1.0e8/datpk
       if(.not.l32) fac=fac/65536.0
       wave=fac*wave
     endif
! CE3TSK: clamp before nint(). Converting a value beyond the target range is undefined
! behaviour and wraps to nonsense instead of clipping.
     clip=2.1e9
     if(.not.l32) clip=32767.0
     if(any(abs(wave).gt.clip)) then
       print*,"Warning - data will be clipped."
       where(wave.gt.clip) wave=clip
       where(wave.lt.-clip) wave=-clip
     endif

     write(fname,1102) ifile
1102 format('000000_',i6.6,'.wav')
     open(10,file=fname,status='unknown',access='stream')
     if(l32) then
       iwave=nint(wave)
       h=default_header32(12000,NMAX)
       write(10) h,iwave              !Save to *.wav file
     else
       i2wave=nint(wave)
       h=default_header(12000,NMAX)
       write(10) h,i2wave             !Save to *.wav file
     endif
     close(10)
     write(*,1110) ifile,xdt,f0,snrdb,fname
1110 format(i4,f7.2,f8.2,f7.1,2x,a17)
  enddo    
999 end program ft8sim
