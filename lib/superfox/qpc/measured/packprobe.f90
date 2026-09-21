program packprobe

! CE3TSK 2026-09-20: the PAYLOAD of a SuperFox transmission, from WSJT-X's own packer.
!
! This is wsjtx-3.0.2/lib/superfox/sftx.f90 (K1JT, GPL v3) up to and including its call of
! sfox_pack - the same input file, the same arguments - and it stops there: it prints the 50
! seven-bit message symbols xin(0:49) and never calls the encoder. WSJT-X's sftx, run on the same
! input, gives the 151 tones of the transmission. measure.py puts the two side by side.
! It is linked against WSJT-X's library (build_packprobe.sh), not against this program's.
!
!   packprobe <message_file_name> <foxcall> <ckey>

  character*120 fname
  character*120 line
  character*40 cmsg(5)
  character*26 freeTextMsg
  character*10 ckey
  character*11 foxcall0,foxcall
  logical*1 bMoreCQs,bSendMsg
  integer*1 xin(0:49)

  if(iargc().ne.3) stop 1
  call getarg(1,fname)
  call getarg(2,foxcall0)
  call getarg(3,ckey)
  open(25,file=trim(fname),status='old')
  do i=1,5
     read(25,1000,end=10) cmsg(i)
1000 format(a40)
  enddo
  i=6
10 close(25)
  nslots=i-1
  freeTextMsg='                          '
  bMoreCQs=cmsg(1)(40:40).eq.'1'
  bSendMsg=cmsg(nslots)(39:39).eq.'1'
  if(bSendMsg) then
     freeTextMsg=cmsg(nslots)(1:26)
     if(nslots.gt.2) nslots=2
  endif
  call foxgen2(nslots,cmsg,line,foxcall)
  call sfox_pack(line,ckey,bMoreCQs,bSendMsg,freeTextMsg,xin)
  write(*,'(50i4)') xin
end program packprobe
