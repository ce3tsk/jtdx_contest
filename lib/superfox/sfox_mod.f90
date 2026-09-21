module sfox_mod
  
  parameter (NMAX=15*12000)       !Samples in iwave (180,000)
  integer MM,NQ,NN,KK,NS,NDS,NFZ,NSPS,NSYNC,NZ,NFFT1
  real baud,tsym,bw
! CE3TSK 2026-09-20: what sfox_demod hands qpc_decode2 beside its arguments, for the SNR estimate
! (sfox_demod.f90, the normalisation of tone bins that hold QRM): s3 as it would have been WITHOUT
! that step, and whether the step touched any bin at all in the last call.
  real, save :: s3plain(0:127,0:127)
  logical, save :: lnormed=.false.
! CE3TSK 2026-09-20: the Fox calls the receiver KNOWS, for qpc_decode2's acceptance floor (there):
! slot 1 the operator's DX call (sfox_known, called by the decoder before each Fox slot), slot 2 the
! last Fox decoded ABOVE the floor. A call in the two forms a SuperFox message carries it in: the
! 28 bits of pack28 (a standard call, or the token of a compound call's 22-bit hash) and the 58 bits
! of a CQ's eleven base-38 characters. -1 = none.
  integer, save :: nfoxknown28(2)=-1
  integer*8, save :: nfoxknown58(2)=-1
! CE3TSK 2026-09-20: what the A-PRIORI pass needs beside the Fox's call (qpc_decode2.f90 has the
! story; SUPERFOX_DECODER_IDEAS.md idea 5, stage A of the hint memory). The Fox's GRID as a CQ
! carries it (15 bits; slot 1 the operator's DX grid, slot 2 what the remembered Fox last sent in a
! CQ decoded above the floor), MY OWN call as pack28 writes it, and what the QSO lets the Fox send
! me: 0 nothing (no transmission of mine in the last two minutes, or no QSO with a Fox in
! progress), 1 a report (I am calling), 2 a report or RR73 (I have sent R+report) - the two rows
! of FT8's own Hound table that hold a-priori types (ft8b.f90, nhaptypes). -1 = none.
! lsfoxap: the decode qpc_decode2 has just returned was made by the a-priori pass; its lines are
! marked '*' as FT8's a-priori decodes are.
  integer, save :: nfoxgrid15(2)=-1
  integer, save :: nsfoxage=0           !Fox slots since slot 2's Fox was last decoded (sfrx_sub.f90: sfox_age)
! CE3TSK 2026-09-20: THE SETTINGS OF WHAT IS THIS PROGRAM'S OWN IN THE RECEIVER. Their values were
! found on simulated transmissions and two on-air recordings; until there is more on-air material
! they are settings and not constants (the operator, 2026-09-20). Read ONCE from the environment of
! the decoder process by sfox_config below - the GUI hands its own environment on - so a change
! takes a restart. A value that cannot be read - or is longer than 32 characters - STOPS the decoder with a message: a setting silently
! ignored would be measured as if it had been applied. The defaults are what was measured.
!   JTDX_SFOX_AP=0          the a-priori pass off                                  (default on)
!   JTDX_SFOX_APFAM=cmor    the families it tries: c the Fox's CQ, m the Fox answering ME alone,
!                           o me beside reports to others, r nothing but reports   (default all four)
!   JTDX_SFOX_APFLOOR1=x    the SNR floor of a message stated whole, dB            (default -20.5)
!   JTDX_SFOX_APFLOOR2=x    the floor of the other hypotheses                      (default -18.7)
!   JTDX_SFOX_APFLOOR=x     both floors at once
!   JTDX_SFOX_APLOOKS=1|2   forms of the spectra tried: themselves; the likelihoods too (default 2)
!   JTDX_SFOX_APAGE=n       Fox slots the remembered Fox and its grid are kept without a decode
!                           of that Fox; 0 = nothing is remembered                 (default 4)
!   JTDX_SFOX_POOL=0        the POOL pass off (below)                              (default on)
!   JTDX_SFOX_POOLL=n       its list size, 1 to 256                                (default 16)
!   JTDX_SFOX_POOLLOOKS=1|2 forms of the spectra it tries: themselves; the likelihoods too (default 2)
!   JTDX_SFOX_POOLCRC=n     how many of the list's paths, best first, may be tried against the
!                           CRC, 1 to 16                                           (default 1)
!   JTDX_SFOX_POOLFLOOR=x   the SNR floor of its decodes, dB                       (default -19.5)
!   JTDX_SFOX_POOLAGE=n     odd slots LISTENED TO in which a Hound may be absent before it leaves
!                           the pool (the clock stands while I transmit); 0 = no pool   (default 4)
!   JTDX_SFOX_FLOOR=x       the ordinary search's SNR floor for a Fox that is NOT known, dB: WSJT-X's
!                           -16.5, MSHV's -16.95 (a known Fox has none)             (default -16.95)
!   JTDX_SFOX_LISTANY=0     the list pass's round with NOTHING told off (below)      (default on)
!   JTDX_SFOX_LISTANYFLOOR=x its SNR floor, dB                                       (default -16.95)
!   JTDX_SFOX_LISTANYLOOKS=1|2|L the same for that round (83 of its 83 decodes on record came at the
!                           likelihoods; kept at 2 until on-air material says)     (default 2)
!   JTDX_SFOX_SYNC3=1       MSHV's THREE sync windows as the LAST step of a Fox slot (below)   (default OFF)
!   JTDX_SFOX_LIST=0        the LIST pass off (below)                              (default on)
!   JTDX_SFOX_LISTL=n       its list size, 1 to 256                                (default 64)
!   JTDX_SFOX_LISTLOOKS=1|2|L forms of the spectra it tries: themselves; the likelihoods too; L the
!                           likelihoods ONLY (for experiments: every AWGN decode on record came at that look)
!                                                                                  (default 2)
!   JTDX_SFOX_LISTCRC=n     how many of the list's paths, best first, may be tried against the
!                           CRC, 1 to 16                                           (default 4)
!   JTDX_SFOX_LISTFLOOR=x   the SNR floor of its decodes, dB                       (default -17.3)
!   JTDX_SFOX_NORMX=x       a tone bin is normalised when its mean power is over x times the
!                           noise's (sfox_demod)                                   (default 2.0)
! The switches that were there before are read where they act: JTDX_SFOX_NORM=0 (sfox_demod),
! JTDX_SFOX_KNOWN=0 (qpc_decode2), JTDX_SFQRM=0|1 (decoder.f90). JTDX_MEMO_STATS makes the pass
! say on stderr what it did.
  logical, save :: lsfcfg=.false.
  integer, save :: nsfap=1
  logical, save :: lsfapfam(4)=.true.
  real, save :: sfapfloor1=-20.5
  real, save :: sfapfloor2=-18.7
  integer, save :: nsfaplooks=2
  integer, save :: nsfapage=4
  real, save :: sfnormx=2.0
  logical, save :: lsfstats=.false.
! CE3TSK 2026-09-20: STAGE B of the hint memory - THE POOL (SUPERFOX_DECODER_IDEAS.md 4.11;
! test/experiments/sfox_scl). The Hounds heard calling or answering the Fox in DX Call, in the odd
! slots, by this program's FT8 decoder (decoder.f90: ft8_decoded -> sfox_pool_line): their calls
! as pack28 writes them, and from how many LISTENED odd slots each has been absent. When the ordinary search
! and stage A's pass have failed, the LIST decoder (qpc/qpc_scl.c) is run with every Hound slot of
! the message held to "one of the pool, or empty" (qpc_decode2.f90: sfox_poolpass). Forgotten with
! the rest on a band or mode change (sfox_forget) and when the DX call changes; a Hound leaves it
! when it was absent from JTDX_SFOX_POOLAGE odd slots that were LISTENED to (sfox_odd_listened) - so
! the pool keeps while I call in every odd slot, when no Hound can be heard.
  integer, parameter :: NSFPOOLMAX=62   !with "empty" and my own call: the list decoder's 64 candidates a slot
  integer, save :: nsfpool=0
  integer, save :: nsfpool28(NSFPOOLMAX)=-1
  integer, save :: nsfpoolold(NSFPOOLMAX)=0
  integer, save :: nsfpoolon=1
  integer, save :: nsfpooll=16
  integer, save :: nsfpoolcrc=1
  integer, save :: nsfpoollooks=2
  real, save :: sfpoolfloor=-19.5
  integer, save :: nsfpoolage=4
  integer, save :: nsfpoolfox=-1        !the Fox these Hounds answered, as pack28 writes it: the pool is THAT Fox's
! CE3TSK 2026-09-21: THE LIST PASS (qpc_decode2.f90: sfox_listpass; SUPERFOX_DECODER_IDEAS.md 4.11 "S2
! finished", test/experiments/sfox_scl row S2c): the pool pass without the pool - when the search,
! stage A's pass and the pool pass have found nothing, the list decoder with the known Fox TOLD and the
! Hound slots free. Nothing is remembered for it; these are its settings.
! THE FLOOR, -17.3 dB, and what it was laid on (test/experiments/sfox_scl/run_listpass.py, results/
! s6_*): a free list decoder picks the strongest tones, so even from NOISE its most probable word reads
! -17.9 dB (median) - this floor is not the pool pass's -19.5, whose words are held to candidates. The
! pass's TRUE decodes read -17.23 at the lowest (124 on AWGN; 62 fading: -16.70). At -17.3 the floor
! costs none of them and passes 8 of 3998 words of noise alone (0.2 %) and 78 of 2000 of a Fox at -19 /
! -20 dB (3.9 %): a chance CRC pass on a dead frequency, where a Hound waits longest, is 500 times less
! likely to be printed. It does NOT part the words of a Fox just under the threshold (127 of 610 pass):
! there the CRC stands alone, as in stage A. At -17.2 it would cost 1 true decode in 124, at -17.1 four.
! The margin is thin and the figures are simulated ones - a SETTING, to be looked at with on-air material.
! CE3TSK 2026-09-21: MSHV'S THREE SYNC WINDOWS AS THE LAST STEP (SUPERFOX_DECODER_IDEAS.md 4.14; decoder.f90
! superfox_extra, qpc_sync3.f90). OFF by default (the operator: an option until there is on-air material).
! When the receiver, its passes and the FT8 QRM remover have found nothing in a Fox slot, the search and the
! passes run once more on each distinct candidate of MSHV's three windows (RX +/- 60 Hz, 700-800 Hz,
! 200-3200 Hz), each only if it can end inside the RX budget - so everything decoded today keeps its time.
! Measured in the experiment (the extra candidates searched with the passes on each): no DX call 287 -> 317
! of 600 (AWGN), 316 -> 331 (fading); DX call 357 -> 389, 350 -> 366 - about +0.15 to +0.25 dB, no line in
! 1000 periods of noise; about 1 s more in a slot in which nothing decodes. nsfextra: which candidate the
! receiver works on now (0 = the ordinary sync) - state, not a setting.
  integer, save :: nsfsync3=0
  integer, save :: nsfextra=0
  integer, save :: nsfliston=1
! CE3TSK 2026-09-21: ON PAR WITH MSHV FOR A FOX THAT IS NOT KNOWN (SUPERFOX_DECODER_IDEAS.md 4.14, idea 9):
! the search's floor as a setting - MSHV accepts down to -16.95 dB for every Fox - and a last round of the
! list pass with nothing told at all, for a Fox the receiver does not know, with a floor of its own.
! Measured (test/experiments/sfox_scl/results/s7_*, a busy Fox, NO DX call, 600 files a channel): WSJT-X's
! floor and no such round 211 (AWGN) / 287 (fading); the floor at -16.95 alone 233 / 287; the round alone
! 265 / 316; both 287 / 316 - 50 % at -17.2 dB (MSHV -17.2) and -15.7 (MSHV -15.6). The round's floor at
! -16.5 keeps 245, at -17.3 291: its true decodes read -16.95 and up here, while the best words of NOISE
! reach -16.95 once in 3998 and -17.3 in 124 - so -16.95, a hundred times fewer chances for 4 decodes in
! 600. No line in 2000 periods of noise and 1000 of a Fox at -19 / -20 dB; with BOTH floors taken away
! one false line in those 2000. About 0.15 s more in a period in which nothing decodes.
  real, save :: sfsearchfloor=-16.95
  integer, save :: nsflistany=1
  real, save :: sflistanyfloor=-16.95
  integer, save :: nsflistanylooks=2   !1 the spectra, 2 both, 3 ("L") the likelihoods alone - as nsflistlooks
  integer, save :: nsflistl=64
  integer, save :: nsflistcrc=4
  integer, save :: nsflistlooks=2
  real, save :: sflistfloor=-17.3
  integer, save :: nsfme28=-1
  integer, save :: nsfprog=0
  logical, save :: lsfoxap=.false.

contains
  subroutine sfox_init(mm0,nn0,kk0,itu,fspread,delay,fsample,ns0)

    character*2 itu
    integer isps(54)
    integer iloc(1)
    data isps/ 896, 960, 972, 980,1000,1008,1024,1029,1050,1080,   &
              1120,1125,1134,1152,1176,1200,1215,1225,1250,1260,   &
              1280,1296,1323,1344,1350,1372,1400,1440,1458,1470,   &
              1500,1512,1536,1568,1575,1600,1620,1680,1701,1715,   &
              1728,1750,1764,1792,1800,1875,1890,1920,1944,1960,   &
              2000,2016,2025,2048/

    MM=mm0              !Bits per symbol
    NQ=2**MM            !Q, number of MFSK tones
    NN=nn0              !Codeword length
    KK=kk0              !Number of information symbols
    NS=ns0              !Number of sync symbols
    NDS=NN+NS           !Total number of channel symbols
    NFZ=3               !First zero

    jsps=nint(12.8*fsample/NDS)
    iloc=minloc(abs(isps-jsps))
    NSPS=isps(iloc(1))  !Samples per symbol
    NSYNC=NS*NSPS       !Samples in sync waveform
    NZ=NSPS*NDS         !Samples in full Tx waveform
    NFFT1=2*NSPS        !Length of FFTs for symbol spectra
    
    baud=fsample/NSPS
    tsym=1.0/baud
    bw=NQ*baud

    fspread=0.0
    delay=0.0
    if(itu.eq.'LQ') then
       fspread=0.5
       delay=0.5
    else if(itu.eq.'LM') then
       fspread=1.5
       delay=2.0
    else if(itu.eq.'LD') then
       fspread=10.0
       delay=6.0
    else if(itu.eq.'MQ') then
       fspread=0.1
       delay=0.5
    else if(itu.eq.'MM') then
       fspread=0.5
       delay=1.0
    else if(itu.eq.'MD') then
       fspread=1.0
       delay=2.0
    else if(itu.eq.'HQ') then
       fspread=0.5
       delay=1.0
    else if(itu.eq.'HM') then
       fspread=10.0
       delay=3.0
    else if(itu.eq.'HD') then
       fspread=30.0
       delay=7.0
    endif

    return
  end subroutine sfox_init

  subroutine sfox_config

! the settings above, once for the process

    character*32 v
    real x
    integer n,i,j,ios

    if(lsfcfg) return
    lsfcfg=.true.
    call get_environment_variable('JTDX_MEMO_STATS',v,n,ios)       !not a setting of the receiver's: set or not, any length
    lsfstats=(ios.eq.0 .or. ios.eq.-1) .and. n.gt.0
    call get_environment_variable('JTDX_SFOX_AP',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_AP',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       if(v(1:n).eq.'0') then
          nsfap=0
       else if(v(1:n).ne.'1') then
          call sfox_badcfg('JTDX_SFOX_AP',v(1:n),'0 or 1')
       endif
    endif
    call get_environment_variable('JTDX_SFOX_APFAM',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_APFAM',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       lsfapfam=.false.
       do i=1,n
          j=index('cmorCMOR',v(i:i))
          if(j.lt.1) call sfox_badcfg('JTDX_SFOX_APFAM',v(1:n),'letters of cmor')
          lsfapfam(mod(j-1,4)+1)=.true.
       enddo
    endif
    call get_environment_variable('JTDX_SFOX_APFLOOR',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_APFLOOR',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isnum(v(1:n))) read(v(1:n),*,iostat=ios) x
       if(ios.ne.0 .or. x.lt.-99.0 .or. x.gt.30.0) call sfox_badcfg('JTDX_SFOX_APFLOOR',v(1:n),'dB, -99 to 30')
       sfapfloor1=x; sfapfloor2=x
    endif
    call get_environment_variable('JTDX_SFOX_APFLOOR1',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_APFLOOR1',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isnum(v(1:n))) read(v(1:n),*,iostat=ios) x
       if(ios.ne.0 .or. x.lt.-99.0 .or. x.gt.30.0) call sfox_badcfg('JTDX_SFOX_APFLOOR1',v(1:n),'dB, -99 to 30')
       sfapfloor1=x
    endif
    call get_environment_variable('JTDX_SFOX_APFLOOR2',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_APFLOOR2',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isnum(v(1:n))) read(v(1:n),*,iostat=ios) x
       if(ios.ne.0 .or. x.lt.-99.0 .or. x.gt.30.0) call sfox_badcfg('JTDX_SFOX_APFLOOR2',v(1:n),'dB, -99 to 30')
       sfapfloor2=x
    endif
    call get_environment_variable('JTDX_SFOX_APLOOKS',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_APLOOKS',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isint(v(1:n))) read(v(1:n),*,iostat=ios) i
       if(ios.ne.0 .or. i.lt.1 .or. i.gt.2) call sfox_badcfg('JTDX_SFOX_APLOOKS',v(1:n),'1 or 2')
       nsfaplooks=i
    endif
    call get_environment_variable('JTDX_SFOX_APAGE',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_APAGE',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isint(v(1:n))) read(v(1:n),*,iostat=ios) i
       if(ios.ne.0 .or. i.lt.0 .or. i.gt.1000) call sfox_badcfg('JTDX_SFOX_APAGE',v(1:n),'Fox slots, 0 to 1000')
       nsfapage=i
    endif
    call get_environment_variable('JTDX_SFOX_POOL',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_POOL',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       if(v(1:n).eq.'0') then
          nsfpoolon=0
       else if(v(1:n).ne.'1') then
          call sfox_badcfg('JTDX_SFOX_POOL',v(1:n),'0 or 1')
       endif
    endif
    call get_environment_variable('JTDX_SFOX_POOLL',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_POOLL',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isint(v(1:n))) read(v(1:n),*,iostat=ios) i
       if(ios.ne.0 .or. i.lt.1 .or. i.gt.256) call sfox_badcfg('JTDX_SFOX_POOLL',v(1:n),'a list size, 1 to 256')
       nsfpooll=i
    endif
    call get_environment_variable('JTDX_SFOX_POOLLOOKS',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_POOLLOOKS',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isint(v(1:n))) read(v(1:n),*,iostat=ios) i
       if(ios.ne.0 .or. i.lt.1 .or. i.gt.2) call sfox_badcfg('JTDX_SFOX_POOLLOOKS',v(1:n),'1 or 2')
       nsfpoollooks=i
    endif
    call get_environment_variable('JTDX_SFOX_POOLCRC',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_POOLCRC',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isint(v(1:n))) read(v(1:n),*,iostat=ios) i
       if(ios.ne.0 .or. i.lt.1 .or. i.gt.16) call sfox_badcfg('JTDX_SFOX_POOLCRC',v(1:n),'paths, 1 to 16')
       nsfpoolcrc=i
    endif
    call get_environment_variable('JTDX_SFOX_POOLFLOOR',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_POOLFLOOR',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isnum(v(1:n))) read(v(1:n),*,iostat=ios) x
       if(ios.ne.0 .or. x.lt.-99.0 .or. x.gt.30.0) call sfox_badcfg('JTDX_SFOX_POOLFLOOR',v(1:n),'dB, -99 to 30')
       sfpoolfloor=x
    endif
    call get_environment_variable('JTDX_SFOX_POOLAGE',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_POOLAGE',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isint(v(1:n))) read(v(1:n),*,iostat=ios) i
       if(ios.ne.0 .or. i.lt.0 .or. i.gt.1000) call sfox_badcfg('JTDX_SFOX_POOLAGE',v(1:n),'listened odd slots, 0 to 1000')
       nsfpoolage=i
    endif
    call get_environment_variable('JTDX_SFOX_SYNC3',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_SYNC3',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       if(v(1:n).eq.'1') then
          nsfsync3=1
       else if(v(1:n).ne.'0') then
          call sfox_badcfg('JTDX_SFOX_SYNC3',v(1:n),'0 or 1')
       endif
    endif
    call get_environment_variable('JTDX_SFOX_FLOOR',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_FLOOR',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isnum(v(1:n))) read(v(1:n),*,iostat=ios) x
       if(ios.ne.0 .or. x.lt.-99.0 .or. x.gt.30.0) call sfox_badcfg('JTDX_SFOX_FLOOR',v(1:n),'dB, -99 to 30')
       sfsearchfloor=x
    endif
    call get_environment_variable('JTDX_SFOX_LISTANY',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_LISTANY',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       if(v(1:n).eq.'0') then
          nsflistany=0
       else if(v(1:n).ne.'1') then
          call sfox_badcfg('JTDX_SFOX_LISTANY',v(1:n),'0 or 1')
       endif
    endif
    call get_environment_variable('JTDX_SFOX_LISTANYFLOOR',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_LISTANYFLOOR',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isnum(v(1:n))) read(v(1:n),*,iostat=ios) x
       if(ios.ne.0 .or. x.lt.-99.0 .or. x.gt.30.0) call sfox_badcfg('JTDX_SFOX_LISTANYFLOOR',v(1:n),'dB, -99 to 30')
       sflistanyfloor=x
    endif
    call get_environment_variable('JTDX_SFOX_LIST',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_LIST',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       if(v(1:n).eq.'0') then
          nsfliston=0
       else if(v(1:n).ne.'1') then
          call sfox_badcfg('JTDX_SFOX_LIST',v(1:n),'0 or 1')
       endif
    endif
    call get_environment_variable('JTDX_SFOX_LISTL',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_LISTL',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isint(v(1:n))) read(v(1:n),*,iostat=ios) i
       if(ios.ne.0 .or. i.lt.1 .or. i.gt.256) call sfox_badcfg('JTDX_SFOX_LISTL',v(1:n),'a list size, 1 to 256')
       nsflistl=i
    endif
    call get_environment_variable('JTDX_SFOX_LISTLOOKS',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_LISTLOOKS',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) nsflistlooks=sfox_looks('JTDX_SFOX_LISTLOOKS',v(1:n))
    call get_environment_variable('JTDX_SFOX_LISTANYLOOKS',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_LISTANYLOOKS',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) nsflistanylooks=sfox_looks('JTDX_SFOX_LISTANYLOOKS',v(1:n))
    call get_environment_variable('JTDX_SFOX_LISTCRC',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_LISTCRC',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isint(v(1:n))) read(v(1:n),*,iostat=ios) i
       if(ios.ne.0 .or. i.lt.1 .or. i.gt.16) call sfox_badcfg('JTDX_SFOX_LISTCRC',v(1:n),'paths, 1 to 16')
       nsflistcrc=i
    endif
    call get_environment_variable('JTDX_SFOX_LISTFLOOR',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_LISTFLOOR',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isnum(v(1:n))) read(v(1:n),*,iostat=ios) x
       if(ios.ne.0 .or. x.lt.-99.0 .or. x.gt.30.0) call sfox_badcfg('JTDX_SFOX_LISTFLOOR',v(1:n),'dB, -99 to 30')
       sflistfloor=x
    endif
    call get_environment_variable('JTDX_SFOX_NORMX',v,n,ios)
    if(ios.eq.-1) call sfox_badcfg('JTDX_SFOX_NORMX',v,'at most 32 characters')
    if(ios.eq.0 .and. n.gt.0) then
       ios=1; if(sfox_isnum(v(1:n))) read(v(1:n),*,iostat=ios) x
       if(ios.ne.0 .or. x.lt.1.1 .or. x.gt.1000.0) call sfox_badcfg('JTDX_SFOX_NORMX',v(1:n),'a factor, 1.1 to 1000')
       sfnormx=x
    endif

    return
  end subroutine sfox_config

  logical function sfox_isnum(t)

! a number and nothing but a number: list-directed input stops at a blank, a comma or a slash, and
! "-10,5" went in as -10 (review) - a decimal comma is a likely slip of the hand on this station

    character*(*) t

    sfox_isnum=len(t).gt.0 .and. verify(t,'+-.0123456789').eq.0 .and. scan(t,'0123456789').gt.0
    if(sfox_isnum) sfox_isnum=scan(t(2:),'+-').eq.0 .and. index(t,'.').eq.index(t,'.',.true.)

  end function sfox_isnum

  integer function sfox_looks(name,t)

! the list pass's looks: 1 the spectra, 2 the spectra and the likelihoods, L (returned as 3) the likelihoods alone

    character*(*) name,t
    integer i,ios

    if(t.eq.'L' .or. t.eq.'l') then
       sfox_looks=3; return
    endif
    ios=1; if(sfox_isint(t)) read(t,*,iostat=ios) i
    if(ios.ne.0 .or. i.lt.1 .or. i.gt.2) call sfox_badcfg(name,t,'1, 2 or L')
    sfox_looks=i

  end function sfox_looks

  logical function sfox_isint(t)

    character*(*) t

    sfox_isint=sfox_isnum(t) .and. index(t,'.').eq.0

  end function sfox_isint

  subroutine sfox_badcfg(name,val,want)

! a setting that cannot be read is not skipped: the decoder says so and stops

    character*(*) name,val,want

    write(0,'(5a)') 'SuperFox receiver: ',name,'="',val,'" cannot be read - it takes '//want//'. The decoder stops.'
    call flush(0)
    call exit(1)

  end subroutine sfox_badcfg

end module sfox_mod
