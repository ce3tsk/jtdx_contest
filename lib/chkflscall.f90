subroutine chkflscall(call_a,call_b,falsedec)

  use ft8_mod1, only : lallcall7,LALLCALL7_FILTER   ! CE3TSK: JTDX_ALLCALL7_FILTER, for tests of the gated paths; the source switch (in the module since 2026-09-05, cwfilter's load reads it too)
  character*12 call_a,call_b
  logical(1) falsedec,lfound
! CE3TSK: source-level switch, not exposed in the GUI or the ini. The ALLCALL7.TXT lookup
! rejects a message whose callsigns are all absent from the file; with the July 2024 file
! that is every QSO between two stations licensed since (measured on the 240 on-air
! periods: 44 of 5267 classical decodes and 53 of 6095 light-preset decodes thrown away,
! all but three or four of them real - RI1FJL KN6JIB DM13, ER35MD N7EYE DM33 at -13 dB -
! for one or two false decodes an hour). Off: chkflscall never flags anything, the file
! may stay in place. Set LALLCALL7_FILTER (ft8_mod1.f90) to .true. to restore the lookup.

  falsedec=.false.
  if(.not.(LALLCALL7_FILTER .or. lallcall7)) return
  if(call_a(1:2).eq.'<.') return
  falsedec=.true.; lfound=.false.
!print *,"11","'"//call_a//"'","'"//call_b//"'"
  if(call_a.eq.'MYCALL      ' .or. call_a.eq.'CQ          ') then
    call searchcalls(call_b,"            ",lfound); if(lfound) falsedec=.false.
  else if(call_b(1:1).eq.'<') then
    call searchcalls(call_a,"            ",lfound); if(lfound) falsedec=.false.
  else
    call searchcalls(call_a,call_b,lfound); if(lfound) falsedec=.false.
  endif

  return
end subroutine chkflscall