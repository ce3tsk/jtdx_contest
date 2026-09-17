subroutine jt9a()
  use, intrinsic :: iso_c_binding, only: c_f_pointer
  use prog_args
!  use timer_module, only: timer
!  use timer_impl, only: init_timer !, limtrace
  use jt65_mod6
  use ft8_mod1, only : dd8
  use ft4_mod1, only : dd4
  use ft8ensemble, only : nbgrun,nbgunits,lbgabort   ! CE3TSK: pipeline ensemble
  use ft4_mod1, only : nbg4run   ! CE3TSK: the FT4 TX background (item 59)
  include 'jt9com.f90'

! These routines connect the shared memory region to the decoder.
  interface
     function address_jtdxjt9()
       use, intrinsic :: iso_c_binding, only: c_ptr
       type(c_ptr) :: address_jtdxjt9
     end function address_jtdxjt9
  end interface

  integer*1 attach_jtdxjt9
  integer size_jtdxjt9
! Multiple instances:
  character*80 mykey
  type(dec_data), pointer, volatile :: shared_data !also makes target volatile
  type(params_block) :: local_params
  integer :: nreq=-1, nlastreq=-1   ! CE3TSK: the request being served, and the last one served
  logical fileExists

! Multiple instances:
  i0 = len(trim(shm_key))

!  call init_timer (trim(data_dir)//'/timer.out')
!  open(23,file=trim(data_dir)//'/CALL3.TXT',status='unknown')

!  limtrace=-1                            !Disable all calls to timer()

! Multiple instances: set the shared memory key before attaching
  mykey=trim(repeat(shm_key,1))
  i0 = len(mykey)
  i0=setkey_jtdxjt9(trim(mykey))

  i1=attach_jtdxjt9()

10 inquire(file=trim(temp_dir)//'/.lock',exist=fileExists)
  if(fileExists) then
!     call sleep_msec(100)
     call sleep_msec(10)
     go to 10
  endif


11 inquire(file=trim(temp_dir)//'/.quit',exist=fileExists)
  if(fileExists) then
     i1=detach_jtdxjt9()
     go to 999
  endif
  if(i1.eq.999999) stop                  !Silence compiler warning

  nbytes=size_jtdxjt9()
  if(nbytes.le.0) then
     print*,'jt9a: Shared memory mem_jtdxjt9 does not exist.'
     print*,"Must start 'jtdxjt9 -s <thekey>' from within WSJT-X."
     go to 999
  endif
  call c_f_pointer(address_jtdxjt9(),shared_data)
  local_params=shared_data%params !save a copy because jtdx.exe carries on accessing
  nreq=local_params%ndecreq   ! CE3TSK: the request this pass serves
  call flush(6)

  nnmode=local_params%nmode

  if(local_params%nmode.eq.8) then; npts1=180000
  else if(local_params%nmode.eq.4) then; npts1=73728
! CE3TSK: FT2's period is 3.75 s and the FT4 chain decodes it from a stream stretched x2, so the
! capture is half of dd4's length: 36864 real samples, 3.072 s of the period. The rest of the
! period is out of reach - dd4 is 73728 and raising it would change FT4's FFT length.
  else if(local_params%nmode.eq.52) then; npts1=36864
  else; npts1=NPTS
  endif

! CE3TSK: the shared buffer is 16 bit again (AUDIO_DEPTH_PLAN.md, 2026-09-03). The decoder's
! float content is kept exactly where the 32 bit chain had it - 65536 times the sample - so
! the GUI path and file mode (jt9.f90 scales 16 bit files by the same 65536) stay one
! arithmetic and no recorded expectation moves. A power of two is exact in float; it is the
! additive epsilons and dB thresholds downstream that make the scale worth pinning.
  if(local_params%ndiskdat) then
    if(local_params%nmode.eq.8) then ! nblocks values shall match to ihsym/m_hsymStop in mainwindow
      if(local_params%nswl) then; nblocks=51
      else if(local_params%learlystart) then; nblocks=48
      else; nblocks=49
      endif
      local_params%nzhsym=nblocks; nlastsam=nblocks*3456
      dd(1:nlastsam)=65536.0*shared_data%id2(1:nlastsam)
      dd(nlastsam+1:npts1)=0.
    else
     dd(1:npts1)=65536.0*shared_data%id2(1:npts1)
    endif
  else
     if(local_params%nmode.eq.8) then
        rms=sum(abs(shared_data%dd2(1:10)))+sum(abs(shared_data%dd2(76001:76010)))+ &
            sum(abs(shared_data%dd2(151670:151680)))
     else if(local_params%nmode.eq.4) then
        rms=sum(abs(shared_data%dd2(1:10)))+sum(abs(shared_data%dd2(30001:30010)))+ &
            sum(abs(shared_data%dd2(60470:60480)))
     else if(local_params%nmode.eq.52) then   ! CE3TSK: the same three probes inside FT2's 36864
        rms=sum(abs(shared_data%dd2(1:10)))+sum(abs(shared_data%dd2(18001:18010)))+ &
            sum(abs(shared_data%dd2(36855:36864)))
     else
        rms=sum(abs(shared_data%dd2(1:10)))+sum(abs(shared_data%dd2(300000:300010)))+ &
            sum(abs(shared_data%dd2(623991:624000)))
     endif
     if(rms.gt.0.001) then
        dd(1:npts1)=65536.0*shared_data%dd2(1:npts1)
!print *,'win7',rms
     else ! workaround for zero data values of dd2 array under WinXP
        dd(1:npts1)=65536.0*shared_data%id2(1:npts1)
!print *, 'winxp',rms
     endif
  endif

  if(local_params%nmode.eq.8) then; dd8(1:npts1)=dd(1:npts1)
  else if(local_params%nmode.eq.4) then; dd4(1:npts1)=dd(1:npts1)
  else if(local_params%nmode.eq.52) then
! CE3TSK: the stretch - every sample twice, a zero order hold. This is the whole of FT2's receive
! side: what the FT4 decoder then sees is an FT4 signal, and only the frequency limits on the way in
! and the frequency and DT on the way out have to be converted (decoder.f90, ft4emit, ft4b).
     do i=1,npts1
        dd4(2*i-1)=dd(i); dd4(2*i)=dd(i)
     enddo
  endif

!  call timer('decoder ',0)
  call multimode_decoder(local_params)
  nlastreq=nreq   ! CE3TSK: served; anything newer than this is work waiting
!  call timer('decoder ',1)
! CE3TSK: pipeline ensemble (PIPELINED_DECODE_PLAN.md) - the background phase decodes the
! period again in the idle time, until the GUI removes .lock for the next decode; if it did,
! the new data is already in shared memory: go straight to it
! CE3TSK: nmode 52 is FT2, decoded by the same FT4 chain, so its background phase is this one
  if((local_params%nmode.eq.4 .or. local_params%nmode.eq.52) .and. local_params%nft4bgeffort.ne.0) then
! CE3TSK: the FT4 TX background (item 59), FT8's block below mirrored - one unit in the idle
! time, .lock/.bgabort rules identical. Item 78: the switch alone decides, as nft8bgeffort does
! for FT8 - the member target no longer has to exceed the RX count (with no member left the
! unit runs the phase's extras alone)
     nbg4run=1; lbgabort=.false.
     call multimode_decoder(local_params)
     nbg4run=0
     if(lbgabort) then
        inquire(file=trim(temp_dir)//'/.lock',exist=fileExists)
        if(.not.fileExists) go to 11
     endif
  endif
  if(local_params%nmode.eq.8 .and. local_params%nft8bgeffort.ne.0) then
     nbgrun=1; nbgunits=local_params%nft8bgeffort; lbgabort=.false.
     call multimode_decoder(local_params)
     nbgrun=0
! CE3TSK: an abort means one of two things. The GUI removed .lock for the next period's
! decode (its data is already in shared memory): go straight to it. Or the GUI created the
! .bgabort sentinel (a band or mode change) while .lock is still there: nothing new is
! waiting - fall through to the normal wait below. Jumping back unconditionally re-decoded
! the same retained audio in a loop (every message printed again each time, mostly as hint
! decodes) until the next decode() cleared the sentinel - with Monitor off, for good.
     if(lbgabort) then
        inquire(file=trim(temp_dir)//'/.lock',exist=fileExists)
        if(.not.fileExists) go to 11
     endif
  endif

! CE3TSK: this wait is where a decode request used to be lost. It sleeps while .lock is
! ABSENT, and .lock going absent is exactly how the GUI asks for a decode - so a request that
! arrived before the decoder got here was never noticed, and the two sides deadlocked: the GUI
! waiting for <DecodeFinished> with its Open menu greyed, the decoder sleeping here for a lock
! that only <DecodeFinished> would have recreated. Serving by request NUMBER instead of by the
! lock's edge cannot miss one. The lock still gates the ordinary path below, and re-decoding
! the same audio in a loop - the failure the comment above the background blocks records - is
! impossible, because a request is served once and then equals nlastreq.
100 inquire(file=trim(temp_dir)//'/.lock',exist=fileExists)
  if(fileExists) go to 10
  if(shared_data%params%ndecreq.ne.nlastreq) go to 11
  call sleep_msec(100)
  go to 100

999 continue
!    call timer('decoder ',101)

  return
end subroutine jt9a
