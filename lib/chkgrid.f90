subroutine chkgrid(callsign,grid,lchkcall,lgvalid,lwrongcall)

  character, intent(in) :: callsign*12,grid*12
  logical(1) lchkcall,lgvalid,lwrongcall
  logical llook,lland

lchkcall=.false.; lgvalid=.false.; lwrongcall=.false.

! CE3TSK 2026-10-13: Antarctica. The square of a research station is looked into, whichever country runs it - the
! rules below say whose calls it takes: every call of the country that runs the station, because the home call is
! what an FT8 message with a square carries (LU1DMZ came so from Orcadas). A handful were looked into already
! (HB22, IB59, JA00, KC90...); these are the others - the stations, refuges and camps in use or kept today, the
! square across the edge where a station stands within 3 km of one (Kohnen is 200 m from JB05, and JB05 is what
! DP1POL sent from there), three squares at the South Pole beside JA00, and the three squares of the South Shetland
! Islands that hold no station. A visitor from a country without a station there, and a ship, are not covered
llook=.false.; lland=.false.
if(grid(2:2).gt.'@' .and. grid(2:2).lt.'D') then                          ! south of 60 degrees south
  if(grid(1:4).eq.'BA58' .or. grid(1:4).eq.'CB15' .or. grid(1:4).eq.'DA09' .or. grid(1:4).eq.'DB00' .or. &
     grid(1:4).eq.'DB30' .or. grid(1:4).eq.'DB40' .or. grid(1:4).eq.'EB80' .or. grid(1:4).eq.'FB45' .or. &
     grid(1:4).eq.'FB58' .or. grid(1:4).eq.'FC52' .or. grid(1:4).eq.'FC61' .or. grid(1:4).eq.'FC62' .or. &
     grid(1:4).eq.'FC74' .or. grid(1:4).eq.'FC75' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'FC86' .or. &
     grid(1:4).eq.'FC87' .or. grid(1:4).eq.'FC94' .or. grid(1:4).eq.'FC95' .or. grid(1:4).eq.'FC96' .or. &
     grid(1:4).eq.'HB64' .or. grid(1:4).eq.'HB74' .or. grid(1:4).eq.'IB36' .or. grid(1:4).eq.'IB45' .or. &
     grid(1:4).eq.'IB94' .or. grid(1:4).eq.'IB95' .or. grid(1:4).eq.'JB04' .or. grid(1:4).eq.'JB05' .or. &
     grid(1:4).eq.'JB17' .or. grid(1:4).eq.'JB18' .or. grid(1:4).eq.'JB28' .or. grid(1:4).eq.'KB18' .or. &
     grid(1:4).eq.'KB28' .or. grid(1:4).eq.'KB92' .or. grid(1:4).eq.'KC91' .or. grid(1:4).eq.'LB29' .or. &
     grid(1:4).eq.'LC00' .or. grid(1:4).eq.'LC22' .or. grid(1:4).eq.'LC32' .or. grid(1:4).eq.'MA89' .or. &
     grid(1:4).eq.'MB49' .or. grid(1:4).eq.'MB86' .or. grid(1:4).eq.'MC60' .or. grid(1:4).eq.'MC80' .or. &
     grid(1:4).eq.'MC91' .or. grid(1:4).eq.'NC63' .or. grid(1:4).eq.'OB31' .or. grid(1:4).eq.'OC03' .or. &
     grid(1:4).eq.'PA80' .or. grid(1:4).eq.'PA90' .or. grid(1:4).eq.'PB14' .or. grid(1:4).eq.'PB79' .or. &
     grid(1:4).eq.'QA00' .or. grid(1:4).eq.'QB24' .or. grid(1:4).eq.'QB93' .or. grid(1:4).eq.'QB97' .or. &
     grid(1:4).eq.'QC90' .or. grid(1:4).eq.'RB12' .or. grid(1:4).eq.'RB15' .or. grid(1:4).eq.'RB22' .or. &
     grid(1:4).eq.'RB25') llook=.true.
endif
! CE3TSK 2026-10-13: and Alaska's north coast, where the rule for the United States had no square - Utqiagvik (BQ11),
! Prudhoe Bay (BQ50), Kaktovik (BQ80): the thirteen squares of it along 70 degrees north
if(grid(2:2).eq.'Q' .and. (grid(1:1).eq.'A' .or. grid(1:1).eq.'B')) then
  if(grid(1:4).eq.'AQ80' .or. grid(1:4).eq.'AQ90' .or. grid(1:4).eq.'BQ00' .or. grid(1:4).eq.'BQ10' .or. &
     grid(1:4).eq.'BQ11' .or. grid(1:4).eq.'BQ20' .or. grid(1:4).eq.'BQ21' .or. grid(1:4).eq.'BQ30' .or. &
     grid(1:4).eq.'BQ40' .or. grid(1:4).eq.'BQ50' .or. grid(1:4).eq.'BQ60' .or. grid(1:4).eq.'BQ70' .or. &
     grid(1:4).eq.'BQ80') llook=.true.
endif
if(llook) then
  continue
else if(grid(1:1).gt.'@' .and. grid(1:1).lt.'J') then
  if(grid(1:1).eq.'A' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'E') .or. grid(2:2).eq.'M' .or. grid(2:2).eq.'N' .or. &
     grid(2:2).eq.'Q' .or. grid(2:2).eq.'R')) then; lchkcall=.true.

  else if(grid(1:1).eq.'B' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'G') .or. grid(2:2).eq.'M' .or. grid(2:2).eq.'N' .or. &
          grid(2:2).eq.'Q' .or. grid(2:2).eq.'R')) then; lchkcall=.true.

  else if(grid(1:1).eq.'C' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'G') .or. &
          (grid(2:2).eq.'G' .and. grid(3:3).gt.'7' .and. grid(4:4).lt.'4') .or. &
          (grid(2:2).eq.'I' .and. grid(3:3).gt.'0' .and. grid(4:4).gt.'1') .or. &
! CE3TSK 2026-10-13: squares that a rule below accepts and that stood among the empty ones here, so the rule was
! never asked, are looked into now - in this list and the ones that follow. CQ71: Canada's (Sachs Harbour)
          (grid(2:2).gt.'I' .and. grid(2:2).lt.'M') .or. (grid(2:2).eq.'Q' .and. grid(3:4).ne.'71') .or. &
          grid(2:2).eq.'R')) then; lchkcall=.true.

  else if(grid(1:1).eq.'D' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'G') .or. (grid(2:2).gt.'G'.and.grid(2:2).lt.'K').or. &
          (grid(2:2).eq.'G' .and. grid(3:4).ne.'52' .and. grid(3:4).ne.'73') .or. &
          (grid(2:2).eq.'Q' .and. grid(3:4).ne.'10') .or. grid(2:2).eq.'R')) then; lchkcall=.true. ! DQ10: Canada's

  else if(grid(1:1).eq.'E' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'C') .or. &
          (grid(2:2).eq.'C' .and. grid(3:4).ne.'41') .or. (grid(2:2).eq.'F' .and. grid(3:4).ne.'96') .or. &
          (grid(2:2).gt.'C' .and. grid(2:2).lt.'F') .or. (grid(2:2).eq.'G' .and. grid(3:4).ne.'93') .or.grid(2:2).eq.'H'.or. &
! CE3TSK 2026-10-13: nine squares of the Canadian Arctic that Canada's rule has (Resolute EQ24, Eureka EQ79...)
          (grid(2:2).eq.'Q' .and. grid(3:4).ne.'15' .and. grid(3:4).ne.'22' .and. grid(3:4).ne.'24' .and. &
           grid(3:4).ne.'71' .and. grid(3:4).ne.'73' .and. grid(3:4).ne.'79' .and. grid(3:4).ne.'84' .and. &
           grid(3:4).ne.'86' .and. grid(3:4).ne.'96') .or. &
          (grid(2:2).eq.'R' .and. grid(3:4).ne.'60'))) then; lchkcall=.true. ! VY0

! CE3TSK 2026-10-13: FC97 is looked into now - LZ0A in FC97, Livingston Island; it was among the empty squares
  else if(grid(1:1).eq.'F' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'C') .or. (grid(2:2).eq.'C'.and.grid(3:4).ne.'97').or. &
          (grid(2:2).eq.'Q'.and.grid(3:3).lt.'3'.and.grid(3:4).ne.'12') .or. & ! FQ12, FR71, FR82: Canada's (Alert)
          (grid(2:2).eq.'R'.and.grid(3:4).ne.'71'.and.grid(3:4).ne.'82'))) then; lchkcall=.true.

  else if(grid(1:1).eq.'G' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'C') .or. (grid(2:2).gt.'K'.and.grid(2:2).lt.'N').or. &
          (grid(2:2).eq.'K' .and. grid(3:3).gt.'0') .or. (grid(2:2).eq.'N' .and. grid(3:3).gt.'3') .or. &
          (grid(2:2).eq.'Q' .and. grid(4:4).gt.'4') .or. grid(2:2).eq.'R')) then; lchkcall=.true.

  else if(grid(1:1).eq.'H' .and. (grid(2:2).eq.'A' .or. (grid(2:2).eq.'B' .and. grid(3:4).ne.'22') .or. & ! LU1ZG in HB22QD
! CE3TSK 2026-10-13: HG59 (Trindade) and HJ50 (St Peter and St Paul): Brazil's rule has both
          grid(2:2).eq.'C' .or. (grid(2:2).gt.'D'.and.grid(2:2).lt.'H'.and.grid(2:4).ne.'G59') .or. &
          (grid(2:2).eq.'J'.and.grid(3:4).ne.'50') .or. grid(2:2).eq.'L' .or. grid(2:2).eq.'N' .or. grid(2:2).eq.'O' .or. &
          (grid(2:2).eq.'K' .and. grid(3:3).lt.'7') .or. (grid(2:2).eq.'M' .and. grid(4:4).lt.'6') .or. &
          (grid(2:2).eq.'Q' .and. grid(4:4).gt.'0') .or. grid(2:2).eq.'R')) then; lchkcall=.true.

! CE3TSK 2026-10-13: IE49 and IE59 (Gough Island): the rule for ZD9 has both. IB88: ZS7 at SANAE, added below
  else if(grid(1:1).eq.'I' .and. (grid(2:2).eq.'A' .or. &
          (grid(2:2).gt.'B' .and. grid(2:2).lt.'F' .and. grid(2:4).ne.'E49' .and. grid(2:4).ne.'E59') .or. &
          (grid(2:2).eq.'B' .and. grid(3:4).ne.'59' .and. grid(3:4).ne.'88') .or. & ! DP0GVN, ZS7
          (grid(2:2).eq.'F' .and. grid(3:4).ne.'32') .or. grid(2:2).eq.'G' .or. (grid(2:2).eq.'H' .and.grid(3:3).ne.'7').or. &
          (grid(2:2).eq.'I' .and. grid(3:4).ne.'22') .or. (grid(2:2).eq.'O' .and. grid(3:3).lt.'3') .or. &
          (grid(2:2).eq.'P' .and. grid(4:4).gt.'7') .or. (grid(2:2).eq.'Q' .and. grid(4:4).gt.'1') .or. grid(2:2).eq.'R')) &
    then; lchkcall=.true.
  endif

else  if(grid(1:1).gt.'I' .and. grid(1:1).lt.'S') then

  if(grid(1:1).eq.'J' .and. (grid(2:2).eq.'C' .or. &
      (grid(2:2).eq.'A' .and. grid(3:4).ne.'00') .or. & ! KC4AAA
      (grid(2:2).eq.'B' .and. grid(3:4).ne.'59') .or. & ! Novolazarevskaya
      (grid(2:2).eq.'D' .and. grid(3:4).ne.'15') .or. grid(2:2).eq.'E' .or. (grid(2:2).eq.'F' .and. grid(3:3).lt.'8') .or. &
      (grid(2:2).eq.'G' .and. grid(3:3).lt.'6') .or. (grid(2:2).eq.'H' .and. grid(3:3).lt.'5') .or. &
      (grid(2:2).eq.'I' .and. grid(3:3).lt.'2') .or. (grid(2:2).eq.'R' .and. grid(4:4).gt.'0'))) then; lchkcall=.true.

  else if(grid(1:1).eq.'K' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'C') .or. &
      (grid(2:2).eq.'C' .and. grid(3:4).ne.'90') .or. grid(2:2).eq.'D' .or. & ! 8J1RL
      (grid(2:2).eq.'E' .and. grid(3:4).ne.'83' .and. grid(3:4).ne.'93') .or. (grid(2:2).eq.'F' .and. grid(4:4).lt.'5') .or. &
      (grid(2:2).eq.'R' .and. grid(4:4).gt.'0'))) then; lchkcall=.true.

  else if(grid(1:1).eq.'L' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'E') .or. &
      (grid(2:2).eq.'E' .and. grid(3:4).ne.'53' .and. grid(3:4).ne.'54' .and. grid(3:4).ne.'63') .or. grid(2:2).eq.'F' .or. &
      (grid(2:2).eq.'G' .and. grid(4:4).lt.'4') .or. (grid(2:2).eq.'J' .and. grid(3:3).gt.'5') .or. &
      (grid(2:2).eq.'Q' .and. grid(4:4).lt.'5' .and. grid(4:4).lt.'9') .or. &
      (grid(2:2).eq.'R' .and. grid(4:4).gt.'1'))) then; lchkcall=.true.

! CE3TSK 2026-10-13: MC81 and MC12 - VK0 at Davis and Mawson, Antarctica, added below
  else if(grid(1:1).eq.'M' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'D' .and. grid(2:4).ne.'C81' .and. &
      grid(2:4).ne.'C12') .or. &
      (grid(2:2).eq.'D' .and. grid(3:4).ne.'66' .and. grid(3:4).ne.'67' .and. grid(3:4).ne.'49') .or. &
      (grid(2:2).eq.'E' .and. grid(3:4).ne.'40' .and. grid(3:4).ne.'41' .and. grid(3:4).ne.'50') .or. &
      (grid(2:2).eq.'F' .and. grid(3:4).ne.'81' .and. grid(3:4).ne.'82') .or. grid(2:2).eq.'G' .or. &
      (grid(2:2).eq.'H' .and. grid(3:4).ne.'10') .or. (grid(2:2).eq.'I' .and. grid(3:3).lt.'5') .or. &
      (grid(2:2).eq.'J' .and. grid(3:3).lt.'6') .or. (grid(2:2).eq.'K' .and. grid(3:3).lt.'5') .or. & 
      (grid(2:2).eq.'R' .and. grid(4:4).gt.'1'))) then; lchkcall=.true.

  else if(grid(1:1).eq.'N' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'H') .or. & 
      (grid(2:2).eq.'H' .and. grid(3:4).ne.'87' .and. grid(3:4).ne.'88') .or. & 
      (grid(2:2).eq.'I' .and. grid(3:3).lt.'9' .and. grid(3:4).ne.'89') .or. & 
      (grid(2:2).eq.'J' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'6') .or. & 
      (grid(2:2).eq.'R' .and. grid(4:4).gt.'1'))) then; lchkcall=.true.

! CE3TSK 2026-10-13: OC53 is looked into now - VK0 at Casey station, Antarctica; it was among the empty squares
  else if(grid(1:1).eq.'O' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'F' .and. grid(2:4).ne.'C53') .or. &
      (grid(2:2).eq.'F' .and. grid(3:3).lt.'7') .or. (grid(2:2).eq.'G' .and. grid(3:3).lt.'6') .or. &
      (grid(2:2).eq.'H' .and. grid(3:4).ne.'90' .and. grid(3:4).ne.'92' .and. grid(3:4).ne.'29' .and. grid(3:4).ne.'99').or. &
      grid(2:2).eq.'R')) then; lchkcall=.true.

! CE3TSK 2026-10-13: PC93 and, in the next list, QC03 - Dumont d'Urville, Antarctica (FT#Y), added below
  else if(grid(1:1).eq.'P' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'F' .and. grid(2:4).ne.'C93') .or. &
      (grid(2:2).eq.'F'.and.grid(4:4).lt.'2').or. &
      (grid(2:2).eq.'K' .and. grid(3:3).gt.'3' .and. grid(3:4).ne.'90') .or. &
      (grid(2:2).eq.'Q' .and. grid(4:4).gt.'6') .or. grid(2:2).eq.'R')) then; lchkcall=.true.

  else if(grid(1:1).eq.'Q' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'D' .and. grid(2:4).ne.'C03') .or. &
      (grid(2:2).eq.'D' .and. grid(3:4).ne.'94' .and. grid(3:4).ne.'95') .or. (grid(2:2).eq.'E' .and. grid(4:4).lt.'6') .or. &
      (grid(2:2).eq.'F' .and. grid(3:3).gt.'6' .and. grid(3:4).ne.'98') .or. &
      (grid(2:2).eq.'K' .and. grid(3:3).gt.'2' .and. grid(3:4).ne.'36') .or. &
      (grid(2:2).eq.'L' .and. grid(3:3).gt.'2' .and. grid(3:4).ne.'64' .and. grid(3:4).ne.'74') .or. &
      (grid(2:2).eq.'M' .and. grid(3:3).gt.'0' .and. grid(3:4).ne.'19') .or. &
      (grid(2:2).eq.'N' .and. grid(3:3).gt.'7') .or. &
      (grid(2:2).eq.'Q' .and. grid(4:4).gt.'7') .or. grid(2:2).eq.'R')) then; lchkcall=.true.

! CE3TSK 2026-10-13: RB32 (McMurdo, Scott Base): the rule for the United States has it
  else if(grid(1:1).eq.'R' .and. ((grid(2:2).gt.'@' .and. grid(2:2).lt.'D' .and. grid(2:4).ne.'B32') .or. &
      (grid(2:2).eq.'D' .and. grid(3:4).ne.'47' .and. grid(3:4).ne.'29' .and. grid(3:4).ne.'39') .or. &
      (grid(2:2).eq.'K' .and. grid(4:4).gt.'4' .and. grid(3:4).ne.'39') .or. (grid(2:2).gt.'K' .and. grid(2:2).lt.'O') .or. &
      (grid(2:2).eq.'O' .and. grid(4:4).eq.'0') .or. (grid(2:2).eq.'Q' .and. grid(4:4).gt.'1') .or. grid(2:2).eq.'R')) &
    then; lchkcall=.true.
  endif

else if (grid(1:1).gt.'/' .and. grid(1:1).lt.':') then ! 0..9
  lchkcall=.true.
endif

if(.not.lchkcall) then

  if(callsign(1:1).gt.'K' .and. callsign(1:1).lt.'[') then; go to 2
  else if(callsign(1:1).gt.'/' .and. callsign(1:1).lt.':') then; go to 4
  endif

  if(callsign(1:1).eq.'A') then

! USA AA..AL,K,N,W
    if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'M') then
! CE3TSK 2026-10-13: land that this rule refused - the United States (the south tip of Big Bend, Texas); Hawaii (the
! Northwestern Hawaiian Islands); Alaska (its north coast - Utqiagvik BQ11, Prudhoe Bay BQ50)
      if(grid(1:4).eq.'AL63' .or. grid(1:4).eq.'AL65' .or. grid(1:4).eq.'AQ80' .or. grid(1:4).eq.'AQ90' .or. &
         grid(1:4).eq.'BQ00' .or. grid(1:4).eq.'BQ10' .or. grid(1:4).eq.'BQ11' .or. grid(1:4).eq.'BQ20' .or. &
         grid(1:4).eq.'BQ21' .or. grid(1:4).eq.'BQ30' .or. grid(1:4).eq.'BQ40' .or. grid(1:4).eq.'BQ50' .or. &
         grid(1:4).eq.'BQ60' .or. grid(1:4).eq.'BQ70' .or. grid(1:4).eq.'BQ80' .or. grid(1:4).eq.'DL88') lland=.true.
! CE3TSK 2026-10-13: Antarctica - the stations of the United States: the South Pole (JA00, as its callbook entry has it,
! and PA90 with the squares either side - the station stands 300 m from the Pole at 139 degrees east), McMurdo RB32 and
! the Sound west of it RB22 (the icebreaker of the yearly resupply: KK7YXD was heard from there), Marble Point RB12, the
! Allan Hills camp QB93, Palmer FC75 (and FC85, 2 km east of it: KC4AAC has sent both), Copacabana GC07, Cape Shirreff
! FC97, Siple Dome BA58, WAIS Divide DB30 and Byrd DA09 (and DB40, DB00, 2 km from them)
      if(grid(1:4).eq.'JA00' .or. grid(1:4).eq.'PA80' .or. grid(1:4).eq.'PA90' .or. grid(1:4).eq.'QA00' .or. &
         grid(1:4).eq.'RB32' .or. grid(1:4).eq.'RB22' .or. grid(1:4).eq.'RB12' .or. grid(1:4).eq.'QB93' .or. &
         grid(1:4).eq.'FC75' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'GC07' .or. grid(1:4).eq.'FC97' .or. &
         grid(1:4).eq.'BA58' .or. grid(1:4).eq.'DB30' .or. grid(1:4).eq.'DB40' .or. grid(1:4).eq.'DA09' .or. &
         grid(1:4).eq.'DB00') lgvalid=.true.
      if((grid(1:2).eq.'FM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':').or. &
       (grid(1:2).eq.'EL' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'DL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
       grid(1:2).eq.'EM' .or. grid(1:2).eq.'DM' .or. grid(1:2).eq.'EN' .or. &
       (grid(1:2).eq.'CM' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'FN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') .or. &
       (grid(1:2).eq.'DN' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9') .or. &
       (grid(1:2).eq.'CN' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9') .or. &
       (grid(1:2).eq.'CO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'FK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.'9') .or. &
       grid(1:2).eq.'BP' .or. &
       (grid(1:2).eq.'BO' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'AP' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
       grid(1:4).eq.'FK28' .or. &
       (grid(1:2).eq.'AO' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'BL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
       (grid(1:2).eq.'BK' .and. (grid(3:4).eq.'19' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'29')) .or. &
       (grid(1:2).eq.'AL' .and. (grid(3:4).eq.'91' .or. grid(3:4).eq.'92' .or. grid(3:4).eq.'08' .or. grid(3:4).eq.'18' .or. &
        grid(3:4).eq.'27' .or. grid(3:4).eq.'36' .or. grid(3:4).eq.'45' .or. grid(3:4).eq.'64' .or. &
        grid(3:4).eq.'73' .or. grid(3:4).eq.'93')) .or. &
       (grid(1:3).eq.'QK2' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'AO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3') .or. &
       (grid(1:2).eq.'RO' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3').or. &
        grid(3:4).eq.'63')) .or. &
       grid(1:4).eq.'AP30' .or. grid(1:4).eq.'QK36' .or. grid(1:4).eq.'QL20' .or. grid(1:4).eq.'RK39' .or. &
! CE3TSK 2026-10-13: AH45 and AH55 added - American Samoa (Tutuila, Manu'a); of it only Swains Island (AH48) was here
       grid(1:4).eq.'AH45' .or. grid(1:4).eq.'AH55' .or. &
! CE3TSK 2026-10-13: these had no square - Guantanamo Bay (KG4) FK29, Baker and Howland (KH1) AJ10, Palmyra,
! Kingman Reef and Jarvis (KH5) AJ85, AJ86, AI99; and CM79 holds the coast of California just south of 40 north
       grid(1:4).eq.'FK29' .or. grid(1:4).eq.'AJ10' .or. grid(1:4).eq.'AJ85' .or. grid(1:4).eq.'AJ86' .or. &
! CE3TSK 2026-10-13: and CM93, the Channel Islands of California south of 34 north (Santa Rosa)
       grid(1:4).eq.'AI99' .or. grid(1:4).eq.'CM79' .or. grid(1:4).eq.'CM93' .or. &
       grid(1:4).eq.'AK56' .or. grid(1:4).eq.'AH48' .or. grid(1:4).eq.'JA00' .or. grid(1:4).eq.'RB32' .or. &
       grid(1:4).eq.'CP00') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'FM' .and. (grid(3:4).eq.'12' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'23' .or. grid(3:4).eq.'24')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'EL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9' &
              .and. grid(3:4).ne.'28'.and. grid(3:4).ne.'58') .or. grid(3:4).eq.'04' .or. grid(3:4).eq.'05' .or. &
              grid(3:4).eq.'14')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DL' .and. (grid(3:4).eq.'78' .or. grid(3:4).eq.'88')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DM' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EN' .and. ((grid(3:3).eq.'9' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
             ((grid(3:3).eq.'8' .or. grid(3:3).eq.'7') .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
             (grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).eq.'9' .and. grid(3:3).ne.'2'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'CM' .and. (grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FN' .and. ((grid(4:4).eq.'7' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
              (grid(4:4).eq.'6' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4') .or. &
              (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4') .or. &
              grid(3:4).eq.'04' .or. grid(3:4).eq.'05' .or. grid(3:4).eq.'15' .or. grid(3:4).eq.'52')) then; lgvalid=.false.
      else if(grid(1:2).eq.'CO' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              (grid(3:3).eq.'1' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'7') .or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'24' .or. grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'BO' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':'.and.grid(4:4).gt.'3'.and.grid(4:4).lt.'9').or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              grid(3:4).eq.'14' .or. grid(3:4).eq.'24' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'35')) then; lgvalid=.false.
      else if(grid(1:2).eq.'AP' .and. ((grid(3:3).eq.'4' .and. ((grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
              (grid(4:4).gt.'3' .and. grid(4:4).lt.':'))) .or. &
              (grid(3:3).eq.'5' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'50' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'62' .or. grid(3:4).eq.'63' .or. &
              grid(3:4).eq.'67' .or. grid(3:4).eq.'69' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
      else if(grid(1:2).eq.'AO' .and. ((grid(3:3).eq.'4' .and. ((grid(4:4).gt.'2' .and. grid(4:4).lt.'7') .or. &
              grid(4:4).eq.'8' .or. grid(4:4).eq.'9')) .or. &
              (grid(3:3).eq.'5' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':' .and. grid(4:4).ne.'6') .or. &
              ((grid(3:3).eq.'6' .or. grid(3:3).eq.'7') .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. &
              (grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'4') .or. &
              grid(3:4).eq.'62' .or. grid(3:4).eq.'97' .or. grid(3:4).eq.'86' .or. grid(3:4).eq.'87' .or.grid(3:4).eq.'31')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'BL' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'12' .or. grid(3:4).eq.'21' .or. &
              grid(3:4).eq.'22')) then; lgvalid=.false.
      else if(grid(1:2).eq.'RO' .and. (grid(3:4).eq.'61' .or. grid(3:4).eq.'71')) then; lgvalid=.false.
      endif
endif

! Australia AX,VH..VN,VZ
    else if(callsign(1:2).eq.'AX') then
! CE3TSK 2026-10-13: land that this rule refused - Australia (Macquarie Island with them)
      if(grid(1:4).eq.'OH90' .or. grid(1:4).eq.'QD95' .or. grid(1:4).eq.'QF98' .or. grid(1:4).eq.'QI10') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Australia's stations for every call of the rule: Casey OC53, Davis MC81 (and MC91, 1
! km east of it), Mawson MC12, Law MC80, Edgeworth David OC03, the ice-core camp at Dome C PB14
      if(grid(1:4).eq.'OC53' .or. grid(1:4).eq.'MC81' .or. grid(1:4).eq.'MC91' .or. grid(1:4).eq.'MC12' .or. &
         grid(1:4).eq.'MC80' .or. grid(1:4).eq.'OC03' .or. grid(1:4).eq.'PB14') lgvalid=.true.
      if((grid(1:2).eq.'QG' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7') .or. &
         (grid(1:2).eq.'QF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7') .or. &
         (grid(1:2).eq.'PF' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
         (grid(1:2).eq.'OF' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'QE' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and.grid(4:4).gt.'5'.and. grid(4:4).lt.':').or. &
          grid(3:4).eq.'19')) .or. &
         (grid(1:2).eq.'QH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. & 
         (grid(1:2).eq.'OG' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':') .or. & 
         grid(1:2).eq.'PG' .or. grid(1:2).eq.'PH' .or. &
! CE3TSK 2026-10-13: the islands and the Antarctic bases as the rule for VK has them; an AX call had none
         (callsign(3:3).eq.'0' .and. &
          ((grid(1:2).eq.'MD' .and. (grid(3:4).eq.'66' .or. grid(3:4).eq.'67')) .or. &
           (grid(1:2).eq.'QD' .and. (grid(3:4).eq.'95' .or. grid(3:4).eq.'94')) .or. grid(1:4).eq.'OC53' .or. &
           grid(1:4).eq.'MC81' .or. grid(1:4).eq.'MC12')) .or. &
         (callsign(3:3).eq.'9' .and. &
          ((grid(1:2).eq.'NH' .and. (grid(3:4).eq.'87' .or. grid(3:4).eq.'88')) .or. &
           grid(1:4).eq.'QF98' .or. grid(1:4).eq.'OH29' .or. &
           (grid(1:2).eq.'RG' .and. (grid(3:4).eq.'30' .or. grid(3:4).eq.'31')) .or. &
           (grid(1:2).eq.'QH' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'1' .and. &
            grid(4:4).lt.'5')))) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'QF' .and. grid(3:3).eq.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7') then; lgvalid=.false.
      else if(grid(1:2).eq.'PF' .and. ((grid(3:3).gt.'/'.and.grid(3:3).lt.'8'.and.grid(4:4).gt.'1'.and.grid(4:4).lt.'5').or. &
              (grid(4:4).eq.'5' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'7') .or. &
              (grid(4:4).eq.'6' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'7') .or. grid(3:4).eq.'57')) then; lgvalid=.false.
      else if(grid(1:4).eq.'OF74') then; lgvalid=.false.
      else if(grid(1:2).eq.'QH' .and. ((grid(3:3).eq.'3' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. grid(3:4).eq.'09')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OG' .and. (grid(3:4).eq.'60' .or. grid(3:4).eq.'69')) then; lgvalid=.false.
      else if(grid(1:2).eq.'PH' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':' .and. &
              grid(4:4).ne.'5' .and. grid(4:4).ne.'6') .or. &
              (grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
              (grid(3:3).eq.'9' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'59' .or. grid(3:4).eq.'79' .or. grid(3:4).eq.'89')) then; lgvalid=.false.
      endif
endif

! Pakistan 6P..6S, AP..AS
    else if(callsign(2:2).gt.'O' .and. callsign(2:2).lt.'T') then
! CE3TSK 2026-10-13: land that this rule refused - Pakistan
      if(grid(1:4).eq.'ML43' .or. grid(1:4).eq.'MM44' .or. grid(1:4).eq.'MM77' .or. grid(1:4).eq.'MM86') lland=.true.
      if((grid(1:2).eq.'MM' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'7').or. &
          grid(3:4).eq.'30' .or. grid(3:4).eq.'31' .or. grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) .or. & 
         (grid(1:2).eq.'ML' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'33'))) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'MM' .and. grid(3:3).eq.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'7') then; lgvalid=.false.
      else if(grid(1:2).eq.'ML' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              grid(3:4).eq.'07')) then; lgvalid=.false.
      endif
endif

! Spain AM..AO,EA..EH
    else if(callsign(1:1).eq.'A' .and. callsign(2:2).gt.'L' .and. callsign(2:2).lt.'P') then
! CE3TSK 2026-10-13: Antarctica - Spain's stations Juan Carlos I and Gabriel de Castilla, both FC97 (and FC96, 3 km
! south of the second)
      if(grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC96') lgvalid=.true.
      if((grid(1:2).eq.'IN' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4' .and. &
          grid(3:4).ne.'50').or. &
         (grid(1:2).eq.'IM' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'12' .or. grid(3:4).eq.'13' .or. grid(3:4).eq.'75' .or. grid(3:4).eq.'85')) .or. &
! CE3TSK 2026-10-13: JN02 and JN12 were missing - the Pyrenees side of Aragon and Catalonia (Figueres is JN12)
         (grid(1:2).eq.'JN' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'3').or. &
          grid(3:4).eq.'20')) .or. &
         (grid(1:2).eq.'JM' .and. (grid(3:4).eq.'08' .or. grid(3:4).eq.'09' .or.grid(3:4).eq.'19'.or.grid(3:4).eq.'29')).or. &
         (grid(1:2).eq.'IL' .and. ((grid(4:4).eq.'7' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3') .or. &
         (grid(4:4).eq.'8' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4') .or. grid(3:4).eq.'39'))) lgvalid=.true.

! Botswana 8O,A2
      else if(callsign(1:2).eq.'A2') then
! CE3TSK 2026-10-13: land that this rule refused - Botswana
        if(grid(1:4).eq.'JG95' .or. grid(1:4).eq.'JG96' .or. grid(1:4).eq.'JG97') lland=.true.
        if((grid(1:2).eq.'KG' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'2'.and.grid(4:4).lt.':').or. &
        (grid(1:2).eq.'KH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
        (grid(1:2).eq.'KH' .and. (grid(3:4).eq.'22' .or. grid(3:4).eq.'30')) .or. &
        (grid(1:2).eq.'KG' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'47'))) lgvalid=.true.

! Tonga A3
      else if(callsign(1:2).eq.'A3') then
! CE3TSK 2026-10-13: land that this rule refused - Tonga
        if(grid(1:4).eq.'AG17' .or. grid(1:4).eq.'AH20') lland=.true.
        if((grid(1:2).eq.'AG' .and. (grid(3:4).eq.'28' .or.grid(3:4).eq.'29'.or.grid(3:4).eq.'06'.or.grid(3:4).eq.'07')).or. &
           (grid(1:2).eq.'AH' .and. (grid(3:4).eq.'21' .or.grid(3:4).eq.'31'.or.grid(3:4).eq.'24'.or.grid(3:4).eq.'34'))) &
          lgvalid=.true.

! Oman A4
      else if(callsign(1:2).eq.'A4') then
! CE3TSK 2026-10-13: land that this rule refused - Oman
        if(grid(1:4).eq.'LK58' .or. grid(1:4).eq.'LL85' .or. grid(1:4).eq.'LL86') lland=.true.
        if((grid(1:2).eq.'LL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
           (grid(1:2).eq.'LK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':')) &
          lgvalid=.true.

! Bhutan A5
      else if(callsign(1:2).eq.'A5') then
        if(grid(1:2).eq.'NL' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.'9') &
          lgvalid=.true.

! United Arab Emirates A6
      else if(callsign(1:2).eq.'A6') then
! CE3TSK 2026-10-13: land that this rule refused - the United Arab Emirates
        if(grid(1:4).eq.'LL86') lland=.true.
        if(grid(1:2).eq.'LL' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and.grid(4:4).gt.'1'.and.grid(4:4).lt.'6').or. &
           grid(3:4).eq.'53' .or. grid(3:4).eq.'54')) lgvalid=.true.

! Qatar A7
      else if(callsign(1:2).eq.'A7') then
        if((grid(1:3).eq.'LL5' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.'7') .or. grid(1:4).eq.'LL65') lgvalid=.true.

! Bahrain A9
      else if(callsign(1:2).eq.'A9') then
        if(grid(1:3).eq.'LL5' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.'7') lgvalid=.true.

! Argentina AY,AZ, L1..L9, LO..LW
      else if(callsign(1:2).eq.'AY' .or. callsign(1:2).eq.'AZ') then
! CE3TSK 2026-10-13: Antarctica - Argentina's stations: Belgrano II HB22, Orcadas GC79 (LU1DMZ, LU1ZA), Marambio GC15,
! San Martin FC61, Esperanza and Petrel GC16, Carlini and Camara GC07, Decepcion FC97 (and FC96, 2 km south of it),
! Brown and Melchior FC85, Matienzo and Primavera FC95 (and FC94, 3 km south of Matienzo)
      if(grid(1:4).eq.'HB22' .or. grid(1:4).eq.'GC79' .or. grid(1:4).eq.'GC15' .or. grid(1:4).eq.'FC61' .or. &
         grid(1:4).eq.'GC16' .or. grid(1:4).eq.'GC07' .or. grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC96' .or. &
         grid(1:4).eq.'FC85' .or. grid(1:4).eq.'FC95' .or. grid(1:4).eq.'FC94') lgvalid=.true.
      if((grid(1:2).eq.'GF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'FF' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
! CE3TSK 2026-10-13: of the field FG only the two rows north of 22 degrees south were accepted, where the
! country ends, and none of the eight below them, which are its north-west (Salta, Jujuy, Tucuman, Catamarca,
! Santiago del Estero, the Chaco). Every row is accepted now; FG56 and FG57 stay out by the exclusion below.
         (grid(1:2).eq.'FG' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'GG' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'6').or. &
         (grid(1:2).eq.'FE' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'9') .or. &
         (grid(1:2).eq.'FD' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC16' .or. grid(1:4).eq.'HB22') lgvalid=.true. ! LU1ZB in GC16, LU1ZG in HB22

if(lgvalid) then
      if(grid(1:2).eq.'GF' .and. (grid(3:4).eq.'16' .or. grid(3:4).eq.'17')) then; lgvalid=.false.
! CE3TSK 2026-10-13: and the squares beyond the country's northern end, which hold nothing of it: FG58 (Chile,
! Bolivia), FG59..FG99 (Bolivia, Paraguay - the country ends at 21 degrees 47 minutes south, in row 8), FG98 and
! GG25 (Paraguay, Brazil). Each lies 55 km or more beyond the border; GG15, 5 km beyond it, stays
      else if(grid(1:2).eq.'FG' .and. (grid(3:4).eq.'56' .or. grid(3:4).eq.'57' .or. grid(3:4).eq.'58' .or. &
              grid(3:4).eq.'98' .or. (grid(4:4).eq.'9' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'GG' .and. (grid(3:4).eq.'20' .or. grid(3:4).eq.'30' .or. grid(3:4).eq.'31' .or. &
              grid(3:4).eq.'35' .or. grid(3:4).eq.'25')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FE' .and. ((grid(3:3).eq.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7') .or. &
              (grid(3:3).eq.'3' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'70' .or. grid(3:4).eq.'73')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FD' .and. ((grid(3:3).gt.'2'.and.grid(3:3).lt.'5'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.'8').or. &
              (grid(3:3).eq.'7' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
              (grid(3:3).eq.'6' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':'))) then; lgvalid=.false.
      endif
endif
! CE3TSK 2026-10-13: squares that hold a strip of the country and were refused, most of them by the exclusions
! above - along the Andes (FG40, FE35, FE36, FE37: 60 to 840 km2), on the coast of Santa Cruz (FD69), in
! Tierra del Fuego (FD47, FD64) and Isla de los Estados (FD85). A square can be two countries'
      if(grid(1:4).eq.'FG40' .or. grid(1:4).eq.'FE35' .or. grid(1:4).eq.'FE36' .or. grid(1:4).eq.'FE37' .or. &
         grid(1:4).eq.'FD69' .or. grid(1:4).eq.'FD47' .or. grid(1:4).eq.'FD64' .or. grid(1:4).eq.'FD85') lgvalid=.true.

! India 8T..8Y,AT..AW,VT..VW
      else if(callsign(1:1).eq.'A' .and. callsign(2:2).gt.'S' .and. callsign(2:2).lt.'X') then
! CE3TSK 2026-10-13: Antarctica - India's stations: Maitri JB59, Bharati MC80
        if(grid(1:4).eq.'JB59' .or. grid(1:4).eq.'MC80') lgvalid=.true.
        if((grid(1:2).eq.'MJ' .and. (grid(3:4).eq.'99' .or. grid(3:4).eq.'89' .or. grid(3:4).eq.'88' .or. &
! CE3TSK 2026-10-13: MJ98 (Thoothukudi) and, below, MM85 and MM95 (Ladakh north of 35 degrees): India's, and refused
            grid(3:4).eq.'68' .or. grid(3:4).eq.'98')).or. &
           (grid(1:2).eq.'MK' .and. ((grid(3:3).gt.'5'.and.grid(3:3).lt.':').or.grid(3:4).eq.'51'.or.grid(3:4).eq.'52')).or. &
           (grid(1:2).eq.'NK' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.'6') .or. &
            (grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or.grid(3:4).eq.'39')).or. &
           (grid(1:2).eq.'ML' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
           (grid(1:2).eq.'NL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
! CE3TSK 2026-10-13: the Andaman and Nicobar Islands (VU4) had no square; theirs are accepted now
           (grid(1:3).eq.'NK6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. grid(1:4).eq.'NK73' .or. &
           (grid(1:3).eq.'NJ6' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
! CE3TSK 2026-10-13: there was nothing for the north-east (Assam and its neighbours, 90 to 98 degrees east);
! the squares the country touches there are accepted now (geodata/dxcc_grids.json, Natural Earth)
           (grid(1:2).eq.'NL' .and. ((grid(3:3).eq.'5' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'8') .or. &
            (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'9') .or. &
            (grid(3:3).eq.'7' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
            (grid(3:3).eq.'8' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':'))) .or. &
           (grid(1:2).eq.'MM' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
           grid(1:4).eq.'NM00' .or. grid(1:4).eq.'MM85' .or. grid(1:4).eq.'MM95') lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'MK' .and. grid(3:3).eq.'6' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.'5') then; lgvalid=.false.
        else if(grid(1:2).eq.'NK' .and. (grid(3:4).eq.'26' .or. grid(3:4).eq.'27')) then; lgvalid=.false.
! CE3TSK 2026-10-13: this exclusion took rows 0, 5, 8 and 9 out of EVERY column of ML east of 70 degrees -
! Delhi is ML88, Jhansi ML95, Bikaner ML68. Only ML58 and ML59 are left in it.
! CE3TSK 2026-10-13: ML49 joins this exclusion (Pakistan, 115 km beyond the border) and ML58 leaves it (it holds
! 276 km2 of India). The line that stood here for NL excluded nothing but NL40: it asked for a row digit that
! is 9, or 8, and below 5. The squares north of the Himalaya that hold nothing of India are named now - NL19,
! NL28, NL29, NL39, NL49 (Nepal, Tibet; 54 to 165 km). NL09 and NL48 have India in them and stay; so do MK51
! and MK52 (Lakshadweep's outer reefs) and the squares the border comes within 25 km of
        else if(grid(1:2).eq.'ML' .and. (grid(3:4).eq.'59' .or. grid(3:4).eq.'49')) then; lgvalid=.false.
        else if(grid(1:2).eq.'NL' .and. (grid(3:4).eq.'19' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'29' .or. &
                grid(3:4).eq.'39' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'40')) then; lgvalid=.false.
        else if(grid(1:2).eq.'MM' .and. (grid(3:4).eq.'61' .or. grid(3:4).eq.'62')) then; lgvalid=.false.
        endif
endif

! Liberia 5L,5M,6Z,A8,D5,EL
    else if(callsign(1:2).eq.'A8') then
      if(grid(1:2).eq.'IJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9' .and. &
         grid(3:4).ne.'67' .and. grid(3:4).ne.'67') lgvalid=.true.

    endif ! 'A'

  else if(callsign(1:1).eq.'B') then
    if(callsign(2:2).gt.'/' .and. callsign(2:2).lt.':' .and. callsign(3:3).gt.'/' .and. callsign(3:3).lt.':') return ! checkcall
! CE3TSK 2026-10-13: land that this rule refused - China
    if(.not.((callsign(2:2).gt.'L' .and. callsign(2:2).lt.'R') .or. (callsign(2:2).gt.'T' .and. callsign(2:2).lt.'Y'))) then
      if(grid(1:4).eq.'MN60' .or. grid(1:4).eq.'MN92' .or. grid(1:4).eq.'MN94' .or. grid(1:4).eq.'OK55' .or. &
         grid(1:4).eq.'OK56' .or. grid(1:4).eq.'OK66' .or. grid(1:4).eq.'ON55' .or. grid(1:4).eq.'ON65') lland=.true.
    endif
! CE3TSK 2026-10-13: Antarctica - China's stations: Great Wall GC07, Zhongshan MC80, Kunlun MA89, Taishan MB86, Qinling
! RB15
    if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'MC80' .or. grid(1:4).eq.'MA89' .or. grid(1:4).eq.'MB86' .or. &
       grid(1:4).eq.'RB15') lgvalid=.true.
! China, Taiwan B, 3H..3U, XS
    if(grid(1:2).eq.'OL' .or. &
       (grid(1:2).eq.'PL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2') .or. grid(1:2).eq.'OM' .or. &
       (grid(1:2).eq.'PM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3') .or. &
       (grid(1:2).eq.'PN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8') .or. &
       (grid(1:2).eq.'PO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
       (grid(1:2).eq.'OK' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'58' .or.grid(3:4).eq.'59')).or. &
       grid(1:2).eq.'ON' .or. grid(1:4).eq.'OO90' .or. grid(1:4).eq.'OO91' .or. grid(1:2).eq.'NN' .or. grid(1:2).eq.'NM'.or. &
       (grid(1:2).eq.'NL' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'MN' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
       (grid(1:2).eq.'MM' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':')) lgvalid=.true.

if(lgvalid) then
! grid exceptions for Taiwan BM,BN,BO,BP,BQ,BU,BV,BW,BX
    if((callsign(2:2).gt.'L' .and. callsign(2:2).lt.'R') .or. (callsign(2:2).gt.'T' .and. callsign(2:2).lt.'Y')) then
! CE3TSK 2026-10-13: OL94 added - Kinmen
      if(grid(1:4).ne.'PL04' .and. grid(1:4).ne.'PL03' .and. grid(1:4).ne.'PL05' .and. grid(1:4).ne.'PL02' .and. &
         grid(1:4).ne.'PL01' .and. grid(1:4).ne.'OL93' .and. grid(1:4).ne.'PL15' .and. grid(1:4).ne.'OL80' .and. &
         grid(1:4).ne.'OL94') lgvalid=.false.
    endif
endif
if(lgvalid) then
       if(grid(1:2).eq.'NL' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.'7'.and. &
          grid(3:4).ne.'83' .and. grid(3:4).ne.'84' .and. grid(3:4).ne.'85') .or. grid(3:4).eq.'77' .or. grid(3:4).eq.'87')) &
        then; lgvalid=.false.
       else if(grid(1:2).eq.'ON' .and.((grid(3:3).gt.'/'.and.grid(3:3).lt.'5'.and.grid(4:4).gt.'2'.and.grid(4:4).lt.':').or. &
               (grid(3:3).gt.'4' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':') .or. &
               grid(3:4).eq.'76' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
       else if(grid(1:2).eq.'PN' .and. ((grid(3:3).eq.'7' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6') .or. &
               (grid(3:3).eq.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5') .or. &
! CE3TSK 2026-10-13: PN52, PN53 and PN54 are no longer in this exclusion - the east of Jilin and Heilongjiang
! (Hunchun, Suifenhe), with stations on the air. PN50 and PN51 stay in it.
               grid(3:4).eq.'40' .or. grid(3:4).eq.'50' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'59' .or. &
               grid(3:4).eq.'69' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
       else if(grid(1:2).eq.'MM' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8') .or. &
               (grid(3:3).eq.'7' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6') .or. &
               (grid(3:3).eq.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5'))) then; lgvalid=.false.
       else if(grid(1:2).eq.'OL' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).eq.'0') .or. &
               grid(3:4).eq.'11' .or. grid(3:4).eq.'21')) then; lgvalid=.false.
       endif
endif
if(.not.lgvalid) then
! CE3TSK 2026-10-13: Scarborough Reef (BS7) had no square
    if(callsign(1:3).eq.'BS7' .and. grid(1:4).eq.'OK85') lgvalid=.true.
! Spratly Islands 1S,9M0,BM9S,BN9S,BO9S,BP9S,BQ9S,BU9S,BV9S,BW9S,BX9S
    if(callsign(3:4).eq.'9S' .and. ((callsign(2:2).gt.'K' .and. callsign(2:2).lt.'R') .or. (callsign(2:2).gt.'T'.and. &
              callsign(2:2).lt.'Y'))) then
      if((grid(1:2).eq.'OK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
         (grid(1:2).eq.'OJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':')) &
        lgvalid=.true.
endif

    endif ! 'B'

  else if(callsign(1:1).eq.'C') then

 ! Chile 3G,CA..CE,XQ,XR
    if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'F') then
! CE3TSK 2026-10-13: Antarctica - Chile's stations for every call of the rule: Frei, Escudero and Prat GC07, O'Higgins
! GC16, Guillermo Mann FC97, Gonzalez Videla and Yelcho FC85, Carvajal FC52, Union Glacier EB80
    if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC16' .or. grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC85' .or. &
       grid(1:4).eq.'FC52' .or. grid(1:4).eq.'EB80') lgvalid=.true.
! CE3TSK 2026-10-13: FF56 added - the upper Maipo valley, where the border runs east of 70 degrees west
      if((grid(1:2).eq.'FF' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'5') .or. grid(1:4).eq.'FF56' .or. &
! CE3TSK 2026-10-13: and FF55 and FF59, strips of the same kind. And Chile's Antarctic calls (the third character
! is 9) had no square: the South Shetland Islands, and the base on the peninsula across from them (GC16)
         grid(1:4).eq.'FF55' .or. grid(1:4).eq.'FF59' .or. &
! CE3TSK 2026-10-13: and FC86, FC87, FC96 - Smith and Low Islands and the south end of Deception, uninhabited
         (callsign(3:3).eq.'9' .and. (grid(1:4).eq.'FC97' .or. grid(1:4).eq.'GC28' .or. &
          grid(1:4).eq.'FC86' .or. grid(1:4).eq.'FC87' .or. grid(1:4).eq.'FC96' .or. &
          (grid(1:3).eq.'GC0' .and. (grid(4:4).eq.'7' .or. grid(4:4).eq.'8')) .or. &
          (grid(1:3).eq.'GC1' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.'9'))) .or. &
         (grid(1:2).eq.'FG' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'6') .or. &
          (grid(3:3).eq.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'8'))) .or. &
         (grid(1:2).eq.'FE' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5') .or. &
         (grid(1:2).eq.'FH' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
          grid(3:4).eq.'52')) .or. &
         (grid(1:2).eq.'FD' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'53' .or. grid(3:4).eq.'85')) .or. &
         grid(1:4).eq.'EF96' .or. grid(1:4).eq.'FF06' .or. grid(1:4).eq.'EG93' .or. grid(1:4).eq.'FG03' .or. &
         grid(1:4).eq.'DG52' .or. grid(1:4).eq.'DG73') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'FF' .and. grid(3:3).eq.'3' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') then; lgvalid=.false.
      else if(grid(1:2).eq.'FE' .and. (grid(3:4).eq.'40' .or. grid(3:4).eq.'41' .or. grid(3:4).eq.'29')) then; lgvalid=.false.
      endif
endif

! Cuba CL,CM,CO,T4
    else if(callsign(1:2).eq.'CL' .or. callsign(1:2).eq.'CM' .or. callsign(1:2).eq.'CO') then
! CE3TSK 2026-10-13: land that this rule refused - Cuba
      if(grid(1:4).eq.'FK29') lland=.true.
      if((grid(1:2).eq.'EL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'FL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3') .or. grid(1:4).eq.'FK19') lgvalid=.true.

! Canada CF..CK,CY,CZ,VA..VG,VO,VX,VY,XJ..XO
    else if((callsign(2:2).gt.'E' .and. callsign(2:2).lt.'L').or. callsign(1:2).eq.'CY' .or. callsign(1:2).eq.'CZ') then
      if((grid(1:2).eq.'CN' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'FN' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
         (grid(1:2).eq.'EN' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. grid(1:2).eq.'CO' .or. &
         (grid(1:2).eq.'GN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         grid(1:2).eq.'FO' .or. grid(1:2).eq.'EO' .or. grid(1:2).eq.'DO' .or. &
         (grid(1:2).eq.'GO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'DN'.and. grid(4:4).gt.'8' .and. grid(4:4).lt.':') .or. &
         grid(1:2).eq.'CP' .or. grid(1:2).eq.'DP' .or. grid(1:2).eq.'EP' .or. &
         (grid(1:2).eq.'FP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8').or. &
          grid(3:4).eq.'96')) .or. &
         (grid(1:2).eq.'EQ' .and. (grid(3:4).eq.'15' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'24' .or.grid(3:4).eq.'71'.or. &
          grid(3:4).eq.'73' .or. grid(3:4).eq.'79' .or. grid(3:4).eq.'84' .or. grid(3:4).eq.'86' .or.grid(3:4).eq.'96')).or. &
         (grid(1:2).eq.'FR' .and. (grid(3:4).eq.'71' .or. grid(3:4).eq.'82')) .or. &
         (grid(1:2).eq.'FQ' .and. (grid(3:4).eq.'50' .or. grid(3:4).eq.'12')) .or. grid(1:4).eq.'GN03' .or. &
         grid(1:4).eq.'FN93' .or. grid(1:4).eq.'ER60' .or. grid(1:4).eq.'DQ10' .or. grid(1:4).eq.'CQ71') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'EO' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'7' .and.grid(4:4).lt.':').or. &
         (grid(4:4).eq.'7' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'9') .or. grid(3:4).eq.'76' .or. grid(3:4).eq.'86')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'CO' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. &
              grid(4:4).lt.'8').or. grid(3:4).eq.'08' .or. grid(3:4).eq.'18' .or. grid(3:4).eq.'30'.or.grid(3:4).eq.'31'.or. &
              grid(3:4).eq.'40')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EP' .and. ((grid(4:4).eq.'0' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':') .or. &
              (grid(4:4).eq.'1' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'9') .or. &
              (grid(4:4).eq.'2' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'8'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'CN' .and. (grid(3:4).eq.'68' .or. grid(3:4).eq.'98')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'4' .and. &
              grid(4:4).lt.'8') .or. grid(3:4).eq.'04')) then; lgvalid=.false.
      endif
endif

! Portugal, Madeira, Azores CQ..CU
! CE3TSK 2026-10-12: the test ended at CT ("less than U"), so a CU call - the Azores - matched no rule and
! NO grid was accepted for it: every weak CQ of every Azores station was deleted, and a directed one
! ("CQ TEST CU3...") at any strength. The Azores squares were in the rule all along.
    else if(callsign(2:2).gt.'P' .and. callsign(2:2).lt.'V') then
      if((grid(1:2).eq.'IN' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
! CE3TSK 2026-10-13: Madeira's four squares (IM12, IM13, IM10, IM20) stood INSIDE the test for the mainland's
! columns ("the third character greater than 4") and could never be reached: no square was accepted for a
! Madeira station.
         (grid(1:2).eq.'IM' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'12' .or. grid(3:4).eq.'13' .or. grid(3:4).eq.'10' .or. grid(3:4).eq.'20')) .or. &
         (grid(1:2).eq.'HM' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'6'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'76'))) lgvalid=.true.

! Uruguay CV..CX
    else if(callsign(2:2).gt.'U' .and. callsign(2:2).lt.'Y') then
! CE3TSK 2026-10-13: Antarctica - Uruguay's stations: Artigas GC07, Elichiribehety GC16
      if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC16') lgvalid=.true.
      if(grid(1:2).eq.'GF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') &
        lgvalid=.true.

! Bolivia CP
    else if(callsign(1:2).eq.'CP') then
! CE3TSK 2026-10-13: land that this rule refused - Bolivia
      if(grid(1:4).eq.'FG99') lland=.true.
      if((grid(1:2).eq.'FH' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'FG' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'GH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4').or. &
         (grid(1:2).eq.'FI' .and. (grid(3:4).eq.'60' .or. grid(3:4).eq.'70')) .or. &
         (grid(1:2).eq.'GG' .and. (grid(3:4).eq.'09' .or. grid(3:4).eq.'19'))) lgvalid=.true.

! Nauru C2, Andorra C3
    else if((callsign(1:2).eq.'C2' .and. grid(1:4).eq.'RI39') .or. (callsign(1:2).eq.'C3' .and. grid(1:4).eq.'JN02')) then
      lgvalid=.true.

! Morocco 5C..5G,CN
    else if(callsign(1:2).eq.'CN') then
! CE3TSK 2026-10-13: land that this rule refused - Morocco (the south - Dakhla is IL23)
      if(grid(1:4).eq.'IL11' .or. grid(1:4).eq.'IL12' .or. grid(1:4).eq.'IL13' .or. grid(1:4).eq.'IL21' .or. &
         grid(1:4).eq.'IL22' .or. grid(1:4).eq.'IL23' .or. grid(1:4).eq.'IL26' .or. grid(1:4).eq.'IL33' .or. &
         grid(1:4).eq.'IL36' .or. grid(1:4).eq.'IL46' .or. grid(1:4).eq.'IL56') lland=.true.
      if((grid(1:2).eq.'IM' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'6').or. &
         (grid(1:2).eq.'IL' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.':').or. &
         grid(3:4).eq.'69' .or. grid(3:4).eq.'79'))) lgvalid=.true.

if(lgvalid) then
      if(grid(1:3).eq.'IM9' .and. (grid(4:4).eq.'0' .or. grid(4:4).eq.'1' .or. grid(4:4).eq.'5')) then; lgvalid=.false.
      else if(grid(1:2).eq.'IL' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'0' .and. &
              grid(4:4).lt.'6') .or. (grid(4:4).eq.'1' .or. grid(4:4).eq.'3' .or. grid(4:4).eq.':') .or. &
             (grid(4:4).eq.'2' .or. grid(4:4).eq.'6' .or. grid(4:4).eq.':') .or. &
             grid(3:4).eq.'31' .or. grid(3:4).eq.'32' .or. grid(3:4).eq.'39')) then; lgvalid=.false.
      endif
endif

! Cyprus 5B,C4,H2,P3
    else if(callsign(1:2).eq.'C4') then
      if((grid(1:3).eq.'KM6' .and. (grid(4:4).eq.'4' .or. grid(4:4).eq.'5')) .or. &
         (grid(1:3).eq.'KM7' .and. (grid(4:4).eq.'4' .or. grid(4:4).eq.'5'))) lgvalid=.true.

 ! The Gambia C5
    else if(callsign(1:2).eq.'C5' .and. grid(1:2).eq.'IK' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'4' .and. &
            grid(4:4).eq.'3') then
      lgvalid=.true.

 ! Bahamas C6
! CE3TSK 2026-10-13: land that this rule refused - Great Inagua, FL30 and FL31
    else if(callsign(1:2).eq.'C6' .and. ((grid(1:2).eq.'FL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. &
            grid(4:4).gt.'1' .and. grid(4:4).lt.'7') .or. grid(1:4).eq.'FL30' .or. grid(1:4).eq.'FL31')) then
      lgvalid=.true.

 ! Mozambique C8,C9
    else if(callsign(1:2).eq.'C8' .or. callsign(1:2).eq.'C9') then
! CE3TSK 2026-10-13: land that this rule refused - Mozambique (the coast east of 40 degrees - Pemba is LH07)
      if(grid(1:4).eq.'LH03' .or. grid(1:4).eq.'LH04' .or. grid(1:4).eq.'LH05' .or. grid(1:4).eq.'LH06' .or. &
         grid(1:4).eq.'LH07' .or. grid(1:4).eq.'LH08' .or. grid(1:4).eq.'LH09') lland=.true.
      if((grid(1:2).eq.'KG' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KH' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'LH' .and. grid(3:3).eq.'1' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':')) lgvalid=.true.

! not valid callsigns 'C'
    else if(callsign(2:2).eq.'0' .or. callsign(2:2).eq.'1' .or. callsign(2:2).eq.'7') then
      lwrongcall=.true.; return

    endif ! 'C'

  else if(callsign(1:1).eq.'D') then

! Fed. Rep. of Germany DA..DR, Y2..Y9
    if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'S') then
! CE3TSK 2026-10-13: Antarctica - Germany's stations: Neumayer III IB59, Kohnen JB04 (200 m from JB05 and 2 km from
! IB94, IB95: all four), Gondwana RB25, GARS O'Higgins GC16, Dallmann GC07
      if(grid(1:4).eq.'IB59' .or. grid(1:4).eq.'JB04' .or. grid(1:4).eq.'JB05' .or. grid(1:4).eq.'IB94' .or. &
         grid(1:4).eq.'IB95' .or. grid(1:4).eq.'RB25' .or. grid(1:4).eq.'GC16' .or. &
         grid(1:4).eq.'GC07') lgvalid=.true.
      if((grid(1:2).eq.'JO' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
          grid(3:4).eq.'45')) .or. &
         (grid(1:2).eq.'JN' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'IB59') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'JO' .and. ((grid(3:3).lt.'2' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5') .or. &
         grid(3:4).eq.'35')) lgvalid=.false.
endif

! Republic of Korea 6K,6L,6M,6N,D7,D8,D9,DS,DT,HL
    else if(callsign(1:2).eq.'DS' .or. callsign(1:2).eq.'DT' .or. &
            (callsign(2:2).gt.'6' .and. callsign(2:2).lt.':')) then
! CE3TSK 2026-10-13: land that this rule refused - Korea
      if(grid(1:4).eq.'PM27') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Korea's stations: King Sejong GC07, Jang Bogo RB25
      if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'RB25') lgvalid=.true.
      if((grid(1:3).eq.'PM3' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'9') .or. &
         (grid(1:3).eq.'PM4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9') .or. &
         grid(1:4).eq.'PM24' .or. grid(1:4).eq.'PM57' .or. grid(1:4).eq.'GC07') lgvalid=.true. ! DT8A in GC07OS South Shetland Islands

! Philippines 4D,4E,4F,4G,4H,4I,DU,DV,DW,DX,DY,DZ
      else if(callsign(2:2).gt.'T' .and. callsign(2:2).lt.'[') then
        if((grid(1:2).eq.'PK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4') .or. &
         (grid(1:2).eq.'PJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'04')) .or. &
         (grid(1:2).eq.'OJ' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'OK' .and. grid(3:3).eq.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7') .or. &
         (grid(1:2).eq.'PL' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01' .or. grid(3:4).eq.'10')) .or. &
         grid(1:4).eq.'OK70' .or. grid(1:4).eq.'OK71') lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'PK' .and. ((grid(3:3).eq.'3' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. &
           (grid(3:3).eq.'2' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':'))) then; lgvalid=.false.
        else if(grid(1:2).eq.'PJ' .and. (grid(3:4).eq.'08' .or. grid(3:4).eq.'15')) then; lgvalid=.false.
        else if(grid(1:2).eq.'OJ' .and. (grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) then; lgvalid=.false.
        endif
endif

! D0,D1 prefixes in transmitted message, mapping is required to prevent treating it as false decodes
    else if(callsign(1:2).eq.'D0' .or. callsign(1:2).eq.'D1') then
      if(grid(1:3).eq.'KN8' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':' .or. &
         grid(1:3).eq.'KN9' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':' .or. &
         grid(1:4).eq.'LN08' .or. grid(1:4).eq.'LN09') lgvalid=.true.

! Angola D2,D3
    else if(callsign(1:2).eq.'D2' .or. callsign(1:2).eq.'D3') then
! CE3TSK 2026-10-13: land that this rule refused - Angola (the north of Cabinda with them)
      if(grid(1:4).eq.'JI65' .or. grid(1:4).eq.'KH27' .or. grid(1:4).eq.'KI03' .or. grid(1:4).eq.'KI10') lland=.true.
      if((grid(1:2).eq.'JI' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'JH' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KH' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'01' .or. grid(3:4).eq.'28')) .or. &
         (grid(1:3).eq.'KI0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
         (grid(1:3).eq.'JH5' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5')) lgvalid=.true.

! Cape Verde D4
    else if(callsign(1:2).eq.'D4' .and. ((grid(1:3).eq.'HK7' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
      (grid(1:3).eq.'HK8' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'7'))) then
      lgvalid=.true.

! Liberia 5L,5M,6Z,A8,D5,EL
    else if(callsign(1:2).eq.'D5') then
      if(grid(1:2).eq.'IJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9' .and. &
         grid(3:4).ne.'67' .and. grid(3:4).ne.'67') lgvalid=.true.

! Comoros D6
    else if(callsign(1:2).eq.'D6' .and. grid(1:2).eq.'LH' .and. (grid(3:4).eq.'17' .or. grid(3:4).eq.'18' .or. &
      grid(3:4).eq.'27')) then
      lgvalid=.true.

    endif ! 'D'

  else if(callsign(1:1).eq.'E') then

! Spain AM..AO,EA..EH
    if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'I') then
! CE3TSK 2026-10-13: Antarctica - Spain's stations Juan Carlos I and Gabriel de Castilla, both FC97 (and FC96, 3 km
! south of the second)
      if(grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC96') lgvalid=.true.
      if((grid(1:2).eq.'IN' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4'.and. &
          grid(3:4).ne.'50') .or. &
         (grid(1:2).eq.'IM' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'12' .or. grid(3:4).eq.'13' .or. grid(3:4).eq.'75' .or. grid(3:4).eq.'85')) .or. &
! CE3TSK 2026-10-13: JN02 and JN12 were missing - the Pyrenees side of Aragon and Catalonia (Figueres is JN12)
         (grid(1:2).eq.'JN' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'3').or. &
          grid(3:4).eq.'20')) .or. &
         (grid(1:2).eq.'JM' .and. (grid(3:4).eq.'08' .or. grid(3:4).eq.'09' .or.grid(3:4).eq.'19'.or.grid(3:4).eq.'29')).or. &
         (grid(1:2).eq.'IL' .and. ((grid(4:4).eq.'7' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3') .or. &
         (grid(4:4).eq.'8' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4') .or. grid(3:4).eq.'39'))) lgvalid=.true.

! Ukraine EM..EO,UR..UZ
    else if(callsign(2:2).gt.'L' .and. callsign(2:2).lt.'P') then
! CE3TSK 2026-10-13: land that this rule refused - Ukraine
      if(grid(1:4).eq.'KO72' .or. grid(1:4).eq.'LN08' .or. grid(1:4).eq.'LN09') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Ukraine's station Vernadsky FC74
      if(grid(1:4).eq.'FC74') lgvalid=.true.
      if((grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
          grid(3:4).eq.'52' .or. grid(3:4).eq.'62')) .or. &
         (grid(1:2).eq.'KN' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':')) &
        lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'KO' .and. (grid(3:4).eq.'81' .or. grid(3:4).eq.'91')) then; lgvalid=.false.
      else if(grid(1:2).eq.'KN' .and. ((grid(3:3).gt.'0'.and.grid(3:3).lt.'4'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.'7').or. &
          grid(3:4).eq.'95' .or. grid(3:4).eq.'96')) then; lgvalid=.false.
      endif
endif

! Ireland EI,EJ
    else if(callsign(1:2).eq.'EI' .or. callsign(1:2).eq.'EJ') then
      if(grid(1:2).eq.'IO' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'6') &
        lgvalid=.true.

! Thailand E2,HS
    else if(callsign(1:2).eq.'E2') then
      if((grid(1:2).eq.'OK' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.'9').or. &
          grid(3:4).eq.'09')) .or. &
       (grid(1:2).eq.'NK' .and. ((grid(3:3).gt.'8' .and. grid(3:3).lt.':') .or. grid(3:4).eq.'87' .or. grid(3:4).eq.'88'.or. &
        grid(3:4).eq.'89')) .or. &
       (grid(1:2).eq.'NJ' .and. ((grid(3:3).eq.'9' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
        grid(3:4).eq.'88' .or. grid(3:4).eq.'89')) .or. &
       (grid(1:2).eq.'OJ' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or.grid(3:4).eq.'16')).or. &
       grid(1:4).eq.'NL90' .or. grid(1:4).eq.'OL00') lgvalid=.true.

! Eritrea E3
    else if(callsign(1:2).eq.'E3') then
      if((grid(1:2).eq.'KK' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8').or. &
         (grid(1:2).eq.'LK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'7')) &
        lgvalid=.true.

! Palestine E4
    else if(callsign(1:2).eq.'E4' .and. (grid(1:4).eq.'KM71' .or. grid(1:4).eq.'KM72')) then
      lgvalid=.true.

! North Cook and South Cook Islands E5
    else if(callsign(1:2).eq.'E5') then
      if((grid(1:2).eq.'BH' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01' .or. grid(3:4).eq.'10')) .or. &
         (grid(1:2).eq.'BG' .and. (grid(3:4).eq.'08' .or. grid(3:4).eq.'09'.or.grid(3:4).eq.'18'.or.grid(3:4).eq.'19')) .or. &
         (grid(1:2).eq.'AH' .and. (grid(3:4).eq.'78' .or. grid(3:4).eq.'79'.or.grid(3:4).eq.'81'.or.grid(3:4).eq.'86' .or. &
          grid(3:4).eq.'99')) .or. &
         (grid(1:2).eq.'BI' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01'.or.grid(3:4).eq.'10'.or.grid(3:4).eq.'11'))) &
        lgvalid=.true.

! Niue E6
    else if(callsign(1:2).eq.'E6') then
      if(grid(1:4).eq.'AH50' .or. grid(1:4).eq.'AH51') lgvalid=.true.

! Bosnia-Herzegovina E7
    else if(callsign(1:2).eq.'E7') then
      if(grid(1:2).eq.'JN' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'6' .and. &
         grid(3:4).ne.'72' .and. grid(3:4).ne.'73') lgvalid=.true.

! Belarus EU,EV,EW
    else if(callsign(2:2).gt.'T' .and. callsign(2:2).lt.'X') then
! CE3TSK 2026-10-13: Antarctica - Belarus' station Vechernyaya LC32
      if(grid(1:4).eq.'LC32') lgvalid=.true.
      if(grid(1:2).eq.'KO' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'7') &
        lgvalid=.true.

! Estonia ES
    else if(callsign(1:2).eq.'ES') then
      if((grid(1:2).eq.'KO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'KO49') lgvalid=.true.

! Moldova ER
    else if(callsign(1:2).eq.'ER') then
      if((grid(1:2).eq.'KN' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9').or. &
         grid(1:4).eq.'KN56') lgvalid=.true.

! Armenia EK
    else if(callsign(1:2).eq.'EK') then
      if((grid(1:2).eq.'LN' .and. (grid(3:4).eq.'10' .or. grid(3:4).eq.'11' .or. grid(3:4).eq.'20' .or. &
          grid(3:4).eq.'21')) .or. &
         (grid(1:2).eq.'LM' .and. (grid(3:4).eq.'29' .or. grid(3:4).eq.'39' .or. grid(3:4).eq.'38'))) lgvalid=.true.

! Kyrgyzstan EX
    else if(callsign(1:2).eq.'EX') then
! CE3TSK 2026-10-13: land that this rule refused - Kyrgyzstan
      if(grid(1:4).eq.'MN40' .or. grid(1:4).eq.'NN02') lland=.true.
      if((grid(1:2).eq.'MN' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4').or. &
         (grid(1:2).eq.'MM' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).eq.'9')) lgvalid=.true.

! Tajikistan EY
    else if(callsign(1:2).eq.'EY') then
! CE3TSK 2026-10-13: land that this rule refused - Tajikistan
      if(grid(1:4).eq.'MN51') lland=.true.
      if((grid(1:2).eq.'MM' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'MN' .and. (grid(3:4).eq.'40' .or. grid(3:4).eq.'50'))) lgvalid=.true.

! Turkmenistan EZ
    else if(callsign(1:2).eq.'EZ') then
! CE3TSK 2026-10-13: land that this rule refused - Turkmenistan
      if(grid(1:4).eq.'MN02') lland=.true.
      if((grid(1:2).eq.'LM' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'LN' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
         (grid(1:2).eq.'MM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'MN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2')) &
        lgvalid=.true.

! Liberia 5L,5M,6Z,A8,D5,EL
    else if(callsign(1:2).eq.'EL') then
      if(grid(1:2).eq.'IJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9' .and. &
         grid(3:4).ne.'67' .and. grid(3:4).ne.'67') lgvalid=.true.

! Iran 9B..9D,EP,EQ
    else if(callsign(1:2).eq.'EP' .or. callsign(1:2).eq.'EQ') then
! CE3TSK 2026-10-13: land that this rule refused - Iran
      if(grid(1:4).eq.'LM23' .or. grid(1:4).eq.'MM07') lland=.true.
      if((grid(1:2).eq.'LM' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'LL' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'49' .or. grid(3:4).eq.'75' .or. grid(3:4).eq.'85' .or. grid(3:4).eq.'95')) .or. &
         (grid(1:2).eq.'ML' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':' .and. &
          grid(3:4).ne.'15' .and. grid(3:4).ne.'19') .or. &
         (grid(1:2).eq.'MM' .and. grid(3:3).eq.'0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7')) &
        lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'LM' .and. ((grid(3:3).eq.'2' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'4') .or. &
           (grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7'.and.grid(4:4).lt.':' .and. &
            grid(3:4).ne.'78' .and. grid(3:4).ne.'88'))) lgvalid=.false.
endif

! Ethiopia 9E,9F,ET
    else if(callsign(1:2).eq.'ET') then
! CE3TSK 2026-10-13: land that this rule refused - Ethiopia
      if(grid(1:4).eq.'LJ03') lland=.true.
      if((grid(1:2).eq.'LJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'LK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'KJ' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KK' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5')) &
        lgvalid=.true.

! not valid callsigns E0 E1 E8 E9
    else if(callsign(2:2).eq.'0' .or. callsign(2:2).eq.'1' .or. callsign(2:2).eq.'8' .or. callsign(2:2).eq.'9') then
      lwrongcall=.true.; return

    endif ! 'E'

  else if(callsign(1:1).eq.'F') then

! France F,FA..FF,FI,FL,FN,FQ,FU,FV,FX,FZ,HW..HY,TH,TM,TP,TQ,TV
    if((callsign(2:2).gt.'/' .and. callsign(2:2).lt.':') .or. (callsign(2:2).gt.'@' .and. callsign(2:2).lt.'G') .or. &
       callsign(2:2).eq.'I' .or. callsign(2:2).eq.'L' .or. callsign(2:2).eq.'N' .or. callsign(2:2).eq.'Q' .or. &
       callsign(2:2).eq.'U' .or. callsign(2:2).eq.'V' .or. callsign(2:2).eq.'X' .or. callsign(2:2).eq.'Z') then
! CE3TSK 2026-10-13: land that this rule refused - France (Corsica)
      if(grid(1:4).eq.'JN32' .or. grid(1:4).eq.'JN41' .or. grid(1:4).eq.'JN42' .or. grid(1:4).eq.'JN43') lland=.true.
! CE3TSK 2026-10-13: Antarctica - France's stations for every call of the rule: Dumont d'Urville QC03 and Cap Prud'homme
! PC93, Concordia PB14, D85 PB79
      if(grid(1:4).eq.'QC03' .or. grid(1:4).eq.'PC93' .or. grid(1:4).eq.'PB14' .or. &
         grid(1:4).eq.'PB79') lgvalid=.true.
      if((grid(1:2).eq.'IN' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
         (grid(1:3).eq.'IN7' .and. (grid(4:4).eq.'7' .or. grid(4:4).eq.'8')) .or. &
         (grid(1:2).eq.'JN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
         (grid(1:3).eq.'JN4' .and. (grid(4:4).eq.'8' .or. grid(4:4).eq.'9')) .or. &
         grid(1:4).eq.'JN02'  .or. grid(1:4).eq.'JN12' .or. grid(1:4).eq.'IN92' .or. &
         (grid(1:2).eq.'JO' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'10' .or. grid(3:4).eq.'20' .or. grid(3:4).eq.'11'))) &
        lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'IN' .and. (grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) lgvalid=.false.
endif

! Martinique FM,TO
    else if(callsign(1:2).eq.'FM' .and. grid(1:4).eq.'FK94') then
      lgvalid=.true.

! Guadeloupe FG,TO
    else if(callsign(1:2).eq.'FG') then
      if(grid(1:2).eq.'FK' .and. (grid(3:4).eq.'96' .or. grid(3:4).eq.'95')) lgvalid=.true.

! St. Pierre & Miquelon FP
    else if(callsign(1:2).eq.'FP') then
      if(grid(1:2).eq.'GN' .and. (grid(3:4).eq.'16' .or. grid(3:4).eq.'17')) lgvalid=.true.

! St. Martin FS,TO
    else if(callsign(1:2).eq.'FS' .and. grid(1:4).eq.'FK88') then
       lgvalid=.true.

! St. Barthelemy FJ,TO
    else if(callsign(1:2).eq.'FJ' .and. grid(1:4).eq.'FK87') then
       lgvalid=.true.

! French Guiana FY
    else if(callsign(1:2).eq.'FY') then
      if(grid(1:2).eq.'GJ' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'6') &
       lgvalid=.true.

! Reunion Island FR,TO
    else if(callsign(1:2).eq.'FR') then
      if(grid(1:2).eq.'LG' .and. (grid(3:4).eq.'79' .or. grid(3:4).eq.'78')) lgvalid=.true.

! New Caledonia FK
    else if(callsign(1:2).eq.'FK') then
      if((grid(1:2).eq.'RG' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'RH' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
         (grid(1:2).eq.'RG' .and. (grid(3:4).eq.'19' .or. grid(3:4).eq.'57' .or. grid(3:4).eq.'67')) .or. &
! CE3TSK 2026-10-13: the Chesterfield Islands' squares: their own rule, next, asks for FK again and is never reached
         (grid(1:2).eq.'QH' .and. (grid(3:4).eq.'90' .or. grid(3:4).eq.'91')) .or. grid(1:4).eq.'QG98') lgvalid=.true.

! Chesterfield Islands FK,TX
    else if(callsign(1:2).eq.'FK') then
      if((grid(1:2).eq.'QH' .and. (grid(3:4).eq.'90' .or. grid(3:4).eq.'91')) .or. grid(1:4).eq.'QG98') lgvalid=.true.

! Mayotte FH
    else if(callsign(1:2).eq.'FH') then
      if(grid(1:2).eq.'LH' .and. (grid(3:4).eq.'27' .or. grid(3:4).eq.'26')) lgvalid=.true.

! Glorioso, Juan de Nova, Europa, Tromelin, Crozet, Kerguelen, Amsterdam & St. Paul Islands   FT
    else if(callsign(1:2).eq.'FT') then
! CE3TSK 2026-10-13: Antarctica - France's stations for every call of the rule: Dumont d'Urville QC03 and Cap Prud'homme
! PC93, Concordia PB14, D85 PB79
      if(grid(1:4).eq.'QC03' .or. grid(1:4).eq.'PC93' .or. grid(1:4).eq.'PB14' .or. &
         grid(1:4).eq.'PB79') lgvalid=.true.
      if((grid(1:2).eq.'LH' .and. (grid(3:4).eq.'38' .or. grid(3:4).eq.'12' .or. grid(3:4).eq.'74')) .or. &
         (grid(1:2).eq.'LE' .and. (grid(3:4).eq.'53' .or. grid(3:4).eq.'54' .or. grid(3:4).eq.'63')) .or. &
         (grid(1:2).eq.'ME' .and. (grid(3:4).eq.'40' .or. grid(3:4).eq.'41' .or. grid(3:4).eq.'50')) .or. &
         (grid(1:2).eq.'MF' .and. (grid(3:4).eq.'81' .or. grid(3:4).eq.'82')) .or. &
! CE3TSK 2026-10-13: and Dumont d'Urville, Antarctica (FT#Y): the base lies on the 140th meridian, so both squares
         (callsign(4:4).eq.'Y' .and. (grid(1:4).eq.'QC03' .or. grid(1:4).eq.'PC93')) .or. &
         grid(1:4).eq.'LG07' .or. grid(1:4).eq.'KG98') lgvalid=.true.

! French Polynesia, Austral, Marquesas, Clipperton Islands FO,TX
    else if(callsign(1:2).eq.'FO') then
! CE3TSK 2026-10-13: land that this rule refused - French Polynesia
      if(grid(1:4).eq.'CG08' .or. grid(1:4).eq.'CG09' .or. grid(1:4).eq.'CG17' .or. grid(1:4).eq.'CG18' .or. &
         grid(1:4).eq.'CG26' .or. grid(1:4).eq.'CG28' .or. grid(1:4).eq.'CH00' .or. grid(1:4).eq.'CH01' .or. &
         grid(1:4).eq.'CH02' .or. grid(1:4).eq.'CH11') lland=.true.
      if((grid(1:2).eq.'BH' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'6').or. &
         (grid(1:2).eq.'BG' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':').or. &
         (grid(1:3).eq.'BI9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
         (grid(1:2).eq.'CI' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01')) .or. &
         grid(1:4).eq.'CH09' .or. grid(1:4).eq.'DK50') lgvalid=.true.

! Wallis & Futuna Islands FW,TW
    else if(callsign(1:2).eq.'FW') then
      if(grid(1:2).eq.'AH' .and. (grid(3:4).eq.'05' .or. grid(3:4).eq.'16')) lgvalid=.true.

    endif ! 'F'

! UK  GB,GX / 2E,G,M / 2D,GD,GT,MD,MT / 2I,GI,GN,MI,MN / 2J,GH,GJ,MH,MJ / 2A,2M,GM,GS,MA,MM,MS / 2U,GP,GU,MP,MU / 2W,GC,GW,MC,MW
  else if(callsign(1:1).eq.'G') then
! CE3TSK 2026-10-13: land that this rule refused - Scotland (Foula, Shetland)
    if(grid(1:4).eq.'IP80') lland=.true.
! CE3TSK 2026-10-13: Antarctica - the United Kingdom's stations: Rothera FC52, Port Lockroy FC85, Halley HB74 (and HB64,
! where it stood until 2017 and towards which its ice shelf carries it), Signy GC79, Fossil Bluff FB58, Sky Blu FB45
    if(grid(1:4).eq.'FC52' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'HB74' .or. grid(1:4).eq.'HB64' .or. &
       grid(1:4).eq.'GC79' .or. grid(1:4).eq.'FB58' .or. grid(1:4).eq.'FB45') lgvalid=.true.
    if((grid(1:2).eq.'IO' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':') .or. &
       (grid(1:3).eq.'JO0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
! CE3TSK 2026-10-13: IN79 was missing - the Lizard, the south-west end of Cornwall
       grid(1:4).eq.'IP90' .or. grid(1:4).eq.'IN69' .or. grid(1:4).eq.'IN79' .or. grid(1:4).eq.'IN89' .or. &
       grid(1:4).eq.'IO37') lgvalid=.true.

if(lgvalid .and. grid(1:2).eq.'IO') then
    if((grid(1:3).eq.'IO5' .and. grid(4:4).ne.'4' .and. grid(4:4).ne.'7') .or. &
       (grid(1:3).eq.'IO6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
       grid(3:4).eq.'96' .or. grid(3:4).eq.'98') lgvalid=.false.
endif

  else if(callsign(1:1).eq.'H') then

! Hungary HA,HG
    if(callsign(1:2).eq.'HA' .or. callsign(1:2).eq.'HG') then
      if((grid(1:2).eq.'JN' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9').or. &
       (grid(1:2).eq.'KN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.'9' .and. &
        grid(3:4).ne.'16')) lgvalid=.true.

! Switzerland, Liechtenstein HB,HE
    else if(callsign(1:2).eq.'HB' .or. callsign(1:2).eq.'HE') then
! CE3TSK 2026-10-13: land that this rule refused - Switzerland (the canton of Geneva west of 6 degrees)
      if(grid(1:4).eq.'JN26') lland=.true.
      if(((grid(1:3).eq.'JN3' .or. grid(1:3).eq.'JN4') .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'8') .or. &
         grid(1:4).eq.'JN56') lgvalid=.true.

! Thailand E2,HS
    else if(callsign(1:2).eq.'HS') then
      if((grid(1:2).eq.'OK' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.'9').or. &
          grid(3:4).eq.'09')) .or. &
       (grid(1:2).eq.'NK' .and. ((grid(3:3).gt.'8' .and. grid(3:3).lt.':') .or. grid(3:4).eq.'87' .or. grid(3:4).eq.'88'.or. &
        grid(3:4).eq.'89')) .or. &
       (grid(1:2).eq.'NJ' .and. ((grid(3:3).eq.'9' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
        grid(3:4).eq.'88' .or. grid(3:4).eq.'89')) .or. &
       (grid(1:2).eq.'OJ' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or.grid(3:4).eq.'16')).or. &
       grid(1:4).eq.'NL90' .or. grid(1:4).eq.'OL00') lgvalid=.true.

! Cyprus 5B,C4,H2,P3
    else if(callsign(1:2).eq.'H2') then
      if((grid(1:3).eq.'KM6' .and. (grid(4:4).eq.'4' .or. grid(4:4).eq.'5')) .or. &
         (grid(1:3).eq.'KM7' .and. (grid(4:4).eq.'4' .or. grid(4:4).eq.'5'))) lgvalid=.true.

! Poland 3Z,HF,SN..SR
    else if(callsign(1:2).eq.'HF') then
! CE3TSK 2026-10-13: Antarctica - Poland's stations: Arctowski GC07, Dobrowolski OC03
    if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'OC03') lgvalid=.true.
    if((grid(1:2).eq.'JO' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
       (grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and.grid(4:4).lt.'5').or. &
        grid(3:4).eq.'20')) .or. &
       grid(1:4).eq.'KN09' .or. grid(1:4).eq.'KN19' .or. grid(1:4).eq.'KO20' .or. grid(1:4).eq.'JN99' .or. &
       grid(1:4).eq.'JN89') lgvalid=.true.

! Nicaragua H6,H7,HT,YN
    else if(callsign(1:2).eq.'H6' .or. callsign(1:2).eq.'H7' .or. callsign(1:2).eq.'HT') then
! CE3TSK 2026-10-13: land that this rule refused - Nicaragua
      if(grid(1:4).eq.'EK85') lland=.true.
      if(grid(1:2).eq.'EK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') &
        lgvalid=.true.

! Ecuador HC HD
    else if(callsign(1:2).eq.'HC' .or. callsign(1:2).eq.'HD') then
! CE3TSK 2026-10-13: land that this rule refused - Ecuador (Darwin and Wolf Islands with them); the Galapagos Islands
! (Darwin, Wolf and Genovesa Islands)
      if(grid(1:4).eq.'EJ31' .or. grid(1:4).eq.'EJ41' .or. grid(1:4).eq.'EJ50' .or. grid(1:4).eq.'FI04' .or. &
         grid(1:4).eq.'FI16') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Ecuador's station Maldonado GC07
      if(grid(1:4).eq.'GC07') lgvalid=.true.
      if((grid(1:2).eq.'FI' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'6'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'05' .or. grid(3:4).eq.'06')) .or. &
         (grid(1:2).eq.'FJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3'.and.grid(4:4).eq.'0').or.grid(3:4).eq.'01')).or. &
         (grid(1:2).eq.'EI' .and. ((grid(3:3).eq.'9' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. &
          grid(3:4).eq.'49' .or. grid(3:4).eq.'59' .or. grid(3:4).eq.'48' .or. grid(3:4).eq.'58')) .or. &
         (grid(1:2).eq.'EJ' .and. (grid(3:4).eq.'90' .or. grid(3:4).eq.'40'))) lgvalid=.true.

! Haiti 4V,HH
    else if(callsign(1:2).eq.'HH') then
! CE3TSK 2026-10-13: land that this rule refused - Haiti
      if(grid(1:4).eq.'FK28') lland=.true.
      if((grid(1:2).eq.'FK' .and. (grid(3:4).eq.'38' .or. grid(3:4).eq.'39' .or. grid(3:4).eq.'48' .or. &
          grid(3:4).eq.'49')) .or. grid(1:4).eq.'FL30') lgvalid=.true.

! Dominican Republic HI
    else if(callsign(1:2).eq.'HI') then
! CE3TSK 2026-10-13: land that this rule refused - the Dominican Republic
      if(grid(1:4).eq.'FK38') lland=.true.
      if(grid(1:2).eq.'FK' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'58' .or. &
         grid(3:4).eq.'59' .or. grid(3:4).eq.'47')) lgvalid=.true.

! Republic of Korea 6K,6L,6M,6N,D7,D8,D9,DS,DT,HL
    else if(callsign(1:2).eq.'HL') then
! CE3TSK 2026-10-13: land that this rule refused - Korea
      if(grid(1:4).eq.'PM27') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Korea's stations: King Sejong GC07, Jang Bogo RB25
      if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'RB25') lgvalid=.true.
      if((grid(1:3).eq.'PM3' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'9') .or. &
         (grid(1:3).eq.'PM4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9') .or. &
         grid(1:4).eq.'PM24' .or. grid(1:4).eq.'PM57' .or. grid(1:4).eq.'GC07') lgvalid=.true. ! DT8A in GC07OS South Shetland Islands

! Colombia 5J,5K,HJ,HK
    else if(callsign(1:2).eq.'HK' .or. callsign(1:2).eq.'HJ') then
! CE3TSK 2026-10-13: land that this rule refused - Colombia
      if(grid(1:4).eq.'EK93' .or. grid(1:4).eq.'FJ00') lland=.true.
      if((grid(1:2).eq.'FJ' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'7') .or. &
          grid(3:4).eq.'01' .or. grid(3:4).eq.'02' .or. grid(3:4).eq.'03')) .or. &
        (grid(1:2).eq.'FK' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3' .and. &
         grid(3:4).ne.'22') .or. &
        (grid(1:2).eq.'FI' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'6' .and.grid(4:4).lt.':').or. &
         grid(3:4).eq.'45' .or. grid(3:4).eq.'55' .or. grid(3:4).eq.'46' .or. grid(3:4).eq.'56') .and.grid(3:4).ne.'27').or. &
        (grid(1:2).eq.'EJ' .and. (grid(3:4).eq.'93' .or. grid(3:4).eq.'94')) .or. grid(1:4).eq.'EK92') lgvalid=.true.

! Saudi Arabia 7Z,8Z,HZ
      else if(callsign(1:2).eq.'HZ') then
! CE3TSK 2026-10-13: land that this rule refused - Saudi Arabia
        if(grid(1:4).eq.'LM11') lland=.true.
        if((grid(1:2).eq.'LL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8').or. &
        (grid(1:2).eq.'KL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':') .or. &
        (grid(1:2).eq.'LK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'5' .and.grid(4:4).lt.':').or. &
        (grid(1:2).eq.'LM' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01' .or. grid(3:4).eq.'10')) .or. &
        (grid(1:2).eq.'KM' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and.grid(4:4).lt.'2') .or. &
        grid(1:4).eq.'KM92') lgvalid=.true.

! Panama 3E,3F,H3,H8,H9,HO,HP
  else if(callsign(2:2).eq.'P' .or. callsign(2:2).eq.'3' .or. callsign(2:2).eq.'8' .or. &
           callsign(2:2).eq.'9' .or. callsign(2:2).eq.'O') then
! CE3TSK 2026-10-13: land that this rule refused - Panama
    if(grid(1:4).eq.'FJ07' .or. grid(1:4).eq.'FJ19') lland=.true.
    if((grid(1:2).eq.'FJ' .and. (grid(3:4).eq.'08' .or. grid(3:4).eq.'09' .or. grid(3:4).eq.'17' .or. &
        grid(3:4).eq.'18')) .or. &
       (grid(1:2).eq.'EJ' .and. (grid(3:4).eq.'88' .or. grid(3:4).eq.'89' .or. grid(3:4).eq.'97' .or. &
        grid(3:4).eq.'98' .or. grid(3:4).eq.'99'))) lgvalid=.true.

! Honduras HQ,HR
      else if(callsign(2:2).eq.'R' .or. callsign(2:2).eq.'Q') then
! CE3TSK 2026-10-13: land that this rule refused - Honduras
        if(grid(1:4).eq.'EK62' .or. grid(1:4).eq.'EK87') lland=.true.
        if(grid(1:2).eq.'EK' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'2' .and.grid(4:4).lt.'7'.and. &
           grid(3:4).ne.'56' .and. grid(3:4).ne.'83') lgvalid=.true.

! Iraq HN,YI
    else if(callsign(1:2).eq.'HN') then
      if((grid(1:2).eq.'LM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8').or. &
         (grid(1:2).eq.'KM' .and.(grid(3:4).eq.'92' .or. grid(3:4).eq.'93')) .or. &
         (grid(1:2).eq.'LL' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'5' .and. grid(4:4).eq.'9')) lgvalid=.true.

! El Salvador HU,YS
    else if(callsign(1:2).eq.'HU') then
      if(grid(1:2).eq.'EK' .and. (grid(3:4).eq.'53' .or. grid(3:4).eq.'54' .or. grid(3:4).eq.'63' .or. grid(3:4).eq.'43')) &
        lgvalid=.true.

! South Africa H5,S4,S8,V9,ZR,ZS,ZT,ZU
    else if(callsign(1:2).eq.'H5') then
! CE3TSK 2026-10-13: land that this rule refused - South Africa
      if(grid(1:4).eq.'JG92' .or. grid(1:4).eq.'JG93' .or. grid(1:4).eq.'JG94' .or. grid(1:4).eq.'JG95') lland=.true.
! CE3TSK 2026-10-13: Antarctica - South Africa's station SANAE IV IB88, for every call of the rule
      if(grid(1:4).eq.'IB88') lgvalid=.true.
      if((grid(1:2).eq.'KG' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8').or. &
          (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4'))) .or. &
        (grid(1:2).eq.'KF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. &
        (grid(1:2).eq.'JF' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':' .and. &
         grid(3:4).ne.'85') .or. &
        (grid(1:2).eq.'JG' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
        grid(1:4).eq.'KE83' .or. grid(1:4).eq.'KE93') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'KF' .and. ((grid(4:4).eq.'5' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'6') .or. &
         (grid(4:4).eq.'6' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'6') .or. grid(3:4).eq.'57')) lgvalid=.false.
endif

! France F,FA..FF,FI,FL,FN,FQ,FU,FV,FX,FZ,HW..HY,TH,TM,TP,TQ,TV
    else if(callsign(2:2).gt.'V' .and. callsign(2:2).lt.'Z') then
! CE3TSK 2026-10-13: land that this rule refused - France (Corsica)
      if(grid(1:4).eq.'JN32' .or. grid(1:4).eq.'JN41' .or. grid(1:4).eq.'JN42' .or. grid(1:4).eq.'JN43') lland=.true.
! CE3TSK 2026-10-13: Antarctica - France's stations for every call of the rule: Dumont d'Urville QC03 and Cap Prud'homme
! PC93, Concordia PB14, D85 PB79
      if(grid(1:4).eq.'QC03' .or. grid(1:4).eq.'PC93' .or. grid(1:4).eq.'PB14' .or. &
         grid(1:4).eq.'PB79') lgvalid=.true.
      if((grid(1:2).eq.'IN' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
         (grid(1:3).eq.'IN7' .and. (grid(4:4).eq.'7' .or. grid(4:4).eq.'8')) .or. &
         (grid(1:2).eq.'JN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
         (grid(1:3).eq.'JN4' .and. (grid(4:4).eq.'8' .or. grid(4:4).eq.'9')) .or. &
         grid(1:4).eq.'JN02'  .or. grid(1:4).eq.'JN12' .or. grid(1:4).eq.'IN92' .or. &
         (grid(1:2).eq.'JO' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'10' .or. grid(3:4).eq.'20' .or. grid(3:4).eq.'11'))) &
        lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'IN' .and. (grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) lgvalid=.false.
endif

! Vatican City HV
! CE3TSK 2026-10-13: there was no rule for these calls, so no square was accepted for them and every weak CQ of
! theirs was deleted. The squares are the ones the country touches (geodata/dxcc_grids.json, Natural Earth).
    else if(callsign(1:2).eq.'HV') then
      if(grid(1:4).eq.'JN61') lgvalid=.true.

! Solomon Islands, Temotu Province H4
    else if(callsign(1:2).eq.'H4') then
      if(grid(1:4).eq.'QH98' .or. &
         (grid(1:2).eq.'QI' .and. ((grid(3:3).eq.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
           (grid(3:3).eq.'8' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4') .or. &
           (grid(3:3).eq.'7' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'4'))) .or. &
         (grid(1:2).eq.'RH' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
           (grid(3:3).eq.'3' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
           (grid(3:3).gt.'0' .and. grid(3:3).lt.'3' .and. grid(4:4).eq.'9') .or. &
           grid(3:4).eq.'47')) .or. &
         (grid(1:2).eq.'RI' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
           grid(3:4).eq.'30' .or. grid(3:4).eq.'11'))) lgvalid=.true.

! not valid callsigns
    else if(callsign(1:2).eq.'H1') then
      lwrongcall=.true.; return

    endif ! 'H'

! Italy, Sardinia I
  else if(callsign(1:1).eq.'I') then
! CE3TSK 2026-10-13: Antarctica - Italy's stations: Mario Zucchelli RB25, Browning Pass RB15, Concordia PB14, Talos Dome
! QB97, Mid Point QB24, Cap Prud'homme PC93
    if(grid(1:4).eq.'RB25' .or. grid(1:4).eq.'RB15' .or. grid(1:4).eq.'PB14' .or. grid(1:4).eq.'QB97' .or. &
       grid(1:4).eq.'QB24' .or. grid(1:4).eq.'PC93') lgvalid=.true.
    if((grid(1:2).eq.'JN' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') .or. &
       grid(3:4).eq.'40' .or. grid(3:4).eq.'41')) .or. &
       (grid(1:2).eq.'JM' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
       grid(3:4).eq.'48' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'65' .or. grid(3:4).eq.'56'))) lgvalid=.true.

if(lgvalid) then
    if(grid(1:2).eq.'JM' .and. (grid(3:4).eq.'69' .or. grid(3:4).eq.'86' .or. grid(3:4).eq.'96' .or. grid(3:4).eq.'97' .or. &
       grid(3:4).eq.'98')) then; lgvalid=.false.
    else if(grid(1:2).eq.'JN' .and. &
       ((grid(3:3).eq.'7' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'8') .or. &
        (grid(3:3).eq.'8' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'8') .or. &
        (grid(3:3).eq.'9' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'8') .or. &
        grid(3:4).eq.'30' .or. grid(3:4).eq.'31' .or. grid(3:4).eq.'50')) then; lgvalid=.false.
    endif
endif

  else if(callsign(1:1).eq.'J') then

! Japan 7J..7N,8J..8N,JA,JE..JS
    if(callsign(1:2).eq.'JA' .or. (callsign(2:2).gt.'D' .and. callsign(2:2).lt.'T')) then
! CE3TSK 2026-10-13: land that this rule refused - Japan (the Ogasawara, Volcano and Izu Islands, Minami Torishima)
      if(grid(1:4).eq.'PL15' .or. grid(1:4).eq.'QL04' .or. grid(1:4).eq.'QL05' .or. grid(1:4).eq.'QL07' .or. &
         grid(1:4).eq.'QL16' .or. grid(1:4).eq.'QL17' .or. grid(1:4).eq.'QL64' .or. grid(1:4).eq.'QM00' .or. &
         grid(1:4).eq.'QM01') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Japan's stations: Syowa KC90 (and KC91, 400 m north of it), S17 LC00, Asuka KB28, Dome
! Fuji KB92, Mizuho LB29
      if(grid(1:4).eq.'KC90' .or. grid(1:4).eq.'KC91' .or. grid(1:4).eq.'LC00' .or. grid(1:4).eq.'KB28' .or. &
         grid(1:4).eq.'KB92' .or. grid(1:4).eq.'LB29') lgvalid=.true.
      if((grid(1:2).eq.'PM' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
         (grid(1:3).eq.'QM0' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. &
         (grid(1:2).eq.'QN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'6').or. &
         (grid(1:3).eq.'PN9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
         (grid(1:2).eq.'PL' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'QM19' .or. grid(1:4).eq.'KC90') lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'PM' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and.grid(4:4).gt.'6'.and.grid(4:4).lt.':').or. &
           (grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3'.and.grid(3:4).ne.'62').or. &
           grid(3:4).eq.'45' .or. grid(3:4).eq.'46' .or. grid(3:4).eq.'55' .or. grid(3:4).eq.'56' .or.grid(3:4).eq.'88' .or. &
           grid(3:4).eq.'89')) then; lgvalid=.false.
        else if(grid(1:2).eq.'PL'.and.((grid(3:3).gt.'0'.and.grid(3:3).lt.'3'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
           grid(3:4).eq.'38' .or. grid(3:4).eq.'39' .or. grid(3:4).eq.'57' .or. grid(3:4).eq.'59')) then; lgvalid=.false.
        else if(grid(1:2).eq.'QN'.and.((grid(3:3).eq.'2'.and.(grid(4:4).eq.'0'.or.grid(4:4).eq.'1'.or.grid(4:4).eq.'5')).or. &
           grid(3:4).eq.'10')) then; lgvalid=.false.
        endif
endif

! Greece J4,SV..SZ
    else if(callsign(1:2).eq.'J4') then
      if((grid(1:2).eq.'KM' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'46' .or. grid(3:4).eq.'24' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'14').and.grid(3:4).ne.'05').or. &
         grid(1:4).eq.'JM99' .or. &
         (grid(1:2).eq.'KN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. (grid(4:4).eq.'0' .or. grid(4:4).eq.'1'))) &
        lgvalid=.true.

! Djibouti J2
    else if(callsign(1:2).eq.'J2') then
! CE3TSK 2026-10-13: land that this rule refused - Djibouti
      if(grid(1:4).eq.'LK00' .or. grid(1:4).eq.'LK10') lland=.true.
      if(grid(1:2).eq.'LK' .and. (grid(3:4).eq.'01' .or. grid(3:4).eq.'11' .or. grid(3:4).eq.'12')) lgvalid=.true.

! Grenada J3
    else if(callsign(1:2).eq.'J3') then
      if(grid(1:2).eq.'FK' .and. (grid(3:4).eq.'92' .or. grid(3:4).eq.'91')) lgvalid=.true.

! Guinea-Bissau J5
    else if(callsign(1:2).eq.'J5') then
      if(grid(1:2).eq.'IK' .and. (grid(3:4).eq.'21' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'11' .or. &
         grid(3:4).eq.'12' .or. grid(3:4).eq.'31' .or. grid(3:4).eq.'32' .or. grid(3:4).eq.'20')) lgvalid=.true.

! St. Lucia J6
    else if(callsign(1:2).eq.'J6') then
      if(grid(1:2).eq.'FK' .and. (grid(3:4).eq.'93' .or. grid(3:4).eq.'94')) lgvalid=.true.

! Dominica J7
    else if(callsign(1:2).eq.'J7' .and. grid(1:4).eq.'FK95') then
      lgvalid=.true.

! St. Vincent J8
    else if(callsign(1:2).eq.'J8') then
      if(grid(1:2).eq.'FK' .and. (grid(3:4).eq.'93' .or. grid(3:4).eq.'92')) lgvalid=.true.

! Ogasawara and Minami Torishima JD1
    else if(callsign(1:3).eq.'JD1') then
      if(grid(1:2).eq.'QL' .and. (grid(3:4).eq.'04' .or. grid(3:4).eq.'05' .or. grid(3:4).eq.'16' .or. grid(3:4).eq.'17')) &
        lgvalid=.true.

! Mongolia JT..JV
    else if(callsign(2:2).eq.'T' .or. callsign(2:2).eq.'U' .or. callsign(2:2).eq.'V') then
! CE3TSK 2026-10-13: land that this rule refused - Mongolia
      if(grid(1:4).eq.'NN38' .or. grid(1:4).eq.'NN39' .or. grid(1:4).eq.'NO92' .or. grid(1:4).eq.'OO80') lland=.true.
      if((grid(1:2).eq.'ON' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. &
        (grid(1:2).eq.'NN' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':').or. &
        (grid(1:2).eq.'NO' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
        (grid(1:2).eq.'OO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
        (grid(1:2).eq.'OO' .and. (grid(3:4).eq.'60' .or. grid(3:4).eq.'70'))) lgvalid=.true.

! Svalbard, Bear Island JW
    else if(callsign(1:2).eq.'JW') then
      if((grid(1:2).eq.'JQ' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KQ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'JR' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).eq.'0') .or. &
         (grid(1:2).eq.'KR' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).eq.'0') .or. &
         grid(1:4).eq.'JQ94') lgvalid=.true.

! Jan Mayen JX
    else if(callsign(1:2).eq.'JX') then
      if(grid(1:2).eq.'IQ' .and. (grid(3:4).eq.'50' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'61')) lgvalid=.true.

! Jordan JY
    else if(callsign(1:2).eq.'JY') then
      if((grid(1:2).eq.'KM' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4').or. &
         (grid(1:2).eq.'KL' .and. (grid(3:4).eq.'79' .or. grid(3:4).eq.'89'))) lgvalid=.true.

! not valid callsigns
    else if(callsign(2:2).eq.'B' .or. callsign(2:2).eq.'C' .or. callsign(2:2).eq.'1' .or. &
            callsign(2:2).eq.'9' .or. callsign(2:2).eq.'Z' .or. &
            (callsign(1:2).eq.'JD' .and. callsign(3:3).gt.'1' .and. callsign(3:3).lt.':')) then
      lwrongcall=.true.; return

    endif ! 'J'

! USA AA..AL,K,N,W
  else if(callsign(1:1).eq.'K') then
    if(callsign(2:2).gt.'/' .and. callsign(2:2).lt.':' .and. callsign(3:3).gt.'/' .and. callsign(3:3).lt.':') return ! checkcall
! CE3TSK 2026-10-13: land that this rule refused - the United States (the south tip of Big Bend, Texas); Hawaii (the
! Northwestern Hawaiian Islands); Alaska (its north coast - Utqiagvik BQ11, Prudhoe Bay BQ50)
    if(grid(1:4).eq.'AL63' .or. grid(1:4).eq.'AL65' .or. grid(1:4).eq.'AQ80' .or. grid(1:4).eq.'AQ90' .or. &
       grid(1:4).eq.'BQ00' .or. grid(1:4).eq.'BQ10' .or. grid(1:4).eq.'BQ11' .or. grid(1:4).eq.'BQ20' .or. &
       grid(1:4).eq.'BQ21' .or. grid(1:4).eq.'BQ30' .or. grid(1:4).eq.'BQ40' .or. grid(1:4).eq.'BQ50' .or. &
       grid(1:4).eq.'BQ60' .or. grid(1:4).eq.'BQ70' .or. grid(1:4).eq.'BQ80' .or. grid(1:4).eq.'DL88') lland=.true.
! CE3TSK 2026-10-13: Antarctica - the stations of the United States: the South Pole (JA00, as its callbook entry has it,
! and PA90 with the squares either side - the station stands 300 m from the Pole at 139 degrees east), McMurdo RB32 and
! the Sound west of it RB22 (the icebreaker of the yearly resupply: KK7YXD was heard from there), Marble Point RB12, the
! Allan Hills camp QB93, Palmer FC75 (and FC85, 2 km east of it: KC4AAC has sent both), Copacabana GC07, Cape Shirreff
! FC97, Siple Dome BA58, WAIS Divide DB30 and Byrd DA09 (and DB40, DB00, 2 km from them)
    if(grid(1:4).eq.'JA00' .or. grid(1:4).eq.'PA80' .or. grid(1:4).eq.'PA90' .or. grid(1:4).eq.'QA00' .or. &
       grid(1:4).eq.'RB32' .or. grid(1:4).eq.'RB22' .or. grid(1:4).eq.'RB12' .or. grid(1:4).eq.'QB93' .or. &
       grid(1:4).eq.'FC75' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'GC07' .or. grid(1:4).eq.'FC97' .or. &
       grid(1:4).eq.'BA58' .or. grid(1:4).eq.'DB30' .or. grid(1:4).eq.'DB40' .or. grid(1:4).eq.'DA09' .or. &
       grid(1:4).eq.'DB00') lgvalid=.true.
    if((grid(1:2).eq.'FM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'EL' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'DL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
       grid(1:2).eq.'EM' .or. grid(1:2).eq.'DM' .or. grid(1:2).eq.'EN' .or. &
       (grid(1:2).eq.'CM' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'FN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') .or. &
       (grid(1:2).eq.'DN' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9') .or. &
       (grid(1:2).eq.'CN' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9') .or. &
       (grid(1:2).eq.'CO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'FK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.'9') .or. &
       grid(1:2).eq.'BP' .or. &
       (grid(1:2).eq.'BO' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'AP' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
       grid(1:4).eq.'FK28' .or. &
       (grid(1:2).eq.'AO' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'BL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
       (grid(1:2).eq.'BK' .and. (grid(3:4).eq.'19' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'29')) .or. &
       (grid(1:2).eq.'AL' .and. (grid(3:4).eq.'91' .or. grid(3:4).eq.'92' .or. grid(3:4).eq.'08' .or. grid(3:4).eq.'18' .or. &
        grid(3:4).eq.'27' .or. grid(3:4).eq.'36' .or. grid(3:4).eq.'45' .or. grid(3:4).eq.'64' .or. &
        grid(3:4).eq.'73' .or. grid(3:4).eq.'93')) .or. &
       (grid(1:3).eq.'QK2' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'AO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3') .or. &
       (grid(1:2).eq.'RO' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3').or. &
        grid(3:4).eq.'63')) .or. &
       grid(1:4).eq.'AP30' .or. grid(1:4).eq.'QK36' .or. grid(1:4).eq.'QL20' .or. grid(1:4).eq.'RK39' .or. &
! CE3TSK 2026-10-13: AH45 and AH55 added - American Samoa (Tutuila, Manu'a); of it only Swains Island (AH48) was here
       grid(1:4).eq.'AH45' .or. grid(1:4).eq.'AH55' .or. &
! CE3TSK 2026-10-13: these had no square - Guantanamo Bay (KG4) FK29, Baker and Howland (KH1) AJ10, Palmyra,
! Kingman Reef and Jarvis (KH5) AJ85, AJ86, AI99; and CM79 holds the coast of California just south of 40 north
       grid(1:4).eq.'FK29' .or. grid(1:4).eq.'AJ10' .or. grid(1:4).eq.'AJ85' .or. grid(1:4).eq.'AJ86' .or. &
! CE3TSK 2026-10-13: and CM93, the Channel Islands of California south of 34 north (Santa Rosa)
       grid(1:4).eq.'AI99' .or. grid(1:4).eq.'CM79' .or. grid(1:4).eq.'CM93' .or. &
       grid(1:4).eq.'AK56' .or. grid(1:4).eq.'AH48' .or. grid(1:4).eq.'JA00' .or. grid(1:4).eq.'RB32' .or. &
       grid(1:4).eq.'CP00') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'FM' .and. (grid(3:4).eq.'12' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'23' .or. grid(3:4).eq.'24')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'EL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9' &
              .and. grid(3:4).ne.'28'.and. grid(3:4).ne.'58') .or. grid(3:4).eq.'04' .or. grid(3:4).eq.'05' .or. &
              grid(3:4).eq.'14')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DL' .and. (grid(3:4).eq.'78' .or. grid(3:4).eq.'88')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DM' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EN' .and. ((grid(3:3).eq.'9' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
             ((grid(3:3).eq.'8' .or. grid(3:3).eq.'7') .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
             (grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).eq.'9' .and. grid(3:3).ne.'2'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'CM' .and. (grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FN' .and. ((grid(4:4).eq.'7' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
              (grid(4:4).eq.'6' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4') .or. &
              (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4') .or. &
              grid(3:4).eq.'04' .or. grid(3:4).eq.'05' .or. grid(3:4).eq.'15' .or. grid(3:4).eq.'52')) then; lgvalid=.false.
      else if(grid(1:2).eq.'CO' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              (grid(3:3).eq.'1' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'7') .or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'24' .or. grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'BO' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':'.and.grid(4:4).gt.'3'.and.grid(4:4).lt.'9').or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              grid(3:4).eq.'14' .or. grid(3:4).eq.'24' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'35')) then; lgvalid=.false.
      else if(grid(1:2).eq.'AP' .and. ((grid(3:3).eq.'4' .and. ((grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
              (grid(4:4).gt.'3' .and. grid(4:4).lt.':'))) .or. &
              (grid(3:3).eq.'5' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'50' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'62' .or. grid(3:4).eq.'63' .or. &
              grid(3:4).eq.'67' .or. grid(3:4).eq.'69' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
      else if(grid(1:2).eq.'AO' .and. ((grid(3:3).eq.'4' .and. ((grid(4:4).gt.'2' .and. grid(4:4).lt.'7') .or. &
              grid(4:4).eq.'8' .or. grid(4:4).eq.'9')) .or. &
              (grid(3:3).eq.'5' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':' .and. grid(4:4).ne.'6') .or. &
              ((grid(3:3).eq.'6' .or. grid(3:3).eq.'7') .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. &
              (grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'4') .or. &
              grid(3:4).eq.'62' .or. grid(3:4).eq.'97' .or. grid(3:4).eq.'86' .or. grid(3:4).eq.'87' .or.grid(3:4).eq.'31')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'BL' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'12' .or. grid(3:4).eq.'21' .or. &
              grid(3:4).eq.'22')) then; lgvalid=.false.
      else if(grid(1:2).eq.'RO' .and. (grid(3:4).eq.'61' .or. grid(3:4).eq.'71')) then; lgvalid=.false.
      endif
endif

  endif ! end of block A..K

2 if(callsign(1:1).eq.'L') then

! Norway LA..LN
    if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'O') then
! CE3TSK 2026-10-13: land that this rule refused - Norway
      if(grid(1:4).eq.'JP86') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Norway's stations: Troll JB17 and its airfield JB18, Tor JB28
      if(grid(1:4).eq.'JB17' .or. grid(1:4).eq.'JB18' .or. grid(1:4).eq.'JB28') lgvalid=.true.
      if((grid(1:2).eq.'JO' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'7'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'37')) .or. &
         (grid(1:2).eq.'JP' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'KP' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KQ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
         grid(1:4).eq.'JQ90') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'JP' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         (grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
         (grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'7') .or. &
         grid(3:4).eq.'23' .or. grid(3:4).eq.'24' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'58' .or. &
         grid(3:4).eq.'59' .or. grid(3:4).eq.'69' .or. grid(3:4).eq.'97')) then; lgvalid=.false.
      else if(grid(1:2).eq.'KP' .and. grid(4:4).eq.'8' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'6') then; lgvalid=.false.
      else if(grid(1:2).eq.'KQ' .and. (grid(3:4).eq.'01' .or. grid(3:4).eq.'51')) then; lgvalid=.false.
      endif
endif

! Argentina AY,AZ, L1..L9, LO..LW
    else if(callsign(2:2).gt.'N' .and. callsign(2:2).lt.'X') then
! CE3TSK 2026-10-13: Antarctica - Argentina's stations: Belgrano II HB22, Orcadas GC79 (LU1DMZ, LU1ZA), Marambio GC15,
! San Martin FC61, Esperanza and Petrel GC16, Carlini and Camara GC07, Decepcion FC97 (and FC96, 2 km south of it),
! Brown and Melchior FC85, Matienzo and Primavera FC95 (and FC94, 3 km south of Matienzo)
      if(grid(1:4).eq.'HB22' .or. grid(1:4).eq.'GC79' .or. grid(1:4).eq.'GC15' .or. grid(1:4).eq.'FC61' .or. &
         grid(1:4).eq.'GC16' .or. grid(1:4).eq.'GC07' .or. grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC96' .or. &
         grid(1:4).eq.'FC85' .or. grid(1:4).eq.'FC95' .or. grid(1:4).eq.'FC94') lgvalid=.true.
      if((grid(1:2).eq.'GF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'FF' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
! CE3TSK 2026-10-13: of the field FG only the two rows north of 22 degrees south were accepted, where the
! country ends, and none of the eight below them, which are its north-west (Salta, Jujuy, Tucuman, Catamarca,
! Santiago del Estero, the Chaco). Every row is accepted now; FG56 and FG57 stay out by the exclusion below.
         (grid(1:2).eq.'FG' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'GG' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'6').or. &
         (grid(1:2).eq.'FE' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'9') .or. &
         (grid(1:2).eq.'FD' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC16' .or. grid(1:4).eq.'HB22') lgvalid=.true. ! LU1ZB in GC16, LU1ZG in HB22

if(lgvalid) then
      if(grid(1:2).eq.'GF' .and. (grid(3:4).eq.'16' .or. grid(3:4).eq.'17')) then; lgvalid=.false.
! CE3TSK 2026-10-13: and the squares beyond the country's northern end, which hold nothing of it: FG58 (Chile,
! Bolivia), FG59..FG99 (Bolivia, Paraguay - the country ends at 21 degrees 47 minutes south, in row 8), FG98 and
! GG25 (Paraguay, Brazil). Each lies 55 km or more beyond the border; GG15, 5 km beyond it, stays
      else if(grid(1:2).eq.'FG' .and. (grid(3:4).eq.'56' .or. grid(3:4).eq.'57' .or. grid(3:4).eq.'58' .or. &
              grid(3:4).eq.'98' .or. (grid(4:4).eq.'9' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'GG' .and. (grid(3:4).eq.'20' .or. grid(3:4).eq.'30' .or. grid(3:4).eq.'31' .or. &
              grid(3:4).eq.'35' .or. grid(3:4).eq.'25')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FE' .and. ((grid(3:3).eq.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7') .or. &
              (grid(3:3).eq.'3' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'70' .or. grid(3:4).eq.'73')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FD' .and. ((grid(3:3).gt.'2'.and.grid(3:3).lt.'5'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.'8').or. &
              (grid(3:3).eq.'7' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
              (grid(3:3).eq.'6' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':'))) then; lgvalid=.false.
      endif
endif
! CE3TSK 2026-10-13: squares that hold a strip of the country and were refused, most of them by the exclusions
! above - along the Andes (FG40, FE35, FE36, FE37: 60 to 840 km2), on the coast of Santa Cruz (FD69), in
! Tierra del Fuego (FD47, FD64) and Isla de los Estados (FD85). A square can be two countries'
      if(grid(1:4).eq.'FG40' .or. grid(1:4).eq.'FE35' .or. grid(1:4).eq.'FE36' .or. grid(1:4).eq.'FE37' .or. &
         grid(1:4).eq.'FD69' .or. grid(1:4).eq.'FD47' .or. grid(1:4).eq.'FD64' .or. grid(1:4).eq.'FD85') lgvalid=.true.

! Luxemburg LX
    else if(callsign(1:2).eq.'LX') then
      if((grid(1:2).eq.'JN' .and. (grid(3:4).eq.'29' .or. grid(3:4).eq.'39')) .or. &
         (grid(1:2).eq.'JO' .and. (grid(3:4).eq.'20' .or. grid(3:4).eq.'30'))) lgvalid=.true.

! Lithuania LY
    else if(callsign(1:2).eq.'LY') then
      if(grid(1:2).eq.'KO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'7') &
        lgvalid=.true.

! Bulgaria LZ
    else if(callsign(1:2).eq.'LZ') then
! CE3TSK 2026-10-13: Antarctica - Bulgaria's station St. Kliment Ohridski FC97, for every call of the rule
      if(grid(1:4).eq.'FC97') lgvalid=.true.
      if(grid(1:2).eq.'KN' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'5') &
        lgvalid=.true.
! CE3TSK 2026-10-13: LZ0A in FC97, the base on Livingston Island (South Shetland Islands)
      if(callsign(3:3).eq.'0' .and. grid(1:4).eq.'FC97') lgvalid=.true.

! Argentina AY,AZ, L1..L9, LO..LW
    else if(callsign(2:2).gt.'0' .and. callsign(2:2).lt.':') then
! CE3TSK 2026-10-13: Antarctica - Argentina's stations: Belgrano II HB22, Orcadas GC79 (LU1DMZ, LU1ZA), Marambio GC15,
! San Martin FC61, Esperanza and Petrel GC16, Carlini and Camara GC07, Decepcion FC97 (and FC96, 2 km south of it),
! Brown and Melchior FC85, Matienzo and Primavera FC95 (and FC94, 3 km south of Matienzo)
      if(grid(1:4).eq.'HB22' .or. grid(1:4).eq.'GC79' .or. grid(1:4).eq.'GC15' .or. grid(1:4).eq.'FC61' .or. &
         grid(1:4).eq.'GC16' .or. grid(1:4).eq.'GC07' .or. grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC96' .or. &
         grid(1:4).eq.'FC85' .or. grid(1:4).eq.'FC95' .or. grid(1:4).eq.'FC94') lgvalid=.true.
      if((grid(1:2).eq.'GF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'FF' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
! CE3TSK 2026-10-13: of the field FG only the two rows north of 22 degrees south were accepted, where the
! country ends, and none of the eight below them, which are its north-west (Salta, Jujuy, Tucuman, Catamarca,
! Santiago del Estero, the Chaco). Every row is accepted now; FG56 and FG57 stay out by the exclusion below.
         (grid(1:2).eq.'FG' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'GG' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'6').or. &
         (grid(1:2).eq.'FE' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'9') .or. &
         (grid(1:2).eq.'FD' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC16' .or. grid(1:4).eq.'HB22') lgvalid=.true. ! LU1ZB in GC16, LU1ZG in HB22

if(lgvalid) then
      if(grid(1:2).eq.'GF' .and. (grid(3:4).eq.'16' .or. grid(3:4).eq.'17')) then; lgvalid=.false.
! CE3TSK 2026-10-13: and the squares beyond the country's northern end, which hold nothing of it: FG58 (Chile,
! Bolivia), FG59..FG99 (Bolivia, Paraguay - the country ends at 21 degrees 47 minutes south, in row 8), FG98 and
! GG25 (Paraguay, Brazil). Each lies 55 km or more beyond the border; GG15, 5 km beyond it, stays
      else if(grid(1:2).eq.'FG' .and. (grid(3:4).eq.'56' .or. grid(3:4).eq.'57' .or. grid(3:4).eq.'58' .or. &
              grid(3:4).eq.'98' .or. (grid(4:4).eq.'9' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'GG' .and. (grid(3:4).eq.'20' .or. grid(3:4).eq.'30' .or. grid(3:4).eq.'31' .or. &
              grid(3:4).eq.'35' .or. grid(3:4).eq.'25')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FE' .and. ((grid(3:3).eq.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7') .or. &
              (grid(3:3).eq.'3' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'70' .or. grid(3:4).eq.'73')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FD' .and. ((grid(3:3).gt.'2'.and.grid(3:3).lt.'5'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.'8').or. &
              (grid(3:3).eq.'7' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
              (grid(3:3).eq.'6' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':'))) then; lgvalid=.false.
      endif
endif
! CE3TSK 2026-10-13: squares that hold a strip of the country and were refused, most of them by the exclusions
! above - along the Andes (FG40, FE35, FE36, FE37: 60 to 840 km2), on the coast of Santa Cruz (FD69), in
! Tierra del Fuego (FD47, FD64) and Isla de los Estados (FD85). A square can be two countries'
      if(grid(1:4).eq.'FG40' .or. grid(1:4).eq.'FE35' .or. grid(1:4).eq.'FE36' .or. grid(1:4).eq.'FE37' .or. &
         grid(1:4).eq.'FD69' .or. grid(1:4).eq.'FD47' .or. grid(1:4).eq.'FD64' .or. grid(1:4).eq.'FD85') lgvalid=.true.

! not valid callsigns
    else if(callsign(1:2).eq.'L0') then
      lwrongcall=.true.; return

    endif ! 'L'

! UK  GB,GX / 2E,G,M / 2D,GD,GT,MD,MT / 2I,GI,GN,MI,MN / 2J,GH,GJ,MH,MJ / 2A,2M,GM,GS,MA,MM,MS / 2U,GP,GU,MP,MU / 2W,GC,GW,MC,MW
  else if(callsign(1:1).eq.'M') then
! CE3TSK 2026-10-13: land that this rule refused - Scotland (Foula, Shetland)
    if(grid(1:4).eq.'IP80') lland=.true.
! CE3TSK 2026-10-13: Antarctica - the United Kingdom's stations: Rothera FC52, Port Lockroy FC85, Halley HB74 (and HB64,
! where it stood until 2017 and towards which its ice shelf carries it), Signy GC79, Fossil Bluff FB58, Sky Blu FB45
    if(grid(1:4).eq.'FC52' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'HB74' .or. grid(1:4).eq.'HB64' .or. &
       grid(1:4).eq.'GC79' .or. grid(1:4).eq.'FB58' .or. grid(1:4).eq.'FB45') lgvalid=.true.
    if((grid(1:2).eq.'IO' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':') .or. &
       (grid(1:3).eq.'JO0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
! CE3TSK 2026-10-13: IN79 was missing - the Lizard, the south-west end of Cornwall
       grid(1:4).eq.'IP90' .or. grid(1:4).eq.'IN69' .or. grid(1:4).eq.'IN79' .or. grid(1:4).eq.'IN89' .or. &
       grid(1:4).eq.'IO37') lgvalid=.true.

if(lgvalid .and. grid(1:2).eq.'IO') then
    if((grid(1:3).eq.'IO5' .and. grid(4:4).ne.'4' .and. grid(4:4).ne.'7') .or. &
       (grid(1:3).eq.'IO6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
       grid(3:4).eq.'96' .or. grid(3:4).eq.'98') lgvalid=.false.
endif

! USA AA..AL,K,N,W
  else if(callsign(1:1).eq.'N') then
    if(callsign(2:2).gt.'/' .and. callsign(2:2).lt.':' .and. callsign(3:3).gt.'/' .and. callsign(3:3).lt.':') return ! checkcall
! CE3TSK 2026-10-13: land that this rule refused - the United States (the south tip of Big Bend, Texas); Hawaii (the
! Northwestern Hawaiian Islands); Alaska (its north coast - Utqiagvik BQ11, Prudhoe Bay BQ50)
    if(grid(1:4).eq.'AL63' .or. grid(1:4).eq.'AL65' .or. grid(1:4).eq.'AQ80' .or. grid(1:4).eq.'AQ90' .or. &
       grid(1:4).eq.'BQ00' .or. grid(1:4).eq.'BQ10' .or. grid(1:4).eq.'BQ11' .or. grid(1:4).eq.'BQ20' .or. &
       grid(1:4).eq.'BQ21' .or. grid(1:4).eq.'BQ30' .or. grid(1:4).eq.'BQ40' .or. grid(1:4).eq.'BQ50' .or. &
       grid(1:4).eq.'BQ60' .or. grid(1:4).eq.'BQ70' .or. grid(1:4).eq.'BQ80' .or. grid(1:4).eq.'DL88') lland=.true.
! CE3TSK 2026-10-13: Antarctica - the stations of the United States: the South Pole (JA00, as its callbook entry has it,
! and PA90 with the squares either side - the station stands 300 m from the Pole at 139 degrees east), McMurdo RB32 and
! the Sound west of it RB22 (the icebreaker of the yearly resupply: KK7YXD was heard from there), Marble Point RB12, the
! Allan Hills camp QB93, Palmer FC75 (and FC85, 2 km east of it: KC4AAC has sent both), Copacabana GC07, Cape Shirreff
! FC97, Siple Dome BA58, WAIS Divide DB30 and Byrd DA09 (and DB40, DB00, 2 km from them)
    if(grid(1:4).eq.'JA00' .or. grid(1:4).eq.'PA80' .or. grid(1:4).eq.'PA90' .or. grid(1:4).eq.'QA00' .or. &
       grid(1:4).eq.'RB32' .or. grid(1:4).eq.'RB22' .or. grid(1:4).eq.'RB12' .or. grid(1:4).eq.'QB93' .or. &
       grid(1:4).eq.'FC75' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'GC07' .or. grid(1:4).eq.'FC97' .or. &
       grid(1:4).eq.'BA58' .or. grid(1:4).eq.'DB30' .or. grid(1:4).eq.'DB40' .or. grid(1:4).eq.'DA09' .or. &
       grid(1:4).eq.'DB00') lgvalid=.true.
    if((grid(1:2).eq.'FM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'EL' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'DL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
       grid(1:2).eq.'EM' .or. grid(1:2).eq.'DM' .or. grid(1:2).eq.'EN' .or. &
       (grid(1:2).eq.'CM' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'FN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') .or. &
       (grid(1:2).eq.'DN' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9') .or. &
       (grid(1:2).eq.'CN' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9') .or. &
       (grid(1:2).eq.'CO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'FK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.'9') .or. &
       grid(1:2).eq.'BP' .or. &
       (grid(1:2).eq.'BO' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'AP' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
       grid(1:4).eq.'FK28' .or. &
       (grid(1:2).eq.'AO' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'BL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
       (grid(1:2).eq.'BK' .and. (grid(3:4).eq.'19' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'29')) .or. &
       (grid(1:2).eq.'AL' .and. (grid(3:4).eq.'91' .or. grid(3:4).eq.'92' .or. grid(3:4).eq.'08' .or. grid(3:4).eq.'18' .or. &
        grid(3:4).eq.'27' .or. grid(3:4).eq.'36' .or. grid(3:4).eq.'45' .or. grid(3:4).eq.'64' .or. &
        grid(3:4).eq.'73' .or. grid(3:4).eq.'93')) .or. &
       (grid(1:3).eq.'QK2' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'AO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3') .or. &
       (grid(1:2).eq.'RO' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3').or. &
        grid(3:4).eq.'63')) .or. &
       grid(1:4).eq.'AP30' .or. grid(1:4).eq.'QK36' .or. grid(1:4).eq.'QL20' .or. grid(1:4).eq.'RK39' .or. &
! CE3TSK 2026-10-13: AH45 and AH55 added - American Samoa (Tutuila, Manu'a); of it only Swains Island (AH48) was here
       grid(1:4).eq.'AH45' .or. grid(1:4).eq.'AH55' .or. &
! CE3TSK 2026-10-13: these had no square - Guantanamo Bay (KG4) FK29, Baker and Howland (KH1) AJ10, Palmyra,
! Kingman Reef and Jarvis (KH5) AJ85, AJ86, AI99; and CM79 holds the coast of California just south of 40 north
       grid(1:4).eq.'FK29' .or. grid(1:4).eq.'AJ10' .or. grid(1:4).eq.'AJ85' .or. grid(1:4).eq.'AJ86' .or. &
! CE3TSK 2026-10-13: and CM93, the Channel Islands of California south of 34 north (Santa Rosa)
       grid(1:4).eq.'AI99' .or. grid(1:4).eq.'CM79' .or. grid(1:4).eq.'CM93' .or. &
       grid(1:4).eq.'AK56' .or. grid(1:4).eq.'AH48' .or. grid(1:4).eq.'JA00' .or. grid(1:4).eq.'RB32' .or. &
       grid(1:4).eq.'CP00') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'FM' .and. (grid(3:4).eq.'12' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'23' .or. grid(3:4).eq.'24')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'EL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9' &
              .and. grid(3:4).ne.'28'.and. grid(3:4).ne.'58') .or. grid(3:4).eq.'04' .or. grid(3:4).eq.'05' .or. &
              grid(3:4).eq.'14')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DL' .and. (grid(3:4).eq.'78' .or. grid(3:4).eq.'88')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DM' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EN' .and. ((grid(3:3).eq.'9' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
             ((grid(3:3).eq.'8' .or. grid(3:3).eq.'7') .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
             (grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).eq.'9' .and. grid(3:3).ne.'2'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'CM' .and. (grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FN' .and. ((grid(4:4).eq.'7' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
              (grid(4:4).eq.'6' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4') .or. &
              (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4') .or. &
              grid(3:4).eq.'04' .or. grid(3:4).eq.'05' .or. grid(3:4).eq.'15' .or. grid(3:4).eq.'52')) then; lgvalid=.false.
      else if(grid(1:2).eq.'CO' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              (grid(3:3).eq.'1' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'7') .or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'24' .or. grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'BO' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':'.and.grid(4:4).gt.'3'.and.grid(4:4).lt.'9').or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              grid(3:4).eq.'14' .or. grid(3:4).eq.'24' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'35')) then; lgvalid=.false.
      else if(grid(1:2).eq.'AP' .and. ((grid(3:3).eq.'4' .and. ((grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
              (grid(4:4).gt.'3' .and. grid(4:4).lt.':'))) .or. &
              (grid(3:3).eq.'5' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'50' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'62' .or. grid(3:4).eq.'63' .or. &
              grid(3:4).eq.'67' .or. grid(3:4).eq.'69' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
      else if(grid(1:2).eq.'AO' .and. ((grid(3:3).eq.'4' .and. ((grid(4:4).gt.'2' .and. grid(4:4).lt.'7') .or. &
              grid(4:4).eq.'8' .or. grid(4:4).eq.'9')) .or. &
              (grid(3:3).eq.'5' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':' .and. grid(4:4).ne.'6') .or. &
              ((grid(3:3).eq.'6' .or. grid(3:3).eq.'7') .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. &
              (grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'4') .or. &
              grid(3:4).eq.'62' .or. grid(3:4).eq.'97' .or. grid(3:4).eq.'86' .or. grid(3:4).eq.'87' .or.grid(3:4).eq.'31')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'BL' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'12' .or. grid(3:4).eq.'21' .or. &
              grid(3:4).eq.'22')) then; lgvalid=.false.
      else if(grid(1:2).eq.'RO' .and. (grid(3:4).eq.'61' .or. grid(3:4).eq.'71')) then; lgvalid=.false.
      endif
endif

  else if(callsign(1:1).eq.'O') then

! Finland OF..OJ
    if(callsign(2:2).gt.'E' .and. callsign(2:2).lt.'K') then
! CE3TSK 2026-10-13: Antarctica - Finland's station Aboa IB36
      if(grid(1:4).eq.'IB36') lgvalid=.true.
      if((grid(1:2).eq.'KP' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'6') .or. &
         grid(1:4).eq.'JP90' .or. grid(1:4).eq.'KO09' .or. grid(1:4).eq.'KO19' .or. grid(1:4).eq.'KQ30') lgvalid=.true.

! Belgium ON..OT
    else if(callsign(2:2).gt.'M' .and. callsign(2:2).lt.'U') then
! CE3TSK 2026-10-13: Antarctica - Belgium's station Princess Elisabeth KB18
      if(grid(1:4).eq.'KB18') lgvalid=.true.
      if((grid(1:2).eq.'JO' .and. (grid(3:4).eq.'10' .or. grid(3:4).eq.'11' .or. grid(3:4).eq.'20' .or. grid(3:4).eq.'21' .or. &
         grid(3:4).eq.'30')) .or. grid(1:4).eq.'JN29') lgvalid=.true.

! Austria OE
    else if(callsign(1:2).eq.'OE') then
! CE3TSK 2026-10-13: land that this rule refused - Austria
      if(grid(1:4).eq.'JN79') lland=.true.
      if(grid(1:2).eq.'JN' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.'9') &
        lgvalid=.true.

! Czech Republic OK,OL
    else if(callsign(2:2).eq.'K' .or. callsign(2:2).eq.'L') then
! CE3TSK 2026-10-13: Antarctica - the Czech stations: Mendel GC16, Eco-Nelson GC07
      if(grid(1:4).eq.'GC16' .or. grid(1:4).eq.'GC07') lgvalid=.true.
      if((grid(1:2).eq.'JO' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).eq.'0').or. &
         (grid(1:2).eq.'JN' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'JO71') lgvalid=.true.

! Slovak Republic OM
    else if(callsign(1:2).eq.'OM') then
      if((grid(1:2).eq.'JN' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':')) &
        lgvalid=.true.

! Denmark 5P,5Q,OU,OV,OZ
    else if(callsign(2:2).eq.'Z' .or. callsign(2:2).eq.'U' .or. callsign(2:2).eq.'V') then
      if(grid(1:2).eq.'JO' .and. &
         ((grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
          grid(3:4).eq.'75' .or. grid(3:4).eq.'76')) lgvalid=.true.

! Lebanon OD
    else if(callsign(1:2).eq.'OD') then
! CE3TSK 2026-10-13: land that this rule refused - Lebanon
      if(grid(1:4).eq.'KM83') lland=.true.
      if(grid(1:2).eq.'KM' .and. (grid(3:4).eq.'73' .or. grid(3:4).eq.'74' .or. grid(3:4).eq.'84')) lgvalid=.true.

! Peru 4T,OA..OC
    else if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'D') then
! CE3TSK 2026-10-13: land that this rule refused - Peru
      if(grid(1:4).eq.'FH09' .or. grid(1:4).eq.'FI55') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Peru's station Machu Picchu GC07
      if(grid(1:4).eq.'GC07') lgvalid=.true.
      if((grid(1:2).eq.'FH' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'FI' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
         (grid(1:3).eq.'EI9' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'7')) lgvalid=.true.

! Greenland OX,XP
    else if(callsign(1:2).eq.'OX' .or. callsign(1:2).eq.'XP') then
      if((grid(1:2).eq.'GP' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'HP' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'HQ90' .or. &
         (grid(1:2).eq.'GQ' .and. (grid(3:4).eq.'30' .or. grid(3:4).eq.'31' .or. grid(3:4).eq.'41' .or.grid(3:4).eq.'22'.or. &
          grid(3:4).eq.'14' .or. grid(3:4).eq.'12')) .or. &
         (grid(1:2).eq.'FQ' .and. (grid(3:4).eq.'76' .or. grid(3:4).eq.'67' .or. grid(3:4).eq.'57' .or.grid(3:4).eq.'56'.or. &
          grid(3:4).eq.'38'))) lgvalid=.true.

! Faroe Islands OW,OY
! CE3TSK 2026-10-13: there was no rule for these calls, so no square was accepted for them and every weak CQ of
! theirs was deleted. The squares are the ones the country touches (geodata/dxcc_grids.json, Natural Earth).
    else if(callsign(1:2).eq.'OY' .or. callsign(1:2).eq.'OW') then
      if(grid(1:4).eq.'IP61' .or. grid(1:4).eq.'IP62') lgvalid=.true.

! not valid callsigns O0..O9
    else if(callsign(2:2).gt.'/' .and. callsign(2:2).lt.':') then
      lwrongcall=.true.; return

    endif ! 'O'

  else if(callsign(1:1).eq.'P') then

! Netherlands PA..PI
    if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'J') then
! CE3TSK 2026-10-13: Antarctica - the Dutch laboratory at Rothera FC52
      if(grid(1:4).eq.'FC52') lgvalid=.true.
      if(((grid(1:3).eq.'JO2' .or. grid(1:3).eq.'JO3') .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
         grid(1:4).eq.'JO11') lgvalid=.true.

! Brazil PP..PY, ZV..ZZ
    else if(callsign(1:1).eq.'P' .and. callsign(2:2).gt.'O' .and. callsign(2:2).lt.'Z') then
! CE3TSK 2026-10-13: land that this rule refused - Brazil
      if(grid(1:4).eq.'FI22' .or. grid(1:4).eq.'FI49' .or. grid(1:4).eq.'FJ40' .or. grid(1:4).eq.'FJ52' .or. &
         grid(1:4).eq.'FJ62' .or. grid(1:4).eq.'GJ05') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Brazil's station Comandante Ferraz GC07 and its refuges (Cruls GC07, Goeldi GC28)
      if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC28') lgvalid=.true.
      if((grid(1:2).eq.'GG' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.':') .or. grid(3:4).eq.'09')) .or. &
         (grid(1:2).eq.'HI' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6'.and. &
          grid(3:4).ne.'25') .or. grid(3:4).eq.'06' .or. grid(3:4).eq.'07' .or. grid(3:4).eq.'36')) .or. &
         (grid(1:2).eq.'HH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2') .or. &
         (grid(1:2).eq.'GH' .and. grid(3:4).ne.'01') .or. grid(1:2).eq.'GI' .or. &
         (grid(1:2).eq.'GF' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'6'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'36')) .or. &
         (grid(1:2).eq.'GJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'4').or. &
          grid(3:4).eq.'04' .or. grid(3:4).eq.'44')) .or. &
         (grid(1:2).eq.'FJ' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
          grid(3:4).eq.'95')) .or. &
         (grid(1:2).eq.'FI' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'FH' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'93' .or. grid(3:4).eq.'94' .or. grid(3:4).eq.'95')) .or. &
         grid(1:4).eq.'HG59' .or. grid(1:4).eq.'HJ50') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'GG' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. &
         grid(4:4).lt.'6' .and. grid(3:4).ne.'64' .and. grid(3:4).ne.'65') .or. &
         (grid(3:3).eq.'1' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'7'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'HH' .and. (grid(3:3).eq.'1' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7')) then; lgvalid=.false.
      else if(grid(1:2).eq.'GI' .and. ((grid(3:3).gt.'7'.and.grid(3:3).lt.':'.and.grid(4:4).gt.'7'.and.grid(4:4).lt.':').or. &
              grid(3:4).eq.'79')) then; lgvalid=.false.
      else if(grid(1:2).eq.'GF' .and. (grid(3:4).eq.'17' .or. grid(3:4).eq.'27')) then; lgvalid=.false.
      else if(grid(1:2).eq.'GJ' .and. (grid(3:4).eq.'13' .or. grid(3:4).eq.'23' .or. grid(3:4).eq.'52' .or. &
              grid(3:4).eq.'53')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FJ' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5') &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'FI' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'FH' .and. ((grid(3:3).gt.'3'.and.grid(3:3).lt.'7'.and.grid(4:4).gt.'5'.and.grid(4:4).lt.'8').or. &
              grid(3:4).eq.'76' .or. grid(3:4).eq.'68')) then; lgvalid=.false.
      endif
endif

! Papua New Guinea P2
    else if(callsign(1:2).eq.'P2') then
! CE3TSK 2026-10-13: land that this rule refused - Papua New Guinea
      if(grid(1:4).eq.'QI95') lland=.true.
      if((grid(1:2).eq.'QI' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9').or.&
         (grid(1:2).eq.'QH' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':')) &
        lgvalid=.true.

! Cyprus 5B,C4,H2,P3
    else if(callsign(1:2).eq.'P3') then
      if((grid(1:3).eq.'KM6' .and. (grid(4:4).eq.'4' .or. grid(4:4).eq.'5')) .or. &
         (grid(1:3).eq.'KM7' .and. (grid(4:4).eq.'4' .or. grid(4:4).eq.'5'))) lgvalid=.true.

! Aruba P4
    else if(callsign(1:2).eq.'P4' .and. (grid(1:4).eq.'FK42' .or. grid(1:4).eq.'FK52')) then
      lgvalid=.true.

! DPR of Korea P5..P9
    else if(callsign(2:2).gt.'4' .and. callsign(2:2).lt.':') then
! CE3TSK 2026-10-13: land that this rule refused - North Korea
      if(grid(1:4).eq.'PN43') lland=.true.
      if((grid(1:2).eq.'PM' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or.&
         (grid(1:2).eq.'PN' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3')) &
        lgvalid=.true.

! Curacao, Bonaire, Saba & St. Eustatius, Sint Maarten PJ
! CE3TSK 2026-10-13: the three alternatives had no brackets round them, so "PJ and" held for the first only:
! the other two were true of ANY call that came this far - P05AA was accepted in FK87, P07AA in FK88, and the
! "not valid callsigns" rule below never saw them.
    else if(callsign(1:2).eq.'PJ' .and. &
            (((callsign(3:3).eq.'2' .or. callsign(3:3).eq.'4') .and. grid(1:4).eq.'FK52') .or. &
             ((callsign(3:3).eq.'5' .or. callsign(3:3).eq.'6') .and. grid(1:4).eq.'FK87') .or. &
             ((callsign(3:3).eq.'7' .or. callsign(3:3).eq.'8' .or. callsign(3:3).eq.'0') .and. grid(1:4).eq.'FK88'))) then
      lgvalid=.true.

! Suriname PZ
    else if(callsign(1:2).eq.'PZ') then
! CE3TSK 2026-10-13: land that this rule refused - Suriname
      if(grid(1:4).eq.'GJ03' .or. grid(1:4).eq.'GJ04' .or. grid(1:4).eq.'GJ11' .or. grid(1:4).eq.'GJ16' .or. &
         grid(1:4).eq.'GJ21' .or. grid(1:4).eq.'GJ33' .or. grid(1:4).eq.'GJ35') lland=.true.
      if(grid(1:2).eq.'GJ' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'6') &
        lgvalid=.true.

! Indonesia  7A..7I, 8A..8I, PK..PO, YB..YH
    else if(callsign(2:2).gt.'J' .and. callsign(2:2).lt.'P') then
! CE3TSK 2026-10-13: land that this rule refused - Indonesia
      if(grid(1:4).eq.'OH99' .or. grid(1:4).eq.'PI34' .or. grid(1:4).eq.'PJ35' .or. grid(1:4).eq.'PJ51' .or. &
         grid(1:4).eq.'PJ70') lland=.true.
      if((grid(1:2).eq.'NJ' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'NI' .and. grid(3:3).gt.'8' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'OI' .and. .not.(grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'4') &
          .and. .not.(grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).eq.'0')) .or. &
         (grid(1:2).eq.'PI' .and. .not.(grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).eq.'0')) .or. &
         (grid(1:2).eq.'OJ' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
         (grid(1:2).eq.'PJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'QI' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'1' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8').or. &
         (grid(1:2).eq.'PH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).eq.'9')) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'PJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'1' .and.grid(4:4).lt.'5').or. &
         grid(3:4).eq.'43' .or. grid(3:4).eq.'44')) then; lgvalid=.false.
      else if(grid(1:2).eq.'NJ' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'/' .and. grid(4:4) .lt.'6') .or. &
         (grid(3:3).eq.'7' .and. ((grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. grid(4:4).eq.'0' .or. grid(4:4).eq.'1' .or. &
          grid(4:4).eq.'3')) .or. (grid(3:3).eq.'8' .and. grid(4:4).gt.'5'.and. grid(4:4).lt.':') .or. &
         grid(3:4).eq.'43' .or. grid(3:4).eq.'44' .or. grid(3:4).eq.'95')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OJ' .and. ((grid(3:3).gt.'4'.and.grid(3:3).lt.'7'.and.grid(4:4).gt.'1'.and.grid(4:4).lt.'5').or. &
         (grid(3:3).eq.'1' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5').or. &
         grid(3:4).eq.'24' .or. grid(3:4).eq.'04')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OI' .and. ((grid(4:4).eq.'1' .and. grid(3:3).gt.'1'.and. grid(3:3).lt.'5') .or. &
         (grid(4:4).eq.'5' .and. grid(3:3).gt.'2'.and. grid(3:3).lt.'6') .or. &
         grid(3:4).eq.'44' .or. grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'PI' .and. (grid(3:4).eq.'61' .or.grid(3:4).eq.'62'.or.grid(3:4).eq.'71'.or.grid(3:4).eq.'83'.or. &
         grid(3:4).eq.'25' .or. grid(3:4).eq.'35' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'44' .or.grid(3:4).eq.'29')) then
        lgvalid=.false.
      endif
endif

! not valid callsigns
    else if(callsign(1:2).eq.'P1' .or. callsign(1:2).eq.'P0') then
      lwrongcall=.true.; return

    endif ! 'P'

! Russia
  else if(callsign(1:1).eq.'R') then
! CE3TSK 2026-10-13: Antarctica - Russia's stations: Bellingshausen GC07, Novolazarevskaya JB59, Progress MC80, Mirny
! NC63, Vostok OB31, Molodyozhnaya LC22, Druzhnaya IV MC60, Soyuz MB49, Bunger Oasis OC03, Leningradskaya QC90, Russkaya
! CB15
    if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'JB59' .or. grid(1:4).eq.'MC80' .or. grid(1:4).eq.'NC63' .or. &
       grid(1:4).eq.'OB31' .or. grid(1:4).eq.'LC22' .or. grid(1:4).eq.'MC60' .or. grid(1:4).eq.'MB49' .or. &
       grid(1:4).eq.'OC03' .or. grid(1:4).eq.'QC90' .or. grid(1:4).eq.'CB15') lgvalid=.true.
! CE3TSK 2026-10-13: Franz Josef Land (RI1F) had no square; its own are accepted for these calls now
    if(callsign(1:4).eq.'RI1F' .and. (grid(1:4).eq.'KR80' .or. grid(1:4).eq.'LQ59' .or. grid(1:4).eq.'LQ99' .or. &
       (grid(1:2).eq.'LR' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.':' .and. grid(4:4).eq.'0') .or. grid(3:4).eq.'51' .or. &
        grid(3:4).eq.'71' .or. grid(3:4).eq.'81' .or. grid(3:4).eq.'91')) .or. &
       (grid(1:2).eq.'MR' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. (grid(4:4).eq.'0' .or. grid(4:4).eq.'1')))) &
      lgvalid=.true.
    if((grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.':').or. &
       (grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'6'))) .or. &
        grid(1:2).eq.'LO' .or. grid(1:2).eq.'MO' .or. grid(1:2).eq.'NO' .or. &
       (grid(1:2).eq.'LN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'KN' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'KP' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. grid(3:4).eq.'30')) .or. grid(1:2).eq.'NP' &
       .or. grid(1:2).eq.'OO' .or. grid(1:2).eq.'PO' .or. grid(1:2).eq.'LP' .or. grid(1:2).eq.'MP' .or. &
       (grid(1:2).eq.'PN' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
        grid(3:4).eq.'39' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'52' .or. grid(3:4).eq.'62')) .or.grid(1:4).eq.'MQ60'.or. &
       (grid(1:2).eq.'QN' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
        grid(3:4).eq.'28' .or. grid(3:4).eq.'29')) .or. &
       (grid(1:2).eq.'QO' .and. (grid(3:3).eq.'0' .or. grid(3:3).eq.'1' .or. grid(3:3).eq.'8' .or. grid(3:3).eq.'9' .or. &
        grid(3:4).eq.'49' .or. grid(3:4).eq.'59')) .or. grid(1:4).eq.'JO94' .or. &
       grid(1:2).eq.'OP' .or. grid(1:2).eq.'PP' .or. grid(1:4).eq.'JB59' .or. grid(1:4).eq.'NQ03' .or. &
       (grid(1:2).eq.'RP' .and. (grid(3:4).eq.'59' .or. grid(3:4).eq.'84'))) lgvalid=.true.
! CE3TSK 2026-10-13: Chukotka east of 180 degrees (Provideniya is AP34) had no square; the ones the country
! touches there are accepted now (geodata/dxcc_grids.json)
    if(grid(1:2).eq.'AP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. &
       ((grid(3:3).eq.'2' .or. grid(3:3).eq.'3') .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
       ((grid(3:3).eq.'4' .or. grid(3:3).eq.'5') .and. (grid(4:4).eq.'5' .or. grid(4:4).eq.'6')))) lgvalid=.true.

if(lgvalid) then
    if(grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
       grid(3:4).eq.'35' .or. grid(3:4).eq.'50' .or. grid(3:4).eq.'60' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'61')) &
      then; lgvalid=.false.
    else if(grid(1:2).eq.'LO' .and. (grid(3:4).eq.'50' .or. grid(3:4).eq.'60')) then; lgvalid=.false.
    else if(grid(1:2).eq.'LN' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).eq.'1') .or.  &
            grid(3:4).eq.'43' .or. grid(3:4).eq.'44' .or. grid(3:4).eq.'48')) then; lgvalid=.false.
    else if(grid(1:2).eq.'MO' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'9'.and.grid(4:4).gt.'/'.and.grid(4:4).lt.'3').or. &
            (grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).eq.'3'))) then; lgvalid=.false.
! CE3TSK 2026-10-12: KN77, KN87 and KN88 are no longer in this exclusion. Stations with these calls are on the
! air from the three squares - 61 of them in sixteen months of one station's decodes - and each lost every weak
! CQ. The rest of the block stays excluded: nothing was heard from it.
    else if(grid(1:2).eq.'KN' .and. (grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. ((grid(4:4).gt.'6' .and. &
            grid(4:4).lt.':') .or. grid(4:4).eq.'3')) .and. grid(3:4).ne.'77' .and. grid(3:4).ne.'87' .and. &
            grid(3:4).ne.'88') then; lgvalid=.false.
    else if(grid(1:2).eq.'KP' .and. (grid(3:4).eq.'42' .or. grid(3:4).eq.'43' .or. grid(3:4).eq.'99')) then; lgvalid=.false.
    else if(grid(1:2).eq.'OO' .and. grid(3:4).eq.'00') then; lgvalid=.false.
    else if(grid(1:2).eq.'PO' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).eq.'0') .or.  &
            grid(3:4).eq.'11' .or. grid(3:4).eq.'12' .or. grid(3:4).eq.'21')) then; lgvalid=.false.
    else if(grid(1:2).eq.'LP' .and. grid(4:4).eq.'9' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'9' .and. &
            grid(3:3).ne.'4' .and. grid(3:3).ne.'5') then; lgvalid=.false.
    else if(grid(1:2).eq.'PN' .and. (grid(3:4).eq.'56' .or. grid(3:4).eq.'83' .or. grid(3:4).eq.'93' .or. &
            grid(3:4).eq.'94' .or. grid(3:4).eq.'95')) then; lgvalid=.false.
    endif
endif

  else if(callsign(1:1).eq.'S') then

! Sweden 7S,8S,SA..SM
    if(callsign(1:1).eq.'S' .and. callsign(2:2).gt.'@' .and. callsign(2:2).lt.'N') then
! CE3TSK 2026-10-13: land that this rule refused - Sweden
      if(grid(1:4).eq.'KP26') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Sweden's stations: Wasa IB36, Svea IB45
      if(grid(1:4).eq.'IB36' .or. grid(1:4).eq.'IB45') lgvalid=.true.
      if((grid(1:2).eq.'JO' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'JP' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9').or. &
         (grid(1:2).eq.'KP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'2'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'25'))) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'JO' .and. (grid(3:4).eq.'85' .or. grid(3:4).eq.'95')) then; lgvalid=.false.
      else if(grid(1:2).eq.'JP' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. &
             (grid(3:3).eq.'7' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.'9') .or. grid(3:4).eq.'89' .or. &
              grid(3:4).eq.'99')) then; lgvalid=.false.
      else if(grid(1:2).eq.'KP' .and. (grid(3:4).eq.'13' .or. grid(3:4).eq.'14')) then; lgvalid=.false.
      endif
endif

! Poland 3Z,HF,SN..SR
    else if(callsign(1:1).eq.'S' .and. callsign(2:2).gt.'M' .and. callsign(2:2).lt.'S') then
! CE3TSK 2026-10-13: Antarctica - Poland's stations: Arctowski GC07, Dobrowolski OC03
      if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'OC03') lgvalid=.true.
      if((grid(1:2).eq.'JO' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
          grid(3:4).eq.'20')) .or. &
         grid(1:4).eq.'KN09' .or. grid(1:4).eq.'KN19' .or. grid(1:4).eq.'KO20' .or. grid(1:4).eq.'JN99' .or. &
         grid(1:4).eq.'JN89') lgvalid=.true.
  
! Greece J4,SV..SZ
    else if(callsign(2:2).gt.'U' .and. callsign(2:2).lt.'[') then
      if((grid(1:2).eq.'KM' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'46' .or. grid(3:4).eq.'24' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'14').and.grid(3:4).ne.'05').or. &
         grid(1:4).eq.'JM99' .or. &
         (grid(1:2).eq.'KN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. (grid(4:4).eq.'0' .or. grid(4:4).eq.'1'))) &
        lgvalid=.true.

! Slovenia S5
    else if(callsign(1:2).eq.'S5') then
      if(grid(1:2).eq.'JN' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'7') &
        lgvalid=.true.

! Singapore 9V,S6
! CE3TSK 2026-10-13: here, among the calls that begin with S, the rule asked for 9V and was never met.
    else if(callsign(1:2).eq.'S6') then
      if(grid(1:2).eq.'OJ' .and. (grid(3:4).eq.'11' .or. grid(3:4).eq.'21')) lgvalid=.true.

! Western Sahara S0
    else if(callsign(1:2).eq.'S0') then
      if(grid(1:2).eq.'IL' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') &
        lgvalid=.true.

! Bangladesh S2,S3
    else if(callsign(1:2).eq.'S2' .or. callsign(1:2).eq.'S3') then
! CE3TSK 2026-10-13: land that this rule refused - Bangladesh
      if(grid(1:4).eq.'NL60') lland=.true.
      if(grid(1:2).eq.'NL' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'7') &
        lgvalid=.true.

! South Africa H5,S4,S8,V9,ZR,ZS,ZT,ZU
    else if(callsign(1:2).eq.'S4' .or. callsign(1:2).eq.'S8') then
! CE3TSK 2026-10-13: land that this rule refused - South Africa
      if(grid(1:4).eq.'JG92' .or. grid(1:4).eq.'JG93' .or. grid(1:4).eq.'JG94' .or. grid(1:4).eq.'JG95') lland=.true.
! CE3TSK 2026-10-13: Antarctica - South Africa's station SANAE IV IB88, for every call of the rule
      if(grid(1:4).eq.'IB88') lgvalid=.true.
      if((grid(1:2).eq.'KG' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8').or. &
          (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4'))) .or. &
        (grid(1:2).eq.'KF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. &
        (grid(1:2).eq.'JF' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':' .and. &
         grid(3:4).ne.'85') .or. &
        (grid(1:2).eq.'JG' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
        grid(1:4).eq.'KE83' .or. grid(1:4).eq.'KE93') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'KF' .and. ((grid(4:4).eq.'5' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'6') .or. &
         (grid(4:4).eq.'6' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'6') .or. grid(3:4).eq.'57')) lgvalid=.false.
endif

! Seychelles S7
    else if(callsign(1:2).eq.'S7') then
      if((grid(1:2).eq.'LI' .and. (grid(3:4).eq.'75' .or. grid(3:4).eq.'76' .or. grid(3:4).eq.'74' .or. grid(3:4).eq.'64' &
        .or. grid(3:4).eq.'63' .or. grid(3:4).eq.'82' .or. grid(3:4).eq.'62' .or. grid(3:4).eq.'50' .or. grid(3:4).eq.'30'))&
         .or. (grid(1:2).eq.'LH' .and. (grid(3:4).eq.'59' .or. grid(3:4).eq.'39'))) lgvalid=.true.

! Sao Tome & Principe S9
    else if(callsign(1:2).eq.'S9' .and. (grid(1:4).eq.'JJ30' .or. grid(1:4).eq.'JJ31')) then
      lgvalid=.true.

! Sudan 6T,6U,ST
    else if(callsign(1:2).eq.'ST') then
! CE3TSK 2026-10-13: land that this rule refused - Sudan
      if(grid(1:4).eq.'KJ18' .or. grid(1:4).eq.'KJ19' .or. grid(1:4).eq.'KJ28' .or. grid(1:4).eq.'KJ29' .or. &
         grid(1:4).eq.'KJ39' .or. grid(1:4).eq.'KJ49' .or. grid(1:4).eq.'KJ59' .or. grid(1:4).eq.'KJ69' .or. &
         grid(1:4).eq.'KK02' .or. grid(1:4).eq.'KK03' .or. grid(1:4).eq.'KL52') lland=.true.
      if((grid(1:2).eq.'KK' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'KL' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
         grid(1:4).eq.'KJ79') lgvalid=.true.

! Egypt 6A,6B,SS,SU
! CE3TSK 2026-10-13: there was no rule for these calls, so no square was accepted for them and every weak CQ of
! theirs was deleted. The squares are the ones the country touches (geodata/dxcc_grids.json, Natural Earth).
    else if(callsign(1:2).eq.'SU' .or. callsign(1:2).eq.'SS') then
      if((grid(1:2).eq.'KL' .and. ((grid(3:3).eq.'8' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3') .or. &
          (grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':'))) .or. &
         (grid(1:2).eq.'KM' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2')) &
        lgvalid=.true.

! not valid callsigns
    else if(callsign(1:2).eq.'S1') then
      lwrongcall=.true.; return

    endif ! 'S'

  else if(callsign(1:1).eq.'T') then

! Turkey TA..TC,YM
    if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'D') then
! CE3TSK 2026-10-13: land that this rule refused - Turkey
      if(grid(1:4).eq.'KM29' .or. grid(1:4).eq.'KM75' .or. grid(1:4).eq.'LN20') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Turkey's station on Horseshoe Island FC62
      if(grid(1:4).eq.'FC62') lgvalid=.true.
      if((grid(1:2).eq.'KM' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'85')) .or. &
         (grid(1:2).eq.'KN' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
          grid(3:4).eq.'20' .or. grid(3:4).eq.'32' .or. grid(3:4).eq.'62' .or. grid(3:4).eq.'72')) .or. &
         (grid(1:2).eq.'LN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
         (grid(1:2).eq.'LM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':'.and. &
          grid(3:4).ne.'16')) lgvalid=.true. 

! Iceland TF
    else if(callsign(1:2).eq.'TF') then
      if((grid(1:2).eq.'HP' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'7').or. &
         (grid(1:2).eq.'IP' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'7')) &
        lgvalid=.true.

! Corsica TK
    else if(callsign(1:2).eq.'TK') then
      if(grid(1:3).eq.'JN4' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4') lgvalid=.true.

! Gabon TR
    else if(callsign(1:2).eq.'TR') then
      if(callsign(1:5).eq.'TR8CA' .or. &
         (grid(1:2).eq.'JJ' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
          grid(3:4).eq.'52' .or. grid(3:4).eq.'62')) .or. &
         (grid(1:2).eq.'JI' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
          grid(3:4).eq.'56'))) lgvalid=.true.

! Tunisia 3V,TS
    else if(callsign(1:2).eq.'TS') then
      if(grid(1:2).eq.'JM' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') &
        lgvalid=.true.

! Guatemala TD,TG
    else if(callsign(1:2).eq.'TG' .or. callsign(1:2).eq.'TD') then
      if(grid(1:2).eq.'EK' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'8') &
        lgvalid=.true.

! Costa Rica, Cocos Island TE,TI
    else if(callsign(1:2).eq.'TI' .or. callsign(1:2).eq.'TE') then
      if((grid(1:2).eq.'EK' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
         (grid(1:2).eq.'EJ' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'EJ65') lgvalid=.true.

! Somalia 6O,T5
! CE3TSK 2026-10-13: there was no rule for these calls, so no square was accepted for them and every weak CQ of
! theirs was deleted. The squares are the ones the country touches (geodata/dxcc_grids.json, Natural Earth).
    else if(callsign(1:2).eq.'T5') then
      if((grid(1:2).eq.'LI' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
          grid(3:4).eq.'19')) .or. &
         (grid(1:2).eq.'LJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
          (grid(3:3).eq.'2' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'7') .or. &
          (grid(3:3).eq.'3' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
          (grid(3:3).eq.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
          (grid(3:3).gt.'0' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
          (grid(3:3).eq.'5' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':'))) .or. &
         (grid(1:2).eq.'LK' .and. ((grid(3:3).eq.'1' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
          (grid(3:3).gt.'2' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
          grid(3:4).eq.'20'))) lgvalid=.true.

! Afghanistan T6,YA
    else if(callsign(1:2).eq.'T6') then
! CE3TSK 2026-10-13: land that this rule refused - Afghanistan
      if(grid(1:4).eq.'ML09' .or. grid(1:4).eq.'ML19' .or. grid(1:4).eq.'ML29' .or. grid(1:4).eq.'ML39' .or. &
         grid(1:4).eq.'MM58') lland=.true.
      if((grid(1:2).eq.'MM' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9').or. &
         (grid(1:2).eq.'ML' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(3:4).eq.'9')) lgvalid=.true.

! San Marino T7
    else if(callsign(1:2).eq.'T7' .and. grid(1:4).eq.'JN63') then
      lgvalid=.true.

! Palau T8
    else if(callsign(1:2).eq.'T8') then
      if(grid(1:2).eq.'PJ' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'9') &
        lgvalid=.true.

! Cuba CL,CM,CO,T4
    else if(callsign(1:2).eq.'T4') then
! CE3TSK 2026-10-13: land that this rule refused - Cuba
      if(grid(1:4).eq.'FK29') lland=.true.
      if((grid(1:2).eq.'EL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'FL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3') .or. grid(1:4).eq.'FK19') lgvalid=.true.

! Martinique, Guadeloupe, St. Martin, St. Barthelemy, Reunion Island  FM,FG,FS,FJ,FR TO
! CE3TSK 2026-10-13: and Mayotte (TO3K in LH27), with the two squares the FH rule has
! CE3TSK 2026-10-13: and French Guiana and St Pierre and Miquelon, with the squares the FY and FP rules have
    else if(callsign(1:2).eq.'TO') then
      if((grid(1:3).eq.'FK9' .and. (grid(4:4).eq.'6' .or. grid(4:4).eq.'5' .or. grid(4:4).eq.'4')) .or. &
         (grid(1:2).eq.'GJ' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'6').or. &
         (grid(1:2).eq.'GN' .and. (grid(3:4).eq.'16' .or. grid(3:4).eq.'17')) .or. &
         (grid(1:3).eq.'FK8' .and. (grid(4:4).eq.'8' .or. grid(4:4).eq.'7')) .or. &
         (grid(1:2).eq.'LH' .and. (grid(3:4).eq.'27' .or. grid(3:4).eq.'26')) .or. &
         (grid(1:2).eq.'LG' .and. (grid(3:4).eq.'79' .or. grid(3:4).eq.'78'))) lgvalid=.true.

! Tuvalu
    else if(callsign(1:2).eq.'T2') then
! CE3TSK 2026-10-13: land that this rule refused - Tuvalu
      if(grid(1:4).eq.'RI90') lland=.true.
      if((grid(1:2).eq.'RI' .and. (grid(3:4).eq.'91' .or. grid(3:4).eq.'92' .or. grid(3:4).eq.'82' .or.grid(3:4).eq.'83'.or. &
         grid(3:4).eq.'84'))  .or. grid(1:4).eq.'RH99') lgvalid=.true.

! Kiribati, Banana Island
    else if(callsign(1:2).eq.'T3') then
      if(grid(1:1).eq.'R') then
        if((grid(1:3).eq.'RJ6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
           (grid(1:3).eq.'RI7' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
           grid(1:4).eq.'RI49' .or. grid(1:4).eq.'RI87' .or. grid(1:4).eq.'RI88') lgvalid=.true.
      else if(grid(1:1).eq.'A') then
        if((grid(1:2).eq.'AI' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'4' .and. &
            grid(4:4).lt.'8') .or. grid(1:4).eq.'AJ94') lgvalid=.true.
      else if(grid(1:1).eq.'B') then
        if(grid(1:4).eq.'BJ03' .or. grid(1:4).eq.'BJ11' .or. grid(1:4).eq.'BJ12' .or. &
           (grid(1:3).eq.'BI2' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'7') .or. &
           grid(1:4).eq.'BH48' .or. grid(1:4).eq.'BH49' .or. grid(1:4).eq.'BI40') lgvalid=.true.
      endif

! Cameroon
    else if(callsign(1:2).eq.'TJ') then
! CE3TSK 2026-10-13: land that this rule refused - Cameroon
      if(grid(1:4).eq.'JJ81' .or. grid(1:4).eq.'JJ82') lland=.true.
      if((grid(1:2).eq.'JJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'JK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4')) &
        lgvalid=.true.

! Central African Republic
    else if(callsign(1:2).eq.'TL') then
      if((grid(1:2).eq.'JJ' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).eq.'0')) lgvalid=.true.

! Republic of the Congo
    else if(callsign(1:2).eq.'TN') then
! CE3TSK 2026-10-13: land that this rule refused - the Republic of the Congo
      if(grid(1:4).eq.'JI64') lland=.true.
      if((grid(1:2).eq.'JI' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'JJ' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4')) &
        lgvalid=.true.

! Chad
    else if(callsign(1:2).eq.'TT') then
! CE3TSK 2026-10-13: land that this rule refused - Chad
      if(grid(1:4).eq.'JJ69' .or. grid(1:4).eq.'JK63' .or. grid(1:4).eq.'JK64' .or. grid(1:4).eq.'JK65') lland=.true.
      if((grid(1:2).eq.'JK' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'JJ' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'KJ09' .or. &
         (grid(1:2).eq.'KK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2') .or. &
         (grid(1:2).eq.'JL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4').or. &
         (grid(1:2).eq.'KL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2')) &
        lgvalid=.true.

! Cote d'Ivoire:
    else if(callsign(1:2).eq.'TU') then
      if((grid(1:2).eq.'IJ' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'IK' .and. (grid(3:4).eq.'60' .or. grid(3:4).eq.'70' .or. grid(3:4).eq.'50'))) lgvalid=.true.

! Benin:
! CE3TSK 2026-10-13: the rule stopped at 2 degrees east and the country goes on to nearly 4: Cotonou and
! Porto-Novo are in JJ16, and a station there lost every weak CQ. The second column of squares is in now.
    else if(callsign(1:2).eq.'TY') then
      if((grid(1:2).eq.'JJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'JK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3')) &
        lgvalid=.true.

! Mali:
    else if(callsign(1:2).eq.'TZ') then
      if((grid(1:2).eq.'IK' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'JK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'IL' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'JL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2')) &
        lgvalid=.true.

! Chesterfield, French Polynesia, Austral, Marquesas, Clipperton Islands FK,TX
    else if(callsign(1:2).eq.'TX') then
      if((grid(1:2).eq.'QH' .and. (grid(3:4).eq.'90' .or. grid(3:4).eq.'91')) .or. grid(1:4).eq.'QG98' .or. &
         (grid(1:2).eq.'BH' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'6').or. &
         (grid(1:2).eq.'BG' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':').or. &
         (grid(1:3).eq.'BI9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
         (grid(1:2).eq.'CI' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01')) .or. &
         grid(1:4).eq.'CH09' .or. grid(1:4).eq.'DK50') lgvalid=.true.

! Wallis & Futuna Islands FW,TW
    else if(callsign(1:2).eq.'TW') then
      if(grid(1:2).eq.'AH' .and. (grid(3:4).eq.'05' .or. grid(3:4).eq.'16')) lgvalid=.true.

! France F,FA..FF,FI,FL,FN,FQ,FU,FV,FX,FZ,HW..HY,TH,TM,TP,TQ,TV
    else if(callsign(2:2).eq.'H' .or. callsign(2:2).eq.'M' .or. callsign(2:2).eq.'P' .or. callsign(2:2).eq.'Q' .or. &
            callsign(2:2).eq.'V') then
! CE3TSK 2026-10-13: land that this rule refused - France (Corsica)
      if(grid(1:4).eq.'JN32' .or. grid(1:4).eq.'JN41' .or. grid(1:4).eq.'JN42' .or. grid(1:4).eq.'JN43') lland=.true.
! CE3TSK 2026-10-13: Antarctica - France's stations for every call of the rule: Dumont d'Urville QC03 and Cap Prud'homme
! PC93, Concordia PB14, D85 PB79
      if(grid(1:4).eq.'QC03' .or. grid(1:4).eq.'PC93' .or. grid(1:4).eq.'PB14' .or. &
         grid(1:4).eq.'PB79') lgvalid=.true.
      if((grid(1:2).eq.'IN' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
         (grid(1:3).eq.'IN7' .and. (grid(4:4).eq.'7' .or. grid(4:4).eq.'8')) .or. &
         (grid(1:2).eq.'JN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
         (grid(1:3).eq.'JN4' .and. (grid(4:4).eq.'8' .or. grid(4:4).eq.'9')) .or. &
         grid(1:4).eq.'JN02'  .or. grid(1:4).eq.'JN12' .or. grid(1:4).eq.'IN92' .or. &
         (grid(1:2).eq.'JO' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'10' .or. grid(3:4).eq.'20' .or. grid(3:4).eq.'11'))) &
        lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'IN' .and. (grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) lgvalid=.false.
endif

! not valid callsigns
    else if(callsign(1:2).eq.'T1' .or. callsign(1:2).eq.'T0') then
      lwrongcall=.true.; return

    endif ! 'T'

  else if(callsign(1:1).eq.'U') then

! Russia R, U0..U9, UA..UI
    if((callsign(2:2).gt.'@' .and. callsign(2:2).lt.'J') .or. &
          (callsign(2:2).gt.'/' .and. callsign(2:2).lt.':')) then
! CE3TSK 2026-10-13: Antarctica - Russia's stations: Bellingshausen GC07, Novolazarevskaya JB59, Progress MC80, Mirny
! NC63, Vostok OB31, Molodyozhnaya LC22, Druzhnaya IV MC60, Soyuz MB49, Bunger Oasis OC03, Leningradskaya QC90, Russkaya
! CB15
    if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'JB59' .or. grid(1:4).eq.'MC80' .or. grid(1:4).eq.'NC63' .or. &
       grid(1:4).eq.'OB31' .or. grid(1:4).eq.'LC22' .or. grid(1:4).eq.'MC60' .or. grid(1:4).eq.'MB49' .or. &
       grid(1:4).eq.'OC03' .or. grid(1:4).eq.'QC90' .or. grid(1:4).eq.'CB15') lgvalid=.true.
    if((grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.':').or. &
       (grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'6'))) .or. &
        grid(1:2).eq.'LO' .or. grid(1:2).eq.'MO' .or. grid(1:2).eq.'NO' .or. &
       (grid(1:2).eq.'LN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'KN' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'KP' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. grid(3:4).eq.'30')) .or. grid(1:2).eq.'NP' &
       .or. grid(1:2).eq.'OO' .or. grid(1:2).eq.'PO' .or. grid(1:2).eq.'LP' .or. grid(1:2).eq.'MP' .or. &
       (grid(1:2).eq.'PN' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
        grid(3:4).eq.'39' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'52' .or. grid(3:4).eq.'62')) .or.grid(1:4).eq.'MQ60'.or. &
       (grid(1:2).eq.'QN' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
        grid(3:4).eq.'28' .or. grid(3:4).eq.'29')) .or. &
       (grid(1:2).eq.'QO' .and. (grid(3:3).eq.'0' .or. grid(3:3).eq.'1' .or. grid(3:3).eq.'8' .or. grid(3:3).eq.'9' .or. &
        grid(3:4).eq.'49' .or. grid(3:4).eq.'59')) .or. grid(1:4).eq.'JO94' .or. &
       grid(1:2).eq.'OP' .or. grid(1:2).eq.'PP' .or. grid(1:4).eq.'JB59' .or. grid(1:4).eq.'NQ03' .or. &
       (grid(1:2).eq.'RP' .and. (grid(3:4).eq.'59' .or. grid(3:4).eq.'84'))) lgvalid=.true.
! CE3TSK 2026-10-13: Chukotka east of 180 degrees (Provideniya is AP34) had no square; the ones the country
! touches there are accepted now (geodata/dxcc_grids.json)
    if(grid(1:2).eq.'AP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. &
       ((grid(3:3).eq.'2' .or. grid(3:3).eq.'3') .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
       ((grid(3:3).eq.'4' .or. grid(3:3).eq.'5') .and. (grid(4:4).eq.'5' .or. grid(4:4).eq.'6')))) lgvalid=.true.

if(lgvalid) then
    if(grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
       grid(3:4).eq.'35' .or. grid(3:4).eq.'50' .or. grid(3:4).eq.'60' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'61')) &
      then; lgvalid=.false.
    else if(grid(1:2).eq.'LO' .and. (grid(3:4).eq.'50' .or. grid(3:4).eq.'60')) then; lgvalid=.false.
    else if(grid(1:2).eq.'LN' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).eq.'1') .or.  &
            grid(3:4).eq.'43' .or. grid(3:4).eq.'44' .or. grid(3:4).eq.'48')) then; lgvalid=.false.
    else if(grid(1:2).eq.'MO' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'9'.and.grid(4:4).gt.'/'.and.grid(4:4).lt.'3').or. &
            (grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).eq.'3'))) then; lgvalid=.false.
! CE3TSK 2026-10-12: KN77, KN87 and KN88 are no longer in this exclusion. Stations with these calls are on the
! air from the three squares - 61 of them in sixteen months of one station's decodes - and each lost every weak
! CQ. The rest of the block stays excluded: nothing was heard from it.
    else if(grid(1:2).eq.'KN' .and. (grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. ((grid(4:4).gt.'6' .and. &
            grid(4:4).lt.':') .or. grid(4:4).eq.'3')) .and. grid(3:4).ne.'77' .and. grid(3:4).ne.'87' .and. &
            grid(3:4).ne.'88') then; lgvalid=.false.
    else if(grid(1:2).eq.'KP' .and. (grid(3:4).eq.'42' .or. grid(3:4).eq.'43' .or. grid(3:4).eq.'99')) then; lgvalid=.false.
    else if(grid(1:2).eq.'OO' .and. grid(3:4).eq.'00') then; lgvalid=.false.
    else if(grid(1:2).eq.'PO' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).eq.'0') .or.  &
            grid(3:4).eq.'11' .or. grid(3:4).eq.'12' .or. grid(3:4).eq.'21')) then; lgvalid=.false.
    else if(grid(1:2).eq.'LP' .and. grid(4:4).eq.'9' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'9' .and. &
            grid(3:3).ne.'4' .and. grid(3:3).ne.'5') then; lgvalid=.false.
    else if(grid(1:2).eq.'PN' .and. (grid(3:4).eq.'56' .or. grid(3:4).eq.'83' .or. grid(3:4).eq.'93' .or. &
            grid(3:4).eq.'94' .or. grid(3:4).eq.'95')) then; lgvalid=.false.
    endif
endif
! CE3TSK 2026-10-13: U5 is Ukraine's prefix (cty.dat), and this rule takes every U with a digit as Russian: a U5
! call was accepted in 18 of the 55 squares Ukraine's rule has, and not in Kyiv. Those squares, as that rule has them,
! are accepted for it as well, with Ukraine's station in Antarctica (Vernadsky FC74); nothing is taken away
    if(callsign(2:2).eq.'5' .and. .not.lgvalid) then
      if((grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
          grid(3:4).eq.'52' .or. grid(3:4).eq.'62')) .or. &
         (grid(1:2).eq.'KN' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':')) &
        lgvalid=.true.
      if(grid(1:2).eq.'KO' .and. (grid(3:4).eq.'81' .or. grid(3:4).eq.'91')) then; lgvalid=.false.
      else if(grid(1:2).eq.'KN' .and. ((grid(3:3).gt.'0'.and.grid(3:3).lt.'4'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.'7').or. &
          grid(3:4).eq.'95' .or. grid(3:4).eq.'96')) then; lgvalid=.false.
      endif
      if(grid(1:4).eq.'FC74') lgvalid=.true.
    endif

! Ukraine EM..EO,UR..UZ
    else if(callsign(2:2).gt.'Q' .and. callsign(2:2).lt.'[') then
! CE3TSK 2026-10-13: land that this rule refused - Ukraine
      if(grid(1:4).eq.'KO72' .or. grid(1:4).eq.'LN08' .or. grid(1:4).eq.'LN09') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Ukraine's station Vernadsky FC74
      if(grid(1:4).eq.'FC74') lgvalid=.true.
      if((grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
          grid(3:4).eq.'52' .or. grid(3:4).eq.'62')) .or. &
         (grid(1:2).eq.'KN' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':')) &
        lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'KO' .and. (grid(3:4).eq.'81' .or. grid(3:4).eq.'91')) then; lgvalid=.false.
      else if(grid(1:2).eq.'KN' .and. ((grid(3:3).gt.'0'.and.grid(3:3).lt.'4'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.'7').or. &
          grid(3:4).eq.'95' .or. grid(3:4).eq.'96')) then; lgvalid=.false.
      endif
endif

! Kazakhstan UN..UQ
! CE3TSK 2026-10-13: of the field LN only the rows from 46 degrees north were accepted, which leaves out the
! Mangystau region (Aktau is LN53) and the land between the Caspian and the Aral Sea. The squares the country
! touches there are accepted now (geodata/dxcc_grids.json).
    else if(callsign(2:2).gt.'M' .and. callsign(2:2).lt.'R') then
      if((grid(1:2).eq.'MN' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
         (grid(1:2).eq.'LN' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'6' .and. &
           grid(3:4).ne.'51') .or. grid(3:4).eq.'44' .or. grid(3:4).eq.'85' .or. grid(3:4).eq.'94' .or. grid(3:4).eq.'95')) .or. &
         (grid(1:2).eq.'MO' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
         (grid(1:2).eq.'LN' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'LO' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
         (grid(1:2).eq.'NN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'NO' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01' .or.grid(3:4).eq.'10'.or.grid(3:4).eq.'20')).or. &
         (grid(1:2).eq.'MO' .and. (grid(3:4).eq.'45' .or. grid(3:4).eq.'55')) .or. &
         (grid(1:2).eq.'MN' .and. ((grid(4:4).eq.'1' .and. (grid(3:3).eq.'3' .or.grid(3:3).eq.'4'.or.grid(3:3).eq.'5')).or. &
          grid(3:4).eq.'40')) .or. &
         (grid(1:3).eq.'NN0' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5')) lgvalid=.true.

! Uzbekistan UJ..UM
! CE3TSK 2026-10-13: there was no rule for these calls, so no square was accepted for them and every weak CQ of
! theirs was deleted. The squares are the ones the country touches (geodata/dxcc_grids.json, Natural Earth).
    else if(callsign(2:2).gt.'I' .and. callsign(2:2).lt.'N') then
      if((grid(1:2).eq.'LN' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'6') .or. &
         (grid(1:2).eq.'MM' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
           (grid(3:3).eq.'2' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
           grid(3:4).eq.'19' .or. grid(3:4).eq.'59')) .or. &
         (grid(1:2).eq.'MN' .and. ((grid(3:3).eq.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
           (grid(3:3).eq.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
           (grid(3:3).eq.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
           (grid(3:3).eq.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
           (grid(3:3).gt.'0' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
           (grid(3:3).eq.'0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5')))) lgvalid=.true.

    endif ! 'U'

  else if(callsign(1:1).eq.'V') then

! Canada CF..CK,CY,CZ,VA..VG,VO,VX,VY,XJ..XO
    if((callsign(2:2).gt.'@' .and. callsign(2:2).lt.'H') .or. callsign(1:2).eq.'VO' .or. callsign(1:2).eq.'VX' .or. &
       callsign(1:2).eq.'VY') then
      if((grid(1:2).eq.'CN' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'FN' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
         (grid(1:2).eq.'EN' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. grid(1:2).eq.'CO' .or. &
         (grid(1:2).eq.'GN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         grid(1:2).eq.'FO' .or. grid(1:2).eq.'EO' .or. grid(1:2).eq.'DO' .or. &
         (grid(1:2).eq.'GO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'DN'.and. grid(4:4).gt.'8' .and. grid(4:4).lt.':') .or. &
         grid(1:2).eq.'CP' .or. grid(1:2).eq.'DP' .or. grid(1:2).eq.'EP' .or. &
         (grid(1:2).eq.'FP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8').or. &
          grid(3:4).eq.'96')) .or. &
         (grid(1:2).eq.'EQ' .and. (grid(3:4).eq.'15' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'24' .or.grid(3:4).eq.'71'.or. &
          grid(3:4).eq.'73' .or. grid(3:4).eq.'79' .or. grid(3:4).eq.'84' .or. grid(3:4).eq.'86' .or.grid(3:4).eq.'96')).or. &
         (grid(1:2).eq.'FR' .and. (grid(3:4).eq.'71' .or. grid(3:4).eq.'82')) .or. &
         (grid(1:2).eq.'FQ' .and. (grid(3:4).eq.'50' .or. grid(3:4).eq.'12')) .or. grid(1:4).eq.'GN03' .or. &
         grid(1:4).eq.'FN93' .or. grid(1:4).eq.'ER60' .or. grid(1:4).eq.'DQ10' .or. grid(1:4).eq.'CQ71') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'EO' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'7' .and.grid(4:4).lt.':').or. &
         (grid(4:4).eq.'7' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'9') .or. grid(3:4).eq.'76' .or. grid(3:4).eq.'86')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'CO' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. &
              grid(4:4).lt.'8').or. grid(3:4).eq.'08' .or. grid(3:4).eq.'18' .or. grid(3:4).eq.'30'.or.grid(3:4).eq.'31'.or. &
              grid(3:4).eq.'40')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EP' .and. ((grid(4:4).eq.'0' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':') .or. &
              (grid(4:4).eq.'1' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'9') .or. &
              (grid(4:4).eq.'2' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'8'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'CN' .and. (grid(3:4).eq.'68' .or. grid(3:4).eq.'98')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'4' .and. &
              grid(4:4).lt.'8') .or. grid(3:4).eq.'04')) then; lgvalid=.false.
      endif
endif

! Australia AX,VH..VN,VZ
    else if((callsign(2:2).gt.'G' .and. callsign(2:2).lt.'O') .or. callsign(1:2).eq.'VZ') then
! CE3TSK 2026-10-13: land that this rule refused - Australia (Macquarie Island with them)
      if(grid(1:4).eq.'OH90' .or. grid(1:4).eq.'QD95' .or. grid(1:4).eq.'QF98' .or. grid(1:4).eq.'QI10') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Australia's stations for every call of the rule: Casey OC53, Davis MC81 (and MC91, 1
! km east of it), Mawson MC12, Law MC80, Edgeworth David OC03, the ice-core camp at Dome C PB14
      if(grid(1:4).eq.'OC53' .or. grid(1:4).eq.'MC81' .or. grid(1:4).eq.'MC91' .or. grid(1:4).eq.'MC12' .or. &
         grid(1:4).eq.'MC80' .or. grid(1:4).eq.'OC03' .or. grid(1:4).eq.'PB14') lgvalid=.true.
      if((grid(1:2).eq.'QG' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7') .or. &
         (grid(1:2).eq.'QF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7') .or. &
         (grid(1:2).eq.'PF' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
         (grid(1:2).eq.'OF' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'QE' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'19')) .or. &
         (grid(1:2).eq.'QH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. & 
         (grid(1:2).eq.'OG' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':') .or. & 
         grid(1:2).eq.'PG' .or. grid(1:2).eq.'PH' .or. &
! Heard Island, Macquarie Island
! CE3TSK 2026-10-13: by the digit, for every prefix of this rule (it asked for VK: a VI0 or VI9 call had none)
         (callsign(3:3).eq.'0' .and. &
          ((grid(1:2).eq.'MD' .and. (grid(3:4).eq.'66' .or. grid(3:4).eq.'67')) .or. &
! CE3TSK 2026-10-13: and Casey, Davis and Mawson stations, Antarctica (OC53, MC81, MC12)
           (grid(1:2).eq.'QD' .and. (grid(3:4).eq.'95' .or. grid(3:4).eq.'94')) .or. grid(1:4).eq.'OC53' .or. &
           grid(1:4).eq.'MC81' .or. grid(1:4).eq.'MC12')) .or. &
! Cocos (Keeling) Islands, Lord Howe Island, Mellish Reef, Norfolk Island, Willis Island VK9
         (callsign(3:3).eq.'9' .and. &
          ((grid(1:2).eq.'NH' .and. (grid(3:4).eq.'87' .or. grid(3:4).eq.'88')) .or. &
           grid(1:4).eq.'QF98' .or. grid(1:4).eq.'OH29' .or. &
           (grid(1:2).eq.'RG' .and. (grid(3:4).eq.'30' .or. grid(3:4).eq.'31')) .or. &
           (grid(1:2).eq.'QH' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'1' .and. &
            grid(4:4).lt.'5')))) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'QF' .and. grid(3:3).eq.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7') then; lgvalid=.false.
      else if(grid(1:2).eq.'PF' .and. ((grid(3:3).gt.'/'.and.grid(3:3).lt.'8'.and.grid(4:4).gt.'1'.and.grid(4:4).lt.'5').or. &
              (grid(4:4).eq.'5' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'7') .or. &
              (grid(4:4).eq.'6' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'7') .or. grid(3:4).eq.'57')) then; lgvalid=.false.
      else if(grid(1:4).eq.'OF74') then; lgvalid=.false.
      else if(grid(1:2).eq.'QH' .and. ((grid(3:3).eq.'3' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. grid(3:4).eq.'09')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OG' .and. (grid(3:4).eq.'60' .or. grid(3:4).eq.'69')) then; lgvalid=.false.
      else if(grid(1:2).eq.'PH' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':' .and. &
              grid(4:4).ne.'5' .and. grid(4:4).ne.'6') .or. &
              (grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
              (grid(3:3).eq.'9' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'59' .or. grid(3:4).eq.'79' .or. grid(3:4).eq.'89')) then; lgvalid=.false.
      endif
endif

! India 8T..8Y,AT..AW,VT..VW
    else if(callsign(2:2).gt.'S' .and. callsign(2:2).lt.'X') then
! CE3TSK 2026-10-13: Antarctica - India's stations: Maitri JB59, Bharati MC80
        if(grid(1:4).eq.'JB59' .or. grid(1:4).eq.'MC80') lgvalid=.true.
        if((grid(1:2).eq.'MJ' .and. (grid(3:4).eq.'99' .or. grid(3:4).eq.'89' .or. grid(3:4).eq.'88' .or. &
! CE3TSK 2026-10-13: MJ98 (Thoothukudi) and, below, MM85 and MM95 (Ladakh north of 35 degrees): India's, and refused
            grid(3:4).eq.'68' .or. grid(3:4).eq.'98')).or. &
           (grid(1:2).eq.'MK' .and. ((grid(3:3).gt.'5'.and.grid(3:3).lt.':').or.grid(3:4).eq.'51'.or.grid(3:4).eq.'52')).or. &
           (grid(1:2).eq.'NK' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.'6') .or. &
            (grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or.grid(3:4).eq.'39')).or. &
           (grid(1:2).eq.'ML' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
           (grid(1:2).eq.'NL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
! CE3TSK 2026-10-13: the Andaman and Nicobar Islands (VU4) had no square; theirs are accepted now
           (grid(1:3).eq.'NK6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. grid(1:4).eq.'NK73' .or. &
           (grid(1:3).eq.'NJ6' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
! CE3TSK 2026-10-13: there was nothing for the north-east (Assam and its neighbours, 90 to 98 degrees east);
! the squares the country touches there are accepted now (geodata/dxcc_grids.json, Natural Earth)
           (grid(1:2).eq.'NL' .and. ((grid(3:3).eq.'5' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'8') .or. &
            (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'9') .or. &
            (grid(3:3).eq.'7' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
            (grid(3:3).eq.'8' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':'))) .or. &
           (grid(1:2).eq.'MM' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
           grid(1:4).eq.'NM00' .or. grid(1:4).eq.'MM85' .or. grid(1:4).eq.'MM95') lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'MK' .and. grid(3:3).eq.'6' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.'5') then; lgvalid=.false.
        else if(grid(1:2).eq.'NK' .and. (grid(3:4).eq.'26' .or. grid(3:4).eq.'27')) then; lgvalid=.false.
! CE3TSK 2026-10-13: this exclusion took rows 0, 5, 8 and 9 out of EVERY column of ML east of 70 degrees -
! Delhi is ML88, Jhansi ML95, Bikaner ML68. Only ML58 and ML59 are left in it.
! CE3TSK 2026-10-13: ML49 joins this exclusion (Pakistan, 115 km beyond the border) and ML58 leaves it (it holds
! 276 km2 of India). The line that stood here for NL excluded nothing but NL40: it asked for a row digit that
! is 9, or 8, and below 5. The squares north of the Himalaya that hold nothing of India are named now - NL19,
! NL28, NL29, NL39, NL49 (Nepal, Tibet; 54 to 165 km). NL09 and NL48 have India in them and stay; so do MK51
! and MK52 (Lakshadweep's outer reefs) and the squares the border comes within 25 km of
        else if(grid(1:2).eq.'ML' .and. (grid(3:4).eq.'59' .or. grid(3:4).eq.'49')) then; lgvalid=.false.
        else if(grid(1:2).eq.'NL' .and. (grid(3:4).eq.'19' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'29' .or. &
                grid(3:4).eq.'39' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'40')) then; lgvalid=.false.
        else if(grid(1:2).eq.'MM' .and. (grid(3:4).eq.'61' .or. grid(3:4).eq.'62')) then; lgvalid=.false.
        endif
endif

! Hong Kong VR
    else if(callsign(1:2).eq.'VR'.and. (grid(1:2).eq.'OL' .and. (grid(3:4).eq.'72' .or. grid(3:4).eq.'62'))) then
      lgvalid=.true.

! Antigua & Barbuda V2
    else if(callsign(1:2).eq.'V2') then
      if(grid(1:2).eq.'FK' .and. (grid(3:4).eq.'86' .or. grid(3:4).eq.'97')) lgvalid=.true.

! Belize V3
    else if(callsign(1:2).eq.'V3') then
      if((grid(1:2).eq.'EK' .and. grid(3:3).eq.'5' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. & 
         (grid(1:2).eq.'EK' .and. grid(3:3).eq.'6' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.'9')) lgvalid=.true.

! St. Kitts & Nevis V4
    else if(callsign(1:2).eq.'V4' .and. grid(1:4).eq.'FK87') then
      lgvalid=.true.

! Namibia V5
    else if(callsign(1:2).eq.'V5') then
! CE3TSK 2026-10-13: land that this rule refused - Namibia
      if(grid(1:4).eq.'JH63') lland=.true.
      if((grid(1:2).eq.'JG' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'JH' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
         (grid(1:2).eq.'KG' .and. (grid(3:4).eq.'08' .or. grid(3:4).eq.'09')).or. &
         (grid(1:2).eq.'KH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3')) &
        lgvalid=.true.

! Micronesia V6
    else if(callsign(1:2).eq.'V6') then
! CE3TSK 2026-10-13: land that this rule refused - Micronesia
      if(grid(1:4).eq.'QJ70') lland=.true.
      if((grid(1:2).eq.'PJ' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'QJ' .and. ((grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. grid(3:4).eq.'71' .or. &
          grid(3:4).eq.'73')) .or. &
         (grid(1:2).eq.'RJ' .and. (grid(3:4).eq.'06' .or. grid(3:4).eq.'15')) .or. grid(1:4).eq.'PK90') lgvalid=.true.

! Marshall Islands V7
    else if(callsign(1:2).eq.'V7') then
      if((grid(1:2).eq.'RJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'RK' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/' .and. &
          grid(4:4).lt.'3') .or. grid(3:4).eq.'44' .or. grid(3:4).eq.'39'))) lgvalid=.true.

! Brunei Darussalam V8
    else if(callsign(1:2).eq.'V8') then
      if(grid(1:2).eq.'OJ' .and. (grid(3:4).eq.'74' .or. grid(3:4).eq.'75')) lgvalid=.true.

! Bermuda VP9
    else if(callsign(1:3).eq.'VP9' .and. grid(1:4).eq.'FM72') then
      lgvalid=.true.

! Chagos Islands VQ9
    else if(callsign(1:3).eq.'VQ9') then
      if(grid(1:2).eq.'MI' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5') &
        lgvalid=.true.

! South Africa H5,S4,S8,V9,ZR,ZS,ZT,ZU
    else if(callsign(1:2).eq.'V9') then
! CE3TSK 2026-10-13: land that this rule refused - South Africa
      if(grid(1:4).eq.'JG92' .or. grid(1:4).eq.'JG93' .or. grid(1:4).eq.'JG94' .or. grid(1:4).eq.'JG95') lland=.true.
! CE3TSK 2026-10-13: Antarctica - South Africa's station SANAE IV IB88, for every call of the rule
      if(grid(1:4).eq.'IB88') lgvalid=.true.
      if((grid(1:2).eq.'KG' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8').or. &
          (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4'))) .or. &
        (grid(1:2).eq.'KF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. &
        (grid(1:2).eq.'JF' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':' .and. &
         grid(3:4).ne.'85') .or. &
        (grid(1:2).eq.'JG' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
        grid(1:4).eq.'KE83' .or. grid(1:4).eq.'KE93') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'KF' .and. ((grid(4:4).eq.'5' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'6') .or. &
         (grid(4:4).eq.'6' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'6') .or. grid(3:4).eq.'57')) lgvalid=.false.
endif

! Anguilla, Montserrat, British Virgin Islands VP2
    else if(callsign(1:3).eq.'VP2' .and. &
            (grid(1:2).eq.'FK' .and. (grid(3:4).eq.'88' .or. grid(3:4).eq.'86' .or. grid(3:4).eq.'78'))) then
      lgvalid=.true.

! Turks & Caicos Islands VP5
    else if((callsign(1:3).eq.'VP5' .or. callsign(1:3).eq.'VQ5') .and. &
            (grid(1:2).eq.'FL' .and. (grid(3:4).eq.'31' .or. grid(3:4).eq.'41'))) then
      lgvalid=.true.

! Pitcairn Island, Ducie Island VP6
    else if(callsign(1:3).eq.'VP6' .and. (grid(1:2).eq.'CG' .and. (grid(3:4).eq.'55' .or. grid(3:4).eq.'44' .or. &
            grid(3:4).eq.'46' .or. grid(3:4).eq.'75'))) then
      lgvalid=.true.

! Falkland Islands, South Georgia Island, South Shetland Islands, South Orkney Islands, South Sandwich Islands VP8
! CE3TSK 2026-10-13: of the Falklands only their western end was here (FD97, FD98). GD07, GD08 and GD18 are the
! rest of the two islands - Stanley is in GD18 - and are accepted now.
    else if(callsign(1:3).eq.'VP8') then
! CE3TSK 2026-10-13: Antarctica - the United Kingdom's stations (Rothera FC52, Port Lockroy FC85, Halley HB74 and HB64,
! Signy GC79, Fossil Bluff FB58, Sky Blu FB45), the South Orkney Islands (GC69, GC79) and every square of the South
! Shetland Islands
      if(grid(1:4).eq.'FC52' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'HB74' .or. grid(1:4).eq.'HB64' .or. &
         grid(1:4).eq.'GC79' .or. grid(1:4).eq.'FB58' .or. grid(1:4).eq.'FB45' .or. grid(1:4).eq.'GC69' .or. &
         grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC86' .or. grid(1:4).eq.'FC87' .or. grid(1:4).eq.'FC96' .or. &
         grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC08' .or. grid(1:4).eq.'GC17' .or. grid(1:4).eq.'GC18' .or. &
         grid(1:4).eq.'GC28') lgvalid=.true.
      if((grid(1:2).eq.'FD' .and. grid(3:3).gt.'8' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.'9').or. &
         grid(1:4).eq.'GD86' .or. grid(1:4).eq.'GD07' .or. grid(1:4).eq.'GD08' .or. grid(1:4).eq.'GD18' .or. &
         (grid(1:2).eq.'HD' .and. ((grid(3:4).eq.'15' .or. grid(3:4).eq.'25' .or.grid(3:4).eq.'05' .or.grid(3:4).eq.'16'.or. &
          grid(3:4).eq.'06') .or. (grid(1:3).eq.'HD6' .and.(grid(4:4).gt.'/' .and. grid(4:4).lt.'4')) .or. &
          grid(1:4).eq.'HD53')) .or. &
! CE3TSK 2026-10-13: the South Shetland Islands stand in this rule's heading and had no square
         grid(1:4).eq.'FC97' .or. (grid(1:3).eq.'GC0' .and. (grid(4:4).eq.'7' .or. grid(4:4).eq.'8')) .or. &
         (grid(1:3).eq.'GC1' .and. (grid(4:4).eq.'7' .or. grid(4:4).eq.'8')) .or. grid(1:4).eq.'GC28' .or. &
         grid(1:4).eq.'GC69' .or. grid(1:4).eq.'GC79') lgvalid=.true.

! CE3TSK 2026-10-13: South Georgia and the South Sandwich Islands sign VP0 as well, which had no rule; the
! squares are the ones the VP8 rule has for them
    else if(callsign(1:3).eq.'VP0') then
! CE3TSK 2026-10-13: Antarctica - the United Kingdom's stations (Rothera FC52, Port Lockroy FC85, Halley HB74 and HB64,
! Signy GC79, Fossil Bluff FB58, Sky Blu FB45), the South Orkney Islands (GC69, GC79) and every square of the South
! Shetland Islands
      if(grid(1:4).eq.'FC52' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'HB74' .or. grid(1:4).eq.'HB64' .or. &
         grid(1:4).eq.'GC79' .or. grid(1:4).eq.'FB58' .or. grid(1:4).eq.'FB45' .or. grid(1:4).eq.'GC69' .or. &
         grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC86' .or. grid(1:4).eq.'FC87' .or. grid(1:4).eq.'FC96' .or. &
         grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC08' .or. grid(1:4).eq.'GC17' .or. grid(1:4).eq.'GC18' .or. &
         grid(1:4).eq.'GC28') lgvalid=.true.
      if(grid(1:4).eq.'GD86' .or. &
         (grid(1:2).eq.'HD' .and. ((grid(3:4).eq.'15' .or. grid(3:4).eq.'25' .or.grid(3:4).eq.'05' .or.grid(3:4).eq.'16'.or. &
          grid(3:4).eq.'06') .or. (grid(1:3).eq.'HD6' .and.(grid(4:4).gt.'/' .and. grid(4:4).lt.'4')) .or. &
          grid(1:4).eq.'HD53'))) lgvalid=.true.

! not valid callsigns
    else if(callsign(1:2).eq.'V1' .or. callsign(1:2).eq.'V0') then
      lwrongcall=.true.; return

    endif  ! 'V'

! USA AA..AL,K,N,W
  else if(callsign(1:1).eq.'W') then
    if(callsign(2:2).gt.'/' .and. callsign(2:2).lt.':' .and. callsign(3:3).gt.'/' .and. callsign(3:3).lt.':') return ! checkcall
! CE3TSK 2026-10-13: land that this rule refused - the United States (the south tip of Big Bend, Texas); Hawaii (the
! Northwestern Hawaiian Islands); Alaska (its north coast - Utqiagvik BQ11, Prudhoe Bay BQ50)
    if(grid(1:4).eq.'AL63' .or. grid(1:4).eq.'AL65' .or. grid(1:4).eq.'AQ80' .or. grid(1:4).eq.'AQ90' .or. &
       grid(1:4).eq.'BQ00' .or. grid(1:4).eq.'BQ10' .or. grid(1:4).eq.'BQ11' .or. grid(1:4).eq.'BQ20' .or. &
       grid(1:4).eq.'BQ21' .or. grid(1:4).eq.'BQ30' .or. grid(1:4).eq.'BQ40' .or. grid(1:4).eq.'BQ50' .or. &
       grid(1:4).eq.'BQ60' .or. grid(1:4).eq.'BQ70' .or. grid(1:4).eq.'BQ80' .or. grid(1:4).eq.'DL88') lland=.true.
! CE3TSK 2026-10-13: Antarctica - the stations of the United States: the South Pole (JA00, as its callbook entry has it,
! and PA90 with the squares either side - the station stands 300 m from the Pole at 139 degrees east), McMurdo RB32 and
! the Sound west of it RB22 (the icebreaker of the yearly resupply: KK7YXD was heard from there), Marble Point RB12, the
! Allan Hills camp QB93, Palmer FC75 (and FC85, 2 km east of it: KC4AAC has sent both), Copacabana GC07, Cape Shirreff
! FC97, Siple Dome BA58, WAIS Divide DB30 and Byrd DA09 (and DB40, DB00, 2 km from them)
    if(grid(1:4).eq.'JA00' .or. grid(1:4).eq.'PA80' .or. grid(1:4).eq.'PA90' .or. grid(1:4).eq.'QA00' .or. &
       grid(1:4).eq.'RB32' .or. grid(1:4).eq.'RB22' .or. grid(1:4).eq.'RB12' .or. grid(1:4).eq.'QB93' .or. &
       grid(1:4).eq.'FC75' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'GC07' .or. grid(1:4).eq.'FC97' .or. &
       grid(1:4).eq.'BA58' .or. grid(1:4).eq.'DB30' .or. grid(1:4).eq.'DB40' .or. grid(1:4).eq.'DA09' .or. &
       grid(1:4).eq.'DB00') lgvalid=.true.
    if((grid(1:2).eq.'FM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'EL' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'DL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
       grid(1:2).eq.'EM' .or. grid(1:2).eq.'DM' .or. grid(1:2).eq.'EN' .or. &
       (grid(1:2).eq.'CM' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'FN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') .or. &
       (grid(1:2).eq.'DN' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9') .or. &
       (grid(1:2).eq.'CN' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9') .or. &
       (grid(1:2).eq.'CO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'FK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.'9') .or. &
       grid(1:2).eq.'BP' .or. &
       (grid(1:2).eq.'BO' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'AP' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
       grid(1:4).eq.'FK28' .or. &
       (grid(1:2).eq.'AO' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'BL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
       (grid(1:2).eq.'BK' .and. (grid(3:4).eq.'19' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'29')) .or. &
       (grid(1:2).eq.'AL' .and. (grid(3:4).eq.'91' .or. grid(3:4).eq.'92' .or. grid(3:4).eq.'08' .or. grid(3:4).eq.'18' .or. &
        grid(3:4).eq.'27' .or. grid(3:4).eq.'36' .or. grid(3:4).eq.'45' .or. grid(3:4).eq.'64' .or. &
        grid(3:4).eq.'73' .or. grid(3:4).eq.'93')) .or. &
       (grid(1:3).eq.'QK2' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'AO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3') .or. &
       (grid(1:2).eq.'RO' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3').or. &
        grid(3:4).eq.'63')) .or. &
       grid(1:4).eq.'AP30' .or. grid(1:4).eq.'QK36' .or. grid(1:4).eq.'QL20' .or. grid(1:4).eq.'RK39' .or. &
! CE3TSK 2026-10-13: AH45 and AH55 added - American Samoa (Tutuila, Manu'a); of it only Swains Island (AH48) was here
       grid(1:4).eq.'AH45' .or. grid(1:4).eq.'AH55' .or. &
! CE3TSK 2026-10-13: these had no square - Guantanamo Bay (KG4) FK29, Baker and Howland (KH1) AJ10, Palmyra,
! Kingman Reef and Jarvis (KH5) AJ85, AJ86, AI99; and CM79 holds the coast of California just south of 40 north
       grid(1:4).eq.'FK29' .or. grid(1:4).eq.'AJ10' .or. grid(1:4).eq.'AJ85' .or. grid(1:4).eq.'AJ86' .or. &
! CE3TSK 2026-10-13: and CM93, the Channel Islands of California south of 34 north (Santa Rosa)
       grid(1:4).eq.'AI99' .or. grid(1:4).eq.'CM79' .or. grid(1:4).eq.'CM93' .or. &
       grid(1:4).eq.'AK56' .or. grid(1:4).eq.'AH48' .or. grid(1:4).eq.'JA00' .or. grid(1:4).eq.'RB32' .or. &
       grid(1:4).eq.'CP00') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'FM' .and. (grid(3:4).eq.'12' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'23' .or. grid(3:4).eq.'24')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'EL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9' &
              .and. grid(3:4).ne.'28'.and. grid(3:4).ne.'58') .or. grid(3:4).eq.'04' .or. grid(3:4).eq.'05' .or. &
              grid(3:4).eq.'14')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DL' .and. (grid(3:4).eq.'78' .or. grid(3:4).eq.'88')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DM' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EN' .and. ((grid(3:3).eq.'9' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
             ((grid(3:3).eq.'8' .or. grid(3:3).eq.'7') .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
             (grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).eq.'9' .and. grid(3:3).ne.'2'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'CM' .and. (grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FN' .and. ((grid(4:4).eq.'7' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
              (grid(4:4).eq.'6' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4') .or. &
              (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4') .or. &
              grid(3:4).eq.'04' .or. grid(3:4).eq.'05' .or. grid(3:4).eq.'15' .or. grid(3:4).eq.'52')) then; lgvalid=.false.
      else if(grid(1:2).eq.'CO' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              (grid(3:3).eq.'1' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'7') .or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'24' .or. grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'BO' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':'.and.grid(4:4).gt.'3'.and.grid(4:4).lt.'9').or. &
              (grid(3:3).eq.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              grid(3:4).eq.'14' .or. grid(3:4).eq.'24' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'35')) then; lgvalid=.false.
      else if(grid(1:2).eq.'AP' .and. ((grid(3:3).eq.'4' .and. ((grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
              (grid(4:4).gt.'3' .and. grid(4:4).lt.':'))) .or. &
              (grid(3:3).eq.'5' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
              grid(3:4).eq.'50' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'62' .or. grid(3:4).eq.'63' .or. &
              grid(3:4).eq.'67' .or. grid(3:4).eq.'69' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
      else if(grid(1:2).eq.'AO' .and. ((grid(3:3).eq.'4' .and. ((grid(4:4).gt.'2' .and. grid(4:4).lt.'7') .or. &
              grid(4:4).eq.'8' .or. grid(4:4).eq.'9')) .or. &
              (grid(3:3).eq.'5' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':' .and. grid(4:4).ne.'6') .or. &
              ((grid(3:3).eq.'6' .or. grid(3:3).eq.'7') .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. &
              (grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'4') .or. &
              grid(3:4).eq.'62' .or. grid(3:4).eq.'97' .or. grid(3:4).eq.'86' .or. grid(3:4).eq.'87' .or.grid(3:4).eq.'31')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'BL' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'12' .or. grid(3:4).eq.'21' .or. &
              grid(3:4).eq.'22')) then; lgvalid=.false.
      else if(grid(1:2).eq.'RO' .and. (grid(3:4).eq.'61' .or. grid(3:4).eq.'71')) then; lgvalid=.false.
      endif
endif

  else if(callsign(1:1).eq.'X') then

! Mexico, Revillagigedo 4A..4C, 6D..6J, XA..XI
    if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'J') then
      if((grid(1:2).eq.'DL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.':').or.grid(3:4).eq.'08'.or.grid(3:4).eq.'09')).or. &
         (grid(1:2).eq.'EL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8').or. &
         (grid(1:2).eq.'EK' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'34')) .or. &
         (grid(1:2).eq.'DM' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
         (grid(1:2).eq.'DK' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'DK' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'38'))) &
        lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'DL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2') .or. &
          grid(3:4).eq.'22' .or. grid(3:4).eq.'23' .or. grid(3:4).eq.'32' .or. grid(3:4).eq.'33' .or. grid(3:4).eq.'60')) then
        lgvalid=.false.
      else if(grid(1:2).eq.'EL' .and. ((grid(3:3).gt.'1'.and.grid(3:3).lt.'7'.and.grid(4:4).gt.'2'.and.grid(4:4).lt.'8').or. &
          grid(3:4).eq.'17' .or. grid(3:4).eq.'20' .or. grid(3:4).eq.'21' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'32' .or. &
          grid(3:4).eq.'62')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EK' .and. (grid(3:4).eq.'05' .or.grid(3:4).eq.'65'.or.grid(3:4).eq.'66'.or.grid(3:4).eq.'67'.or. &
        grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DM' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).eq.'2') then; lgvalid=.false.
      else if(grid(1:2).eq.'DK' .and. (grid(3:4).eq.'76' .or. grid(3:4).eq.'77' .or. grid(3:4).eq.'86')) then; lgvalid=.false.
      endif
endif

! Canada CF..CK,CY,CZ,VA..VG,VO,VX,VY,XJ..XO
    else if(callsign(1:1).eq.'X' .and. callsign(2:2).gt.'I' .and. callsign(2:2).lt.'P') then
      if((grid(1:2).eq.'CN' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'FN' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
         (grid(1:2).eq.'EN' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. grid(1:2).eq.'CO' .or. &
         (grid(1:2).eq.'GN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         grid(1:2).eq.'FO' .or. grid(1:2).eq.'EO' .or. grid(1:2).eq.'DO' .or. &
         (grid(1:2).eq.'GO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'DN'.and. grid(4:4).gt.'8' .and. grid(4:4).lt.':') .or. &
         grid(1:2).eq.'CP' .or. grid(1:2).eq.'DP' .or. grid(1:2).eq.'EP' .or. &
         (grid(1:2).eq.'FP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8').or. &
          grid(3:4).eq.'96')) .or. &
         (grid(1:2).eq.'EQ' .and. (grid(3:4).eq.'15' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'24' .or.grid(3:4).eq.'71'.or. &
          grid(3:4).eq.'73' .or. grid(3:4).eq.'79' .or. grid(3:4).eq.'84' .or. grid(3:4).eq.'86' .or.grid(3:4).eq.'96')).or. &
         (grid(1:2).eq.'FR' .and. (grid(3:4).eq.'71' .or. grid(3:4).eq.'82')) .or. &
         (grid(1:2).eq.'FQ' .and. (grid(3:4).eq.'50' .or. grid(3:4).eq.'12')) .or. grid(1:4).eq.'GN03' .or. &
         grid(1:4).eq.'FN93' .or. grid(1:4).eq.'ER60' .or. grid(1:4).eq.'DQ10' .or. grid(1:4).eq.'CQ71') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'EO' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'7' .and.grid(4:4).lt.':').or. &
         (grid(4:4).eq.'7' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'9') .or. grid(3:4).eq.'76' .or. grid(3:4).eq.'86')) &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'CO' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. &
              grid(4:4).lt.'8').or. grid(3:4).eq.'08' .or. grid(3:4).eq.'18' .or. grid(3:4).eq.'30'.or.grid(3:4).eq.'31'.or. &
              grid(3:4).eq.'40')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EP' .and. ((grid(4:4).eq.'0' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':') .or. &
              (grid(4:4).eq.'1' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'9') .or. &
              (grid(4:4).eq.'2' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'8'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'CN' .and. (grid(3:4).eq.'68' .or. grid(3:4).eq.'98')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'4' .and. &
              grid(4:4).lt.'8') .or. grid(3:4).eq.'04')) then; lgvalid=.false.
      endif
endif

! Burkina Faso XT
    else if(callsign(1:2).eq.'XT') then
! CE3TSK 2026-10-13: land that this rule refused - Burkina Faso
      if(grid(1:4).eq.'IK95' .or. grid(1:4).eq.'JK00') lland=.true.
      if((grid(1:2).eq.'IK' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'JK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'IJ' .and. (grid(3:4).eq.'79' .or. grid(3:4).eq.'89'))) &
        lgvalid=.true.

! Cambodia XU
    else if(callsign(1:2).eq.'XU') then
      if(grid(1:2).eq.'OK' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') &
        lgvalid=.true.

! Vietnam 3W,XV
    else if(callsign(1:2).eq.'XV') then
      if((grid(1:2).eq.'OL' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'3').or. &
         (grid(1:2).eq.'OK' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'5') .or. &
         (grid(1:2).eq.'OJ' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'7'.and.grid(4:4).lt.':').or. &
         grid(1:4).eq.'OJ19' .or.  grid(1:4).eq.'OL41' .or.  grid(1:4).eq.'OL23') lgvalid=.true.

! Laos XW
    else if(callsign(1:2).eq.'XW') then
      if((grid(1:2).eq.'OK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'OL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3')) &
        lgvalid=.true.

! Macao XX9
    else if(callsign(1:3).eq.'XX9' .and. grid(1:4).eq.'OL62') then
      lgvalid=.true.

 ! Chile 3G,CA..CE,XQ,XR
    else if(callsign(1:2).eq.'XQ' .or. callsign(1:2).eq.'XR') then
! CE3TSK 2026-10-13: Antarctica - Chile's stations for every call of the rule: Frei, Escudero and Prat GC07, O'Higgins
! GC16, Guillermo Mann FC97, Gonzalez Videla and Yelcho FC85, Carvajal FC52, Union Glacier EB80
    if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC16' .or. grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC85' .or. &
       grid(1:4).eq.'FC52' .or. grid(1:4).eq.'EB80') lgvalid=.true.
! CE3TSK 2026-10-13: FF56 added - the upper Maipo valley, where the border runs east of 70 degrees west
      if((grid(1:2).eq.'FF' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'5') .or. grid(1:4).eq.'FF56' .or. &
! CE3TSK 2026-10-13: and FF55 and FF59, strips of the same kind. And Chile's Antarctic calls (the third character
! is 9) had no square: the South Shetland Islands, and the base on the peninsula across from them (GC16)
         grid(1:4).eq.'FF55' .or. grid(1:4).eq.'FF59' .or. &
! CE3TSK 2026-10-13: and FC86, FC87, FC96 - Smith and Low Islands and the south end of Deception, uninhabited
         (callsign(3:3).eq.'9' .and. (grid(1:4).eq.'FC97' .or. grid(1:4).eq.'GC28' .or. &
          grid(1:4).eq.'FC86' .or. grid(1:4).eq.'FC87' .or. grid(1:4).eq.'FC96' .or. &
          (grid(1:3).eq.'GC0' .and. (grid(4:4).eq.'7' .or. grid(4:4).eq.'8')) .or. &
          (grid(1:3).eq.'GC1' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.'9'))) .or. &
         (grid(1:2).eq.'FG' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'6') .or. &
          (grid(3:3).eq.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'8'))) .or. &
         (grid(1:2).eq.'FE' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5') .or. &
         (grid(1:2).eq.'FH' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
          grid(3:4).eq.'52')) .or. &
         (grid(1:2).eq.'FD' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'53' .or. grid(3:4).eq.'85')) .or. &
         grid(1:4).eq.'EF96' .or. grid(1:4).eq.'FF06' .or. grid(1:4).eq.'EG93' .or. grid(1:4).eq.'FG03' .or. &
         grid(1:4).eq.'DG52' .or. grid(1:4).eq.'DG73') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'FF' .and. grid(3:3).eq.'3' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') then; lgvalid=.false.
      else if(grid(1:2).eq.'FE' .and. (grid(3:4).eq.'40' .or. grid(3:4).eq.'41' .or. grid(3:4).eq.'29')) then; lgvalid=.false.
      endif
endif

! Myanmar XY,XZ
    else if(callsign(1:2).eq.'XY' .or. callsign(1:2).eq.'XZ') then
! CE3TSK 2026-10-13: land that this rule refused - Myanmar (the south - Dawei NK94, Myeik NK92 - and the Coco Islands)
      if(grid(1:4).eq.'NJ99' .or. grid(1:4).eq.'NK64' .or. grid(1:4).eq.'NK75' .or. grid(1:4).eq.'NK80' .or. &
         grid(1:4).eq.'NK81' .or. grid(1:4).eq.'NK82' .or. grid(1:4).eq.'NK83' .or. grid(1:4).eq.'NK84' .or. &
         grid(1:4).eq.'NK85' .or. grid(1:4).eq.'NK90' .or. grid(1:4).eq.'NK91' .or. grid(1:4).eq.'NK92' .or. &
         grid(1:4).eq.'NK93' .or. grid(1:4).eq.'NK94' .or. grid(1:4).eq.'NK95') lland=.true.
      if((grid(1:2).eq.'NK' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
         (grid(1:2).eq.'NL' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9').or. &
         (grid(1:2).eq.'OL' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01'))) &
        lgvalid=.true.

! Greenland OX,XP
    else if(callsign(1:2).eq.'XP') then
      if((grid(1:2).eq.'GP' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'HP' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'HQ90' .or. &
         (grid(1:2).eq.'GQ' .and. (grid(3:4).eq.'30' .or. grid(3:4).eq.'31' .or. grid(3:4).eq.'41' .or.grid(3:4).eq.'22'.or. &
          grid(3:4).eq.'14' .or. grid(3:4).eq.'12')) .or. &
         (grid(1:2).eq.'FQ' .and. (grid(3:4).eq.'76' .or. grid(3:4).eq.'67' .or. grid(3:4).eq.'57' .or.grid(3:4).eq.'56'.or. &
          grid(3:4).eq.'38'))) lgvalid=.true.

! China, Taiwan  B, 3H..3U, XS
    else if(callsign(1:2).eq.'XS') then
! CE3TSK 2026-10-13: land that this rule refused - China
      if(grid(1:4).eq.'MN60' .or. grid(1:4).eq.'MN92' .or. grid(1:4).eq.'MN94' .or. grid(1:4).eq.'OK55' .or. &
         grid(1:4).eq.'OK56' .or. grid(1:4).eq.'OK66' .or. grid(1:4).eq.'ON55' .or. grid(1:4).eq.'ON65') lland=.true.
! CE3TSK 2026-10-13: Antarctica - China's stations: Great Wall GC07, Zhongshan MC80, Kunlun MA89, Taishan MB86, Qinling
! RB15
    if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'MC80' .or. grid(1:4).eq.'MA89' .or. grid(1:4).eq.'MB86' .or. &
       grid(1:4).eq.'RB15') lgvalid=.true.
    if(grid(1:2).eq.'OL' .or. &
       (grid(1:2).eq.'PL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2') .or. grid(1:2).eq.'OM' .or. &
       (grid(1:2).eq.'PM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3') .or. &
       (grid(1:2).eq.'PN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8') .or. &
       (grid(1:2).eq.'PO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
       (grid(1:2).eq.'OK' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'58' .or.grid(3:4).eq.'59')).or. &
       grid(1:2).eq.'ON' .or. grid(1:4).eq.'OO90' .or. grid(1:4).eq.'OO91' .or. grid(1:2).eq.'NN' .or. grid(1:2).eq.'NM'.or. &
       (grid(1:2).eq.'NL' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'MN' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
       (grid(1:2).eq.'MM' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':')) lgvalid=.true.

if(lgvalid) then
       if(grid(1:2).eq.'NL' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.'7'.and. &
          grid(3:4).ne.'83' .and. grid(3:4).ne.'84' .and. grid(3:4).ne.'85') .or. grid(3:4).eq.'77' .or. grid(3:4).eq.'87')) &
        then; lgvalid=.false.
       else if(grid(1:2).eq.'ON' .and.((grid(3:3).gt.'/'.and.grid(3:3).lt.'5'.and.grid(4:4).gt.'2'.and.grid(4:4).lt.':').or. &
               (grid(3:3).gt.'4' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':') .or. &
               grid(3:4).eq.'76' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
       else if(grid(1:2).eq.'PN' .and. ((grid(3:3).eq.'7' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6') .or. &
               (grid(3:3).eq.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5') .or. &
! CE3TSK 2026-10-13: PN52, PN53 and PN54 are no longer in this exclusion - the east of Jilin and Heilongjiang
! (Hunchun, Suifenhe), with stations on the air. PN50 and PN51 stay in it.
               grid(3:4).eq.'40' .or. grid(3:4).eq.'50' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'59' .or. &
               grid(3:4).eq.'69' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
       else if(grid(1:2).eq.'MM' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8') .or. &
               (grid(3:3).eq.'7' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6') .or. &
               (grid(3:3).eq.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5'))) then; lgvalid=.false.
       else if(grid(1:2).eq.'OL' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).eq.'0') .or. &
               grid(3:4).eq.'11' .or. grid(3:4).eq.'21')) then; lgvalid=.false.
       endif
endif

! not valid callsigns X0...X9
    else if(callsign(2:2).gt.'/' .and. callsign(2:2).lt.':') then
      lwrongcall=.true.; return

    endif ! 'X'

  else if(callsign(1:1).eq.'Y') then

! Romania YO..YR
    if(callsign(2:2).gt.'N' .and. callsign(2:2).lt.'S') then
! CE3TSK 2026-10-13: Antarctica - the Law-Racovita station MC80, Romania's with Australia
      if(grid(1:4).eq.'MC80') lgvalid=.true.
      if(grid(1:2).eq.'KN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'9') &
        lgvalid=.true.

! Serbia YU,YT
    else if(callsign(2:2).eq.'U' .or. callsign(2:2).eq.'T') then
      if((grid(1:3).eq.'JN9' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'7') .or. &
         (grid(1:3).eq.'KN0' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'7') .or. &
         (grid(1:3).eq.'KN1' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5')) lgvalid=.true.

! Indonesia  7A..7I, 8A..8I, PK..PO, YB..YH
    else if(callsign(2:2).gt.'A' .and. callsign(2:2).lt.'I') then
! CE3TSK 2026-10-13: land that this rule refused - Indonesia
      if(grid(1:4).eq.'OH99' .or. grid(1:4).eq.'PI34' .or. grid(1:4).eq.'PJ35' .or. grid(1:4).eq.'PJ51' .or. &
         grid(1:4).eq.'PJ70') lland=.true.
      if((grid(1:2).eq.'NJ' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':') .or. &
       (grid(1:2).eq.'NI' .and. grid(3:3).gt.'8' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
        grid(1:2).eq.'OI' .or. grid(1:2).eq.'PI' .or. &
       (grid(1:2).eq.'OJ' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
       (grid(1:2).eq.'PJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
       (grid(1:2).eq.'QI' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'1' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') .or. &
       (grid(1:2).eq.'PH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).eq.'9')) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'PJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and.grid(4:4).gt.'1'.and.grid(4:4).lt.'5').or. &
         grid(3:4).eq.'43' .or. grid(3:4).eq.'44')) then; lgvalid=.false.
      else if(grid(1:2).eq.'NJ' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'/' .and. grid(4:4) .lt.'6').or. &
         (grid(3:3).eq.'7' .and. ((grid(4:4).gt.'5'.and. grid(4:4).lt.':') .or. grid(4:4).eq.'0' .or. grid(4:4).eq.'1'.or. &
          grid(4:4).eq.'3')) .or. (grid(3:3).eq.'8' .and. grid(4:4).gt.'5'.and. grid(4:4).lt.':') .or. &
         grid(3:4).eq.'43' .or. grid(3:4).eq.'44' .or. grid(3:4).eq.'95')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OJ'.and.((grid(3:3).gt.'4'.and.grid(3:3).lt.'7'.and.grid(4:4).gt.'1'.and.grid(4:4).lt.'5').or. &
         (grid(3:3).eq.'1' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5') .or. &
         grid(3:4).eq.'24' .or. grid(3:4).eq.'04')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OI' .and. ((grid(4:4).eq.'1' .and. grid(3:3).gt.'1'.and. grid(3:3).lt.'5') .or. &
        (grid(4:4).eq.'5' .and. grid(3:3).gt.'2'.and. grid(3:3).lt.'6') .or. &
        grid(3:4).eq.'44' .or. grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'PI' .and.(grid(3:4).eq.'61'.or.grid(3:4).eq.'62'.or.grid(3:4).eq.'71'.or.grid(3:4).eq.'83'.or. &
         grid(3:4).eq.'25' .or. grid(3:4).eq.'35' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'44' .or. grid(3:4).eq.'29')) &
         then; lgvalid=.false.
      endif
endif

! Venezuela, Aves Island YV..YY 4M
    else if(callsign(2:2).gt.'U' .and. callsign(2:2).lt.'Z') then
! CE3TSK 2026-10-13: land that this rule refused - Venezuela
      if(grid(1:4).eq.'FJ46' .or. grid(1:4).eq.'FJ56' .or. grid(1:4).eq.'FJ72' .or. grid(1:4).eq.'FJ82' .or. &
         grid(1:4).eq.'GJ08') lland=.true.
      if((grid(1:2).eq.'FJ' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'60' .or. grid(3:4).eq.'70')) .or. &
         (grid(1:2).eq.'FK' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
         grid(1:4).eq.'FK85') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'FJ' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.'7' .and. &
         grid(3:4).ne.'57') .or. grid(3:4).eq.'92' .or. grid(3:4).eq.'93')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FJ' .and. (grid(3:4).eq.'32' .or.grid(3:4).eq.'72'.or.grid(3:4).eq.'82'.or.grid(3:4).eq.'92'.or. &
              grid(3:4).eq.'91')) then; lgvalid=.false.
      endif
endif

! Latvia YL
    else if(callsign(1:2).eq.'YL') then
      if((grid(1:2).eq.'KO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.'8').or. &
        (grid(1:2).eq.'KO' .and. (grid(3:4).eq.'25' .or. grid(3:4).eq.'35' .or. grid(3:4).eq.'46' .or. grid(3:4).eq.'28'))) &
        lgvalid=.true.

! Afghanistan T6,YA
    else if(callsign(1:2).eq.'YA') then
! CE3TSK 2026-10-13: land that this rule refused - Afghanistan
      if(grid(1:4).eq.'ML09' .or. grid(1:4).eq.'ML19' .or. grid(1:4).eq.'ML29' .or. grid(1:4).eq.'ML39') lland=.true.
      if((grid(1:2).eq.'MM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9').or. &
         (grid(1:2).eq.'ML' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(3:4).eq.'9')) lgvalid=.true.

! Iraq HN,YI
    else if(callsign(1:2).eq.'YI') then
      if((grid(1:2).eq.'LM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8').or. &
       (grid(1:2).eq.'KM' .and.(grid(3:4).eq.'92' .or. grid(3:4).eq.'93')) .or. &
       (grid(1:2).eq.'LL' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'5' .and. grid(4:4).eq.'9')) lgvalid=.true.

! Vanuatu YJ
    else if(callsign(1:2).eq.'YJ') then
! CE3TSK 2026-10-13: land that this rule refused - Vanuatu
      if(grid(1:4).eq.'RG57' .or. grid(1:4).eq.'RG67') lland=.true.
      if((grid(1:2).eq.'RH' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'6').or. &
        grid(1:4).eq.'RG49' .or. grid(1:4).eq.'RH50' .or. grid(1:4).eq.'RH36') lgvalid=.true.

! Syria 6C,YK
    else if(callsign(1:2).eq.'YK') then
! CE3TSK 2026-10-13: land that this rule refused - Syria
      if(grid(1:4).eq.'LM03') lland=.true.
      if((grid(1:2).eq.'KM' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'7').or. &
         (grid(1:2).eq.'LM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8')) &
        lgvalid=.true.

! Turkey TA..TC,YM
    else if(callsign(1:2).eq.'YM') then
! CE3TSK 2026-10-13: land that this rule refused - Turkey
      if(grid(1:4).eq.'KM29' .or. grid(1:4).eq.'KM75' .or. grid(1:4).eq.'LN20') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Turkey's station on Horseshoe Island FC62
      if(grid(1:4).eq.'FC62') lgvalid=.true.
      if((grid(1:2).eq.'KM' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'85')) .or. &
         (grid(1:2).eq.'KN' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
          grid(3:4).eq.'20' .or. grid(3:4).eq.'32' .or. grid(3:4).eq.'62' .or. grid(3:4).eq.'72')) .or. &
         (grid(1:2).eq.'LN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
         (grid(1:2).eq.'LM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':'.and. &
          grid(3:4).ne.'16')) lgvalid=.true.

! Nicaragua H6,H7,HT,YN
    else if(callsign(1:2).eq.'YN') then
! CE3TSK 2026-10-13: land that this rule refused - Nicaragua
      if(grid(1:4).eq.'EK85') lland=.true.
      if(grid(1:2).eq.'EK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') &
        lgvalid=.true.

! El Salvador YS,HU
    else if(callsign(1:2).eq.'YS') then
      if(grid(1:2).eq.'EK' .and. (grid(3:4).eq.'53' .or. grid(3:4).eq.'54' .or. grid(3:4).eq.'63' .or. grid(3:4).eq.'43')) &
        lgvalid=.true.

! Fed. Rep. of Germany DA..DR, Y2..Y9
    else if(callsign(1:1).eq.'Y' .and. callsign(2:2).gt.'1' .and. callsign(2:2).lt.':') then
! CE3TSK 2026-10-13: Antarctica - Germany's stations: Neumayer III IB59, Kohnen JB04 (200 m from JB05 and 2 km from
! IB94, IB95: all four), Gondwana RB25, GARS O'Higgins GC16, Dallmann GC07
      if(grid(1:4).eq.'IB59' .or. grid(1:4).eq.'JB04' .or. grid(1:4).eq.'JB05' .or. grid(1:4).eq.'IB94' .or. &
         grid(1:4).eq.'IB95' .or. grid(1:4).eq.'RB25' .or. grid(1:4).eq.'GC16' .or. &
         grid(1:4).eq.'GC07') lgvalid=.true.
      if((grid(1:2).eq.'JO' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
          grid(3:4).eq.'45')) .or. &
         (grid(1:2).eq.'JN' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':').or. &
         grid(1:4).eq.'IB59') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'JO' .and. ((grid(3:3).lt.'2' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5') .or. &
         grid(3:4).eq.'35')) lgvalid=.false.
endif

! not valid callsigns Y1,Y0
    else if(callsign(1:2).eq.'Y1' .or. callsign(1:2).eq.'Y0') then
      lwrongcall=.true.; return

    endif ! 'Y'

  else if(callsign(1:1).eq.'Z') then

! North Macedonia Z3
    if(callsign(1:2).eq.'Z3') then
! CE3TSK 2026-10-13: land that this rule refused - North Macedonia
      if(grid(1:4).eq.'KN12') lland=.true.
      if((grid(1:3).eq.'KN0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
         grid(1:4).eq.'KN11' .or. grid(1:4).eq.'KN11') lgvalid=.true.

! Republic of Kosovo Z6
! CE3TSK 2026-10-13: land that this rule refused - the south end of the country, KN01
    else if(callsign(1:2).eq.'Z6' .and. (grid(1:4).eq.'KN02' .or. grid(1:4).eq.'KN03' .or. grid(1:4).eq.'KN01')) then
      lgvalid=.true.

! New Zealand, Chatham Islands, Kermadec Islands, N.Z. Subantarctic Is. ZK ZL ZM
    else if(callsign(2:2).gt.'J' .and. callsign(2:2).lt.'N') then
! CE3TSK 2026-10-13: Antarctica - Scott Base RB32, for every call of the rule
      if(grid(1:4).eq.'RB32') lgvalid=.true.
      if((grid(1:2).eq.'RF' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'6').or. &
        (grid(1:2).eq.'RE' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
        grid(1:4).eq.'AE16' .or. grid(1:4).eq.'AE15' .or. grid(1:4).eq.'RE31' .or. grid(1:4).eq.'RE90' .or. &
        grid(1:4).eq.'RE92' .or. &
! CE3TSK 2026-10-13: the Kermadec Islands (ZL8) had no square: the rule had AF10, AG09 and AG08 here, which are
! theirs with the field letters exchanged (open sea, and two squares of Fiji). Corrected: Raoul Island is AG10,
! Macauley and Curtis AF09, L'Esperance Rock AF08. And Tokelau: this rule takes every ZK call, so the one for
! ZK3 further down is never reached; its two squares are accepted here. And Scott Base (ZL5, ZM5) in RB32
        grid(1:4).eq.'AG10' .or. grid(1:4).eq.'AF09' .or. grid(1:4).eq.'AF08' .or. &
        (callsign(1:3).eq.'ZK3' .and. (grid(1:4).eq.'AI31' .or. grid(1:4).eq.'AI40')) .or. &
        (callsign(3:3).eq.'5' .and. callsign(2:2).ne.'K' .and. grid(1:4).eq.'RB32') .or. &
        grid(1:4).eq.'RD29' .or. grid(1:4).eq.'RD39' .or. grid(1:4).eq.'RD47') lgvalid=.true.

! South Africa H5,S4,S8,V9,ZR,ZS,ZT,ZU
    else if(callsign(2:2).gt.'Q' .and. callsign(2:2).lt.'V') then
! CE3TSK 2026-10-13: land that this rule refused - South Africa
      if(grid(1:4).eq.'JG92' .or. grid(1:4).eq.'JG93' .or. grid(1:4).eq.'JG94' .or. grid(1:4).eq.'JG95') lland=.true.
! CE3TSK 2026-10-13: Antarctica - South Africa's station SANAE IV IB88, for every call of the rule
    if(grid(1:4).eq.'IB88') lgvalid=.true.
! CE3TSK 2026-10-13: ZS7 at SANAE, Antarctica (IB88) had no square
      if((callsign(1:3).eq.'ZS7' .and. grid(1:4).eq.'IB88') .or. &
        (grid(1:2).eq.'KG' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8').or. &
          (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4'))) .or. &
        (grid(1:2).eq.'KF' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. &
        (grid(1:2).eq.'JF' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':' .and. &
         grid(3:4).ne.'85') .or. &
        (grid(1:2).eq.'JG' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
        grid(1:4).eq.'KE83' .or. grid(1:4).eq.'KE93') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'KF' .and. ((grid(4:4).eq.'5' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'6') .or. &
         (grid(4:4).eq.'6' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'6') .or. grid(3:4).eq.'57')) lgvalid=.false.
endif

! Paraguay ZP
    else if(callsign(1:2).eq.'ZP') then
      if((grid(1:2).eq.'GG' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':').or. &
        (grid(1:2).eq.'FG' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
        grid(1:4).eq.'FH90' .or. grid(1:4).eq.'GH00' .or. grid(1:4).eq.'FG95') lgvalid=.true.

! Albania ZA
    else if(callsign(1:2).eq.'ZA') then
      if((grid(1:3).eq.'JN9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
         (grid(1:3).eq.'KN0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
         grid(1:4).eq.'JM99' .or. grid(1:4).eq.'KM09') lgvalid=.true.

! Gibraltar ZB,ZG
    else if((callsign(1:2).eq.'ZB' .or. callsign(1:2).eq.'ZG') .and. grid(1:4).eq.'IM76') then
      lgvalid=.true.

! UK Base Areas in Cyprus
! CE3TSK 2026-10-13: and KM65 - the Dhekelia area reaches north of 35 degrees (Ayios Nikolaos)
    else if(callsign(1:3).eq.'ZC4' .and. (grid(1:4).eq.'KM64' .or. grid(1:4).eq.'KM65')) then
      lgvalid=.true.

! Zimbabwe Z2
    else if(callsign(1:2).eq.'Z2') then
      if((grid(1:2).eq.'KH' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
        (grid(1:2).eq.'KG' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':')) &
        lgvalid=.true.

! Republic of South Sudan Z8
    else if(callsign(1:2).eq.'Z8') then
      if((grid(1:2).eq.'KJ' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':').or. &
        (grid(1:2).eq.'KK' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3')) &
        lgvalid=.true.

! St. Helena ZD7
    else if(callsign(1:3).eq.'ZD7' .and. (grid(1:4).eq.'IH74' .or. grid(1:4).eq.'IH73')) then
      lgvalid=.true.

! Ascension Island ZD8
    else if(callsign(1:3).eq.'ZD8' .and. grid(1:4).eq.'II22') then
      lgvalid=.true.

! Tristan da Cunha & Gough ZD9
    else if(callsign(1:3).eq.'ZD9' .and. (grid(1:4).eq.'IF32' .or. grid(1:4).eq.'IE49' .or. grid(1:4).eq.'IE59')) then
      lgvalid=.true.

! Cayman Islands ZF
    else if(callsign(1:2).eq.'ZF' .and. (grid(1:4).eq.'EK99' .or. grid(1:4).eq.'FK09')) then
      lgvalid=.true.

! Tokelau Islands ZK3
    else if(callsign(1:3).eq.'ZK3' .and. (grid(1:4).eq.'AI31' .or. grid(1:4).eq.'AI40')) then
      lgvalid=.true.

! Brazil PP..PY, ZV..ZZ
    else if(callsign(2:2).gt.'U' .and. callsign(2:2).lt.'[') then
! CE3TSK 2026-10-13: land that this rule refused - Brazil
      if(grid(1:4).eq.'FI22' .or. grid(1:4).eq.'FI49' .or. grid(1:4).eq.'FJ40' .or. grid(1:4).eq.'FJ52' .or. &
         grid(1:4).eq.'FJ62' .or. grid(1:4).eq.'GJ05') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Brazil's station Comandante Ferraz GC07 and its refuges (Cruls GC07, Goeldi GC28)
      if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC28') lgvalid=.true.
      if((grid(1:2).eq.'GG' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.':') .or. grid(3:4).eq.'09')) .or. &
         (grid(1:2).eq.'HI' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6'.and. &
          grid(3:4).ne.'25') .or. grid(3:4).eq.'06' .or. grid(3:4).eq.'07' .or. grid(3:4).eq.'36')) .or. &
         (grid(1:2).eq.'HH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2') .or. &
         (grid(1:2).eq.'GH' .and. grid(3:4).ne.'01') .or. grid(1:2).eq.'GI' .or. &
         (grid(1:2).eq.'GF' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'6'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'36')) .or. &
         (grid(1:2).eq.'GJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'4').or. &
          grid(3:4).eq.'04' .or. grid(3:4).eq.'44')) .or. &
         (grid(1:2).eq.'FJ' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
          grid(3:4).eq.'95')) .or. &
         (grid(1:2).eq.'FI' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':') .or. &
         (grid(1:2).eq.'FH' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'93' .or. grid(3:4).eq.'94' .or. grid(3:4).eq.'95')) .or. &
         grid(1:4).eq.'HG59' .or. grid(1:4).eq.'HJ50') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'GG' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. &
         grid(4:4).lt.'6' .and. grid(3:4).ne.'64' .and. grid(3:4).ne.'65') .or. &
         (grid(3:3).eq.'1' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'7'))) then; lgvalid=.false.
      else if(grid(1:2).eq.'HH' .and. (grid(3:3).eq.'1' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7')) then; lgvalid=.false.
      else if(grid(1:2).eq.'GI' .and. ((grid(3:3).gt.'7'.and.grid(3:3).lt.':'.and.grid(4:4).gt.'7'.and.grid(4:4).lt.':').or. &
              grid(3:4).eq.'79')) then; lgvalid=.false.
      else if(grid(1:2).eq.'GF' .and. (grid(3:4).eq.'17' .or. grid(3:4).eq.'27')) then; lgvalid=.false.
      else if(grid(1:2).eq.'GJ' .and. (grid(3:4).eq.'13' .or. grid(3:4).eq.'23' .or. grid(3:4).eq.'52' .or. &
              grid(3:4).eq.'53')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FJ' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5') &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'FI' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') &
        then; lgvalid=.false.
      else if(grid(1:2).eq.'FH' .and. ((grid(3:3).gt.'3'.and.grid(3:3).lt.'7'.and.grid(4:4).gt.'5'.and.grid(4:4).lt.'8').or. &
              grid(3:4).eq.'76' .or. grid(3:4).eq.'68')) then; lgvalid=.false.
      endif
endif

! not valid callsigns Z1 Z7 Z9
    else if(callsign(2:2).eq.'1' .or. callsign(2:2).eq.'7' .or. callsign(2:2).eq.'9') then
      lwrongcall=.true.; return

    endif ! 'Z'

  endif ! end of block L..Z

4   if(callsign(1:1).eq.'1') then

! Sov Mil Order of Malta 1A
      if(callsign(1:2).eq.'1A' .and. grid(1:4).eq.'JN61') then
        lgvalid=.true.

! 1B
      else if(callsign(1:2).eq.'1B') then
      if((grid(1:3).eq.'KM6' .or. grid(1:3).eq.'KM7') .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'6') lgvalid=.true.

! Spratly Islands 1S,9M0,BM9S,BN9S,BO9S,BP9S,BQ9S,BU9S,BV9S,BW9S,BX9S
      else if(callsign(1:2).eq.'1S') then
      if((grid(1:2).eq.'OK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
         (grid(1:2).eq.'OJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':')) &
        lgvalid=.true.

! not valid callsigns 1C..1R 1T..1Z
      else if((callsign(2:2).gt.'B' .and. callsign(2:2).lt.'S') .or. (callsign(2:2).gt.'S' .and. callsign(2:2).lt.'[')) then
        lwrongcall=.true.; return

      endif ! '1'

    else if(callsign(1:1).eq.'2') then

! UK GB / 2E,G,M / 2D,GD,GT,MD,MT / 2I,GI,GN,MI,MN / 2J,GH,GJ,MH,MJ / 2A,2M,GM,GS,MA,MM,MS / 2U,GP,GU,MP,MU / 2W,GC,GW,MC,MW
      if(callsign(2:2).eq.'E' .or. callsign(2:2).eq.'A' .or. callsign(2:2).eq.'M' .or. callsign(2:2).eq.'W' .or. &
         callsign(2:2).eq.'I' .or. callsign(2:2).eq.'J' .or. callsign(2:2).eq.'U' .or. callsign(2:2).eq.'D') then
! CE3TSK 2026-10-13: land that this rule refused - Scotland (Foula, Shetland)
        if(grid(1:4).eq.'IP80') lland=.true.
! CE3TSK 2026-10-13: Antarctica - the United Kingdom's stations: Rothera FC52, Port Lockroy FC85, Halley HB74 (and HB64,
! where it stood until 2017 and towards which its ice shelf carries it), Signy GC79, Fossil Bluff FB58, Sky Blu FB45
        if(grid(1:4).eq.'FC52' .or. grid(1:4).eq.'FC85' .or. grid(1:4).eq.'HB74' .or. grid(1:4).eq.'HB64' .or. &
           grid(1:4).eq.'GC79' .or. grid(1:4).eq.'FB58' .or. grid(1:4).eq.'FB45') lgvalid=.true.
        if((grid(1:2).eq.'IO' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':') .or. &
           (grid(1:3).eq.'JO0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
! CE3TSK 2026-10-13: IN79 was missing - the Lizard, the south-west end of Cornwall
           grid(1:4).eq.'IP90' .or. grid(1:4).eq.'IN69' .or. grid(1:4).eq.'IN79' .or. grid(1:4).eq.'IN89' .or. &
           grid(1:4).eq.'IO37') lgvalid=.true.

if(lgvalid .and. grid(1:2).eq.'IO') then
    if((grid(1:3).eq.'IO5' .and. grid(4:4).ne.'4' .and. grid(4:4).ne.'7') .or. &
       (grid(1:3).eq.'IO6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
       grid(3:4).eq.'96' .or. grid(3:4).eq.'98') lgvalid=.false.
endif

! not valid callsigns other 2x
      else
        lwrongcall=.true.; return

      endif ! '2'

    else if(callsign(1:1).eq.'3') then

! Poland 3Z,HF,SN..SR
      if(callsign(1:2).eq.'3Z') then
! CE3TSK 2026-10-13: Antarctica - Poland's stations: Arctowski GC07, Dobrowolski OC03
        if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'OC03') lgvalid=.true.
        if((grid(1:2).eq.'JO' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'KO' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
          grid(3:4).eq.'20')) .or. &
           grid(1:4).eq.'KN09' .or. grid(1:4).eq.'KN19' .or. grid(1:4).eq.'KO20' .or. grid(1:4).eq.'JN99' .or. &
           grid(1:4).eq.'JN89') lgvalid=.true.

! Monaco 3A
      else if(callsign(1:2).eq.'3A' .and. grid(1:4).eq.'JN33') then
        lgvalid=.true.

 ! Chile 3G,CA..CE,XQ,XR
      else if(callsign(1:2).eq.'3G') then
! CE3TSK 2026-10-13: Antarctica - Chile's stations for every call of the rule: Frei, Escudero and Prat GC07, O'Higgins
! GC16, Guillermo Mann FC97, Gonzalez Videla and Yelcho FC85, Carvajal FC52, Union Glacier EB80
      if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'GC16' .or. grid(1:4).eq.'FC97' .or. grid(1:4).eq.'FC85' .or. &
         grid(1:4).eq.'FC52' .or. grid(1:4).eq.'EB80') lgvalid=.true.
! CE3TSK 2026-10-13: FF56 added - the upper Maipo valley, where the border runs east of 70 degrees west
      if((grid(1:2).eq.'FF' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'5') .or. grid(1:4).eq.'FF56' .or. &
! CE3TSK 2026-10-13: and FF55 and FF59, strips of the same kind. And Chile's Antarctic calls (the third character
! is 9) had no square: the South Shetland Islands, and the base on the peninsula across from them (GC16)
         grid(1:4).eq.'FF55' .or. grid(1:4).eq.'FF59' .or. &
! CE3TSK 2026-10-13: and FC86, FC87, FC96 - Smith and Low Islands and the south end of Deception, uninhabited
         (callsign(3:3).eq.'9' .and. (grid(1:4).eq.'FC97' .or. grid(1:4).eq.'GC28' .or. &
          grid(1:4).eq.'FC86' .or. grid(1:4).eq.'FC87' .or. grid(1:4).eq.'FC96' .or. &
          (grid(1:3).eq.'GC0' .and. (grid(4:4).eq.'7' .or. grid(4:4).eq.'8')) .or. &
          (grid(1:3).eq.'GC1' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.'9'))) .or. &
         (grid(1:2).eq.'FG' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'6') .or. &
          (grid(3:3).eq.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'8'))) .or. &
         (grid(1:2).eq.'FE' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5') .or. &
         (grid(1:2).eq.'FH' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
          grid(3:4).eq.'52')) .or. &
         (grid(1:2).eq.'FD' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'53' .or. grid(3:4).eq.'85')) .or. &
         grid(1:4).eq.'EF96' .or. grid(1:4).eq.'FF06' .or. grid(1:4).eq.'EG93' .or. grid(1:4).eq.'FG03' .or. &
         grid(1:4).eq.'DG52' .or. grid(1:4).eq.'DG73') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'FF' .and. grid(3:3).eq.'3' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') then; lgvalid=.false.
      else if(grid(1:2).eq.'FE' .and. (grid(3:4).eq.'40' .or. grid(3:4).eq.'41' .or. grid(3:4).eq.'29')) then; lgvalid=.false.
      endif
endif

! Agalega & St. Brandon 3B6,3B7
      else if((callsign(1:3).eq.'3B6' .or. callsign(1:3).eq.'3B7') .and. (grid(1:4).eq.'LH89' .or. grid(1:4).eq.'LH93')) then
        lgvalid=.true.

! Mauritius 3B8
! CE3TSK 2026-10-13: land that this rule refused - Agalega (LH89) and Rodrigues (MH10), which are Mauritius' too
      else if(callsign(1:3).eq.'3B8' .and. (grid(1:4).eq.'LG89' .or. grid(1:4).eq.'LH80' .or. grid(1:4).eq.'LH89' .or. &
              grid(1:4).eq.'MH10')) then
        lgvalid=.true.

! Rodriguez Island 3B9
      else if(callsign(1:3).eq.'3B9' .and. grid(1:4).eq.'MH10') then
        lgvalid=.true.

! Equatorial Guinea 3C, Annobon Island 3C0
      else if(callsign(1:2).eq.'3C') then
! CE3TSK 2026-10-13: land that this rule refused - Equatorial Guinea
        if(grid(1:4).eq.'JJ43') lland=.true.
      if((grid(1:3).eq.'JJ4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. grid(1:4).eq.'JJ51' .or. &
         grid(1:4).eq.'JJ52' .or. grid(1:4).eq.'JI28') lgvalid=.true.

! Fiji, Conway Reef, Rotuma Island 3D2
      else if(callsign(1:3).eq.'3D2') then
! CE3TSK 2026-10-13: land that this rule refused - Fiji
        if(grid(1:4).eq.'RH87') lland=.true.
        if((grid(1:2).eq.'RH' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'4').or. &
           (grid(1:3).eq.'AH0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
           grid(1:4).eq.'RG78' .or. grid(1:4).eq.'AG08' .or. grid(1:4).eq.'AG09') lgvalid=.true.

! Kingdom of eSwatini 3DA
      else if(callsign(1:3).eq.'3DA') then
        if((grid(1:3).eq.'KG5' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5') .or. grid(1:4).eq.'KG63' .or. &
           grid(1:4).eq.'KG64') lgvalid=.true.

! Tunisia 3V,TS
      else if(callsign(1:2).eq.'3V') then
        if(grid(1:2).eq.'JM' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') &
          lgvalid=.true.

! Vietnam 3W,XV
      else if(callsign(1:2).eq.'3W') then
        if((grid(1:2).eq.'OL' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'3').or. &
           (grid(1:2).eq.'OK' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'5') .or. &
           (grid(1:2).eq.'OJ' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'7'.and.grid(4:4).lt.':').or. &
           grid(1:4).eq.'OJ19' .or.  grid(1:4).eq.'OL41' .or.  grid(1:4).eq.'OL23') lgvalid=.true.

! Guinea 3X
      else if(callsign(1:2).eq.'3X') then
! CE3TSK 2026-10-13: land that this rule refused - Guinea
        if(grid(1:4).eq.'IK60') lland=.true.
        if((grid(1:2).eq.'IJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'6'.and.grid(4:4).lt.':').or. &
           (grid(1:2).eq.'IK' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'3').or. &
           grid(1:4).eq.'IJ39') lgvalid=.true.

! Bouvet, Peter 1st Island 3Y
! CE3TSK 2026-10-13: three characters of the call were compared with the two of '3Y', which is true of no call.
! CE3TSK 2026-10-13: and Norway's stations in Antarctica, which sign 3Y too: Troll JB17, its airfield JB18, Tor JB28
      else if(callsign(1:2).eq.'3Y' .and. (grid(1:4).eq.'JD15' .or.  grid(1:4).eq.'EC41' .or. &
              grid(1:4).eq.'JB17' .or. grid(1:4).eq.'JB18' .or. grid(1:4).eq.'JB28')) then
        lgvalid=.true.

! Panama 3E,3F,H3,H8,H9,HO,HP
      else if(callsign(2:2).eq.'E' .or. callsign(2:2).eq.'F') then
! CE3TSK 2026-10-13: land that this rule refused - Panama
        if(grid(1:4).eq.'FJ07' .or. grid(1:4).eq.'FJ19') lland=.true.
        if((grid(1:2).eq.'FJ' .and. (grid(3:4).eq.'08' .or. grid(3:4).eq.'09' .or. grid(3:4).eq.'17' .or. &
            grid(3:4).eq.'18')) .or. &
           (grid(1:2).eq.'EJ' .and. (grid(3:4).eq.'88' .or. grid(3:4).eq.'89' .or. grid(3:4).eq.'97' .or. &
            grid(3:4).eq.'98' .or. grid(3:4).eq.'99'))) lgvalid=.true.

! China, Taiwan  B, 3H..3U, XS
      else if(callsign(2:2).gt.'G' .and. callsign(2:2).lt.'V') then
        if(len_trim(callsign).gt.5) return ! checkcall
! CE3TSK 2026-10-13: land that this rule refused - China
        if(grid(1:4).eq.'MN60' .or. grid(1:4).eq.'MN92' .or. grid(1:4).eq.'MN94' .or. grid(1:4).eq.'OK55' .or. &
           grid(1:4).eq.'OK56' .or. grid(1:4).eq.'OK66' .or. grid(1:4).eq.'ON55' .or. &
           grid(1:4).eq.'ON65') lland=.true.
! CE3TSK 2026-10-13: Antarctica - China's stations: Great Wall GC07, Zhongshan MC80, Kunlun MA89, Taishan MB86, Qinling
! RB15
       if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'MC80' .or. grid(1:4).eq.'MA89' .or. grid(1:4).eq.'MB86' .or. &
          grid(1:4).eq.'RB15') lgvalid=.true.
       if(grid(1:2).eq.'OL' .or. &
       (grid(1:2).eq.'PL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2') .or. grid(1:2).eq.'OM' .or. &
       (grid(1:2).eq.'PM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3') .or. &
       (grid(1:2).eq.'PN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8') .or. &
       (grid(1:2).eq.'PO' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. &
       (grid(1:2).eq.'OK' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'58' .or.grid(3:4).eq.'59')).or. &
       grid(1:2).eq.'ON' .or. grid(1:4).eq.'OO90' .or. grid(1:4).eq.'OO91' .or. grid(1:2).eq.'NN' .or. grid(1:2).eq.'NM'.or. &
       (grid(1:2).eq.'NL' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. &
       (grid(1:2).eq.'MN' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
       (grid(1:2).eq.'MM' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':')) lgvalid=.true.

if(lgvalid) then
       if(grid(1:2).eq.'NL' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.'7'.and. &
          grid(3:4).ne.'83' .and. grid(3:4).ne.'84' .and. grid(3:4).ne.'85') .or. grid(3:4).eq.'77' .or. grid(3:4).eq.'87')) &
        then; lgvalid=.false.
       else if(grid(1:2).eq.'ON' .and.((grid(3:3).gt.'/'.and.grid(3:3).lt.'5'.and.grid(4:4).gt.'2'.and.grid(4:4).lt.':').or. &
               (grid(3:3).gt.'4' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':') .or. &
               grid(3:4).eq.'76' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
       else if(grid(1:2).eq.'PN' .and. ((grid(3:3).eq.'7' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6') .or. &
               (grid(3:3).eq.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5') .or. &
! CE3TSK 2026-10-13: PN52, PN53 and PN54 are no longer in this exclusion - the east of Jilin and Heilongjiang
! (Hunchun, Suifenhe), with stations on the air. PN50 and PN51 stay in it.
               grid(3:4).eq.'40' .or. grid(3:4).eq.'50' .or. grid(3:4).eq.'51' .or. grid(3:4).eq.'59' .or. &
               grid(3:4).eq.'69' .or. grid(3:4).eq.'79')) then; lgvalid=.false.
       else if(grid(1:2).eq.'MM' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'8') .or. &
               (grid(3:3).eq.'7' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6') .or. &
               (grid(3:3).eq.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5'))) then; lgvalid=.false.
       else if(grid(1:2).eq.'OL' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).eq.'0') .or. &
               grid(3:4).eq.'11' .or. grid(3:4).eq.'21')) then; lgvalid=.false.
       endif
endif

      endif ! '3'

    else if(callsign(1:1).eq.'4') then

! Venezuela, Aves Island YV..YY 4M
      if(callsign(1:2).eq.'4M') then
! CE3TSK 2026-10-13: land that this rule refused - Venezuela
        if(grid(1:4).eq.'FJ46' .or. grid(1:4).eq.'FJ56' .or. grid(1:4).eq.'FJ72' .or. grid(1:4).eq.'FJ82' .or. &
           grid(1:4).eq.'GJ08') lland=.true.
      if((grid(1:2).eq.'FJ' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'60' .or. grid(3:4).eq.'70')) .or. &
         (grid(1:2).eq.'FK' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
         grid(1:4).eq.'FK85') lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'FJ' .and. ((grid(3:3).gt.'2' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.'7' .and. &
         grid(3:4).ne.'57') .or. grid(3:4).eq.'92' .or. grid(3:4).eq.'93')) then; lgvalid=.false.
      else if(grid(1:2).eq.'FJ' .and. (grid(3:4).eq.'32' .or.grid(3:4).eq.'72'.or.grid(3:4).eq.'82'.or.grid(3:4).eq.'92'.or. &
              grid(3:4).eq.'91')) then; lgvalid=.false.
      endif
endif

! Israel 4X, 4Z
      else if(callsign(1:2).eq.'4X' .or. callsign(1:2).eq.'4Z') then
        if((grid(1:3).eq.'KM7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. grid(1:4).eq.'KL79') lgvalid=.true.

! Montenegro 4O
      else if(callsign(1:2).eq.'4O') then
        if((grid(1:3).eq.'JN9' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'4') .or. grid(1:4).eq.'KN02' .or. &
           grid(1:4).eq.'KN03') lgvalid=.true.

! Sri Lanka 4P,4Q,4R,4S
      else if(callsign(2:2).gt.'O' .and. callsign(2:2).lt.'T') then
        if((grid(1:3).eq.'MJ9' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
           (grid(1:3).eq.'NJ0' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':')) lgvalid=.true.

! Philippines 4D,4E,4F,4G,4H,4I,DU,DV,DW,DX,DY,DZ
      else if(callsign(2:2).gt.'C' .and. callsign(2:2).lt.'J') then
        if((grid(1:2).eq.'PK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4') .or. &
         (grid(1:2).eq.'PJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'04')) .or. &
         (grid(1:2).eq.'OJ' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'OK' .and. grid(3:3).eq.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7') .or. &
         (grid(1:2).eq.'PL' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01' .or. grid(3:4).eq.'10')) .or. &
         grid(1:4).eq.'OK70' .or. grid(1:4).eq.'OK71') lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'PK' .and. ((grid(3:3).eq.'3' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':') .or. &
           (grid(3:3).eq.'2' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':'))) then; lgvalid=.false.
        else if(grid(1:2).eq.'PJ' .and. (grid(3:4).eq.'08' .or. grid(3:4).eq.'15')) then; lgvalid=.false.
        else if(grid(1:2).eq.'OJ' .and. (grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) then; lgvalid=.false.
        endif
endif

! Azerbaijan 4J, 4K
! CE3TSK 2026-10-13: 4J was asked for twice and 4K never.
      else if(callsign(1:2).eq.'4J' .or. callsign(1:2).eq.'4K') then
        if((grid(1:2).eq.'LN' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
           (grid(1:2).eq.'LM' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'7'.and.grid(4:4).lt.':').or. &
           grid(1:4).eq.'LN50') lgvalid=.true.

! Georgia 4L
! CE3TSK 2026-10-13: this rule asked for 4J, which the one above had taken: no square was accepted for 4L.
      else if(callsign(1:2).eq.'4L') then
! CE3TSK 2026-10-13: land that this rule refused - Georgia
        if(grid(1:4).eq.'KN93') lland=.true.
        if(grid(1:2).eq.'LN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') &
          lgvalid=.true.

! ITU HQ, United Nations HQ, Vienna Intl Ctr
! CE3TSK 2026-10-13: and 4U1WB (Washington, FM18) and 4U1GSC (Brindisi, JN80), each in its own square
      else if(callsign(2:2).eq.'U' .and. &
              (callsign(1:6).eq.'4U1ITU' .or. callsign(1:5).eq.'4U1UN' .or. callsign(1:6).eq.'4U1VIC' .or. &
               (callsign(1:6).eq.'4U1WB ' .and. grid(1:4).eq.'FM18') .or. &
               (callsign(1:7).eq.'4U1GSC ' .and. grid(1:4).eq.'JN80') .or. &
               grid(1:4).eq.'JN88')) then
        lgvalid=.true.

! Mexico, Revillagigedo 4A..4C, 6D..6J, XA..XI
      else if(callsign(2:2).gt.'@' .and. callsign(2:2).lt.'D') then
      if((grid(1:2).eq.'DL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.':').or.grid(3:4).eq.'08'.or.grid(3:4).eq.'09')).or. &
         (grid(1:2).eq.'EL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8').or. &
         (grid(1:2).eq.'EK' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'34')) .or. &
         (grid(1:2).eq.'DM' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
         (grid(1:2).eq.'DK' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'DK' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'38'))) &
        lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'DL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2') .or. &
          grid(3:4).eq.'22' .or. grid(3:4).eq.'23' .or. grid(3:4).eq.'32' .or. grid(3:4).eq.'33' .or. grid(3:4).eq.'60')) then
        lgvalid=.false.
      else if(grid(1:2).eq.'EL' .and. ((grid(3:3).gt.'1'.and.grid(3:3).lt.'7'.and.grid(4:4).gt.'2'.and.grid(4:4).lt.'8').or. &
          grid(3:4).eq.'17' .or. grid(3:4).eq.'20' .or. grid(3:4).eq.'21' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'32' .or. &
          grid(3:4).eq.'62')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EK' .and. (grid(3:4).eq.'05' .or.grid(3:4).eq.'65'.or.grid(3:4).eq.'66'.or.grid(3:4).eq.'67'.or. &
        grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DM' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).eq.'2') then; lgvalid=.false.
      else if(grid(1:2).eq.'DK' .and. (grid(3:4).eq.'76' .or. grid(3:4).eq.'77' .or. grid(3:4).eq.'86')) then; lgvalid=.false.
      endif
endif

! Haiti 4V,HH
      else if(callsign(1:2).eq.'4V') then
! CE3TSK 2026-10-13: land that this rule refused - Haiti
        if(grid(1:4).eq.'FK28') lland=.true.
        if((grid(1:2).eq.'FK' .and. (grid(3:4).eq.'38' .or. grid(3:4).eq.'39' .or. grid(3:4).eq.'48' .or. &
           grid(3:4).eq.'49')) .or. grid(1:4).eq.'FL30') lgvalid=.true.

! Timor - Leste 4W
      else if(callsign(1:2).eq.'4W') then
! CE3TSK 2026-10-13: land that this rule refused - Timor-Leste
        if(grid(1:4).eq.'PI30') lland=.true.
        if(grid(1:2).eq.'PI' .and. (grid(3:4).eq.'20' .or. grid(3:4).eq.'21' .or. grid(3:4).eq.'31')) lgvalid=.true.

! Peru 4T,OA..OC
      else if(callsign(1:2).eq.'4T') then
! CE3TSK 2026-10-13: land that this rule refused - Peru
        if(grid(1:4).eq.'FH09' .or. grid(1:4).eq.'FI55') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Peru's station Machu Picchu GC07
        if(grid(1:4).eq.'GC07') lgvalid=.true.
        if((grid(1:2).eq.'FH' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.':').or. &
           (grid(1:2).eq.'FI' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
           (grid(1:3).eq.'EI9' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'7')) lgvalid=.true.

      endif ! '4'

    else if(callsign(1:1).eq.'5') then

! Denmark 5P,5Q,OU,OV,OZ
      if(callsign(1:2).eq.'5P' .or. callsign(1:2).eq.'5Q') then
      if(grid(1:2).eq.'JO' .and. &
         ((grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
          grid(3:4).eq.'75' .or. grid(3:4).eq.'76')) lgvalid=.true.

! Cyprus 5B,C4,H2,P3
      else if(callsign(1:2).eq.'5B') then
        if((grid(1:3).eq.'KM6' .and. (grid(4:4).eq.'4' .or. grid(4:4).eq.'5')) .or. &
           (grid(1:3).eq.'KM7' .and. (grid(4:4).eq.'4' .or. grid(4:4).eq.'5'))) lgvalid=.true.

! Morocco 5C..5G,CN
      else if(callsign(2:2).gt.'B' .and. callsign(2:2).lt.'H') then
        if((grid(1:2).eq.'IM' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6').or. &
           (grid(1:2).eq.'IL' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'6'.and.grid(4:4).gt.'0'.and.grid(4:4).lt.':').or. &
           grid(3:4).eq.'69' .or. grid(3:4).eq.'79'))) lgvalid=.true.

! Libya 5A
      else if(callsign(1:2).eq.'5A') then
! CE3TSK 2026-10-13: the first line asked for a column above 4 and below '/', which no digit is - so the coast west
! of 20 degrees east was refused, from Zuwara to Sirte: Tripoli (JM62) and Misrata (JM72). It is ':' now, as in every
! other rule
        if((grid(1:2).eq.'JM' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'3').or. &
         (grid(1:2).eq.'KM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
         (grid(1:2).eq.'JL' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3') .or. &
         (grid(1:2).eq.'JM' .and. (grid(3:4).eq.'40' .or. grid(3:4).eq.'53')) .or. &
           grid(1:4).eq.'JL91' .or. grid(1:4).eq.'KK19') lgvalid=.true.

! Tanzania 5H, 5I
      else if(callsign(1:2).eq.'5H' .or. callsign(1:2).eq.'5I') then
! CE3TSK 2026-10-13: land that this rule refused - Tanzania
        if(grid(1:4).eq.'KI59' .or. grid(1:4).eq.'LH09') lland=.true.
        if((grid(1:2).eq.'KI' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'9').or. &
           (grid(1:2).eq.'KH' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':')) &
          lgvalid=.true.

! Colombia 5J,5K,HJ,HK
      else if(callsign(1:2).eq.'5K' .or. callsign(1:2).eq.'5J') then
! CE3TSK 2026-10-13: land that this rule refused - Colombia
        if(grid(1:4).eq.'EK93' .or. grid(1:4).eq.'FJ00') lland=.true.
        if((grid(1:2).eq.'FJ' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'7') .or. &
          grid(3:4).eq.'01' .or. grid(3:4).eq.'02' .or. grid(3:4).eq.'03')) .or. &
        (grid(1:2).eq.'FK' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3' .and. &
         grid(3:4).ne.'22') .or. &
        (grid(1:2).eq.'FI' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'6' .and.grid(4:4).lt.':').or. &
         grid(3:4).eq.'45' .or. grid(3:4).eq.'55' .or. grid(3:4).eq.'46' .or. grid(3:4).eq.'56') .and.grid(3:4).ne.'27').or. &
        (grid(1:2).eq.'EJ' .and. (grid(3:4).eq.'93' .or. grid(3:4).eq.'94')) .or. grid(1:4).eq.'EK92') lgvalid=.true.

! Liberia 5L,5M,6Z,A8,D5,EL
      else if(callsign(1:2).eq.'5L' .or. callsign(1:2).eq.'5M') then
        if(grid(1:2).eq.'IJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3' .and.grid(4:4).lt.'9'.and. &
           grid(3:4).ne.'67' .and. grid(3:4).ne.'67') lgvalid=.true.

! Nigeria 5N,5O
      else if(callsign(1:2).eq.'5N' .or. callsign(1:2).eq.'5O') then
        if((grid(1:2).eq.'JJ' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
           (grid(1:2).eq.'JK' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4')) &
          lgvalid=.true.

! Madagascar 5R,5S,6X
      else if(callsign(1:2).eq.'5R' .or. callsign(1:2).eq.'5S') then
        if((grid(1:2).eq.'LH' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'7').or. &
           (grid(1:2).eq.'LG' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
           grid(1:2).eq.'LH' .and. (grid(3:4).eq.'12' .or. grid(3:4).eq.'47' .or. grid(3:4).eq.'48')) lgvalid=.true.

! Mauritania 5T
      else if(callsign(1:2).eq.'5T') then
! CE3TSK 2026-10-13: land that this rule refused - Mauritania
        if(grid(1:4).eq.'IK15' .or. grid(1:4).eq.'IL34' .or. grid(1:4).eq.'IL35') lland=.true.
        if((grid(1:2).eq.'IK' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'8'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
            grid(3:4).eq.'34' .or. grid(3:4).eq.'44')) .or. &
           (grid(1:2).eq.'IL' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'7'.and.grid(4:4).gt.'/'.and.grid(4:4).lt.'6').or. &
            grid(3:4).eq.'56' .or. grid(3:4).eq.'66' .or. grid(3:4).eq.'57' .or. grid(3:4).eq.'75'))) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'IK' .and. grid(3:4).eq.'15' .or. grid(3:4).eq.'25') then
        lgvalid=.false.
      else if(grid(1:2).eq.'IL' .and. ((grid(3:3).gt.'0'.and.grid(3:3).lt.'3'.and.grid(4:4).gt.'1'.and.grid(4:4).lt.'6').or. &
          grid(3:4).eq.'34' .or. grid(3:4).eq.'35')) then; lgvalid=.false.
      endif
endif

! Niger 5U
      else if(callsign(1:2).eq.'5U') then
        if((grid(1:2).eq.'JK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.':').or. &
           (grid(1:2).eq.'JL' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'3').or. &
           grid(1:4).eq.'JK11' .or. &
           grid(1:2).eq.'JL' .and. (grid(3:4).eq.'30' .or. grid(3:4).eq.'31' .or. grid(3:4).eq.'53' .or. grid(3:4).eq.'63')) &
          lgvalid=.true.

! Togo 5V
      else if(callsign(1:2).eq.'5V') then
        if((grid(1:3).eq.'JJ0' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
           (grid(1:2).eq.'JK' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01')) .or. &
           (grid(1:2).eq.'IK' .and. (grid(3:4).eq.'90' .or. grid(3:4).eq.'91'))) lgvalid=.true.

! Samoa 5W
      else if(callsign(1:2).eq.'5W') then
        if(grid(1:2).eq.'AH' .and. (grid(3:4).eq.'36' .or. grid(3:4).eq.'46' .or. grid(3:4).eq.'45')) lgvalid=.true.

! Uganda 5X
      else if(callsign(1:2).eq.'5X') then
        if((grid(1:2).eq.'KJ' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'4').or. &
           (grid(1:2).eq.'KI' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).eq.'9') .or. &
           (grid(1:2).eq.'KJ' .and. (grid(3:4).eq.'40' .or. grid(3:4).eq.'64' .or. grid(3:4).eq.'74')) .or. &
           (grid(1:2).eq.'KI' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'58'))) &
          lgvalid=.true.

! Kenya 5Y,5Z
      else if(callsign(1:2).eq.'5Z' .or. callsign(1:2).eq.'5Y') then
! CE3TSK 2026-10-13: land that this rule refused - Kenya
        if(grid(1:4).eq.'KJ64' .or. grid(1:4).eq.'KJ75') lland=.true.
        if((grid(1:2).eq.'KI' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
         (grid(1:2).eq.'KJ' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5').or. &
         (grid(1:2).eq.'LI' .and. grid(3:3).eq.'0' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
         (grid(1:2).eq.'LJ' .and. grid(3:3).eq.'0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
         grid(1:4).eq.'KJ60' .or. grid(1:4).eq.'KI69') lgvalid=.true.

      endif ! '5'

    else if(callsign(1:1).eq.'6') then

! Syria 6C,YK
      if(callsign(1:2).eq.'6C') then
! CE3TSK 2026-10-13: land that this rule refused - Syria
        if(grid(1:4).eq.'LM03') lland=.true.
        if((grid(1:2).eq.'KM' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.'7').or. &
         (grid(1:2).eq.'LM' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8')) &
        lgvalid=.true.

! Mexico, Revillagigedo 4A..4C, 6D..6J, XA..XI
      else if(callsign(2:2).gt.'C' .and. callsign(2:2).lt.'K') then
      if((grid(1:2).eq.'DL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.':').or.grid(3:4).eq.'08'.or.grid(3:4).eq.'09')).or. &
         (grid(1:2).eq.'EL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8').or. &
         (grid(1:2).eq.'EK' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'34')) .or. &
         (grid(1:2).eq.'DM' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3').or. &
         (grid(1:2).eq.'DK' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'DK' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'38'))) &
        lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'DL' .and. ((grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2') .or. &
          grid(3:4).eq.'22' .or. grid(3:4).eq.'23' .or. grid(3:4).eq.'32' .or. grid(3:4).eq.'33' .or. grid(3:4).eq.'60')) then
        lgvalid=.false.
      else if(grid(1:2).eq.'EL' .and. ((grid(3:3).gt.'1'.and.grid(3:3).lt.'7'.and.grid(4:4).gt.'2'.and.grid(4:4).lt.'8').or. &
          grid(3:4).eq.'17' .or. grid(3:4).eq.'20' .or. grid(3:4).eq.'21' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'32' .or. &
          grid(3:4).eq.'62')) then; lgvalid=.false.
      else if(grid(1:2).eq.'EK' .and. (grid(3:4).eq.'05' .or.grid(3:4).eq.'65'.or.grid(3:4).eq.'66'.or.grid(3:4).eq.'67'.or. &
        grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'DM' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).eq.'2') then; lgvalid=.false.
      else if(grid(1:2).eq.'DK' .and. (grid(3:4).eq.'76' .or. grid(3:4).eq.'77' .or. grid(3:4).eq.'86')) then; lgvalid=.false.
      endif
endif

! Republic of Korea 6K,6L,6M,6N,D7,D8,D9,DS,DT,HL
      else if(callsign(1:1).eq.'6' .and. callsign(2:2).gt.'J' .and. callsign(2:2).lt.'O') then
! CE3TSK 2026-10-13: land that this rule refused - Korea
        if(grid(1:4).eq.'PM27') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Korea's stations: King Sejong GC07, Jang Bogo RB25
        if(grid(1:4).eq.'GC07' .or. grid(1:4).eq.'RB25') lgvalid=.true.
        if((grid(1:3).eq.'PM3' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.'9') .or. &
           (grid(1:3).eq.'PM4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'9') .or. &
           grid(1:4).eq.'PM24' .or. grid(1:4).eq.'PM57' .or. grid(1:4).eq.'GC07') lgvalid=.true. ! DT8A in GC07OS South Shetland Islands

! Senegal 6V,6W
      else if(callsign(1:2).eq.'6V' .or. callsign(1:2).eq.'6W') then
        if(grid(1:2).eq.'IK' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.'7').or. &
           grid(3:4).eq.'42' .or. grid(3:4).eq.'43')) lgvalid=.true.

! Madagascar 5R,5S,6X
      else if(callsign(1:2).eq.'6X') then
        if((grid(1:2).eq.'LH' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'7').or. &
           (grid(1:2).eq.'LG' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
           grid(1:2).eq.'LH' .and. (grid(3:4).eq.'12' .or. grid(3:4).eq.'47' .or. grid(3:4).eq.'48')) lgvalid=.true.

! Jamaica 6Y
      else if(callsign(1:2).eq.'6Y') then
        if(grid(1:2).eq.'FK' .and. (grid(3:4).eq.'17' .or. grid(3:4).eq.'18' .or. grid(3:4).eq.'08' .or. grid(3:4).eq.'06')) &
          lgvalid=.true.

! Sudan 6T,6U,ST
      else if(callsign(2:2).eq.'T' .or. callsign(2:2).eq.'U') then
! CE3TSK 2026-10-13: land that this rule refused - Sudan
        if(grid(1:4).eq.'KJ18' .or. grid(1:4).eq.'KJ19' .or. grid(1:4).eq.'KJ28' .or. grid(1:4).eq.'KJ29' .or. &
           grid(1:4).eq.'KJ39' .or. grid(1:4).eq.'KJ49' .or. grid(1:4).eq.'KJ59' .or. grid(1:4).eq.'KJ69' .or. &
           grid(1:4).eq.'KK02' .or. grid(1:4).eq.'KK03' .or. grid(1:4).eq.'KL52') lland=.true.
        if((grid(1:2).eq.'KK' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.':') .or. &
           (grid(1:2).eq.'KL' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
           grid(1:4).eq.'KJ79') lgvalid=.true.

! Pakistan 6P..6S, AP..AS
      else if(callsign(2:2).gt.'O' .and. callsign(2:2).lt.'T') then
! CE3TSK 2026-10-13: land that this rule refused - Pakistan
        if(grid(1:4).eq.'ML43' .or. grid(1:4).eq.'MM44' .or. grid(1:4).eq.'MM77' .or. &
           grid(1:4).eq.'MM86') lland=.true.
      if((grid(1:2).eq.'MM' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'7').or. &
          grid(3:4).eq.'30' .or. grid(3:4).eq.'31' .or. grid(3:4).eq.'84' .or. grid(3:4).eq.'85')) .or. & 
         (grid(1:2).eq.'ML' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'33'))) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'MM' .and. grid(3:3).eq.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'7') then; lgvalid=.false.
      else if(grid(1:2).eq.'ML' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8') .or. &
              grid(3:4).eq.'07')) then; lgvalid=.false.
      endif
endif

! Liberia 5L,5M,6Z,A8,D5,EL
      else if(callsign(1:2).eq.'6Z') then
        if(grid(1:2).eq.'IJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'3' .and.grid(4:4).lt.'9'.and. &
           grid(3:4).ne.'67' .and. grid(3:4).ne.'67') lgvalid=.true.

! Egypt 6A,6B,SS,SU
! CE3TSK 2026-10-13: there was no rule for these calls, so no square was accepted for them and every weak CQ of
! theirs was deleted. The squares are the ones the country touches (geodata/dxcc_grids.json, Natural Earth).
      else if(callsign(1:2).eq.'6A' .or. callsign(1:2).eq.'6B') then
        if((grid(1:2).eq.'KL' .and. ((grid(3:3).eq.'8' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'3') .or. &
            (grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.':'))) .or. &
           (grid(1:2).eq.'KM' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2')) &
          lgvalid=.true.

! Somalia 6O,T5
      else if(callsign(1:2).eq.'6O') then
        if((grid(1:2).eq.'LI' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
            grid(3:4).eq.'19')) .or. &
           (grid(1:2).eq.'LJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
            (grid(3:3).eq.'2' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'7') .or. &
            (grid(3:3).eq.'3' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.':') .or. &
            (grid(3:3).eq.'4' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.':') .or. &
            (grid(3:3).gt.'0' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':') .or. &
            (grid(3:3).eq.'5' .and. grid(4:4).gt.'7' .and. grid(4:4).lt.':'))) .or. &
           (grid(1:2).eq.'LK' .and. ((grid(3:3).eq.'1' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
            (grid(3:3).gt.'2' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
            grid(3:4).eq.'20'))) lgvalid=.true.

      endif ! '6'

    else if(callsign(1:1).eq.'7') then

! Indonesia 7A..7I, 8A..8I, PK..PO, YB..YH
      if(callsign(1:1).eq.'7' .and. callsign(2:2).gt.'@' .and. callsign(2:2).lt.'J') then
! CE3TSK 2026-10-13: land that this rule refused - Indonesia
        if(grid(1:4).eq.'OH99' .or. grid(1:4).eq.'PI34' .or. grid(1:4).eq.'PJ35' .or. grid(1:4).eq.'PJ51' .or. &
           grid(1:4).eq.'PJ70') lland=.true.
        if((grid(1:2).eq.'NJ' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':') .or. &
        (grid(1:2).eq.'NI' .and. grid(3:3).gt.'8' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
        (grid(1:2).eq.'OI' .and. .not.(grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and.grid(4:4).lt.'4') &
         .and. .not.(grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).eq.'0')) .or. &
        (grid(1:2).eq.'PI' .and. .not.(grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).eq.'0')) .or. &
        (grid(1:2).eq.'OJ' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
        (grid(1:2).eq.'PJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
        (grid(1:2).eq.'QI' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'1' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') .or. &
        (grid(1:2).eq.'PH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).eq.'9')) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'PJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'1' .and.grid(4:4).lt.'5').or. &
         grid(3:4).eq.'43' .or. grid(3:4).eq.'44')) then; lgvalid=.false.
      else if(grid(1:2).eq.'NJ' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'/' .and. grid(4:4) .lt.'6') .or. &
         (grid(3:3).eq.'7' .and. ((grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. grid(4:4).eq.'0' .or. grid(4:4).eq.'1' .or. &
          grid(4:4).eq.'3')) .or. (grid(3:3).eq.'8' .and. grid(4:4).gt.'5'.and. grid(4:4).lt.':') .or. &
         grid(3:4).eq.'43' .or. grid(3:4).eq.'44' .or. grid(3:4).eq.'95')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OJ' .and. ((grid(3:3).gt.'4'.and.grid(3:3).lt.'7'.and.grid(4:4).gt.'1'.and.grid(4:4).lt.'5').or. &
         (grid(3:3).eq.'1' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5').or. &
         grid(3:4).eq.'24' .or. grid(3:4).eq.'04')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OI' .and. ((grid(4:4).eq.'1' .and. grid(3:3).gt.'1'.and. grid(3:3).lt.'5') .or. &
         (grid(4:4).eq.'5' .and. grid(3:3).gt.'2'.and. grid(3:3).lt.'6') .or. &
         grid(3:4).eq.'44' .or. grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'PI' .and. (grid(3:4).eq.'61' .or.grid(3:4).eq.'62'.or.grid(3:4).eq.'71'.or.grid(3:4).eq.'83'.or. &
         grid(3:4).eq.'25' .or. grid(3:4).eq.'35' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'44' .or.grid(3:4).eq.'29')) then
        lgvalid=.false.
      endif
endif

! Japan 7J..7N,8J..8N,JA,JE..JS
      else if(callsign(1:1).eq.'7' .and. callsign(2:2).gt.'I' .and. callsign(2:2).lt.'O') then
! CE3TSK 2026-10-13: land that this rule refused - Japan (the Ogasawara, Volcano and Izu Islands, Minami Torishima)
        if(grid(1:4).eq.'PL15' .or. grid(1:4).eq.'QL04' .or. grid(1:4).eq.'QL05' .or. grid(1:4).eq.'QL07' .or. &
           grid(1:4).eq.'QL16' .or. grid(1:4).eq.'QL17' .or. grid(1:4).eq.'QL64' .or. grid(1:4).eq.'QM00' .or. &
           grid(1:4).eq.'QM01') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Japan's stations: Syowa KC90 (and KC91, 400 m north of it), S17 LC00, Asuka KB28, Dome
! Fuji KB92, Mizuho LB29
        if(grid(1:4).eq.'KC90' .or. grid(1:4).eq.'KC91' .or. grid(1:4).eq.'LC00' .or. grid(1:4).eq.'KB28' .or. &
           grid(1:4).eq.'KB92' .or. grid(1:4).eq.'LB29') lgvalid=.true.
        if((grid(1:2).eq.'PM' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
           (grid(1:3).eq.'QM0' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. &
           (grid(1:2).eq.'QN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6').or. &
           (grid(1:3).eq.'PN9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
           (grid(1:2).eq.'PL' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
           grid(1:4).eq.'QM19' .or. grid(1:4).eq.'KC90') lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'PM' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and.grid(4:4).gt.'6'.and.grid(4:4).lt.':').or. &
           (grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3'.and.grid(3:4).ne.'62').or. &
           grid(3:4).eq.'45' .or. grid(3:4).eq.'46' .or. grid(3:4).eq.'55' .or. grid(3:4).eq.'56' .or.grid(3:4).eq.'88' .or. &
           grid(3:4).eq.'89')) then; lgvalid=.false.
        else if(grid(1:2).eq.'PL'.and.((grid(3:3).gt.'0'.and.grid(3:3).lt.'3'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
           grid(3:4).eq.'38' .or. grid(3:4).eq.'39' .or. grid(3:4).eq.'57' .or. grid(3:4).eq.'59')) then; lgvalid=.false.
        else if(grid(1:2).eq.'QN'.and.((grid(3:3).eq.'2'.and.(grid(4:4).eq.'0'.or.grid(4:4).eq.'1'.or.grid(4:4).eq.'5')).or. &
           grid(3:4).eq.'10')) then; lgvalid=.false.
        endif
endif

! Yemen 7O
      else if(callsign(1:2).eq.'7O') then
! CE3TSK 2026-10-13: land that this rule refused - Yemen
        if(grid(1:4).eq.'LK62' .or. grid(1:4).eq.'LK72') lland=.true.
        if(grid(1:2).eq.'LK' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'7' .and.grid(4:4).gt.'2'.and.grid(4:4).lt.'9').or. &
          grid(3:4).eq.'05' .or. grid(3:4).eq.'06' .or. grid(3:4).eq.'12' .or. grid(3:4).eq.'22')) lgvalid=.true.

! Lesotho 7P
      else if(callsign(1:2).eq.'7P') then
        if((grid(1:2).eq.'KG' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
          (grid(1:2).eq.'KF' .and. (grid(3:4).eq.'39' .or. grid(3:4).eq.'49'))) lgvalid=.true.

! Malawi 7Q
      else if(callsign(1:2).eq.'7Q') then
! CE3TSK 2026-10-13: land that this rule refused - Malawi
        if(grid(1:4).eq.'KI70') lland=.true.
        if((grid(1:2).eq.'KH' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
          grid(1:4).eq.'KI60' .or. (grid(1:3).eq.'KH7' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5')) lgvalid=.true.

! Sweden 7S,8S,SA..SM
      else if(callsign(1:2).eq.'7S') then
! CE3TSK 2026-10-13: land that this rule refused - Sweden
        if(grid(1:4).eq.'KP26') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Sweden's stations: Wasa IB36, Svea IB45
      if(grid(1:4).eq.'IB36' .or. grid(1:4).eq.'IB45') lgvalid=.true.
      if((grid(1:2).eq.'JO' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'JP' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9').or. &
         (grid(1:2).eq.'KP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'2'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'25'))) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'JO' .and. (grid(3:4).eq.'85' .or. grid(3:4).eq.'95')) then; lgvalid=.false.
      else if(grid(1:2).eq.'JP' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. &
             (grid(3:3).eq.'7' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.'9') .or. grid(3:4).eq.'89' .or. &
              grid(3:4).eq.'99')) then; lgvalid=.false.
      else if(grid(1:2).eq.'KP' .and. (grid(3:4).eq.'13' .or. grid(3:4).eq.'14')) then; lgvalid=.false.
      endif
endif

! Algeria 7R, 7T..7Y
      else if(callsign(1:2).eq.'7R' .or. callsign(2:2).gt.'S' .and. callsign(2:2).lt.'Z') then
! CE3TSK 2026-10-13: land that this rule refused - Algeria
        if(grid(1:4).eq.'IL91' .or. grid(1:4).eq.'JK18' .or. grid(1:4).eq.'JL52' .or. grid(1:4).eq.'JL53' .or. &
           grid(1:4).eq.'JL54' .or. grid(1:4).eq.'JL55' .or. grid(1:4).eq.'JM43') lland=.true.
        if((grid(1:2).eq.'JM' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'5'.and.grid(4:4).gt.'/'.and.grid(4:4).lt.'7').or. &
            grid(3:4).eq.'37') .and. grid(3:4).ne.'43') .or. &
        (grid(1:2).eq.'IM' .and. ((grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and.grid(4:4).lt.'6').or. &
            grid(3:4).eq.'70')) .or. &
        (grid(1:2).eq.'JL' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
         grid(3:3).eq.'5' .and. grid(3:3).lt.'1' .and. grid(4:4).eq.'5')) .or. &
        (grid(1:2).eq.'IL' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2' .and.grid(4:4).lt.':').or. &
            grid(3:4).eq.'92')) .or. &
        (grid(1:2).eq.'JK' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'4' .and. grid(4:4).eq.'9')) lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'IL' .and. ((grid(3:3).eq.'5' .and. grid(3:3).lt.'2' .and. grid(4:4).eq.'6') .or. &
           grid(3:4).eq.'63' .or. grid(3:4).eq.'64' .or. grid(3:4).eq.'73')) lgvalid=.false.
endif

! Saudi Arabia 7Z,8Z,HZ
      else if(callsign(1:2).eq.'7Z') then
! CE3TSK 2026-10-13: land that this rule refused - Saudi Arabia
        if(grid(1:4).eq.'LM11') lland=.true.
        if((grid(1:2).eq.'LL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8').or. &
        (grid(1:2).eq.'KL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':') .or. &
        (grid(1:2).eq.'LK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'5' .and.grid(4:4).lt.':').or. &
        (grid(1:2).eq.'LM' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01' .or. grid(3:4).eq.'10')) .or. &
        (grid(1:2).eq.'KM' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and.grid(4:4).lt.'2') .or. &
        grid(1:4).eq.'KM92') lgvalid=.true.

      endif ! '7'

    else if(callsign(1:1).eq.'8') then

! Indonesia 7A..7I, 8A..8I, PK..PO, YB..YH
      if(callsign(1:1).eq.'8' .and. callsign(2:2).gt.'@' .and. callsign(2:2).lt.'J') then
! CE3TSK 2026-10-13: land that this rule refused - Indonesia
        if(grid(1:4).eq.'OH99' .or. grid(1:4).eq.'PI34' .or. grid(1:4).eq.'PJ35' .or. grid(1:4).eq.'PJ51' .or. &
           grid(1:4).eq.'PJ70') lland=.true.
        if((grid(1:2).eq.'NJ' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':') .or. &
        (grid(1:2).eq.'NI' .and. grid(3:3).gt.'8' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':') .or. &
        (grid(1:2).eq.'OI' .and. .not.(grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/' .and.grid(4:4).lt.'4') &
         .and. .not.(grid(3:3).gt.'1' .and. grid(3:3).lt.'8' .and. grid(4:4).eq.'0')) .or. &
        (grid(1:2).eq.'PI' .and. .not.(grid(3:3).gt.'3' .and. grid(3:3).lt.':' .and. grid(4:4).eq.'0')) .or. &
        (grid(1:2).eq.'OJ' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
        (grid(1:2).eq.'PJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5') .or. &
        (grid(1:2).eq.'QI' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'1' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') .or. &
        (grid(1:2).eq.'PH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).eq.'9')) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'PJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'1' .and.grid(4:4).lt.'5').or. &
         grid(3:4).eq.'43' .or. grid(3:4).eq.'44')) then; lgvalid=.false.
      else if(grid(1:2).eq.'NJ' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'/' .and. grid(4:4) .lt.'6') .or. &
         (grid(3:3).eq.'7' .and. ((grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. grid(4:4).eq.'0' .or. grid(4:4).eq.'1' .or. &
          grid(4:4).eq.'3')) .or. (grid(3:3).eq.'8' .and. grid(4:4).gt.'5'.and. grid(4:4).lt.':') .or. &
         grid(3:4).eq.'43' .or. grid(3:4).eq.'44' .or. grid(3:4).eq.'95')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OJ' .and. ((grid(3:3).gt.'4'.and.grid(3:3).lt.'7'.and.grid(4:4).gt.'1'.and.grid(4:4).lt.'5').or. &
         (grid(3:3).eq.'1' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'5').or. &
         grid(3:4).eq.'24' .or. grid(3:4).eq.'04')) then; lgvalid=.false.
      else if(grid(1:2).eq.'OI' .and. ((grid(4:4).eq.'1' .and. grid(3:3).gt.'1'.and. grid(3:3).lt.'5') .or. &
         (grid(4:4).eq.'5' .and. grid(3:3).gt.'2'.and. grid(3:3).lt.'6') .or. &
         grid(3:4).eq.'44' .or. grid(3:4).eq.'39')) then; lgvalid=.false.
      else if(grid(1:2).eq.'PI' .and. (grid(3:4).eq.'61' .or.grid(3:4).eq.'62'.or.grid(3:4).eq.'71'.or.grid(3:4).eq.'83'.or. &
         grid(3:4).eq.'25' .or. grid(3:4).eq.'35' .or. grid(3:4).eq.'34' .or. grid(3:4).eq.'44' .or.grid(3:4).eq.'29')) then
        lgvalid=.false.
      endif
endif

! Japan 7J..7N,8J..8N,JA,JE..JS
      else if(callsign(1:1).eq.'8' .and. callsign(2:2).gt.'I' .and. callsign(2:2).lt.'O') then
! CE3TSK 2026-10-13: land that this rule refused - Japan (the Ogasawara, Volcano and Izu Islands, Minami Torishima)
        if(grid(1:4).eq.'PL15' .or. grid(1:4).eq.'QL04' .or. grid(1:4).eq.'QL05' .or. grid(1:4).eq.'QL07' .or. &
           grid(1:4).eq.'QL16' .or. grid(1:4).eq.'QL17' .or. grid(1:4).eq.'QL64' .or. grid(1:4).eq.'QM00' .or. &
           grid(1:4).eq.'QM01') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Japan's stations: Syowa KC90 (and KC91, 400 m north of it), S17 LC00, Asuka KB28, Dome
! Fuji KB92, Mizuho LB29
        if(grid(1:4).eq.'KC90' .or. grid(1:4).eq.'KC91' .or. grid(1:4).eq.'LC00' .or. grid(1:4).eq.'KB28' .or. &
           grid(1:4).eq.'KB92' .or. grid(1:4).eq.'LB29') lgvalid=.true.
        if((grid(1:2).eq.'PM' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
           (grid(1:3).eq.'QM0' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':') .or. &
           (grid(1:2).eq.'QN' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'6').or. &
           (grid(1:3).eq.'PN9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3') .or. &
           (grid(1:2).eq.'PL' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
           grid(1:4).eq.'QM19' .or. grid(1:4).eq.'KC90') lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'PM' .and. ((grid(3:3).gt.'3' .and. grid(3:3).lt.'8' .and.grid(4:4).gt.'6'.and.grid(4:4).lt.':').or. &
           (grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'3'.and.grid(3:4).ne.'62').or. &
           grid(3:4).eq.'45' .or. grid(3:4).eq.'46' .or. grid(3:4).eq.'55' .or. grid(3:4).eq.'56' .or.grid(3:4).eq.'88' .or. &
           grid(3:4).eq.'89')) then; lgvalid=.false.
        else if(grid(1:2).eq.'PL'.and.((grid(3:3).gt.'0'.and.grid(3:3).lt.'3'.and.grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
           grid(3:4).eq.'38' .or. grid(3:4).eq.'39' .or. grid(3:4).eq.'57' .or. grid(3:4).eq.'59')) then; lgvalid=.false.
        else if(grid(1:2).eq.'QN'.and.((grid(3:3).eq.'2'.and.(grid(4:4).eq.'0'.or.grid(4:4).eq.'1'.or.grid(4:4).eq.'5')).or. &
           grid(3:4).eq.'10')) then; lgvalid=.false.
        endif
endif

! Botswana 8O,A2
      else if(callsign(1:2).eq.'8O') then
! CE3TSK 2026-10-13: land that this rule refused - Botswana
        if(grid(1:4).eq.'JG95' .or. grid(1:4).eq.'JG96' .or. grid(1:4).eq.'JG97') lland=.true.
        if((grid(1:2).eq.'KG' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'2'.and.grid(4:4).lt.':').or. &
        (grid(1:2).eq.'KH' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2') .or. &
        (grid(1:2).eq.'KH' .and. (grid(3:4).eq.'22' .or. grid(3:4).eq.'30')) .or. &
        (grid(1:2).eq.'KG' .and. (grid(3:4).eq.'48' .or. grid(3:4).eq.'47'))) lgvalid=.true.

! Barbados 8P
      else if(callsign(1:2).eq.'8P' .and. grid(1:4).eq.'GK03') then
        lgvalid=.true.

! Maldives 8Q
      else if(callsign(1:2).eq.'8Q') then
        if((grid(1:3).eq.'MJ6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'8') .or. grid(1:4).eq.'MI69') lgvalid=.true.

! Guyana 8R
      else if(callsign(1:2).eq.'8R') then
! CE3TSK 2026-10-13: land that this rule refused - Guyana
        if(grid(1:4).eq.'FJ98') lland=.true.
        if((grid(1:2).eq.'GJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'0'.and.grid(4:4).lt.'9').or. &
        (grid(1:3).eq.'FJ9' .and. grid(4:4).gt.'3' .and. grid(4:4).lt.'8')) lgvalid=.true.

! Sweden 7S,8S,SA..SM
      else if(callsign(1:2).eq.'8S') then
! CE3TSK 2026-10-13: land that this rule refused - Sweden
        if(grid(1:4).eq.'KP26') lland=.true.
! CE3TSK 2026-10-13: Antarctica - Sweden's stations: Wasa IB36, Svea IB45
      if(grid(1:4).eq.'IB36' .or. grid(1:4).eq.'IB45') lgvalid=.true.
      if((grid(1:2).eq.'JO' .and. grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.':').or. &
         (grid(1:2).eq.'JP' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'9').or. &
         (grid(1:2).eq.'KP' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'2'.and.grid(4:4).lt.':').or. &
          grid(3:4).eq.'25'))) lgvalid=.true.

if(lgvalid) then
      if(grid(1:2).eq.'JO' .and. (grid(3:4).eq.'85' .or. grid(3:4).eq.'95')) then; lgvalid=.false.
      else if(grid(1:2).eq.'JP' .and. ((grid(3:3).eq.'6' .and. grid(4:4).gt.'4' .and. grid(4:4).lt.'9') .or. &
             (grid(3:3).eq.'7' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.'9') .or. grid(3:4).eq.'89' .or. &
              grid(3:4).eq.'99')) then; lgvalid=.false.
      else if(grid(1:2).eq.'KP' .and. (grid(3:4).eq.'13' .or. grid(3:4).eq.'14')) then; lgvalid=.false.
      endif
endif

! India 8T..8Y,AT..AW,VT..VW
      else if(callsign(2:2).gt.'S' .and. callsign(2:2).lt.'Z') then
! CE3TSK 2026-10-13: Antarctica - India's stations: Maitri JB59, Bharati MC80
        if(grid(1:4).eq.'JB59' .or. grid(1:4).eq.'MC80') lgvalid=.true.
        if((grid(1:2).eq.'MJ' .and. (grid(3:4).eq.'99' .or. grid(3:4).eq.'89' .or. grid(3:4).eq.'88' .or. &
! CE3TSK 2026-10-13: MJ98 (Thoothukudi) and, below, MM85 and MM95 (Ladakh north of 35 degrees): India's, and refused
            grid(3:4).eq.'68' .or. grid(3:4).eq.'98')).or. &
           (grid(1:2).eq.'MK' .and. ((grid(3:3).gt.'5'.and.grid(3:3).lt.':').or.grid(3:4).eq.'51'.or.grid(3:4).eq.'52')).or. &
           (grid(1:2).eq.'NK' .and. ((grid(3:3).eq.'0' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.'6') .or. &
            (grid(3:3).gt.'/' .and. grid(3:3).lt.'3' .and. grid(4:4).gt.'5'.and.grid(4:4).lt.':').or.grid(3:4).eq.'39')).or. &
           (grid(1:2).eq.'ML' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.':') .or. &
           (grid(1:2).eq.'NL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5') .or. &
! CE3TSK 2026-10-13: the Andaman and Nicobar Islands (VU4) had no square; theirs are accepted now
           (grid(1:3).eq.'NK6' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'4') .or. grid(1:4).eq.'NK73' .or. &
           (grid(1:3).eq.'NJ6' .and. grid(4:4).gt.'5' .and. grid(4:4).lt.':') .or. &
! CE3TSK 2026-10-13: there was nothing for the north-east (Assam and its neighbours, 90 to 98 degrees east);
! the squares the country touches there are accepted now (geodata/dxcc_grids.json, Natural Earth)
           (grid(1:2).eq.'NL' .and. ((grid(3:3).eq.'5' .and. grid(4:4).gt.'1' .and. grid(4:4).lt.'8') .or. &
            (grid(3:3).eq.'6' .and. grid(4:4).gt.'0' .and. grid(4:4).lt.'9') .or. &
            (grid(3:3).eq.'7' .and. grid(4:4).gt.'2' .and. grid(4:4).lt.':') .or. &
            (grid(3:3).eq.'8' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':'))) .or. &
           (grid(1:2).eq.'MM' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
           grid(1:4).eq.'NM00' .or. grid(1:4).eq.'MM85' .or. grid(1:4).eq.'MM95') lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'MK' .and. grid(3:3).eq.'6' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.'5') then; lgvalid=.false.
        else if(grid(1:2).eq.'NK' .and. (grid(3:4).eq.'26' .or. grid(3:4).eq.'27')) then; lgvalid=.false.
! CE3TSK 2026-10-13: this exclusion took rows 0, 5, 8 and 9 out of EVERY column of ML east of 70 degrees -
! Delhi is ML88, Jhansi ML95, Bikaner ML68. Only ML58 and ML59 are left in it.
! CE3TSK 2026-10-13: ML49 joins this exclusion (Pakistan, 115 km beyond the border) and ML58 leaves it (it holds
! 276 km2 of India). The line that stood here for NL excluded nothing but NL40: it asked for a row digit that
! is 9, or 8, and below 5. The squares north of the Himalaya that hold nothing of India are named now - NL19,
! NL28, NL29, NL39, NL49 (Nepal, Tibet; 54 to 165 km). NL09 and NL48 have India in them and stay; so do MK51
! and MK52 (Lakshadweep's outer reefs) and the squares the border comes within 25 km of
        else if(grid(1:2).eq.'ML' .and. (grid(3:4).eq.'59' .or. grid(3:4).eq.'49')) then; lgvalid=.false.
        else if(grid(1:2).eq.'NL' .and. (grid(3:4).eq.'19' .or. grid(3:4).eq.'28' .or. grid(3:4).eq.'29' .or. &
                grid(3:4).eq.'39' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'40')) then; lgvalid=.false.
        else if(grid(1:2).eq.'MM' .and. (grid(3:4).eq.'61' .or. grid(3:4).eq.'62')) then; lgvalid=.false.
        endif
endif

! Saudi Arabia 7Z,8Z,HZ
      else if(callsign(1:2).eq.'8Z') then
! CE3TSK 2026-10-13: land that this rule refused - Saudi Arabia
        if(grid(1:4).eq.'LM11') lland=.true.
        if((grid(1:2).eq.'LL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8').or. &
        (grid(1:2).eq.'KL' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':') .or. &
        (grid(1:2).eq.'LK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'8' .and. grid(4:4).gt.'5' .and.grid(4:4).lt.':') .or. &
        (grid(1:2).eq.'LM' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'01' .or. grid(3:4).eq.'10')) .or. &
        (grid(1:2).eq.'KM' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and.grid(4:4).lt.'2') .or. &
        grid(1:4).eq.'KM92') lgvalid=.true.

      endif ! '8'

    else if(callsign(1:1).eq.'9') then

! Croatia 9A
      if(callsign(1:2).eq.'9A') then
        if(grid(1:2).eq.'JN' .and. ((grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and.grid(4:4).gt.'1'.and.grid(4:4).lt.'6').or. &
           grid(3:4).eq.'64' .or. grid(3:4).eq.'65' .or. grid(3:4).eq.'76' .or. grid(3:4).eq.'86') .and. grid(3:4).ne.'93') & 
          lgvalid=.true.

! Malta 9H
      else if(callsign(1:2).eq.'9H') then
        if(grid(1:2).eq.'JM' .and. (grid(3:4).eq.'75' .or. grid(3:4).eq.'76')) lgvalid=.true.

! West Malaysia, East Malaysia 9M,9W
      else if(callsign(1:2).eq.'9M' .or. callsign(1:2).eq.'9W') then
! CE3TSK 2026-10-13: land that this rule refused - Malaysia
        if(grid(1:4).eq.'OJ42' .or. grid(1:4).eq.'OJ50') lland=.true.
! CE3TSK 2026-10-13: a bracket stood in the wrong place, so the four single squares of the last line (OJ21, OJ22,
! OJ41, OJ87) were accepted in EVERY field - a 9M or 9W call in JO21, in FN41... They are squares of OJ now.
! And Spratly as 9M0: its own rule further down is never reached, this one takes every 9M call - its squares
! are accepted here for 9M0
        if(grid(1:4).eq.'NJ96' .or. (grid(1:2).eq.'OJ' .and. &
           ((((grid(3:3).gt.'/' .and. grid(3:3).lt.'2') .or. (grid(3:3).gt.'4' .and.grid(3:3).lt.':')) .and. &
            grid(4:4).gt.'0'.and.grid(4:4).lt.'7') .or. &
           grid(3:4).eq.'21' .or. grid(3:4).eq.'22' .or. grid(3:4).eq.'41' .or. grid(3:4).eq.'87')) .or. &
           (callsign(1:3).eq.'9M0' .and. &
            ((grid(1:2).eq.'OK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
             (grid(1:2).eq.'OJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':')))) &
          lgvalid=.true.

! Kuwait 9K
      else if(callsign(1:2).eq.'9K') then
        if((grid(1:2).eq.'LL' .and. (grid(3:4).eq.'39' .or. grid(3:4).eq.'49' .or. grid(3:4).eq.'38' .or. &
            grid(3:4).eq.'48')) .or. &
           (grid(1:2).eq.'LM' .and. (grid(3:4).eq.'30' .or. grid(3:4).eq.'40'))) lgvalid=.true.

! Singapore 9V,S6
      else if(callsign(1:2).eq.'9V') then
        if(grid(1:2).eq.'OJ' .and. (grid(3:4).eq.'11' .or. grid(3:4).eq.'21')) lgvalid=.true.

! Trinidad & Tobago 9Y,9Z
      else if(callsign(1:2).eq.'9Y' .or. callsign(1:2).eq.'9Z') then
        if(grid(1:2).eq.'FK' .and. (grid(3:4).eq.'90' .or. grid(3:4).eq.'91')) lgvalid=.true.

! Nepal 9N
      else if(callsign(1:2).eq.'9N') then
        if((grid(1:2).eq.'NL' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'5' .and.grid(4:4).lt.':').or. &
          (grid(1:2).eq.'NM' .and. (grid(3:4).eq.'00' .or. grid(3:4).eq.'10'))) lgvalid=.true.

! Ghana 9G
! CE3TSK 2026-10-13: the rule named the IJ columns 0 to 4 (rows 6 to 9: open sea, Sierra Leone, Liberia, Guinea;
! 740 to 1 640 km from Ghana) and JJ84, JJ94 (the Central African Republic); the country is in columns 8 and 9
! (Accra is IJ95). Its own squares are accepted (geodata/dxcc_grids.json), those of the slip no longer
      else if(callsign(1:2).eq.'9G') then
        if((grid(1:2).eq.'IJ' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
           (grid(1:3).eq.'JJ0' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':').or. &
           (grid(1:2).eq.'IK' .and. grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).eq.'0').or. &
           grid(1:4).eq.'IK91' .or. grid(1:4).eq.'JK01' .or. &
           grid(1:4).eq.'JK00') lgvalid=.true.

! Zambia 9I,9J
      else if(callsign(1:2).eq.'9J' .or. callsign(1:2).eq.'9I') then
! CE3TSK 2026-10-13: land that this rule refused - Zambia
        if(grid(1:4).eq.'KH03' .or. grid(1:4).eq.'KH04' .or. grid(1:4).eq.'KH05' .or. &
           grid(1:4).eq.'KH06') lland=.true.
        if((grid(1:2).eq.'KH' .and. grid(3:3).gt.'0' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.':').or. &
           (grid(1:2).eq.'KI' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'7' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'2').or. &
           grid(1:4).eq.'KH31') lgvalid=.true.

! Sierra Leone 9L
      else if(callsign(1:2).eq.'9L') then
        if((grid(1:2).eq.'IJ' .and. grid(3:3).gt.'2' .and. grid(3:3).lt.'5' .and. grid(4:4).gt.'6'.and.grid(4:4).lt.':').or. &
           grid(1:4).eq.'IJ46') lgvalid=.true.

! Dem. Rep. of the Congo 9O..9T
      else if(callsign(2:2).gt.'N' .and. callsign(2:2).lt.'U') then
! CE3TSK 2026-10-13: land that this rule refused - the Democratic Republic of the Congo
        if(grid(1:4).eq.'JI63' .or. grid(1:4).eq.'JI76' .or. grid(1:4).eq.'JI91' .or. &
           grid(1:4).eq.'KI53') lland=.true.
        if((grid(1:2).eq.'JI' .and. ((grid(3:3).gt.'5' .and. grid(3:3).lt.'8'.and.grid(4:4).gt.'3'.and.grid(4:4).lt.'6').or. &
! CE3TSK 2026-10-13: (a bracket stood in the wrong place: the square 81 was accepted in EVERY field. It is JI81)
            grid(3:3).gt.'7' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'1'.and.grid(4:4).lt.':' .or. grid(3:4).eq.'81')).or. &
           (grid(1:2).eq.'KI' .and. ((grid(3:3).gt.'/'.and.grid(3:3).lt.'5').or.grid(3:4).eq.'51'.or.grid(3:4).eq.'52')).or. &
           (grid(1:2).eq.'KJ' .and. ((grid(3:3).gt.'/' .and. grid(3:3).lt.'6'.and.grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
            grid(3:4).eq.'25' .or. grid(3:4).eq.'35')) .or. &
           (grid(1:2).eq.'JJ' .and. ((grid(3:3).gt.'8' .and. grid(3:3).lt.':'.and.grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
            grid(3:4).eq.'80' .or. grid(3:4).eq.'81' .or. grid(3:4).eq.'95')) .or. &
           (grid(1:2).eq.'KH' .and. ((grid(3:3).gt.'0' .and. grid(3:3).lt.'5'.and.grid(4:4).gt.'7'.and.grid(4:4).lt.':').or. &
            grid(3:4).eq.'37' .or. grid(3:4).eq.'47' .or. grid(3:4).eq.'46'))) lgvalid=.true.

! Burundi 9U
      else if(callsign(1:2).eq.'9U') then
        if(grid(1:2).eq.'KI' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.'8') &
          lgvalid=.true.

! Rwanda 9X
      else if(callsign(1:2).eq.'9X') then
        if(grid(1:2).eq.'KI' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'6' .and. grid(4:4).gt.'6'.and.grid(4:4).lt.'9') &
          lgvalid=.true.

! Spratly Islands 1S,9M0,BM9S,BN9S,BO9S,BP9S,BQ9S,BU9S,BV9S,BW9S,BX9S
      else if(callsign(1:3).eq.'9M0') then
      if((grid(1:2).eq.'OK' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'2').or. &
         (grid(1:2).eq.'OJ' .and. grid(3:3).gt.'3' .and. grid(3:3).lt.'9' .and. grid(4:4).gt.'6' .and. grid(4:4).lt.':')) &
        lgvalid=.true.

! Iran 9B..9D,EP,EQ
      else if(callsign(2:2).gt.'A' .and. callsign(2:2).lt.'E') then
! CE3TSK 2026-10-13: land that this rule refused - Iran
        if(grid(1:4).eq.'LM23' .or. grid(1:4).eq.'MM07') lland=.true.
        if((grid(1:2).eq.'LM' .and. grid(3:3).gt.'1' .and. grid(3:3).lt.':') .or. &
           (grid(1:2).eq.'LL' .and. ((grid(3:3).gt.'4' .and. grid(3:3).lt.':'.and.grid(4:4).gt.'5'.and.grid(4:4).lt.':').or. &
            grid(3:4).eq.'49' .or. grid(3:4).eq.'75' .or. grid(3:4).eq.'85' .or. grid(3:4).eq.'95')) .or. &
           (grid(1:2).eq.'ML' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'4'.and.grid(4:4).lt.':'.and. &
            grid(3:4).ne.'15' .and. grid(3:4).ne.'19') .or. &
           (grid(1:2).eq.'MM' .and. grid(3:3).eq.'0' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'7')) &
          lgvalid=.true.

if(lgvalid) then
        if(grid(1:2).eq.'LM' .and. ((grid(3:3).eq.'2' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'4') .or. &
           (grid(3:3).gt.'4' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'7'.and.grid(4:4).lt.':' .and. &
            grid(3:4).ne.'78' .and. grid(3:4).ne.'88'))) lgvalid=.false.
endif

! Ethiopia 9E,9F,ET
      else if(callsign(1:2).eq.'9E' .or. callsign(1:2).eq.'9F') then
! CE3TSK 2026-10-13: land that this rule refused - Ethiopia
        if(grid(1:4).eq.'LJ03') lland=.true.
        if((grid(1:2).eq.'LJ' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'4' .and. grid(4:4).gt.'3'.and.grid(4:4).lt.':').or. &
           (grid(1:2).eq.'LK' .and. grid(3:3).gt.'/' .and. grid(3:3).lt.'2' .and. grid(4:4).gt.'/'.and.grid(4:4).lt.'5').or. &
           (grid(1:2).eq.'KJ' .and. grid(3:3).gt.'5' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'2'.and.grid(4:4).lt.':').or. &
           (grid(1:2).eq.'KK' .and. grid(3:3).gt.'6' .and. grid(3:3).lt.':' .and. grid(4:4).gt.'/' .and. grid(4:4).lt.'5')) &
          lgvalid=.true.

      endif ! '9'

  endif ! end of block 1..9
endif

! CE3TSK 2026-10-13: land that a rule refused. A square is the country's when the country reaches into it - the table
! is to catch squares that lie outside a country altogether - and 121 copies of rules left out squares their country
! has land in: strips along a border or a coast, islands, and whole regions (Libya's coast, Alaska's north coast,
! Mozambique's north, the south of Myanmar). Each such copy sets lland for its squares, and here the square is
! accepted, whatever the rule's own exclusions said. Measured, not read: every call form against its country's
! squares (geodata/dxcc_grids.json, Natural Earth), kept where the land reaches more than 1 km into the square.
! Left as they were: Asiatic and European Russia, Canada and Greenland, where it is the empty north.
if(lland) lgvalid=.true.

  return
end subroutine chkgrid
