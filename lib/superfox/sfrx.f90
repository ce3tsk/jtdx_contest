program sfrx

! Command-line SuperFox decoder

! CE3TSK: from WSJT-X 3.0.2 lib/superfox/sfrx.f90 (SUPERFOX_PLAN.md): the reference the GUI
! path is tested against, since it calls the same sfrx_sub. What differs from the donor:
! - no git tag in the usage text, no debug line on unit 72 (it left a fort.72 in the working
!   directory), no date: sfrx_sub does not take one;
! - a file name without a yymmdd_hhmmss stamp means nutc = 0 instead of ending the program with
!   "Bad integer", and a file shorter than 15 s is decoded zero-padded instead of ending it with
!   an end-of-file error. Both happened with the donor on this project's own recordings;
! - the file name may be 500 characters; with the donor's 120 a long path is cut and the file
!   "cannot be opened";
! - the wav header is taken as 44 bytes, as in the donor. A longer header (jtdx's own recordings
!   carry a 276-byte one) shifts the audio by some 10 ms, which only moves DT by that much.

  use sfox_mod

  integer*2 iwave(NMAX)
  logical ldecoded
  integer ihdr(11)
  character*500 fname                 !CE3TSK: the donor's 120 silently fails on a long path

  narg=iargc()

  if(narg.lt.3) then
     print*,'Usage:    sfrx fsync ftol infile [...]'
     print*,'          sfrx 750 50 240811_102400.wav'
     print*,'Reads one or more .wav files (12000 Hz, 16 bit, mono) and calls the SuperFox'
     print*,'decoder on each. fsync = expected frequency of the lowest tone, ftol = search'
     print*,'range either side of it, both in Hz.'
     go to 999
  endif

  call getarg(1,fname)
  read(fname,*,err=998) fsync
  call getarg(2,fname)
  read(fname,*,err=998) ftol

  nfqso=nint(fsync)
  ntol=nint(ftol)

  do ifile=3,narg
     call getarg(ifile,fname)
     open(10,file=trim(fname),status='old',access='stream',err=4)
     go to 5
4    print*,'Cannot open file ',trim(fname)
     go to 999
5    iwave=0
     read(10,iostat=ios) ihdr
     if(ios.eq.0) then
        do i=1,NMAX                        !a short file leaves the rest of iwave at zero
           read(10,iostat=ios) iwave(i)
           if(ios.ne.0) exit
        enddo
     endif
     close(10)

     nutc=0
     nz=len(trim(fname))
     if(nz.ge.10) then
        if(fname(nz-3:nz).eq.'.wav') then
           read(fname(nz-9:nz-4),*,iostat=ios) nutc
           if(ios.ne.0) nutc=0
        endif
     endif

     call sfrx_sub(nutc,nfqso,ntol,iwave,ldecoded)
  enddo
  go to 999

998 print*,'fsync and ftol must be numbers: sfrx fsync ftol infile [...]'

999 end program sfrx
