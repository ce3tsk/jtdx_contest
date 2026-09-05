subroutine osd174_91(llr,apmask,ndeep,message77,cw,nhardmin,dmin,nthr)
!
! An ordered-statistics decoder for the (174,91) code.
!
use ft8_mod1, only : first_osd,gen
integer, parameter:: N=174, K=91, M=N-K
integer*1 apmask(N),apmaskr(N)
integer*1 misub(K)
integer indices(N)
integer*1 cw(N),hdec(N)
integer*1 decoded(K)
integer*1 message77(77)
integer indx(N)
real llr(N),rx(N),absrx(N)
integer*8 grow(0:2,K),hw(0:2),apw(0:2),mskK(0:2),mskP(0:2),mskT(0:2),m0w(0:2),c0w(0:2),cww(0:2)   ! CE3TSK packed rows
integer*8 xw(0:2),msw(0:2),miw(0:2),mew(0:2),cew(0:2),cebw(0:2),e2sw(0:2),e2w(0:2)
integer iw1,ib1,iw2,ib2,ic
real osd_wsum
integer osd_bitof
include "ldpc_174_91_c_generator.f90"
logical reset

integer, DIMENSION(:,:), ALLOCATABLE :: indexes
integer, DIMENSION(:), ALLOCATABLE :: fp
integer, DIMENSION(:), ALLOCATABLE :: np
integer, DIMENSION(:), ALLOCATABLE :: tl   ! CE3TSK: tail of each hash bucket
integer*8 keymask

interface
  subroutine boxit91p(indexes,fp,np,tl,reset,ipat,npindex,i1,i2)
    integer indexes(:,:),fp(0:),np(:),tl(0:)
    integer ipat,npindex,i1,i2
    logical reset
  end subroutine boxit91p
  subroutine fetchit91p(indexes,fp,np,reset,ipat,i1,i2,nthr)
    integer indexes(:,:),fp(0:),np(:)
    integer ipat,i1,i2,nthr
    logical reset
  end subroutine fetchit91p
end interface

npre1=0; npre2=0; d1=0.
if(first_osd) then ! fill the generator matrix
!$omp critical(first_osd)
! CE3TSK: re-check inside the critical section. Two threads that both saw first_osd true at
! process start both got in here; the second one re-zeroed and re-filled gen while a third
! thread, which had already seen the flag false, was reading it - one OSD call in that
! thread then ran on a zeroed matrix and lost a decode, a rare first-decode non-determinism.
  if(first_osd) then
  gen=0
  do i=1,M
    do j=1,23
      read(g(i)(j:j),"(Z1)") istr
      ibmax=4
      if(j.eq.23) ibmax=3
      do jj=1, ibmax 
        irow=(j-1)*4+jj
        if( btest(istr,4-jj) ) gen(irow,K+i)=1 
      enddo
    enddo
  enddo
  do irow=1,K
    gen(irow,irow)=1
  enddo
!$omp flush
first_osd=.false.
endif
!$omp end critical(first_osd)
endif

rx=llr

! Hard decisions on the received word.
hdec=0
where(rx .ge. 0) hdec=1

! Use magnitude of received symbols as a measure of reliability.
absrx=abs(rx)
call indexx(absrx,N,indx)

! CE3TSK: from here on the GF(2) work is done on bit-packed rows - three 64-bit words per
! 174-bit row - instead of one byte per bit. Row XORs, column swaps, pivot tests, encoding
! and hard-error counts are word operations (popcnt for the counts); their results cannot
! differ from the byte version. The soft distances are still single-precision sums of
! absrx over the set bits taken in increasing index order, which is exactly the value
! sum(nxor*absrx) produced (the zero terms it added change nothing), and the expression on
! the "n1 /= iflag" branch is evaluated in the same left-to-right order as before.
! test/fortran/osd_packed_vs_ref compares this routine with the original bit for bit.

! Re-order the columns of the generator matrix in order of decreasing reliability.
do i=1,N
  indices(i)=indx(N+1-i)
enddo
grow=0_8
do i=1,K
  do ic=1,N
    if(gen(i,indices(ic)).eq.1) grow((ic-1)/64,i)=ibset(grow((ic-1)/64,i),mod(ic-1,64))
  enddo
enddo

! Gaussian elimination to put the most reliable received bits in positions 1:K.
do id=1,K ! diagonal element indices
  do icol=id,K+20  ! The 20 is ad hoc - beware
    if( btest(grow((icol-1)/64,id),mod(icol-1,64)) ) then
      if( icol .ne. id ) then ! reorder column: swap bits id and icol in every row
        iw1=(id-1)/64; ib1=mod(id-1,64); iw2=(icol-1)/64; ib2=mod(icol-1,64)
        do ii=1,K
          if( btest(grow(iw1,ii),ib1) .neqv. btest(grow(iw2,ii),ib2) ) then
            grow(iw1,ii)=ieor(grow(iw1,ii),ishft(1_8,ib1))
            grow(iw2,ii)=ieor(grow(iw2,ii),ishft(1_8,ib2))
          endif
        enddo
        itmp=indices(id)
        indices(id)=indices(icol)
        indices(icol)=itmp
      endif
      iw1=(id-1)/64; ib1=mod(id-1,64)
      do ii=1,K
        if( ii .ne. id .and. btest(grow(iw1,ii),ib1) ) then
          grow(0,ii)=ieor(grow(0,ii),grow(0,id))
          grow(1,ii)=ieor(grow(1,ii),grow(1,id))
          grow(2,ii)=ieor(grow(2,ii),grow(2,id))
        endif
      enddo
      exit
    endif
  enddo
enddo

! The hard decisions for the K MRB bits define the order 0 message, m0.
hdec=hdec(indices)   ! hard decisions from received symbols
absrx=absrx(indices)
apmaskr=apmask(indices)
hw=0_8; apw=0_8
do i=1,N
  if(hdec(i).eq.1) hw((i-1)/64)=ibset(hw((i-1)/64),mod(i-1,64))
  if(apmaskr(i).eq.1) apw((i-1)/64)=ibset(apw((i-1)/64),mod(i-1,64))
enddo
! bit masks: message bits 1:K, parity bits K+1:N, the first nt parity bits, the first ntau
mskK=0_8; mskP=0_8
do i=1,K; mskK((i-1)/64)=ibset(mskK((i-1)/64),mod(i-1,64)); enddo
do i=K+1,N; mskP((i-1)/64)=ibset(mskP((i-1)/64),mod(i-1,64)); enddo
m0w=iand(hw,mskK)              ! zero'th order message, as a pattern over bits 1:K

call mrbencode91p(m0w,c0w,grow,K)
xw=ieor(c0w,hw)
nhardmin=popcnt(xw(0))+popcnt(xw(1))+popcnt(xw(2))
dmin=osd_wsum(xw,absrx,1,N)
cww=c0w
ntotal=0
nrejected=0

if(ndeep.eq.0) goto 998  ! norder=0
if(ndeep.gt.5) ndeep=5
if( ndeep.eq. 1) then
   nord=1
   npre1=0
   npre2=0
   nt=40
   ntheta=12
elseif(ndeep.eq.2) then
   nord=1
   npre1=1
   npre2=0
   nt=40
   ntheta=12
elseif(ndeep.eq.3) then
   nord=1
   npre1=1
   npre2=1
   nt=40
   ntheta=12
   ntau=14
elseif(ndeep.eq.4) then
   nord=2
   npre1=1
   npre2=0
   nt=40
   ntheta=12
   ntau=19
elseif(ndeep.eq.5) then
   nord=2
   npre1=1
   npre2=1
   nt=40
   ntheta=12
   ntau=19
endif
mskT=0_8
do i=K+1,K+nt; mskT((i-1)/64)=ibset(mskT((i-1)/64),mod(i-1,64)); enddo

do iorder=1,nord
   misub(1:K-iorder)=0
   misub(K-iorder+1:K)=1
   iflag=K-iorder+1
   do while(iflag .ge.0)
      if(iorder.eq.nord .and. npre1.eq.0) then
         iend=iflag
      else
         iend=1
      endif
      call osd_packk(misub,K,msw)
      do n1=iflag,iend,-1
         miw=msw; miw((n1-1)/64)=ibset(miw((n1-1)/64),mod(n1-1,64))
         if(iand(apw(0),miw(0)).ne.0_8 .or. iand(apw(1),miw(1)).ne.0_8) cycle
         ntotal=ntotal+1
         mew=ieor(m0w,miw)
         if(n1.eq.iflag) then
            call mrbencode91p(mew,cew,grow,K)
            cebw=cew                            ! codeword of the pattern with only misub's bits
            e2sw=iand(ieor(cew,hw),mskP)        ! parity-bit errors of it
            e2w=e2sw
            nd1Kpt=popcnt(iand(e2sw(1),mskT(1)))+popcnt(iand(e2sw(2),mskT(2)))+1
            d1=osd_wsum(iand(ieor(mew,hw),mskK),absrx,1,K)
         else
            e2w=ieor(e2sw,iand(grow(:,n1),mskP))
            nd1Kpt=popcnt(iand(e2w(1),mskT(1)))+popcnt(iand(e2w(2),mskT(2)))+2
         endif
         if(nd1Kpt .le. ntheta) then
            if(n1.ne.iflag) cew=ieor(cebw,grow(:,n1))   ! same codeword as re-encoding me
            xw=ieor(cew,hw)
            if(n1.eq.iflag) then
               dd=d1+osd_wsum(e2sw,absrx,K+1,N)
            else
               dd=d1+real(ieor(osd_bitof(cew,n1),int(hdec(n1))))*absrx(n1)
               dd=dd+osd_wsum(e2w,absrx,K+1,N)
            endif
            if( dd .lt. dmin ) then
               dmin=dd
               cww=cew
               nhardmin=popcnt(xw(0))+popcnt(xw(1))+popcnt(xw(2))
               nd1Kptbest=nd1Kpt
            endif
         else
            nrejected=nrejected+1
         endif
      enddo
! Get the next test error pattern, iflag will go negative
! when the last pattern with weight iorder has been generated.
      call nextpat91(misub,k,iorder,iflag)
   enddo
enddo

if(npre2.eq.1) then
   allocate(indexes(5000,2), STAT = nAllocateStatus1)
   if (nAllocateStatus1 .ne. 0) STOP "Not enough memory"
! CE3TSK: the pattern hash table only ever holds 2**ntau entries (16384 at depth 3, 524288 at
! depth 5); it was 525001 long and reset to -1 in full on every call - a 2 MB memset per
! OSD call was most of OSD's own time. Unused entries are never read, so this is exact.
   allocate(fp(0:2**ntau-1), STAT = nAllocateStatus1)
   if (nAllocateStatus1 .ne. 0) STOP "Not enough memory"
   allocate(np(5000), STAT = nAllocateStatus1)
   if (nAllocateStatus1 .ne. 0) STOP "Not enough memory"
   allocate(tl(0:2**ntau-1), STAT = nAllocateStatus1)
   if (nAllocateStatus1 .ne. 0) STOP "Not enough memory"
! CE3TSK: the hash key of a pattern is its ntau parity bits K+1..K+ntau read straight out of
! word 1 of the packed row (bits 27..27+ntau-1 for K=91). The byte version built the key bit
! by bit in the other bit order; the mapping is a bijection and chains are appended in the
! same order, so every bucket holds the same pairs in the same order as before.
   keymask=ishft(1_8,ntau)-1_8
   reset=.true.
   ntotal=0
   do i1=K,1,-1
      do i2=i1-1,1,-1
         ntotal=ntotal+1
         ipat=int(iand(ishft(ieor(grow(1,i1),grow(1,i2)),-27),keymask))
         call boxit91p(indexes,fp,np,tl,reset,ipat,ntotal,i1,i2)
      enddo
   enddo

   ncount2=0
   ntotal2=0
   reset=.true.
! Now run through again and do the second pre-processing rule
   misub(1:K-nord)=0
   misub(K-nord+1:K)=1
   iflag=K-nord+1
   do while(iflag .ge.0)
      call osd_packk(misub,K,msw)
      mew=ieor(m0w,msw)
      call mrbencode91p(mew,cebw,grow,K)
      e2sw=iand(ieor(cebw,hw),mskP)
      do i2=0,ntau
         ntotal2=ntotal2+1
         e2w=e2sw
         if(i2.gt.0) e2w((K+i2-1)/64)=ieor(e2w((K+i2-1)/64),ishft(1_8,mod(K+i2-1,64)))
         ipat=int(iand(ishft(e2w(1),-27),keymask))
778      continue
            call fetchit91p(indexes,fp,np,reset,ipat,in1,in2,nthr)
            if(in1.gt.0.and.in2.gt.0) then
               ncount2=ncount2+1
               miw=msw
               miw((in1-1)/64)=ibset(miw((in1-1)/64),mod(in1-1,64))
               miw((in2-1)/64)=ibset(miw((in2-1)/64),mod(in2-1,64))
               if(popcnt(miw(0))+popcnt(miw(1)).lt.nord+npre1+npre2 .or. &
                  iand(apw(0),miw(0)).ne.0_8 .or. iand(apw(1),miw(1)).ne.0_8) cycle
               cew=cebw                            ! codeword of m0 xor mi, from the base pattern
               if(.not.btest(msw((in1-1)/64),mod(in1-1,64))) cew=ieor(cew,grow(:,in1))
               if(.not.btest(msw((in2-1)/64),mod(in2-1,64))) cew=ieor(cew,grow(:,in2))
               xw=ieor(cew,hw)
               dd=osd_wsum(xw,absrx,1,N)
               if( dd .lt. dmin ) then
                  dmin=dd
                  cww=cew
                  nhardmin=popcnt(xw(0))+popcnt(xw(1))+popcnt(xw(2))
               endif
               goto 778
             endif
      enddo
      call nextpat91(misub,K,nord,iflag)
   enddo
   deallocate (indexes, STAT = nDeAllocateStatus1)
   if (nDeAllocateStatus1.ne.0) print *, 'failed to release memory'
   deallocate (fp, STAT = nDeAllocateStatus1)
   if (nDeAllocateStatus1.ne.0) print *, 'failed to release memory'
   deallocate (np, STAT = nDeAllocateStatus1)
   if (nDeAllocateStatus1.ne.0) print *, 'failed to release memory'
   deallocate (tl)
endif

998 continue
do i=1,N
  cw(i)=0; if(btest(cww((i-1)/64),mod(i-1,64))) cw(i)=1
enddo
! Re-order the codeword to [message bits][parity bits] format. 
cw(indices)=cw
hdec(indices)=hdec
decoded=cw(1:K) 
call chkcrc14a(decoded,nbadcrc)
message77=decoded(1:77)
if(nbadcrc.eq.1) nhardmin=-nhardmin

return
end subroutine osd174_91


! CE3TSK: packed helpers. A pattern or codeword is three 64-bit words, bit p-1 of word
! (p-1)/64 standing for position p (1..174).
subroutine mrbencode91p(mew,cw,grow,K)
! the pattern must be K-masked: a bit at position K+1..128 would index grow past its K rows
  integer*8 mew(0:2),cw(0:2),grow(0:2,K)
  integer*8 w
  cw=0_8
  do iw=0,1
    w=mew(iw)
    do while(w.ne.0_8)
      ib=trailz(w)
      i=iw*64+ib+1
      cw(0)=ieor(cw(0),grow(0,i)); cw(1)=ieor(cw(1),grow(1,i)); cw(2)=ieor(cw(2),grow(2,i))
      w=iand(w,w-1_8)
    enddo
  enddo
  return
end subroutine mrbencode91p

subroutine osd_packk(m8,K,mw)
  integer*1 m8(K)
  integer*8 mw(0:2)
  mw=0_8
  do i=1,K
    if(m8(i).eq.1) mw((i-1)/64)=ibset(mw((i-1)/64),mod(i-1,64))
  enddo
  return
end subroutine osd_packk


integer function osd_bitof(w,ip)
  integer*8 w(0:2)
  osd_bitof=0
  if(btest(w((ip-1)/64),mod(ip-1,64))) osd_bitof=1
  return
end function osd_bitof

! sum of absrx over the set bits of w within positions ilo..ihi, in increasing order -
! exactly what sum(bits*absrx) computed, the zero terms of that sum being no-ops
real function osd_wsum(w,absrx,ilo,ihi)
  integer*8 w(0:2),ww
  real absrx(*)
  osd_wsum=0.
  do iw=0,2
    ww=w(iw)
    do while(ww.ne.0_8)
      ib=trailz(ww)
      i=iw*64+ib+1
      if(i.ge.ilo .and. i.le.ihi) osd_wsum=osd_wsum+absrx(i)
      ww=iand(ww,ww-1_8)
    enddo
  enddo
  return
end function osd_wsum

subroutine nextpat91(mi,k,iorder,iflag)
  integer*1 mi(k),ms(k)
! generate the next test error pattern
  ind=-1
  do i=1,k-1
     if( mi(i).eq.0 .and. mi(i+1).eq.1) ind=i 
  enddo
  if( ind .lt. 0 ) then ! no more patterns of this order
    iflag=ind
    return
  endif
  ms=0
  ms(1:ind-1)=mi(1:ind-1)
  ms(ind)=1
  ms(ind+1)=0
  if( ind+1 .lt. k ) then
     nz=iorder-sum(ms)
     ms(k-nz+1:k)=1
  endif
  mi=ms
  do i=1,k  ! iflag will point to the lowest-index 1 in mi
    if(mi(i).eq.1) then
      iflag=i 
      exit
    endif
  enddo
  return
end subroutine nextpat91

subroutine boxit91p(indexes,fp,np,tl,reset,ipat,npindex,i1,i2)
  integer indexes(:,:),fp(0:),np(:),tl(0:)
  integer ipat,npindex,i1,i2
  logical reset
! CE3TSK: keyed version of boxit91: the caller supplies the pattern's hash key; the bucket
! chain is appended at its tail in O(1), in the same order the byte version produced.
  if(reset) then
    fp=-1
    np=-1
    tl=-1
    indexes=-1
    reset=.false.
  endif
  indexes(npindex,1)=i1
  indexes(npindex,2)=i2
  if(fp(ipat).eq.-1) then
    fp(ipat)=npindex
  else
    np(tl(ipat))=npindex
  endif
  tl(ipat)=npindex
  return
end subroutine boxit91p

subroutine fetchit91p(indexes,fp,np,reset,ipat,i1,i2,nthr)
  integer indexes(:,:),fp(0:),np(:)
  integer lastpat(48),inext(48)   ! CE3TSK: per slice
  integer ipat,i1,i2,nthr
  logical reset
  save lastpat,inext
! CE3TSK: keyed version of fetchit91, otherwise unchanged
  if(reset) then
    lastpat(nthr)=-1
    reset=.false.
  endif
  index=fp(ipat)
  if(lastpat(nthr).ne.ipat .and. index.gt.0) then ! return first set of indices
     i1=indexes(index,1)
     i2=indexes(index,2)
     inext(nthr)=np(index)
  elseif(lastpat(nthr).eq.ipat .and. inext(nthr).gt.0) then
     i1=indexes(inext(nthr),1)
     i2=indexes(inext(nthr),2)
     inext(nthr)=np(inext(nthr))
  else
     i1=-1
     i2=-1
     inext(nthr)=-1
  endif
  lastpat(nthr)=ipat
  return
end subroutine fetchit91p
