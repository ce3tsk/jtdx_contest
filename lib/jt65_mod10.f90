! This source code file was last time modified by Igor UA3DJY on February 15th, 2017
! All changes are shown in the patch file coming together with the full JTDX source code.

module jt65_mod10 
! used to store last callsigns seen in JT modes in the memory
! used in some Hint decoders and for false FTRSD decodes checking

! CE3TSK 2026-10-23 (TODO 3.-199): as many as CALL3.TXT's calls, + 1 for chkftrsd's decode (cqcall3, call3list.f90);
! were MAXCALLS=35000 lines
  integer*1, allocatable :: sym2(:,:)
  integer nused
  logical(1) first
  character*22, allocatable :: msg0(:)
  data first/.true./

  save first,nused,msg0,sym2

end module jt65_mod10