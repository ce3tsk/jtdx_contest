subroutine chkfalse8(msg37,i3,n3,nbadcrc,iaptype,lcall1hash,ngriddel)

  use ft8_mod1, only : mycall,hiscall,hisgrid4
  character msg37*37,decoded*22,callsign*12,calltmp*12,call_a*12,call_b*12,grid*12,callmask6*6
  character*6 mask6(3)
  integer, intent(in) :: i3,n3
  logical(1) falsedec,lchkcall,lgvalid,lwrongcall
  character word4*4                ! CE3TSK 2026-11-08 (TODO 3.-241): a third word of four characters
  integer irpt                     ! ... the number of an R+report in it
  logical(1) lreply                ! ... an R+report or RR73 (2026-11-09, TODO 3.-243)
  logical(1), intent(in) :: lcall1hash
! CE3TSK 2026-10-12: why the message was deleted, when it was the prefix-to-grid table (chkgrid) that deleted
! it: the number of the place below, 1 to 7 in the order of this file - positive when the square is not in the
! table for the prefix, negative when the prefix is not allocated - and 0 for every other outcome. The callers
! (ft8b, ft4b) hand it to deleted_decodes.f90, which has the story.
  integer, intent(out) :: ngriddel
  data mask6/'001000','101000','011000'/

  ngriddel=0
  call_a=''; call_b=''
! iaptype=0 non-AP decoder
! iaptype=1 CQ message AP decoder

! i3 n3                                      Bits               Total  Message type
! ---------------------------------------------------------------------------------------
! 0   0 FREE TEXT MSG                        71                   71
! 0   1 K1ABC RR73; W9XYZ <KH1/KH7Z> -12     28 28 10 5           71   DXpedition Mode
! 0   2 PA3XYZ/P R 590003 IO91NP             28 1 1 3 12 25       70   EU VHF contest (2)
! 0   2 PA3XYZ 520093 IO91NP                 28 1 1 3 12 25       70   EU VHF contest (2)
! 0   3 WA9XYZ KA1ABC R 16A EMA              28 28 1 4 3 7        71   ARRL Field Day
! 0   3 WA9XYZ KA1ABC 7D EMA                 28 28 1 4 3 7        71   ARRL Field Day
! 0   3 WA9XYZ G8ABC 1D DX                   28 28 1 4 3 7        71   ARRL Field Day
! 0   4 WA9XYZ KA1ABC R 32A EMA              28 28 1 4 3 7        71   ARRL Field Day
! 0   5 123456789ABCDEF012                   71                   71   Telemetry (18 hex)
! 0   5 7123456789ABCDEF01                   71                   71   Telemetry (18 hex)
! 0   5 71234567                             71                   71   Telemetry (18 hex)
! 0   5 81234567                             71                   71   Telemetry (18 hex)
! 0   5 8123456789ABCDEF01                   71                   71   Telemetry (18 hex)
! 1     WA9XYZ/R KA1ABC/R R FN42             28 1 28 1 1 15       74   Standard msg, with std or non std callsign
! 1     WA9XYZ KA1ABC R-11                   28 1 28 1 1 15       74   Standard msg, with std or non std callsign
! 1     CQ 039 P30KJE/R DR46
! 2     PA1XYZ/P GM4ABC/P R FN42             28 1 28 1 1 15       74   EU VHF Contest
! 2     CQ 039 P30KJE/P DR46
! 3     TU; W9XYZ K1ABC R 579 MA             1 28 28 1 3 13       74   ARRL RTTY contest
! 3     TU; W9XYZ G8ABC R 559 0013           1 28 28 1 3 13       74   ARRL RTTY (DX)
! 4     <WA9XYZ> PJ4/KA1ABC RR73             13 58 1 2            74   Nonstandard call
! 4     <WA9XYZ> PJ4/KA1ABC                  13 58 1 2            74   Nonstandard call
! 4     PJ4/KA1ABC <WA9XYZ> RRR              13 58 1 2            74   Nonstandard call
! 4     CQ ZW5STAYHOME                       13 58 1 2            74   Nonstandard call

!print *,iaptype
!print *,i3,n3,msg37

! 'CQ 5NGX46MFKUE' 'CQ 53HDFKJEASD' 'CQ HDF53KJEASD'  i3=4 n3=1 
! -23  0.3 1482 ~ CQ EQ3YCI/KVNA  i3=4 n3=1 
! -23 -1.3 CQ P28Z58W/77M  i3=4 n3=1
  if(msg37(1:3).eq.'CQ ') then
    callsign=''; ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1
    if(i3.eq.4) then
      islash=index(msg37,'/')
      if(islash.le.0) then
        callsign=trim(msg37(ispc1+1:)); nlencall=len_trim(callsign); if(nlencall.lt.1) return; ispccall=index(callsign,' ')
! CQ 9D6T0GA4 Z space inside callsign of any length: protocol violation, or last char is digit
        if(ispccall.lt.nlencall .or. callsign(nlencall:nlencall).lt.':') then; nbadcrc=1; msg37=''; return; endif
      else
        if(islash.gt.ispc1 .and. islash.lt.ispc2) then
          if(islash-ispc1.le.ispc2-islash) then; callsign=msg37(islash+1:ispc2-1)
          else; callsign=msg37(ispc1+1:islash-1)
          endif
        endif
      endif
      include 'callsign_q.f90'
      if(islash.gt.0) then
        falsedec=.false.; call chkflscall('CQ          ',callsign,falsedec)
        if(falsedec) then; nbadcrc=1; msg37=''; return; endif
      else
        nlen=len_trim(callsign)
        if(nlen.ge.10) then
          falsedec=.false.
          call chklong8(callsign,falsedec)
          if(falsedec) then; nbadcrc=1; msg37=''; return; endif
        else if(nlen.eq.6 .and. index(callsign,'/').lt.1) then ! 'CQ VK6ZRW'  protocol violation, i3 for stdcall shall be equal to 1
          do i=1,6
            if(callsign(i:i).gt.'/' .and. callsign(i:i).lt.':') then; callmask6(i:i)='1'; else; callmask6(i:i)='0'; endif
          enddo
          falsedec=.false.
          do i=1,3
            if(callmask6.eq.mask6(i)) then; falsedec=.true.; exit; endif ! standard callsign, false decode
          enddo
          if(falsedec) then; nbadcrc=1; msg37=''; return; endif
        endif
      endif

! i=1 AP mask iaptype 1 false decodes
! -23  0.9 2630 ~ CQ P5RXL MK92 *
! -24  0.3  550 ~ CQ DZ0BIL/R AD94 *
!  -7  0.2  400 ~ CQ LI9NAI/R CF97 * in some noisy setups such decodes possible with a high SNR
    else if(iaptype.eq.1) then
      ispc3=index(msg37((ispc2+1):),' ')+ispc2
      if(msg37(ispc2-2:ispc2-1).eq.'/R' .or. msg37(ispc2-2:ispc2-1).eq.'/P') then; callsign=msg37(ispc1+1:ispc2-3)
      else; callsign=msg37(ispc1+1:ispc2-1)
      endif
      include 'callsign_q.f90'
      grid=''
      if(ispc3-ispc2.eq.5 .and. msg37(ispc2+1:ispc2+1).gt.'@' .and. msg37(ispc2+1:ispc2+1).lt.'S' .and. &
         msg37(ispc2+2:ispc2+2).gt.'@' .and. msg37(ispc2+2:ispc2+2).lt.'S' .and. &
         msg37(ispc2+3:ispc2+3).lt.':' .and. msg37(ispc2+4:ispc2+4).lt.':') grid=msg37(ispc2+1:ispc3-1)
      if(len_trim(grid).eq.4) then
        call chkgrid(callsign,grid,lchkcall,lgvalid,lwrongcall)
        if(lwrongcall .or. .not.lgvalid) then
          ngriddel=merge(-1,1,lwrongcall); nbadcrc=1; msg37=''; return
        endif
      endif
    endif

    if(i3.eq.4) then
! CQ Y09Q7/SS60M  i3=4 n3=1
      callsign=''; callsign=trim(msg37(4:)); islash=index(callsign,'/'); islash2=index(callsign(islash+1:),'/')
      nlencall=len_trim(callsign)
      if(nlencall.eq.11 .and. islash.gt.0 .and. islash2.lt.1) then
        if(islash.lt.7) then
          if((callsign(islash+1:islash+1).lt.':' .and. callsign(islash+2:islash+2).lt.':') .or. &
             callsign(islash+1:islash+1).eq.'Q' .or. callsign(nlencall:nlencall).lt.':') then
            nbadcrc=1; msg37=''; return
          endif
        endif
        if(islash.gt.5) then
          if(callsign(islash-1:islash-1).lt.':' .or. (callsign(1:1).lt.':' .and. callsign(2:2).lt.':') .or. &
             callsign(1:1).eq.'Q') then
            nbadcrc=1; msg37=''; return
          endif
        endif
        if(islash.gt.4 .and. islash.lt.8) then
          call_a=''; call_b=''; call_a=callsign(1:islash-1); call_b=callsign(islash+1:)
          falsedec=.false.
          call chkflscall(call_a,call_b,falsedec)
          if(falsedec) then; nbadcrc=1; msg37=''; return; endif
        endif
      endif
    endif

! CQ 039 P30KJE/R DR46 i3=1 n3=5
! CQ 039 P30KJE/P DR46 i3=2 n3=5
    if(msg37(4:4).lt.':' .and. msg37(4:4).gt.'/' .and. msg37(7:7).eq.' ' .and. (i3.eq.1 .or. i3.eq.2)) then
      islash=index(msg37,'/')
      if(islash.gt.10 .and. islash.lt.15) then
        callsign=''; grid=''; callsign=msg37(8:islash-1); grid=msg37(islash+3:islash+6); nlengrid=len_trim(grid)
        if(nlengrid.eq.4 .and. grid(1:1).gt.'@' .and. grid(4:4).lt.':') then
          call chkgrid(callsign,grid,lchkcall,lgvalid,lwrongcall)
          if(lwrongcall .or. .not.lgvalid) then
            ngriddel=merge(-2,2,lwrongcall); nbadcrc=1; msg37=''; return
          endif
        endif
        falsedec=.false.
        call chkflscall('MYCALL      ',callsign,falsedec)
        if(falsedec) then; nbadcrc=1; msg37=''; return; endif
      endif
    endif

! CQ CJ3OPE/R FP61 i3=1
! CQ KU3XIK NN41
    if(i3.eq.1 .and. iaptype.eq.0) then
      callsign=''; grid=''
      islash=index(msg37,'/R ')
      if(islash.gt.6) then
! CE3TSK 2026-10-13: the rover's call is the LAST word before "/R". Everything between "CQ " and "/R" was
! taken for it, so "CQ TEST K1ABC/R FN42" - a rover's CQ in a VHF contest - was judged as a call
! "TEST K1ABC": the table read TE as Costa Rica, found FN42 outside it, and a weak one was deleted
! ("grid/3" in the record); "CQ DX K1ABC/R FN42" went the same way, as a Philippine call. The plain
! "CQ K1ABC/R FN42" is judged as before. The square of a DIRECTED rover CQ is not judged here - no more
! than the square of "CQ TEST K1ABC FN42", which this routine hands on a few lines below: whether the grid
! fits the call is the window's question, and a prefix that is not allocated is check 8's (chkdircq, in
! all three modes) when the word has three or four characters. A directed rover CQ with a word of one or
! two ("CQ DX K1ABC/R FN42") is judged by nothing, as "CQ DX K1ABC FN42" never was (corrected 2026-10-14:
! this said check 8 took them all; until 2026-10-13 such a CQ was deleted through the fault above).
        icall=index(msg37(1:islash-1),' ',back=.true.)+1
        callsign=msg37(icall:islash-1)
        if(icall.eq.4) grid=msg37(islash+3:islash+6)
      else if(islash.lt.1) then
        ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1; ispc3=index(msg37((ispc2+1):),' ')+ispc2
        if(len_trim(msg37).eq.ispc3-1) then
          callsign=msg37(ispc1+1:ispc2-1); grid=msg37(ispc2+1:ispc3-1)
        else
          return ! 4-word message, will be processed in ft8b.f90
        endif
      endif
      nlengrid=len_trim(grid)
      if(nlengrid.eq.4 .and. grid(1:1).gt.'@' .and. grid(4:4).lt.':') then
        call chkgrid(callsign,grid,lchkcall,lgvalid,lwrongcall)
        if(lwrongcall .or. .not.lgvalid) then
          ngriddel=merge(-3,3,lwrongcall); nbadcrc=1; msg37=''; return
        endif
      endif
      if(islash.gt.6) then
        falsedec=.false.
        call chkflscall('MYCALL      ',callsign,falsedec)
        if(falsedec) then; nbadcrc=1; msg37=''; return; endif
      endif
    endif

    return ! prevent further checking if message starts from 'CQ '
! TO DO via patterns: '175715 -18 -0.1 2519 ~ CQ P2PSR6UWBHS'
  endif

  if(iaptype.eq.2) then ! message starts with user's callsign, 2nd callsign is random
! ES1JA JA1QZQ/R R GD47
! UA3ALE A70DDN/R OF23
! ES6DO NH4CXV MA87
! EA1AHY PW1BSL R GR47 i3=1
! ES6DO ST0VQA R174 invalid message, can not be transmitted
    ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1
    callsign=''; grid=''
    callsign=msg37(ispc1+1:ispc2-1)
    islash=index(callsign,'/R')
    if(islash.gt.3) then
      calltmp='            '; calltmp=callsign(1:islash-1); callsign=calltmp
    endif
    include 'callsign_q.f90'
    ispc3=index(msg37((ispc2+1):),' ')+ispc2
    indxr=index(msg37,' R ')
    if(indxr.gt.7) then
      ispc4=index(msg37((ispc3+1):),' ')+ispc3
      if(ispc4-ispc3.eq.5) then
        if(msg37(ispc3+1:ispc3+1).gt.'@' .and. msg37(ispc3+1:ispc3+1).lt.'S' .and. &
           msg37(ispc3+2:ispc3+2).gt.'@' .and. msg37(ispc3+2:ispc3+2).lt.'S' .and. &
           msg37(ispc3+3:ispc3+3).lt.':' .and. msg37(ispc3+4:ispc3+4).lt.':') then
          grid=msg37(ispc3+1:ispc4-1)
        else ! invalid message
          nbadcrc=1; msg37=''; return
        endif
      endif
    else if(indxr.lt.1) then
      if(ispc3-ispc2.eq.5) then
! CE3TSK 2026-11-08 (TODO 3.-241): the third word of four characters was taken for a square, and anything not shaped like
! one for an invalid message. Two words a station sends are not what that makes of them: the R+report ("R-12",
! "R+05"; pack77 sends -50 to +50) and RR73 - which pack77 does send as the square RR73, open Arctic Ocean (83 N,
! 175 E) where no station is, so the word is always the acknowledgment. Both were deleted here - "R-12" as invalid,
! "RR73" judged against the squares of the prefix - so FT4 and FT2, which try this type before type 3 while they wait
! for a report or an R-report, threw away the DX station's R-report, and the duplicate list then kept the next pass
! from decoding it again (measured: LATE_START_AP.md, F1). From the station being worked they stand as they are.
! 2026-11-09 (TODO 3.-243, the operator's decision on the review's points 4 and 7): from any other call they are judged
! as the roger with a square is here - the prefix alone (chkprefix), on record as prefix/4 when it is not allocated.
! The R+report was deleted as "invalid", off the record (the rule was written for "R174", which unpack77 no longer
! makes), and RR73 judged as a square of open sea, on record as grid/4; both are real messages to me - a late
! R-report after the DX call has moved on - and the plain report from any call has always passed this place. The
! sequencer takes either only from a station it has sent a call or a report to (qsohistory.cpp, RRREPORT and RRR73),
! so a phantom of them is shown but answers nothing. An unresolved hash names no station: deleted, off the record.
        word4=msg37(ispc2+1:ispc3-1)
        lreply=word4.eq.'RR73'
        if(word4(1:1).eq.'R' .and. (word4(2:2).eq.'-' .or. word4(2:2).eq.'+') .and. &
           word4(3:3).ge.'0' .and. word4(3:3).le.'9' .and. word4(4:4).ge.'0' .and. word4(4:4).le.'9') then
          irpt=10*(ichar(word4(3:3))-ichar('0'))+ichar(word4(4:4))-ichar('0')
          lreply=irpt.le.50   ! the review of 2026-11-08: +50 is sent too (it stopped at +49)
        endif
        if(lreply) then
          if(fromhis(msg37(ispc1+1:ispc2-1))) return
          if(callsign(1:5).eq.'<...>') then; nbadcrc=1; msg37=''; return; endif
          call chkprefix(callsign,lwrongcall)
          if(lwrongcall) then; ngriddel=-4; nbadcrc=1; msg37=''; endif
          return
        endif
        if(msg37(ispc2+1:ispc2+1).gt.'@' .and. msg37(ispc2+1:ispc2+1).lt.'S' .and. &
           msg37(ispc2+2:ispc2+2).gt.'@' .and. msg37(ispc2+2:ispc2+2).lt.'S' .and. &
           msg37(ispc2+3:ispc2+3).lt.':' .and. msg37(ispc2+4:ispc2+4).lt.':') then
          grid=msg37(ispc2+1:ispc3-1)
        else ! invalid message
          nbadcrc=1; msg37=''; return
        endif
      endif
    endif
    if(len_trim(grid).eq.4) then
! CE3TSK 2026-10-13: "MYCALL CALL2 R GRID" - the roger of a WW Digi or ARRL Digi QSO, sent to this station.
! Decoded without a-priori help it has reached the program whatever its grid since 2026-09-30 (ft8b.f90,
! check 9); decoded WITH it - the weak one, the one that needs the help - it was still deleted here when the
! table did not hold the square for the call's prefix: a station away from home, or one the table has
! wrong. Now the square of the roger is not judged here or in place 5, where the second call is anybody's:
! the window marks a grid that does not fit the call, and a marked caller is not answered on its own. A
! prefix that is not allocated still deletes it - asked of the table about the prefix alone (chkprefix), so
! that the answer does not depend on the square. "MYCALL CALL2 GRID", the first call of a station, is judged
! as before - and so is the roger of the station being worked, in place 6, for the reason given there.
! CE3TSK 2026-10-14: and so is a second call that arrived as an UNRESOLVED HASH ("MYCALL <...> R GRID", review
! of 2026-10-14): it names no station and the window cannot mark it, while 1 in 64 phantoms of this a-priori
! type reads its free bits as a 22-bit hash. A real station that answers me with a non-standard call sent me
! that call in full first, and its hash then resolves - and a resolved hash arrives here without its brackets
! (delbraces in ft8b.f90, the same lines in ft4b.f90): judged as any call, its prefix alone (review 3.-72). chkgrid
! knows no "<" and deletes the unresolved one, as until 2026-10-13.
      if(indxr.gt.7 .and. callsign(1:5).ne.'<...>') then
        call chkprefix(callsign,lwrongcall); lgvalid=.true.
      else
        call chkgrid(callsign,grid,lchkcall,lgvalid,lwrongcall)
      endif
      if(lwrongcall .or. .not.lgvalid) then
        ngriddel=merge(-4,4,lwrongcall); nbadcrc=1; msg37=''; return
      endif
    endif
!      call chkflscall('MYCALL      ',callsign,falsedec) ! too high risk to eliminate correct decode if callsign missed in ALLCALL7
!      if(falsedec) then; nbadcrc=1; msg37=''; return; endif
    return ! no need in other checks
  endif

  if(iaptype.eq.40) then ! AP mask '<mycall> ??? ???'
! JH4PUL/3 BG4NSA RQ43 * reported false decodes
! JH4PUL/3 M06FLK R AE10 * at least it is possible to transmit the message
    ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1
    callsign=''; grid=''
    callsign=msg37(ispc1+1:ispc2-1)
    include 'callsign_q.f90'
    ispc3=index(msg37((ispc2+1):),' ')+ispc2
    indxr=index(msg37,' R ')
    if(indxr.gt.7) then
      ispc4=index(msg37((ispc3+1):),' ')+ispc3
      if(ispc4-ispc3.eq.5 .and. msg37(ispc3+1:ispc3+1).gt.'@' .and. msg37(ispc3+1:ispc3+1).lt.'S' .and. &
         msg37(ispc3+2:ispc3+2).gt.'@' .and. msg37(ispc3+2:ispc3+2).lt.'S' .and. &
         msg37(ispc3+3:ispc3+3).lt.':' .and. msg37(ispc3+4:ispc3+4).lt.':') grid=msg37(ispc3+1:ispc4-1)
    else if(indxr.lt.1) then
      if(ispc3-ispc2.eq.5 .and. msg37(ispc2+1:ispc2+1).gt.'@' .and. msg37(ispc2+1:ispc2+1).lt.'S' .and. &
         msg37(ispc2+2:ispc2+2).gt.'@' .and. msg37(ispc2+2:ispc2+2).lt.'S' .and. &
         msg37(ispc2+3:ispc2+3).lt.':' .and. msg37(ispc2+4:ispc2+4).lt.':') grid=msg37(ispc2+1:ispc3-1)
    endif
    ! CE3TSK 2026-11-08 (TODO 3.-241): his RR73 - the square of open sea pack77 sends for the word - stands from the DX
    ! call, as in place 4; after an R (no program sends "R RR73") it is judged as before (the review of 2026-11-08).
    ! 2026-11-09 (TODO 3.-243): from any other call it is judged as the roger with a square is here, the prefix alone
    ! (it was judged as a square, on record as grid/5); an unresolved hash names no station - deleted, off the record.
    if(indxr.lt.1 .and. grid.eq.'RR73') then
      if(fromhis(msg37(ispc1+1:ispc2-1))) return
      if(callsign(1:5).eq.'<...>') then; nbadcrc=1; msg37=''; return; endif
      call chkprefix(callsign,lwrongcall)
      if(lwrongcall) then; ngriddel=-5; nbadcrc=1; msg37=''; endif
      return
    endif
    if(len_trim(grid).eq.4) then
      if(indxr.gt.7 .and. callsign(1:5).ne.'<...>') then   ! CE3TSK 2026-10-13: the roger's square is not judged, and 2026-10-14 an unresolved hash is - see place 4 above
        call chkprefix(callsign,lwrongcall); lgvalid=.true.
      else
        call chkgrid(callsign,grid,lchkcall,lgvalid,lwrongcall)
      endif
      if(lwrongcall .or. .not.lgvalid) then
        ngriddel=merge(-5,5,lwrongcall); nbadcrc=1; msg37=''; return
      endif
    endif
    return ! no need in other checks for iaptype 40
  endif

! RV6ARZ CX7CO R NI25 ! mycall hiscall ??? AP mask false decode, both callsigns are correct, checking for valid grid
! ES6DO TK60CNES R MJ79 ! non standard DXCall, iaptype 11
! UA0ZZZ AA1AAA/1 R GC45 ! non standard DXCall, iaptype 11
! JH4PUL/3 HB9CEX R PN76 * ! non standard mycall, iaptype 41
! JH4PUL/3 JT1DN HB30 * ! non standard mycall, iaptype 41
  if(iaptype.eq.3 .or. iaptype.eq.11 .or. iaptype.eq.21 .or. iaptype.eq.41) then
    indxr=index(msg37,' R ')
    if(iaptype.eq.21 .and. indxr.gt.0) then; nbadcrc=1; msg37=''; return; endif ! not valid message in Hound mode
    islash1=index(msg37,'/'); ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1
    if(ispc1.gt.3 .and. ispc2.gt.7) then
      if(islash1.lt.ispc1 .or. (islash1.eq.ispc2-2 .and. msg37(ispc2-1:ispc2-1).lt.':' )) then ! AA1AAA/1 can be first or 2nd callsign
        if(msg37(1:ispc1-1).eq.mycall .and. msg37(ispc1+1:ispc2-1).eq.hiscall) then
          ispc3=index(msg37((ispc2+1):),' ')+ispc2; grid=''
          if(indxr.gt.7) then
            ispc4=index(msg37((ispc3+1):),' ')+ispc3
            if(ispc4-ispc3.eq.5 .and. msg37(ispc3+1:ispc3+1).gt.'@' .and. msg37(ispc3+1:ispc3+1).lt.'S' .and. &
               msg37(ispc3+2:ispc3+2).gt.'@' .and. msg37(ispc3+2:ispc3+2).lt.'S' .and. &
               msg37(ispc3+3:ispc3+3).lt.':' .and. msg37(ispc3+4:ispc3+4).lt.':') grid=msg37(ispc3+1:ispc4-1)
          else if(indxr.lt.1) then
            if(ispc3-ispc2.eq.5 .and. msg37(ispc2+1:ispc2+1).gt.'@' .and. msg37(ispc2+1:ispc2+1).lt.'S' .and. &
               msg37(ispc2+2:ispc2+2).gt.'@' .and. msg37(ispc2+2:ispc2+2).lt.'S' .and. &
               msg37(ispc2+3:ispc2+3).lt.':' .and. msg37(ispc2+4:ispc2+4).lt.':') grid=msg37(ispc2+1:ispc3-1)
          endif
! CE3TSK 2026-11-08 (TODO 3.-241): both calls are mine and the DX station's here, so "RR73" is his acknowledgment - pack77
! sends the word as the square RR73, open Arctic Ocean where no station is. It was judged against the squares of his
! prefix and deleted, and FT4 and FT2 - whose type 3 comes before type 6 - then kept their RR73 pass from running (the
! duplicate list; LATE_START_AP.md, F1). Hound mode's type 21 always let it through (AP mask 'mybcall hisbcall ???'
! can trigger RR73 message); its own line below is folded into this one. After an R - "MYCALL HISCALL R RR73", which
! no program sends, and whose phantom would be taken for his RR73 - it is judged on the square as before (the review
! of 2026-11-08: the first form of this line let it through).
          if(indxr.lt.1 .and. grid.eq.'RR73') return
          if(len_trim(grid).eq.4 .and. grid.ne.hisgrid4) then ! correctly decoded contest message
            if(iaptype.eq.21) then; nbadcrc=1; msg37=''; return; endif ! not valid message in Hound mode
! CE3TSK 2026-10-13: here the roger IS judged on its square, as the grid message is - unlike places 4 and 5.
! Both calls are given to the decoder for this type, so a phantom needs no call to come out right: sixteen
! free bits, half of them "R" and a square. The program takes "MYCALL HISCALL R GRID" from the station being
! worked as the roger of a WW Digi QSO - a QSO under way is not held back by the window's mark - and would
! log a QSO that never was. The table is what stands between (the operator, 2026-09-30); a square the
! operator has for the call (hisgrid4) is not asked about.
            call chkgrid(hiscall,grid,lchkcall,lgvalid,lwrongcall)
            if(.not.lgvalid) then   ! CE3TSK 2026-10-14: on record as prefix/6 when it is the prefix (it said grid/6)
! CE3TSK 2026-10-14 (review 3.-72 item 9): for a square of open sea or the far south or north, chkgrid returns before
! it looks at the call (lchkcall) - the record said grid/6 there even for a prefix that is
! not allocated. The label is asked of the prefix alone then, as places 4 and 5 ask it; what is deleted is the same.
              if(.not.lwrongcall) call chkprefix(hiscall,lwrongcall)
              ngriddel=merge(-6,6,lwrongcall); nbadcrc=1; msg37=''; return
            endif
          endif
        endif
      endif
    endif
    return ! no need in other checks
  endif

  if((i3.ge.1 .and. i3.le.3) .and. (index(msg37,' R ').gt.0 .or. index(msg37,'/R ').gt.0 .or. index(msg37,'/P ').gt.0)) then
! i3=1: 'VF5RMP/P OW3ATG/P R MF' '378YYA/R 9H5PQU R OL53'
! i3=2: 'YT4MEY/P WL8QEW R IR03'
! i3=1: 'ER2XXD 8L7UWC R 529 26'			   
! i3=3 n3=1: '1M8JSH TI0CZQ R 539 42'
! i3=2 n3=5: 'DQ2VF/P J2OON KG37'
! 'N40RTH AD2WPI/R MG62'
!print *,msg37
    islash1=index(msg37,'/')
    call_a=''; call_b=''
    ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1
    if(islash1.lt.1 .and. ispc1.gt.3 .and. ispc2.gt.7) then
      call_a=msg37(1:ispc1-1); call_b=msg37(ispc1+1:ispc2-1)
      if(call_b.ne.hiscall) then
        include 'call_q.f90'
        falsedec=.false.
        call chkflscall(call_a,call_b,falsedec)
        if(falsedec) then; nbadcrc=1; msg37=''; return; endif
      endif
    endif

    if(islash1.gt.0 .and. ispc1.gt.3 .and. ispc2.gt.7) then
      islash2=index(msg37((islash1+1):),'/')+islash1
      if(islash1.gt.ispc1) then
        call_a=msg37(1:ispc1-1); call_b=msg37(ispc1+1:islash1-1)
      else
        call_a=msg37(1:islash1-1)
        if(islash2.gt.islash1) then
          call_b=msg37(ispc1+1:islash2-1)
        else
          call_b=msg37(ispc1+1:ispc2-1)
        endif
      endif
      include 'call_q.f90'
      if(call_a.ne.mycall .and. call_b.ne.hiscall) then
        falsedec=.false.
        call chkflscall(call_a,call_b,falsedec)
        if(falsedec) then; nbadcrc=1; msg37=''; return; endif
      endif
    endif
  endif

! i3=1 n3=4
! 000189 -23  0.4  300 ~ <...> QG6GFE CQ14
! 110015 -21 -0.0 2050 ~ <...> NF7OEX CP03
! check if grid is correct for call_2
  if(lcall1hash .and. i3.eq.1) then ! hash can be associated with a valid callsign in the table
    ispc1=index(msg37,' ')
    if(msg37(1:ispc1-1).ne.mycall) then
      ispc2=index(msg37((ispc1+1):),' ')+ispc1
      callsign=''; grid=''
      callsign=msg37(ispc1+1:ispc2-1)
      islash=index(callsign,'/')
      if(islash.lt.1) then
        include 'callsign_q.f90'
        if(callsign.ne.hiscall) then
          ispc3=index(msg37((ispc2+1):),' ')+ispc2
          if(ispc3-ispc2.eq.5 .and. msg37(ispc2+1:ispc2+1).gt.'@' .and. msg37(ispc2+1:ispc2+1).lt.'S' .and. &
             msg37(ispc2+2:ispc2+2).gt.'@' .and. msg37(ispc2+2:ispc2+2).lt.'S' .and. &
             msg37(ispc2+3:ispc2+3).lt.':' .and. msg37(ispc2+4:ispc2+4).lt.':') then
            grid=msg37(ispc2+1:ispc3-1)
            call chkgrid(callsign,grid,lchkcall,lgvalid,lwrongcall)
            if(lwrongcall .or. .not.lgvalid) then
              ngriddel=merge(-7,7,lwrongcall); nbadcrc=1; msg37=''; return
            endif
          endif
        endif
      endif
    endif
  endif

! i3=1 n3=1 '4P6NPC 5M5UJG BK49'
! i3=3 n3=1 '8Z7TLB JQ0OTZ 589 02'
! i3=0 n3=3 'CE3NTP 4C2DWN 5E ONE'
  if(iaptype.ne.2) then
    if(((i3.eq.1 .or. i3.eq.3) .and. index(msg37,' R ').le.0 .and. index(msg37,'/').le.0) .or. (i3.eq.0 .and. n3.eq.3)) then
      if(msg37(1:2).eq.'<.') go to 2
      ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1
      if(ispc1.gt.3 .and. ispc2.gt.7) then
        call_a=msg37(1:ispc1-1); if(call_a.eq.mycall) go to 2 ! exception to incoming calls
        call_b=msg37(ispc1+1:ispc2-1)
        include 'call_q.f90'
        falsedec=.false.
        call chkflscall(call_a,call_b,falsedec)
        if(falsedec) then; nbadcrc=1; msg37=''; return; endif
      endif
    endif
  endif

2 continue

  if(i3.eq.4) then
    ibrace1=index(msg37,'<.')
    if(index(msg37,'<.').gt.4) then
      callsign=''; callsign=msg37(1:ibrace1-2); islash=index(callsign,'/'); nlencall=len_trim(callsign)
      if(nlencall.lt.1) go to 4
! i3=4 n3=2 '6397X6JN637 <...> RRR' 
! 'R 5QCDOIY9 <...> RR73'
      if(index(trim(callsign),' ').gt.0 .or. (islash.lt.1 .and. callsign(nlencall:nlencall).lt.':')) then
        nbadcrc=1; msg37=''; return
      endif
      include 'callsign_q.f90'
      falsedec=.false.
      call chklong8(callsign,falsedec)
      if(falsedec) then; nbadcrc=1; msg37=''; return; endif
    endif
    if(msg37(1:2).eq.'<.' .or. index(msg37,'.>').gt.4) then
!'<...> 104J71MFOT9 RR73'
! i3=4 n3=0 '<...> /7ZZZZ/ /8'
! i3=4 n3=2 '<...> 6HGLDZ 5IN2 RRR'
      callsign=''; nlenmsg=len_trim(msg37)
      if(msg37(1:2).eq.'<.') then
        if(msg37(nlenmsg-4:nlenmsg).eq.' RR73') then; callsign=msg37(7:nlenmsg-5)
        else if(msg37(nlenmsg-2:nlenmsg).eq.' 73') then; callsign=msg37(7:nlenmsg-3)
        else if(msg37(nlenmsg-3:nlenmsg).eq.' RRR') then; callsign=msg37(7:nlenmsg-4)
        else; callsign=msg37(7:nlenmsg)
        endif
      else
        ibracket=index(msg37,'<.'); callsign=msg37(1:ibracket-2)
      endif
      nlencall=len_trim(callsign)
      if(nlencall.gt.0) then
        ispace=index(callsign(1:nlencall),' '); islash=index(callsign,'/')
        if(ispace.gt.0 .or. (islash.lt.1 .and. callsign(nlencall:nlencall).lt.':') .or. islash.eq.nlencall) then
          nbadcrc=1; msg37=''; return
        endif
! KY2HTW/0O5M <...>  i3=4 n3=0
        if(islash.gt.4) then
          islash2=index(callsign(islash+1:),'/')
          if(nlencall.eq.11 .and. islash2.lt.1) then
            if(islash.lt.7) then
              if((callsign(islash+1:islash+1).lt.':' .and. callsign(islash+2:islash+2).lt.':') .or. &
                 callsign(islash+1:islash+1).eq.'Q' .or. callsign(nlencall:nlencall).lt.':') then
                nbadcrc=1; msg37=''; return
              endif
            endif
            if(islash.gt.5) then
              if(callsign(islash-1:islash-1).lt.':' .or. (callsign(1:1).lt.':' .and.callsign(2:2).lt.':') .or. &
                 callsign(1:1).eq.'Q') then
                nbadcrc=1; msg37=''; return
              endif
            endif
            if(islash.gt.4 .and. islash.lt.8) then
              call_a=''; call_b=''; call_a=callsign(1:islash-1); call_b=callsign(islash+1:)
              falsedec=.false.
              call chkflscall(call_a,call_b,falsedec)
              if(falsedec) then; nbadcrc=1; msg37=''; return; endif
            endif
          endif
        endif
        include 'callsign_q.f90'
        falsedec=.false.
        call chklong8(callsign,falsedec)
        if(falsedec) then; nbadcrc=1; msg37=''; return; endif
      endif
    endif
! '<...> T99LMF GC41' need to understand how to check such message
  endif

4 continue

  if(i3.eq.3 .and. msg37(1:3).eq.'TU;') then 
!i3=3 n3=3 'TU; QV4UPP 632TGU 529'
    ispc1=index(msg37,' '); ispc2=index(msg37((ispc1+1):),' ')+ispc1 
    ispc3=index(msg37((ispc2+1):),' ')+ispc2
    if(ispc2.gt.7 .and. ispc3.gt.11) then
      call_a=msg37(ispc1+1:ispc2-1); call_b=msg37(ispc2+1:ispc3-1)
      include 'call_q.f90'
      falsedec=.false.
      call chkflscall(call_a,call_b,falsedec)
      if(falsedec) then; nbadcrc=1; msg37=''; return; endif
    endif
  endif

  if(i3.eq.0 .and. n3.eq.0) then
! 'J/.ZM1R-D1Q89'
    if(index(msg37,'/.').gt.0) then; nbadcrc=1; msg37=''; return; endif
    decoded=msg37(1:22)
    falsedec=.false.
    call filtersfree(decoded,falsedec)
    if(falsedec) then; nbadcrc=1; msg37=''; return; endif
  endif

! CQ_RLXA 551626 ^J65XI 
  if(i3.eq.0 .and. (msg37(1:3).eq.'CQ_' .or. index(msg37,'^').gt.0)) then
    nbadcrc=1; msg37=''; return
  endif

  return

contains
! CE3TSK 2026-11-08 (TODO 3.-241): "from the station being worked" - the word is the DX call. One test for places 4
! and 5, so that a later change to it reaches both.
! 2026-11-09 (TODO 3.-244, the review's point 6): or the DX call with a rover's /R or a portable's /P that the other one
! lacks - "K1ABC/R" in the message, "K1ABC" in the DX Call box, or the other way round - as ft8b compares his base call
! (hisbcall). Any other difference is another call: "K1ABCD", "K1AB", "PJ4/K1ABC" unless the box holds it.
  logical(1) function fromhis(word)
    character(len=*), intent(in) :: word
    fromhis=nosuffix(word).eq.nosuffix(hiscall)   ! (no DX call matches nothing: no word of a message is empty)
  end function fromhis
  character(len=12) function nosuffix(call)   ! the call without a trailing /R or /P (the blank after it: its end)
    character(len=*), intent(in) :: call
    integer n
    nosuffix=call
    n=index(nosuffix,'/R '); if(n.eq.0) n=index(nosuffix,'/P ')
    if(n.gt.0) nosuffix(n:)=' '
  end function nosuffix
end subroutine chkfalse8
