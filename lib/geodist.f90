! This source code file was last time modified by Igor UA3DJY on February 3rd, 2017
! All changes are shown in the patch file coming together with the full JTDX source code.

subroutine geodist(Eplat,Eplon,Stlat,Stlon,Az,Baz,Dist)
  implicit none
  real eplat, eplon, stlat, stlon, az, baz, dist

! JHT: In actual fact, I use the first two arguments for "My Location",
!     the second two for "His location"; West longitude is positive.

!      Taken directly from:
!      Thomas, P.D., 1970, Spheroidal geodesics, reference systems,
!      & local geometry, U.S. Naval Oceanographi!Office SP-138,
!      165 pp.
!      assumes North Latitude and East Longitude are positive

!      EpLat, EpLon = End point Lat/Long
!      Stlat, Stlon = Start point lat/long
!      Az, BAz = direct & reverse azimuith
!      Dist = Dist (km); Deg = central angle, discarded

  real BOA, F, P1R, P2R, L1R, L2R, DLR, T1R, T2R, TM,          &
       DTM, STM, CTM, SDTM,CDTM, KL, KK, SDLMR, L,                &
       CD, DL, SD, T, U, V, D, X, E, Y, A, FF64, TDLPM,           &
       HAPBR, HAMBR, A1M2, A2M1

  real AL,BL,D2R,Pi2
  real AzS,BazS,DistS                 ! CE3TSK 2026-10-14: the spherical answer, compared and used near the antipode

  data AL/6378206.4/              ! Clarke 1866 ellipsoid
  data BL/6356583.8/
!      real pi /3.14159265359/
  data D2R/0.01745329251994/      ! degrees to radians conversion factor
  data Pi2/6.28318530718/

  if(abs(Eplat-Stlat).lt.0.02 .and. abs(Eplon-Stlon).lt.0.02) then
     Az=0.
     Baz=180.0
     Dist=0
     go to 999
  endif

  if(abs(Eplat-Stlat).lt.0.02 .and. abs(Eplon-Stlon).lt.0.02) then
     Az=0.
     Baz=180.0
     Dist=0
     go to 999
  endif

  BOA = BL/AL
  F = 1.0 - BOA
! Convert st/end pts to radians
  P1R = Eplat * D2R
  P2R = Stlat * D2R
  L1R = Eplon * D2R
  L2R = StLon * D2R
  DLR = L2R - L1R                 ! DLR = Delta Long in Rads
  T1R = ATan(BOA * Tan(P1R))
  T2R = ATan(BOA * Tan(P2R))
  TM = (T1R + T2R) / 2.0
  DTM = (T2R - T1R) / 2.0
  STM = Sin(TM)
  CTM = Cos(TM)
  SDTM = Sin(DTM)
  CDTM = Cos(DTM)
  KL = STM * CDTM
  KK = SDTM * CTM
  SDLMR = Sin(DLR/2.0)
  L = SDTM * SDTM + SDLMR * SDLMR * (CDTM * CDTM - STM * STM)
  CD = 1.0 - 2.0 * L
  DL = ACos(CD)
  SD = Sin(DL)
  T = DL/SD
  U = 2.0 * KL * KL / (1.0 - L)
  V = 2.0 * KK * KK / L
  D = 4.0 * T * T
  X = U + V
  E = -2.0 * CD
  Y = U - V
  A = -D * E
  FF64 = F * F / 64.0
  Dist = AL*SD*(T -(F/4.0)*(T*X-Y)+FF64*(X*(A+(T-(A+E)                 &
       /2.0)*X)+Y*(-2.0*D+E*Y)+D*X*Y))/1000.0
  TDLPM = Tan((DLR+(-((E*(4.0-X)+2.0*Y)*((F/2.0)*T+FF64*               &
       (32.0*T+(A-20.0*T)*X-2.0*(D+2.0)*Y))/4.0)*Tan(DLR)))/2.0)
  HAPBR = ATan2(SDTM,(CTM*TDLPM))
  HAMBR = Atan2(CDTM,(STM*TDLPM))
  A1M2 = Pi2 + HAMBR - HAPBR
  A2M1 = Pi2 - HAMBR - HAPBR

! CE3TSK 2026-10-14 (review TODO 3.-72): near the antipode and near a pole the formula above breaks down -
! ACos of a cosine rounded past -1, DL/SD with SD = 0, KK*KK/L with L = 0 - and gives NaN. A NaN azimuth never
! satisfies the two tests below, and the loops at 1 and 5 then ran FOR EVER: the program hung, in the GUI
! thread, scoring a decode with such a grid in a contest or showing the DX grid's distance (PL05-GG04,
! FG04-OL05, JA00-JA00KL, MR39qx-AA40ba). A result that is not finite, or a distance beyond half the globe,
! is now taken from a spherical great circle instead (geodist_sphere below); every other pair keeps its
! result to the bit.
! CE3TSK 2026-10-14 (reviews 3.-79, 3.-81): near the antipode the formula also gives FINITE results that are wrong -
! measured against the exact geodesic (Karney, Clarke 1866) on a million near-antipodal pairs: by up to 2588 km, and
! 180 degrees in azimuth (AE08-JN01 20609 km and JC98xq-AP91xg 20100 km are 19999 km, NI09-EJ00 19924 km is 19999).
! No two points of the ellipsoid are farther apart than its half meridian, 20003.8 km. So the sphere answers also
! when the formula says 20004 km or more, or the sphere itself says 19990 km or more - the antipode's neighbourhood:
! the worst distance error of the probe fell to 13.3 km (169 km with a cut on the formula alone at 19990 km). The
! sphere is not better everywhere there - near opposite poles the formula was within 3 km and the sphere is up to
! 11 km off (review 3.-82) - but its worst is far smaller. Every other pair keeps the formula's result to the bit.
  call geodist_sphere(Eplat,Eplon,Stlat,Stlon,AzS,BazS,DistS)
  if(.not.(abs(A1M2).lt.20.0 .and. abs(A2M1).lt.20.0 .and. Dist.ge.0.0 .and. Dist.lt.20004.0 .and. DistS.lt.19990.0)) then
     Az=AzS; Baz=BazS; Dist=DistS
     go to 999
  endif

1 If ((A1M2 .ge. 0.0) .AND. (A1M2 .lt. Pi2)) GOTO 5
  If (A1M2 .lt. Pi2) GOTO 4
  A1M2 = A1M2 - Pi2
  GOTO 1
4 A1M2 = A1M2 + Pi2
  GOTO 1

! All of this gens the proper az, baz (forward and back azimuth)

5 If ((A2M1 .ge. 0.0) .AND. (A2M1 .lt. Pi2)) GOTO 9
  If (A2M1 .lt. Pi2) GOTO 8
  A2M1 = A2M1 - Pi2
  GOTO 5
8 A2M1 = A2M1 + Pi2
  GOTO 5

9 Az = A1M2 / D2R
  BAZ = A2M1 / D2R

!Fix the mirrored coords here.
  
  az = 360.0 - az
  baz = 360.0 - baz

999 return

contains

! CE3TSK 2026-10-14: the great circle on a sphere of the mean radius, in the conventions of the formula above
! (west longitude positive; Az from the first point to the second, Baz back). Finite for every input: the
! haversine's argument is held in [0,1], and ATan2 (0,0) is 0 - at the antipode itself every azimuth is right.
  subroutine geodist_sphere(Eplat,Eplon,Stlat,Stlon,Az,Baz,Dist)
    real, intent(in) :: Eplat,Eplon,Stlat,Stlon
    real, intent(out) :: Az,Baz,Dist
    real*8 p1,p2,l1,l2,dl,h,c
    real*8, parameter :: rad=0.017453292519943295d0, R=6371.0d0
    p1=Eplat*rad; p2=Stlat*rad
    l1=-Eplon*rad; l2=-Stlon*rad                  ! east longitude
    dl=l2-l1
    h=sin((p2-p1)/2)**2 + cos(p1)*cos(p2)*sin(dl/2)**2
    h=min(1.d0,max(0.d0,h))
    c=2*atan2(sqrt(h),sqrt(1-h))
    Dist=real(R*c)
    Az=real(modulo(atan2(sin(dl)*cos(p2),cos(p1)*sin(p2)-sin(p1)*cos(p2)*cos(dl))/rad,360.d0))
    Baz=real(modulo(atan2(-sin(dl)*cos(p1),cos(p2)*sin(p1)-sin(p2)*cos(p1)*cos(dl))/rad,360.d0))
  end subroutine geodist_sphere

end subroutine geodist
