! CE3TSK 2026-11-17 (MSK144_PLAN.md step 0): what stands between WSJT-X's MSK144 routines and this program's
! packjt77.
!
! In WSJT-X unpack77 does two things by itself, on every call: it notices a changed own call or DX call and hashes
! it, and it puts the calls of the message it has just unpacked into the hash tables, so that the next message can
! name them by their hash. This program's packjt77 does neither inside unpack77: the first is fillhash (.false.),
! the second is a list per decoding slice that fillhash (.true.) merges into the tables - the FT8 and FT4 decoders
! unpack from many threads and merge once they are done (decoder.f90).
!
! mskrtd is WSJT-X's own text and knows nothing of this, so its caller does the two around every window, as
! decoder.f90's superfox_slot does around a SuperFox slot:
!
!     call msk144_hash_begin (mycall,hiscall)
!     call mskrtd (...)
!     call msk144_hash_commit ()
!
! The slice is 1 (msk144decodeframe.f90). One window at a time, one caller: mskrtd keeps its own state between
! windows and is not re-entrant.

subroutine msk144_hash_begin(mycall,hiscall)

  use ft8_mod1, only : mycall12=>mycall, hiscall12=>hiscall
  character*(*), intent(in) :: mycall,hiscall

  mycall12=mycall          !fillhash reads the two calls from ft8_mod1
  hiscall12=hiscall
  call fillhash(1,.false.) !the slices' lists emptied; my call and the DX call made known, with their hashes
  call fillhash(1,.true.)  !and a DX call that was typed is resolvable at once, as in WSJT-X

  return
end subroutine msk144_hash_begin

subroutine msk144_hash_commit()

  call fillhash(1,.true.)  !the calls of the window's messages into the tables

  return
end subroutine msk144_hash_commit
