! CE3TSK 2026-11-17 (MSK144_PLAN.md step 0): one recorded period through the real-time decoder, as WSJT-X 3.0.2's
! jt9 -k does it (its lib/decode_msk144.f90, which this is derived from). What is WSJT-X's and kept: the 7168-sample
! window stepped by half, the window's time (position / 12000 with position counted from 1), the three receive
! switches off, no phase equalisation, the line as mskrtd writes it, in 80 columns.
!
! What differs, and why:
!   - the arguments are named ones: WSJT-X hands over its parameter block, and this program's block (jt9com.f90) is
!     another and is left alone - MSK144 is not decoded through it;
!   - a window goes through msk144_window (msk144_window.f90): mskrtd between the two hash steps of msk144_hash.f90,
!     as in the running program;
!   - the three switches are logical*1, as mskrtd declares them (WSJT-X 3.0.2 passed default logicals; 3.3.0-beta6
!     corrected it), and the count of messages starts at zero for every period and is given back - 3.0.2 kept it
!     across files and never printed it (its jt9 fell through into another decoder that reported 0; beta6 62f963b8c);
!   - 2026-11-18: short messages ("<W9XYZ K1ABC> R+03", the main window's Sh) are the caller's to switch on, lshmsg -
!     WSJT-X's jt9 -k has them off and no way to ask for them.

subroutine decode_msk144(audio_samples,ntrperiod,nutc,ntol,nfqso,ndepth,mycall_in,hiscall_in,lshmsg,data_dir,   &
     message_count)

  ! constants
  integer, parameter :: SAMPLING_RATE = 12000
  integer, parameter :: BLOCK_SIZE = 7168
  integer, parameter :: STEP_SIZE = BLOCK_SIZE / 2
  integer, parameter :: CALL_LENGTH = 12

  ! arguments
  integer*2, intent(in) :: audio_samples(*)       !the period's samples, ntrperiod * 12000 of them
  integer, intent(in) :: ntrperiod                !seconds
  integer, intent(in) :: nutc,ntol,nfqso,ndepth
  character(len=*), intent(in) :: mycall_in,hiscall_in
  logical, intent(in) :: lshmsg                   !short messages are decoded too
  character(len=*), intent(in) :: data_dir
  integer, intent(out) :: message_count

  ! parameters of mskrtd
  integer*2 :: buffer(BLOCK_SIZE)
  real :: tsec
  logical*1 :: bshmsg ! enables shorthand messages
  logical*1 :: btrain ! turns on training in MSK144 mode
  real*8 :: pcoeffs(5) ! phase equalization
  logical*1 :: bswl
  character(len = 80) :: line
  character(len = CALL_LENGTH) :: mycall
  character(len = CALL_LENGTH) :: hiscall

  ! local variables
  integer :: sample_count
  integer :: position

  bshmsg = lshmsg
  btrain = .false.
  bswl = .false.
  pcoeffs = 0.d0
  message_count = 0

  ! decode in 0.3s blocks
  sample_count = ntrperiod * SAMPLING_RATE
  mycall = mycall_in
  hiscall = hiscall_in

  do position = 1, sample_count - BLOCK_SIZE + 1, STEP_SIZE
    buffer =  audio_samples(position : position + BLOCK_SIZE - 1)
    tsec = position / REAL(SAMPLING_RATE)

    call msk144_window(buffer, nutc, tsec, ntol, nfqso, ndepth, &
      mycall, hiscall, bshmsg, btrain, pcoeffs, bswl, data_dir, line)

    if (line(1:1) .ne. char(0)) then
      line = line(1:index(line, char(0))-1)
      write(*, 1001) line
      1001 format(a80)
      message_count = message_count + 1
    end if
  end do

  return
end subroutine decode_msk144
