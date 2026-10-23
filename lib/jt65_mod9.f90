module jt65_mod9 
! stores callsign DB in memory for false decodes checking
  
! CE3TSK 2026-10-22 (TODO.md 3.-185): the lines allcall7_read () reads at most, comments included - 200000 until then,
! which a 222,041-call file (JP1LRT's) outgrew: the reader stopped in the W calls and the X-Z table stayed empty
  parameter (MAXC=500000)
! supporting 7-char Australian callsigns
! CE3TSK 2026-10-22 (TODO.md 3.-185): allocated by allcall7_read () only when the file is used (sizes there) - off, the
! 16 tables (1.52 M calls, 10.6 MB) take no memory; they were fixed arrays
  character*7, allocatable :: call0c(:),calld(:),callef(:),callgh(:),calli(:),callj(:),callk(:),calllm(:), &
                              calln(:),callo(:),callpq(:),callr(:),callst(:),calluv(:),callw(:),callxz(:)
  character callsign*12
  character*180 line
  integer ncall0c,ncalld,ncallef,ncallgh,ncalli,ncallj,ncallk,ncalllm,ncalln,ncallo,ncallpq,ncallr,ncallst,ncalluv, &
          ncallw,ncallxz
  logical ldbvalid
  data ldbvalid/.false./   ! CE3TSK 2026-10-22 (3.-185): valid only once allcall7_read () has filled every table
  data ncall0c,ncalld,ncallef,ncallgh,ncalli,ncallj,ncallk,ncalllm,ncalln,ncallo,ncallpq,ncallr,ncallst,ncalluv,ncallw, &
       ncallxz/0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0/
! CE3TSK 2026-10-22 (TODO.md 3.-185): whether ALLCALL7.TXT is used at all - LALLCALL7DEFAULT the default, JTDX_ALLCALL7=0
! or 1 overriding it at the first decode (allcall7_read ()). Off, the file is not read and its tables are not allocated:
! ldbvalid is false and searchcalls () finds every call, as JTDX without the file - the /R a-priori check (ft8b.f90),
! the JT65 filters (chkfalse.f90) and the gated chkflscall () all accept. The operator: on by default until measured;
! measured off (FT8 the same, JT65 4 real decodes of 200 more), then "all the allcall7.txt file usage should be off by
! default, i.e. by default it should not be read and no memory should be used" - its code to go later, maybe.
  logical, parameter :: LALLCALL7DEFAULT=.false.
  logical :: luseallcall7=LALLCALL7DEFAULT
! (3.-185) calls the reader left out because their table was full, and whether it stopped at MAXC lines before the end
  integer :: nallcall7skip=0
  logical :: lallcall7cut=.false.

end module jt65_mod9