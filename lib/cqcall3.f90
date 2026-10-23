! This source code file was last time modified by Igor UA3DJY on 20181215
! All changes are shown in the patch file coming together with the full JTDX source code.

subroutine cqcall3()

  use jt65_mod10  ! used to share stored messages
  use packjt
  use call3list   ! CE3TSK 2026-10-23
  use prog_args
 
  integer dgen(12),sym_rev(0:62)
  character*6, allocatable :: call2(:)   ! CE3TSK 2026-10-23: the list's own size, on the heap (call3list.f90)
  character*4, allocatable :: grid2(:)
  character msg*22

     call read_call3 (trim (data_dir)//'/CALL3.TXT', call2, grid2, ncalls)   ! CE3TSK 2026-10-23 (TODO 3.-199, call3list.f90)
     if(first .and. ncalls.lt.1) then
        print *, 'copy CALL3.TXT file to log directory'
        print *, 'https://ce3tsk.com'
        first=.false.
!        stop 'CALL3.TXT is too short or missed?'
     endif

     if(ncalls.eq.0) then
        nused=0
        return
     endif
     if (allocated (sym2)) deallocate (sym2, msg0)
     allocate (sym2(0:62,ncalls+1), msg0(ncalls+1))   ! +1: chkftrsd's decode (nused+1)
     j=0
     do i=1,ncalls
        j=j+1
        msg='CQ '//call2(i)//' '//grid2(i)
        call fmtmsg(msg,iz)
        call packmsg(msg,dgen,itype)            !Pack message into 72 bits
        call rs_encode(dgen,sym_rev)            !RS encode
        call interleave63(sym_rev,1)            !Interleave channel symbols
        call graycode(sym_rev,63,1,sym_rev)     !Apply Gray code
        sym2(0:62,j)=sym_rev(0:62)
        msg0(j)=msg
     enddo
     nused=j

return
end subroutine cqcall3
