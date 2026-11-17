# geodata - geography compiled into the program

Two lookup tables, linked into the program from generated `.cpp` files, the generator of the US licence
state file, and the world map the Reception map window draws. This page says how to regenerate them.

| file | what |
|---|---|
| `geodata.h` / `.cpp` | the API over the tables, hand-written |
| `dxcc_grids_data.h` / `.cpp` | **generated** - the grid squares each DXCC entity occupies |
| `grid_states_data.h` / `.cpp` | **generated** - the US state or states of each grid square |
| `dxcc_grids.json`, `grid_states.json` | the source tables the C++ is generated from |
| `make_geodata_headers.py` | json -> C++ |
| `geodata_check.py`, `geodata_selftest.cpp`, `geodata_dump.cpp` | the checker and the two programs it compiles |
| `make_dxcc_grids.py`, `extract_admin1_subset.py`, `cty.py` | geography -> `dxcc_grids.json` |
| `make_grid_states.py` | geography -> `grid_states.json` |
| `make_license_states.py` | the FCC licence file -> `../us-license-states.txt` |
| `worldmap.bin` | **generated** - land, borders and lakes for View > Reception map, carried in the program as `:/worldmap.bin` |
| `make_world_map.py` | Natural Earth -> `worldmap.bin` |

The generated files are built, never edited by hand.

## The C++ from the json

The json is in this folder, so this step needs nothing else:

    python3 make_geodata_headers.py      # rewrites the four *_data.{h,cpp} files from the json
    python3 geodata_check.py             # compiles them and compares every row with the json

Run the checker after every regeneration: the build does not compare the `.cpp` files with the json.

## The json from the geography

The source data is not shipped here: download the files listed under Data sources below and put them in
this folder. Each script names the file it wants if it is missing.

| table | script | input |
|---|---|---|
| `dxcc_grids.json` | `make_dxcc_grids.py` | `cty.dat`; Natural Earth admin-0 map units and minor islands; the US Census state boundaries (K, KL and KH6 are cut from them); `dxcc_admin1_subset.json`, the admin-1 subset below |
| `dxcc_admin1_subset.json` | `extract_admin1_subset.py` | Natural Earth admin-1 states and provinces |
| `grid_states.json` | `make_grid_states.py` | US Census state boundaries and centres of population |

    python3 extract_admin1_subset.py path/to/ne_10m_admin_1_states_provinces
    python3 make_dxcc_grids.py
    python3 make_grid_states.py

Then generate the C++ and run the checker as above. The scripts need `pyshp` and `shapely`.

The shipped `dxcc_grids.json` was built from the cty.dat of its build date (the json's `version`). A fresh
download gives a slightly different table: AD1C revises cty.dat several times a year. The checker cannot notice
the difference - it compares the C++ with the json, not the json with cty.dat.

## The US licence state file

`../us-license-states.txt` - the state of each US amateur licence, bundled with the program and offered for
download under Settings > General > Data files - is built from the FCC's weekly Universal Licensing System
file of the Amateur Radio Service, https://data.fcc.gov/download/pub/uls/complete/l_amat.zip (about 200 MB,
renewed every Sunday):

    python3 make_license_states.py path/to/l_amat.zip     # writes ../us-license-states.txt

It keeps every active licence and those that ended within the two years before the file, takes the state of
the licensee's mailing address, counts Washington DC as Maryland, and keeps only the states that share a grid
square with another state (read from `grid_states.json`). The run stops if the FCC file's format has
changed.

## The world map

`worldmap.bin` is what View > Reception map draws: the land with its borders at two levels of detail - Natural
Earth's 1:50m for the whole world, its 1:10m with the minor islands once zoomed in - and the lakes. It is built
from four Natural Earth layers (Data sources below), each as published and only rounded to 1e-4 degree; the
script's header describes the file. Map UNITS, not countries: they cut the United Kingdom into England, Scotland,
Wales and Northern Ireland, as DXCC does.

    python3 make_world_map.py            # writes worldmap.bin beside itself; needs pyshp

Put the four zip files in this folder first; the script names those it misses and where they come from. The
same layers always give the same file, byte for byte. The program reads the file with `../worldmap.h` and
refuses one that is not what its header says.

## Data sources

| data | file | link | rights |
|---|---|---|---|
| DXCC entities and prefixes | `cty.dat` (the big version), by Jim Reisert AD1C | http://www.country-files.com/bigcty/cty.dat | MIT licence |
| country borders | Natural Earth 1:10m admin-0 map units | https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_0_map_units.zip | public domain |
| small islands | Natural Earth 1:10m minor islands | https://naciscdn.org/naturalearth/10m/physical/ne_10m_minor_islands.zip | public domain |
| the world map's whole-world level | Natural Earth 1:50m admin-0 map units | https://naciscdn.org/naturalearth/50m/cultural/ne_50m_admin_0_map_units.zip | public domain |
| the world map's lakes | Natural Earth 1:50m lakes | https://naciscdn.org/naturalearth/50m/physical/ne_50m_lakes.zip | public domain |
| provinces that are entities of their own | Natural Earth 1:10m admin-1 states and provinces | https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_1_states_provinces.zip | public domain |
| US state boundaries (both tables) | US Census Bureau cartographic boundaries, 1:20m, 2023 | https://www2.census.gov/geo/tiger/GENZ2023/shp/cb_2023_us_state_20m.zip | US Government, public domain |
| US population | US Census Bureau 2020 centres of population by tract | https://www2.census.gov/geo/docs/reference/cenpop2020/tract/CenPop2020_Mean_TR.txt | US Government, public domain |
| US amateur licences | FCC Universal Licensing System, Amateur Radio Service, weekly complete file | https://data.fcc.gov/download/pub/uls/complete/l_amat.zip | US Government public record |
| positions of the Antarctic stations | Wikidata, the item beside each station in `make_dxcc_grids.py` (`ANTARCTIC_BASES`) | https://www.wikidata.org/ | CC0 |

Each generated table carries a note with its source.
