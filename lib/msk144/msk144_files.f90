! CE3TSK 2026-11-17 (MSK144_PLAN.md step 0): file mode for MSK144 - jtdxjt9 --msk144 file1.wav [file2.wav ...].
!
! The measuring instrument of the MSK144 work, and the twin of WSJT-X's jt9 -k: every file is one recorded T/R
! period, decoded window by window exactly as the running program will decode it, and the lines are WSJT-X's
! ("hhmmss dB T Freq &  message", 80 columns). After each file a line
!     <DecodeFinished>   0 <messages>        0
! as WSJT-X 3.3.0-beta6 prints it (3.0.2 printed 0 for the count, whatever was decoded).
!
!   --msk144              this mode
!   -p SECONDS            the T/R period, 5 10 15 30 (default 15) - seconds in this mode, as in WSJT-X's jt9
!   -f HERTZ              the receive frequency (default 1500; the decoder's band is fixed around 1500)
!   -i HERTZ              F Tol (default 20, WSJT-X's own default). Its long name is --tolerance, but give it as -i:
!                         the option reader (lib/options.f90, WSJT-X's) fetches a long option's argument twice, so
!                         no long option that takes a value works - in this program or in WSJT-X
!   -d DEPTH              1 Fast, 2 Normal, 3 Deep (default 1)
!   -c CALL  -x CALL      my call and the DX call (blank when not given)
!   -q                    short messages (the main window's Sh) are decoded too: "<mycall dxcall> R+03", and they are
!                         those of the two calls given with -c and -x alone (2026-11-18)
!
! The file: 12000 Hz, 16 bit, mono. Samples beyond the period are not read; a file shorter than the period is
! filled with zeros, which the decoder passes over. (WSJT-X's own reader takes the file in blocks of 3456 samples
! and so leaves the last few hundred samples of a 10 s period at zero; this one reads them all.)
! The UTC is taken from the name as jt9files takes it: ..._hhmmss.wav, or ..._hhmm.wav.

subroutine msk144_files(offset,nfiles,ntrperiod,ndepth,nrxfreq,ntol,mycall,hiscall,lshmsg)

  use readwav
  use prog_args, only : data_dir

  integer, parameter :: NMAX=30*12000
  integer, intent(in) :: offset,nfiles,ntrperiod,ndepth,nrxfreq,ntol
  character(len=12), intent(in) :: mycall,hiscall
  logical, intent(in) :: lshmsg
  type(wav_header) wav
  integer*2, allocatable :: id2(:)
  character(len=500) :: infile
  integer :: iarg,arglen,i1,nutc,nsamp,ndecodes

  if(ntrperiod.lt.1 .or. ntrperiod.gt.30) then
     print*,'msk144_files: the T/R period (-p) is in seconds here, 1 to 30 - got',ntrperiod
     return
  endif
  allocate(id2(NMAX))

  do iarg=offset+1,offset+nfiles
     call get_command_argument(iarg,infile,arglen)
     if(arglen.gt.len(infile)) then
        print*,'msk144_files: argument',iarg-offset,' is longer than ',len(infile),' characters - skipped'; cycle
     endif
     call wav%read(infile)
     if(wav%audio_format%sample_rate.ne.12000) then
        print*,'msk144_files: ',trim(infile),' is not 12000 Hz - skipped'
        close(unit=wav%lun); cycle
     endif
     if(wav%audio_format%bits_per_sample.ne.16) then
        print*,'msk144_files: ',trim(infile),' is not 16 bit - skipped'
        close(unit=wav%lun); cycle
     endif
     nsamp=min(ntrperiod*12000,wav%data_size/2)
     id2=0
     if(nsamp.gt.0) read(unit=wav%lun) id2(1:nsamp)
     close(unit=wav%lun)

     nutc=0
     i1=index(infile,'.wav'); if(i1.lt.1) i1=index(infile,'.WAV')
     if(i1.gt.7) then
        if(infile(i1-7:i1-7).eq.'_') then
           read(infile(i1-6:i1-1),*,err=1) nutc
        else if(infile(i1-5:i1-5).eq.'_') then
           read(infile(i1-4:i1-1),*,err=1) nutc; nutc=100*nutc
        endif
     endif
1    continue

     call decode_msk144(id2,ntrperiod,nutc,ntol,nrxfreq,ndepth,mycall,hiscall,lshmsg,trim(data_dir),ndecodes)
     write(*,1002) 0,ndecodes,0
1002 format('<DecodeFinished>',2i4,i9)
     call flush(6)
  enddo

  deallocate(id2)
  return
end subroutine msk144_files
