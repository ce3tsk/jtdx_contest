subroutine chkprefix(callsign,lwrongcall)
! CE3TSK 2026-10-12: does the prefix-to-grid table list this call's prefix as not allocated - whatever the
! square.
!
! chkgrid judges a call TOGETHER with a square, and it looks at the call only when somebody can be in the
! square. For a square of open sea, for the far south and north, and for one that begins with a digit it
! returns before the table: it says "check the call" (lchkcall) and nothing about the prefix. The two checks
! that delete on the prefix alone - a directed CQ ("CQ TEST K1ABC FN42") and "CALL1 CALL2 R GRID", both in
! ft8b.f90 - asked it with the message's own square. So "CQ TEST 1Z9ABC FN42" was deleted and
! "CQ TEST 1Z9ABC RR99" was shown: the less likely the square, the weaker the check - about half of all
! squares are of that kind.
!
! Here the table is asked with a square it always looks into, and only its word on the prefix is kept. That
! word does not depend on the square: test/fortran/chkprefix_sweep asks every prefix with every square. (Its
! first run found ten pairs where it did: the table accepted a P0 or P1 call with 0, 5, 6, 7 or 8 as its
! third character in FK87 or FK88, because its rule for PJ lacked a pair of brackets - put in on 2026-10-13.)
!
! What the table lists is SHORT, and it is not the ITU's allocation: C0 C1 C7, E0 E1 E8 E9, H1,
! J1 J9 JB JC JZ and JD2..JD9, L0, O0..O9, P0 P1, S1, T0 T1, V0 V1, X0..X9, Y0 Y1, Z1 Z7 Z9, 1C..1R, 1T..1Z,
! and 2 followed by a digit or by one of B C F G H K L N O P Q R S T V X Y Z. A call beginning with 0 or
! with Q is none of these and passes.
  implicit none
  character, intent(in) :: callsign*12
  logical(1), intent(out) :: lwrongcall
  character grid*12
  logical(1) lchkcall,lgvalid

  grid='JO31        '
  call chkgrid(callsign,grid,lchkcall,lgvalid,lwrongcall)
end subroutine chkprefix

subroutine chkdircq(msg37,i3,iaptype,lhash,lwrongcall)
! CE3TSK 2026-10-13: check 8 of the record (deleted_decodes.f90), in one place for FT8, FT4 and FT2: a
! directed CQ - "CQ TEST K1ABC FN42" - whose call has a prefix the table lists as not allocated.
!
! The check stood in ft8b.f90 alone. FT4 and FT2 had none: "CQ TEST 1Z9ABC FN42" was deleted in FT8 and
! shown in the other two. All three decoders call this routine now, so that the next change to it cannot be
! made in one of them only.
!
! The prefix is asked of what ft8b.f90 looked at: a message decoded without a-priori help, of the standard
! type (i3=1), that reads "CQ", a word of three or four characters, a call and a 4-character grid; for a word
! of one or two characters ("CQ DX", "CQ EU") it is not asked. A call out of the hash table with a grid - unresolved
! (2026-10-14) or resolved (2026-10-15) - is deleted after a word of any length, of types i3=1 and i3=2 (below). The SQUARE is not judged (the operator,
! 2026-10-12: "CQ TEST" is the contest CQ, and a station the table has wrong must not lose it) - whether
! the grid fits the call is the window's question: phantomdecodes.h marks the line and the sequencer does
! not answer it on its own.
  implicit none
  character(len=37), intent(in) :: msg37
  integer, intent(in) :: i3,iaptype
  logical(1), intent(in) :: lhash        ! the call came out of the hash table, resolved or not (the decoder knows it)
  logical(1), intent(out) :: lwrongcall
  character callsign*12
  integer ispc1,ispc2,ispc3,ispc4,jg

  lwrongcall=.false.
  if(iaptype.ne.0 .or. (i3.ne.1 .and. i3.ne.2) .or. msg37(1:3).ne.'CQ ') return
! CE3TSK 2026-10-14 (reviews 3.-67 and 3.-72): a call that arrived as an UNRESOLVED HASH, with a grid, after a CQ word
! of one or two characters too - "CQ DX <...> FN42", "CQ WW <...> FN42", "CQ A <...> FN42" - where until then only a
! word of three or four was looked at ("CQ TEST <...> FN42", "CQ 290 <...> FN42", the prefix rule below). It names no
! station, and no station sends it: a CQ from a call that does not fit the standard field goes out with the call in
! full and no grid ("CQ PJ4/K1ABC"). Random bits read as a 22-bit hash 1 time in 64, and the window cannot mark it. On
! record with the prefixes that are not allocated (prefix/8). Of type i3=2 too - the same text, as unpack77 adds no /P
! to a hash: FT8 deleted it later and off the record (ft8b.f90, "protocol violations": i3=2 must carry /P), FT4 and
! FT2 showed it until 2026-10-14. A plain "CQ <...> GRID" never arrives: unpack77 refuses a message that begins
! "CQ <" (packjt77.f90) - the three words would do no harm.
! CE3TSK 2026-10-15 (TODO 3.-106, item 4 of the review 3.-104): RESOLVED as well. A resolved hash arrives without its
! brackets - ft8b.f90 calls delbraces, ft4b.f90 does the same in its own lines - and passed here as a call like any
! other; but a phantom's random hash resolves whenever a call the decoder knows has it (the live table, and since
! 3.-101 the remembered compound calls), and no station sends a CQ with a hashed call and a grid, resolved or not:
! none in 8.3 million decodes of this station's 2026 ALL.TXT. The decoder says whether the call came out of the hash
! table (lhash: ft8b's lcall2hash, ft4b's lhash2); an unresolved one is known by its text as well.
  if(lhash .or. index(msg37,' <...> ').gt.0) then
     ispc2=index(msg37(4:),' ')+3; ispc3=index(msg37(ispc2+1:),' ')+ispc2; ispc4=index(msg37(ispc3+1:),' ')+ispc3
     jg=0
     if(ispc4-ispc3.eq.5 .and. msg37(ispc4:).eq.' ') jg=ispc3+1          ! "CQ WORD CALL GRID"
     if(ispc3-ispc2.eq.5 .and. msg37(ispc3:).eq.' ') jg=ispc2+1          ! "CQ CALL GRID"
     if(jg.gt.0) then
        if(msg37(jg:jg).gt.'@' .and. msg37(jg:jg).lt.'S' .and. msg37(jg+1:jg+1).gt.'@' .and.                 &
           msg37(jg+1:jg+1).lt.'S' .and. msg37(jg+2:jg+2).ge.'0' .and. msg37(jg+2:jg+2).le.'9' .and.          &
           msg37(jg+3:jg+3).ge.'0' .and. msg37(jg+3:jg+3).le.'9') then
           lwrongcall=.true.; return
        endif
     endif
  endif
  if(i3.ne.1) return                       ! the prefix: the standard message alone, as before
  ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1
  if((ispc2-ispc1).ne.4 .and. (ispc2-ispc1).ne.5) return
  ispc3=index(msg37((ispc2+1):),' ')+ispc2; ispc4=index(msg37((ispc3+1):),' ')+ispc3
  if((ispc3-ispc2).le.3 .or. (ispc4-ispc3).ne.5) return
  if(.not.(msg37(ispc3+1:ispc3+1).gt.'@' .and. msg37(ispc3+1:ispc3+1).lt.'S' .and. &
           msg37(ispc3+2:ispc3+2).gt.'@' .and. msg37(ispc3+2:ispc3+2).lt.'S' .and. &
           msg37(ispc3+3:ispc3+3).lt.':' .and. msg37(ispc3+4:ispc3+4).lt.':')) return
  callsign=''; callsign=msg37(ispc2+1:ispc3-1)
! (a call out of the hash table, resolved or not, is deleted above with its grid - only a call sent in full reaches
!  this)
  call chkprefix(callsign,lwrongcall)
end subroutine chkdircq

subroutine chkrgrid(msg37,i3,nwhy)
! CE3TSK 2026-10-13: check 9 of the record, in one place for FT8, FT4 and FT2: "CALL1 CALL2 R GRID", the
! roger of a WW Digi or ARRL Digi QSO. As in ft8b.f90 since 2026-09-30, only two things delete it:
!     nwhy =  1   what follows the R is not a 4-character grid, two letters A-R and two digits (the
!                 unpacker writes nothing else there, so this is a guard; it is not put on record)
!     nwhy = -9   the prefix of CALL2 is one the table lists as not allocated (on record, place 9)
! and nwhy = 0 says the message stays. The square is NOT judged: a station sends this from its contest
! mode wherever it is, and one away from home lost its roger - and the QSO - to the table.
!
! FT4 and FT2 had no such check; all three decoders call this routine now. The caller keeps its own test
! of a CALL2 that came out of the hash table (ft8b's lcall2hash) and does not call here for one.
  implicit none
  character(len=37), intent(in) :: msg37
  integer, intent(in) :: i3
  integer, intent(out) :: nwhy
  character call_b*12
  logical(1) lwrongcall
  integer ispc1,ispc2,ispc3,ispc4

  nwhy=0
  if(i3.ne.1 .and. i3.ne.2) return
  if(index(msg37,' R ').le.0) return
  ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1; ispc3=index(msg37((ispc2+1):),' ')+ispc2
  if(ispc3-ispc2.ne.2) return
  if(msg37(ispc2:ispc3).ne.' R ') return
  call_b=''
  if((i3.eq.1 .and. msg37(ispc2-2:ispc2-1).eq.'/R') .or. (i3.eq.2 .and. msg37(ispc2-2:ispc2-1).eq.'/P')) then
    call_b=msg37(ispc1+1:ispc2-3)
  else
    call_b=msg37(ispc1+1:ispc2-1)
  endif
  if(len_trim(call_b).le.2) return
  ispc4=index(msg37((ispc3+1):),' ')+ispc3
  if(.not.(ispc4-ispc3.eq.5 .and. &
           msg37(ispc3+1:ispc3+1).ge.'A' .and. msg37(ispc3+1:ispc3+1).le.'R' .and. &
           msg37(ispc3+2:ispc3+2).ge.'A' .and. msg37(ispc3+2:ispc3+2).le.'R' .and. &
           msg37(ispc3+3:ispc3+3).ge.'0' .and. msg37(ispc3+3:ispc3+3).le.'9' .and. &
           msg37(ispc3+4:ispc3+4).ge.'0' .and. msg37(ispc3+4:ispc3+4).le.'9')) then
    nwhy=1; return
  endif
  call chkprefix(call_b,lwrongcall)
  if(lwrongcall) nwhy=-9
end subroutine chkrgrid
