! This source code file was last time modified by Igor UA3DJY on September 30th, 2018
! All changes are shown in the patch file coming together with the full JTDX source code.

program jt9

! Decoder for JT9. Can run as the back end of JTDX only, with data placed in a shared memory region.

  use options
  use prog_args
  use jtdx_iface     ! CE3TSK: who this decoder is - the GUI checks it, see -v below
  use, intrinsic :: iso_c_binding
  use FFTW3
!  use timer_module, only: timer
!  use timer_impl, only: init_timer, fini_timer
!  use readwav

  include 'jt9com.f90'

  integer(C_INT) iret
  type(dec_data), allocatable :: szprobe   ! CE3TSK: -v only, to report sizeof(dec_data) honestly
!  type(wav_header) wav
  character c
  character(len=500) optarg
  character wisfile*520   ! CE3TSK: was *80 - a data directory over 65 characters cut the name AND its closing NUL off,
                          ! and FFTW read an unterminated string (data_dir is len=500, prog_args.f90)
  ! CE3TSK: band edges default to the full 0-5000 Hz the GUI can pass (the waterfall's
  ! start frequency and Fmax, capped at 5000) rather than WSJT-X's 200-4000; to reproduce
  ! a GUI replay exactly give -L/-H the waterfall's actual range
  integer :: arglen,stat,offset,remain,mode=0,flow=0,fsplit=2700,            &
       fhigh=5000,nrxfreq=1500,ntrperiod=1,ndepth=1
  ! CE3TSK: file mode settings, mirroring what the GUI sends the decoder
  integer :: ncycles=3,nswlcycles=3,nsens=1,nrxfsens=2,naggr=1,ncandthin=100,nthreads=0,nensemble=0,nbgeffort=0,nbgbudget=0,   &
             nrxbudget=27
  ! CE3TSK: the TX background recipe (pipeline ensemble), defaults = the pipeline presets' background
  integer :: nbgswl=1,nbgcycles=3,nbgswlcycles=5,nbgosd=0,nbgtwo=0,nbgalt=1,nbgens=-1,nbgsens=1,nbgrxf=2,nbgclassic=6
  logical :: lswl=.false.,ldeeposd=.false.,learly=.false.,lhidedupes=.false.,ltwo=.false.,lalt=.false.,lagcccomp=.false.
  logical :: lrxbudgetset=.false.   ! CE3TSK item 80: -l given (the RX budget's default is per mode otherwise)
  integer :: nsftol=0   ! CE3TSK: -o, SuperFox receive (SUPERFOX_PLAN.md): 0 off, else the sync search range in Hz
  logical :: read_files = .true., tx9 = .false., display_help = .false.
  type (option) :: long_options(54) = [ &
    option ('help', .false., 'h', 'Display this help message', ''),          &
    option ('version', .false., 'v',                                      &
        'Print name, version and shared-memory interface fingerprint',     &
        ''),                                                               &
    option ('shmem',.true.,'s','Use shared memory for sample data','KEY'),   &
    option ('tr-period', .true., 'p', 'Tx/Rx period, default MINUTES=1',     &
        'MINUTES'),                                                          &
    option ('executable-path', .true., 'e',                                  &
        'Location of subordinate executables (KVASD) default PATH="."',      &
        'PATH'),                                                             &
    option ('data-path', .true., 'a',                                        &
        'Location of writeable data files, default PATH="."', 'PATH'),       &
    option ('temp-path', .true., 't',                                        &
        'Temporary files path, default PATH="."', 'PATH'),                   &
    option ('share-path', .true., 'r',                                       &
        'Share files path, default PATH="."', 'PATH'),                       &
    option ('lowest', .true., 'L',                                           &
        'Lowest frequency decoded (JT65), default HERTZ=200', 'HERTZ'),      &
    option ('highest', .true., 'H',                                          &
        'Highest frequency decoded, default HERTZ=4007', 'HERTZ'),           &
    option ('split', .true., 'S',                                            &
        'Lowest JT9 frequency decoded, default HERTZ=2700', 'HERTZ'),        &
    option ('rx-frequency', .true., 'f',                                     &
        'Receive frequency offset, default HERTZ=1500', 'HERTZ'),            &
    option ('patience', .true., 'w',                                         &
        'FFTW3 planing patience (0-4), default PATIENCE=1', 'PATIENCE'),     &
    option ('fft-threads', .true., 'm',                                      &
        'Number of threads to process large FFTs, default THREADS=1',        &
        'THREADS'),                                                          &
    option ('jt65', .false., '6', 'JT65 mode', ''),                          &
    option ('jt9', .false., '9', 'JT9 mode', ''),                            &
    option ('sub-mode', .true., 'b', 'Sub mode, default SUBMODE=A', 'A'),    &
    option ('depth', .true., 'd',                                            &
        'JT9 decoding depth (1-3), default DEPTH=1', 'DEPTH'),               &
    option ('tx-jt9', .false., 'T', 'Tx mode is JT9', ''),                   &
    option ('my-call', .true., 'c', 'my callsign', 'CALL'),                  &
    option ('my-grid', .true., 'G', 'my grid locator', 'GRID'),              &
    option ('his-call', .true., 'x', 'his callsign', 'CALL'),                &
    option ('his-grid', .true., 'g', 'his grid locator', 'GRID'),             &
    ! CE3TSK: file mode. jtdxjt9 -8 [options] file.wav decodes a file the way File -> Open
    ! does, printing decodes to stdout - restored so decoder changes can be measured
    ! without a GUI replay for every experiment.
    option ('ft8', .false., '8', 'FT8 mode', ''),                               &
    option ('ft4', .false., '4', 'FT4 mode', ''),                               &
    option ('ft8-cycles', .true., 'C',                                          &
        'FT8 decode cycles (3-9), default CYCLES=3', 'CYCLES'),                 &
    option ('ft8-swl-cycles', .true., 'K',                                      &
        'FT8 SWL decode cycles (3-9), default CYCLES=3', 'CYCLES'),             &
    option ('ft8-sensitivity', .true., 'E',                                     &
        'FT8 sensitivity: 0 normal, 1 low thresholds, 2 plus subpass, default 1', 'SENS'), &
    option ('ft8-rxf-sens', .true., 'R',                                        &
        'FT8 RX frequency sensitivity (1-3), default 2', 'SENS'),               &
    option ('aggressive', .true., 'A',                                          &
        'aggressive decoding level (1-5), default 1', 'LEVEL'),                 &
    option ('swl', .false., 'W', 'SWL mode (6 pass)', ''),                      &
    option ('cand-thin', .true., 'N',                                           &
        'candidate list thinning (1-100), default 100', 'PERCENT'),             &
    option ('threads', .true., 'j',                                             &
        'FT8 decoder threads, 0 = auto from core count like the GUI, default 0', 'THREADS'), &
    option ('deep-osd', .false., 'O', 'OSD order 2 for every FT8 candidate (weak-signal mode)', ''), &
    option ('early-start', .false., 'y', 'FT8 early start as the GUI option: 48 blocks', ''),    &
    option ('hide-dupes', .false., 'u', 'hide FT8 dupe messages as the GUI option', ''),    &
    option ('two-slicings', .false., 'z', 'second slicing pass, thread slices offset by half a slice (threads > 1)', ''), &
    option ('alt-pass', .false., 'X',                                          &
        'alternate-approach pass: 7 cycles + OSD order 2 on the subtracted band (threads > 1)', ''), &
    option ('agc-comp', .false., 'Q',                                          &
        'AGC compensation as the GUI option (agccft8 + the AGC sync metric)', ''), &
    option ('ensemble', .true., 'M',                                           &
        'ensemble members 0-5, or budget: as many as fit the RX budget (-l)', &
        'MEMBERS'),                                                                &
    option ('rx-budget', .true., 'l',                                          &
        'RX budget for -M budget, tenths of a second from the decode start, default 27', 'TENTHS'), &
    option ('background', .true., 'B',                                         &
        'TX background phase after the period: 1 on, 0 off (pipeline ensemble)', 'ON'), &
    option ('bg-budget', .true., 'k',                                        &
        'background: window from the decode start, tenths of a second, 0 = the period (GUI rules only)', 'TENTHS'), &
    option ('bg-swl', .true., 'I', 'background: SWL mode 0/1, default 1', 'ON'),       &
    option ('bg-cycles', .true., 'D', 'background: decoding cycles 3-9, default 3', 'CYCLES'), &
    option ('bg-swl-cycles', .true., 'F', 'background: SWL decoding cycles 3-9, default 5', 'CYCLES'), &
    option ('bg-deep-osd', .true., 'J', 'background: OSD order 2 for every candidate 0/1, default 0', 'ON'), &
    option ('bg-two-slicings', .true., 'P', 'background: second slicing pass 0/1, default 0', 'ON'), &
    option ('bg-alt-pass', .true., 'U', 'background: alternate-approach pass 0/1, default 1', 'ON'), &
    option ('bg-ensemble', .true., 'V', 'background: ensemble members 0-5, or auto (every member in file mode)', 'MEMBERS'), &
    option ('bg-sensitivity', .true., 'Y', 'background: sensitivity 0/1/2, default 1', 'SENS'), &
    option ('bg-rxf-sens', .true., 'Z', 'background: RX frequency sensitivity 1-3, default 2', 'SENS'), &
    option ('bg-classic', .true., 'n',                                        &
        'background: the classic unit (P9), a plain non-SWL decode with CYCLES cycles (3-9), 0 off, default 6', 'CYCLES'), &
    option ('superfox', .true., 'o',                                          &
        'SuperFox receive (SuperHound): even slots go to the SuperFox decoder, which looks for the sync tone'  &
        //' HERTZ either side of -f (normally -f 750 -o 50); 0 off, default 0', 'HERTZ') ]

  character(len=12) :: mycall, hiscall
  character(len=6) :: mygrid, hisgrid
  common/patience/npatience,nthreads
  data npatience/1/,nthreads/1/

  nsubmode = 0

  ! CE3TSK: blank before the options are parsed - an option not given left its string as
  ! stack garbage, and jt9files() hands them to the decoder: a garbage hiscall made the
  ! "MyCall DxCall" a-priori passes decode differently from one run to the next (found with
  ! valgrind on the FT4 hint memory work, 2026-08-29; file mode only, the GUI clears its block)
  wisfile=''   ! CE3TSK 2026-09-25: an unrecognised option jumps to the cleanup, which exports
               ! FFTW wisdom to this name - uninitialised, it wrote a file with a junk name into
               ! the caller's directory every time an older decoder was probed with -v
  mycall=''; hiscall=''; mygrid=''; hisgrid=''
  do
     call getopt('hvs:e:a:b:r:m:p:d:f:w:t:9642TL:S:H:c:G:x:g:8C:K:E:R:A:WN:j:OyuzXQM:l:B:k:I:D:F:J:P:U:V:Y:Z:n:o:',   &
          long_options,c,   &
          optarg,arglen,stat,offset,remain,.true.)
     if (stat .ne. 0) then
        exit
     end if
     select case (c)
        case ('h')
           display_help = .true.
        ! CE3TSK 2026-09-25: the identity line the GUI reads at start-up. One line, machine
        ! readable, printed before any shared memory is touched: name, version, the fingerprint of
        ! the interface declarations (commons.h + jt9com.f90) and the size of the block itself. A
        ! decoder that is not this pair's - stock JTDX's, WSJT-X's, or one left over from an older
        ! build - either prints something else or does not know the option at all.
        case ('v')
           allocate(szprobe)
           write(*,'(a,1x,a,1x,a,1x,a,i0)') JTDX_IFACE_APP, 'jtdxjt9', JTDX_IFACE_VER,     &
                'iface='//JTDX_IFACE_HASH//' dec_data=', storage_size(szprobe)/8
           deallocate(szprobe)
           stop
        case ('s')
           read_files = .false.
           shm_key = optarg(:arglen)
        case ('e')
           exe_dir = optarg(:arglen)
        case ('a')
           data_dir = optarg(:arglen)
        case ('r')
           share_dir = optarg(:arglen)
        case ('b')
           nsubmode = ichar (optarg(:1)) - ichar ('A')
        case ('t')
           temp_dir = optarg(:arglen)
        case ('m')
           read (optarg(:arglen), *) nthreads
        case ('p')
           read (optarg(:arglen), *) ntrperiod
        case ('d')
           read (optarg(:arglen), *) ndepth
        case ('f')
           read (optarg(:arglen), *) nrxfreq
        case ('L')
           read (optarg(:arglen), *) flow
        case ('S')
           read (optarg(:arglen), *) fsplit
        case ('H')
           read (optarg(:arglen), *) fhigh
        case ('4')
           mode = 4
        case ('2')                       ! CE3TSK: FT2 - decoded by the FT4 chain from a x2 stretch
           mode = 52
        case ('8')                       ! CE3TSK: file mode options
           mode = 8
        case ('C')
           read (optarg(:arglen), *) ncycles
        case ('K')
           read (optarg(:arglen), *) nswlcycles
        case ('E')
           read (optarg(:arglen), *) nsens
        case ('R')
           read (optarg(:arglen), *) nrxfsens
        case ('A')
           read (optarg(:arglen), *) naggr
        case ('W')
           lswl = .true.
        case ('N')
           read (optarg(:arglen), *) ncandthin
        case ('j')
           read (optarg(:arglen), *) nthreads
        case ('O')
           ldeeposd = .true.
        case ('y')
           learly = .true.
        case ('u')
           lhidedupes = .true.
        case ('z')
           ltwo = .true.
        case ('X')
           lalt = .true.
        case ('Q')
           lagcccomp = .true.
        case ('M')
           if(optarg(:arglen).eq.'budget') then; nensemble=-2
           else if(optarg(:arglen).eq.'auto') then; nensemble=-1   ! CE3TSK item 73
           else; read (optarg(:arglen), *) nensemble; endif
        case ('l')
           read (optarg(:arglen), *) nrxbudget; lrxbudgetset=.true.
        case ('B')   ! CE3TSK: the TX background phase on or off ("auto" and any nonzero value switch it on)
           if(optarg(:arglen).eq.'auto') then; nbgeffort=1; else; read (optarg(:arglen), *) nbgeffort; endif
           if(nbgeffort.ne.0) nbgeffort=1
        case ('k')
           read (optarg(:arglen), *) nbgbudget
        case ('I')
           read (optarg(:arglen), *) nbgswl
        case ('D')
           read (optarg(:arglen), *) nbgcycles
        case ('F')
           read (optarg(:arglen), *) nbgswlcycles
        case ('J')
           read (optarg(:arglen), *) nbgosd
        case ('P')
           read (optarg(:arglen), *) nbgtwo
        case ('U')
           read (optarg(:arglen), *) nbgalt
        case ('V')
           ! CE3TSK item 73: the typed word is -3, distinct from the -1 default that already means auto for FT8 - FT4 must not run a background unless asked
           if(optarg(:arglen).eq.'auto') then; nbgens=-3; else; read (optarg(:arglen), *) nbgens; endif
        case ('Y')
           read (optarg(:arglen), *) nbgsens
        case ('Z')
           read (optarg(:arglen), *) nbgrxf
        case ('n')
           read (optarg(:arglen), *) nbgclassic
        case ('o')   ! CE3TSK: SuperFox receive
           read (optarg(:arglen), *) nsftol
        case ('6')
           if (mode.lt.65) mode = mode + 65
        case ('9')
           if (mode.lt.9.or.mode.eq.65) mode = mode + 9
        case ('T')
           tx9 = .true.
        case ('w')
           read (optarg(:arglen), *) npatience
        case ('c')
! CE3TSK: assigned, not read list-directed - a "/" ends a list-directed read, so -c PJ4/K1ABC
! arrived as "PJ4" and a compound call could not be tested in file mode at all (found with the
! SuperFox own-call test, 2026-09-19). The same for -x below.
           mycall = optarg(:arglen)
        case ('G')
           read (optarg(:arglen), *) mygrid
        case ('x')
           hiscall = optarg(:arglen)
        case ('g')
           read (optarg(:arglen), *) hisgrid
     end select
  end do

  if (display_help .or. stat .lt. 0                      &
       .or. (.not. read_files .and. remain .gt. 0)       &
       .or. (read_files .and. remain .lt. 1)) then
!
!     print *, 'Usage: jt9 [OPTIONS] file1 [file2 ...]'
!     print *, '       Reads data from *.wav files.'
!     print *, ''
!     print *, '       jt9 -s <key> [-w patience] [-m threads] [-e path] [-a path] [-t path]'
!     print *, '       Gets data from shared memory region with key==<key>'
!     print *, ''
!     print *, 'OPTIONS:'
!     print *, ''
!     do i = 1, size (long_options)
!       call long_options(i) % print (6)
!     end do
     go to 999
  endif

  iret=fftwf_init_threads()            !Initialize FFTW threading 

! Default to 1 thread, but use nthreads for the big ones
  call fftwf_plan_with_nthreads(1)

! Import FFTW wisdom, if available
  wisfile=trim(data_dir)//'/jt9_wisdom.dat'// C_NULL_CHAR
  iret=fftwf_import_wisdom_from_filename(wisfile)

  if (.not. read_files) then
     call jt9a()          !We're running under control of WSJT-X
  else
     ! CE3TSK: decode the files named on the command line
     call jt9files(offset,remain,mode,ndepth,flow,fsplit,fhigh,nrxfreq,ncycles,   &
          nswlcycles,nsens,nrxfsens,naggr,lswl,ncandthin,nthreads,ldeeposd,learly,lhidedupes,ltwo,lalt,lagcccomp,   &
          nensemble,nbgeffort,nbgswl,nbgcycles,nbgswlcycles,nbgosd,nbgtwo,nbgalt,nbgens,nbgsens,nbgrxf,nbgbudget,nrxbudget,   &
          nbgclassic,lrxbudgetset,nsftol,   &
          mycall,mygrid,hiscall,hisgrid)
  endif

999 continue

! Save wisdom and free memory
  iret=fftwf_export_wisdom_to_filename(wisfile)
  call four2a(a,-1,1,1,1)
  call filbig(-1.,0,0.,0,0.,0.,0)        !used for FFT plans
  call fftwf_cleanup_threads()
  call fftwf_cleanup()
end program jt9

! CE3TSK: decode wav files from the command line.
!
! Mirrors jt9a(): the samples go into dd (and dd8/dd4), a params_block is filled the way
! mainwindow.cpp fills dec_data.params for a disk file, and multimode_decoder() is called.
! Decodes print to stdout in the same "nutc snr dt freq ~ message" lines the GUI parses,
! which is also the format WSJT-X's jt9 prints, so the two can be diffed directly.
!
! 16 bit files are scaled by 65536 to the 32 bit range the decoder expects - the same thing
! MainWindow::read_wav_file() does since 16 bit support was added there.
subroutine jt9files(offset,nfiles,mode,ndepth,flow,fsplit,fhigh,nrxfreq,ncycles,   &
     nswlcycles,nsens,nrxfsens,naggr,lswl,ncandthin,nthreads,ldeeposd,learly,lhidedupes,ltwo,lalt,lagcccomp,   &
     nensemble,nbgeffort,nbgswl,nbgcycles,nbgswlcycles,nbgosd,nbgtwo,nbgalt,nbgens,nbgsens,nbgrxf,nbgbudget,nrxbudget,   &
          nbgclassic,lrxbudgetset,nsftol,   &
     mycall,mygrid,hiscall,hisgrid)
  use prog_args
  use ft8ensemble, only : nbgrun,nbgunits   ! CE3TSK: pipeline ensemble
  use ft4_mod1, only : nbg4run   ! CE3TSK: the FT4 TX background (item 59)
  use thread_ladder, only : decoder_threads,ft4_members_auto,ft4_bg_auto   ! CE3TSK item 73: -M auto / -V auto in FT4
  use omp_lib, only : omp_get_num_procs
  use readwav
  use jt65_mod6                    ! dd(NPTS)
  use ft8_mod1, only : dd8
  use ft4_mod1, only : dd4
  include 'jt9com.f90'
  integer, intent(in) :: mode   ! CE3TSK review 2026-09-05: JTDX_FILE_MODES switches a loop-local copy (modecur), never this
  integer, intent(in) :: offset,nfiles,ndepth,flow,fsplit,fhigh,nrxfreq,     &
       ncycles,nswlcycles,nsens,nrxfsens,naggr,ncandthin,nthreads,nensemble,nbgeffort,nbgbudget,nrxbudget,   &
       nbgswl,nbgcycles,nbgswlcycles,nbgosd,nbgtwo,nbgalt,nbgens,nbgsens,nbgrxf,nbgclassic
  logical, intent(in) :: ldeeposd,learly,lhidedupes,ltwo,lalt,lagcccomp
  logical, intent(in) :: lswl
  logical, intent(in) :: lrxbudgetset   ! CE3TSK item 80
  integer, intent(in) :: nsftol   ! CE3TSK: -o, SuperFox receive
  character(len=12), intent(in) :: mycall,hiscall
  character(len=6), intent(in) :: mygrid,hisgrid
  type(params_block) :: params
  integer(1), allocatable :: pzero(:)   ! CE3TSK: to zero the block, see below
  character(len=8) :: lockenv   ! CE3TSK: JTDX_BG_LOCK
  character(len=8) :: envval   ! CE3TSK diagnostic hooks: JTDX_STOPHINT, JTDX_DXCSEARCH, JTDX_BANDCHANGE, JTDX_DXCALL2, JTDX_NEWDAT0, JTDX_SHOUND0
  integer :: lenv,ienv,nbandfile
  integer :: ndxfile
  character(len=12) :: hiscall2
  integer :: llock,ilock
  type(wav_header) :: wav
  integer*2, allocatable :: i2(:)
  integer, allocatable :: i4(:)
  character(len=500) :: infile
  integer :: iarg,arglen,i1,nutc,npts1,nsamp,nbytes,nblocks,nlastsam,ios
  integer :: modecur,modelast,lmodes,imodes   ! CE3TSK: JTDX_FILE_MODES - this file's mode, the last decoded file's mode
  integer :: ndelayfile   ! CE3TSK: JTDX_NDELAY_FILE
  character(len=256) :: filemodes

  if(mode.ne.8 .and. mode.ne.4 .and. mode.ne.52) then
     print*,'jt9files: file decoding supports FT8 (-8), FT4 (-4) and FT2 (-2) only'
     return
  endif
! CE3TSK 2026-09-05: JTDX_FILE_MODES=844... - one digit per file, 8 or 4, overriding -8/-4 for
! that file. The GUI switches modes inside one decoder process (the thread pool, every
! threadprivate buffer, the FFTW plan cache and the hint memory all carry over, and the
! decoder sees lmodechanged); file mode could only ever run one mode per process, so that
! sequence was untestable under -fcheck or valgrind. Diagnostic hook, no effect unless set.
  call get_environment_variable('JTDX_FILE_MODES',filemodes,lmodes,imodes)
  if(imodes.eq.-1) print*,'jt9files: JTDX_FILE_MODES is longer than ',len(filemodes),' characters - ignored'
  if(imodes.ne.0) lmodes=0
  modelast=mode   ! CE3TSK review: the mode of the last file actually decoded - the first file reports no change, as before
  do iarg=offset+1,offset+nfiles
     ! CE3TSK review: every file starts from the command-line mode, so a list longer than the digit
     ! string (or a character other than 8/4) falls back to -8/-4 instead of inheriting the previous
     ! file's override; mode itself is never written
     modecur=mode
     if(lmodes.ge.iarg-offset) then
        if(filemodes(iarg-offset:iarg-offset).eq.'8') modecur=8
        if(filemodes(iarg-offset:iarg-offset).eq.'4') modecur=4
        if(filemodes(iarg-offset:iarg-offset).eq.'2') modecur=52   ! CE3TSK
     endif
     call get_command_argument(iarg,infile,arglen)
     ! CE3TSK review: arglen is the argument's full length even when it was truncated into infile,
     ! so the old infile=infile(:arglen) was a substring fault under -fcheck for a path over 500
     ! characters (and a no-op otherwise); such a file is skipped with a message instead
     if(arglen.gt.len(infile)) then
        print*,'jt9files: argument',iarg-offset,' is longer than ',len(infile),' characters - skipped'; cycle
     endif
     call wav%read(infile)
     if(wav%audio_format%sample_rate.ne.12000) then
        print*,'jt9files: ',trim(infile),' is not 12000 Hz - skipped'
        close(unit=wav%lun); cycle
     endif
     if(modecur.eq.8) then; npts1=180000
     else if(modecur.eq.52) then; npts1=36864   ! CE3TSK: FT2 captures 3.072 s and is stretched x2 into dd4
     else; npts1=73728; endif
     nbytes=wav%audio_format%bits_per_sample/8
     nsamp=min(npts1,wav%data_size/nbytes)
     dd(1:npts1)=0.
     if(wav%audio_format%bits_per_sample.eq.16) then
        allocate(i2(nsamp)); read(unit=wav%lun) i2
        dd(1:nsamp)=65536.0*real(i2)
        deallocate(i2)
     else if(wav%audio_format%bits_per_sample.eq.32) then
        allocate(i4(nsamp)); read(unit=wav%lun) i4
        dd(1:nsamp)=real(i4)
        deallocate(i4)
     else
        print*,'jt9files: ',trim(infile),' is neither 16 nor 32 bit - skipped'
        close(unit=wav%lun); cycle
     endif
     close(unit=wav%lun)

     ! the UTC from the filename, as read_wav_file() derives it: ..._hhmmss.wav or ..._hhmm.wav
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

     ! what jt9a() does for a disk file: FT8 decodes only the first nblocks*3456 samples
     if(modecur.eq.8) then
        ! CE3TSK: -o is S-Hound, and the GUI never starts early there (ft8EarlyStartInEffect): a
        ! late Fox would lose its last symbols to the 48-block cut
        if(lswl) then; nblocks=51; else if(learly .and. nsftol.le.0) then; nblocks=48; else; nblocks=49; endif
        nlastsam=nblocks*3456
        dd(nlastsam+1:npts1)=0.
        dd8(1:npts1)=dd(1:npts1)
     else if(modecur.eq.52) then
        nblocks=0
        do i=1,npts1                     ! CE3TSK: the x2 stretch, as jt9a.f90 does it for live audio
           dd4(2*i-1)=dd(i); dd4(2*i)=dd(i)
        enddo
     else
        nblocks=0
        dd4(1:npts1)=dd(1:npts1)
     endif

     ! the parameter block, as mainwindow.cpp fills it, with settings from the options.
     ! CE3TSK: zeroed first - the GUI's block starts life zeroed, this one is a stack variable,
     ! and every field the options do not cover (datetime, the grids, ...) was garbage
     allocate(pzero(storage_size(params)/8)); pzero=0; params=transfer(pzero,params); deallocate(pzero)
     params%mycall=transfer(mycall,params%mycall)
     params%mybcall=transfer(mycall,params%mybcall)
     params%hiscall=transfer(hiscall,params%hiscall)
     params%hisbcall=transfer(hiscall,params%hisbcall)
     params%hisgrid=transfer(hisgrid,params%hisgrid)
     params%listutc=0
     params%napwid=5
     params%nQSOProgress=0
     ! CE3TSK item 75: the QSO state file mode never had - JTDX_QSOPROGRESS=0..5 and JTDX_LASTTX=0..6 (the GUI's nQSOProgress / nlasttx)
     call get_environment_variable('JTDX_QSOPROGRESS',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) then
        read(envval(1:lenv),*,iostat=ienv) nbandfile; if(ienv.eq.0) params%nQSOProgress=max(0,min(5,nbandfile))
     endif
     params%nftx=nrxfreq
     params%nutc=nutc
     params%ntrperiod=15; if(modecur.eq.4) params%ntrperiod=8; if(modecur.eq.52) params%ntrperiod=4   ! CE3TSK: informational only
     params%nfqso=nrxfreq
     params%npts8=74736
     params%nfa=flow
     params%nfsplit=fsplit
     params%nfb=fhigh
     params%ntol=20
     params%kin=nsamp
     params%nzhsym=nblocks
     params%ndepth=ndepth
     params%ncandthin=ncandthin
     params%ndtcenter=0
     params%nft8cycles=ncycles
     params%nft8swlcycles=nswlcycles
     params%ntxmode=modecur
     params%nmode=modecur
     params%nlist=0
     params%nranera=3
     params%ntrials10=1
     params%ntrialsrxf10=1
     params%naggressive=naggr
     params%nharmonicsdepth=0
     params%ntopfreq65=2700
     params%nprepass=4
     params%nsdecatt=1
     params%nlasttx=0
     call get_environment_variable('JTDX_LASTTX',envval,lenv,ienv)   ! item 75
     if(ienv.eq.0 .and. lenv.gt.0) then
        read(envval(1:lenv),*,iostat=ienv) nbandfile; if(ienv.eq.0) params%nlasttx=max(0,min(6,nbandfile))
     endif
     params%ndelay=0
! CE3TSK 2026-09-05: JTDX_NDELAY=n (tenths of a second, as the GUI's m_delay) - the partial
! interval the GUI reports when monitoring starts mid-period (partintft4/partintft8 shuffle the
! buffer and fill the gap with noise); never set by file mode otherwise, so the path was
! untestable here. Diagnostic hook, decodes unchanged without it.
! CE3TSK review 2026-09-05: applied to ONE file of the list - the first, or the k-th with
! JTDX_NDELAY_FILE=k - as the GUI sets m_delay on the single interval where monitoring started and
! clears it after that decode (mainwindow.cpp ~2197, ~2522). Applied to every file it also skipped
! the avexdt update (decoder.f90 skips it while ndelay is set) for the whole run, so the sequence
! the hook exists to test - one partial interval, then normal ones - could not be produced.
     call get_environment_variable('JTDX_NDELAY',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) then
        read(envval(1:lenv),*,iostat=ienv) nbandfile
        if(ienv.eq.0) then
           ndelayfile=1
           call get_environment_variable('JTDX_NDELAY_FILE',envval,lenv,ienv)
           if(ienv.eq.0 .and. lenv.gt.0) then
              read(envval(1:lenv),*,iostat=ienv) ndelayfile; if(ienv.ne.0) ndelayfile=1
           endif
           if(iarg-offset.eq.ndelayfile) params%ndelay=max(0,min(50,nbandfile))
        endif
     endif
     params%nmt=nthreads
     params%nft8rxfsens=nrxfsens
     params%nft4depth=min(3,max(1,ndepth))   ! CE3TSK: -d applies to FT4 too (1: 1 pass, 2: BP only, 3: BP+OSD)
     ! CE3TSK: the rest of the FT4 block. These were left unset, so file mode ran on whatever
     ! static storage happened to hold - false and zero, which is stable but is not what the GUI
     ! sends: FT4TwoSlicings defaults ON there, because the slice boundaries cost 26 of 536
     ! messages without it (FT4_THREADING_PLAN.md). File mode now starts from the same defaults,
     ! so a measurement here means what production does; JTDX_FT4_TWOPASS / _ALT / _ENSEMBLE
     ! still override, which is how the comparison rows in ft4hint.sh ask for the other side.
     params%lft4twopass=.true.
     params%lft4altpass=lalt
     params%nft4ensemble=nensemble
     params%nft4bgensemble=0
     if((modecur.eq.4 .or. modecur.eq.52) .and. nbgens.gt.0) params%nft4bgensemble=min(6,nbgens)   ! CE3TSK item 59: -V = the background's target member count in FT4 (and FT2, which shares the fields)
     ! CE3TSK item 78: -B 1 is the FT4 TX background switch exactly as it is FT8's (nft8bgeffort); -V alone
     ! no longer implies a background, and -B 1 with -V at or below -M runs the phase's extras alone
     params%nft4bgeffort=merge(1,0,(modecur.eq.4 .or. modecur.eq.52) .and. nbgeffort.ne.0)
     if(modecur.eq.4 .or. modecur.eq.52) then   ! CE3TSK item 73: -M auto (-1) and -V auto (-1) resolve by the thread ladder, as the GUI does
        if(nensemble.eq.-1) params%nft4ensemble=ft4_members_auto(decoder_threads(nthreads,omp_get_num_procs()))
        if(nbgens.eq.-3) params%nft4bgensemble=ft4_bg_auto(decoder_threads(nthreads,omp_get_num_procs()))
     endif
     ! CE3TSK item 69: the FT4 TX background's own settings - the GUI's TX background submenu; file-mode
     ! hooks JTDX_FT4_BGDEPTH (0 = the RX phase's), JTDX_FT4_BGOSD, JTDX_FT4_BGALT, JTDX_FT4_BGTWOPASS
     params%nft4bgdepth=0; params%lft4bgdeeposd=.false.; params%lft4bgaltpass=.false.; params%lft4bgtwopass=.true.
     call get_environment_variable('JTDX_FT4_BGDEPTH',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) then
        read(envval(1:lenv),*,iostat=ienv) nbandfile
        if(ienv.eq.0) params%nft4bgdepth=max(0,min(3,nbandfile))
     endif
     call get_environment_variable('JTDX_FT4_BGOSD',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) params%lft4bgdeeposd=(envval(1:1).eq.'1')
     call get_environment_variable('JTDX_FT4_BGALT',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) params%lft4bgaltpass=(envval(1:1).eq.'1')
     call get_environment_variable('JTDX_FT4_BGTWOPASS',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) params%lft4bgtwopass=(envval(1:1).ne.'0')
     ! CE3TSK item 72: the residual unit (TX background) and the decoder sensitivity (RX), file-mode hooks
     params%lft4bgresidual=.false.; params%nft4sens=0; params%nft4bgsens=0
     params%nft4rxfsens=max(0,min(3,nrxfsens)); params%nft4bgrxfsens=max(0,min(3,nbgrxf))   ! CE3TSK item 75: -R / -Z as for FT8 (2 = medium)
     call get_environment_variable('JTDX_FT4_BGRESIDUAL',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) params%lft4bgresidual=(envval(1:1).eq.'1')
     call get_environment_variable('JTDX_FT4_SENS',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) params%nft4sens=merge(1,0,envval(1:1).eq.'1')
     call get_environment_variable('JTDX_FT4_BGSENS',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) params%nft4bgsens=merge(1,0,envval(1:1).eq.'1')
     params%nsecbandchanged=0
     params%ndiskdat=.true.
     params%newdat=.true.
     params%nagain=.false.
     params%nagainfil=.false.
     params%nswl=lswl
     params%nfilter=.false.
     params%nstophint=.false.
     ! CE3TSK diagnostic hook: JTDX_STOPHINT=1 sends nstophint the way an idle GUI does (no QSO
     ! in progress: the DX-call-search AP types run, the QSO hint decoder does not)
     call get_environment_variable('JTDX_STOPHINT',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) params%nstophint=(envval(1:1).eq.'1')
     params%nagcc=lagcccomp
     params%nhint=.true.
     params%fmaskact=.false.
     params%showharmonics=.false.
     params%lft8lowth=(nsens.ge.1)
     params%lft8subpass=(nsens.ge.2)
     params%ltxing=.false.
     params%lhidetest=.false.
     params%lhidetelemetry=.false.
     params%lhideft8dupes=lhidedupes
     params%lhound=.false.
! CE3TSK: -o N = SuperHound as the GUI sets it: Hound on and the SuperFox receiver in the even
! slots, searching N Hz either side of -f. The slot is read from the file name's _hhmmss; a name
! without one counts as even.
     params%nsftol=max(0,nsftol)
     if(nsftol.gt.0) params%lhound=.true.
! CE3TSK test hook: JTDX_HOUND=1 is Hound mode alone, which file mode otherwise never sets. It is
! what test/decode/superfox.sh compares -o against in the odd slot: -o switches Hound on as well, so
! the FT8 decode it must equal is a Hound-mode one, not a plain one.
     call get_environment_variable('JTDX_HOUND',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) then
        if(envval(1:1).eq.'1') params%lhound=.true.
     endif
     params%lhidehash=.false.
     params%lcommonft8b=.true.
     params%lmycallstd=(len_trim(mycall).gt.0)
     params%lhiscallstd=(len_trim(hiscall).gt.0)
     ! CE3TSK test hooks (2026-09-20, review 2): JTDX_DXCALL2=n:CALL - from the n-th file on the DX call is
     ! CALL (the operator changes DX Call within a session; file mode has one -x); JTDX_NEWDAT0=n - the
     ! n-th file comes as a RE-DECODE of its period (newdat false, as the Decode button sends it)
     call get_environment_variable('JTDX_DXCALL2',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.2) then
        nbandfile=index(envval(1:lenv),':')
        if(nbandfile.gt.1 .and. nbandfile.lt.lenv) then
           read(envval(1:nbandfile-1),*,iostat=ienv) ndxfile
           if(ienv.eq.0 .and. iarg-offset.ge.ndxfile) then
              hiscall2=envval(nbandfile+1:lenv)
              params%hiscall=transfer(hiscall2,params%hiscall); params%hisbcall=transfer(hiscall2,params%hisbcall)
           endif
        endif
     endif
     call get_environment_variable('JTDX_NEWDAT0',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) then
        read(envval(1:lenv),*,iostat=ienv) nbandfile; if(ienv.eq.0 .and. iarg-offset.eq.nbandfile) params%newdat=.false.
     endif
     ! ... and JTDX_SHOUND0=n: the n-th file is decoded with S-Hound mode OFF, as when the operator leaves
     ! the mode and comes back (Hound mode stays on) - what the SuperFox receiver remembered must be gone
     call get_environment_variable('JTDX_SHOUND0',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) then
        read(envval(1:lenv),*,iostat=ienv) nbandfile; if(ienv.eq.0 .and. iarg-offset.eq.nbandfile) params%nsftol=0
     endif
     params%lapmyc=.false.
     ! CE3TSK test hook: JTDX_APMYC=1 - "I have transmitted in the last two minutes", which the GUI
     ! reports and file mode never could: with JTDX_QSOPROGRESS it opens the a-priori types for MY call
     call get_environment_variable('JTDX_APMYC',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) then
        if(envval(1:1).eq.'1') params%lapmyc=.true.
     endif
     params%lmodechanged=(modecur.ne.modelast)   ! CE3TSK: JTDX_FILE_MODES switched it since the last DECODED file (a skipped file does not count), as the GUI reports a mode change
     params%lbandchanged=.false.
     ! CE3TSK test hook: JTDX_BANDCHANGE=n flags a band change on the n-th file of the list (the
     ! decoder then empties its hint and call/DT lists, as after a real band change)
     call get_environment_variable('JTDX_BANDCHANGE',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) then
        read(envval(1:lenv),*,iostat=ienv) nbandfile; if(ienv.eq.0 .and. iarg-offset.eq.nbandfile) params%lbandchanged=.true.
     endif
     params%lenabledxcsearch=.true.
     ! CE3TSK diagnostic hook: JTDX_DXCSEARCH=0 disables the DX-call search as the GUI does with an empty DX Call box
     call get_environment_variable('JTDX_DXCSEARCH',envval,lenv,ienv)
     if(ienv.eq.0 .and. lenv.gt.0) params%lenabledxcsearch=(envval(1:1).ne.'0')
     params%lwidedxcsearch=.true.
     params%lmultinst=.false.
     params%lskiptx1=.false.
     params%lforcesync=.false.
     params%learlystart=learly .and. nsftol.le.0
     params%lft8deeposd=ldeeposd
     params%lft4deeposd=ldeeposd   ! CE3TSK: -O reaches FT4's deep OSD too (item 58; ft4bg.sh checks the reach)
     params%lft8twopass=ltwo
     params%lft8altpass=lalt
     params%nft8ensemble=nensemble
     params%nft8bgeffort=merge(nbgeffort,0,modecur.eq.8)   ! item 78: -B reaches one mode's switch, as the GUI sends it
     params%nbgmargin=10; params%nbgbudget=nbgbudget
     ! CE3TSK item 80 review: the RX budget's default is per mode - 2.7 s for FT8 (P8), 1.3 s for FT4 against its
     ! 1.36 s reply deadline (the GUI sends FT4RXBudget=13); -l TENTHS overrides either. The variable's 27 initial
     ! value used to reach FT4 as it was, so `-M budget` without -l ran a 2.7 s budget there
     params%nrxbudget=nrxbudget; if(modecur.eq.4 .and. .not.lrxbudgetset) params%nrxbudget=13
! CE3TSK: FT2's reply deadline is 3.75 - 22*1728/12000 = 0.58 s, so its RX budget is 5 tenths where
! FT4 gets 13 against 1.45 s. Measured tuning comes later (step 3 of the port).
     if(modecur.eq.52 .and. .not.lrxbudgetset) params%nrxbudget=5
     params%lbgswl=(nbgswl.ne.0); params%nft8bgcycles=nbgcycles; params%nft8bgswlcycles=nbgswlcycles
     params%lbgdeeposd=(nbgosd.ne.0); params%lbgtwopass=(nbgtwo.ne.0); params%lbgaltpass=(nbgalt.ne.0)
     params%nft8bgensemble=merge(-1,nbgens,nbgens.eq.-3); params%lbglowth=(nbgsens.ge.1); params%lbgsubpass=(nbgsens.ge.2)   ! item 73: the typed 'auto' is FT8's -1
     params%nft8bgrxfsens=nbgrxf
     params%nft8bgclassic=nbgclassic

     ! CE3TSK: multimode_decoder ends by polling up to 1 s for temp_dir/.lock (the GUI's
     ! "decode finished, you may exit" handshake); create it so file mode does not idle
     open(unit=98,file=trim(temp_dir)//'/.lock',status='replace'); close(98)
     call multimode_decoder(params)
     modelast=modecur   ! CE3TSK review: only a decoded file counts for the next file's lmodechanged
     call flush(6)
     if(params%nft4bgeffort.ne.0) then
        ! CE3TSK: the FT4 TX background (item 59, -B 1 since item 78); JTDX_BG_LOCK=1 runs it under the GUI's abort rules
        call get_environment_variable('JTDX_BG_LOCK',lockenv,llock,ilock)
        nbg4run=2; if(ilock.eq.0 .and. llock.gt.0) nbg4run=1
        call multimode_decoder(params); nbg4run=0; call flush(6)
     endif
     if(nbgeffort.ne.0 .and. modecur.eq.8) then   ! CE3TSK: the background phase, explicitly nbgeffort units (-1: all), no clock
        ! JTDX_BG_LOCK=1: run it under the GUI's rules instead - the budget in auto, and an abort when
        ! temp_dir/.lock disappears (test/decode/bgabort.sh removes it mid-way)
        call get_environment_variable('JTDX_BG_LOCK',lockenv,llock,ilock)
        nbgrun=2; if(ilock.eq.0 .and. llock.gt.0) nbgrun=1
        nbgunits=nbgeffort; call multimode_decoder(params); nbgrun=0; call flush(6)
     endif
     open(unit=98,file=trim(temp_dir)//'/.lock',status='old',iostat=ios); if(ios.eq.0) close(98,status='delete')
  enddo
  return
end subroutine jt9files
