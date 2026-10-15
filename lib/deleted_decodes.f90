module deleted_decodes
! CE3TSK 2026-10-12: the decodes the prefix-to-grid table deleted, kept for the program's record of them.
!
! chkgrid's table says from which squares a prefix can be heard, and which prefixes are not allocated at all.
! A decode that fails it is deleted in eleven places - seven in chkfalse8.f90, the filter FT8, FT4 and FT2
! share, two more that all three decoders call (chkprefix.f90; FT8 alone had them until 2026-10-13) and
! two in ft8b.f90 - and nothing told anybody. A station whose prefix or square the table has wrong lost every weak CQ unseen: on the
! day this was written the table accepted no square at all for the Azores (CU) and refused three squares
! that sixty stations were on the air from, and it took a decoder built for the purpose to see it.
!
! With JTDX_REPORT_DELETED=1 in the environment - the program sets it for its decoder - each such
! deletion is printed once, at the end of the phase that found it, in a decode line's own columns:
!     <DecodeDeleted>hhmmss snr   dt freq c message                               why
! c is the mode's character (~ FT8, : FT4, ; FT2). why is "grid/n" - the square is not in the table for
! that prefix - or "prefix/n" - the prefix is not allocated; n is the place that deleted it:
!     1-7   chkfalse8.f90, in the order of the file (weak or a-priori decodes, all three modes)
!     8     chkdircq, all three modes: a directed CQ ("CQ TEST K1ABC FN42") at any strength (the prefix
!           only: the square of a directed CQ is not judged by the decoder, the window marks it - but for
!           place 2, which judges a weak one whose word is a number and whose call ends in /R or /P); a
!           call that is an unresolved hash with a grid is deleted too, as prefix/8, after a CQ word of any
!           length and of types i3=1 and i3=2 ("CQ DX <...> FN42", "CQ WW <...> FN42", "CQ TEST <...> FN42";
!           a plain "CQ <...>" never arrives - unpack77 refuses it)
!     9     chkrgrid, all three modes: "CALL1 CALL2 R GRID" (the prefix only; so are places 4 and 5 for
!           this form since 2026-10-13, but for a second call that is an unresolved hash, judged on its
!           square as before - place 6, the station being worked, judges its square as before too)
!     10    ft8b.f90, a hashed call followed by a call and a grid (the prefix only)
!     11    ft8b.f90, a rover call whose base call has six characters or more ("CQ KA1ABC/R FN42"); a
!           shorter one ("CQ K1ABC/R FN42") is not looked at there, and is judged only when weak or
!           a-priori (places 1 and 3)
! The program writes the lines to <month>_DELETED.TXT beside ALL.TXT (deletedlog.h). Without the variable
! nothing is kept and nothing is printed: file mode and every expectation set are as they were.
!
! NOT reported: a message deleted on one attempt and decoded on another in the same period. The decoder
! tries a signal several times - another candidate, a later pass, the other side of the filter's SNR gate -
! and what reached the screen was not lost. The decode callbacks tell this module what was shown; that
! list lives for the period, the TX background included, and a message once reported joins it - the
! background decodes the same band again and deletes the same messages, and one line a period is enough.
! One case remains: a message the RX phase deleted and the TX background decodes afterwards is in both
! files, because the RX phase's lines are out by then.
!
! Determinism (SESSION_RULES 8, 9b): a slice writes its own column and nothing else, and the lines are
! printed after the parallel loops, in slice order - the same lines whichever thread finished first.
  implicit none
  private
  public :: ldelreport,ldelquiet,deleted_newperiod,deleted_note,deleted_shown,deleted_emit

  integer, parameter :: NDELMAX=16      ! a slice's deletions in one phase - measured: 25 a DAY on a busy band
  integer, parameter :: NDELSLICES=48   ! as many slices as the decoders ever cut (ft8_mod1's nmaxthreads)
  integer, parameter :: NSHOWNMAX=400   ! the period's printed messages; past that a line is no longer remembered

  type deleted_struct
    character(len=37) :: msg=''
    integer :: nsnr=0, nfreq=0, nsite=0
    real :: xdt=0.
  end type deleted_struct
  type(deleted_struct), save :: deleted(NDELMAX,NDELSLICES)
  integer, save :: ndeleted(NDELSLICES)=0
  character(len=26), save :: shown(NSHOWNMAX)   ! the period's messages that are accounted for: printed, or reported here
  integer, save :: nshown=0
  logical, save :: ldelreport=.false.   ! JTDX_REPORT_DELETED=1, read by decoder.f90 before any slice runs
  logical, save :: ldelquiet=.false.    ! the silent FT8 decode of a SuperFox slot: it shows nothing, so it reports nothing

contains

! a new period's receive phase: nothing shown yet, nothing pending, and not the silent decode
  subroutine deleted_newperiod()
    nshown=0; ndeleted=0; ldelquiet=.false.
  end subroutine deleted_newperiod

! the table deleted msg in slice nslice. nsite: the place, positive for the square, negative for the prefix.
! The same signal comes more than once - each pass and each candidate tries it: one entry, the strongest.
  subroutine deleted_note(nslice,msg,nsnr,xdt,nfreq,nsite)
    integer, intent(in) :: nslice,nsnr,nfreq,nsite
    real, intent(in) :: xdt
    character(len=*), intent(in) :: msg
    integer :: i,n
    if(.not.ldelreport .or. ldelquiet) return
    if(nslice.lt.1 .or. nslice.gt.NDELSLICES .or. nsite.eq.0) return
    n=ndeleted(nslice)
    do i=1,n
      if(deleted(i,nslice)%msg.eq.msg) then
        if(nsnr.gt.deleted(i,nslice)%nsnr) then
          deleted(i,nslice)%nsnr=nsnr; deleted(i,nslice)%xdt=xdt; deleted(i,nslice)%nfreq=nfreq
          deleted(i,nslice)%nsite=nsite
        endif
        return
      endif
    enddo
    if(n.ge.NDELMAX) return             ! a full column takes no more
    n=n+1
    deleted(n,nslice)%msg=msg; deleted(n,nslice)%nsnr=nsnr; deleted(n,nslice)%xdt=xdt
    deleted(n,nslice)%nfreq=nfreq; deleted(n,nslice)%nsite=nsite
    ndeleted(nslice)=n
  end subroutine deleted_note

! a decode line went out: decoded is on the screen, whatever another attempt at it came to
  subroutine deleted_shown(decoded)
    character(len=*), intent(in) :: decoded
    if(.not.ldelreport) return
!$omp critical(deleted_shown_list)
    if(nshown.lt.NSHOWNMAX) then
      nshown=nshown+1; shown(nshown)=decoded
    endif
!$omp end critical(deleted_shown_list)
  end subroutine deleted_shown

! the end of a phase, after its parallel loops and its decode lines: print what is pending, slice by slice,
! and forget it. A message that was shown is dropped; one that two slices deleted is printed once.
  subroutine deleted_emit(nutc,csep)
    integer, intent(in) :: nutc
    character(len=1), intent(in) :: csep
    integer :: i,j,k,kk
    logical :: ldrop
    character(len=12) :: why
    if(ldelreport) then
      do k=1,NDELSLICES
        do i=1,ndeleted(k)
          ldrop=.false.
          do j=1,nshown
            if(shown(j).eq.deleted(i,k)%msg(1:26)) then; ldrop=.true.; exit; endif
          enddo
          if(.not.ldrop) then
            do kk=1,k-1
              do j=1,ndeleted(kk)
                if(deleted(j,kk)%msg.eq.deleted(i,k)%msg) ldrop=.true.
              enddo
            enddo
          endif
          if(ldrop) cycle
          if(deleted(i,k)%nsite.gt.0) then; write(why,'(a,i0)') 'grid/',deleted(i,k)%nsite
          else; write(why,'(a,i0)') 'prefix/',-deleted(i,k)%nsite
          endif
          write(*,1000) nutc,deleted(i,k)%nsnr,deleted(i,k)%xdt,deleted(i,k)%nfreq,csep,deleted(i,k)%msg,trim(why)
1000      format('<DecodeDeleted>',i6.6,i4,f5.1,i5,1x,a1,1x,a37,1x,a)
          if(nshown.lt.NSHOWNMAX) then      ! reported: the period's later phases say nothing more of it
            nshown=nshown+1; shown(nshown)=deleted(i,k)%msg(1:26)
          endif
        enddo
      enddo
      call flush(6)
    endif
    ndeleted=0
  end subroutine deleted_emit

end module deleted_decodes
