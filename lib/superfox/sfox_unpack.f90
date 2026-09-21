subroutine sfox_unpack(nutc,x,nsnr,f0,dt0,foxcall,notp)

! CE3TSK: adapted from WSJT-X 3.0.2 lib/superfox/sfox_unpack.f90 (SUPERFOX_PLAN.md). The
! unpacking is the donor's. What differs:
! - jtdx's unpack28 takes a fourth argument, the slice whose hash list it belongs to; 1 here,
!   the receiver is single threaded. The 22-bit hash lookup itself is band wide;
! - jtdx resolves the operator's OWN hashed callsign in unpack77, not in unpack28, so a Hound
!   with a non-standard call would see "<...>" where the Fox addresses it. Resolved here the
!   same way unpack77 does, from hashmy22;
! - every line goes through sfox_emit below, which prints jtdx's decode-line columns (message
!   at column 23, 26 wide, one marker character) and flushes, as ft8_decoded does. The donor's
!   format put the message one column further right and did not flush.
! - msg is 26 characters, jtdx's message width, where the donor had 22: with the own call
!   resolved, "<PJ4/K1ABC/P> VP8PJ RR73" is 24 and the donor's width would cut the RR73.
! - a compound Fox call decoded from its CQ (i3 = 3) is remembered for the hash lookup, see there.
! - a Hound message is assembled by sfox_fit below, which keeps it within what the GUI parses.

  use packjt                      !CE3TSK: unpackgrid is a module procedure in jtdx
  use packjt77
  parameter (NQU1RKS=203514677)
  integer*1 x(0:49)
  integer*8 n58
  logical success
  character*336 msgbits
  character*26 msg(10)            !### only msg(1) is used ??? ###  CE3TSK: 26, was 22
  character*13 foxcall,c13
  character*10 ssignature
  character*4 crpt(5),grid4,ctail
  character*26 freeTextMsg
  character*38 c
  logical use_otp
  data c/' 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/'/

  ncq=0
  if (notp.eq.0) then
     use_otp = .FALSE.
  else
     use_otp = .TRUE.
  endif
  write(msgbits,1000) x(0:46)
1000 format(47b7.7)
  read(msgbits(327:329),'(b6)') i3            !Message type
  read(msgbits(1:28),'(b28)') n28           !Standard Fox call
  call unpack28(n28,foxcall,success,1)

  if(i3.eq.1) then                            !Compound Fox callsign
!     read(msgbits(87:101),'(b15)') n15
!     call unpackgrid(n15,grid4)
!     msg(1)='CQ '//trim(foxcall)//' '//grid4
!     call sfox_emit(nutc,nsnr,dt0,nint(f0),trim(msg(1)))
!     go to 100
  else if(i3.eq.2) then                       !Up to 4 Hound calls and free text
     call unpacktext77(msgbits(161:231),freeTextMsg(1:13))
     call unpacktext77(msgbits(232:302),freeTextMsg(14:26))
     do i=26,1,-1
        if(freeTextMsg(i:i).ne.'.') exit
        freeTextMsg(i:i)=' '
     enddo
     call sfox_emit(nutc,nsnr,dt0,nint(f0),freeTextMsg)
  else if(i3.eq.3) then                       !CQ FoxCall Grid     
     read(msgbits(1:58),'(b58)') n58          !FoxCall
     do i=11,1,-1
        j=mod(n58,38)+1
        foxcall(i:i)=c(j:j)
        n58=n58/38
     enddo
     foxcall(12:13)='  '
! CE3TSK: a Fox with a compound call sends that call in full only HERE, in its CQ; its other
! messages carry a 22-bit hash of it, which prints as <...> until the call is known (tested with
! MSHV-made transmissions of VP2X/K1JT, 2026-09-19). WSJT-X leaves that to the operator, who
! types the call into DX Call; here the receiver remembers it, the way the FT8 decoder remembers
! every call it prints, and superfox_slot (decoder.f90) moves it into the hash table - so the
! Fox's next transmission reads "K1ABC <VP2X/K1JT> RR73". A deliberate departure from the donor.
     call save_hash_call(adjustl(foxcall),1)
     read(msgbits(59:73),'(b15)') n15
     call unpackgrid(n15,grid4)
     msg(1)='CQ '//trim(foxcall)//' '//grid4
     call sfox_emit(nutc,nsnr,dt0,nint(f0),trim(msg(1)))
     read(msgbits(74:105),'(b32)') n32
     if(n32.eq.NQU1RKS) go to 100
     call unpacktext77(msgbits(74:144),freeTextMsg(1:13))
     call unpacktext77(msgbits(145:215),freeTextMsg(14:26))
     do i=26,1,-1
        if(freeTextMsg(i:i).ne.'.') exit
        freeTextMsg(i:i)=' '
     enddo
     if(len(trim(freeTextMsg)).gt.0) call sfox_emit(nutc,nsnr,dt0,          &
          nint(f0),freeTextMsg)
     go to 100
  endif

  j=281
  iz=4                                         !Max number of reports
  if(i3.eq.2) j=141
  do i=1,iz                                    !Extract the reports
     read(msgbits(j:j+4),'(b5)') n
     if(n.eq.31) then
        crpt(i)='RR73'
     else
        write(crpt(i),1006) n-18
1006    format(i3.2)
        if(crpt(i)(1:1).eq.' ') crpt(i)(1:1)='+'
     endif
     j=j+5
  enddo

! Unpack Hound callsigns and format user-level messages:
  iz=9                                          !Max number of hound calls
  if(i3.eq.2 .or. i3.eq.3) iz=4
  do i=1,iz
     j=28*i + 1
     read(msgbits(j:j+27),'(b28)') n28
     call unpack28(n28,c13,success,1)
     if(c13(1:5).eq.'<...>' .and. mycall13_set .and. (n28-2063592).eq.hashmy22)     &
          c13='<'//trim(mycall13)//'>'
     if(n28.eq.0 .or. n28.eq.NQU1RKS) cycle 
     ctail=' '
     if(trim(c13).eq.'CQ') then
        ncq=ncq+1
     else
        if(i3.eq.2) then
           ctail=crpt(i)
        else
           if(i.le.5) ctail='RR73'
           if(i.gt.5) ctail=crpt(i-5)
        endif
     endif
     call sfox_fit(c13,foxcall,ctail,msg(i))
     if(ncq.le.1 .or. msg(i)(1:3).ne.'CQ ') then
        call sfox_emit(nutc,nsnr,dt0,nint(f0),trim(msg(i)))
     endif
  enddo

  if(msgbits(306:306).eq.'1' .and. ncq.lt.1) then
     call sfox_emit(nutc,nsnr,dt0,nint(f0),'CQ '//foxcall)
  endif

100 read(msgbits(307:326),'(b20)') notp
  if (use_otp) then
     write(ssignature,'(I6.6)') notp
     call sfox_emit(nutc,nsnr,dt0,nint(f0),'$VERIFY$ '//trim(foxcall)//' '//trim(ssignature))
  endif
  return
end subroutine sfox_unpack

subroutine sfox_emit(nutc,nsnr,dt,nfreq,text)

! CE3TSK: one SuperFox decode line on stdout in jtdx's FT8 columns, as ft8_decoded prints them
! (decoder.f90, format 1000): the message starts at column 23 and is 26 wide, then one marker
! character - blank for an ordinary decode. A text longer than 26 is printed whole rather
! than cut: only the "$VERIFY$ <call> <code>" line of a Fox with a long compound call can be,
! the GUI takes that line before it reaches the column parser, and a cut there would lose
! digits of the code.

  use sfox_mod, only : lsfoxap
  character*(*) text
  character*40 m
  character*1 mark

  m=text
  n=max(26,len_trim(m))
! CE3TSK 2026-09-20: '*' marks a line of an A-PRIORI decode (qpc_decode2's a-priori pass), as it
! marks FT8's. Not the $VERIFY$ line: the GUI takes that one apart on blanks, and behind a long
! compound call the marker would stand against the code.
  mark=' '
  if(lsfoxap .and. index(m,'$VERIFY$').ne.1) mark='*'
  write(*,1000) nutc,nsnr,dt,nfreq,m(1:n),mark
1000 format(i6.6,i4,f5.1,i5,1x,'~',1x,a,a1)
  call flush(6)

  return
end subroutine sfox_emit

subroutine sfox_fit(c1,c2,tail,msg)

! CE3TSK: "<Hound> <Fox> report" within the 24 characters the GUI's message parser reads
! (decodedtext.cpp takes message.left(24) BEFORE it removes the <>, so whatever lies beyond is
! lost to the sequencer, and a cut report is a wrong report: "-05" read as "-0"). Only two
! bracketed calls together can exceed it - "<PJ4/K1ABC> <VP2X/K1JT> RR73" is 28 - which needs a
! Hound with a non-standard call AND a Fox with a compound one; the donor's 22-character msg cut
! such a line too. The brackets only say "this call arrived as a hash", and the GUI removes them
! anyway, so they are given up first, the Fox's before the Hound's; if even the bare calls do not
! fit (two calls of 10 or 11 characters) the Hound's call is shown as <...>, which always fits and
! is what a station that does not know the call would see. The report is never cut.

  character*(*) c1,c2,tail,msg
  character*13 a,b
  character*40 m

  a=c1; b=c2
  do k=0,3
     if(k.eq.1) call sfox_bare(b)               !the Fox's brackets go first
     if(k.eq.2) call sfox_bare(a)               !then the Hound's
     if(k.eq.3) a='<...>'
     m=trim(a)//' '//trim(b)
     if(len_trim(tail).gt.0) m=trim(m)//' '//trim(tail)
     if(len_trim(m).le.24) exit
  enddo
  msg=m

  return
end subroutine sfox_fit

subroutine sfox_bare(c)

  character*13 c

  if(c(1:1).eq.'<' .and. c(1:5).ne.'<...>') then
     n=index(c,'>')
     if(n.gt.2) c=c(2:n-1)
  endif

  return
end subroutine sfox_bare
