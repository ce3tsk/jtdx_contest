module jt65_mod9 
! stores callsign DB in memory for false decodes checking
  
  parameter (MAXC=200000)
! supporting 7-char Australian callsigns
  character*7 call0c(115000),calld(111000),callef(90000),callgh(75000),calli(76000),callj(90000),callk(240000),calllm(65000), &
              calln(94000),callo(67000),callpq(58000),callr(65000),callst(75000),calluv(104000),callw(137000),callxz(58000)
  character callsign*12
  character*180 line
  integer ncall0c,ncalld,ncallef,ncallgh,ncalli,ncallj,ncallk,ncalllm,ncalln,ncallo,ncallpq,ncallr,ncallst,ncalluv, &
          ncallw,ncallxz
  logical ldbvalid
  data ldbvalid/.true./
  data ncall0c,ncalld,ncallef,ncallgh,ncalli,ncallj,ncallk,ncalllm,ncalln,ncallo,ncallpq,ncallr,ncallst,ncalluv,ncallw, &
       ncallxz/0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0/

end module jt65_mod9