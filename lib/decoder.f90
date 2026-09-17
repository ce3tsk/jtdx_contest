subroutine multimode_decoder(params)

  !$ use omp_lib
  use prog_args
!  use timer_module, only: timer
  use jt65_decode
  use jt9_decode
  use jt9s_decode
  use jt10_decode
  use ft8_decode
  use ft4_decode
  use ft8_mod1, only : lft8deeposd,lcollectdelta,dd8orig,dd8delta,lsecondpass,nslicesft8,nmaxthreads,tsync8,tsync8s,tsync8l   ! CE3TSK
  use ft8_mod1, only : tsync8p,nsync8p   ! CE3TSK: sync8 wall time by decoding pass, TODO.md item 2.1
  use ft8_mod1, only : lsync8share,avexdt   ! CE3TSK item 76: the shared pass-1 sync surface
  use ft8_mod1, only : lallcall7   ! CE3TSK: JTDX_ALLCALL7_FILTER (chkflscall.f90), off unless a test asks
  use ft8_mod1, only : lft8buffered,nft8res,ldectr2   ! CE3TSK: the emission merge (FT8_EMISSION_ORDER.md)
  use thread_ladder, only : decoder_threads   ! CE3TSK item 63: the one thread ladder, both mode paths
  use ft8ensemble, only : NENSMAX,NENSBASE,ens_perturb,ensdtcorr,ensfreqcorr,ensswl,enscycles,ensalt   ! CE3TSK: ensemble members
  use ft8ensemble, only : ens_merge_tones
  use ft8ensemble, only : nbgrun,nbgunits,lbgabort,tdecstart,bgsyncscale,lretrymode,lbgtones,ens_residual,   &
                          ens_reset_period,bg_lock_gone,ntones,nfail,nretried,nretrystage,   &
                          nrxmembers   ! CE3TSK: pipeline ensemble, P8
  use ft8_mod1, only : evencopyk,oddcopyk,nhintdepth,NHINTMAX,NHINTDEFAULT   ! CE3TSK P11: deeper hint memory
  use ft8_mod1, only : calldteven,calldtodd,lastrxmsg   ! CE3TSK P11: emptied on a band or mode change
  use ft8_mod1, only : ndecodes,allmessages,allsnrs,allfreq,mycall12_0,mycall12_00,hiscall12_0,nmsg,odd,even,oddcopy,     &
                       evencopy,nlasttx,lqsomsgdcd,mycalllen1,msgroot,msgrootlen,lapmyc,lagcc,sumxdtt,avexdt,             &
                       nfawide,nfbwide,mycall,hiscall,lhound,mybcall,hisbcall,lenabledxcsearch,lwidedxcsearch,hisgrid4,   &
                       lmultinst,dd8,nft8cycles,nft8swlcycles,lskiptx1,ncandallthr,nincallthr,incall,msgincall,xdtincall, &
                       maskincallthr,ltxing
  use ft4_mod1, only : llagcc,nFT4decd,nfafilt,nfbfilt,lfilter,lhidetest,lhidetelemetry,dd4
  use ft4_mod1, only : nseen4   ! CE3TSK: the period's printed-message list
  use ft4_mod1, only : lft2,tperiod4,ft4rxcost,nft4rxrun   ! CE3TSK: FT2 rides the FT4 chain; the learned RX costs are per mode
  use ft4_mod1, only : dd4orig,dd4delta,lcollectdelta4,NFT4SLICEMAX,sumxdt4,ndecd4,ncand4, &   ! CE3TSK: the FT4 slice loop
                       t4sync_s,t4down_s,t4bp_s,t4osd_s,t4sub_s,t4cand_s,t4bits_s,n4sync_s,n4bp_s,n4osd_s
  use ft4_mod1, only : ft4hint_rotate,ft4hint_clear,nft4hintdepth,NHINT4MAX   ! CE3TSK: the FT4 hint memory
  use ft4_mod1, only : ft4qso_seed,nft4rxfsens,xdtvirt,nft4virtsrc   ! CE3TSK item 75: the QSO-side memory and the virtual candidate
  use ft4_mod1, only : nft4maxcand,NCAND4MAX,nft4cand   ! CE3TSK: the FT4 candidate cap and count
  use ft4_mod1, only : nft4res   ! CE3TSK: the per-slice decode buffers, emitted after the loop
  use ft4_mod1, only : t4sync,t4bits,t4bp,t4osd,t4sub,t4cand,t4down,n4sync,n4bp,n4osd   ! CE3TSK: FT4 timing
  use ft4_mod1, only : nft4osddeep,nft4syncqual,nft4nsp,nft4bpiter,nft4idfstp,ft4syncmin,nft4alt,nft4ens,ft4falsegate,ft4ensfreq   ! CE3TSK: FT4 hooks
  use ft4_mod1, only : nft4osdbase,nft4altbase,nft4syncqbase,ft4syncminbase   ! CE3TSK items 69/73: per-phase settings
  use ft4_mod1, only : nft4dt2   ! CE3TSK: item 57
  use ft4_mod1, only : nbg4run,nft4ensfrom,dd4prist   ! CE3TSK: the FT4 TX background (item 59)
  use ft4_mod1, only : ft4rxcost,nft4rxrun   ! CE3TSK item 80: FT4's RX budget auto
  use packjt77, only : lcommonft8b,ihash22,calls12,calls22

  include 'jt9com.f90'
!  include 'timer_common.inc'

  type, extends(jt65_decoder) :: counting_jt65_decoder
     integer :: decoded
  end type counting_jt65_decoder

  type, extends(jt9_decoder) :: counting_jt9_decoder
     integer :: decoded
  end type counting_jt9_decoder

  type, extends(jt9s_decoder) :: counting_jt9s_decoder
     integer :: decoded
  end type counting_jt9s_decoder
  
  type, extends(jt10_decoder) :: counting_jt10_decoder
     integer :: decoded
  end type counting_jt10_decoder
  
  type, extends(ft8_decoder) :: counting_ft8_decoder
     integer :: decoded
     real :: xdtt(200)
  end type counting_ft8_decoder

  type, extends(ft4_decoder) :: counting_ft4_decoder
     integer :: decoded
  end type counting_ft4_decoder

  logical first,firstsd
  logical(1) swlold,lhoundprev
  logical(1) :: lswlfilt
  logical :: lswlfiltforce=.false.   ! CE3TSK
  integer nutc,ndelay
  type(params_block) :: params
  character(len=300) :: dumpfile   ! CE3TSK debug dump
  integer :: ldump,idumpstat
  integer :: nslicing,islicing,nhalf   ! CE3TSK: second slicing pass
  integer :: nsl4,nslpass4,nf4w,nf4lo(24),nf4hi(24),nthr4,ncore4,nuse4,ihalf4,nsldiv4,nhalf4,k8
  real(8) :: sumdd4,sumdd8ck   ! CE3TSK: period checksums for the repeatability work
  integer :: kck   ! CE3TSK: the FT4 slice grid and its threads
  integer :: naltpass,ncyc0
  integer :: nmembers,imember,nswlcyc0   ! CE3TSK: ensemble members
  real, allocatable :: dd8prist(:)       ! CE3TSK: the pristine band the members start from
  integer :: nbgnext,nslicing0,naltpass0   ! CE3TSK: pipeline ensemble
  logical :: lrxbudget   ! CE3TSK P8: RX members by budget
  logical :: lhintdepthenv   ! CE3TSK: JTDX_FT4_HINT_DEPTH was given, so the mode must not overwrite it
  logical :: lft4rxbudget   ! CE3TSK item 80: the same for FT4, decided before the decode
  real(8) :: budget4
  integer :: m4,imc4
  real(8) :: rxdeadline,t1rx   ! CE3TSK P8
  real :: xk4   ! CE3TSK: FT4 experiment hook value
  logical :: lpipeline,laltdeferred
  real, allocatable :: dd8merged(:)        ! the band after the recipe's passes, for a deferred alternate pass
  real :: bgcost(0:7,0:NENSMAX)=0.         ! measured unit costs on this machine, for the budget (kinds 0-6; 7 = an FT4 background slicing, item 80)
  real, parameter :: bgresidual=0.8        ! the residual unit's factor on syncmin
  integer :: ios
  integer :: nfslo(nmaxthreads),nfshi(nmaxthreads),k
  integer :: nslicew,ngrid,nslpass,nsl,nb   ! CE3TSK: the anchored slice grid
  logical :: ldeep0
  logical(c_bool) :: nswl0
  logical(c_bool) :: llowth0,lsubp0   ! CE3TSK: the period's sensitivity, restored after the background recipe
  integer :: nrxf0,nbgm
  logical :: lmergecheck   ! CE3TSK: JTDX_MERGE_CHECK diagnostic switch
  data ndelay/0/
  data first/.true./
  data firstsd/.true./
  data swlold/.false./
  data lhoundprev/.false./
!  character(len=20) :: datetime
  character(len=6) :: hisgrid !, mygrid,
  save

  logical newdat65,newdat9,nagainjt9,nagainjt9s,nagainjt10,swlchanged,lowrms,fileExists

!character(10) dat, tim1, tim2, zon
!real(8) :: timer1,timer2 ! milliseconds
!integer(4) :: ival(8)

  type(counting_jt65_decoder) :: my_jt65
  type(counting_jt9_decoder) :: my_jt9
  type(counting_jt9s_decoder) :: my_jt9s
  type(counting_jt10_decoder) :: my_jt10
  type(counting_ft8_decoder) :: my_ft8
  type(counting_ft4_decoder) :: my_ft4
  
 !cast C character arrays to Fortran character strings
!  datetime=transfer(params%datetime, datetime)
  mycall=transfer(params%mycall,mycall)
  mybcall=transfer(params%mybcall,mybcall)
  hiscall=transfer(params%hiscall,hiscall)
  hisbcall=transfer(params%hisbcall,hisbcall)
!  mygrid=transfer(params%mygrid,mygrid)
  hisgrid=transfer(params%hisgrid,hisgrid)
  hisgrid4=hisgrid(1:4)

! CE3TSK: pipeline ensemble - a background call decodes the retained band again with more
! units (ft8_background below) and returns; everything it needs is the saved state of the
! period's decode
  nrxmembers=0   ! CE3TSK P8: the <rxm> tag is printed for every mode; only an FT8 decode sets it
  if(nbgrun.gt.0) then
     if(params%nmode.eq.8 .and. allocated(dd8prist)) then
        call ft8_background()
     else
        write(*,'(a)') '<BackgroundFinished><units>  0<msgs>   0<secs>  0.00<abort>0'; call flush(6)
     endif
     return
  endif
  if(nbg4run.gt.0) then   ! CE3TSK: the FT4 TX background invocation (item 59), FT8's block above mirrored
     if((params%nmode.eq.4 .or. params%nmode.eq.52) .and. allocated(dd4prist)) then
        call ft4_background()
     else
        write(*,'(a)') '<BackgroundFinished><units>  0<msgs>   0<secs>  0.00<abort>0'; call flush(6)
     endif
     return
  endif
  tdecstart=omp_get_wtime()   ! the deadline of a following background phase counts from here

  my_ft8%decoded=0; my_ft8%xdtt=0.
  my_jt65%decoded=0; my_jt9%decoded=0; my_jt9s%decoded=0; my_jt10%decoded=0; my_ft4%decoded=0
  nagainjt9=.false.;  nagainjt9s=.false.;  nagainjt10=.false.; ncandall=0; ncandallthr=0

  if(params%lmodechanged) then; avexdt=0.; if(params%nmode.eq.8) nintcount=3; endif ! avexdt fast track in FT8 after mode change
! CE3TSK: the FT4 RX cost model is learned wall time for a member count, and FT2 does about half the
! work in half the period - keeping FT4's learned costs would size FT2's ensemble from the wrong
! measurements (and the other way round), so a mode change forgets them.
  if(params%lmodechanged) then; ft4rxcost=0.; nft4rxrun=0; endif
  if(params%lbandchanged .and. (params%nmode.eq.8 .or. params%nmode.eq.4)) then; ihash22=-1; calls22=''; calls12=''; endif
! CE3TSK P11: a band or mode change also empties the hint decoder's message lists (the period's,
! the previous same-parity one and the deeper ones) and the call/DT lists - they are keyed by
! audio frequency and DT, meaningless under another dial, and with a four-period memory they
! would otherwise offer the old band's messages for two minutes
  if(params%lbandchanged .or. params%lmodechanged) then
     even%lstate=.false.; odd%lstate=.false.; evencopy%lstate=.false.; oddcopy%lstate=.false.
     evencopyk%lstate=.false.; oddcopyk%lstate=.false.
     calldteven%call2=''; calldtodd%call2=''; lastrxmsg(1)%lstate=.false.
     call ft4hint_clear()   ! CE3TSK: the FT4 hint lists as well
  endif

  if(.not.params%nagain) ndelay=params%ndelay
  lqsomsgdcd=.false.
  if(ndelay.gt.0) then ! received incomplete interval
    if(params%nmode.eq.8) then; call partintft8(ndelay,params%nutc); lqsomsgdcd=.true.
    else if(params%nmode.eq.4 .or. params%nmode.eq.52) then; call partintft4(ndelay,params%nutc)
    else; call partint(ndelay,params%nutc)
    endif
  endif
  ntrials=params%nranera
  if(params%nsecbandchanged.gt.0) then
  nsamplesdel=params%nsecbandchanged*12000
    if(params%nmode.eq.8) then
      if(params%nsecbandchanged.gt.14) then; dd8=0. ! protection
      else; dd8(1:nsamplesdel)=0.
      endif
    else if(params%nmode.eq.4) then
      if(params%nsecbandchanged.gt.6) then; dd4=0. ! protection: interval length is greater than dd4 index range
      else; dd4(1:nsamplesdel)=0.
      endif
    else if(params%nmode.eq.52) then   ! CE3TSK: 3.072 s of real audio, at two virtual samples each
      if(params%nsecbandchanged.gt.3) then; dd4=0.
      else; dd4(1:2*nsamplesdel)=0.
      endif
    endif
  endif

!  if (params%nagain .or. (params%nagainfil .and. (params%nmode.eq.65 .or. &
!      params%nmode.eq.(65+9)))) then
!     open(13,file=trim(temp_dir)//'/decoded.txt',status='unknown',position='append')
!  else
!     open(13,file=trim(temp_dir)//'/decoded.txt',status='unknown')
!  endif

  if(.not.params%nagain) nutc=params%nutc
!call date_and_time(date = dat, time = tim1, zone = zon)
!call date_and_time(values = ival)
!timer1 = dble(ival(8)) * 0.001_8 + &
!dble(ival(7)) + dble(ival(6)) * 60.0_8 + &
!dble(ival(5)) * 3600.0_8
!print *,'Decoder start: ',tim1
!print *,params%newdat,params%nagain,params%nagainfil
  swlchanged=.false.
  if(swlold.neqv.params%nswl) then
     swlchanged=.true.
     swlold=params%nswl
  endif
  if(first .or. swlchanged) then; call cwfilter(params%nswl,first,swlchanged); first=.false.; endif ! + ALLCALL to memory
! CE3TSK P11 hook: JTDX_HINT_DEPTH=1..8 - how many same-parity periods back the hint decoder's
! message lists reach (default NHINTDEFAULT = 4; 1 = the previous one only, JTDX's behaviour)
  call get_environment_variable('JTDX_HINT_DEPTH',dumpfile,ldump,idumpstat)
  nhintdepth=NHINTDEFAULT
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nhintdepth=max(1,min(NHINTMAX,k))
  endif
! CE3TSK: JTDX_FT4_HINT_DEPTH=0..8 - the same for the FT4 hint memory (ft4_mod1); 0 switches the
! FT4 hint pass off, i.e. JTDX's own FT4 behaviour; the default is 4 like FT8's
  lhintdepthenv=.false.
  call get_environment_variable('JTDX_FT4_HINT_DEPTH',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k
     if(ios.eq.0) then; nft4hintdepth=max(0,min(NHINT4MAX,k)); lhintdepthenv=.true.; endif
  endif
! CE3TSK: FT4 experiment hooks (ft4_mod1): JTDX_FT4_OSDDEEP (3), JTDX_FT4_SYNCQUAL (20),
! JTDX_FT4_NSP (4 since item 66; JTDX's 3), JTDX_FT4_BPITER (40), JTDX_FT4_IDFSTP (3), JTDX_FT4_SYNCMIN (1.2)
! CE3TSK item 69: fresh each decode - it was only ever raised (item 58), so deep OSD switched
! off in the GUI stayed on until a restart
  nft4osddeep=3
  call get_environment_variable('JTDX_FT4_OSDDEEP',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nft4osddeep=max(1,min(5,k))
  endif
  nft4osdbase=nft4osddeep   ! CE3TSK item 69: what the TX background starts from
  nft4syncqual=20; ft4syncmin=1.2   ! CE3TSK item 72: fresh each decode, as nft4osddeep - the sensitivity switch below raises neither past its base
  call get_environment_variable('JTDX_FT4_SYNCQUAL',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nft4syncqual=max(0,min(32,k))
  endif
  call get_environment_variable('JTDX_FT4_NSP',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nft4nsp=max(1,min(6,k))
  endif
  call get_environment_variable('JTDX_FT4_BPITER',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nft4bpiter=max(10,min(200,k))
  endif
  call get_environment_variable('JTDX_FT4_IDFSTP',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nft4idfstp=max(1,min(3,k))
  endif
  call get_environment_variable('JTDX_FT4_DT2',dumpfile,ldump,idumpstat)   ! CE3TSK: item 57, 0 = off
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nft4dt2=max(0,min(1,k))
  endif
  nft4alt=0; nft4ens=0   ! CE3TSK: fresh each decode - the env hooks below (file mode) or the GUI's block (FT4 branch)
  call get_environment_variable('JTDX_FT4_ALT',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nft4alt=max(0,min(1,k))
  endif
  nft4altbase=nft4alt   ! CE3TSK item 69
  if(.false.) then
  endif
  ! CE3TSK: how many ensemble members FT4 runs, as FT8 counts its own (FT8EnsembleEffort).
  ! JTDX_FT4_DITHER was the name while the dither was the only member; it still works.
  call get_environment_variable('JTDX_FT4_ENSEMBLE',dumpfile,ldump,idumpstat)
  if(idumpstat.ne.0 .or. ldump.le.0) call get_environment_variable('JTDX_FT4_DITHER',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nft4ens=max(0,min(6,k))   ! file mode; the menu switch means one member
  endif
  call get_environment_variable('JTDX_FT4_ENSFREQ',dumpfile,ldump,idumpstat)   ! CE3TSK: the members' shift, Hz
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) xk4; if(ios.eq.0) ft4ensfreq=max(0.0,min(6.0,xk4))
  endif
  call get_environment_variable('JTDX_FT4_FALSEGATE',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) xk4; if(ios.eq.0) ft4falsegate=max(-30.0,min(30.0,xk4))
  endif
! CE3TSK: JTDX_ALLCALL7_FILTER=1 - the ALLCALL7.TXT lookup, which chkflscall.f90 keeps off in
! the source. Read here, single-threaded, before any slice runs: the flag it sets is shared.
  lallcall7=.false.
  call get_environment_variable('JTDX_ALLCALL7_FILTER',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) lallcall7=(dumpfile(1:1).eq.'1')
  call get_environment_variable('JTDX_FT4_SYNCMIN',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) xk4; if(ios.eq.0) ft4syncmin=max(0.5,min(3.0,xk4))
  endif
  nft4syncqbase=nft4syncqual; ft4syncminbase=ft4syncmin   ! CE3TSK item 73: after every hook, before either phase's switch
! CE3TSK: JTDX_FT4_MAXCAND=1..1000 - the FT4 candidate cap (ft4_mod1; 100 = JTDX's behaviour)
  call get_environment_variable('JTDX_FT4_MAXCAND',dumpfile,ldump,idumpstat)
  if(idumpstat.eq.0 .and. ldump.gt.0) then
     read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nft4maxcand=max(1,min(NCAND4MAX,k))
  endif
! CE3TSK experiment hook: JTDX_SWLFILTER=0/1 forces the subtraction window (cwfilter: the SWL
! mode's 3400-tap cosine or the standard 4000-tap cosine-squared) for every unit of the decode
  call get_environment_variable('JTDX_SWLFILTER',dumpfile,ldump,idumpstat)
  lswlfiltforce=(idumpstat.eq.0 .and. ldump.gt.0)
  if(lswlfiltforce) then
     lswlfilt=(dumpfile(1:1).eq.'1')
     if(swlold.neqv.lswlfilt) then; call cwfilter(lswlfilt,.false.,.true.); swlold=lswlfilt; endif
  endif
  lenabledxcsearch=params%lenabledxcsearch; lwidedxcsearch=params%lwidedxcsearch

  lmultinst=params%lmultinst; lskiptx1=params%lskiptx1; lhidetest=params%lhidetest; lhidetelemetry=params%lhidetelemetry
  ltxing=params%ltxing
  if(params%nmode.eq.8) then
     mycalllen1=len_trim(mycall)+1
     msgroot=''; msgroot=trim(mycall)//' '//trim(hiscall)//' '; msgrootlen=len_trim(msgroot)
     lcommonft8b=params%lcommonft8b; lagcc=params%nagcc; lhound=params%lhound
     nft8cycles=params%nft8cycles; nft8swlcycles=params%nft8swlcycles; forcedt=0.
     lft8deeposd=params%lft8deeposd   ! CE3TSK
     if(params%nagcc .or. params%lforcesync) call agccft8(params%nfa,params%nfb,params%lforcesync,forcedt)
     if((hiscall.ne.hiscall12_0 .and. hiscall.ne.'            ') &
        .or. (mycall.ne.mycall12_0 .and. mycall.ne.'            ') .or. (lhound.neqv.lhoundprev)) then
        if(hiscall.ne.'            ') then
          call tone8(params%lmycallstd,params%lhiscallstd)
          hiscall12_0=hiscall; mycall12_0=mycall
        endif
        lhoundprev=lhound
     endif
     if(params%lmycallstd .and. mycall.ne.'            ' .and. mycall12_00.ne.mycall) then
       call tone8myc(); mycall12_00=mycall
     endif
     ndecodes=0; allmessages=""; allsnrs=0; allfreq=0. !init arrays for multithreading decoding
     numcores=omp_get_num_procs()
     nuserthr=params%nmt
! CE3TSK: JTDX_DUMP_PARAMS - the block as the decoder received it (dump_params below, called from
! the FT8 path here and from the FT4 path, so ft4bg.sh can check the FT4 fields' reach)
     call dump_params()

     numthreads=decoder_threads(nuserthr,numcores)   ! CE3TSK item 63: the ladder lives in thread_ladder.f90

!print *,nuserthr,numcores,numthreads
     call omp_set_dynamic(.false.)
     call omp_set_nested(.true.)

     nfa=params%nfa; nfb=params%nfb; nfqso=params%nfqso
! CE3TSK: the sync baseline (sync8: the 40th percentile of the per-bin sync peaks over the
! wide span nfawide-nfbwide) is taken over 0-5000 Hz whatever the decoded range. Over a busy
! 100-3100 Hz the percentile sits on signals, fewer candidates clear syncmin and decodes are
! lost (default 71 -> 61, exhaustive 85 -> 80 on the benchmark capture), while the full
! band's empty top keeps it on the noise. The candidate range stays nfa-nfb; the filter case
! below narrows only that.
     nfawide=0; nfbwide=max(5000,nfb)
     if(params%nfilter) then  ! 160Hz Filter bandwidth, 580Hz Filter bandwidth in Hound mode
        if(nfqso.lt.nfa .or. nfqso.gt.nfb) then
           write(*,32) nutc,'nfqso is out of bandwidth','d'; 32 format(i6.6,2x,a25,16x,a1); go to 800
        endif
        if(.not.params%lhound) then; nfa=max(nfa,nfqso-60); nfb=min(nfb,nfqso+60)
        else; nfa=max(nfa,nfqso-290); nfb=min(nfb,nfqso+290); endif
        numthreads=min(8,numthreads) ! to do: withdraw limitation when threads are in sync at main passes?
     endif
     if(params%nagainfil) then
        if(nfqso.lt.nfa .or. nfqso.gt.nfb) then
           write(*,64) nutc,'nfqso is out of bandwidth','d'; 64 format(i6.6,2x,a25,16x,a1); go to 800
        endif
        nfa=max(nfa,nfqso-25) ! 50Hz bandwidth for decode via double click
        nfb=min(nfb,nfqso+25)
        numthreads=min(4,numthreads) ! to do: withdraw limitation when threads are in sync at main passes?
     endif
     nsec=mod(nutc,100)
! CE3TSK: above one thread the emission is buffered per slice and merged in slice order by
! my_ft8%emit after each parallel loop; one thread keeps the inline path, byte for byte
! (FT8_EMISSION_ORDER.md, TODO.md 2.2). numthreads is final here - the filter and
! double-click paths above have already trimmed it.
     lft8buffered=numthreads.gt.1; nft8res=0
     nmsg=0
     if(nsec.ne.0 .and. nsec.ne.15 .and. nsec.ne.30 .and. nsec.ne.45) then ! reading simulated wav file
      odd%lstate=.false.; even%lstate=.false.; oddcopy%lstate=.false.; evencopy%lstate=.false.  
      oddcopyk%lstate=.false.; evencopyk%lstate=.false.
     endif
     if(firstsd) then; odd%lstate=.false.; even%lstate=.false.; firstsd=.false.; endif
     if(nsec.eq.0 .or. nsec.eq.30) then
       do k=NHINTMAX,3,-1; evencopyk(:,k)=evencopyk(:,k-1); enddo   ! CE3TSK experiment: the older lists slide back
       evencopyk(:,2)=evencopy
       evencopy%msg=even%msg; evencopy%freq=even%freq
       evencopy%dt=even%dt; evencopy%lstate=even%lstate
       even%lstate=.false.
     endif
     if(nsec.eq.15 .or. nsec.eq.45) then
       do k=NHINTMAX,3,-1; oddcopyk(:,k)=oddcopyk(:,k-1); enddo; oddcopyk(:,2)=oddcopy
       oddcopy%msg=odd%msg; oddcopy%freq=odd%freq
       oddcopy%dt=odd%dt; oddcopy%lstate=odd%lstate
       odd%lstate=.false.
     endif
     nlasttx=params%nlasttx; lapmyc=params%lapmyc; nFT8decd=0; sumxdt=0.0; if(params%nmode.eq.4) sumxdtt=0.0; nrxmembers=0   ! CE3TSK P8
! CE3TSK: with more than one thread the band is decoded in a fixed grid of 12 slices whatever
! the thread count - the grid the 12-thread decoder used - so results do not depend on the
! machine, and threads pick slices up dynamically. Narrower slices were measured to lose
! decodes (35 slices of 143 Hz: default 71 -> 62, exhaustive 85 -> 80) because subtraction of
! a strong neighbour stops helping across a slice edge; JTDX_SLICES=n overrides for tests.
! One thread keeps decoding the whole band in one piece.
     nslicesft8=1; nslicew=nint(5000.0/12)
     if(numthreads.gt.1) then
! the grid is anchored to the full band: cells of 5000/12 = 417 Hz from 0 Hz, and a decoded
! range gets the cells it intersects (100-3100 Hz: 8 slices with the full band's boundaries),
! so a narrower range decodes as the full band does inside it instead of with narrower slices
! (12 slices of 250 Hz over 100-3100 Hz: default 71 -> 64, exhaustive 85 -> 81 with the fixed
! baseline). The count covers both slicings (the offset pass shifts the boundaries by half a
! cell); a slicing uses the first nslpass of them.
        ngrid=12
        call get_environment_variable('JTDX_SLICES',dumpfile,ldump,idumpstat)
        if(idumpstat.eq.0 .and. ldump.gt.0) then
           read(dumpfile(1:ldump),*,iostat=ios) ngrid
           if(ios.ne.0) ngrid=12
        endif
        ngrid=max(1,min(24,ngrid)); nslicew=nint(5000.0/ngrid)
        nslicesft8=max(slice_count(0,nslicew),slice_count(1,nslicew))
     endif
     call fillhash(nslicesft8,.false.)
     if(params%nmode.eq.8) call ft8apset(params%lmycallstd,params%lhiscallstd,nslicesft8)
!do i=9595,9605; print *,i,dd8(i); enddo ! check wav files processing

!     call timer('decft8  ',0)
! CE3TSK: optional second slicing pass. With more than one thread the band is cut into one
! slice per thread and each slice decodes its own candidates strongest-first, so the
! subtraction order differs from a whole-band decode and reaches different signals; a second
! pass over the same data with the slice grid offset by half a slice, the duplicate arrays
! carried over so only new messages print, recovers the single-thread set deterministically
! at twice the threaded time (DECODER_BENCHMARK_PLAN.md step 8).
! CE3TSK: the pass list. Pass 1 is the recipe as set on the main slice grid. If lft8twopass,
! the same recipe runs again with the grid offset by half a slice. If lft8altpass, a final
! pass runs a different approach - 7 plain cycles with OSD order 2 (order 1 stays for the
! QSO-partner and own-call signals ft8b treats specially) - on the main grid: the
! SWL-cycle family works on averaged data with the wide DT window, the plain-cycle + OSD
! family reaches other marginal signals on the raw data, and their union was measured at
! 85-86 against 81 for either alone (DECODER_BENCHMARK_PLAN.md step 10). Every pass after
! the first runs on the band with all earlier passes' subtractions merged in; the duplicate
! arrays carry over so only new messages print.
nslicing=1; if(params%lft8twopass .and. numthreads.gt.1) nslicing=nslicing+1
naltpass=0; if(params%lft8altpass .and. numthreads.gt.1) then; nslicing=nslicing+1; naltpass=nslicing; endif
call get_environment_variable('JTDX_MERGE_CHECK',dumpfile,ldump,idumpstat); lmergecheck=(idumpstat.eq.0)
! CE3TSK: ensemble members (ENSEMBLE_DECODE_PLAN.md, ft8ensemble.f90). After the recipe's
! own passes the pristine band is decoded again through fixed, exactly reproducible
! perturbations - a delay of a few samples, a seeded dither, a sub-bin frequency shift -
! with the member's own recipe. The chain's threshold decisions fall differently on the
! perturbed band, so every member reaches signals the others missed (85 -> 92 -> 95 -> 98
! on the benchmark capture with three members). The duplicate arrays are shared, so only
! new messages print; nothing a member finds is stored as the previous period's signal
! (lsecondpass); a member starts from the pristine band, not the merged one, because a
! different subtraction history is the point. A single thread runs each member's first
! pass only. The work is in ft8_unit (below), one call per unit.
! CE3TSK: pipeline ensemble (PIPELINED_DECODE_PLAN.md). When a background phase may follow
! (params%nft8bgeffort /= 0) the pristine band, the merged band and every pass's deltas are
! kept, ft8b stores the tones of each decode and ft8_decode the candidates that failed in
! the LDPC/OSD stage; ft8_background then runs more units on them in the decoder's idle
! time - jt9a (or the file mode) calls multimode_decoder again with nbgrun set.
nmembers=max(0,min(NENSBASE,params%nft8ensemble))
! CE3TSK P8: "budget auto" (-2): members run while the next one's learned cost fits the RX
! budget - the reply deadline - so a quiet band gets the depth its spare time can hold and
! a crowded band stays at the base recipe (DECODE_RECIPE_PLAN.md section 10)
lrxbudget=(params%nft8ensemble.eq.-2)
lpipeline=(params%nft8bgeffort.ne.0)
call ens_reset_period()
if(allocated(dd8prist)) deallocate(dd8prist)
if(allocated(dd8merged)) deallocate(dd8merged)
if(allocated(dd8orig)) deallocate(dd8orig)
if(allocated(dd8delta)) deallocate(dd8delta)
if(nmembers.gt.0 .or. lrxbudget .or. lpipeline) then; allocate(dd8prist(size(dd8))); dd8prist=dd8; endif
! the master thread is one of the team, so after a pass its own copy of dd8 holds that
! slice's subtractions and copyin would hand them to every thread of the next pass -
! which slice that was is up to the runtime. Keep the running merged band in dd8orig.
if(numthreads.gt.1) then; allocate(dd8orig(size(dd8))); dd8orig=dd8; endif
if(nslicing.gt.1 .or. nmembers.gt.0 .or. lrxbudget .or. lpipeline) then
   allocate(dd8delta(size(dd8),nslicesft8)); dd8delta=0.
   lcollectdelta=(nslicing.gt.1 .or. (lpipeline .and. numthreads.gt.1))   ! a single thread has no dd8orig to diff against
endif
lbgtones=lpipeline
nswl0=params%nswl; ncyc0=nft8cycles; ldeep0=lft8deeposd; nswlcyc0=nft8swlcycles
llowth0=params%lft8lowth; lsubp0=params%lft8subpass; nrxf0=params%nft8rxfsens
nslicing0=nslicing; naltpass0=naltpass
call ft8_unit(0,0)
laltdeferred=.false.
if(lpipeline) then   ! the band after the recipe's passes: the deferred alternate pass and the retry unit work on it
   allocate(dd8merged(size(dd8)))
   if(numthreads.gt.1) then
      dd8merged=dd8orig
      do it=1,nslicesft8; dd8merged=dd8merged+dd8delta(:,it); enddo
      laltdeferred=.not.params%lft8altpass
   else
      dd8merged=dd8
   endif
endif
if(lrxbudget) then
   rxdeadline=tdecstart+dble(max(1,merge(params%nrxbudget,27,params%nrxbudget.gt.0)))/10.d0
   do imember=1,NENSBASE
      if(omp_get_wtime()+dble(bg_cost(1,imember)).gt.rxdeadline) then
         call cost_decay(1,imember)   ! decays until it fits again, as the background's gate does
         exit
      endif
      t1rx=omp_get_wtime()
      call ft8_unit(1,imember)
      call cost_learn(1,imember,real(omp_get_wtime()-t1rx))   ! learn the member's cost as the background does
! CE3TSK P8: seed the next member's unlearned estimate from this one's measured cost, scaled
! by the built-in ratio - on a quiet band the estimates converge within a period instead of
! waiting for the 10 % decay, and on a busy one they scale up just as fast
      if(imember.lt.NENSBASE) then
         if(bgcost(1,imember+1).le.0.) bgcost(1,imember+1)=  &
              bgcost(1,imember)*bg_cost0(1,imember+1)/max(0.01,bg_cost0(1,imember))
      endif
      nmembers=imember
   enddo
else
   do imember=1,nmembers
      call ft8_unit(1,imember)
   enddo
endif
nrxmembers=nmembers
nbgnext=nmembers+1   ! the first member a background phase would run
call ft8_restore()
if(.not.lpipeline) then
   if(allocated(dd8delta)) deallocate(dd8delta)
   if(allocated(dd8orig)) deallocate(dd8orig)
   if(allocated(dd8prist)) deallocate(dd8prist)
endif

    do i=1,nslicesft8
      do m=1,nincallthr(i)
      nindex=maskincallthr(i)+m
      incall(30:2:-1)=incall(30-1:1:-1); incall(1)%msg=msgincall(nindex); incall(1)%xdt=xdtincall(nindex)
      enddo
    enddo

    if(nsec.eq.0 .or. nsec.eq.30) even(nmsg+1:130)%lstate=.false.
    if(nsec.eq.15 .or. nsec.eq.45) odd(nmsg+1:130)%lstate=.false.

!do i=1,nmsg
!  if(nsec.eq.0 .or. nsec.eq.30) print *, even(i)%msg
!  if(nsec.eq.15 .or. nsec.eq.45) print *, odd(i)%msg
!enddo

    if(params%ndelay.eq.0) then
      nFT8decd=my_ft8%decoded; dtmed=0.
      if(params%lforcesync) then; nintcount=3 ! fast track after Sync
      else if(nintcount.gt.0) then; nintcount=nintcount-1
      endif
      if(params%lforcesync .and. nFT8decd.eq.0) then
        avexdt=forcedt
      else
        if(nFT8decd.gt.2) then
          do i=1,nFT8decd
            if(i.lt.nFT8decd-1) then
              if((my_ft8%xdtt(i).gt.my_ft8%xdtt(i+1) .and. my_ft8%xdtt(i).lt.my_ft8%xdtt(i+2)) &
                 .or. (my_ft8%xdtt(i).lt.my_ft8%xdtt(i+1) .and. my_ft8%xdtt(i).gt.my_ft8%xdtt(i+2))) then
                dtmed=my_ft8%xdtt(i)
              else if((my_ft8%xdtt(i+1).gt.my_ft8%xdtt(i) .and. my_ft8%xdtt(i+1).lt.my_ft8%xdtt(i+2)) &
                 .or. (my_ft8%xdtt(i+1).lt.my_ft8%xdtt(i) .and. my_ft8%xdtt(i+1).gt.my_ft8%xdtt(i+2))) then
                dtmed=my_ft8%xdtt(i+1)
              else if((my_ft8%xdtt(i+2).gt.my_ft8%xdtt(i) .and. my_ft8%xdtt(i+2).lt.my_ft8%xdtt(i+1)) &
                 .or. (my_ft8%xdtt(i+2).lt.my_ft8%xdtt(i) .and. my_ft8%xdtt(i+2).gt.my_ft8%xdtt(i+1))) then
                dtmed=my_ft8%xdtt(i+2)
              else
                dtmed=my_ft8%xdtt(i)
              endif
              sumxdt=sumxdt+dtmed
            else
              sumxdt=sumxdt+dtmed ! use last median value
            endif
          enddo
          if(nFT8decd.gt.5) then; avexdt=(avexdt+sumxdt/nFT8decd)/2
          else if(nFT8decd.eq.5) then; avexdt=(1.1*avexdt+0.9*sumxdt/nFT8decd)/2
          else if(nFT8decd.eq.4) then; avexdt=(1.25*avexdt+0.75*sumxdt/nFT8decd)/2
          else if(nFT8decd.eq.3) then; avexdt=(1.35*avexdt+0.65*sumxdt/nFT8decd)/2
          endif
        else if(nFT8decd.gt.0) then
          sumxdt=sum(my_ft8%xdtt(1:nFT8decd))
          if(nFT8decd.eq.2) then; avexdt=(1.5*avexdt+0.5*sumxdt/nFT8decd)/2
          else if(nFT8decd.eq.1) then; avexdt=(1.75*avexdt+0.25*sumxdt)/2
          endif
        endif
      endif
    endif
    if(nFT8decd.gt.10 .and. nintcount.eq.1) avexdt=sumxdt/nFT8decd ! fast track after Sync or mode change on crowded bands
    call fillhash(nslicesft8,.true.)
    ncandall=sum(ncandallthr(1:nslicesft8))
!     call timer('decft8  ',1)
    go to 800
  endif

  if(params%nmode.eq.4 .or. params%nmode.eq.52) then
! CE3TSK: FT2 (nmode 52) is decoded by this same chain. jt9a.f90 / jt9.f90 have already doubled
! every sample, so what follows sees an FT4 signal and needs no changes of its own; the two scalars
! below are the only thing that knows better. Frequencies are halved here, once, and doubled back in
! ft4emit; DT is converted in ft4b, where it is first expressed in seconds.
    lft2=(params%nmode.eq.52); tperiod4=merge(3.75,7.5,lft2)
! CE3TSK: the hint memory is counted in periods, and FT2's are half as long - four of them is 30 s of
! wall time against FT4's 60. Eight keeps the same reach in seconds. The env still wins where given.
    if(.not.lhintdepthenv) nft4hintdepth=merge(8,4,lft2)
    call dump_params()   ! CE3TSK: JTDX_DUMP_PARAMS, as the FT8 path does (ft4bg.sh checks the FT4 fields' reach)
    if(params%nagcc) call agccft4()
    nfa=params%nfa; nfb=params%nfb; nfqso=params%nfqso; lfilter=params%nfilter
    if(lft2) then; nfa=nfa/2; nfb=nfb/2; nfqso=nfqso/2; endif
    if(lfilter) then
      nfafilt=max(nfa,nfqso-95); nfbfilt=min(nfb,nfqso+95) ! 84 + 11Hz possible freq error  
      if(nfqso.lt.nfafilt .or. nfqso.gt.nfbfilt) then
        write(*,128) nutc,'nfqso is out of bandwidth','d'; 128 format(i6.6,2x,a25,16x,a1); go to 800
      endif
    endif
    llagcc=params%nagcc; nFT4decd=0; sumxdtt(1)=0.0; nft4cand=0; nseen4=0   ! CE3TSK: nothing printed yet this period
    call fillhash(1,.false.)
    call ft4hint_rotate(params%nutc)   ! CE3TSK: the finished period's decodes become hints for the same parity
    nft4rxfsens=max(0,min(3,params%nft4rxfsens))   ! CE3TSK item 75: the RX phase's level, then the DT to try at the QSO frequency
    call ft4qso_seed(mycall,hiscall,params%nlasttx,logical(params%nstophint))
    call get_environment_variable('JTDX_FT4_TIMING',dumpfile,ldump,idumpstat)   ! item 75: the trace of the seeded DT
    if(idumpstat.eq.0 .and. ldump.gt.0 .and. xdtvirt.gt.-90.) then
       write(0,'(a,f6.2,a,i1)') 'ft4 virtual candidate DT',xdtvirt,' source ',nft4virtsrc
    endif
    if(allocated(dd4prist)) deallocate(dd4prist)
    if(params%nft4bgeffort.ne.0) then   ! CE3TSK: a background will follow (item 59; item 78: the switch, as FT8's)
       allocate(dd4prist(size(dd4))); dd4prist=dd4
    endif
    ! CE3TSK: Decode -> FT4 decoding -> expert. max() so the menu switch cannot pull a larger
    ! member count set through JTDX_FT4_DITHER back down to one - the switch means "ensemble on",
    ! the variable says how many members (item 49); without it the trial would silently run N=1.
    if(params%lft4altpass) nft4alt=1
    if(params%lft4deeposd) nft4osddeep=max(nft4osddeep,4)   ! CE3TSK: expert deep OSD (item 58); JTDX_FT4_OSDDEEP can go deeper
    if(params%nft4sens.ge.1) then   ! CE3TSK item 72: decoder sensitivity 1 - the hooks can still go lower
       ft4syncmin=min(ft4syncmin,1.0); nft4syncqual=min(nft4syncqual,16)
    endif
    ! CE3TSK item 80: "budget auto" (-2, FT8's P8): the largest member count whose learned RX cost fits
    ! the RX budget (FT4RXBudget / -l, tenths; 1.3 s by default against the 1.36 s reply deadline),
    ! decided here from the previous periods' costs since FT4's members are not separable units;
    ! the first count that does not fit has its estimate decayed so it is tried again later
    lft4rxbudget=(params%nft4ensemble.eq.-2)
    if(lft4rxbudget) then
       budget4=dble(max(1,merge(params%nrxbudget,merge(5,13,lft2),params%nrxbudget.gt.0)))/10.d0
       m4=0
       do k=1,6
          if(dble(ft4_rxcost(k)).gt.budget4) then; call ft4_rxcost_decay(k); exit; endif
          m4=k
       enddo
       nft4ens=max(nft4ens,m4)
    else
       nft4ens=max(nft4ens,max(0,min(6,params%nft4ensemble)))   ! CE3TSK: Decode -> FT4 decoding -> expert -> ensemble;
                                                                ! JTDX_FT4_ENSEMBLE can only raise it, for experiments
    endif
!    call timer('decft4  ',0)
    ! CE3TSK: the FT4 slice loop, built like FT8's (decoder.f90 ~1027, FT4_THREADING_PLAN.md).
    ! The band is cut on the same anchored grid, one slice per thread, each starting from the
    ! pristine period in its own dd4 and handing back what it subtracted. One thread keeps the
    ! single call, so every pinned file-mode number stays reachable.
    ! CE3TSK: JTDX_FT4_TIMING also prints a checksum of the period as the slices receive it, so
    ! "the runs differ" can be traced to either the samples going in or the decoding on top of
    ! them, instead of being argued about.
    call get_environment_variable('JTDX_FT4_TIMING',dumpfile,ldump,idumpstat)
    if(idumpstat.eq.0 .and. ldump.gt.0) then
       sumdd4=0.d0
       do k=1,size(dd4); sumdd4=sumdd4+dble(dd4(k))*dble(k); enddo
       write(0,'(a,i6.6,a,es24.17)') 'ft4 input ',params%nutc,' ddsum ',sumdd4
    endif
    sumxdt4=0.d0; ndecd4=0; ncand4=0
    t4sync_s=0.d0; t4down_s=0.d0; t4bp_s=0.d0; t4osd_s=0.d0; t4sub_s=0.d0; t4cand_s=0.d0; t4bits_s=0.d0
    n4sync_s=0; n4bp_s=0; n4osd_s=0
    ! CE3TSK: FT4 works out its own thread count - the block that does it for FT8 sits inside
    ! the FT8 path, so numthreads is still 0 here (the first version of this loop silently never
    ! ran for that reason). Same ladder, one copy: thread_ladder.f90 (item 63).
    ncore4=omp_get_num_procs(); nuse4=params%nmt
    nthr4=decoder_threads(nuse4,ncore4)
    ! CE3TSK: the grid is anchored, exactly as FT8's is (decoder.f90 ~450): cells of 5000/12 =
    ! 417 Hz counted from 0 Hz, whatever the thread count, so a four-thread machine and a
    ! sixteen-thread machine decode the same band the same way. Sizing slices by the thread count
    ! instead - the first version here - made the result depend on the computer.
    ! CE3TSK: FT8's 417 Hz cell holds about eight of its 50 Hz signals; the same cell holds only
    ! 4.6 of FT4's ~90 Hz ones, and a slice that is narrow in signal widths is where subtraction
    ! stops reaching its neighbours. JTDX_FT4_SLICES=n sets the divisor for measuring that.
    nsldiv4=12
    call get_environment_variable('JTDX_FT4_SLICES',dumpfile,ldump,idumpstat)
    if(idumpstat.eq.0 .and. ldump.gt.0) then
       read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nsldiv4=max(1,min(24,k))
    endif
    nf4w=nint(5000.0/real(nsldiv4))
    nsl4=1
    ! CE3TSK: counted exactly as FT8 counts its own (slice_count above): the cells of the
    ! anchored grid that the decoded range intersects, not the range width divided by the cell.
    ! The first version divided (nfb-nfa) by nf4w and laid the boundaries out from nfa, which
    ! kept the machine-independence but lost the other half of FT8's design - with the edges
    ! measured from nfa they move whenever the operator changes the decode range, so a signal
    ! sits on a boundary at one bandwidth setting and mid-slice at another, and the last slice
    ! swallowed the remainder (699 Hz against 417 on 100-3300 Hz).
    if(nthr4.gt.1) nsl4=max(1,min(NFT4SLICEMAX,max(slice_count(0,nf4w),slice_count(1,nf4w))))
    if(allocated(dd4orig)) deallocate(dd4orig)
    if(allocated(dd4delta)) deallocate(dd4delta)
    lcollectdelta4=.false.
    if(nsl4.gt.1) then
       allocate(dd4orig(size(dd4))); dd4orig=dd4
       allocate(dd4delta(size(dd4),nsl4)); dd4delta=0.
       lcollectdelta4=.true.
       ! CE3TSK: the second slicing moves every boundary half a slice, so a signal that sat on an
       ! edge - decoded by neither neighbour, and never subtracted for them either - is inside a
       ! slice the second time. As in FT8 it is a choice, not a fixture: there it is the menu's
       ! "second slicing pass (2 or more threads, about 2x time)", off by default, and it costs
       ! about double here too. JTDX_FT4_TWOPASS=1 turns it on for measuring.
       nhalf4=0; if(params%lft4twopass) nhalf4=1
       call get_environment_variable('JTDX_FT4_TWOPASS',dumpfile,ldump,idumpstat)   ! for measuring
       if(idumpstat.eq.0 .and. ldump.gt.0) then
          read(dumpfile(1:ldump),*,iostat=ios) k; if(ios.eq.0) nhalf4=merge(1,0,k.gt.0)
       endif
       do ihalf4=0,nhalf4
          ! CE3TSK: the second slicing starts from the band the first one left, its subtractions
          ! merged in slice order - never in the order the threads happened to finish. FT8 builds
          ! its second pass the same way (decoder.f90 ~1068).
          if(ihalf4.eq.1) then
             do k=1,nsl4; dd4orig=dd4orig+dd4delta(:,k); enddo
             dd4delta=0.
          endif
          ! CE3TSK: the same construction as the FT8 loop below - absolute cell edges that fall
          ! inside the range, the first slice clipped to nfa and the last run out to nfb.
          ! the NFT4SLICEMAX clamp the two FT4 copies carried is a no-op: slice_count stops at
          ! 24 itself, which is NFT4SLICEMAX, and it never returns less than 1
          call slice_edges(ihalf4,nf4w,nslpass4,nf4lo,nf4hi)
          nft4res=0
          ! CE3TSK: dynamic, as FT8's loop is - the slices are far from equal in cost. The output
          ! does not depend on which thread takes which slice: that was tested with
          ! schedule(runtime) during the repeatability work, and static drifted exactly as much as
          ! dynamic until the real causes were fixed - the shared OSD enumeration state and the
          ! shared packjt77 tables (DECODER_IMPROVEMENTS item 52).
!$omp parallel do schedule(dynamic,1) num_threads(nthr4) default(shared) private(k)
          do k=1,nslpass4
             dd4=dd4orig
             call my_ft4%decode(ft4_decoded,params%nQSOProgress,nfqso,nf4lo(k),nf4hi(k), &
                  params%nft4depth,params%nstophint,params%nswl,k,nthr4)
          enddo
!$omp end parallel do
          ! CE3TSK: dedupe, print and remember - in slice order, so the period's answer is the
          ! same whatever order the threads finished in (FT4_THREADING_PLAN.md)
          call my_ft4%emit(nslpass4)
       enddo
    else
       nft4res=0
       call my_ft4%decode(ft4_decoded,params%nQSOProgress,nfqso,nfa,nfb,params%nft4depth, &
            params%nstophint,params%nswl,1,1)
       call my_ft4%emit(1)
    endif
    ! CE3TSK: merged in slice order, never in the order the threads finished - avexdt is read
    ! back by the next period, so a float sum in completion order would differ run to run
    do k=1,nsl4
       nFT4decd=nFT4decd+ndecd4(k); nft4cand=nft4cand+ncand4(k)
       sumxdtt(1)=sumxdtt(1)+real(sumxdt4(k))
       t4sync=t4sync+t4sync_s(k); t4down=t4down+t4down_s(k); t4bp=t4bp+t4bp_s(k)
       t4osd=t4osd+t4osd_s(k); t4sub=t4sub+t4sub_s(k); t4cand=t4cand+t4cand_s(k); t4bits=t4bits+t4bits_s(k)
       n4sync=n4sync+n4sync_s(k); n4bp=n4bp+n4bp_s(k); n4osd=n4osd+n4osd_s(k)
    enddo
!    call timer('decft4  ',1)
    if(params%ndelay.eq.0) then
      sumxdt=sumxdtt(1)
      if(nFT4decd.gt.5) then; avexdt=(avexdt+sumxdt/nFT4decd)/2
      else if(nFT4decd.eq.5) then; avexdt=(1.1*avexdt+0.9*sumxdt/nFT4decd)/2
      else if(nFT4decd.eq.4) then; avexdt=(1.25*avexdt+0.75*sumxdt/nFT4decd)/2
      else if(nFT4decd.eq.3) then; avexdt=(1.35*avexdt+0.65*sumxdt/nFT4decd)/2
      else if(nFT4decd.eq.2) then; avexdt=(1.5*avexdt+0.5*sumxdt/nFT4decd)/2
      endif
    endif
    ! CE3TSK: over every slice, as FT8 does (fillhash(nslicesft8,...) at the three sites below).
    ! save_hash_call files each new callsign under the slice index the loop handed down, so a
    ! literal 1 here stored a compound or non-standard call heard above slice 1 and then dropped
    ! it at the next period's nlast_calls=0: it never became resolvable, however often it was
    ! heard, and later messages from that station printed as <...>.
    call fillhash(nsl4,.true.)
    ncandall=nft4cand   ! CE3TSK: <ncand> in the period's closing line, as FT8 reports it
    ! CE3TSK item 80: learn this band's RX cost at the member count that ran (in every mode, so budget
    ! auto starts warm), remember the count for the background phase, and report it in <rxm> as FT8 does
    nft4rxrun=nft4ens; nrxmembers=nft4ens
    call ft4_rxcost_learn(nft4ens,real(omp_get_wtime()-tdecstart))
    call get_environment_variable('JTDX_FT4_TIMING',dumpfile,ldump,idumpstat)   ! CE3TSK: stage wall times to stderr
    if(idumpstat.eq.0 .and. ldump.gt.0) then
      write(0,'(a,i6.6,a,i4,7(a,f7.3),3(a,i8))') 'ft4 timing ',nutc,' cand ',nft4cand,' sync ',t4sync,' bits ',t4bits, &
        ' bp ',t4bp,' osd ',t4osd,' sub ',t4sub,' getcand ',t4cand,' down ',t4down,' nsync ',n4sync,' nbp ',n4bp,' nosd ',n4osd
      t4sync=0.d0; t4bits=0.d0; t4bp=0.d0; t4osd=0.d0; t4sub=0.d0; t4cand=0.d0; t4down=0.d0; n4sync=0; n4bp=0; n4osd=0
    endif
    go to 800
  endif

  lowrms=.false.
  call rms_augap(params%nutc,lowrms)
  if(lowrms) go to 800

!n1000=0  ! attempt to make T10 noise blanker, some degradation in decoding
!do i=2,623998
!!if(abs(dd(i)-dd(i-1)).gt.1000) then; n1000=n1000+1; print *,i; endif
!if((dd(i+1)-dd(i)).gt.1000.0) dd(i+1)=dd(i)+(dd(i)+dd(i-1))/2.0
!if((dd(i)-dd(i+1)).gt.1000.0) dd(i+1)=dd(i)-(dd(i)+dd(i-1))/2.0
!enddo

! signal input level diagnostics
!rrr=0.
!ddd=0.
!do i=1,624000
!if (abs(dd(i)).gt.rrr) rrr=abs(dd(i))
!enddo
!ddd=20*log10(rrr)
!print *,''
!print *,'max possible sample level is 32767'
!print *,'max RX sample level',int(rrr)
!print *,''
!print *,'max possible dynamic range is 90dB'
!print *,'RX signal dynamic range',nint(ddd)
!print *,''
! end of signal input level diagnostics

  newdat65=params%newdat
  newdat9=params%newdat
  if(params%nagain .and. .not.params%nagainfil) newdat9=.true.

  nshift=26000

  call process_dd(params%nagcc,params%nmode,params%ntxmode,params%nzhsym)

  if(.not.params%nagainfil .and. params%ntxmode.eq.9 .and. params%nagain) nagainjt9=.true.
  if(.not.params%nagainfil .and. params%nmode.eq.9 .and. params%nagain) nagainjt9s=.true.
  if(.not.params%nagainfil .and. params%nagain) nagainjt10=.true.
  if(params%nagainfil) nagainjt10=.false.

  if(params%nmode.eq.10) then
     call my_jt10%decode(jt10_decoded,nutc,params%nfqso,newdat9,params%npts8,   &
          params%nfa,params%nfb,params%ntol,params%nzhsym,nagainjt10,params%nagainfil,params%ntrials10, &
          params%ntrialsrxf10,params%nfilter,params%nswl,params%nagcc,params%nhint,params%nstophint, &
          params%nlasttx,mycall,hiscall,hisgrid)
     go to 800
  endif

  if(params%nmode.eq.9) then
     call my_jt9s%decode(jt9s_decoded,nutc,params%nfqso,newdat9,params%npts8,   &
          params%nfa,params%nfb,params%nzhsym,params%nfilter,params%nswl,   &
          nagainjt9s,params%nagainfil,params%ndepth,params%nhint,params%nstophint, &
          params%nlasttx,mycall,hiscall,hisgrid)
     go to 800
  endif

  call omp_set_dynamic(.true.)
!!!  !$omp parallel sections num_threads(2) copyin(/timer_private/) shared(ndecoded) if(.true.) !iif() needed on Mac
  !$omp parallel sections num_threads(2) shared(ndecoded) if(.true.) !iif() needed on Mac
  !$omp section
  if(params%nmode.eq.65 .or. (params%nmode.eq.(65+9) .and. params%ntxmode.eq.65)) then
     ! We're in JT65 mode, or should do JT65 first

     nf1=params%nfa
     nf2=params%nfb

!     call timer('jt65a   ',0)
     call my_jt65%decode(jt65_decoded,nutc,nf1,nf2,params%nfqso,  &
          logical(params%nagainfil),ntrials,params%naggressive,params%nhint,mycall, &
          hiscall,hisgrid,params%nprepass,params%nswl,params%nfilter,params%nstophint, &
          params%nlasttx,params%nsdecatt,params%fmaskact,params%ntxmode,params%ntopfreq65, &
          params%nharmonicsdepth,params%showharmonics)
!     call timer('jt65a   ',1)
  else if(params%nmode.eq.(65+9) .and. params%ntxmode.eq.9) then
     ! We're in JT9 mode, or should do JT9 first
!     call timer('decjt9  ',0)
     call my_jt9%decode(jt9_decoded,nutc,params%nfqso,newdat9,params%npts8,   &
          params%nfa,params%nfsplit,params%nfb,params%ntol,params%nzhsym,                   &
          nagainjt9,params%nagainfil,params%ndepth,params%nmode,params%nhint,params%nstophint, &
          params%nlasttx,mycall,hiscall,hisgrid,params%ntxmode)
!     call timer('decjt9  ',1)
  endif

  !$omp section
  if(params%nmode.eq.(65+9)) then          !Do the other mode (we're in dual mode)
     if (params%ntxmode.eq.9) then
        if(.not.nagainjt9) then
           nf1=params%nfa
           nf2=params%nfb
!           call timer('jt65a   ',0)
           call my_jt65%decode(jt65_decoded,nutc,nf1,nf2,params%nfqso, &
           logical(params%nagainfil),ntrials,params%naggressive,params%nhint,mycall,     &
           hiscall,hisgrid,params%nprepass,params%nswl,params%nfilter,params%nstophint, &
           params%nlasttx,params%nsdecatt,params%fmaskact,params%ntxmode,params%ntopfreq65, &
           params%nharmonicsdepth,params%showharmonics)
!           call timer('jt65a   ',1)
        endif
     else
        if (params%ntxmode.eq.65 .and. params%nagain) go to 2
!        call timer('decjt9  ',0)
        call my_jt9%decode(jt9_decoded,nutc,params%nfqso,newdat9,params%npts8,&
             params%nfa,params%nfsplit,params%nfb,params%ntol,params%nzhsym,                &
             nagainjt9,params%nagainfil,params%ndepth,params%nmode,params%nhint,params%nstophint, &
             params%nlasttx,mycall,hiscall,hisgrid,params%ntxmode)
!        call timer('decjt9  ',1)
2       continue
     end if
  endif

  !$omp end parallel sections
!800 call date_and_time(date = dat, time = tim2, zone = zon)
!call date_and_time(values = ival)
!timer2 = dble(ival(8)) * 0.001_8 + &
!dble(ival(7)) + dble(ival(6)) * 60.0_8 + &
!dble(ival(5)) * 3600.0_8
!print *,'Decoding finished: ',tim2
!write (*,'(1x,a15,f6.3,a8)')'Decoding time: ',timer2-timer1, ' seconds'

800 continue

write(*,1010) avexdt,ncandall,nrxmembers,omp_get_wtime()-tdecstart   ! CE3TSK: <rxs> the RX phase's seconds, for file-mode measurements
call get_environment_variable('JTDX_MEMO_STATS',dumpfile,ldump,idumpstat)   ! CE3TSK timing
if(idumpstat.eq.0) write(0,'(a,3f8.3)') 'sync8 wall total/spectra/lags:',tsync8,tsync8s,tsync8l
if(idumpstat.eq.0) write(0,'(a,9(i3,a,f7.3))') 'sync8 by pass:',(k8,':',tsync8p(k8),k8=1,9)
if(idumpstat.eq.0) write(0,'(a,9(i3,a,i4))') 'sync8 calls by pass:',(k8,':',nsync8p(k8),k8=1,9)
tsync8p=0.d0; nsync8p=0
tsync8=0.d0; tsync8s=0.d0; tsync8l=0.d0
1010 format('<DecodeFinished><avexdt>',f6.2,'<ncand>',i5,'<rxm>',i3,'<rxs>',f7.2)
  call flush(6)
  if(params%lforcesync .and. nFT8decd.eq.0) avexdt=0. ! reset value to let correct sliding in decoder
!  close(13)

  do i=1,1000
    inquire(file=trim(temp_dir)//'/.lock',exist=fileExists)
    if(fileExists) return ! ready to exit from decoder, Decode button hung up issue
    call sleep_msec(1)
  enddo

  return

contains

  subroutine jt65_decoded (this, utc, snr, dt, freq, decoded, servis)
    use jt65_decode
    implicit none

    class(jt65_decoder), intent(inout) :: this
    integer, intent(in) :: utc
    integer, intent(in) :: snr
    real, intent(in) :: dt
    integer, intent(in) :: freq
    character(len=26), intent(in) :: decoded
    character(len=1), intent(in) :: servis

    real dtshift
    dtshift=(real(nshift))/12000.0
    !$omp critical(decode_results)
    write(*,1010) utc,snr,dt-dtshift,freq,decoded,servis
1010 format(i4.4,i4,f5.1,i5,1x,'#',1x,a26,a1)
!    write(13,1012) utc,snr,dt-dtshift,float(freq),decoded,
!1012 format(i4.4,i5,f6.1,f8.0,3x,a26,' JT65')
    call flush(6)

    !$omp end critical(decode_results)
    select type(this)
    type is (counting_jt65_decoder)
       this%decoded = this%decoded + 1
    end select
 end subroutine jt65_decoded

  subroutine jt9_decoded (this, utc, snr, dt, freq, decoded, servis9)
    use jt9_decode
    implicit none

    class(jt9_decoder), intent(inout) :: this
    integer, intent(in) :: utc
    integer, intent(in) :: snr
    real, intent(in) :: dt
    real, intent(in) :: freq
    character(len=26), intent(in) :: decoded
    character(len=1), intent(in) :: servis9

    !$omp critical(decode_results)
    write(*,1000) utc,snr,dt,nint(freq),decoded,servis9
1000 format(i4.4,i4,f5.1,i5,1x,'@',1x,a26,a1)
!    write(13,1002) utc,snr,dt,freq,decoded
!1002 format(i4.4,i5,f6.1,f8.0,3x,a26,' JT9')
    call flush(6)
    !$omp end critical(decode_results)
    select type(this)
    type is (counting_jt9_decoder)
       this%decoded = this%decoded + 1
    end select
  end subroutine jt9_decoded

  subroutine jt9s_decoded (this, utc, snr, dt, freq, decoded, servis9)
    use jt9s_decode
    implicit none

    class(jt9s_decoder), intent(inout) :: this
    integer, intent(in) :: utc
    integer, intent(in) :: snr
    real, intent(in) :: dt
    real, intent(in) :: freq
    character(len=26), intent(in) :: decoded
    character(len=1), intent(in) :: servis9

    write(*,1000) utc,snr,dt,nint(freq),decoded,servis9
1000 format(i4.4,i4,f5.1,i5,1x,'@',1x,a26,a1)
!    write(13,1002) utc,snr,dt,freq,decoded
!1002 format(i4.4,i5,f6.1,f8.0,3x,a26,' JT9')
    call flush(6)
    select type(this)
    type is (counting_jt9s_decoder)
       this%decoded = this%decoded + 1
    end select
  end subroutine jt9s_decoded

  subroutine jt10_decoded (this, utc, snr, dt, freq, decoded, servis9)
    use jt10_decode
    implicit none

    class(jt10_decoder), intent(inout) :: this
    integer, intent(in) :: utc
    integer, intent(in) :: snr
    real, intent(in) :: dt
    real, intent(in) :: freq
    character(len=26), intent(in) :: decoded
    character(len=1), intent(in) :: servis9

    write(*,1000) utc,snr,dt,nint(freq),decoded,servis9
1000 format(i4.4,i4,f5.1,i5,1x,'+',1x,a26,a1)
!    write(13,1002) utc,snr,dt,freq,decoded
!1002 format(i4.4,i5,f6.1,f8.0,3x,a26,' T10')
    call flush(6)
    select type(this)
    type is (counting_jt10_decoder)
       this%decoded = this%decoded + 1
    end select
  end subroutine jt10_decoded

  subroutine ft8_unit(kind,im)
! CE3TSK: one decoding unit over the band. kind 0: the recipe's own pass list (pass 1 on the
! main grid, the half-slice-offset pass, the alternate-approach pass as configured);
! 1: ensemble member im (ft8ensemble table) on the perturbed pristine band; 2: the deferred
! alternate-approach pass on the merged band (pipeline ensemble); 3: the residual pass -
! every known decode subtracted from the pristine band, SWL-5 with a lowered sync
! threshold; 4: the retry unit - the candidates that failed in the LDPC/OSD stage decoded
! again on dithered data; 6: the classic unit (P9) - the plain non-SWL recipe with
! nft8bgclassic cycles and the background's threshold setting on the pristine band, the
! pass sequence no SWL unit runs (DECODE_RECIPE_PLAN.md section 11).
! Every pass after the first of a unit runs on the band with the
! unit's earlier subtractions merged in; the duplicate arrays are shared throughout.
    integer, intent(in) :: kind,im
    integer :: jzb8,jzt8   ! CE3TSK item 76
    real :: syncmin8   ! CE3TSK: pass 1's sync threshold for the shared surface (see below)
    logical(c_bool) :: lsubp6   ! CE3TSK: the classic unit's saved sub-pass setting, restored below
    if(kind.eq.0) then
       nslicing=nslicing0; naltpass=naltpass0; lsecondpass=.false.
    else if(kind.eq.1) then
       nslicing=1; naltpass=0
       if(ensalt(im) .and. numthreads.gt.1) then; nslicing=2; naltpass=2; endif
       ! the member's recipe is fixed (the table was measured with it); the user's deep-OSD
       ! switch stays with the base decode, it would triple a member's time
       params%nswl=ensswl(im); nft8swlcycles=enscycles(im); nft8cycles=ncyc0; lft8deeposd=.false.
       if(numthreads.gt.1) then; call ens_perturb(im,dd8prist,dd8orig,size(dd8))
       else; call ens_perturb(im,dd8prist,dd8,size(dd8)); endif
       dd8delta=0.; lcollectdelta=(nslicing.gt.1); lsecondpass=.true.
    else if(kind.eq.2) then
       nslicing=1; naltpass=1
       params%nswl=nswl0; nft8cycles=ncyc0; lft8deeposd=ldeep0
       dd8orig=dd8merged; dd8delta=0.; lcollectdelta=.false.; lsecondpass=.true.
       ensdtcorr=0.; ensfreqcorr=0.
    else if(kind.eq.3) then
       nslicing=1; naltpass=0
       params%nswl=.true.; nft8swlcycles=5; nft8cycles=ncyc0; lft8deeposd=.false.
       if(numthreads.gt.1) then; call ens_residual(dd8prist,dd8orig,size(dd8))
       else; call ens_residual(dd8prist,dd8,size(dd8)); endif
       dd8delta=0.; lcollectdelta=.false.; lsecondpass=.true.; bgsyncscale=bgresidual
       ensdtcorr=0.; ensfreqcorr=0.
    else if(kind.eq.5) then
       ! the TX background recipe (its own SWL mode, cycles, sensitivities, OSD, passes) as a
       ! fresh decode of the pristine band, the duplicates carried; from here on the background
       ! keeps the background sensitivities until ft8_restore
       nslicing=1; naltpass=0
       if(params%lbgtwopass .and. numthreads.gt.1) nslicing=nslicing+1
       if(params%lbgaltpass .and. numthreads.gt.1) then; nslicing=nslicing+1; naltpass=nslicing; endif
       params%nswl=params%lbgswl; nft8cycles=params%nft8bgcycles; nft8swlcycles=params%nft8bgswlcycles
       lft8deeposd=params%lbgdeeposd
       params%lft8lowth=params%lbglowth; params%lft8subpass=params%lbgsubpass
       params%nft8rxfsens=params%nft8bgrxfsens
       if(numthreads.gt.1) then; dd8orig=dd8prist; else; dd8=dd8prist; endif
       dd8delta=0.; lcollectdelta=(nslicing.gt.1); lsecondpass=.true.
       ensdtcorr=0.; ensfreqcorr=0.
    else if(kind.eq.6) then
       nslicing=1; naltpass=0
       params%nswl=.false.; nft8cycles=max(3,min(9,params%nft8bgclassic)); nft8swlcycles=nswlcyc0; lft8deeposd=.false.
       ! CE3TSK: the classic recipe runs without the sub-pass, but only for this unit. The list is
       ! recipe, classic, members, residual, retry and nothing between here and ft8_restore sets
       ! the field again, so assigning it flat left the members, the residual and the retry
       ! decoding with a weaker recipe than configured whenever the classic unit was switched on.
       lsubp6=params%lft8subpass
       params%lft8lowth=params%lbglowth; params%lft8subpass=.false.
       if(numthreads.gt.1) then; dd8orig=dd8prist; else; dd8=dd8prist; endif
       dd8delta=0.; lcollectdelta=.false.; lsecondpass=.true.
       ensdtcorr=0.; ensfreqcorr=0.
    else if(kind.eq.4) then
       nslicing=1; naltpass=0
       params%nswl=.true.; nft8swlcycles=5; nft8cycles=ncyc0; lft8deeposd=.false.
       ! on the merged band: a candidate that failed in the LDPC/OSD stage was found after
       ! its stronger neighbours had been subtracted; on the pristine band the sync gates
       ! reject it before any retry
       if(numthreads.gt.1) then; dd8orig=dd8merged; else; dd8=dd8merged; endif
       dd8delta=0.; lcollectdelta=.false.; lsecondpass=.true.; lretrymode=.true.
       ensdtcorr=0.; ensfreqcorr=0.
    endif
    call unit_filter()
    do islicing=1,nslicing
    nhalf=0; if(islicing.eq.2 .and. ((params%lft8twopass .and. kind.eq.0) .or. (params%lbgtwopass .and. kind.eq.5))) nhalf=1
    if(islicing.gt.1) then   ! the band with every earlier pass's subtractions, merged in thread order
       ! CE3TSK diagnostic, JTDX_MERGE_CHECK: the per-slice deltas are printed BEFORE the merge
       ! below clears them. Printed after, every slice line was structurally zero while the
       ! merged-band line under it was real, so the probe that exists to prove the merge happens
       ! in slice order could show nothing, and its zeros read as a merge failure.
       if(lmergecheck) then
          do it=1,nslicesft8
             write(0,'(a,i3,2es16.8)') 'delta slice',it,sum(abs(dd8delta(:,it))),sum(dd8delta(1:90000,it))
          enddo
       endif
       do it=1,nslicesft8; dd8orig=dd8orig+dd8delta(:,it); enddo
       dd8delta=0.; dd8=dd8orig
       if(lmergecheck) write(0,'(a,2es16.8)') 'merged band',sum(abs(dd8)),sum(dd8(1:90000))
       lcollectdelta=(islicing.lt.nslicing .or. (lpipeline .and. kind.eq.0)); lsecondpass=.true.
    endif
    if(islicing.eq.naltpass) then
       params%nswl=.false.; nft8cycles=7; lft8deeposd=.true.
    endif
    if(numthreads.eq.1) then
         call my_ft8%decode(ft8_decoded,params%nQSOProgress,nfqso,params%nft8rxfsens,  &
              params%nftx,nutc,nfa,nfb,params%ncandthin,params%ndtcenter, &
              nsec,params%napwid,params%nswl,params%lmycallstd,params%lhiscallstd, &
              params%nfilter,params%nstophint,1,numthreads,logical(params%nagainfil),params%lft8lowth, &
              params%lft8subpass,params%lhideft8dupes,params%lhidehash)
    endif
    if(numthreads.gt.1) then
! CE3TSK: one parallel loop over the slice grid replaces the former one-section-per-thread
! blocks. Every slice starts from the pristine (or merged) band in the thread's private dd8
! and records its own delta, so the merge order is by slice index and the result does not
! depend on which thread ran which slice. Slice k uses index k for all per-slice state
! (candidate counts, incoming calls, stored CQ/MyCall signals, hash lists, OSD scratch).
! the boundaries as the former per-thread blocks computed them: nfdelta = nint(width/n),
! boundary k at nfa + k*nfdelta (+ half a slice on the offset pass), the last one clamped
       call slice_edges(nhalf,nslicew,nslpass,nfslo,nfshi)
       nft8res(1:nslpass)=0
       ! CE3TSK diagnostic: JTDX_DEC_TRACE=2 also checksums the band handed to the slices
       call get_environment_variable('JTDX_DEC_TRACE',dumpfile,ldump,idumpstat)
       ldectr2=.false.
       if(idumpstat.eq.0 .and. ldump.gt.0) then
          ldectr2=dumpfile(1:1).eq.'2'
          if(dumpfile(1:1).eq.'2') then
             sumdd8ck=0.d0
             do kck=1,size(dd8orig); sumdd8ck=sumdd8ck+dble(dd8orig(kck))*dble(kck); enddo
             write(0,'(a,i2,a,i2,a,i2,a,es24.17)') 'DD8ORIG k',kind,' m',im,' s',islicing,' ',sumdd8ck
          endif
       endif
       ! CE3TSK item 76 (TODO.md 2.1): every slice starts pass 1 from dd8orig, so sync8's wide-band
       ! surface is computed once here - its loops in parallel - and read by all of them; the retry
       ! unit has no sync8 pass. The DT window is ft8_decode's (its lines 205-207), exactly.
       if(.not.lretrymode) then
          jzb8=-62 + avexdt*25.; jzt8=62 + avexdt*25.
          if(params%nswl) then; jzb8=-86 + avexdt*25.; jzt8=86 + avexdt*25.; endif
          ! CE3TSK: pass 1's threshold, ft8_decode's own rule (its lines 219-227) - the shared
          ! surface is pass 1 only. This argument used to be a bare `syncmin`, a name that
          ! appears nowhere else in this file: with no implicit none in the host scope it was
          ! an implicitly typed real in static storage, read before it was ever written. Today
          ! sync8_wide uses it only in the candidate trace, so the trace printed a zero
          ! threshold for pass 1 while the per-slice calls printed the real one; the moment the
          ! shared surface gates on it, every multi-threaded FT8 decode would have changed.
          syncmin8=1.5
          if(params%lft8lowth .or. params%nswl) syncmin8=1.225
          call sync8_share(dd8orig,jzb8,jzt8,params%nswl,syncmin8*bgsyncscale)
       endif
!$omp parallel do schedule(dynamic,1) num_threads(numthreads) default(shared) private(k)
       do k=1,nslpass
          dd8=dd8orig
          call my_ft8%decode(ft8_decoded,params%nQSOProgress,nfqso,params%nft8rxfsens,  &
               params%nftx,nutc,nfslo(k),nfshi(k),params%ncandthin,params%ndtcenter, &
               nsec,params%napwid,params%nswl,params%lmycallstd,params%lhiscallstd, &
               params%nfilter,params%nstophint,k,numthreads,logical(params%nagainfil),params%lft8lowth, &
               params%lft8subpass,params%lhideft8dupes,params%lhidehash)
       enddo
!$omp end parallel do
       lsync8share=.false.   ! item 76
! CE3TSK: dedupe, print and remember - in slice order, best SNR winning of two same-text
! copies - so the pass's answer is the same whatever order the threads finished in. Runs
! before the abort check: what a stopping background unit had already decoded still prints.
       call my_ft8%emit(nslpass,nsec,params%lhideft8dupes)
       call ens_merge_tones(nslpass)   ! CE3TSK: the stored tones too, in slice order
    endif
    if(lbgabort) exit
    call fillhash(nslicesft8,.true.)   ! CE3TSK: make this pass's callsigns resolvable in the next one
    enddo   ! CE3TSK: the unit's passes
    bgsyncscale=1.0; lretrymode=.false.
    if(kind.eq.6) params%lft8subpass=lsubp6   ! CE3TSK: per-unit state, as the two resets above
  end subroutine ft8_unit

  subroutine unit_filter()
! CE3TSK: the subtraction window follows the unit's SWL flag. cwfilter used to be set once per
! decode from the period's flag, so after a plain-cycles RX phase every SWL member and the
! background recipe subtracted with the standard window and the pipeline ended 9 messages
! short (91 against 100 after an SWL-5 RX phase, 100-3100 Hz).
    if(lswlfiltforce) return
    if(swlold.neqv.params%nswl) then; call cwfilter(params%nswl,.false.,.true.); swlold=params%nswl; endif
  end subroutine unit_filter

  subroutine ft8_restore()   ! CE3TSK: the recipe and the pass state as the period's decode found them
    params%nswl=nswl0; call unit_filter(); nft8cycles=ncyc0; lft8deeposd=ldeep0; nft8swlcycles=nswlcyc0
    params%lft8lowth=llowth0; params%lft8subpass=lsubp0; params%nft8rxfsens=nrxf0
    lcollectdelta=.false.; lsecondpass=.false.; ensdtcorr=0.; ensfreqcorr=0.; bgsyncscale=1.0; lretrymode=.false.
  end subroutine ft8_restore

  real function bg_cost0(kind,im)   ! CE3TSK: a unit's built-in cost estimate (12-thread benchmark figures, thread-scaled)
    integer, intent(in) :: kind,im
    real :: c
    select case(kind)   ! 12-thread figures from ENSEMBLE_DECODE_PLAN.md, scaled by the thread count
    case(1); c=1.3; if(ensalt(im)) c=2.8
    case(5); c=2.8
    case(2); c=1.5
    case(3); c=1.6
    case(4); c=3.0
    case(6); c=1.0   ! P9 classic unit, 6 cycles: 1.0 s on the benchmark, 0.3-0.5 s on air
    case(7)   ! item 80: an FT4 background slicing with six members and the extras (the recommended recipe, 12 threads); im 2 = the residual.
              ! FT4's own thread count: numthreads is FT8's and stays 0 on the FT4 path (a 12 s estimate skipped every slicing)
       c=1.0; if(im.ge.2) c=0.4
       bg_cost0=c*12.0/real(max(1,min(12,decoder_threads(params%nmt,omp_get_num_procs())))); return
    case default; c=1.0
    end select
    bg_cost0=c*12.0/real(max(1,min(12,numthreads)))
  end function bg_cost0

  ! CE3TSK item 80: FT4's RX-phase cost by member count - the built-in estimate (the crowded file's
  ! 12-thread figures, 0.30 s + 0.12 s a member, thread-scaled), the learned one, and FT8's two
  ! rules on it; a measured count seeds every unlearned count by the built-in ratio
  real function ft4_rxcost0(m)
    integer, intent(in) :: m
    integer :: nt
    nt=decoder_threads(params%nmt,omp_get_num_procs())
    ft4_rxcost0=(0.30+0.12*real(max(0,min(6,m))))*12.0/real(max(1,min(12,nt)))
  end function ft4_rxcost0
  real function ft4_rxcost(m)
    integer, intent(in) :: m
    if(ft4rxcost(m).gt.0.) then; ft4_rxcost=ft4rxcost(m); return; endif
    ft4_rxcost=ft4_rxcost0(m)
  end function ft4_rxcost
  subroutine ft4_rxcost_decay(m)
    integer, intent(in) :: m
    ft4rxcost(m)=0.9*ft4_rxcost(m)
  end subroutine ft4_rxcost_decay
  subroutine ft4_rxcost_learn(m,tmeas)
    integer, intent(in) :: m
    real, intent(in) :: tmeas
    integer :: k
    if(m.lt.0 .or. m.gt.6 .or. tmeas.le.0.) return
    if(ft4rxcost(m).gt.0.) then; ft4rxcost(m)=0.5*ft4rxcost(m)+0.5*tmeas; else; ft4rxcost(m)=tmeas; endif
    do k=0,6
       if(k.ne.m .and. ft4rxcost(k).le.0.) ft4rxcost(k)=ft4rxcost(m)*ft4_rxcost0(k)/max(0.01,ft4_rxcost0(m))
    enddo
  end subroutine ft4_rxcost_learn

  real function bg_cost(kind,im)   ! CE3TSK: a unit's expected wall time; measured on this machine once it ran
    integer, intent(in) :: kind,im
    if(bgcost(kind,im).gt.0.) then; bg_cost=bgcost(kind,im); return; endif
    bg_cost=bg_cost0(kind,im)
  end function bg_cost

  ! CE3TSK: the anchored grid's slice boundaries - absolute cell edges that fall inside
  ! nfa-nfb, the first slice clipped to nfa and the last run out to nfb. This construction
  ! stood written out three times (the FT8 loop, the FT4 receive loop and ft4_background) with
  ! slice_count below repeating the arithmetic a fourth time only to count. Edit one copy and
  ! the receive and background phases slice the band differently, which is exactly what
  ! ft4bg.sh and ft4strict.sh assert cannot happen.
  subroutine slice_edges(nhalf,nw,nslpassx,nflo,nfhi)
    integer, intent(in) :: nhalf,nw
    integer, intent(out) :: nslpassx
    integer, intent(inout) :: nflo(*),nfhi(*)
    integer :: k,nb,nsl
    nslpassx=slice_count(nhalf,nw)
    nsl=0
    do k=1,4*nmaxthreads
       nb=k*nw+nhalf*(nw/2)
       if(nb.ge.nfb .or. nsl+1.ge.nslpassx) exit
       if(nb.le.nfa) cycle
       nsl=nsl+1; nfhi(nsl)=nb
    enddo
    nfhi(nslpassx)=nfb; nflo(1)=nfa
    do k=2,nslpassx; nflo(k)=nfhi(k-1)+1; enddo
  end subroutine slice_edges

  ! CE3TSK: how a unit's measured cost is kept. Both rules stood twice - once for the receive
  ! budget (P8) and once inside ft8_background - so changing a weight in one had the two phases
  ! learning different cost curves from the same member table, which the recipe timings assume
  ! to be one curve. The elapsed time is measured at the call site, exactly where it was.
  subroutine cost_decay(kind,im)   ! did not fit the budget: decay the estimate until it does
    integer, intent(in) :: kind,im
    bgcost(kind,im)=0.9*bg_cost(kind,im)
  end subroutine cost_decay

  subroutine cost_learn(kind,im,tmeas)   ! blend, so one slow run under contention does not set it alone
    integer, intent(in) :: kind,im
    real, intent(in) :: tmeas
    if(bgcost(kind,im).gt.0.) then
       bgcost(kind,im)=0.5*bgcost(kind,im)+0.5*tmeas
    else
       bgcost(kind,im)=tmeas
    endif
  end subroutine cost_learn

  integer function slice_count(nhalf,nw)   ! CE3TSK: slices of the anchored grid over nfa-nfb, boundaries shifted by half a cell for nhalf=1
    integer, intent(in) :: nhalf,nw   ! CE3TSK: nw is the cell width - FT8 passes nslicew, FT4 nf4w
    integer :: k,nb
    slice_count=1
    do k=1,4*nmaxthreads
       nb=k*nw+nhalf*(nw/2)
       if(nb.ge.nfb .or. slice_count.ge.24) exit   ! packjt77's hash tables treat indices above 24 as the TX message
       if(nb.gt.nfa) slice_count=slice_count+1
    enddo
  end function slice_count

  subroutine dump_params()
! CE3TSK: debug dump of the parameter block, for checking that the file mode sends what the GUI
! sends; contained, so both mode paths can call it (2026-09-01, TODO.md 2.5)
       call get_environment_variable('JTDX_DUMP_PARAMS',dumpfile,ldump,idumpstat)
       if(idumpstat.eq.0 .and. ldump.gt.0) then
          open(97,file=dumpfile(1:ldump),status='replace')
          write(97,'(a,*(a))') 'mycall=',params%mycall
          write(97,'(a,*(a))') 'mybcall=',params%mybcall
          write(97,'(a,*(a))') 'hiscall=',params%hiscall
          write(97,'(a,*(a))') 'hisbcall=',params%hisbcall
          write(97,'(a,*(a))') 'hisgrid=',params%hisgrid
          write(97,*) 'listutc=',params%listutc
          write(97,*) 'napwid=',params%napwid
          write(97,*) 'nbgbudget=',params%nbgbudget
          write(97,*) 'nft8bgclassic=',params%nft8bgclassic
          write(97,*) 'nQSOProgress=',params%nQSOProgress
          write(97,*) 'nftx=',params%nftx
          write(97,*) 'nutc=',params%nutc
          write(97,*) 'ntrperiod=',params%ntrperiod
          write(97,*) 'nfqso=',params%nfqso
          write(97,*) 'npts8=',params%npts8
          write(97,*) 'nfa=',params%nfa
          write(97,*) 'nfsplit=',params%nfsplit
          write(97,*) 'nfb=',params%nfb
          write(97,*) 'ntol=',params%ntol
          write(97,*) 'kin=',params%kin
          write(97,*) 'nzhsym=',params%nzhsym
          write(97,*) 'ndepth=',params%ndepth
          write(97,*) 'ncandthin=',params%ncandthin
          write(97,*) 'ndtcenter=',params%ndtcenter
          write(97,*) 'nft8cycles=',params%nft8cycles
          write(97,*) 'nft8swlcycles=',params%nft8swlcycles
          write(97,*) 'ntxmode=',params%ntxmode
          write(97,*) 'nmode=',params%nmode
          write(97,*) 'nlist=',params%nlist
          write(97,*) 'nranera=',params%nranera
          write(97,*) 'ntrials10=',params%ntrials10
          write(97,*) 'ntrialsrxf10=',params%ntrialsrxf10
          write(97,*) 'naggressive=',params%naggressive
          write(97,*) 'nharmonicsdepth=',params%nharmonicsdepth
          write(97,*) 'ntopfreq65=',params%ntopfreq65
          write(97,*) 'nprepass=',params%nprepass
          write(97,*) 'nsdecatt=',params%nsdecatt
          write(97,*) 'nlasttx=',params%nlasttx
          write(97,*) 'ndelay=',params%ndelay
          write(97,*) 'nmt=',params%nmt
          write(97,*) 'nft8rxfsens=',params%nft8rxfsens
          write(97,*) 'nft4depth=',params%nft4depth
          write(97,*) 'lft4altpass=',params%lft4altpass
          write(97,*) 'nft4ensemble=',params%nft4ensemble
          write(97,*) 'lft4deeposd=',params%lft4deeposd
          write(97,*) 'nft4bgensemble=',params%nft4bgensemble
        write(97,*) 'nft4bgdepth=',params%nft4bgdepth
        write(97,*) 'lft4bgdeeposd=',params%lft4bgdeeposd
        write(97,*) 'lft4bgaltpass=',params%lft4bgaltpass
        write(97,*) 'lft4bgtwopass=',params%lft4bgtwopass
        write(97,*) 'lft4bgresidual=',params%lft4bgresidual
        write(97,*) 'nft4sens=',params%nft4sens
        write(97,*) 'nft4bgsens=',params%nft4bgsens
        write(97,*) 'nft4rxfsens=',params%nft4rxfsens
        write(97,*) 'nft4bgrxfsens=',params%nft4bgrxfsens
        write(97,*) 'nft4bgeffort=',params%nft4bgeffort
        write(97,*) 'nlasttx=',params%nlasttx
          write(97,*) 'nsecbandchanged=',params%nsecbandchanged
          write(97,*) 'ndiskdat=',params%ndiskdat
          write(97,*) 'newdat=',params%newdat
          write(97,*) 'nagain=',params%nagain
          write(97,*) 'nagainfil=',params%nagainfil
          write(97,*) 'nswl=',params%nswl
          write(97,*) 'nfilter=',params%nfilter
          write(97,*) 'nstophint=',params%nstophint
          write(97,*) 'nagcc=',params%nagcc
          write(97,*) 'nhint=',params%nhint
          write(97,*) 'fmaskact=',params%fmaskact
          write(97,*) 'showharmonics=',params%showharmonics
          write(97,*) 'lft8lowth=',params%lft8lowth
          write(97,*) 'lft8subpass=',params%lft8subpass
          write(97,*) 'ltxing=',params%ltxing
          write(97,*) 'lhidetest=',params%lhidetest
          write(97,*) 'lhidetelemetry=',params%lhidetelemetry
          write(97,*) 'lhideft8dupes=',params%lhideft8dupes
          write(97,*) 'lhound=',params%lhound
          write(97,*) 'lhidehash=',params%lhidehash
          write(97,*) 'lcommonft8b=',params%lcommonft8b
          write(97,*) 'lmycallstd=',params%lmycallstd
          write(97,*) 'lhiscallstd=',params%lhiscallstd
          write(97,*) 'lapmyc=',params%lapmyc
          write(97,*) 'lmodechanged=',params%lmodechanged
          write(97,*) 'lbandchanged=',params%lbandchanged
          write(97,*) 'lenabledxcsearch=',params%lenabledxcsearch
          write(97,*) 'lwidedxcsearch=',params%lwidedxcsearch
          write(97,*) 'lmultinst=',params%lmultinst
          write(97,*) 'lskiptx1=',params%lskiptx1
          write(97,*) 'lforcesync=',params%lforcesync
          write(97,*) 'learlystart=',params%learlystart
          write(97,*) 'lft8deeposd=',params%lft8deeposd
          write(97,*) 'lft8twopass=',params%lft8twopass
          write(97,*) 'lft8altpass=',params%lft8altpass
          write(97,*) 'nft8ensemble=',params%nft8ensemble
          close(97)
       endif
  end subroutine dump_params

  subroutine ft4_background()
! CE3TSK: the FT4 TX background (item 59) - FT8's pipeline shape carried over, one unit: the
! ensemble members the RX phase did not run (nft4ensfrom..nft4bgensemble, ft4b's member
! ladder) - or, when none is left (item 78), the phase's own extras alone: deep OSD, the
! alternate pass, the residual, its sensitivity - decoded over the retained pristine band
! with both slicings, the same emit merge -
! so only new messages print, '|'-marked ('#' -> the cross for a hint), and everything feeds
! the hint memory exactly as an RX decode would. Aborts between slicings when the GUI removed
! .lock or created .bgabort (nbg4run=1; file mode nbg4run=2 runs uninterrupted, as FT8 does).
! No clock: the whole unit is ~100 ms + ~75 ms a member against a 7.5 s TX window.
    integer :: nmsg0,nbgu,k4b,ihalf4b,nslpass4b,nsl4b,nb4b,ndepth4b,nres4,ihalf,ndone4
    real :: resfac4,syncmin0
    real(8) :: tbg0,tsl0,deadline4,tbudget4
    nmsg0=nseen4; nbgu=0; lbgabort=.false.; ndone4=0; tbg0=omp_get_wtime()
    ! CE3TSK item 73: the clock, FT8's rule (ft8_background): under the GUI's rules (nbg4run=1,
    ! JTDX_BG_LOCK=1 in file mode) the phase must end before the next decode - the deadline is
    ! the period (7.5 s) or the -k budget from the decode's start, less the margin; a slicing is
    ! not started when its learned cost would not fit (item 80: FT8's bg_cost gate, kind 7 - the
    ! estimate decays on a skip so a one-off slow run does not exclude it for good). File mode
    ! (nbg4run=2) is unclocked.
    tbudget4=7.5d0; if(params%nbgbudget.gt.0) tbudget4=dble(params%nbgbudget)/10.d0
    deadline4=tdecstart+tbudget4-dble(max(0,params%nbgmargin))/10.d0
    ! item 73: the background's own sensitivity, from the hooks' base values; the residual scales from it
    ft4syncmin=ft4syncminbase; nft4syncqual=nft4syncqbase
    if(params%nft4bgsens.ge.1) then; ft4syncmin=min(ft4syncmin,1.0); nft4syncqual=min(nft4syncqual,16); endif
    nft4rxfsens=max(0,min(3,params%nft4bgrxfsens))   ! item 75: the background's own level; the seeded DT is the period's
    ! CE3TSK item 71/72: FT8's residual unit as the last background slicing - the switch below, or JTDX_FT4_RESIDUAL=f
    ! after the members - every known decode subtracted (the merged deltas), the candidate
    ! sync threshold scaled by f (FT8 uses 0.8), no members - as one more slicing over the band
    nres4=0; resfac4=1.0; syncmin0=ft4syncmin
    if(params%lft4bgresidual) then; nres4=1; resfac4=bgresidual; endif   ! item 72: the TX background submenu's switch, FT8's 0.8
    call get_environment_variable('JTDX_FT4_RESIDUAL',dumpfile,ldump,idumpstat)
    if(idumpstat.eq.0 .and. ldump.gt.0) then
       read(dumpfile(1:ldump),*,iostat=ios) xk4
       if(ios.eq.0) then; nres4=1; resfac4=max(0.3,min(1.0,xk4)); endif
    endif
    ! CE3TSK item 69: the background phase's own settings (Decode -> FT4 decoding -> expert -> TX
    ! background), as FT8's background has its own submenu: effort (0 = the RX phase's), deep
    ! OSD, the alternate pass and the second slicing, each starting from the env hooks' base
    ! values rather than from whatever the RX phase switched on
    nft4alt=nft4altbase; if(params%lft4bgaltpass) nft4alt=1
    nft4osddeep=nft4osdbase; if(params%lft4bgdeeposd) nft4osddeep=max(nft4osddeep,4)
    ndepth4b=params%nft4depth
    if(params%nft4bgdepth.ge.1 .and. params%nft4bgdepth.le.3) ndepth4b=params%nft4bgdepth
    nfrom4=max(0,min(6,nft4rxrun))+1   ! item 80: the count the RX phase actually ran (budget auto decides it there)
    nto4=max(0,min(6,params%nft4bgensemble))
    if(allocated(dd4prist) .and. nthr4.gt.1) then   ! item 78: the switch brought us here; one thread has no slice grid to run over
       if(nbg4run.eq.1 .and. bg_lock_gone()) then; lbgabort=.true.; go to 480; endif
       nft4ensfrom=nfrom4; nft4ens=max(0,nto4-nfrom4+1)   ! the members above the RX count; none = the extras alone (item 78)
       ! the RX phase's grid, rebuilt from the same inputs (nsl4/nf4w are period state)
       if(allocated(dd4orig)) deallocate(dd4orig)
       if(allocated(dd4delta)) deallocate(dd4delta)
       allocate(dd4orig(size(dd4prist))); dd4orig=dd4prist
       nsl4b=max(1,min(NFT4SLICEMAX,max(slice_count(0,nf4w),slice_count(1,nf4w))))
       allocate(dd4delta(size(dd4prist),nsl4b)); dd4delta=0.; lcollectdelta4=.true.
       nhalf4b=0; if(params%lft4bgtwopass) nhalf4b=1   ! CE3TSK item 69: the background's own switch
       do ihalf4b=0,nhalf4b+nres4
          if(nbg4run.eq.1 .and. bg_lock_gone()) then; lbgabort=.true.; exit; endif
          imc4=ihalf4b; if(ihalf4b.gt.nhalf4b) imc4=2   ! the cost table's index: slicing 0 / 1, the residual 2
          if(nbg4run.eq.1 .and. omp_get_wtime()+dble(bg_cost(7,imc4)).gt.deadline4) then   ! item 73/80: the clock - no abort, the phase ends early
             call cost_decay(7,imc4); exit
          endif
          tsl0=omp_get_wtime()
          if(ihalf4b.ge.1) then
             do k4b=1,nsl4b; dd4orig=dd4orig+dd4delta(:,k4b); enddo
             dd4delta=0.
          endif
          ihalf=ihalf4b
          if(ihalf4b.gt.nhalf4b) then   ! the residual unit: the unshifted grid, no members, the lowered threshold
             ihalf=0; nft4ens=0; ft4syncmin=syncmin0*resfac4
          endif
          call slice_edges(ihalf,nf4w,nslpass4b,nf4lo,nf4hi)
          nft4res=0
!$omp parallel do schedule(dynamic,1) num_threads(nthr4) default(shared) private(k4b)
          do k4b=1,nslpass4b
             dd4=dd4orig
             call my_ft4%decode(ft4_decoded,params%nQSOProgress,nfqso,nf4lo(k4b),nf4hi(k4b), &
                  ndepth4b,params%nstophint,params%nswl,k4b,nthr4)   ! item 69: the background's effort
          enddo
!$omp end parallel do
          call my_ft4%emit(nslpass4b)
          call cost_learn(7,imc4,real(omp_get_wtime()-tsl0)); ndone4=ndone4+1
       enddo
       ft4syncmin=syncmin0
       if(ndone4.eq.nhalf4b+1+nres4) nbgu=1   ! item 73: the unit counts when every slicing ran
       call fillhash(nsl4b,.true.)   ! CE3TSK: every slice, as in the receive phase above (nsl4 is a scratch counter here)
    endif
480 continue
    nft4ensfrom=1
    write(*,1480) nbgu,nseen4-nmsg0,omp_get_wtime()-tbg0,merge(1,0,lbgabort)   ! item 73: <secs>, FT8's line
1480 format('<BackgroundFinished><units>',i3,'<msgs>',i4,'<secs>',f6.2,'<abort>',i1)
    call flush(6)
  end subroutine ft4_background

  subroutine ft8_background()
! CE3TSK: the pipeline ensemble's background phase (PIPELINED_DECODE_PLAN.md): more units
! on the retained band while the decoder would otherwise idle. The unit list, in order:
! the TX background recipe's own pass list (SWL mode, cycles, sensitivities, OSD, passes as
! set in the TX background menu), the ensemble members it asks for, the residual pass, the
! retry unit. It stops when the list is done, when the next unit would not fit before the
! deadline (the period length from the decode's start, less the margin), or - at once,
! inside a unit - when the GUI removed .lock for the next decode. Only new messages print,
! with the period's UTC; a status line closes the phase.
    integer :: list(24),listim(24),nlist,iu,nunits,nmsg0,nmsg1,nmax,j
    real(8) :: t0,t1,deadline,tbudget
    logical :: ltrace
    character(len=8) :: uname
! the unit list: the background recipe's own pass list, then its ensemble members (the
! members the period's decode did not run; -1 = every one the budget allows), the residual
! pass, and last the retry unit (measured to find nothing - the failures are noise candidates)
    nlist=1; list(1)=5; listim(1)=0
    if(params%nft8bgclassic.gt.0) then; nlist=nlist+1; list(nlist)=6; listim(nlist)=0; endif   ! P9: the classic unit, cheap and early
    nbgm=params%nft8bgensemble; if(nbgm.lt.0) nbgm=NENSMAX
    do im=nbgnext,min(NENSMAX,nbgnext+nbgm-1); nlist=nlist+1; list(nlist)=1; listim(nlist)=im; enddo
! CE3TSK: the deferred alternate pass (unit kind 2). laltdeferred is set when the pipeline runs
! multi-threaded with the alternate pass switched OFF in the menu - i.e. the decodes the receive
! phase deferred - but the unit was never put in this list and JTDX_BG_ORDER had no letter for
! it, so kind 2 and its trace name were unreachable and those decodes were simply never made.
! Behind a hook first, exactly as the residual unit was before item 72: JTDX_BG_ALTDEFER=1
! schedules it, the default list is unchanged, and what it is worth is a measurement, not a guess.
    call get_environment_variable('JTDX_BG_ALTDEFER',dumpfile,ldump,idumpstat)
    if(idumpstat.eq.0 .and. ldump.gt.0 .and. dumpfile(1:1).eq.'1' .and. laltdeferred) then
       nlist=nlist+1; list(nlist)=2; listim(nlist)=0
    endif
    nlist=nlist+1; list(nlist)=3; listim(nlist)=0
    nlist=nlist+1; list(nlist)=4; listim(nlist)=0
    nmax=nlist
! CE3TSK experiment hook: JTDX_BG_ORDER=r,m1,m2,s,... replaces the unit list (r recipe, mN member N,
! s residual, t retry, c classic, a the deferred alternate pass)
    call get_environment_variable('JTDX_BG_ORDER',dumpfile,ldump,idumpstat)
    if(idumpstat.eq.0 .and. ldump.gt.0) then
       nlist=0; k=1
       do while(k.le.ldump .and. nlist.lt.24)
          j=index(dumpfile(k:ldump),','); if(j.eq.0) then; j=ldump+1; else; j=k+j-1; endif
          if(j.gt.k) then
             nlist=nlist+1; listim(nlist)=0
             select case(dumpfile(k:k))
             case('r'); list(nlist)=5
             case('s'); list(nlist)=3
             case('t'); list(nlist)=4
             case('c'); list(nlist)=6
             case('a'); list(nlist)=2   ! CE3TSK: the deferred alternate pass, kind 2
             case('m'); list(nlist)=1; read(dumpfile(k+1:j-1),*,iostat=ios) listim(nlist)
                if(ios.ne.0 .or. listim(nlist).lt.1 .or. listim(nlist).gt.NENSMAX) nlist=nlist-1
             case default; nlist=nlist-1
             end select
          endif
          k=j+1
       enddo
       nmax=nlist
    endif
! CE3TSK P7: the GUI hands a two-period window when the coming period is our TX (it skips
! that period's decode); 0 means the period, as before
    tbudget=dble(params%nbgbudget)/10.d0; if(tbudget.le.0.d0) tbudget=dble(max(1,params%ntrperiod))
    deadline=tdecstart+tbudget-dble(max(0,params%nbgmargin))/10.d0
    call get_environment_variable('JTDX_ENSEMBLE_TRACE',dumpfile,ldump,idumpstat); ltrace=(idumpstat.eq.0)
    nunits=0; nmsg0=ndecodes; t0=omp_get_wtime(); lbgabort=.false.
    do iu=1,nmax
       if(nbgrun.eq.1) then
          if(bg_lock_gone()) then; lbgabort=.true.; exit; endif
          if(iu.gt.1 .and. omp_get_wtime()+dble(bg_cost(list(iu),listim(iu))).gt.deadline) then   ! the recipe itself always runs
             call cost_decay(list(iu),listim(iu))   ! a one-off slow run must not exclude it for good
             exit
          endif
       endif
       t1=omp_get_wtime(); nmsg1=ndecodes
       call ft8_unit(list(iu),listim(iu))
       if(lbgabort) exit
       nunits=nunits+1
       call cost_learn(list(iu),listim(iu),real(omp_get_wtime()-t1))
       if(ltrace) then
          select case(list(iu))
          case(1); write(uname,'(a,i1)') 'member',listim(iu)
          case(2); uname='altpass'
          case(3); uname='residual'
          case(4); uname='retry'
          case(5); uname='bgrecipe'
          case(6); uname='classic'
          end select
          ! CE3TSK: the sub-pass state the unit left behind. The classic unit (kind 6) runs
          ! without the sub-pass and used to assign params%lft8subpass=.false. flat, so every
          ! unit after it in the list decoded with a weaker recipe than configured; it now
          ! saves and restores the setting, and this field is what bgabort.sh's subpass case
          ! asserts - every unit after the classic one must still report the background's value.
          write(0,'(a,i2,1x,a8,a,i3,a,f6.2,a,i4,a,i4,a,i5,a,i2)') 'bg unit',iu,uname,' new',ndecodes-nmsg1, &
               ' secs',omp_get_wtime()-t1, &
               ' tones',ntones,' fails',sum(nfail(1:nslicesft8)),' retried',nretried, &
               ' sub',merge(1,0,logical(params%lft8subpass))
          if(list(iu).eq.4) write(0,'(a,4i6)') '        retry outcomes (0/gates/ldpc/decoded):',nretrystage
          do k=nmsg1+1,min(ndecodes,size(allmessages)); write(0,'(a,a)') '        + ',trim(allmessages(k)); enddo
       endif
    enddo
    call ft8_restore()
    call fillhash(nslicesft8,.true.)
    write(*,1020) nunits,ndecodes-nmsg0,omp_get_wtime()-t0,merge(1,0,lbgabort)
1020 format('<BackgroundFinished><units>',i3,'<msgs>',i4,'<secs>',f6.2,'<abort>',i1)
    call flush(6)
  end subroutine ft8_background

  subroutine ft8_decoded (this,snr,dt,freq,decoded,servis8)
    use ft8_decode
    implicit none

    class(ft8_decoder), intent(inout) :: this
    integer, intent(in) :: snr
    real, intent(in) :: dt
    real, intent(in) :: freq
    character(len=26), intent(in) :: decoded
    character(len=1), intent(in) :: servis8
    character(len=3) :: mark   ! CE3TSK: the marker as printed, up to three UTF-8 bytes

! CE3TSK: an ensemble member's band is delayed or frequency shifted; report the signal's own
! DT and frequency (ft8ensemble.f90 sets the corrections, zero for the base decode). The
! marker '#' (a hint decode in the TX background) prints as the box-drawing cross U+253C.
    mark=servis8
    if(servis8.eq.'#') mark=char(226)//char(148)//char(188)
    write(*,1000) nutc,snr,dt+ensdtcorr,nint(freq+ensfreqcorr),decoded,trim(mark)
1000 format(i6.6,i4,f5.1,i5,1x,'~',1x,a26,a)
!    write(13,1002) nutc,snr,dt,freq,0,decoded
!1002 format(i6.6,i5,f6.1,f8.0,i4,3x,a26,' FT8')
    call flush(6)
!    call flush(13)

    select type(this)
    type is (counting_ft8_decoder)
! CE3TSK: called from every decoding thread; the count and the DT list feed next period's DT
! window centre, so two simultaneous decodes must not lose an increment or share a slot
!$omp critical(ft8_decoded_count)
       if(this%decoded.lt.size(this%xdtt)) then
          this%decoded = this%decoded + 1
          this%xdtt(this%decoded)=dt+ensdtcorr
       endif
!$omp end critical(ft8_decoded_count)
    end select

    return
  end subroutine ft8_decoded

  subroutine ft4_decoded (this,snr,dt,freq,decoded,servis4)
    use ft4_decode
    implicit none

    class(ft4_decoder), intent(inout) :: this
    integer, intent(in) :: snr
    real, intent(in) :: dt
    real, intent(in) :: freq
    character(len=26), intent(in) :: decoded
    character(len=1), intent(in) :: servis4
    character(len=3) :: mark4   ! CE3TSK: the marker as printed - '#' is the background hint's
                                ! cross U+253C, three UTF-8 bytes, exactly as ft8_decoded prints it
    character(len=1) :: sep4    ! CE3TSK: the mode column - FT8 prints '~', FT4 ':', and FT2 ';'.
                                ! FT2 shares this emit with FT4, so it needs its own character here
                                ! or an operator cannot tell the two apart in the window, in ALL.TXT
                                ! or in anything that reads them afterwards. The COLUMN does not
                                ! move: the format is fixed width and only the character differs.

    mark4=servis4
    if(servis4.eq.'#') mark4=char(226)//char(148)//char(188)
    sep4=':'; if(lft2) sep4=';'
    write(*,1001) nutc,snr,dt,nint(freq),sep4,decoded,trim(mark4)
1001 format(i6.6,i4,f5.1,i5,1x,a1,1x,a26,a)
    call flush(6)
    
    select type(this)
    type is (counting_ft4_decoder)
       this%decoded = this%decoded + 1
    end select

    return
  end subroutine ft4_decoded

end subroutine multimode_decoder
