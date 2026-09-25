# geodata - geography compiled into the program

Two lookup tables and the small API over them. Nothing here is read from disk at run time: the data is in
the generated `.cpp` files and linked in.

| file | what |
|---|---|
| `geodata.h` / `.cpp` | the API, hand-written: pack a grid square, its neighbours, does this entity occupy it, which US states does it cover |
| `dxcc_grids_data.h` / `.cpp` | **generated** - 346 DXCC entities, 15 859 entity-square pairs; 31 KB of squares, 44 KB compiled |
| `grid_states_data.h` / `.cpp` | **generated** - 723 grid squares, 1 005 square-state entries, 11 KB compiled |
| `geodata_selftest.cpp` | a standalone self-test, not part of the program |
| `geodata_dump.cpp` | prints every entity and square as the compiled tables see them, for the checker |
| `dxcc_grids.json`, `grid_states.json` | the source tables the generator reads |
| `make_geodata_headers.py`, `geodata_check.py` | json -> C++, and the checker |
| `make_dxcc_grids.py`, `make_grid_states.py`, `extract_admin1_subset.py`, `cty.py` | geography -> json; they need the source data, see below |

**The generated files are built, not edited.** There are two levels, and every script for both is here:

    geography -> json        make_dxcc_grids.py, make_grid_states.py, extract_admin1_subset.py, cty.py
    json -> C++              make_geodata_headers.py, then geodata_check.py to verify

**Level 2 runs here as it stands** - the json is in this folder, so the C++ can be rebuilt and checked from
the public tree alone:

    cd geodata
    python3 make_geodata_headers.py      # rewrites the four *_data.{h,cpp} files from the json here
    python3 geodata_check.py             # compiles them and compares every row with the json

**Level 1 needs the geography**, which is not shipped here (about 9 MB): `cty.dat`, the US Census state
boundaries and centres of population, and two Natural Earth layers. Each script says exactly which file it
wants and where to fetch it if you run it without one. Drop them in this folder - or run the scripts from
the work tree, where they already sit - and:

    python3 make_dxcc_grids.py           # 346 entities -> the squares they occupy
    python3 make_grid_states.py          # grid square -> US state(s), by people and by area

Run from the work tree instead (`python3 tools/make_dxcc_grids.py`) and they read and write the work
tree's `geodata/` - **one directory, not both trees**: level 1 writes the json where its sources are, and
copying it into the two program trees is a separate step. Level 2 is the one that writes both trees, which
is why `make_geodata_headers.py` run from `tools/` updates `jtdx_contest/geodata/` and `src/geodata/` at
once. The json files here are copies of the work tree's, which is where they are built from cty.dat and the
Natural Earth and Census sources (`geodata/README.md` there).

The tables themselves - where the geography comes from, how each entity's squares were derived and how far
each row can be trusted - are documented in the work tree's `geodata/README.md`. Every row carries a
`source` (`poly`, `admin1`, `islands`, `point`, `split`, `sovereign`); a `point` row is a 120 km disk around
a coordinate, not a border, and a consumer that wants strictness should say so.

## Packing

A 4-character Maidenhead square is one of 18 x 18 x 10 x 10 = 32 400 values, so it fits a `uint16_t`:

    code = ((fieldLon * 18 + fieldLat) * 10 + squareLon) * 10 + squareLat

Each entity's squares are sorted, so membership is a binary search - at most 3 327 values (Antarctica).
Everything is read-only and safe to call from any thread.

## Four rules the API keeps

- **Never judge what it cannot judge.** `gridFitsEntity` returns true for an unknown entity, an
  unparsable grid, **and an entity the table holds no squares for**. The caller is asking "is this
  implausible?", and silence is not evidence.
- **One administration, several entities.** `gridFitsEntity` accepts any square of the entity's group
  (`US` covers K, KL, KH6, KP4 ...; `RU` covers European and Asiatic Russia and Kaliningrad) unless the
  caller passes `sameAdmin = false`. A KL7 holder living in Ohio is not an impossible station. Measured:
  it halves the real decodes the test would reject and costs 2.8 % of its detections, all of which the
  other conditions catch - see the work tree's `geodata/README.md`.
- **Upper case, as the air is.** `packGrid` takes `FF46`, not `ff46`; a rejected grid reads as
  "no answer", which for `statesOfGrid` and `gridPopulation` is indistinguishable from "nothing here".
- **Tolerance is asked for, not assumed.** By default the eight neighbouring squares are accepted too,
  because a square is only 110-220 km wide and operators send neighbouring locators; pass `false` for the
  strict test.

## In the build, not yet called

`geodata.cpp`, `dxcc_grids_data.cpp` and `grid_states_data.cpp` are listed in `CMakeLists.txt` and compile
with the program (2026-09-24). Nothing calls the API yet, so with `-Wl,--gc-sections` the linker drops all
of it again and the binary does not grow - the wiring is in place for the feature that will use it, and the
`version` / `content` stamps are there for **Help - About**. The external-file override is still to come;
the work tree's `geodata/README.md` describes it under "How this reaches the program".

`geodata_selftest.cpp` and `geodata_dump.cpp` are deliberately **not** in `CMakeLists.txt` - they have
their own `main()`. `geodata_check.py` compiles both with `-Wall -Wextra -Werror` on every run, so they
cannot rot unnoticed.

**One thing the build does not check**: nothing ties the generated `.cpp` to the `.json` beside it. Edit
the json and the next build happily links the old table; only `geodata_check.py` notices, and no test runs
as part of a build in this project. Regenerating and running the checker after touching a table is a step
a person has to take.

## Review, 2026-09-24

This code was reviewed independently the day it was written. **The hand-written C++ came back clean**:
exhaustive ASan+UBSan sweeps over all 32 400 valid and 33 136 invalid codes found nothing, `-Wconversion
-Wsign-conversion -Wshadow -Wformat=2 -Wcast-qual` are silent, `-pedantic-errors` C++11 passes, every array
matches its declared count (checked from the object files, not by counting commas), and the generator is
byte-stable. Four things were fixed:

1. **The generator did not validate the states path** the way it validates the dxcc one. A bad square key
   became `-1` and a share of 1.004 became `256` in `uint8_t` arrays - a narrowing error at best, and with
   `-Wno-narrowing` a silently corrupted table whose binary search then returns wrong answers. It now
   refuses both.
2. **`gridFitsEntity` judged an entity it holds no squares for.** Unreachable today - the builder refuses
   to emit an empty entity - but it would have turned a new entity into "this station is impossible",
   which is the opposite of the rule.
3. **`near` is a macro in the Windows SDK** (`minwindef.h`). The local array is now `around`.
4. **The header promised more than the code did**: `unpackGrid` writes one byte for a code no square has,
   `packGrid` is upper-case only, and `statesOfGrid` returning 0 means "no answer", not "no states". The
   comments now say so. `entityIndex` also became a binary search, the generator having been taught to
   guarantee the prefix order it needs.

## Second review, 2026-09-24 (after the build wiring)

A second independent pass went over the wiring and the API again. It confirmed the data: `prefix[]`
strictly ascending under `strcmp`, every `offset[]` monotone and ending exactly at its count, squares
ascending within each entity, every index inside its array, no mutable state anywhere, and both
`geodata_selftest.cpp` and `geodata_dump.cpp` still compiling clean under `-Wall -Wextra -Werror`. **One
real defect came out of it, and it mattered:**

**`entityIndex` was case-sensitive, and the program's own resolver upper-cases what it returns.** cty.dat
spells twenty-nine primary prefixes with a lower-case suffix - `3Y/b` Bouvet, `3Y/p` Peter I, `VP8/o` South
Orkney, `SV/a` Mount Athos, `FT/x` Kerguelen, `JD/m` Minami Torishima and the rest - while
`CountryDat::_extractMasterPrefix` ends in `toUpper()`. Fed from there, `entityIndex("VP8/O")` returned
-1, and since "unknown entity" means **never judge**, the plausibility test would have been silently off
for exactly the rare entities a doubtful decode most often claims. The exact binary search is still the
fast path; when it misses, a case-insensitive scan of the 346 prefixes follows (no two differ by case
alone, so a hit is unambiguous). `geodata_check.py` now tests both spellings of five prefixes, because
nothing in the old test set used a lower-case one.

A fourth round then reviewed those fixes, and found that `geodata_check.py` had been taking the self-test
on trust: it ignored the exit code and asserted nothing about which lines came back, so a self-test that
crashed, was truncated, or simply lost the new block read as "all match". It now checks the exit code and
the whole inventory of expected lines, and each `case` line carries the prefix that was actually matched -
two spellings agreeing on the *wrong* entity would have passed before. The generator refuses a table in
which two prefixes differ by case alone, since that is what makes the fallback unambiguous, and the
self-test covers the two shapes a sloppy comparison gets wrong: a prefix that is a strict prefix of another
(`K`, `KH6`, `KL`) and one starting with punctuation (`*GM/s`).

Three documentation faults were fixed with it: this file still said the component was not in the build;
the generated banners and the header pointed at `tools/` and `test/`, which exist only in the author's work
tree and not in the tree these files ship in; and the static-footprint figures counted the square
arrays only. Measured from the object files, the two tables together are **55 KB** - 44 KB of entities and
squares, 11 KB of grid-square-to-state - and the earlier 31 KB / 7 KB / 4.4 KB each counted one array of
several.
