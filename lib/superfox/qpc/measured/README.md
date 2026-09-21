# The SuperFox code definition, measured

`../qpc_measured.c` tells the polar encoder/decoder (`../np_qpc.c`, GPL v3) WHICH code SuperFox
uses: the positions of the 50 message symbols among the 128 inputs of the polar transform, in
message order. WSJT-X ships the same information in `qpc_n127k50q128.c`, a file whose author
reserves its tables; that file is not part of this program. The numbers here were measured from
the tone sequence of ONE transmission of WSJT-X 3.0.2's own SuperFox transmit tool (written to a
file; nothing went on the air) - they are properties of what a SuperFox transmitter sends, and
any receiver of the mode must use exactly these.

| file | what |
| --- | --- |
| `measure.py` | `bootstrap`: all of the following, from nothing. `find`: look for one transmission whose 50 message symbols are all different and non-zero (packer only). `measure`: ONE run of WSJT-X's `sftx` + one of `packprobe`, the arithmetic, `observation.txt` and `../qpc_measured.c`. `regen`: write `../qpc_measured.c` again from `observation.txt` and the files here, no WSJT-X needed (after editing a text of this directory, which the C file copies). `check`: the generated file, the record and the copies agree; with `--wsjtx DIR` the measurement is made again and confirmed on 24 further transmissions. The method is in its opening text |
| `packprobe.f90` | WSJT-X's `sftx.f90` up to its call of `sfox_pack`, stopped before the encoder: prints the 50 message symbols |
| `build_packprobe.sh DIR` | builds it against WSJT-X's own library (`DIR` = a WSJT-X 3.0.2 build from source) |
| `sfox_1.dat`, `probe_args.txt` | the transmission: five old-style Fox messages (nine Hounds), the Fox call, the key, and how many single-field changes the guided search of `find` tried. The call signs are random strings. (`sftx` 3.0.2 takes the Fox's call from the messages; its second argument is read and not used) |
| `observation.txt` | the record: the 151 tones, the 50 message symbols, the transform's output, the result |

Every file of this table is also copied, line for line, into the `#if 0` at the end of
`../qpc_measured.c`. `.gitignore` keeps `packprobe_build/` out of a repository: a binary built
there names the paths of the machine it was built on.

**No claim is made on the table.** The numbers were read off a transmitted signal; neither CE3TSK
nor JTDX_CONTEST claims a copyright or any other right in them. What authorship is claimed for is
the measuring programs and their record - the files of this directory, which are also the part of
`../qpc_measured.c` between its `#if 0` and `#endif`: `measure.py`, `build_packprobe.sh` and the
texts under the GNU GPL, version 3 or later; `packprobe.f90`, derived from WSJT-X's `sftx.f90` by
Joseph Taylor K1JT, under the GPL version 3 as WSJT-X is.

To make `../qpc_measured.c` again FROM NOTHING: build WSJT-X 3.0.2 from its sources, then

    python3 measure.py bootstrap --wsjtx <the directory WSJT-X was built in>

It builds `packprobe` (into `./packprobe_build`), finds the transmission if `sfox_1.dat` is not
there (`--fresh`: even if it is), runs `sftx` once, writes `observation.txt` and
`../qpc_measured.c`, and checks the result - including on 24 further transmissions it was not
made from. `check` alone verifies a file that EXISTS (without `--wsjtx` it needs no WSJT-X);
`measure` is the step that writes it.
