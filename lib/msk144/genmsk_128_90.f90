! CE3TSK 2026-11-17 (MSK144_PLAN.md step 0): WSJT-X 3.0.2's genmsk_128_90, with one more argument and two callers'
! names at the end of this file. This program's pack77 and unpack77 take one more argument each, and genft8 shows
! the two ways they are used: ntxhash 1 with slice 26 for a message that is TRANSMITTED (its hashed calls are kept
! in the transmit tables, so msgsent reads <PJ4/K1ABC> and not <...>), ntxhash 0 with slice 27 for a "service
! message" - a RECEIVED frame built again, which must leave every table alone. WSJT-X has one table for both.
!   genmsk_128_90     the generator as WSJT-X calls it: the transmit pair (the simulator, msk144code, the
!                     transmit path of step 2)
!   genmsk_128_90_rx  the service pair: msk144signalquality, which rebuilds each decoded frame
! 2026-11-18 (the review 3.-270, P3): ONE CALLER AT A TIME. WSJT-X's routine keeps every variable static (its `save`),
! and WSJT-X calls it from one thread - it decodes in the thread that paints. Here the decoder has a thread of its
! own and rebuilds every decoded frame (genmsk_128_90_rx) while the main window's thread may be making our own
! message (genmsk_128_90): the two wrote the same message, bits and codeword, and a transmission's tones could be
! another message's. WSJT-X's text is genmsk_128_90_body below, as it was; genmsk_128_90_hash, which both names call,
! lets one caller in at a time (the lock is a process-wide one: it holds between any two threads, OpenMP's or not;
! every Fortran file of this program is compiled with OpenMP).
subroutine genmsk_128_90_body(msg0,ichk,msgsent,i4tone,itype,ntxhash)
! s8 + 48bits + s8 + 80 bits = 144 bits (72ms message duration)
!
! Encode an MSK144 message
! Input:
!   - msg0     requested message to be transmitted
!   - ichk     if ichk=1, return only msgsent
!              if ichk.ge.10000, set imsg=ichk-10000 for short msg
!   - msgsent  message as it will be decoded
!   - i4tone   array of audio tone values, 0 or 1
!   - itype    message type 
!                 1 = standard message  "Call_1 Call_2 Grid/Rpt"
!                 2 = type 1 prefix
!                 3 = type 1 suffix
!                 4 = type 2 prefix
!                 5 = type 2 suffix
!                 6 = free text (up to 13 characters)
!                 7 = short message     "<Call_1 Call2> Rpt"

  use iso_c_binding, only: c_loc,c_size_t
  use packjt77
  character*37 msg0
  character*37 message                    !Message to be generated
  character*37 msgsent                    !Message as it will be received
  character*77 c77
  integer*4 i4tone(144)
  integer*1 codeword(128)
  integer*1 msgbits(77) 
  integer*1 bitseq(144)                   !Tone #s, data and sync (values 0-1)
  integer*1 s8(8)
  real*8 pp(12)
  real*8 xi(864),xq(864),pi,twopi
  data s8/0,1,1,1,0,0,1,0/
  equivalence (ihash,i1hash)
  logical first,unpk77_success
  data first/.true./
  save

  if(first) then
    first=.false.
    nsym=128
    pi=4.0*atan(1.0)
    twopi=8.*atan(1.0)
    do i=1,12
      pp(i)=sin((i-1)*pi/12)
    enddo
  endif

  message(1:37)=' ' 
  itype=1
  if(msg0(1:1).eq.'@') then                    !Generate a fixed tone
     read(msg0(2:5),*,end=1,err=1) nfreq       !at specified frequency
     go to 2
1    nfreq=1000
2    i4tone(1)=nfreq
     msgsent=msg0                              ! CE3TSK 2026-11-18: WSJT-X 3.3.0-beta6's correction (1fbe191e0) -
     go to 999                                 ! a frequency, not channel tones: no polarity flip
  else
     message=msg0

     do i=1, 37
        if(ichar(message(i:i)).eq.0) then
           message(i:37)=' '
           exit
        endif
     enddo
     do i=1,37                               !Strip leading blanks
        if(message(1:1).ne.' ') exit
        message=message(i+1:)
     enddo

     if(message(1:1).eq.'<') then
        i2=index(message,'>')
        i1=0
        if(i2.gt.0) i1=index(message(1:i2),' ')
        if(i1.gt.0) then
           call genmsk40(message,msgsent,ichk,i4tone,itype)
           if(itype.lt.0) go to 999
           i4tone(41)=-40
           go to 999
        endif
     endif

     i3=-1
     n3=-1
     call pack77(message,i3,n3,c77,ntxhash)        ! CE3TSK: as genft8 - see the head of this file
     if(ntxhash.eq.1) then
        call unpack77(c77,0,msgsent,unpk77_success,26) !Unpack to get msgsent
     else
        call unpack77(c77,0,msgsent,unpk77_success,27)
     endif
     if(ichk.eq.1) go to 999
     read(c77,"(77i1)") msgbits
     call encode_128_90(msgbits,codeword)

!Create 144-bit channel vector:
!8-bit sync word + 48 bits + 8-bit sync word + 80 bits
     bitseq=0 
     bitseq(1:8)=s8
     bitseq(9:56)=codeword(1:48)
     bitseq(57:64)=s8
     bitseq(65:144)=codeword(49:128)
     bitseq=2*bitseq-1

     xq(1:6)=bitseq(1)*pp(7:12)   !first bit is mapped to 1st half-symbol on q
     do i=1,71
       is=(i-1)*12+7
       xq(is:is+11)=bitseq(2*i+1)*pp
     enddo 
     xq(864-5:864)=bitseq(1)*pp(1:6)   !last half symbol
     do i=1,72                                    
       is=(i-1)*12+1
       xi(is:is+11)=bitseq(2*i)*pp
     enddo
! Map I and Q  to tones. 
    i4tone=0 
    do i=1,72
      i4tone(2*i-1)=(bitseq(2*i)*bitseq(2*i-1)+1)/2;
      i4tone(2*i)=-(bitseq(2*i)*bitseq(mod(2*i,144)+1)-1)/2;
    enddo
  endif

! Flip polarity
  i4tone=-i4tone+1
999 return
end subroutine genmsk_128_90_body

subroutine genmsk_128_90_hash(msg0,ichk,msgsent,i4tone,itype,ntxhash)   ! CE3TSK: one caller at a time, see the head
  character*37 msg0,msgsent
  integer*4 i4tone(144)
!$omp critical(genmsk_128_90)
  call genmsk_128_90_body(msg0,ichk,msgsent,i4tone,itype,ntxhash)
!$omp end critical(genmsk_128_90)
  return
end subroutine genmsk_128_90_hash

subroutine genmsk_128_90(msg0,ichk,msgsent,i4tone,itype)   ! CE3TSK: a message to be transmitted
  character*37 msg0,msgsent
  integer*4 i4tone(144)
  call genmsk_128_90_hash(msg0,ichk,msgsent,i4tone,itype,1)
  return
end subroutine genmsk_128_90

subroutine genmsk_128_90_rx(msg0,ichk,msgsent,i4tone,itype)   ! CE3TSK: a received frame, built again
  character*37 msg0,msgsent
  integer*4 i4tone(144)
  call genmsk_128_90_hash(msg0,ichk,msgsent,i4tone,itype,0)
  return
end subroutine genmsk_128_90_rx
