subroutine qpc_decode2(c0,fsync,ftol,xdec,ndepth,dth,damp,crc_ok,   &
     snrsync,fbest,tbest,snr)

! CE3TSK 2026-09-20: WSJT-X 3.0.2's search, unchanged in what it tries and in what order, so the
! decodes are the donor's (test/decode/superfox.sh). Two things cost time for nothing and are
! gone: sfox_demod was called inside the loop over the four smoothing weights with the same
! signal, frequency and time each time - it is called once per time/frequency step now and its
! symbol spectra are copied for each weight; and the median is selected, not sorted for
! (sfox_pctile.f90).

   use qpc_mod
   use sfox_mod, only : s3plain,lnormed   !CE3TSK: sfox_demod's spectra without its normalisation step, for the SNR
   use sfox_mod, only : nfoxknown28,nfoxknown58   !CE3TSK: the Fox calls the receiver knows, for the acceptance floor
   use sfox_mod, only : nfoxgrid15,nsfme28,nsfprog,lsfoxap   !CE3TSK: and what else the a-priori pass states
   use sfox_mod, only : nsfap,lsfapfam,sfapfloor1,sfapfloor2,nsfaplooks,lsfstats,sfox_config   !CE3TSK: its settings
   use sfox_mod, only : nsfpool,nsfpool28,nsfpoolfox                  !CE3TSK: the pool pass: the Hounds heard
   use sfox_mod, only : nsfpoolon,nsfpooll,nsfpoolcrc,sfpoolfloor,nsfpoollooks   !CE3TSK: and its settings
   use sfox_mod, only : nsfliston,nsflistl,nsflistcrc,sflistfloor,nsflistlooks   !CE3TSK: the list pass's settings

   parameter(NMAX=15*12000,NFT=365,NZ=100)
   complex c0(NMAX)                    !Signal as received
   complex c(NMAX)                     !Signal as received
   real py(0:127,0:127)                !Probabilities for received synbol values
   real py0(0:127,0:127)               !Probabilities for strong signal
   real pyd(0:127,0:127)               !Dithered values for py
   real s2(0:127,0:151)                !Symbol spectra, including sync
   real s3(0:127,0:127)                !Synchronized symbol spectra
   real s3raw(0:127,0:127)             !CE3TSK: the same, as sfox_demod returned them
   real s3rawp(0:127,0:127),s3p(0:127,0:127)   !CE3TSK: and without the bin normalisation: as returned, as weighted
   real rowrest(0:127),restmed,row(127),rowmed,psum
   character msgbits*329,envk*8
   integer*8 n58
! CE3TSK: qpc/qpc_ap.c - qpc_decode with nap message symbols TOLD (not in qpc_mod, which is the donor's file unchanged)
   interface
      subroutine qpc_decode_ap(xdec, ydec, py, nap, kap, vap) bind(C,name="qpc_decode_ap")
        use iso_c_binding, only: c_float, c_signed_char, c_int
        real(c_float), intent(inout) :: py(128,128)
        integer(c_signed_char), intent(out) :: ydec(127)
        integer(c_signed_char), intent(out) :: xdec(50)
        integer(c_int), intent(in), value :: nap
        integer(c_int), intent(in) :: kap(50)
        integer(c_signed_char), intent(in) :: vap(50)
      end subroutine qpc_decode_ap
   end interface
! CE3TSK: qpc/qpc_scl.c - this program's LIST decoder, and the candidate sets of the Hound slots
   interface
      integer(c_int) function qpc_decode_scl(xdec, ydec, py, L, nap, kap, vap, ncrc, npaths) bind(C,name="qpc_decode_scl")
        use iso_c_binding, only: c_float, c_signed_char, c_int
        real(c_float), intent(inout) :: py(128,128)
        integer(c_signed_char), intent(out) :: ydec(128)
        integer(c_signed_char), intent(out) :: xdec(50)
        integer(c_int), intent(in), value :: L, nap, ncrc
        integer(c_int), intent(in) :: kap(50)
        integer(c_signed_char), intent(in) :: vap(50)
        integer(c_int), intent(out) :: npaths
      end function qpc_decode_scl
      subroutine qpc_scl_setcand(slot, n, n28) bind(C,name="qpc_scl_setcand")
        use iso_c_binding, only: c_int
        integer(c_int), intent(in), value :: slot, n
        integer(c_int), intent(in) :: n28(*)
      end subroutine qpc_scl_setcand
      subroutine qpc_scl_cand(on) bind(C,name="qpc_scl_cand")
        use iso_c_binding, only: c_int
        integer(c_int), intent(in), value :: on
      end subroutine qpc_scl_cand
   end interface
   integer, save :: nknown=-1            !CE3TSK: 1 = a known Fox is accepted below the floor; JTDX_SFOX_KNOWN=0: 0
   integer nown
   logical lqrm(0:127)
   logical lnormed0
   real No
   integer crc_chk,crc_sent
   integer*8 n47
   integer idf(NZ),idt(NZ)
   integer nseed(33)
   integer*1 xdec(0:49)                !Decoded message
   integer*1 ydec(0:127)               !Decoded symbols
   logical crc_ok
   integer maxdither(8)
   integer isync(24)                   !Symbol numbers for sync tones
   data isync/1,2,4,7,11,16,22,29,37,39,42,43,45,48,52,57,63,70,78,80,83,  &
      84,86,89/
   data n47/47/,maxdither/20,50,100,200,500,1000,2000,5000/
   data nseed/                                                         &
      321278106,  -658879006,  1239150429,  -941466001, -698554454, &
      1136210962,  1633585627,  1261915021, -1134191465, -487888229, &
      2131958895, -1429290834, -1802468092,  1801346659, 1966248904, &
      402671397, -1961400750, -1567227835,  1895670987, -286583128, &
      -595933665, -1699285543,  1518291336,  1338407128,  838354404, &
      -2081343776, -1449416716,  1236537391,  -133197638,  337355509, &
      -460640480,  1592689606,          0/

   data idf/0,  0, -1,  0, -1,  1,  0, -1,  1, -2,  0, -1,  1, -2,  2, &
        0, -1,  1, -2,  2, -3,  0, -1,  1, -2,  2, -3,  3,  0, -1, &
        1, -2,  2, -3,  3, -4,  0, -1,  1, -2,  2, -3,  3, -4,  4, &
        0, -1,  1, -2,  2, -3,  3, -4,  4, -5, -1,  1, -2,  2, -3, &
        3, -4,  4, -5,  1, -2,  2, -3,  3, -4,  4, -5, -2,  2, -3, &
        3, -4,  4, -5,  2, -3,  3, -4,  4, -5, -3,  3, -4,  4, -5, &
        3, -4,  4, -5, -4,  4, -5,  4, -5, -5/
   data idt/0 , -1,  0,  1, -1,  0, -2,  1, -1,  0,  2, -2,  1, -1,  0, &
        -3,  2, -2,  1, -1,  0,  3, -3,  2, -2,  1, -1,  0, -4,  3, &
        -3,  2, -2,  1, -1,  0,  4, -4,  3, -3,  2, -2,  1, -1,  0, &
        -5,  4, -4,  3, -3,  2, -2,  1, -1,  0, -5,  4, -4,  3, -3, &
         2, -2,  1, -1, -5,  4, -4,  3, -3,  2, -2,  1, -5,  4, -4, &
         3, -3,  2, -2, -5,  4, -4,  3, -3,  2, -5,  4, -4,  3, -3, &
        -5,  4, -4,  3, -5,  4, -4, -5,  4, -5/


   fsample=12000.0
   baud=12000.0/1024.0
   nstype=1
   n47=47
   mask21=2**21 - 1
   crc_ok=.false.
   lsfoxap=.false.

   call qpc_sync(c0,fsample,isync,fsync,ftol,f2,t2,snrsync)
   f00=1500.0 + f2
   t00=t2
   fbest=f00
   tbest=t00
   maxd=1
   if(ndepth.gt.0) maxd=maxdither(ndepth)
   maxft=NZ
   if(snrsync.lt.4.0 .or. ndepth.le.0) maxft=1
   do idith=1,maxft
      if(idith.ge.2) maxd=1
      deltaf=idf(idith)*0.5
      deltat=idt(idith)*8.0/1024.0
      f=f00+deltaf
      t=t00+deltat
      fshift=1500.0 - (f+baud)        !Shift frequencies down by f + 1 bin
      call twkfreq2(c0,c,NMAX,fsample,fshift)
      call sfox_demod(c,1500.0,t,isync,s2,s3raw)    !Compute s2 and s3, once for the four weights
      lnormed0=lnormed
      if(lnormed0) s3rawp=s3plain
      a=1.0
      b=0.0
      do kk=1,4
         if(kk.eq.2) b=0.4
         if(kk.eq.3) b=0.5
         if(kk.eq.4) b=0.6
         s3=s3raw

         if(b.gt.0.0) then
            do j=0,127
               call smo121a(s3(:,j),128,a,b)
            enddo
         endif
         call sfox_pctile(s3,128*128,50,base3)
         s3=s3/base3

         EsNoDec=3.16
         No=1.
         py0=s3
         call qpc_likelihoods2(py,s3,EsNoDec,No)       !For weak signals
         
         call random_seed(put=nseed)
         do kkk=1,maxd
            if(kkk.eq.1) then
               pyd=py0
            else
               pyd=0.
               if(kkk.gt.2) then
                  call random_number(pyd)
                  pyd=2.0*(pyd-0.5)
               endif
               where(py.gt.dth) pyd=0.          !Don't perturb large likelihoods
               pyd=py*(1.0 + damp*pyd)          !Compute dithered likelihood
            endif
            do j=0,127
               ss=sum(pyd(:,j))
               if(ss.gt.0.0) then
                 pyd(:,j)=pyd(:,j)/ss
               else
                 pyd(:,j)=0.0
               endif
            enddo

            call qpc_decode(xdec,ydec,pyd)
            xdec=xdec(49:0:-1)
            crc_chk=iand(nhash2(xdec,n47,571),mask21)           !Compute crc_chk
            crc_sent=128*128*xdec(47) + 128*xdec(48) + xdec(49)
            crc_ok=crc_chk.eq.crc_sent

            if(crc_ok) then
               call sfox_estimate       !the SNR: an internal routine below, the a-priori pass needs it too
! CE3TSK 2026-09-20: THE FLOOR AND A FOX THAT IS KNOWN (SUPERFOX_DECODER_IDEAS.md idea 2;
! test/experiments/sfox_floor/). The floor is there because a slot without a Fox makes 400 decoder
! calls (796 when something gives the sync a peak) against a 21-bit CRC, and a chance pass reads about -19 dB. It also rejects CORRECT
! decodes - measured on AWGN, 40 files a level: 1 at -16.5 dB, 5 at -17.0 (13 -> 18 decoded), 1 at
! -17.5, every one with an estimate of -16.5 to -16.7; one in 280 with a fading Fox. A decode that
! carries a call the receiver KNOWS - the operator's DX call, or the Fox it last decoded above the
! floor - is kept. What that risks, said exactly (review, 2026-09-20): in a slot WITHOUT that Fox a
! chance pass would have to hit the call's 28 bits beside the CRC - 1 in 10^12 periods. With the
! Fox ON THE AIR just under the threshold it is otherwise: the decoder is successive cancellation,
! the call's four symbols are among the first 24 of the 50 it decides, and a word that goes wrong
! after them has the RIGHT call - about half such a near-miss word per undecoded period at -17 to
! -18 dB, each passing the CRC with 2^-21: 2 to 3 in 10^7 such periods. That is the order WSJT-X's
! receiver has ABOVE the floor as well (those words read -16.48 to -16.58 dB, the floor does not
! part them); the exemption about doubles it. Such a line would show the right Fox, some true
! Hound lines, some garbage ones and a code that does not verify.
! Worth about 0.15 dB at the threshold; nothing changes for a Fox that is not known, which is
! every identity test. JTDX_SFOX_KNOWN=0 switches it off.
               if(.not.(snr.ge.-16.5)) then              !written so that a NaN estimate does not pass (review 2)
                  if(nknown.lt.0) then
                     nknown=1
                     call get_environment_variable('JTDX_SFOX_KNOWN',envk,nenv,istat)
                     if(istat.eq.0 .and. nenv.gt.0) then
                        if(envk(1:1).eq.'0') nknown=0
                     endif
                  endif
                  crc_ok=.false.
                  if(nknown.eq.1) then
                     write(msgbits,'(47b7.7)') xdec(0:46)
                     read(msgbits(327:329),'(b3)') i3
                     if(i3.eq.3) then
                        read(msgbits(1:58),'(b58)') n58
                        crc_ok=any(nfoxknown58.ge.0 .and. nfoxknown58.eq.n58)
                     else
                        read(msgbits(1:28),'(b28)') n28
                        crc_ok=any(nfoxknown28.ge.0 .and. nfoxknown28.eq.n28)
                     endif
                  endif
               endif
! NB the search ENDS here also when the floor has just rejected the word (the donor's behaviour),
! and the a-priori pass below is then not reached. That loses nothing - a rejected word is of a Fox
! that is not known, and the pass states only known ones - but test/decode/superfox.sh's rows
! "JTDX_SFOX_KNOWN=0 prints nothing" rest on it: with the exemption off, a known Fox's CQ at
! -17 dB is rejected HERE and never reaches the pass that would decode it at -20.
!               write(61,3061) idith,kk,kkk,idf(idith),idt(idith),a,b
!3061           format(5i5,2f8.3)
               return
            endif
         enddo    !kk: dither of smoothing weights
      enddo       !kkk: dither of probabilities
   enddo          !idith: dither of frequency and time
   call sfox_appass                   !CE3TSK: nothing found - what can be STATED about the message is tried
   if(.not.crc_ok) call sfox_poolpass !CE3TSK: and then WHO MAY BE IN IT: the Hounds heard
   if(.not.crc_ok) call sfox_listpass !CE3TSK: and then the known Fox told and NOTHING said about the Hounds: the list decoder alone
   return

contains

subroutine sfox_estimate

   integer i,j

! CE3TSK 2026-09-20: the SNR from spectra in which the Fox's OWN bins are not divided (sfox_demod
! has the story). The Fox's symbols are known now, so each bin's mean can be taken WITHOUT the
! cells the Fox itself put there. What is left is noise, QRM - and, from about +10 dB up, the
! Fox's own leakage, which the rectangular symbol FFT spreads over ALL bins alike. So "twice the
! noise" would call every bin of a +20 dB Fox a QRM bin (it read +9 for WSJT-X's +14); QRM is what
! stands out from the OTHER bins: a bin whose rest-mean is over three times the MEDIAN of all 128
! rest-means (and of the noise's) holds QRM. When NO bin does, every bin is read plain and the
! estimate is WSJT-X's to the digit (-16 to +25 dB checked, 120 files). When some do, the symbols
! the Fox sent IN those bins are left out of the sum - what stands there is the caller, divided
! or not - and the callers' leakage has also lifted the ONE median the plain spectra are scaled
! by (a -10 dB Fox under three +10 dB callers read -15), so each of the other bins is scaled by
! its OWN median over the transmission, which neither the Fox's handful of cells nor a leakage
! tail moves. What is left of the difference to a clean band's reading is real: the Fox is
! measured against noise plus what the callers leak.
   if(lnormed0) then
      s3p=s3rawp
      if(b.gt.0.0) then
         do j=0,127
            call smo121a(s3p(:,j),128,a,b)
         enddo
      endif
      call sfox_pctile(s3p,128*128,50,base3)
      s3p=s3p/base3
! The rest-mean leaves out not only the Fox's own cell but every cell within 8 bins of the Fox's
! tone IN THAT SYMBOL: a strong Fox's leakage is heaviest right beside the tone it is sending, so a
! bin next to a much-used tone stood out from the rest and was taken for QRM (a one-Hound Fox at
! +20 dB read +12 for +13). QRM does not care where the Fox's tone is; the Fox's leakage does.
      do i=0,127
         nown=0; rowrest(i)=0.
         do j=1,127
            if(abs(int(ydec(j))-i).gt.8) then
               nown=nown+1; rowrest(i)=rowrest(i)+s3p(i,j)
            endif
         enddo
         if(nown.gt.0) rowrest(i)=rowrest(i)/nown
      enddo
      call sfox_pctile(rowrest,128,50,restmed)
! three times: QRM bins are ten times the rest or more; at twice, a +30 dB Fox still had one bin over
      lqrm=rowrest.gt.3.0*max(restmed,1.443)
      if(any(lqrm)) then
         do i=0,127
            if(lqrm(i)) cycle
            row=s3p(i,1:127)
            call sfox_pctile(row,127,50,rowmed)
            if(rowmed.gt.0.0) s3p(i,:)=s3p(i,:)/rowmed
         enddo
         psum=0.; nown=0
         do j=1,127
            if(lqrm(ydec(j))) cycle
            psum=psum+s3p(ydec(j),j); nown=nown+1
         enddo
         if(nown.ge.32) then
            snr=db(psum/nown) - db(127.0) - 4.0      !qpc_snr's formula, over the symbols used
         else
            call qpc_snr(s3,ydec,snr)                !QRM nearly everywhere: the normalised spectra
         endif
      else
         call qpc_snr(s3p,ydec,snr)
      endif
   else
   call qpc_snr(s3,ydec,snr)
   endif

   return
end subroutine sfox_estimate

subroutine sfox_appass

! CE3TSK 2026-09-20: THE A-PRIORI PASS - stage A of the SuperFox hint memory (SUPERFOX_DECODER_IDEAS.md
! idea 5; test/experiments/sfox_ap/ has every measurement quoted here). It follows the FT8
! decoder's a-priori mechanism (FT8_DECODER.md 2.3): it runs only when the ordinary search has
! failed, it states part of the message and decodes the rest, what it may state about ME follows
! the QSO's progress, and its decodes are marked '*'.
!
! WHAT IS STATED, AND WHY WHOLE MESSAGES. A stated message symbol is decoded as a frozen one
! (qpc/qpc_ap.c). The decoder is successive cancellation: it decides position 127 first, position
! 0 last, and its first decisions are its worst. The nine weakest positions of the fifty carry one
! symbol of the Fox's call, two of Hound slot 0 and one each of slots 1, 2, 3, 4, 5 and 7 - so a
! hypothesis is worth what it states OF THOSE. Measured (AWGN, 50 % points; the plain receiver's is
! about -17 dB on these files of one to four Hounds, -16.7 for the threshold harness's full
! message): with the TRUE symbols stated, the Fox's call alone gains 0.3 dB, the call and three
! Hound slots 1.2 dB, the call and all nine slots 4 dB. Stating the Fox's call and every EMPTY slot
! around ONE unknown Hound in slot 0 gained 0.5 dB, and nothing at all for a Fox with a full
! message. What gains is a message stated (almost) whole - so that is all this pass tries, for a
! Fox that is KNOWN (sfox_mod: the DX call, or the Fox last decoded above the floor):
!   1  "CQ <Fox> <grid>" with the grid known  50 % at -21 dB    2  the same, grid free  -17.5 dB
!   3  my report and nothing else            -20.7 dB          4  RR73 to me and nothing else  -21.4 dB
!   5-7   my report first, 2-4 reports in all, no RR73  -19.1 to -19.4 dB
!   8-11  RR73 to me and 1-4 reports to others          -18.4 to -19.2 dB
!   12-15 1-4 reports and nothing else - the RR73 slots hold six of the nine  -18.5 to -19.2 dB
! (on the mid-latitude fading channel, plain -15.4 dB: 1, 3 and 4 near -20 dB.)
! An unused Hound slot is not empty: WSJT-X (sfox_pack) and MSHV write a fixed token there, and the
! seven 32-bit words of a CQ-only message hold it as well. 3-11 need my own call and a QSO that
! expects them (sfox_mod: nsfprog). A Fox with RR73s to others, or with a full message, gets
! nothing from this stage - that takes the Hounds' calls, which is the hint memory proper.
!
! ONE time/frequency step (the sync's own), one smoothing weight, two forms of the symbol spectra
! (themselves, then the likelihoods): at most 16 hypotheses for the DX call and 6 more when the
! remembered Fox is another one (what concerns MY call is tried for the DX call only: that is who
! the QSO is with), two decoder calls each - at most 44 against the ordinary search's 796, in
! every pass of the receiver; the FT8 QRM remover's residuals make up to three passes a slot.
! Measured on 864 files: the spectra alone 390 decoded, the likelihoods as well 450, all four
! smoothing weights 450 again - the same files; a hundred dithers of the likelihoods, tried in the
! first version, found a handful more for fifty times the calls.
!
! WHAT KEEPS FALSE LINES OUT. Every bit the hypothesis states must stand in the decoded word,
! also where it could not be told to the decoder because the symbol holds free bits as well (i3
! beside the signature, four zero bits beside the CQ flag, a report slot's zeros); and the word's
! SNR estimate must reach a floor of its own (below, where it is applied): -20.5 dB for a message
! stated whole, -18.7 dB for the others. What the floors cost, measured WITHOUT them: the partial
! hypotheses' true decodes read -18.3 and up at their -19 dB threshold, and one of the five found at
! -20 dB reads under -18.7; of the whole messages' few true decodes at -23 dB one in three reads
! -20.6. What they keep out: 99 in 100 of the words the pass makes from noise read less.
! Counted over those words - how many WOULD be accepted if their CRC passed by chance, which one
! word in 2^21 does (one known Fox, my call in a QSO: 30 words a pass): noise alone 1 word in 1000
! periods (a false line in 2*10^9 periods); FT8 bands 1 in 40 periods from the receiver alone
! (8*10^7) and 1 in 25 with the QRM remover's residuals, 60 to 67 words a period (5*10^7); another
! Fox on the frequency 1 in 9 (2*10^7). With the KNOWN Fox on the air and not decoded it is about
! one word a period, because a hypothesis that is nearly right makes a word that is nearly the
! Fox's - no floor parts them, only the CRC does: one false line in about 2 million such periods,
! which would show the right Fox with a Hound, a report or my own call that it did not send. The
! one that matters is a false "RR73 to me" after my R+report, which logs a QSO that was not
! completed: ten of the words are of that kind, about one such line in 10^6 periods of waiting for
! the RR73 of a Fox too weak to decode. FT8's a-priori decodes stand on a 14-bit CRC, 128 times
! weaker. No wrong line in the 7500 periods of the saved runs (test/experiments/sfox_ap/results).
! NOTHING OF THIS IS FINAL: it was measured on simulated transmissions and two recordings, and until
! there is more on-air material the families, the floors, the looks and the memory's age are
! SETTINGS (sfox_mod.f90 lists them; JTDX_SFOX_AP=0 switches the pass off).

   integer, parameter :: NHMAX=34
   integer kaph(50,NHMAX),naph(NHMAX),nhyp,ihyp,m,i,j,islot,ig,ia,ib,me,id,ng,iv
   integer*1 vaph(50,NHMAX)
   character*329 hypb(NHMAX),hb
   logical hypk(329,NHMAX),hk(329),lsame
   real pys(0:127,0:127),apfloor
   integer ncalls,ifam
   logical lstats

! the pass's SETTINGS are sfox_mod's (there, with what each is for): the families tried, the two
! floors, the looks - the defaults are what the comment above reports
   call sfox_config
   if(nsfap.eq.0) return
   lstats=lsfstats
   if(all(nfoxknown28.lt.0) .and. all(nfoxknown58.lt.0)) return

! the hypotheses, once: for each the bits it states, and the message symbols that are stated whole
   lsame=(nfoxknown28(1).ge.0 .and. nfoxknown28(1).eq.nfoxknown28(2)) .or.   &
         (nfoxknown58(1).ge.0 .and. nfoxknown58(1).eq.nfoxknown58(2))
   nhyp=0
   do islot=1,2
      if(islot.eq.2 .and. lsame) cycle                        !one Fox, known twice
      do id=1,15
      do ig=1,2
         if(ig.eq.2 .and. (id.ne.1 .or. islot.ne.1 .or. .not.lsame)) cycle
         ifam=4                                               !the family: c 1-2, m 3-4, o 5-11, r 12-15
         if(id.le.11) ifam=3
         if(id.le.4) ifam=2
         if(id.le.2) ifam=1
         if(.not.lsfapfam(ifam)) cycle
         hb=repeat('0',329); hk=.false.
         if(id.le.2) then                                     !CQ: 1 with the grid, 2 without
            if(nfoxknown58(islot).lt.0) cycle
            write(hb(1:58),'(b58.58)') nfoxknown58(islot); hk(1:58)=.true.
            if(id.eq.1) then                                  !the DX grid; the remembered one when it is another
               ng=nfoxgrid15(islot)
               if(ig.eq.2) ng=nfoxgrid15(2)
               if(ng.lt.0 .or. (ig.eq.2 .and. ng.eq.nfoxgrid15(1))) cycle
               write(hb(59:73),'(b15.15)') ng; hk(59:73)=.true.
            endif
            do i=0,6
               write(hb(74+32*i:105+32*i),'(b32.32)') 203514677
            enddo
            hk(74:305)=.true.
            hb(327:329)='011'; hk(327:329)=.true.             !i3 = 3
         else
            if(nfoxknown28(islot).lt.0) cycle
            me=-1
            if(id.eq.3) then                                  !my report, alone
               ia=0; ib=1; me=5
            else if(id.eq.4) then                             !RR73 to me, alone
               ia=1; ib=0; me=0
            else if(id.le.7) then                             !my report first, 2..4 reports in all
               ia=0; ib=id-3; me=5
            else if(id.le.11) then                            !RR73 to me, 1..4 reports to others
               ia=1; ib=id-7; me=0
            else                                              !1..4 reports, nothing else
               ia=0; ib=id-11
            endif
            if(me.ge.0 .and. (nsfme28.lt.0 .or. nsfprog.lt.1)) cycle
            if(me.ge.0 .and. islot.eq.2) cycle                 !the QSO in progress is with the DX call (review)
            if(me.eq.0 .and. nsfprog.lt.2) cycle               !RR73 only to one who has sent R+report
            write(hb(1:28),'(b28.28)') nfoxknown28(islot); hk(1:28)=.true.
            do i=0,8                                           !slots 0-4 RR73 (ia used), 5-8 reports (ib used)
               if((i.le.4 .and. i.ge.ia) .or. (i.ge.5 .and. i-5.ge.ib)) then
                  write(hb(29+28*i:56+28*i),'(b28.28)') 203514677; hk(29+28*i:56+28*i)=.true.
               endif
            enddo
            if(me.ge.0) then
               write(hb(29+28*me:56+28*me),'(b28.28)') nsfme28; hk(29+28*me:56+28*me)=.true.
            endif
            do i=ib,3
               hk(281+5*i:285+5*i)=.true.                       !no report in an unused report slot
            enddo
            hk(301:305)=.true.; hk(327:329)=.true.              !unused bits; i3 = 0
         endif
         if(nhyp.ge.NHMAX) cycle
         nhyp=nhyp+1; naph(nhyp)=0; hypb(nhyp)=hb; hypk(:,nhyp)=hk
         do m=0,46
            if(.not.all(hk(7*m+1:7*m+7))) cycle
            naph(nhyp)=naph(nhyp)+1; kaph(naph(nhyp),nhyp)=49-m  !message symbol m is x(49-m): sfox_pack reverses them
            read(hb(7*m+1:7*m+7),'(b7)') iv; vaph(naph(nhyp),nhyp)=int(iv,1)
         enddo
      enddo
      enddo
   enddo
   if(nhyp.eq.0) return
   ncalls=0

   f=f00; t=t00                                                 !the sync's own estimate: the search's first step
   fshift=1500.0 - (f+baud)
   call twkfreq2(c0,c,NMAX,fsample,fshift)
   call sfox_demod(c,1500.0,t,isync,s2,s3raw)
   lnormed0=lnormed
   if(lnormed0) s3rawp=s3plain
   a=1.0; b=0.0                                                 !its first smoothing weight: none
   s3=s3raw
   call sfox_pctile(s3,128*128,50,base3)
   s3=s3/base3
   call qpc_likelihoods2(py,s3,3.16,1.0)
   do kkk=1,nsfaplooks                                          !its first two trials: the spectra, the likelihoods
      if(kkk.eq.1) then
         pys=s3
      else
         pys=py
      endif
      do j=0,127
         ss=sum(pys(:,j))
         if(ss.gt.0.0) then
            pys(:,j)=pys(:,j)/ss
         else
            pys(:,j)=0.0
         endif
      enddo
      do ihyp=1,nhyp
         pyd=pys                                                !the decoder writes into what it is given
         ncalls=ncalls+1
         call qpc_decode_ap(xdec,ydec,pyd,naph(ihyp),kaph(:,ihyp),vaph(:,ihyp))
         xdec=xdec(49:0:-1)
         crc_chk=iand(nhash2(xdec,n47,571),mask21)
         crc_sent=128*128*xdec(47) + 128*xdec(48) + xdec(49)
         if(crc_chk.ne.crc_sent) cycle
         write(msgbits,'(47b7.7)') xdec(0:46)
         crc_ok=.true.
         do i=1,329
            if(hypk(i,ihyp) .and. msgbits(i:i).ne.hypb(ihyp)(i:i)) crc_ok=.false.
         enddo
         if(.not.crc_ok) cycle
         call sfox_estimate
! -20.5 dB for a message stated WHOLE - the CQ with its grid, my report alone, RR73 to me alone: 42
! or 43 of the 47 symbols told - and -18.7 dB for the rest, the CQ without its grid (40) among
! them: it decodes no deeper than -18 dB and reads -18.2 and up there (review: counted by "40 or
! more" it had the whole messages' floor, which keeps out nothing at the level it works at)
         apfloor=sfapfloor2
         if(naph(ihyp).ge.42) apfloor=sfapfloor1
         if(.not.(snr.ge.apfloor)) then                        !a NaN estimate does not pass either
            if(lstats) write(0,'(a,f6.1,a,f6.1,a)') 'SuperFox a-priori pass: a word with a good CRC reads',snr,  &
                 ' dB, under its floor of',apfloor,' - rejected'
            crc_ok=.false.
            cycle
         endif
         lsfoxap=.true.
         fbest=f; tbest=t
         if(lstats) write(0,'(a,i3,a,i4,a,i3,a)') 'SuperFox a-priori pass:',nhyp,' hypotheses,',ncalls,  &
              ' decoder calls, decoded with',naph(ihyp),' symbols stated'
         return
      enddo
   enddo
   crc_ok=.false.
   if(lstats) write(0,'(a,i3,a,i4,a)') 'SuperFox a-priori pass:',nhyp,' hypotheses,',ncalls,' decoder calls, nothing'

   return
end subroutine sfox_appass

subroutine sfox_poolpass

! CE3TSK 2026-09-20: THE POOL PASS - stage B of the hint memory (SUPERFOX_DECODER_IDEAS.md 4.11;
! test/experiments/sfox_scl/README.md has the plan, the measurements and their files). Stage A
! states whole messages and has nothing for a BUSY Fox: one Hound slot left open costs 1 dB (slot
! 8) to 4 dB (slot 0) of what the rest is worth, and a list decoder alone gives back 0.4 to 1.4 dB
! of it. What does work is telling the decoder WHO MAY BE in the message: this program decodes the
! odd slots with its own FT8 decoder, so it knows the Hounds that call and answer the Fox
! (sfox_mod: the pool). Here the Fox in DX Call is told, every one of the nine Hound slots is held
! to "one of the pool, or my own call while my QSO runs, or empty" - a path of the list decoder
! may only take, at a slot's four positions, values that a candidate still alive on it has there
! (qpc/qpc_scl.c) - and the list searches over ASSIGNMENTS OF CALLS instead of raw symbols. No
! order, no placement and no prediction of the message is needed; ONE set serves all slots (RR73
! and report slots apart was measured 0.3 dB better, and a misjudged Hound breaks it).
! Measured AS IT RUNS HERE (L = 16, two looks, the floor; 40 files a level, a Fox with six Hounds,
! all of them in a pool of 16; test/experiments/sfox_scl/results/s4c_*): 50 % from about -16.7 to
! -18.3 dB on AWGN (13 and 3 of 40 at -17.0 and -17.5 plain: the -16.9 first written here was another
! run's) and from -15.3 to -17.3 dB on the mid-latitude fading channel. With larger lists and no
! floor (L = 64, 20 files a level, s4b_*): a Hound of the message that was NOT heard - the pass
! gains nothing and costs nothing (16 / 8 / 1 of 20 with and without), no wrong line; nothing
! printed in 300 periods of noise and 200 of another Fox; L = 64 reaches -18.0 dB on the fading
! channel, L = 256 -18.6 on AWGN.
! TWO LOOKS a receiver pass, stage A's: the sync's own step, no smoothing, the spectra and then the
! likelihoods - about 10 ms at L = 16 (the list is needed: L = 1 reaches -17.6 dB). The three smoothing weights
! of the search were tried as well (review): 109 decodes of 200 files against 108, and the one more
! read -20.2 dB - the 1-2-1 smoothing takes 2.6 to 3.4 dB off the tone peaks and so off the
! estimate, which a floor laid on unsmoothed estimates cannot pass. They are not tried.
! FALSE LINES: the CRC is tried on the BEST path only (JTDX_SFOX_POOLCRC, default 1; review 2: with
! the slots held to candidates EVERY decode on record has its right path first - 108 of 108, 83 of
! 83, the reviewer's 1745 - the "best in 19 of 20" was the UNconstrained list's; all 256 paths of
! a large list once produced a false line) - 2 checks a receiver pass, and the QRM remover makes up
! to three passes a slot, against the search's 796; a standard message only (i3 = 0, the layout the sets are laid
! on); and a floor of its own, -19.5 dB: the pass's decodes read -17.1 (median) and -18.1 at the
! lowest, its words with a bad CRC at most -19.0 to -19.6 when that Fox is not on the air. A false
! line would show calls that WERE heard - which is why the floor and the short CRC list are there.
! Marked '*'. ALL OF IT IS SETTINGS (sfox_mod.f90: the pass itself, the list size, the looks, the
! CRC's path count, the floor, the pool's age): nothing here is final before there is on-air
! material - every figure above comes from simulated transmissions.

   integer kapf(50),m,i,j,iv,ios,ncand,ncalls,irank,npaths,ncset(64),ncrcs,nme
   integer*1 vapf(50)
   character*28 fb
   real pys(0:127,0:127)

   call sfox_config
   if(nsfpoolon.eq.0 .or. nsfpool.lt.1) return
   if(nfoxknown28(1).lt.0) return                 !the Hounds were heard answering the DX call: that Fox only
   if(nsfpoolfox.ne.nfoxknown28(1)) return        !and the pool is of the Fox it was filled for (review: DX Call changed)
   ncand=1; ncset(1)=203514677                    !"empty" - the token an unused slot is filled with
   do i=1,nsfpool
      ncand=ncand+1; ncset(ncand)=nsfpool28(i)
   enddo
! my own call while my QSO with this Fox runs, by stage A's rule and PER SLOT (review 2: laid on all
! nine slots it let "RR73 to me" be decoded while I was only calling - which no Fox can send, and
! which is the one false line that logs a QSO): a REPORT slot (5-8) once I call, an RR73 slot (0-4)
! only after my R+report. It stands last in the set, so the sets differ by their length alone.
   nme=0
   if(nsfme28.ge.0 .and. nsfprog.ge.1) then
      if(.not.any(nsfpool28(1:nsfpool).eq.nsfme28)) then
         nme=1; ncset(ncand+1)=nsfme28
      endif
   endif
   do i=0,8
      if(i.le.4) then
         call qpc_scl_setcand(i,ncand+merge(nme,0,nsfprog.ge.2),ncset)
      else
         call qpc_scl_setcand(i,ncand+nme,ncset)
      endif
   enddo
   write(fb,'(b28.28)') nfoxknown28(1)
   kapf=0; vapf=0_1
   do m=0,3
      kapf(m+1)=49-m                              !message symbol m is x(49-m): sfox_pack reverses them
      read(fb(7*m+1:7*m+7),'(b7)',iostat=ios) iv; vapf(m+1)=int(iv,1)
   enddo
   ncalls=0; ncrcs=0

   f=f00; t=t00
   fshift=1500.0 - (f+baud)
   call twkfreq2(c0,c,NMAX,fsample,fshift)
   call sfox_demod(c,1500.0,t,isync,s2,s3raw)
   lnormed0=lnormed
   if(lnormed0) s3rawp=s3plain
   a=1.0; b=0.0                                   !no smoothing: what the estimate and its floor are laid on
   s3=s3raw
   call sfox_pctile(s3,128*128,50,base3)
   s3=s3/base3
   call qpc_likelihoods2(py,s3,3.16,1.0)
   do kkk=1,nsfpoollooks                          !the spectra themselves, then the likelihoods (a setting: two)
      if(kkk.eq.1) then
         pys=s3
      else
         pys=py
      endif
      do j=0,127
         ss=sum(pys(:,j))
         if(ss.gt.0.0) then
            pys(:,j)=pys(:,j)/ss
         else
            pys(:,j)=0.0
         endif
      enddo
      pyd=pys
      ncalls=ncalls+1
      call qpc_scl_cand(1)
      irank=qpc_decode_scl(xdec,ydec,pyd,nsfpooll,4,kapf,vapf,nsfpoolcrc,npaths)
      call qpc_scl_cand(0)
      ncrcs=ncrcs+npaths                          !paths the decoder tried against the CRC
      xdec=xdec(49:0:-1)
      crc_chk=iand(nhash2(xdec,n47,571),mask21)
      crc_sent=128*128*xdec(47) + 128*xdec(48) + xdec(49)
      if(crc_chk.ne.crc_sent) cycle
      write(msgbits,'(47b7.7)') xdec(0:46)
      if(msgbits(327:329).ne.'000') cycle         !a standard message: the layout the sets are laid on
      if(msgbits(1:28).ne.fb) cycle               !the Fox that was told (it is, by construction)
      call sfox_estimate
      if(.not.(snr.ge.sfpoolfloor)) then                  !a NaN estimate does not pass either
         if(lsfstats) write(0,'(a,f6.1,a,f6.1,a)') 'SuperFox pool pass: a word with a good CRC reads',snr,  &
              ' dB, under its floor of',sfpoolfloor,' - rejected'
         cycle
      endif
      crc_ok=.true.
      lsfoxap=.true.                              !marked '*' as an a-priori decode is
      fbest=f; tbest=t
      if(lsfstats) write(0,'(a,i3,a,i3,a,i3)') 'SuperFox pool pass:',ncand-1,' calls in the pool, decoded at look',ncalls,  &
           ', path of rank',irank
      return
   enddo
   crc_ok=.false.
   if(lsfstats) write(0,'(a,i3,a,i3,a,i4,a)') 'SuperFox pool pass:',ncand-1,' calls in the pool,',ncalls,' looks,',ncrcs,  &
        ' CRC checks, nothing'

   return
end subroutine sfox_poolpass

subroutine sfox_listpass

! CE3TSK 2026-09-21: THE LIST PASS - the pool pass without the pool (SUPERFOX_DECODER_IDEAS.md 4.11,
! "S2 finished"; test/experiments/sfox_scl/README.md rows S2c and S6 have the measurements and their
! files). Stage A needs a message it can state whole, the pool pass needs every Hound of the message
! HEARD in the odd slots. What is left is the busy Fox whose Hounds were not heard, the empty pool,
! the first period on a frequency - and there the receiver still knows ONE thing: the Fox (the DX
! call, or the Fox it last decoded above the floor). Here that Fox is told - its call is message
! symbols 0-3, four of the first decisions successive cancellation gets wrong - the nine Hound slots
! are left FREE, and the list decoder (qpc/qpc_scl.c) keeps the 64 best paths.
! Measured AS SHIPPED (the CRC on 4 paths; 100 files a level, a Fox with six Hounds, the same files
! through every variant; test/experiments/sfox_scl/results/s6_*): 50 % from -16.9 to -17.6 dB on AWGN
! (233 -> 357 decodes of 600) and from -15.4 to -16.1 dB on the mid-latitude fading channel (288 ->
! 350); the CRC on all 64 paths 365 / 364 (-17.65 / -16.25); L = 16 / 256 340 / 370 and 341 / 358. The
! list WITHOUT the Fox told gives half of it (-17.25 / -15.9), and next to nothing for a Fox that is
! not known - the -16.5 dB floor takes those decodes - which is why this is a pass for a KNOWN Fox
! and not a list decoder inside the search. As a pass it also leaves the search the donor's, call
! for call: what WSJT-X's receiver decodes is decoded as before, with this pass on or off.
! TWO LOOKS, stage A's and the pool pass's: the sync's own step, no smoothing, the spectra and then
! the likelihoods. Measured inside the search: the list in its first two calls - these two looks -
! decodes exactly what the list in the first eight does (the three smoothing weights too), at every
! list size, on both channels. Two list decodes at L = 64 are about 0.1 s, and only in a receiver
! pass that has found nothing.
! FALSE LINES. Every path carries the told Fox, so a chance CRC pass would print the right Fox with
! Hounds it did not send, and the search's -16.5 dB floor does not apply to it. Against that: the
! CRC is tried on the best 4 paths only (JTDX_SFOX_LISTCRC; 357 of the 365 AWGN decodes and 350 of
! the 364 fading ones have their path among the first four - without candidate sets the right path
! is NOT always first, so 1 would cost a fifth of the gain) - 8 checks a receiver pass, each a
! 2^-21 chance, against the search's 796; only the two message types that carry the Fox's call in
! bits 1-28 are accepted (i3 = 0 Hounds, i3 = 2 free text and Hounds); and a floor of its own,
! JTDX_SFOX_LISTFLOOR, -17.3 dB (sfox_mod.f90 has what it was laid on: it costs no true decode on record
! and takes 99.8 % of the words of noise alone, 96 % of those of a Fox at -19 / -20 dB - and does not
! part the words of a Fox just under the threshold, where the CRC stands alone). No line in 3000 periods
! of noise and of a Fox under the threshold as the program runs it, none in the experiment's 2000 with
! the Fox told and 2500 with nothing told. Marked
! '*'. My own call is nowhere stated here: if this pass prints "RR73 to me", the decoder FOUND it
! in free slots and the CRC-21 vouches for it as for any ordinary decode.
! ALL OF IT IS SETTINGS (sfox_mod.f90): nothing here is final before there is on-air material -
! every figure above comes from simulated transmissions.

   integer kapf(50),m,j,iv,ios,ncalls,irank,npaths,ncrcs,islot,nfox
   integer*1 vapf(50)
   character*28 fb
   real pys(0:127,0:127),snrwas

   call sfox_config
   if(nsfliston.eq.0) return
! a Fox the receiver knows, as bits 1-28 of a message carry it (a compound call as its hash: sfox_known)
   if(all(nfoxknown28.lt.0)) return
   ncalls=0; ncrcs=0

   f=f00; t=t00
   fshift=1500.0 - (f+baud)
   call twkfreq2(c0,c,NMAX,fsample,fshift)
   call sfox_demod(c,1500.0,t,isync,s2,s3raw)
   lnormed0=lnormed
   if(lnormed0) s3rawp=s3plain
   a=1.0; b=0.0                                   !no smoothing: what the estimate and its floor are laid on
   s3=s3raw
   call sfox_pctile(s3,128*128,50,base3)
   s3=s3/base3
   call qpc_likelihoods2(py,s3,3.16,1.0)
   call qpc_scl_cand(0)                           !the pool pass's candidate sets do not act here: the Hound slots are free
   do islot=1,2                                   !the DX call, then the remembered Fox when it is another
      nfox=nfoxknown28(islot)
      if(nfox.lt.0) cycle
      if(islot.eq.2 .and. nfox.eq.nfoxknown28(1)) cycle
      write(fb,'(b28.28)') nfox
      kapf=0; vapf=0_1
      do m=0,3
         kapf(m+1)=49-m                           !message symbol m is x(49-m): sfox_pack reverses them
         read(fb(7*m+1:7*m+7),'(b7)',iostat=ios) iv; vapf(m+1)=int(iv,1)
      enddo
      do kkk=1,nsflistlooks                       !the spectra themselves, then the likelihoods (a setting: two)
         if(kkk.eq.1) then
            pys=s3
         else
            pys=py
         endif
         do j=0,127
            ss=sum(pys(:,j))
            if(ss.gt.0.0) then
               pys(:,j)=pys(:,j)/ss
            else
               pys(:,j)=0.0
            endif
         enddo
         pyd=pys
         ncalls=ncalls+1
         irank=qpc_decode_scl(xdec,ydec,pyd,nsflistl,4,kapf,vapf,nsflistcrc,npaths)
         ncrcs=ncrcs+npaths                       !paths the decoder tried against the CRC
         xdec=xdec(49:0:-1)
         crc_chk=iand(nhash2(xdec,n47,571),mask21)
         crc_sent=128*128*xdec(47) + 128*xdec(48) + xdec(49)
         if(crc_chk.ne.crc_sent) then
            if(lsfstats) then                     !what the most probable word reads: the figure the floor is laid against
               snrwas=snr; call sfox_estimate
               write(0,'(a,i2,a,i2,a,i3,a,f7.2,a)') 'SuperFox list pass: Fox',islot,' look',kkk,', no CRC in',npaths,  &
                    ' paths; the most probable word reads',snr,' dB'
               snr=snrwas
            endif
            cycle
         endif
         write(msgbits,'(47b7.7)') xdec(0:46)
! i3 = 0 Hounds, i3 = 2 free text and Hounds: the two layouts with the Fox's call in bits 1-28
         if(msgbits(327:329).ne.'000' .and. msgbits(327:329).ne.'010') cycle
         if(msgbits(1:28).ne.fb) cycle            !the Fox that was told (it is, by construction)
         call sfox_estimate
         if(.not.(snr.ge.sflistfloor)) then       !a NaN estimate does not pass either
            if(lsfstats) write(0,'(a,f6.1,a,f6.1,a)') 'SuperFox list pass: a word with a good CRC reads',snr,  &
                 ' dB, under its floor of',sflistfloor,' - rejected'
            cycle
         endif
         crc_ok=.true.
         lsfoxap=.true.                           !marked '*' as an a-priori decode is
         fbest=f; tbest=t
         if(lsfstats) write(0,'(a,i2,a,i3,a,i3,a,f7.2,a)') 'SuperFox list pass: Fox',islot,  &
              ' told (1 the DX call, 2 the remembered one), decoded at look',kkk,', path of rank',irank,', reads',snr,' dB'
         return
      enddo
   enddo
   crc_ok=.false.
   if(lsfstats) write(0,'(a,i3,a,i4,a)') 'SuperFox list pass:',ncalls,' looks,',ncrcs,' CRC checks, nothing'

   return
end subroutine sfox_listpass

end subroutine qpc_decode2

subroutine smo121a(x,nz,a,b)

  real x(nz)
  fac=1.0/(a+2*b)
  x0=x(1)
  do i=2,nz-1
     x1=x(i)
     x(i)=fac*(a*x(i) + b*(x0+x(i+1)))
     x0=x1
  enddo

  return
end subroutine smo121a
