subroutine sfrx_sub(nutc,nfqso,ntol,iwave,ldecoded)

! CE3TSK: the SuperFox receiver's entry point, adapted from WSJT-X 3.0.2
! lib/superfox/sfrx_sub.f90 (SUPERFOX_PLAN.md). What differs from the donor, and why:
! - the date argument, "use julian" and the ntime8 block are gone: the donor computed a time it
!   never used;
! - sfox_remove_ft8 is not called HERE. The donor decodes and subtracts ordinary FT8 signals
!   with its own FT8 helpers before it looks for the Fox. This program does that with its OWN
!   FT8 decoder, and only when the Fox was not found in the band as received (milestone 3,
!   decoder.f90: superfox_slot, superfox_retry); the residual comes back through sfrx_core;
! - c0 and dd are allocated once and kept instead of being automatic arrays. jtdxjt9 is built
!   with OpenMP, which puts every local array on the stack, and these two alone are 2.2 MB of
!   the main thread's 8 MB. Kept, not reallocated: four2a caches its FFTW plans by array address
!   and stops the program at 2100 of them, so the address has to be the same in every period;
! - ldecoded says whether a transmission was decoded (its CRC and its SNR both passed).
! Everything that decides a decode - sfox_ana, sfox_remove_tone, qpc_decode2 and its depth,
! threshold and damping - is the donor's, unchanged.

  use sfox_mod

  integer*2 iwave(NMAX)
  logical ldecoded
  real, allocatable, save :: dd(:)

  if(.not.allocated(dd)) allocate(dd(NMAX))
  dd=iwave
  call sfrx_core(nutc,nfqso,ntol,dd,ldecoded)

  return
end subroutine sfrx_sub

subroutine sfrx_core(nutc,nfqso,ntol,dd,ldecoded)

! The receiver proper, on 15 s of samples as REALS on the 16-bit scale: what sfrx_sub makes of a
! recording, or the band with the FT8 signals taken out of it - which is not rounded to integers
! a second time on its way here.

  use sfox_mod

  real dd(NMAX)
  logical ldecoded
  integer*1 xdec(0:49)
  character*13 foxcall
  complex, allocatable, save :: c0(:)  !Complex form of signal as received
  logical crc_ok

  if(.not.allocated(c0)) allocate(c0(NMAX))
  call sfox_config                     !CE3TSK: the receiver's settings, once (sfox_mod)

  fsync=nfqso
  ftol=ntol
  fsample=12000.0
  call sfox_init(7,127,50,'no',fspread,delay,fsample,24)
  npts=15*12000

  call sfox_ana(dd,npts,c0,npts)

  call sfox_remove_tone(c0,fsync)  ! Needs testing

  ndepth=3
  dth=0.5
  damp=1.0

  call qpc_decode2(c0,fsync,ftol, xdec,ndepth,dth,damp,crc_ok,   &
       snrsync,fbest,tbest,snr)
  ldecoded=crc_ok
  if(crc_ok) then
     nsnr=nint(snr)
     nsignature = 1
     call sfox_unpack(nutc,xdec,nsnr,fbest-750.0,tbest,foxcall,nsignature)
! CE3TSK: a Fox decoded ABOVE the floor is a Fox the receiver knows from here on (qpc_decode2) -
! by the ordinary search: an a-priori decode is of a Fox already known, and teaches nothing -
! but ANY decode of the remembered Fox says it is still there (sfox_age)
     if(snr.ge.-16.5 .and. .not.lsfoxap) call sfox_remember(xdec,foxcall)
     call sfox_seen(xdec)
  endif

  return
end subroutine sfrx_core

subroutine sfox_known(call0,islot)

! CE3TSK 2026-09-20: tells the receiver a Fox call it may trust (sfox_mod has what for). islot 1:
! the operator's DX call, set by the decoder before every Fox slot - blank clears it. (Slot 2, the
! Fox this receiver last decoded above its acceptance floor, is sfox_remember's.)
! A Fox is never one of pack28's special tokens: "QRZ", "CQ DX", "DE ABC" in DX Call pack to 1, 2
! and 0 - and 0 is what the first 28 bits of the all-zero word read, which digital silence decodes
! to on every one of its 796 tries (its CRC does not pass; this is belt and braces). Lower case is
! taken as upper case: the GUI sends upper case, file mode's -x may not.

  use packjt77, only : pack28     !pack28 is a module procedure in jtdx; ONLY it: the module also exports a loop
                                  !variable i by accident (its nthrindex initialiser), which an implicitly
                                  !typed unit would then share with every other user of the module (review 2)
  use sfox_mod, only : nfoxknown28,nfoxknown58
  parameter (NTOKENS=2063592)
  character*(*) call0
  character*13 c13
  integer*8 n58
  logical ok

  if(islot.lt.1 .or. islot.gt.2) return
  nfoxknown28(islot)=-1; nfoxknown58(islot)=-1
  c13=adjustl(call0)
  do i=1,13
     if(c13(i:i).ge.'a' .and. c13(i:i).le.'z') c13(i:i)=char(ichar(c13(i:i))-32)
  enddo
  if(len_trim(c13).lt.3) return
  call sfox_call58(c13,n58,ok)
  if(.not.ok) return
  call pack28(c13,n28,0)                       !as sfox_pack writes the call of every message but a CQ
  if(n28.lt.NTOKENS) return
  nfoxknown28(islot)=n28; nfoxknown58(islot)=n58

  return
end subroutine sfox_known

subroutine sfox_call58(c13,n58,ok)

! a call as sfox_pack writes it into a CQ: eleven characters, left justified, base 38

  character*13 c13
  character*38 c
  integer*8 n58
  logical ok
  data c/' 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/'/

  ok=.false.
  n58=0
  do i=1,11
     j=index(c,c13(i:i))
     if(j.lt.1) return
     n58=n58*38 + j - 1
  enddo
  ok=.true.

  return
end subroutine sfox_call58

subroutine sfox_remember(x,foxcall)

! CE3TSK 2026-09-20 (review): the Fox just decoded ABOVE the acceptance floor becomes the Fox the
! receiver knows (slot 2). Its call is taken in the form the MESSAGE carried it - the 28 bits of
! every message but a CQ, the 58 of a CQ - and not from the text: a compound Fox prints as
! "<VP2X/K1JT>" in its ordinary messages, and a test for "<" - the first version - never remembered
! such a Fox at all except from a CQ-only period, which a busy one does not send. The OTHER form is
! worked out from the text when the text is a call (brackets off; an unresolved "<...>" is not),
! and left empty otherwise - so a new Fox always replaces the old one in both forms.

  use packjt77, only : pack28     !only it: see sfox_known
  use sfox_mod, only : nfoxknown28,nfoxknown58,nfoxgrid15,nsfoxage
  parameter (NTOKENS=2063592)
  integer*1 x(0:49)
  integer*8 n58was
  character*13 foxcall,c13
  character*329 msgbits
  integer*8 n58
  logical ok

  n28was=nfoxknown28(2); n58was=nfoxknown58(2)
  nfoxknown28(2)=-1; nfoxknown58(2)=-1; nsfoxage=0
  write(msgbits,'(47b7.7)') x(0:46)
  read(msgbits(327:329),'(b3)') i3
  c13=' '; k=0
  do i=1,13
     if(foxcall(i:i).eq.'<' .or. foxcall(i:i).eq.'>') cycle
     k=k+1; c13(k:k)=foxcall(i:i)
  enddo
  c13=adjustl(c13)
  ok=len_trim(c13).ge.3 .and. index(c13,'.').eq.0
  if(i3.eq.3) then
     read(msgbits(1:58),'(b58)') n58
     nfoxknown58(2)=n58
     if(ok) then
        call pack28(c13,n28,0)
        if(n28.ge.NTOKENS) nfoxknown28(2)=n28
     endif
  else
     read(msgbits(1:28),'(b28)') n28
     if(n28.ge.NTOKENS) nfoxknown28(2)=n28
     if(ok) then
        call sfox_call58(c13,n58,ok)
        if(ok) nfoxknown58(2)=n58
     endif
  endif
! its GRID, for the a-priori pass's "CQ <Fox> <grid>" (qpc_decode2): what a CQ says, kept while the
! Fox stays the same one, gone with it. The 15 bits AS THEY ARE: a Fox that calls "CQ FOX" with no
! grid (MSHV lets it) sends 32401 there every time, and the pass needs the bits, not their meaning.
  if(.not.((n28was.ge.0 .and. n28was.eq.nfoxknown28(2)) .or. (n58was.ge.0 .and. n58was.eq.nfoxknown58(2)))) nfoxgrid15(2)=-1
  if(i3.eq.3) then
     read(msgbits(59:73),'(b15)') n15
     nfoxgrid15(2)=n15
  endif

  return
end subroutine sfox_remember

subroutine sfox_grid(grid0)

! CE3TSK 2026-09-20: the operator's DX grid, as a CQ of that Fox would carry it (slot 1 of
! sfox_mod's nfoxgrid15; the decoder calls this before every Fox slot, after sfox_known). Four
! characters AA00-RR99 or nothing: packgrid turns anything else into a report or a text code.

  use packjt, only : packgrid
  use sfox_mod, only : nfoxgrid15
  character*(*) grid0
  character*4 g
  logical text

  nfoxgrid15(1)=-1
  if(len(grid0).lt.4) return
  g=grid0(1:4)
  do i=1,2
     if(g(i:i).ge.'a' .and. g(i:i).le.'r') g(i:i)=char(ichar(g(i:i))-32)
     if(g(i:i).lt.'A' .or. g(i:i).gt.'R') return
     if(g(i+2:i+2).lt.'0' .or. g(i+2:i+2).gt.'9') return
  enddo
  call packgrid(g,n15,text)
  if(.not.text .and. n15.ge.0 .and. n15.lt.32400) nfoxgrid15(1)=n15

  return
end subroutine sfox_grid

subroutine sfox_me(call0,nprog)

! CE3TSK 2026-09-20: my own call and what the QSO lets a Fox send me, for the a-priori pass
! (sfox_mod: nsfme28, nsfprog; the decoder calls this before every Fox slot). nprog 0: nothing is
! expected, and the call is not kept either.

  use packjt77, only : pack28     !only it: see sfox_known
  use sfox_mod, only : nsfme28,nsfprog
  parameter (NTOKENS=2063592)
  character*(*) call0
  character*13 c13

  nsfme28=-1; nsfprog=0
  if(nprog.lt.1) return
  c13=adjustl(call0)
  do i=1,13
     if(c13(i:i).ge.'a' .and. c13(i:i).le.'z') c13(i:i)=char(ichar(c13(i:i))-32)
  enddo
  if(len_trim(c13).lt.3) return
  call pack28(c13,n28,0)
  if(n28.lt.NTOKENS) return                     !"CQ", "QRZ", "DE": not a call (sfox_known)
  nsfme28=n28; nsfprog=min(nprog,2)

  return
end subroutine sfox_me

subroutine sfox_forget

! CE3TSK 2026-09-20: a band or mode change. What the receiver REMEMBERS of a Fox - the Fox last
! decoded above the floor and its grid, slot 2 - belongs to the frequency it was heard on, as the
! FT8 and FT4 hint lists do, and goes where they go (decoder.f90). Slot 1, the operator's DX call
! and grid, is set anew before every Fox slot.

  use sfox_mod, only : nfoxknown28,nfoxknown58,nfoxgrid15,nsfoxage,nsfpool,nsfpoolfox

  nfoxknown28(2)=-1; nfoxknown58(2)=-1; nfoxgrid15(2)=-1; nsfoxage=0
  nsfpool=0; nsfpoolfox=-1             !and the Hounds heard: they were heard on this frequency

  return
end subroutine sfox_forget

subroutine sfox_age

! CE3TSK 2026-09-20 (review): the remembered Fox AGES. The decoder calls this once before every Fox
! slot; four slots (the setting JTDX_SFOX_APAGE) without a decode of that Fox - two minutes, the
! depth of the FT8 hint memory - and it is forgotten. A band or mode change forgets it at once (sfox_forget), but the GUI reports
! a band change only when the BAND changes: after a QSY within the band to another DXpedition the
! old Fox and its grid would otherwise stay known on a frequency that is not theirs, for good.
! (The DX call, slot 1, does not age: the operator keeps it.)

  use sfox_mod, only : nfoxknown28,nfoxknown58,nfoxgrid15,nsfoxage,nsfapage,sfox_config

  call sfox_config                     !the age is a setting: JTDX_SFOX_APAGE, default 4
  if(nfoxknown28(2).lt.0 .and. nfoxknown58(2).lt.0) return
  nsfoxage=nsfoxage+1
! the remembered Fox and its grid ONLY - not sfox_forget, which is the band change's and empties the
! pool of Hounds as well: the pool is the DX call's, which does not age (review: a remembered Fox
! reaching its age wiped Hounds heard fifteen seconds earlier, in the slot they were heard for)
  if(nsfoxage.gt.nsfapage) then
     nfoxknown28(2)=-1; nfoxknown58(2)=-1; nfoxgrid15(2)=-1; nsfoxage=0
  endif

  return
end subroutine sfox_age

subroutine sfox_pool_line(decoded,hiscall0,mycall0)

! CE3TSK 2026-09-20: one FT8 decode of an odd slot in S-Hound mode (decoder.f90, ft8_decoded; the
! caller holds the lock - several FT8 threads decode). "<Fox> <Hound> <grid>" and "<Fox> <Hound>
! R-nn" with the Fox in DX Call: that Hound is one the Fox may name in its next messages - into the
! pool with it, or its age back to zero. Not the Fox itself and not MY call as the second word (my
! call joins the candidates by the QSO's rule, sfox_poolpass; review), nothing pack28 does not
! take as a call ("CQ", "QRZ", a hashed <...>). Case is folded on both sides: FT8 prints upper
! case, file mode's -x may not (review). The pool is ONE Fox's: a Hound answering another DX call
! than the pool was filled for empties it first (the operator changed DX Call without a band
! change). A full pool gives up the Hound absent for longest.
! Known limit: the Hounds of a Fox with a COMPOUND call print as "<...> W0AAA R-10" until the
! Fox's call is in the hash table, so nothing is pooled before the first Fox slot of a session.

  use packjt77, only : pack28     !only it: see sfox_known
  use sfox_mod, only : nsfpool,nsfpool28,nsfpoolold,NSFPOOLMAX,nsfpoolage,nsfpoolfox
  parameter (NTOKENS=2063592)
  character*(*) decoded,hiscall0,mycall0
  character*13 w1,w2,fox,me
  character*26 d

  if(nsfpoolage.lt.1) return
  d=decoded; fox=adjustl(hiscall0); me=adjustl(mycall0)
  call sfox_upper(d); call sfox_upper(fox); call sfox_upper(me)
  if(len_trim(fox).lt.3) return
  i1=index(d,' ')
  if(i1.lt.4 .or. i1.gt.14) return
  w1=d(1:i1-1)
  if(w1.ne.fox) return
  i2=index(d(i1+1:),' ')
  if(i2.lt.4 .or. i2.gt.14) return
  w2=d(i1+1:i1+i2-1)
  if(index(w2,'<').gt.0 .or. index(w2,'.').gt.0 .or. index(w2,';').gt.0) return
  if(w2.eq.fox .or. w2.eq.me) return
! a call has a letter AND a digit: "K1JT RRR TNX", "K1JT 5NN TU", "K1JT RR73; W0AAA <...> -10" are
! lines to the Fox whose second word pack28 would take as the hash of a non-standard call - pool
! places given to nobody (review 2)
  if(scan(w2,'0123456789').eq.0 .or. scan(w2,'ABCDEFGHIJKLMNOPQRSTUVWXYZ').eq.0) return
  if(w2.eq.'RR73' .or. w2.eq.'5NN' .or. w2(1:2).eq.'R+' .or. w2(1:2).eq.'R-') return
  call pack28(w2,n28,0)
  if(n28.lt.NTOKENS) return
  call pack28(fox,nfox,0)
  if(nfox.lt.NTOKENS) return
  if(nfox.ne.nsfpoolfox) then
     nsfpool=0; nsfpoolfox=nfox
  endif
  iold=1
  do i=1,nsfpool
     if(nsfpool28(i).eq.n28) then
        nsfpoolold(i)=0
        return
     endif
     if(nsfpoolold(i).gt.nsfpoolold(iold)) iold=i
  enddo
  if(nsfpool.lt.NSFPOOLMAX) then
     nsfpool=nsfpool+1; iold=nsfpool
  endif
  nsfpool28(iold)=n28; nsfpoolold(iold)=0

  return
end subroutine sfox_pool_line

subroutine sfox_upper(c)

  character*(*) c

  do i=1,len(c)
     if(c(i:i).ge.'a' .and. c(i:i).le.'z') c(i:i)=char(ichar(c(i:i))-32)
  enddo

  return
end subroutine sfox_upper

subroutine sfox_odd_listened

! CE3TSK 2026-09-20: an odd slot of S-Hound mode has reached the FT8 decoder - it was LISTENED to
! (decoder.f90, at the gate). That is the pool's clock: every Hound in it is one slot older, the
! decodes of this slot then put those heard back to zero (sfox_pool_line), and who was absent from
! more than JTDX_SFOX_POOLAGE such slots is out. While I transmit in the odd slots none of them is
! listened to, no Hound CAN be heard, and the pool stands as it was (review: counted in Fox
! slots it would have been empty two minutes into my own calling).

  use sfox_mod, only : nsfpool,nsfpool28,nsfpoolold,nsfpoolage,sfox_config

  call sfox_config
  k=0
  do i=1,nsfpool
     nsfpoolold(i)=nsfpoolold(i)+1
     if(nsfpoolold(i).gt.nsfpoolage) cycle
     k=k+1; nsfpool28(k)=nsfpool28(i); nsfpoolold(k)=nsfpoolold(i)
  enddo
  nsfpool=k

  return
end subroutine sfox_odd_listened

subroutine sfox_seen(x)

! any decode that carries the remembered Fox - above the floor, under it, a-priori - is a sign of
! life: its age starts again

  use sfox_mod, only : nfoxknown28,nfoxknown58,nsfoxage
  integer*1 x(0:49)
  character*329 msgbits
  integer*8 n58

  write(msgbits,'(47b7.7)') x(0:46)
  read(msgbits(327:329),'(b3)') i3
  if(i3.eq.3) then
     read(msgbits(1:58),'(b58)') n58
     if(nfoxknown58(2).ge.0 .and. n58.eq.nfoxknown58(2)) nsfoxage=0
  else
     read(msgbits(1:28),'(b28)') n28
     if(nfoxknown28(2).ge.0 .and. n28.eq.nfoxknown28(2)) nsfoxage=0
  endif

  return
end subroutine sfox_seen
