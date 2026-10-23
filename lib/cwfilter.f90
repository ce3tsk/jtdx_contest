subroutine cwfilter(swl,first,swlchanged)

  use ft8_mod1, only : cw,windowc1,windowx,pivalue,facx,mcq,m73,mrr73,mrrr,one,twopi,facc1,dt,csync,idtone25,csynccq, &
                       NFILT1,NFILT2,endcorr,endcorrswl,ctwkw,ctwkn,ctwk256
  use jt65_mod9 ! callsign DB to memory
  use prog_args ! path to files

  parameter (NFFT=180000)
  complex csig0(151680)
  real*4 window1(-NFILT1/2:NFILT1/2),window2(-NFILT2/2:NFILT2/2)
  character*37 msgcq25(25),msgsent37
  integer itone(79)
  integer*1 msgbits(77)
  logical(1), intent(in) :: swl
  logical, intent(in) :: first,swlchanged

!pushing callsigns from ALLCALL to memory
  if(first) then
    call allcall7_read ()   ! CE3TSK 2026-10-22 (TODO.md 3.-185): its own routine since, tested alone (test/fortran)

    pivalue=4.d0*atan(1.d0)
    twopi=8.d0*atan(1.d0)
    dt=1.d0/12000.d0

    do i=0,54; windowc1(i)=(1.0+cos(i*pivalue/55))/2; enddo
    facx=1.0/300.0
    do i=0,200
      windowx(i)=(1.0+cos(i*pivalue/200))/2
    enddo
    windowx=facx*windowx
    facc1=0.01/sqrt(61440.) ! 1.0/sqrt(192000.*3200.) NFFT1*NFFT2

    mcq=2*mcq-1
    mrrr=2*mrrr-1
    m73=2*m73-1
    mrr73=2*mrr73-1

    one=.false.
    do i=0,511
      do j=0,8
        if(iand(i,2**j).ne.0) one(i,j)=.true.
      enddo
    enddo

    msgcq25=''
    msgcq25(2)='CQ 2E0DLA IO92'
    msgcq25(3)='CQ BH3NEB ON81'
    msgcq25(4)='CQ CG3CGT FN04'
    msgcq25(5)='CQ DX CT1JA IM59'
    msgcq25(6)='CQ CU20E'
    msgcq25(7)='CQ NA CX1OB GF14'
    msgcq25(8)='CQ DF2AJ JN49'
    msgcq25(9)='CQ DG4XPZ JN58'
    msgcq25(10)='CQ EA8XR IL19'
    msgcq25(11)='CQ F1YE IN94'
    msgcq25(12)='CQ DX G1KLN IO82'
    msgcq25(13)='CQ HL2KVF PM38'
    msgcq25(14)='CQ IU1ZSV JN45'
    msgcq25(15)='CQ JG1TWO PM96'
    msgcq25(16)='CQ K2ST EM96'
    msgcq25(17)='CQ N9TUX EL98'
    msgcq25(18)='CQ SA NO2FA FM18'
    msgcq25(19)='CQ OH6GKE KP13'
    msgcq25(20)='CQ PD0ORM JO24'
    msgcq25(21)='CQ DX PT7DS HI06'
    msgcq25(22)='CQ RA3XEP KO84'
    msgcq25(23)='CQ SM2GSH KP05'
    msgcq25(24)='CQ UA9OHX NO15'
    msgcq25(25)='CQ JA W0YH EN12'
    do i=2,25
      i3=-1; n3=-1
      call genft8sd(msgcq25(i),i3,n3,msgsent37,msgbits,itone)
      if(i.eq.2) then
        call gen_ft8wave(itone,79,1920,2.0,12000.0,0.0,csig0,xjunk,1,151680)
        m=1
        do j=0,14
          do k=1,32
            if(j.lt.7) then; csync(j,k)=csig0(m); else; csynccq(j-7,k)=csig0(m); endif
            m=m+60
          enddo
        enddo
      endif
      idtone25(i,1:29)=itone(8:36)
      idtone25(i,30:58)=itone(44:72)
    enddo
    dt2=0.005 ! fs2=200 Hz
    k=1
    do ifr=-5,5                              !Search over +/- 2.5 Hz
      delf=ifr*0.5
      dphi=twopi*delf*dt2
      phi=0.0
      do i=1,32
        ctwkw(k,i)=cmplx(cos(phi),sin(phi))
        phi=mod(phi+dphi,twopi)
      enddo
      k=k+1
    enddo
    k=1
    do ifr=-5,5                              !Search over +/- 1.25 Hz
      delf=ifr*0.25
      dphi=twopi*delf*dt2
      phi=0.0
      do i=1,32
        ctwkn(k,i)=cmplx(cos(phi),sin(phi))
        phi=mod(phi+dphi,twopi)
      enddo
      k=k+1
    enddo
! lcqsignal
    delf=3.125
    dphi=twopi*delf*dt2
    phi=0.0
    do i=1,256
      ctwk256(i)=cmplx(cos(phi),sin(phi))
      phi=mod(phi+dphi,twopi)
    enddo
  endif

! this filter is being used for signal subtraction
! Create and normalize the filter
  if(first .or. swlchanged) then
     fac=1.0/float(nfft)
     sumw=0.0
     if(.not.swl) then
        do j=-NFILT1/2,NFILT1/2
           window1(j)=cos(pivalue*j/NFILT1)**2
           sumw=sumw+window1(j)
        enddo
        cw=0.
        cw(1:NFILT1+1)=window1/sumw
        cw=cshift(cw,NFILT1/2+1)
        do j=1,NFILT1/2+1
          endcorr(j)=1.0/(1.0-sum(window1(j-1:NFILT1/2))/sumw)
        enddo
     else
        do j=-NFILT2/2,NFILT2/2
           window2(j)=cos(pivalue*j/NFILT2)
           sumw=sumw+window2(j)
        enddo
        cw=0.
        cw(1:NFILT2+1)=window2/sumw
        cw=cshift(cw,NFILT2/2+1)
        do j=1,NFILT2/2+1
          endcorrswl(j)=1.0/(1.0-sum(window2(j-1:NFILT2/2))/sumw)
        enddo
     endif
     call four2a(cw,NFFT,1,-1,1)
     cw=cw*fac
  endif

  return  
end subroutine cwfilter
  

! CE3TSK 2026-10-22 (TODO.md 3.-185): ALLCALL7.TXT into jt65_mod9's 16 tables, once, at the first decode - moved here out
! of cwfilter's first call, with three changes. JTDX_ALLCALL7=0 or 1 says whether the file is used at all (luseallcall7,
! LALLCALL7DEFAULT the default, off): off, it is not read, the tables are not allocated, ldbvalid is false and
! searchcalls () finds every call; on, the tables are allocated once the file is open, and ldbvalid says whether every
! one of them got a call. It reads up to MAXC lines, 500000 (200000 until then, which JP1LRT's 222,041-call file
! outgrew - the X-Z table stayed empty). And a table is filled only while it has room (it was never checked): a call
! whose table is full is left out, as is all of a file longer than MAXC lines, and that is said once - "ALLCALL7.TXT
! too long, calls skipped", the rest used as read.
subroutine allcall7_read ()

  use jt65_mod9 ! callsign DB to memory
  use prog_args ! path to files
  character*16 envval
  integer lenv,ienvstat,ios,i,i1

  call get_environment_variable('JTDX_ALLCALL7',envval,lenv,ienvstat)   ! read once, single-threaded
  if(ienvstat.eq.0 .and. lenv.gt.0) then
    if(envval(1:1).eq.'0') luseallcall7=.false.
    if(envval(1:1).eq.'1') luseallcall7=.true.
  endif
  if(.not.luseallcall7) then
    ldbvalid=.false.   ! not used: not read, no table allocated; searchcalls () finds every call, as JTDX without the file - and no warning
    return
  endif
! CE3TSK 2026-09-05: status='old' with iostat, not 'unknown' - the latter CREATED an empty
! ALLCALL7.TXT wherever the share directory had none, and that empty file is what an install
! without it then reported as broken on every start. A missing file now leaves every count at
! zero, so the check below reports it once and sets ldbvalid false (searchcalls then accepts
! every callsign, as JTDX does without the file). The tables feed ft8b's /R acceptance and
! chkfalse's JT65 filter directly, not only chkflscall - so since 2026-10-22 (3.-185) it is the
! switch above, not chkflscall's, that decides whether any of them is used.
  open(24,file=trim(share_dir)//'/ALLCALL7.TXT',status='old',iostat=ios) ! accepting Australian 7-char callsigns
  if(ios.eq.0) then
    if(.not.allocated(call0c)) allocate(call0c(115000),calld(111000),callef(90000),callgh(75000),calli(76000),        &
       callj(90000),callk(240000),calllm(65000),calln(94000),callo(67000),callpq(58000),callr(65000),callst(75000), &
       calluv(104000),callw(137000),callxz(58000))   ! the sizes the fixed tables had
    do i=1,MAXC
      read(24,1004,end=20) line
1004  format(a80)
      if(line(1:4).eq.'ZZZZ') exit
      if(line(1:2).eq.'//') cycle
      i1=index(line,',')
      if(i1.lt.4 .or. i1.gt.8) cycle
      callsign=line(1:i1-1)
      if(callsign(1:1).gt.'0' .and. callsign(1:1).lt.'D') then; call put(call0c,ncall0c)
      else if(callsign(1:1).gt.'C' .and. callsign(1:1).lt.'E') then; call put(calld,ncalld)
      else if(callsign(1:1).gt.'D' .and. callsign(1:1).lt.'G') then; call put(callef,ncallef)
      else if(callsign(1:1).gt.'F' .and. callsign(1:1).lt.'I') then; call put(callgh,ncallgh)
      else if(callsign(1:1).gt.'H' .and. callsign(1:1).lt.'J') then; call put(calli,ncalli)
      else if(callsign(1:1).gt.'I' .and. callsign(1:1).lt.'K') then; call put(callj,ncallj)
      else if(callsign(1:1).gt.'J' .and. callsign(1:1).lt.'L') then; call put(callk,ncallk)
      else if(callsign(1:1).gt.'K' .and. callsign(1:1).lt.'N') then; call put(calllm,ncalllm)
      else if(callsign(1:1).gt.'M' .and. callsign(1:1).lt.'O') then; call put(calln,ncalln)
      else if(callsign(1:1).gt.'N' .and. callsign(1:1).lt.'P') then; call put(callo,ncallo)
      else if(callsign(1:1).gt.'O' .and. callsign(1:1).lt.'R') then; call put(callpq,ncallpq)
      else if(callsign(1:1).gt.'Q' .and. callsign(1:1).lt.'S') then; call put(callr,ncallr)
      else if(callsign(1:1).gt.'R' .and. callsign(1:1).lt.'U') then; call put(callst,ncallst)
      else if(callsign(1:1).gt.'T' .and. callsign(1:1).lt.'W') then; call put(calluv,ncalluv)
      else if(callsign(1:1).gt.'V' .and. callsign(1:1).lt.'X') then; call put(callw,ncallw)
      else if(callsign(1:1).gt.'W' .and. callsign(1:1).le.'Z') then; call put(callxz,ncallxz)
      endif
    enddo
    if(i.gt.MAXC) then   ! MAXC lines read: whatever follows them but the end mark is left out
      read(24,1004,end=20) line
      if(line(1:4).ne.'ZZZZ') lallcall7cut=.true.
    endif
20  close(24)
  endif
!print *,ncall0c,"call0c"; print *,ncalld,"calld"; print *,ncallef,"callef"; print *,ncallgh,"callgh"
!print *,ncalli,"calli"; print *,ncallj,"callj"; print *,ncallk,"callk"; print *,ncalllm,"calllm"
!print *,ncalln,"calln"; print *,ncallo,"callo"; print *,ncallpq,"callpq"; print *,ncallr,"callr"
!print *,ncallst,"callst"; print *,ncalluv,"calluv"; print *,ncallw,"callw"; print *,ncallxz,"callxz"
  ldbvalid=.not.(ncall0c.lt.1 .or. ncalld.lt.1 .or. ncallef.lt.1 .or. ncallgh.lt.1 .or. ncalli.lt.1 .or. ncallj.lt.1 .or. &
     ncallk.lt.1 .or. ncalllm.lt.1 .or. ncalln.lt.1 .or. ncallo.lt.1 .or. ncallpq.lt.1 .or. ncallr.lt.1 .or.  &
     ncallst.lt.1 .or. ncalluv.lt.1 .or. ncallw.lt.1 .or. ncallxz.lt.1)   ! (3.-185) set here: it starts false
  if(.not.ldbvalid) then
    write(*,4) 'ALLCALL7.TXT is too short or broken?','d' ! 50th position for "d"
4   format(a36,13x,a1)
    call flush(6)
  endif
  if(nallcall7skip.gt.0 .or. lallcall7cut) then   ! (3.-185) once, the tables as read
    write(*,4) 'ALLCALL7.TXT too long, calls skipped','d'
    call flush(6)
  endif

contains

! (3.-185) a call into its table while the table has room; else counted, left out
  subroutine put(tab,n)
    character*7, intent(inout) :: tab(:)
    integer, intent(inout) :: n
    if(n.lt.size(tab)) then
      n=n+1; tab(n)=callsign(1:7)
    else
      nallcall7skip=nallcall7skip+1
    endif
  end subroutine put

end subroutine allcall7_read
