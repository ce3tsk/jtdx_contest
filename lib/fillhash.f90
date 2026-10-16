subroutine fillhash(numthreads,lfill)

  use packjt77 ! also setting mycall13,dxcall13
  use ft8_mod1, only : mycall,hiscall
  use prog_args, only : data_dir   ! CE3TSK 2026-10-15: compound_calls.txt (packjt77 remember_*)
  integer, intent(in) :: numthreads
  logical, intent(in) :: lfill
  character*13 cw
  integer n10dx,n12dx,n22dx

  if(.not.lremloaded) call remember_load(data_dir)
  if(lfill) then
    do i=1,numthreads
      do m=1,nlast_calls(i)
        nposition=(i-1)*NSLICECALLS+m
        cw=last_calls(nposition)
!print *,i,m,cw
        n10=ihashcall(cw,10)
        if(n10.ge.0 .and. n10 .le. 1023 .and. cw.ne.mycall13) calls10(n10)=cw

        n12=ihashcall(cw,12)
        if(n12.ge.0 .and. n12 .le. 4095 .and. cw.ne.mycall13) calls12(n12)=cw

        n22=ihashcall(cw,22)
        if(any(ihash22.eq.n22)) then   ! If entry exists, make sure callsign is the most recently received one
          where(ihash22.eq.n22) calls22=cw
          go to 900
        endif

! New entry: move table down, making room for new one at the top
        ihash22(nzhash:2:-1)=ihash22(nzhash-1:1:-1)

! Add the new entry
        calls22(nzhash:2:-1)=calls22(nzhash-1:1:-1)
        ihash22(1)=n22
        calls22(1)=cw
        if(nzhash.lt.MAXHASH) nzhash=nzhash+1
900     continue
      enddo
    enddo
! CE3TSK 2026-10-15: the calls heard sending, with this period's time (packjt77 sent12call, unpack77's type 4 rule) -
! 2026-10-16 (TODO 3.-113, the review 3.-111 D3): from periods on the air only, not from wav files opened in the program
    if(lonair) then
      do i=1,numthreads
        do m=1,nlast_senders(i)
          cw=last_senders((i-1)*NSLICECALLS+m)
          n12=ihashcall(cw,12)
          if(n12.ge.0 .and. n12.le.4095 .and. cw.ne.mycall13) then
            sent12call(n12)=cw; sent12time(n12)=nsecrun
          endif
        enddo
      enddo
    endif
! CE3TSK 2026-10-15: the remembered compound calls, from trusted decodes only (note_decode, packjt77); to their file -
! a newly confirmed one at once, a later day every ten minutes. Not from wav files opened in the program (lonair,
! TODO 3.-105): they are read with the list, and feed nothing to it.
    if(lonair) then
      do i=1,numthreads
        do m=1,nlast_trusted(i)
          call remember_call(last_trusted((i-1)*NSLICECALLS+m))
        enddo
      enddo
    endif
    if(lremdirty .and. (lremnew .or. nremwrite.lt.0 .or. nsecrun-nremwrite.ge.600)) call remember_save(data_dir)
  else
    nlast_calls=0
    nlast_senders=0
    nlast_trusted=0
    call remember_today()   ! CE3TSK 2026-10-15: the day a remembered call was last heard
    mycall13=mycall//' '; dxcall13=hiscall//' '
    if(mycall13.ne.mycall13_0) then
      if(len(trim(mycall13)).gt.2) then
        mycall13_set=.true.
        mycall13_0=mycall13
        call save_hash_mycall(mycall13,hashmy10,hashmy12,hashmy22)
!print *,mycall13,hashmy10,hashmy12,hashmy22
      else
        mycall13_set=.false.
      endif
    endif

! CE3TSK 2026-09-19: this used to act only when the DX call DIFFERED from the last one seen
! (dxcall13_0), and dxcall13_0 survives the DX call being cleared. So: work VP2X/K1JT, change
! band - the decoder wipes the 22-bit table (decoder.f90, lbandchanged) and the GUI clears the DX
! call - type VP2X/K1JT again: equal to dxcall13_0, nothing was done, dxcall13_set stayed .false.
! and the call was never hashed again. For a SuperFox with a compound call that is fatal: every
! one of its lines carries only the hash, so they read <...> until the decoder is restarted, and
! the GUI's blind-call rule halts every transmission. Now: a DX call that is set is made sure of
! whenever it is new, was unset in between, or is no longer in the table (wiped, or pushed out).
    if(len(trim(dxcall13)).gt.2) then
      if(dxcall13.ne.dxcall13_0 .or. .not.dxcall13_set .or. .not.any(calls22.eq.dxcall13)) then
        dxcall13_set=.true.
        dxcall13_0=dxcall13
        hashdx10=ihashcall(dxcall13,10)
! make sure new DX Call is stored in hash tables prior to decoding
! it is needed if manually callsign set in DX Call window was not decoded before
        call save_hash_call(dxcall13,1)
! CE3TSK 2026-10-16 (TODO 3.-113, the review 3.-111 D1): and into the tables AT ONCE, as my call is - save_hash_call
! queues it for the merge after a decode (fillhash .true.: FT4 and FT2 at the period's end, FT8 after its first pass),
! so in the first period after a band or mode change emptied the tables a typed DX call's hash read <...>
! (test/decode/bandhash.sh). save_hash_mycall is the plain insert (calls22, calls12, calls10); its hashes are not kept
        call save_hash_mycall(dxcall13,n10dx,n12dx,n22dx)
!print *,dxcall13,hashdx10
      endif
    else
      dxcall13_set=.false.
    endif
  endif

  return
end subroutine fillhash
