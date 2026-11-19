! CE3TSK 2026-11-17 (MSK144_PLAN.md step 0): WSJT-X 3.0.2's lib/fmtmsg.f90 under another name. This program's own
! fmtmsg (lib/fmtmsg.f90) is the old modes' and reads 22 characters; the MSK144 routines format 37-character
! messages and the two-call string a short message's hash is taken from. Nothing else differs from WSJT-X's.
subroutine fmtmsg37(msg,iz)

  character*(*) msg

! Convert all letters to upper case
  iz=len(msg)
  do i=1,iz
     if(msg(i:i).ge.'a' .and. msg(i:i).le.'z')                       &
          msg(i:i)= char(ichar(msg(i:i))+ichar('A')-ichar('a'))
     if(msg(i:i).ne.' ') iz=i
  enddo

  do iter=1,37                           !Collapse multiple blanks into one
     ib2=index(msg(1:iz),'  ')
     if(ib2.lt.1) go to 100
     msg=msg(1:ib2)//msg(ib2+2:)
     iz=iz-1
  enddo

100 return
end subroutine fmtmsg37
