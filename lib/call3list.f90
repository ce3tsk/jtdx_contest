! CE3TSK 2026-10-23 (TODO.md 3.-199): the calls and squares of CALL3.TXT for the Hint decoders - cqcall3 (JT65's CQ
! hints and the FTRSD check), hintwidedx, hintrxgrid (JT65), hintrxgrid9 (JT9), hintrxgrid10 (JT10).
!
! Each of them read the file itself, the first MAXCALLS=35000 LINES of it, and cut every call to six characters ("Fix
! for compound callsigns!"). The bundled file of 2016-2017 had 34 323 standard calls, so that was all of it; the one of
! 2026-09-20 (wsjt-x-improved.sourceforge.io, also Settings > Colors' download) has 202 183 entries, sorted, and the
! first 35000 lines ended at EA8BJ - F, G, JA, K, W ... unknown to the hints - and its compound calls (1B/TA1HZ) came
! out as 1B/TA1. Here the whole file is read; a line is taken as the readers took it (comment lines '//' skipped, a
! line starting ZZZZ ends the list, the call 3 characters at least, the square 4 - its first 4 kept), and a call longer
! than 6 characters or with a '/' is skipped, not cut: a hint message cannot carry it with a square. The lists are
! allocated to the file's size (the readers' fixed arrays lived on the decoder thread's stack); a missing file is an
! empty list - it is no longer created empty (status 'unknown'), which kept the program's first-start copy away.
module call3list

  implicit none
  integer, parameter :: MAXCALL3 = 1000000   ! a bound against a runaway file: five times the 2026 list

contains

  subroutine read_call3 (path, calls, grids, n)
    character(len=*), intent(in) :: path
    character(len=6), allocatable, intent(out) :: calls(:)
    character(len=4), allocatable, intent(out) :: grids(:)
    integer, intent(out) :: n
    integer :: lu, pass, ios, i1, i2, m
    character(len=180) :: line
    character(len=12) :: callsign

    n = 0
    open (newunit=lu, file=path, status='old', action='read', iostat=ios)
    if (ios /= 0) then
       allocate (calls(0), grids(0))
       return
    endif
    do pass = 1, 2   ! count, then fill
       if (pass == 2) then
          allocate (calls(n), grids(n))
          rewind (lu)
       endif
       m = 0
       do
          read (lu, '(a)', iostat=ios) line
          if (ios /= 0) exit
          if (line(1:4) == 'ZZZZ') exit
          if (line(1:2) == '//') cycle
          i1 = index (line, ',')
          if (i1 < 4) cycle
          i2 = index (line(i1+1:), ',')
          if (i2 < 5) cycle
          callsign = line(1:i1-1)
          if (i1 - 1 > 6 .or. index (callsign, '/') > 0) cycle
          if (m >= MAXCALL3) exit
          if (pass == 2 .and. m >= n) exit   ! the file grown since the count (a download landing then)
          m = m + 1
          if (pass == 2) then
             calls(m) = callsign(1:6)
             grids(m) = line(i1+1:i1+4)
          endif
       enddo
       n = m
    enddo
    close (lu)
  end subroutine read_call3

end module call3list
