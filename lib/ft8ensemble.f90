! CE3TSK: ensemble decoding (ENSEMBLE_DECODE_PLAN.md).
!
! The FT8 chain is a sequence of threshold decisions on soft data - the sync baseline and
! thresholds, the strongest-first subtraction order, ft8b's sync-score rules, the first
! codeword that passes the CRC - and any of them falls differently when the input moves by
! less than the noise: a dither at 1 % of the RMS changes a single exhaustive decode of the
! benchmark capture between 77 and 86 messages. Each decode is one sample of what the band
! contains, and different samples fail on different signals, so the union of a few samples
! climbs well past any single one. This module makes such samples exactly reproducible:
! decoder.f90 decodes the pristine band again through each member's perturbation with the
! member's recipe, sharing the duplicate arrays so only new messages print.
!
! The member table is in the measured order of gain per second on wav/full_band_16.wav
! (exhaustive preset 85 messages in 2.6 s at 12 threads): +7 -> 92, +3 -> 95, +3 -> 98,
! +2 -> 100, +1 -> 101 of the 104 validated messages, none outside the reference set.
!
! Everything here is deterministic on any machine for members 1, 2 and 5 (integer shifts, a
! xorshift64 dither with IEEE single arithmetic only); the frequency shift of members 3 and
! 4 goes through FFTW and a phasor recurrence, deterministic for a given FFTW build.
module ft8ensemble
  use prog_args, only : temp_dir
  implicit none
  private
  public :: NENSMAX,NENSBASE,enskind,ensval,ensseed,ensswl,enscycles,ensalt,ensdtcorr,ensfreqcorr,ens_perturb
  public :: nbgrun,nbgunits,lbgabort,tdecstart,bgsyncscale,nretrydither,lretrymode,ift8stage,lbgtones,NRETRY,nrxmembers
  public :: ens_reset_period,ens_store_tone,ens_residual,ens_record_fail,ens_retry_candidates,bg_lock_gone, &
            ens_dither_cd0,ntones,nfail,nretried,nretrystage,ens_merge_tones

  ! members 1-5 are the RX-phase table (ensemble effort); 6-8 are extra members the pipeline
  ! ensemble's background phase runs at a high background effort
  integer, parameter :: NENSMAX=8, NENSBASE=5
  ! perturbation: 1 = delay by ensval samples, 2 = dither at ensval x RMS with seed ensseed,
  ! 3 = frequency shift by ensval Hz
! CE3TSK: members 6-8 are background-only and ordered by measured usefulness
! (DECODE_RECIPE_PLAN.md): the +0.8 Hz shift first - it pays on sparse and noisy bands -
! then the two that never found anything (dither seed 2, 8-sample delay), so a trimmed
! background effort (the pipeline ensemble preset uses 5) keeps the one that pays and an
! effort of "auto" still runs them all.
  integer, parameter :: enskind(NENSMAX)=(/1,2,3,3,1, 3,2,1/)
  real,    parameter :: ensval(NENSMAX)=(/128.,0.03,-0.8,1.2,128., 0.8,0.03,8./)
  integer, parameter :: ensseed(NENSMAX)=(/0,1,0,0,0, 0,2,0/)
  ! the member's recipe: SWL mode with enscycles SWL cycles, then the alternate-approach pass
  ! (7 plain cycles + OSD order 2 on the member's subtracted band) if ensalt - "exhaustive"
  logical, parameter :: ensswl(NENSMAX)=(/.true.,.true.,.true.,.true.,.true., .true.,.true.,.true./)
  integer, parameter :: enscycles(NENSMAX)=(/5,5,5,5,5, 5,5,5/)
  logical, parameter :: ensalt(NENSMAX)=(/.false.,.false.,.true.,.true.,.true., .true.,.false.,.false./)

  ! ---- pipeline ensemble (PIPELINED_DECODE_PLAN.md) ----
  ! nbgrun: 0 during a period's decode; 1 while the background phase runs under the GUI (it
  ! yields as soon as the GUI removes temp_dir/.lock for the next decode); 2 in file mode
  ! (an explicit number of units, no clock). nbgunits: units requested, -1 = as the budget allows.
  integer, save :: nbgrun=0, nbgunits=0
  logical, save :: lbgabort=.false.     ! the GUI asked for the next decode during the background phase
  real(8), save :: tdecstart=0.d0       ! wall time the period's decode started; the deadline counts from it
  logical, save :: lbgtones=.false.     ! ft8b stores each decode's tones (for the residual unit)
  logical, save :: lretrymode=.false.   ! ft8_decode takes its candidates from the failure list, no sync8
  real, save :: bgsyncscale=1.0         ! the residual unit's factor on syncmin
  integer, parameter :: NRETRY=3        ! tries per failed candidate in the retry unit
  integer :: nrxmembers=0               ! CE3TSK P8: members the period's RX phase ran (the <rxm> tag)
  integer, save :: nretrydither=0       ! > 0: ft8b dithers the downsampled signal with this seed
  integer, save :: ift8stage=0          ! ft8b's outcome: 1 rejected by the sync gates, 2 an LDPC/OSD attempt failed
  !$omp threadprivate(nretrydither,ift8stage)
  ! the decodes' tones, for subtracting every known signal from the pristine band
  integer, parameter :: NTONEMAX=200
  integer, save :: ntones=0
  integer, save :: itones(79,NTONEMAX)
  real, save :: tonef(NTONEMAX),tonedt(NTONEMAX)
  logical(1), save :: tonesw(NTONEMAX)
! CE3TSK: with the buffered emission the tone store is per slice during the pass and merged in
! slice order afterwards (ens_merge_tones) - the shared store under a critical was appended in
! thread-arrival order, and ens_residual subtracts in list order, so the residual band's float
! content depended on which thread stored first (the NP0PSX case, FT8_EMISSION_ORDER.md).
  integer, parameter :: NSLB=24   ! = ft8_mod1's NSLICE8MAX
  integer, save :: ntones_b(NSLB)=0
  integer, save :: itones_b(79,NTONEMAX,NSLB)
  real, save :: tonef_b(NTONEMAX,NSLB),tonedt_b(NTONEMAX,NSLB)
  logical(1), save :: tonesw_b(NTONEMAX,NSLB)
  ! the candidates that reached the LDPC/OSD stage and failed, per slice
  integer, parameter :: NFAILMAX=256, NFAILSL=48
  integer, save :: nfail(NFAILSL)=0
  real, save :: failf(NFAILMAX,NFAILSL),faildt(NFAILMAX,NFAILSL),failsync(NFAILMAX,NFAILSL)
  real, save :: retrydither=-1.   ! fraction of the downsampled signal's RMS; JTDX_RETRY_DITHER overrides (experiments)
  integer, save :: nretried=0     ! candidates the retry unit handed to ft8b (trace)
  integer, save :: nretrystage(0:3)=0   ! their outcomes: 1 sync gates, 2 LDPC/OSD failed, 3 decoded (trace)

  ! corrections the decode callback adds to a member's reported DT (s) and frequency (Hz):
  ! a delayed band measures a later DT, a shifted band a shifted frequency. Zero for the
  ! base decode.
  real, save :: ensdtcorr=0.,ensfreqcorr=0.

  complex, allocatable, save :: z(:)   ! one FFT work array, so four2a reuses its plan

contains

  subroutine ens_perturb(m,x,y,n)
    integer, intent(in) :: m,n
    real, intent(in) :: x(n)
    real, intent(out) :: y(n)
    integer :: i,j,ns
    integer(8) :: s
    real :: u,gain
    real(8) :: sumsq,rms,dphi,c,sn,wr,wi,pr,pi,t
    real(8), parameter :: twopi=6.283185307179586d0,fs=12000.d0

    ensdtcorr=0.; ensfreqcorr=0.
    select case(enskind(m))

    case(1)   ! delay: zeros in front, the tail dropped
       ns=nint(ensval(m)); ns=max(0,min(n-1,ns))
       y(1:ns)=0.; y(ns+1:n)=x(1:n-ns)
       ensdtcorr=-real(ns)/12000.

    case(2)   ! dither: Gaussian by the sum of 12 uniforms, xorshift64 with a fixed seed
       sumsq=0.d0
       do i=1,n; sumsq=sumsq+dble(x(i))*dble(x(i)); enddo
       rms=sqrt(sumsq/n)
       gain=ensval(m)*real(rms)
       s=int(ensseed(m),8)*2654435761_8+88172645463325252_8   ! never zero, and not the raw seed
       do i=1,n
          u=0.
          do j=1,12
             s=ieor(s,ishft(s,13)); s=ieor(s,ishft(s,-7)); s=ieor(s,ishft(s,17))
             u=u+real(ishft(s,-40))/16777216.   ! the top 24 bits, exact in single precision
          enddo
          y(i)=x(i)+(u-6.)*gain
       enddo

    case(3)   ! frequency shift: analytic signal (FFT), rotated by a phasor recurrence, real part
       if(.not.allocated(z)) allocate(z(n))
       if(size(z).ne.n) then; deallocate(z); allocate(z(n)); endif
       z=cmplx(x,0.)
       call four2a(z,n,1,-1,1)
       z(2:n/2)=2.*z(2:n/2); z(n/2+2:n)=cmplx(0.,0.)
       call four2a(z,n,1,1,1)   ! unnormalised, divided out below
       dphi=twopi*dble(ensval(m))/fs
       ! cos and sin of the small step from their series - no libm, so the phasor is the same
       ! on every machine; the recurrence then only multiplies and adds
       t=dphi*dphi
       c=1.d0-t/2.d0*(1.d0-t/12.d0*(1.d0-t/30.d0*(1.d0-t/56.d0*(1.d0-t/90.d0))))
       sn=dphi*(1.d0-t/6.d0*(1.d0-t/20.d0*(1.d0-t/42.d0*(1.d0-t/72.d0*(1.d0-t/110.d0)))))
       wr=1.d0; wi=0.d0
       do i=1,n
          y(i)=real((dble(real(z(i)))*wr-dble(aimag(z(i)))*wi)/n)
          pr=wr*c-wi*sn; pi=wr*sn+wi*c; wr=pr; wi=pi
       enddo
       ensfreqcorr=-ensval(m)

    case default
       y=x
    end select
  end subroutine ens_perturb

  ! ---- pipeline ensemble ----

  subroutine ens_reset_period()   ! at the start of a period's decode
    ntones=0; ntones_b=0; nfail=0; lbgabort=.false.; lretrymode=.false.; bgsyncscale=1.0; nretried=0; nretrystage=0
  end subroutine ens_reset_period

  ! ft8b stores the tones of a decode (refined DT) so the residual unit can subtract it; a
  ! decode found again by a later unit at the same place is not stored twice - subtracting a
  ! signal twice would add it back inverted
  subroutine ens_store_tone(itone,f,dt,swl,islice)
    use ft8_mod1, only : lft8buffered
    integer, intent(in) :: itone(79)
    real, intent(in) :: f,dt
    logical(1), intent(in) :: swl
    integer, intent(in) :: islice
    integer :: k,is
    if(lft8buffered) then   ! CE3TSK: this slice's own column, merged in slice order later
       is=max(1,min(NSLB,islice))
       do k=1,ntones_b(is)
          if(abs(tonef_b(k,is)-f).lt.2.0 .and. abs(tonedt_b(k,is)-dt).lt.0.06) return
       enddo
       do k=1,ntones   ! already merged from an earlier pass
          if(abs(tonef(k)-f).lt.2.0 .and. abs(tonedt(k)-dt).lt.0.06) return
       enddo
       if(ntones_b(is).lt.NTONEMAX) then
          ntones_b(is)=ntones_b(is)+1; itones_b(:,ntones_b(is),is)=itone
          tonef_b(ntones_b(is),is)=f; tonedt_b(ntones_b(is),is)=dt; tonesw_b(ntones_b(is),is)=swl
       endif
       return
    endif
    !$omp critical(ens_tones)
    do k=1,ntones
       if(abs(tonef(k)-f).lt.2.0 .and. abs(tonedt(k)-dt).lt.0.06) go to 10
    enddo
    if(ntones.lt.NTONEMAX) then
       ntones=ntones+1; itones(:,ntones)=itone; tonef(ntones)=f; tonedt(ntones)=dt; tonesw(ntones)=swl
    endif
10  continue
    !$omp end critical(ens_tones)
  end subroutine ens_store_tone

  ! CE3TSK: the slices' buffered tones, merged into the shared store in slice order - called
  ! from the serial region after each slicing pass, so the residual unit's subtraction order
  ! cannot depend on which thread finished first
  subroutine ens_merge_tones(nsl)
    integer, intent(in) :: nsl
    integer :: is,j,k
    do is=1,min(nsl,NSLB)
       do j=1,ntones_b(is)
          do k=1,ntones
             if(abs(tonef(k)-tonef_b(j,is)).lt.2.0 .and. abs(tonedt(k)-tonedt_b(j,is)).lt.0.06) go to 20
          enddo
          if(ntones.lt.NTONEMAX) then
             ntones=ntones+1; itones(:,ntones)=itones_b(:,j,is)
             tonef(ntones)=tonef_b(j,is); tonedt(ntones)=tonedt_b(j,is); tonesw(ntones)=tonesw_b(j,is)
          endif
20        continue
       enddo
    enddo
    ntones_b(1:min(nsl,NSLB))=0
  end subroutine ens_merge_tones

  ! y = x with every stored decode subtracted (subtractft8 works on the module band dd8, so
  ! the master's copy is borrowed for the loop; called outside the parallel region)
  subroutine ens_residual(x,y,n)
    use ft8_mod1, only : dd8
    integer, intent(in) :: n
    real, intent(in) :: x(n)
    real, intent(out) :: y(n)
    integer :: k
    dd8(1:n)=x
    do k=1,ntones
       call subtractft8(itones(:,k),tonef(k),tonedt(k),tonesw(k))
    enddo
    y=dd8(1:n)
  end subroutine ens_residual

  ! ft8_decode records a candidate that reached the LDPC/OSD stage and failed; one entry per
  ! place (3 Hz, 0.08 s), keeping the best sync
  subroutine ens_record_fail(islice,f,dt,sync)
    integer, intent(in) :: islice
    real, intent(in) :: f,dt,sync
    integer :: k,is
    is=max(1,min(NFAILSL,islice))
    do k=1,nfail(is)
       if(abs(failf(k,is)-f).lt.3.0 .and. abs(faildt(k,is)-dt).lt.0.08) then
          if(sync.gt.failsync(k,is)) failsync(k,is)=sync
          return
       endif
    enddo
    if(nfail(is).lt.NFAILMAX) then
       nfail(is)=nfail(is)+1; failf(nfail(is),is)=f; faildt(nfail(is),is)=dt; failsync(nfail(is),is)=sync
    endif
  end subroutine ens_record_fail

  ! the retry unit's candidate list for a slice: its recorded failures, minus those at the
  ! place of a known decode (10 Hz, 0.1 s), strongest sync first
  subroutine ens_retry_candidates(islice,candidate,ncand)
    integer, intent(in) :: islice
    real, intent(out) :: candidate(4,2000)
    integer, intent(out) :: ncand
    integer :: k,j,is,idx(NFAILMAX)
    logical :: known
    real :: s
    is=max(1,min(NFAILSL,islice)); ncand=0; candidate=0.
    ! order by sync, descending (selection sort - the list is short)
    do k=1,nfail(is); idx(k)=k; enddo
    do k=1,nfail(is)-1
       do j=k+1,nfail(is)
          if(failsync(idx(j),is).gt.failsync(idx(k),is)) then; s=idx(k); idx(k)=idx(j); idx(j)=nint(s); endif
       enddo
    enddo
    do k=1,nfail(is)
       known=.false.
       do j=1,ntones
          if(abs(tonef(j)-failf(idx(k),is)).lt.10.0 .and. abs(tonedt(j)-faildt(idx(k),is)).lt.0.1) then
             known=.true.; exit
          endif
       enddo
       if(known) cycle
       ncand=ncand+1
       candidate(1,ncand)=failf(idx(k),is); candidate(2,ncand)=faildt(idx(k),is)
       candidate(3,ncand)=failsync(idx(k),is); candidate(4,ncand)=0.
       if(ncand.ge.2000) exit
    enddo
    !$omp atomic
    nretried=nretried+ncand
  end subroutine ens_retry_candidates

  ! true when the GUI has removed temp_dir/.lock - it wants the next decode now
  logical function bg_lock_gone()
! CE3TSK: the background stops when the GUI wants the next decode (.lock removed) or when
! the operating context changed under it (band or mode change: the GUI creates .bgabort,
! and removes it again at each decode's start) - without the sentinel a band change would
! let the background print the previous band's decodes under the new dial for up to the
! rest of its window
    logical :: lexists,labort
    inquire(file=trim(temp_dir)//'/.lock',exist=lexists)
    inquire(file=trim(temp_dir)//'/.bgabort',exist=labort)
    bg_lock_gone=(.not.lexists) .or. labort
  end function bg_lock_gone

  ! the retry unit: Gaussian dither on the downsampled candidate signal, seeded by the try
  ! and the candidate's frequency, at retrydither x its RMS (3 % unless JTDX_RETRY_DITHER)
  subroutine ens_dither_cd0(cd0,iseed,f1)
    complex, intent(inout) :: cd0(-800:4000)
    integer, intent(in) :: iseed
    real, intent(in) :: f1
    integer(8) :: s
    integer :: i,j,ienv,lenv
    real :: u,v,gain
    real(8) :: sumsq
    character(len=32) :: env
    if(retrydither.lt.0.) then
       retrydither=0.03
       call get_environment_variable('JTDX_RETRY_DITHER',env,lenv,ienv)
       if(ienv.eq.0 .and. lenv.gt.0) read(env(1:lenv),*,iostat=ienv) retrydither
    endif
    sumsq=0.d0
    do i=0,3199; sumsq=sumsq+dble(real(cd0(i)))**2+dble(aimag(cd0(i)))**2; enddo
    gain=retrydither*real(sqrt(sumsq/3200.d0/2.d0))   ! per component
    s=int(iseed,8)*2654435761_8+int(nint(f1*10.),8)*40503_8+88172645463325252_8
    do i=-800,4000
       u=0.; v=0.
       do j=1,12
          s=ieor(s,ishft(s,13)); s=ieor(s,ishft(s,-7)); s=ieor(s,ishft(s,17))
          u=u+real(ishft(s,-40))/16777216.
          s=ieor(s,ishft(s,13)); s=ieor(s,ishft(s,-7)); s=ieor(s,ishft(s,17))
          v=v+real(ishft(s,-40))/16777216.
       enddo
       cd0(i)=cd0(i)+cmplx((u-6.)*gain,(v-6.)*gain)
    enddo
  end subroutine ens_dither_cd0

end module ft8ensemble
