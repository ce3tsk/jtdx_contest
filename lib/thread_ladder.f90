module thread_ladder
! CE3TSK: the decoder's thread count from the operator's setting and the core count - ONE
! copy (2026-09-02, DECODER_IMPROVEMENTS item 63). It used to exist three times: JTDX's
! ladder inside decoder.f90's FT8 branch, a hand copy in the FT4 branch (the FT8 one sits
! inside its branch, so the variable was still zero when FT4 ran), and auto_ft8_threads() in
! decodepreset.h, which the GUI needs before the decoder runs to size a preset's member
! count. The C++ copy stays - it is the GUI's - and test/decode/params_layout.sh pins it
! against this module over every setting and core count, as it pins the params block.
!
! The rule: 0 (auto) leaves a few cores free - 1 core gives 1 thread, up to 4 cores leave one
! free, up to 8 two, up to 15 three, up to 20 four, up to 29 five, and 30 or more are capped
! at 24. An explicit setting is used as it is, clamped to the core count. (JTDX's FT8 copy
! also fell back to ONE thread for a setting of 25 or more; the GUI never sends one - it
! clamps to 0..24 in readSettings - and file mode's -j now clamps to the cores as the FT4
! copy and the C++ already did.)
  implicit none
  private
  public :: auto_threads, decoder_threads, ft4_members_auto, ft4_bg_auto
contains
  pure integer function auto_threads(ncores)
    integer, intent(in) :: ncores
    if(ncores.le.1) then; auto_threads=1
    else if(ncores.lt.5) then; auto_threads=ncores-1
    else if(ncores.lt.9) then; auto_threads=ncores-2
    else if(ncores.lt.16) then; auto_threads=ncores-3
    else if(ncores.lt.21) then; auto_threads=ncores-4
    else if(ncores.lt.30) then; auto_threads=ncores-5
    else; auto_threads=24
    endif
  end function auto_threads

  pure integer function decoder_threads(nuser,ncores)
    integer, intent(in) :: nuser,ncores
    if(nuser.gt.0) then; decoder_threads=max(1,min(nuser,ncores))
    else; decoder_threads=auto_threads(ncores)
    endif
  end function decoder_threads

  ! CE3TSK item 73: FT4 ensemble members by thread count - "auto" in the RX and TX background
  ! ensemble menus (FT8's ensemble_members() is 3 from 12 threads, 2 from 6, 1 from 3). The
  ! FT4 counts were calibrated at 12 threads (6 members at reply time = 0.57 s mean, 1.29 s
  ! worst, against the 1360 ms deadline; DECODER_IMPROVEMENTS item 67) and a member's cost
  ! scales with the thread count, so the ladder halves with it. The background is idle time:
  ! it keeps at least 3 from 3 threads and leaves the clock (item 73) to cut what does not fit.
  ! decodepreset.h carries the C++ copy; params_layout.sh pins the two against each other.
  pure integer function ft4_members_auto(nthreads)
    integer, intent(in) :: nthreads
    if(nthreads.ge.12) then; ft4_members_auto=6
    else if(nthreads.ge.8) then; ft4_members_auto=4
    else if(nthreads.ge.6) then; ft4_members_auto=3
    else if(nthreads.ge.4) then; ft4_members_auto=2
    else if(nthreads.ge.3) then; ft4_members_auto=1
    else; ft4_members_auto=0
    endif
  end function ft4_members_auto

  pure integer function ft4_bg_auto(nthreads)
    integer, intent(in) :: nthreads
    if(nthreads.ge.3) then; ft4_bg_auto=max(3,ft4_members_auto(nthreads))
    else; ft4_bg_auto=0
    endif
  end function ft4_bg_auto
end module thread_ladder
