subroutine sfox_pctile(x,npts,npct,xpct)

! CE3TSK 2026-09-20: the npct-th percentile of x - the SAME value lib/pctile.f90 returns, the j-th
! smallest with j=nint(npts*0.01*npct) - found by selection (Hoare's FIND) instead of a Shell
! sort of all npts values. The SuperFox receiver asks for the median of 16384 and of 19456
! spectrum values twelve times for every time/frequency step of its search, 1200 times in a slot
! where a false sync sends it through all hundred steps: measured with callgrind, 62 % of that
! slot's two seconds were the sort. An order statistic is one value whatever finds it, so the
! decodes are unchanged - test/decode/superfox.sh compares them with the donor's, file by file.

  parameter (NMAX=32768)
  real*4 x(npts)
  real*4 tmp(NMAX)
  real*4 xpct,pivot,t
  integer l,r,i,j,k

  if(npts.le.0) then
     xpct=1.0
     return
  endif
  if(npts.gt.NMAX) stop 'sfox_pctile: more than 32768 points'

  tmp(1:npts)=x
  k=nint(npts*0.01*npct)
  if(k.lt.1) k=1
  if(k.gt.npts) k=npts

  l=1
  r=npts
  do while(l.lt.r)
     pivot=tmp(k)
     i=l
     j=r
     do
        do while(tmp(i).lt.pivot)
           i=i+1
        enddo
        do while(pivot.lt.tmp(j))
           j=j-1
        enddo
        if(i.le.j) then
           t=tmp(i); tmp(i)=tmp(j); tmp(j)=t
           i=i+1
           j=j-1
        endif
        if(i.gt.j) exit
     enddo
     if(j.lt.k) l=i
     if(k.lt.i) r=j
  enddo
  xpct=tmp(k)

  return
end subroutine sfox_pctile
