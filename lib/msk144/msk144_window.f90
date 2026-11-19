! CE3TSK 2026-11-17 (MSK144_PLAN.md step 1a): one window through the MSK144 decoder - the one place where a window is
! decoded, for the command-line decoder (decode_msk144.f90) and for the running program (msk144decoder.cpp) alike.
!
! The 7168 samples of a window, the period's time, the moment of the window in the period (tsec), and what the
! operator has set: F Tol, the receive frequency, the depth, my call, the DX call, short messages, SWL, a phase
! measurement under way, the phase coefficients, the directory a measurement's file goes to. Back comes a line
! ("hhmmss dB T Freq &  message", ended by a NUL) or, when nothing was decoded, a NUL in its first place. btrain is
! given back cleared when a measurement has ended.
!
! mskrtd is WSJT-X's own text; the two hash steps around it are what this program's packjt77 needs (msk144_hash.f90).
! One caller at a time, the windows in their order: mskrtd keeps the noise level, the last message and the recent
! calls from one window to the next.

subroutine msk144_window(id2,nutc,tsec,ntol,nrxfreq,ndepth,mycall,hiscall,bshmsg,btrain,pcoeffs,bswl,datadir,line)

  integer*2, intent(in) :: id2(7168)
  integer, intent(in) :: nutc,ntol,nrxfreq,ndepth
  real, intent(in) :: tsec
  character(len=*), intent(in) :: mycall,hiscall,datadir
  logical*1, intent(in) :: bshmsg,bswl
  logical*1, intent(inout) :: btrain
  real*8, intent(in) :: pcoeffs(5)
  character(len=*), intent(out) :: line
  character(len=12) :: mycall12,hiscall12      !mskrtd reads twelve characters of each

  mycall12=mycall
  hiscall12=hiscall
  call msk144_hash_begin(mycall12,hiscall12)
  call mskrtd(id2,nutc,tsec,ntol,nrxfreq,ndepth,mycall12,hiscall12,bshmsg,btrain,pcoeffs,bswl,datadir,line)
  call msk144_hash_commit()

  return
end subroutine msk144_window

! The running program's decoder is this process's own (lib/msk144 linked into jtdx), not jtdxjt9: what jtdxjt9 keeps
! in files - the remembered compound calls, the feed's list - is jtdxjt9's, and this process must neither read nor
! write it (packjt77's lremember is .true. until told otherwise, and prog_args' data_dir is "." here). Called once,
! before the first window.
subroutine msk144_in_program()

  use packjt77, only : lremember

  lremember=.false.

  return
end subroutine msk144_in_program
