#!/usr/bin/env python3
"""Which 4-character Maidenhead grid squares does each DXCC entity occupy?

Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software:
you may redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 or (at your option) any later version.
It is distributed WITHOUT ANY WARRANTY.  The TABLES it writes carry their own note on rights.

    python3 tools/make_dxcc_grids.py

Writes geodata/dxcc_grids.txt and .json: one entry per entity, keyed by the cty.dat primary prefix,
listing the squares it occupies.  The plausibility test a caller wants is then a hash lookup - "is
the grid this station claims one of its entity's squares (or a neighbour of one)?" - with no
geometry at run time.

The sources, all open (SESSION_RULES.md rule 6b):
  - entity list and coordinates: geodata/cty.dat (MIT), the copy the tables were built from
  - borders: Natural Earth 1:10m admin_0_map_units (public domain), kept in geodata/
    https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_0_map_units.zip
  - K, KL and KH6: the US Census state boundaries (public domain), kept in geodata/
  - the positions of the Antarctic stations: Wikidata (CC0), in the list ANTARCTIC_BASES below

WHERE IT IS APPROXIMATE, and it says so per entity:
  - 'poly'  the entity matched a Natural Earth map unit; its squares come from the polygon.
  - 'sovereign' only the SOVEREIGNT field matched, so the row is the whole sovereign state including
            its dependencies - wide, and weak as a test.  Reported at the end of a run.
  - 'admin1' it is a subdivision that is a DXCC entity of its own (the Canaries, Sardinia, Sicily,
            Kaliningrad, Svalbard ...), cut from Natural Earth's admin-1 layer once by
            tools/extract_admin1_subset.py into a small file kept here.
  - 'islands' it is none of those, but Natural Earth's minor-islands layer has island polygons within
            ISLAND_KM of its cty.dat coordinate; its squares come from those.  The layer carries no
            names, so the match is geographic - which is why the radius is kept modest.
  - 'point' neither (a scattered archipelago, a subdivision such as the Canaries, or one of the
            single-building entities).  Its squares come from the cty.dat coordinate and everything
            within POINT_KM of it.
  - A matched PARENT keeps its whole polygon, so Spain still lists the Canary squares that belong to
    EA8.  For a plausibility test that errs on the permissive side, which is the safe direction; a
    later pass can subtract the children.
"""
import os, sys, json, math, hashlib, shapefile
from datetime import date, datetime, timezone
from shapely.geometry import shape, box, Point
from shapely.ops import unary_union
from shapely.strtree import STRtree

HERE = os.path.dirname(os.path.abspath(__file__))
# Run from tools/ in the work tree, or from inside a geodata/ folder that carries the inputs.
DATA = os.path.normpath(HERE if os.path.exists(os.path.join(HERE, 'cty.dat'))
                        else os.path.join(HERE, '..', 'geodata'))
NE   = os.path.join(DATA, 'ne_10m_admin_0_map_units')
MINOR = os.path.join(DATA, 'ne_10m_minor_islands')      # 2795 unnamed island polygons
ADM1  = os.path.join(DATA, 'dxcc_admin1_subset.json')   # the subdivisions that are entities of their own
OUT  = os.path.join(DATA, 'dxcc_grids.txt')
OUTJ = os.path.join(DATA, 'dxcc_grids.json')
POINT_KM     = 120.0     # radius around a cty.dat coordinate when nothing else is found
ISLAND_KM    = 200.0     # how far from that coordinate to accept minor-island polygons
# A square counts if the entity touches it at all: for a plausibility test being permissive is the
# safe direction, and an area threshold silently loses the tiny entities (Spratly, the reefs).
sys.dont_write_bytecode = True          # no __pycache__ beside the sources, in either tree
sys.path.insert(0, HERE if os.path.exists(os.path.join(HERE, 'cty.py'))
                else os.path.join(HERE, '..', 'test', 'experiments', 'phantom_decodes'))
from cty import load_cty, cty_path, gridname, norm, squares_of   # the resolver, and the helpers the gap check shares

BUILT_BY = ('built by tools/make_dxcc_grids.py of JTDX_CONTEST, Tihomir Sokcevic CE3TSK '
            '(the script is GPL v3)')
CLAIM = ('derived from cty.dat (MIT licence), Natural Earth and the US Census state boundaries (public domain) and, '
         'for the positions of the Antarctic stations, Wikidata (CC0), stating facts about geography; no rights of its '
         'own are claimed over it (review 2026-10-04: it called cty.dat public domain).')
# cty.dat spells some entities differently from Natural Earth
ALIAS = {
    'Fed. Rep. of Germany': 'Germany', 'Timor - Leste': 'East Timor', 'Bosnia-Herzegovina':
    'Bosnia and Herzegovina', 'Dem. Rep. of the Congo': 'Democratic Republic of the Congo',
    'Republic of Korea': 'South Korea', 'DPR of Korea': 'North Korea', 'Czech Republic': 'Czechia',
    'Trinidad & Tobago': 'Trinidad and Tobago', 'Antigua & Barbuda': 'Antigua and Barbuda',
    'St. Kitts & Nevis': 'Saint Kitts and Nevis', 'St. Vincent': 'Saint Vincent and the Grenadines',
    'St. Lucia': 'Saint Lucia', 'Turks & Caicos Islands': 'Turks and Caicos Islands',
    'Sao Tome & Principe': 'São Tomé and Principe', 'Wallis & Futuna Islands': 'Wallis and Futuna',
    'Vatican City': 'Vatican',
    'The Gambia': 'Gambia',
    # found by review 2026-09-24: these nine exist in Natural Earth under another name, and without
    # the alias each fell back to a 120 km disk around its cty.dat coordinate - Bratislava was not in
    # Slovakia, Izmir not in Asiatic Turkey, Kuala Lumpur not in West Malaysia.
    'Slovak Republic': 'Slovakia', 'Asiatic Turkey': 'Turkey', 'Cote d\'Ivoire': "Côte d'Ivoire",
    'Republic of Kosovo': 'Kosovo', 'Republic of South Sudan': 'South Sudan',
    'Reunion Island': 'Reunion', 'West Malaysia': 'Malaysia',
    'Western Kiribati': 'Kiribati', 'Central Kiribati': 'Kiribati', 'Eastern Kiribati': 'Kiribati',
    # 2026-10-01 (review): ten dead entries removed - seven spellings cty.dat no longer uses (Burma, Ivory Coast, Cape
    # Verde, Swaziland, Turkey, Macedonia, Rep. of South Africa) and United States / European / Asiatic Russia, which
    # the split branch builds before any alias is read. The guard after the loop now refuses such entries.
    # 2026-10-01: without it KH0 was a 120 km disk around Saipan, and the northern islands (Pagan, Agrihan,
    # Farallon de Pajaros - QK28 QK29 QL20) were outside the Marianas
    'Mariana Islands': 'Northern Mariana Islands',
    # 2026-10-13: two more that Natural Earth has under another name. Each fell to the island layer, which has no
    # names and matches by distance: Guantanamo Bay got FL20, an islet one square north of the base (which is FK29),
    # and Tokelau got Nukunonu's and Fakaofo's square and not Atafu's (AI31)
    'Guantanamo Bay': 'US Naval Base Guantanamo Bay', 'Tokelau Islands': 'Tokelau',
}
# Entities that share one ADMINISTRATION.  DXCC counts them apart - that is what DXCC is for - but a
# station of one may legitimately transmit from another: a KL7 holder living in Ohio, an Asiatic-Russia
# call west of the Urals, a Spanish operator on the Canaries.  Measured over era B (2026-09-24): letting
# a grid of any entity in the group count halves the real decodes this leg would wrongly reject
# (153 -> 78) and costs 17 of 606 false detections, all of which the other conditions catch anyway.
# The table carries the group; whether to use it is the consumer's decision.
# MEASURED per group - era B (labelled truth) and the 2022-2025 archive (60 349 well-attested pairs):
#   RU  rescues 74 real decodes in era B and 34 in the archive, costs 12 false detections
#   US  rescues 53 in the archive (a KL7 holder in Ohio, a KP3 in Texas), costs 5
#   ES, PT, IT  one rescue each in the archive, no cost - the parents mostly contain their children
#   GB  rescued nothing and cost one: DROPPED.  British regional prefixes are not portable anyway -
#       a G station operating in Scotland signs GM, so the group had no basis in practice either.
GROUPS = {
    'US': ['United States', 'Alaska', 'Hawaii', 'Puerto Rico', 'US Virgin Islands', 'Guam',
           'American Samoa', 'Mariana Islands', 'Midway Island', 'Wake Island', 'Johnston Island',
           'Baker & Howland Islands', 'Palmyra & Jarvis Islands', 'Kure Island', 'Navassa Island',
           'Guantanamo Bay', 'Desecheo Island'],
    'RU': ['European Russia', 'Asiatic Russia', 'Kaliningrad'],
    'ES': ['Spain', 'Canary Islands', 'Balearic Islands', 'Ceuta & Melilla'],
    'PT': ['Portugal', 'Azores', 'Madeira Islands'],
    'IT': ['Italy', 'Sardinia', 'Sicily', 'African Italy'],
    'GR': ['Greece', 'Crete', 'Dodecanese', 'Mount Athos'],
    'JA': ['Japan', 'Ogasawara', 'Minami Torishima'],
    'DK': ['Denmark', 'Greenland', 'Faroe Islands'],
    'NO': ['Norway', 'Svalbard', 'Jan Mayen', 'Bear Island'],
    'FR': ['France', 'Corsica'],
    'NL': ['Netherlands', 'Curacao', 'Bonaire', 'Sint Maarten', 'Aruba'],
    # 2026-10-13, not measured (nobody was heard): the Antarctic Treaty area.  cty.dat gives the plain prefixes of
    # Antarctica to one of the three by guess - CE9 and XR9 to the South Shetland Islands, where Chile also has
    # stations on the continent (O'Higgins, Yelcho, Carvajal, Union Glacier)
    'AQ': ['Antarctica', 'South Shetland Islands', 'South Orkney Islands'],   # (not Peter I Island: its calls are its own)
}

# Entities that need more than one map unit.  Natural Earth keeps some regions apart from the state they
# belong to, and nothing else matches them, so the entity row lost them (2026-10-01, found by
# test/geodata_gap_check.py): Somaliland and Puntland - all of northern Somalia, 21 squares, 19 of them not
# even next to a Somalia square; 6O3T in LJ29 (Hargeisa) was heard here 179 times and marked as outside
# Somalia every time - and, harmless only because each missing square neighbours one the row has, Iraqi
# Kurdistan, Vojvodina (Novi Sad, JN95) and Bougainville. The Paracels: no DXCC entity of their own, held by
# China and kept apart by Natural Earth - a Chinese station there sent OK66 and was outside China.
MULTI = {'Palestine': ['West Bank', 'Gaza'],
         'Somalia': ['Somalia', 'Somaliland', 'Puntland'],
         'Iraq': ['Iraq', 'Iraqi Kurdistan'],
         'Serbia': ['Serbia', 'Vojvodina'],
         'Papua New Guinea': ['Papua New Guinea', 'Bougainville'],
         'China': ['China', 'Paracel Is.'],
         # 2026-10-13: the two base areas are map units of their own. The island layer had given the entity KM65
         # and KM66, islets off the north coast of Cyprus; the bases are KM64 (Akrotiri, Dhekelia) and KM65 (Dhekelia
         # north of 35 degrees, Ayios Nikolaos)
         'UK Base Areas on Cyprus': ['Akrotiri Sovereign Base Area', 'Dhekelia Sovereign Base Area']}
# Land an entity owns that the steps below cannot find (2026-10-01) - the Franz Josef Land fault again: an
# island or point row is built from what lies near the cty.dat coordinate, so part of an entity that sits
# inside another map unit, or far from that coordinate, was "outside" it.  Added to the row, never instead
# of it: no square is taken away.
#   ADD_LAND    prefix -> (the map unit the land sits inside, a box holding it and nothing else of that unit)
#   ADD_POINTS  prefix -> further parts of the entity, (lon, lat): their square is added
ADD_LAND = {
    'VU7':  ('India', (71.0, 8.0, 74.3, 12.7)),          # Minicoy (MJ68), with the rest of Lakshadweep
    'JD/o': ('Japan', (140.5, 24.0, 143.0, 28.0)),       # the Volcano Islands, Iwo Jima (QL04 QL05)
    'H40':  ('Solomon Is.', (165.0, -13.0, 171.0, -9.0)),   # the whole province, Tikopia (RH47) with it
}
ADD_POINTS = {
    'KH5':  [(-160.02, -0.37)],      # Jarvis Island, about 1 000 km from Palmyra
    '3B6':  [(59.60, -16.50)],       # St. Brandon (Cargados Carajos), about 700 km from Agalega
    'E5/s': [(-159.78, -18.86), (-163.17, -18.05)],   # Aitutaki, Palmerston
    'CE0Y': [(-105.36, -26.47)],     # Sala y Gomez, part of the Easter Island entity
    'VP8/g': [(-42.03, -53.55)],     # Shag Rocks (GD86; Black Rock, GD96, is next to it), 250 km west
    # second round, the review of 2026-10-01 and the nearest-entity sweep of test/geodata_gap_check.py
    'FT/j': [(40.37, -22.36), (39.69, -21.48)],                  # Europa, Bassas da India
    '1S':   [(111.92, 8.64), (113.84, 7.37), (112.91, 7.85)],    # Spratly I., Layang-Layang, Amboyna Cay
    'ZD9':  [(-9.88, -40.32)],                                   # Gough Island, 412 km from Tristan
    'KH6':  [(-161.92, 23.06), (-164.70, 23.58), (-166.28, 23.87), (-167.99, 25.00),   # the Northwestern
             (-170.60, 25.42), (-171.73, 25.77), (-173.96, 26.06), (-175.83, 27.83)],   # Hawaiian Islands *
    'E5/n': [(-163.11, -13.25)],                                 # Suwarrow
    'FO/a': [(-144.33, -27.60), (-143.53, -27.92), (-154.70, -21.80)],   # Rapa, Marotiri, Iles Maria
    'V6':   [(143.91, 7.37), (144.45, 7.25), (143.04, 6.68), (144.50, 8.60),   # Woleai Ifalik Eauripik Faraulep
             (154.28, 8.15)],                                    # Minto Reef
    'P2':   [(159.45, -4.57)],                                   # Nukumanu, Bougainville province
    # Matthew and Hunter Islands, claimed by both Vanuatu and France (New Caledonia): in BOTH rows - the table is
    # a plausibility check, and a station of either claimant there must not be marked (the operator, 2026-10-01)
    'YJ':   [(171.32, -22.35), (172.05, -22.40)],                # Matthew, Hunter
    'FK':   [(171.32, -22.35), (172.05, -22.40)],                # Matthew, Hunter
    'V7':   [(160.83, 9.82), (170.10, 12.24)],                   # Ujelang, Bikar
    'T32':  [(-150.22, -9.95)],                                  # Caroline Island
    'ZL9':  [(179.05, -47.75), (178.80, -49.68), (166.60, -48.02)],    # Bounty, Antipodes, Snares
    'S7':   [(51.12, -10.17), (51.03, -9.23)],                   # Farquhar, Providence and St. Pierre
}
# * Nihoa, Necker, French Frigate Shoals, Gardner Pinnacles, Maro Reef, Laysan, Lisianski, Pearl and Hermes -
#   the Census 'HI' shape that KH6 is cut from holds the main islands only; Midway and Kure are entities of
#   their own.  Palmerston (E5/s) is in the first round's table below.

# Antarctic stations, by the country that runs them (2026-10-13): (name, lon, lat, Wikidata item).  A station's square
# joins the row of its country - not as land of that DXCC entity, but because this table is a plausibility check, and
# a station there signs with its home call: the /Z, KC4/ or VP8/ of a visitor's call does not fit into an FT8 message
# that carries a square (LU1DMZ was heard so from Orcadas, GC79, and marked as outside Argentina).  A call cty.dat
# itself puts in Antarctica (KC4USV, RI1AN.., DP0GVN, VK0.., FT#Y, ZS7) never needed this.
# What is listed: research stations, their refuges and field camps that are in use or kept - open all year, in summer,
# or closed for the time being - each under the country that runs it, a joint one under each.  NOT covered: a visitor
# from a country that runs no station there, and a ship.
# Positions: Wikidata (CC0), the item beside each entry, as read on 2026-10-13.  Which stations there are and whose
# they are is each national programme's own published information; the choice made here was compared with Wikipedia's
# list of research stations in Antarctica and with the facilities list the national programmes keep together (COMNAP,
# version 3.5.0 of March 2024) - the choice only: no datum of either is in this table.
# The decoder's own table (lib/chkgrid.f90) accepts the same squares for the same countries, and the square across an
# edge a station stands within 3 km of (this table needs none of those: the program accepts a neighbouring square
# anyway); tools/chkgrid_audit.py bases asks it about every station here, and that it accepts no other.
ANTARCTIC_BASES = {
    'LU':  [('Belgrano II', -34.627, -77.874, 'Q2357907'), ('Brown', -62.870, -64.895, 'Q2484332'),
            ('Carlini', -58.667, -62.238, 'Q1710747'), ('Camara', -59.919, -62.595, 'Q3635543'),
            ('Decepcion', -60.683, -62.983, 'Q3635546'), ('Esperanza', -56.998, -63.397, 'Q1368174'),
            ('Marambio', -56.623, -64.242, 'Q141493'), ('Matienzo', -60.071, -64.976, 'Q1134373'),
            ('Melchior', -62.983, -64.333, 'Q1149597'), ('Orcadas', -44.742, -60.740, 'Q1440159'),
            ('Petrel', -56.231, -63.478, 'Q48594'), ('Primavera', -60.954, -64.156, 'Q1149608'),
            ('San Martin', -67.100, -68.133, 'Q923120')],   # Argentina
    'VK':  [('Casey', 110.527, -66.282, 'Q1047169'), ('Davis', 77.969, -68.576, 'Q1315213'),
            ('Mawson', 62.874, -67.603, 'Q1434388'), ('Edgeworth David', 100.600, -66.250, 'Q15218894'),
            ('Law', 76.379, -69.388, 'Q846325'), ('Dome C ice-core camp', 123.350, -75.100, 'Q3045552')],   # Australia
    'EU':  [('Vechernyaya', 46.153, -67.658, 'Q29838553')],   # Belarus
    'ON':  [('Princess Elisabeth', 23.347, -71.950, 'Q588342')],   # Belgium
    'PY':  [('Comandante Ferraz', -58.392, -62.085, 'Q683143'), ('Goeldi', -55.367, -61.222, 'Q3935486'),
            ('Cruls', -58.585, -62.143, 'Q3935688')],   # Brazil
    'LZ':  [('St. Kliment Ohridski', -60.365, -62.641, 'Q2071240')],   # Bulgaria
    'CE':  [('Arturo Prat', -59.664, -62.479, 'Q715071'), ('Frei', -58.963, -62.200, 'Q1928160'),
            ('Escudero', -58.975, -62.205, 'Q2714138'), ("O'Higgins", -57.900, -63.321, 'Q2318477'),
            ('Carvajal', -68.915, -67.761, 'Q1450354'), ('Gonzalez Videla', -62.857, -64.824, 'Q1537450'),
            ('Yelcho', -63.584, -64.876, 'Q9080188'), ('Guillermo Mann', -60.771, -62.470, 'Q14769589'),
            ('Union Glacier', -82.907, -79.769, 'Q18644137'), ('Risopatron', -59.701, -62.378, 'Q5721594'),
            ('Ripamonti', -58.935, -62.210, 'Q5721591')],   # Chile
    'BY':  [('Great Wall', -58.962, -62.218, 'Q1548160'), ('Zhongshan', 76.372, -69.373, 'Q197899'),
            ('Kunlun', 77.116, -80.417, 'Q3235369'), ('Taishan', 76.967, -73.850, 'Q15729517'),
            ('Qinling', 163.715, -74.934, 'Q60793962')],   # China
    'OK':  [('Mendel', -57.883, -63.801, 'Q743379'), ('Eco-Nelson', -58.983, -62.246, 'Q13382296')],   # the Czech Republic
    'HC':  [('Maldonado', -59.741, -62.449, 'Q2317027')],   # Ecuador
    'OH':  [('Aboa', -13.417, -73.050, 'Q2081935')],   # Finland
    'F':   [("Dumont d'Urville", 140.001, -66.663, 'Q403901'), ("Cap Prud'homme", 139.907, -66.688, 'Q29838548'),
            ('Concordia', 123.333, -75.100, 'Q403127'), ('D85', 134.146, -70.425, 'Q29838551')],   # France
    'DL':  [('Neumayer III', -8.280, -70.670, 'Q7933'), ('Kohnen', 0.066, -75.002, 'Q688142'),
            ('Gondwana', 164.221, -74.636, 'Q884888'), ("GARS O'Higgins", -57.901, -63.321, 'Q19134919'),
            ('Dallmann', -58.667, -62.238, 'Q225923')],   # Germany
    'VU':  [('Maitri', 11.731, -70.767, 'Q626852'), ('Bharati', 76.195, -69.407, 'Q627152')],   # India
    'I':   [('Mario Zucchelli', 164.113, -74.695, 'Q1677142'), ('Concordia', 123.333, -75.100, 'Q403127'),
            ('Browning Pass', 163.915, -74.623, 'Q22089892'), ('Talos Dome', 159.212, -72.835, 'Q29838560'),
            ("Cap Prud'homme", 139.907, -66.688, 'Q29838548'), ('Mid Point', 145.820, -75.542, 'Q19545426')],   # Italy
    'JA':  [('Syowa', 39.582, -69.004, 'Q1550784'), ('Asuka', 24.131, -71.525, 'Q2082272'),
            ('Dome Fuji', 39.699, -77.317, 'Q7474939'), ('Mizuho', 44.278, -70.699, 'Q2350884'),
            ('S17', 40.092, -69.028, 'Q29838557')],   # Japan
    'HL':  [('King Sejong', -58.787, -62.223, 'Q246364'), ('Jang Bogo', 164.200, -74.617, 'Q245875')],   # Korea
    'PA':  [('Dirck Gerritsz', -68.128, -67.571, 'Q105605495')],   # the Netherlands
    'ZL':  [('Scott Base', 166.750, -77.850, 'Q1136714')],   # New Zealand
    'LA':  [('Troll', 2.535, -72.011, 'Q1082371'), ('Tor', 5.160, -71.890, 'Q763215'),
            ('Troll Airfield', 2.467, -71.957, 'Q3355017')],   # Norway
    'OA':  [('Machu Picchu', -58.471, -62.092, 'Q2886434')],   # Peru
    'SP':  [('Arctowski', -58.473, -62.160, 'Q638234'), ('Dobrowolski', 100.750, -66.275, 'Q1965470')],   # Poland
    'YO':  [('Law-Racovita', 76.379, -69.388, 'Q846325')],   # Romania
    'UA':  [('Bellingshausen', -58.961, -62.198, 'Q815995'), ('Mirny', 93.015, -66.552, 'Q646492'),
            ('Vostok', 106.866, -78.464, 'Q211014'), ('Novolazarevskaya', 11.825, -70.776, 'Q1411423'),
            ('Progress', 76.387, -69.379, 'Q1469332'), ('Molodyozhnaya', 45.841, -67.666, 'Q1422189'),
            ('Leningradskaya', 159.389, -69.502, 'Q2374874'), ('Russkaya', -136.867, -74.767, 'Q2628033'),
            ('Soyuz', 68.795, -70.577, 'Q4430443'), ('Druzhnaya-4', 73.709, -69.748, 'Q3635547'),
            ('Oazis-2, beside Dobrowolski', 100.750, -66.275, 'Q1965470')],   # Russia
    'ZS':  [('SANAE IV', -2.829, -71.674, 'Q1334024')],   # South Africa
    'EA':  [('Juan Carlos I', -60.388, -62.663, 'Q403043'), ('Gabriel de Castilla', -60.676, -62.977, 'Q403020')],   # Spain
    'SM':  [('Wasa', -13.417, -73.050, 'Q2280257'), ('Svea', -11.217, -74.583, 'Q3445731')],   # Sweden
    'TA':  [('Horseshoe Island', -67.237, -67.830, 'Q118874897')],   # Turkey
    'UR':  [('Vernadsky', -64.257, -65.246, 'Q1469808')],   # Ukraine
    'G':   [('Rothera', -68.128, -67.567, 'Q1635867'), ('Halley', -25.467, -75.573, 'Q1571679'),
            ('Signy', -45.600, -60.717, 'Q1751827'), ('Fossil Bluff', -68.283, -71.333, 'Q1412986'),
            ('Sky Blu', -71.585, -74.856, 'Q626914'), ('Port Lockroy', -63.494, -64.825, 'Q2104764')],   # the United Kingdom
    'K':   [('McMurdo', 166.668, -77.848, 'Q498627'), ('Palmer', -64.053, -64.774, 'Q1333903'),
            ('Marble Point', 163.833, -77.433, 'Q2604787'), ('Siple Dome', -149.002, -81.654, 'Q120969566'),
            ('WAIS Divide', -112.072, -79.471, 'Q5477552'), ('Byrd', -119.533, -80.017, 'Q2987067'),
            ('South Pole', 139.273, -89.998, 'Q243307'), ('Copacabana (Lenie)', -58.446, -62.179, 'Q3432209'),
            ('Cape Shirreff', -60.771, -62.470, 'Q7498973'), ('Allan Hills camp', 159.700, -76.700, 'Q370398')],   # the United States
    'CX':  [('Artigas', -58.902, -62.185, 'Q809827'), ('Elichiribehety', -56.991, -63.402, 'Q58625')],   # Uruguay
}
ANTARCTIC_COUNTRIES = len(ANTARCTIC_BASES)
for _p, _b in ANTARCTIC_BASES.items():
    for _n, _lon, _lat, _q in _b:
        if not (_lat < -60.0 and -180.0 <= _lon <= 180.0 and _q[:1] == 'Q'):
            raise SystemExit('FAILED: ANTARCTIC_BASES %s %r: not a place south of 60 degrees south, or no item' % (_p, _n))
# the United Kingdom is eight entities to cty.dat (a VP8 call that is not on its list of Antarctic calls is the
# Falkland Islands' to it), Turkey two
for _p in ('GM', 'GW', 'GI', 'GD', 'GJ', 'GU', '*GM/s', 'VP8'): ANTARCTIC_BASES[_p] = ANTARCTIC_BASES['G']
ANTARCTIC_BASES['*TA1'] = ANTARCTIC_BASES['TA']
for _p, _b in ANTARCTIC_BASES.items():
    ADD_POINTS[_p] = ADD_POINTS.get(_p, []) + [(_lon, _lat) for _n, _lon, _lat, _q in _b]
# The South Pole station's callbook entry gives JA00 (longitude 0), and that is the square the decoder's table had
ADD_POINTS['K'] = ADD_POINTS.get('K', []) + [(1.0, -89.5)]

# gridname, norm and squares_of come from cty.py: test/geodata_gap_check.py uses the same three.

# Three splits the map units cannot give us, done from data already in the tree:
#   K / KL / KH6  - the US Census state boundaries (geodata/cb_2023_us_state_20m), exact.
#   UA / UA9      - Russia cut at 60 deg E.  The DXCC line follows the Urals and the Caucasus, so
#                   this is an APPROXIMATION, and squares near the cut belong to both halves here.
#   R1FJ          - Franz Josef Land: Russia north of 79 deg N between 30 and 70 deg E (2026-10-01).
#   VU4           - the Andaman and Nicobar Islands: India between 91.5 and 94.5 deg E, south of 14.5 deg N
#                   (2026-10-13).
US_SHP = os.path.join(DATA, 'cb_2023_us_state_20m')
def us_parts():
    if not os.path.exists(US_SHP + '.shp'):
        if not os.path.exists(US_SHP + '.zip'):
            raise SystemExit('FAILED: %s is missing - fetch\n  https://www2.census.gov/geo/tiger/'
                             'GENZ2023/shp/cb_2023_us_state_20m.zip\ninto %s  (K/KL/KH6 are cut '
                             'from it)' % (US_SHP + '.zip', DATA))
        import zipfile; zipfile.ZipFile(US_SHP + '.zip').extractall(DATA)
    rr = shapefile.Reader(US_SHP); i = [x[0] for x in rr.fields[1:]].index('STUSPS')
    by = {}
    for sr in rr.shapeRecords():
        g = shape(sr.shape.__geo_interface__)
        if not g.is_valid: g = g.buffer(0)
        by.setdefault(sr.record[i], []).append(g)
    by = {k: unary_union(v) for k, v in by.items()}
    lower48 = unary_union([g for k, g in by.items() if k not in ('AK', 'HI', 'PR')])
    return {'K': lower48, 'KL': by['AK'], 'KH6': by['HI']}

# Every input is checked HERE, before the first polygon is read: a missing 5 MB download used to be
# reported only after unpacking the Census states and 2 795 island polygons (review 2026-09-24).
CTY = cty_path(tree_only=True)          # never the running station's copy: a table must be
                                        # reproducible from the tree it ships in
for what, url in ((US_SHP + '.zip', 'https://www2.census.gov/geo/tiger/GENZ2023/shp/'
                                    'cb_2023_us_state_20m.zip   (K/KL/KH6 are cut from it)'),
                  (MINOR + '.zip', 'https://naciscdn.org/naturalearth/10m/physical/'
                                   'ne_10m_minor_islands.zip'),
                  (NE + '.zip', 'https://naciscdn.org/naturalearth/10m/cultural/'
                                'ne_10m_admin_0_map_units.zip')):
    if not os.path.exists(what) and not os.path.exists(what[:-4] + '.shp'):
        raise SystemExit('FAILED: %s is missing - fetch\n  %s\ninto %s' % (what, url, DATA))
if not os.path.exists(ADM1):
    raise SystemExit('FAILED: %s is missing - run extract_admin1_subset.py against a downloaded\n'
                     '  ne_10m_admin_1_states_provinces.  Without it seventeen entities silently\n'
                     '  shrink to a disk around their coordinate.' % ADM1)

ents, _, _ = load_cty(CTY)
prefixes = {}
for line in open(CTY, encoding='utf-8', errors='replace'):
    f = line.split(':')
    if len(f) > 8 and line[0] not in ' \t':
        prefixes[f[0].strip()] = f[7].strip()

def minor_islands():
    if not os.path.exists(MINOR + '.shp'):
        import zipfile
        if not os.path.exists(MINOR + '.zip'):
            raise SystemExit('FAILED: %s.zip is missing - fetch it from\n  https://naciscdn.org/'
                             'naturalearth/10m/physical/ne_10m_minor_islands.zip' % MINOR)
        zipfile.ZipFile(MINOR + '.zip').extractall(DATA)
    geoms = []
    for sh in shapefile.Reader(MINOR).shapes():
        g = shape(sh.__geo_interface__)
        geoms.append(g if g.is_valid else g.buffer(0))
    return geoms, STRtree(geoms)

SPECIAL = us_parts()
ISLANDS, ITREE = minor_islands()
ADMIN1 = {}
if os.path.exists(ADM1):
    for pfx, v in json.load(open(ADM1))['entities'].items():
        g = shape(v['geometry']); ADMIN1[pfx] = g if g.is_valid else g.buffer(0)
    print('admin-1 subset: %d entities' % len(ADMIN1))
else:
    raise SystemExit('FAILED: %s is missing - run tools/extract_admin1_subset.py against a downloaded\n'
                     '  ne_10m_admin_1_states_provinces.  Without it seventeen entities silently\n'
                     '  shrink to a disk around their coordinate.' % ADM1)
_ru = None
if not os.path.exists(NE + '.shp'):                    # kept zipped in the tree: 5 MB instead of 10
    import zipfile
    if not os.path.exists(NE + '.zip'):
        raise SystemExit('FAILED: %s.zip is missing - fetch it from\n  https://naciscdn.org/'
                         'naturalearth/10m/cultural/ne_10m_admin_0_map_units.zip' % NE)
    zipfile.ZipFile(NE + '.zip').extractall(DATA)
r = shapefile.Reader(NE); flds = [x[0] for x in r.fields[1:]]
# Key by the UNIT-level names only.  SOVEREIGNT groups every dependency under its sovereign state, so
# keying by it made 'france' the union of France, French Guiana, Reunion, New Caledonia, French
# Polynesia, Mayotte and Clipperton - 140 squares in four oceans - and 'denmark' include Greenland
# (review 2026-09-24).  The sovereign names are kept apart and used only if nothing else matches, and
# an entity that lands there is marked 'sovereign' in the table, because that row is the whole state.
units, sovereign = {}, {}
for sr in r.shapeRecords():
    g = shape(sr.shape.__geo_interface__)
    if not g.is_valid: g = g.buffer(0)
    for k in ('NAME', 'NAME_LONG', 'GEOUNIT', 'SUBUNIT'):
        units.setdefault(norm(sr.record[flds.index(k)]), []).append(g)
    sovereign.setdefault(norm(sr.record[flds.index('SOVEREIGNT')]), []).append(g)
units = {k: unary_union(v) for k, v in units.items()}
sovereign = {k: unary_union(v) for k, v in sovereign.items()}
_ru = units.get(norm('Russia'))
if _ru is None:          # loud: without it UA, UA9 and R1FJ would quietly fall back to something smaller
    raise SystemExit('FAILED: no Russia in %s - UA, UA9 and R1FJ are cut from it' % NE)
# the 60 deg E approximation, see the note above.  The cut is in the EASTERN hemisphere only.  Chukotka
# and Wrangel lie at -180..-169, so a plain "lon < 60" window put them in EUROPEAN Russia (review
# 2026-09-24): every Bering-Strait square was attributed to UA and UA9 had none.
SPECIAL['UA']  = _ru.intersection(box(19, -90, 60, 90))
SPECIAL['UA9'] = _ru.intersection(unary_union([box(60, -90, 180, 90), box(-180, -90, -160, 90)]))
# Franz Josef Land (2026-10-01).  It has no map unit of its own - its islands are part of Russia's
# polygon - so it fell to the minor-islands branch, which gave it its coordinate's own square (LR40) and
# one small island (LR71), and none of the large islands east of 56 deg E: RI1FJL's real LR90 was marked
# as a grid outside its country.  Russia north of 79 deg N between 30 and 70 deg E is the archipelago and
# nothing else - 36 islands from 46 to 65.5 deg E, and Victoria Island at 36.7 deg E, sometimes counted
# apart; kept, the permissive direction.  The minor-island polygons in the same box join it.
_fjl_box = box(30, 79, 70, 83)
SPECIAL['R1FJ'] = unary_union([_ru.intersection(_fjl_box)]
                              + [g for g in ISLANDS if g.intersects(_fjl_box)])
if SPECIAL['R1FJ'].is_empty or not (79.5 < SPECIAL['R1FJ'].bounds[1] and SPECIAL['R1FJ'].bounds[3] < 82.5):
    raise SystemExit('FAILED: the Franz Josef Land cut is empty or reaches outside the archipelago: %s'
                     % (SPECIAL['R1FJ'].bounds,))
# The Andaman and Nicobar Islands (2026-10-13).  They are part of India's polygon and have no map unit of their own,
# so the row came from the island layer - which took the Coco Islands of Myanmar with it (NK64, 60 km north of the
# last Indian island) - and the land itself was added by hand (ADD_LAND).  India inside this box is the two
# archipelagos and nothing else: Landfall Island ends at 13.7 deg N, Indira Point is 6.75 deg N.
_in = units.get(norm('India'))
if _in is None: raise SystemExit('FAILED: no India in %s - VU4 is cut from it' % NE)
SPECIAL['VU4'] = _in.intersection(box(91.5, 6.0, 94.5, 14.5))
if SPECIAL['VU4'].is_empty or not (6.5 < SPECIAL['VU4'].bounds[1] and SPECIAL['VU4'].bounds[3] < 14.0):
    raise SystemExit('FAILED: the Andaman and Nicobar cut is empty or reaches outside the islands: %s'
                     % (SPECIAL['VU4'].bounds,))
print('Natural Earth: %d name keys' % len(units))

rows, source, group = {}, {}, {}
GROUP_OF = {n: g for g, names in GROUPS.items() for n in names}
for name, cont, lat, lon in ents:
    pfx = prefixes.get(name)
    if pfx is None: raise SystemExit('FAILED: no primary prefix for %r' % name)
    key = norm(ALIAS.get(name, name))
    squares = set()
    geom = None
    if pfx in SPECIAL:
        geom = SPECIAL[pfx]
    elif pfx in ADMIN1:
        geom = ADMIN1[pfx]; source[pfx] = 'admin1'
    elif name in MULTI:
        lost = [u for u in MULTI[name] if norm(u) not in units]
        if lost:          # loud: falling back to one unit would quietly lose the regions MULTI exists for
            raise SystemExit('FAILED: %s needs the map units %s, not in %s' % (name, lost, NE))
        geom = unary_union([units[norm(u)] for u in MULTI[name]])
    elif key in units:
        geom = units[key]
    elif key in sovereign:
        geom = sovereign[key]; source[pfx] = 'sovereign'
    if geom is None:                               # try the minor-island polygons
        pt = Point(lon, lat)
        kmlon = max(10.0, 111.0 * math.cos(math.radians(lat)))     # a degree of longitude, in km
        near = [ISLANDS[int(i)] for i in ITREE.query(pt.buffer(ISLAND_KM / min(111.0, kmlon)))]
        near = [g for g in near
                if math.hypot((g.distance(pt) if g.distance(pt) == 0 else
                               abs(g.centroid.x - lon) * kmlon), abs(g.centroid.y - lat) * 111.0)
                <= ISLAND_KM or g.distance(pt) * 111.0 <= ISLAND_KM]
        if near:
            geom = unary_union(near); source[pfx] = 'islands'
            squares.add(gridname(lon, lat))      # the layer has no names: never lose our own square
    if geom is not None:
        squares |= squares_of(geom)
        p = geom.representative_point()          # never lose an entity smaller than a square
        squares.add(gridname(p.x, p.y))
        source.setdefault(pfx, 'split' if pfx in SPECIAL else 'poly')
    else:                                            # the cty.dat coordinate and its surroundings
        dlat = POINT_KM / 111.0
        dlon = POINT_KM / max(10.0, 111.0 * math.cos(math.radians(lat)))
        x = (int((lon - dlon + 180) // 2) * 2) - 180
        while x <= lon + dlon:
            y = int((lat - dlat) // 1)
            while y <= lat + dlat:
                cx, cy = max(x, min(lon, x + 2)), max(y, min(lat, y + 1))   # nearest point of the square
                dx = (cx - lon) * 111.0 * math.cos(math.radians(lat)); dy = (cy - lat) * 111.0
                if math.hypot(dx, dy) <= POINT_KM:
                    squares.add(gridname(x + 1, y + 0.5))
                y += 1
            x += 2
        squares.add(gridname(lon, lat))
        source[pfx] = 'point'
    if pfx in ADD_LAND:
        unit, bx = ADD_LAND[pfx]
        if norm(unit) not in units: raise SystemExit('FAILED: ADD_LAND %s: no map unit %r' % (pfx, unit))
        land = units[norm(unit)].intersection(box(*bx))
        if land.is_empty: raise SystemExit('FAILED: ADD_LAND %s: nothing of %s in %s' % (pfx, unit, bx))
        squares |= squares_of(land)
    for plon, plat in ADD_POINTS.get(pfx, []): squares.add(gridname(plon, plat))
    if not squares: raise SystemExit('FAILED: no grid square for %s (%s)' % (pfx, name))
    rows[pfx] = (name, sorted(squares))
    group[pfx] = GROUP_OF.get(name, '')
# a key that names nothing would add nothing, silently - and each table has its own key: MULTI and ALIAS the cty.dat
# entity NAME (read as `name in MULTI`, `ALIAS.get (name)`), ADD_LAND and ADD_POINTS the PREFIX (`pfx in ADD_LAND`), so
# a key from the wrong one is as dead as a typo (review 2026-10-01). And a name the split or admin1 branch builds is
# never looked up in MULTI or ALIAS at all; an ALIAS value no map unit has falls back to a 120 km disk (second review).
_names = {n for n, _ in rows.values()}
_built_first = {rows[p][0] for p, v in source.items() if v in ('split', 'admin1')}
for table, keys in (('MULTI', MULTI), ('ALIAS', ALIAS)):
    for k in keys:
        if k in _built_first:
            raise SystemExit('FAILED: %s key %r: that entity is built by the split or admin1 branch, the entry does '
                             'nothing' % (table, k))
for k, v in ALIAS.items():
    if norm(v) not in units and norm(v) not in sovereign:
        raise SystemExit('FAILED: ALIAS %r -> %r: no Natural Earth map unit is called that' % (k, v))
for table, keys, space, what in (('MULTI', MULTI, _names, 'an entity name'), ('ALIAS', ALIAS, _names, 'an entity name'),
                                 ('ADD_LAND', ADD_LAND, rows, 'a prefix'),
                                 ('ANTARCTIC_BASES', ANTARCTIC_BASES, rows, 'a prefix'),
                                 ('ADD_POINTS', ADD_POINTS, rows, 'a prefix')):
    for k in keys:
        if k not in space: raise SystemExit('FAILED: %s key %r is not %s' % (table, k, what))
import collections as _c
print('entities %d: %s' % (len(rows), dict(_c.Counter(source.values()))))
_sov = [p for p, v in source.items() if v == 'sovereign']
if _sov: print('  NOTE sovereign-wide rows (state + dependencies): %s' % ' '.join(sorted(_sov)))

BUILT   = date.today().isoformat()
VERSION = datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')   # the table's own version stamp
HEAD = ('# DXCC entity -> the 4-character Maidenhead grid squares it occupies.\n'
        '#   <primary prefix>  <source>  <group>  <squares>  <entity name>\n'
        '#   group: entities under one administration (US, RU, ES, ...) - DXCC counts them apart, but\n'
        '#   a station of one may transmit from another; "-" means the entity is in no group.\n'
        '#   AQ is Antarctica with the South Shetland and the South Orkney Islands, whose calls cty.dat tells\n'
        '#   apart by guess (Peter I Island, also south of 60 degrees, has calls of its own and is not in it).\n'
        '#   source: poly = a Natural Earth map unit; sovereign = only the sovereign name matched, so\n'
        '#   the row covers that state AND its dependencies; admin1 = a subdivision cut from the admin-1\n'
        '#   layer (dxcc_admin1_subset.json); islands = minor-island polygons within %d km of\n'
        '#   the cty.dat coordinate (that layer has no names, so the match is geographic); point = the\n'
        '#   coordinate itself and everything within %d km of it.\n'
        '#   split = cut from a finer source: K/KL/KH6 from the US Census state boundaries (exact - KH6 also carries\n'
        '#   the Northwestern Hawaiian Islands, added by hand),\n'
        '#   UA/UA9 by cutting Russia at 60 deg E (approximate - the DXCC line follows the Urals),\n'
        '#   R1FJ = Russia north of 79 deg N between 30 and 70 deg E (Franz Josef Land),\n'
        '#   VU4 = India between 91.5 and 94.5 deg E south of 14.5 deg N (the Andaman and Nicobar Islands).\n'
        '#   %s\n'
        '#   also carry land added by hand (ADD_LAND, ADD_POINTS in the builder) - parts of the entity far from\n'
        '#   its coordinate or inside another map unit.\n'
        '#   The rows of the %d countries that run stations in Antarctica also carry the squares of those stations\n'
        '#   (ANTARCTIC_BASES in the builder; positions from Wikidata, CC0): not land of the entity - a station there\n'
        '#   signs with its home call.\n'
        '#   A matched parent keeps its whole polygon, so Spain still lists the Canary squares of EA8:\n'
        '#   the table errs on the permissive side, which is the safe direction for a plausibility test.\n'
        '#\n'
        '# entities: cty.dat (MIT) - borders: Natural Earth 1:10m admin_0_map_units (public domain)\n'
        '#   https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_0_map_units.zip\n'
        '# a square is listed when the entity touches it at all - permissive by design\n'
        '#\n'
        '# version %s (built %s), %s.\n'
        '# This table is %s\n'
        '# See geodata/README.md.  A JSON twin of this table is dxcc_grids.json.\n'
        % (ISLAND_KM, POINT_KM, ' '.join(sorted((set(ADD_LAND) | set(ADD_POINTS)) - set(ANTARCTIC_BASES))),
           ANTARCTIC_COUNTRIES, VERSION, BUILT, BUILT_BY, CLAIM))
body = ''.join('%-8s %-9s %-3s %4d  %-28s %s\n'
               % (pfx, source[pfx], group[pfx] or '-', len(rows[pfx][1]), rows[pfx][0],
                  ' '.join(rows[pfx][1])) for pfx in sorted(rows))
CONTENT = hashlib.sha256(body.encode()).hexdigest()[:12]      # the data itself, header excluded:
with open(OUT, 'w') as fh:                                    # two builds of the same inputs match
    fh.write(HEAD + '# content %s - the sha256 of the rows below, so a rebuild can be compared\n' % CONTENT)
    fh.write(body)
with open(OUTJ, 'w') as fh:
    json.dump({'version': VERSION, 'content': CONTENT, 'built': BUILT,
               'tool': 'tools/make_dxcc_grids.py', 'project': 'JTDX_CONTEST',
               'author': 'Tihomir Sokcevic CE3TSK', 'rights': 'This table is ' + CLAIM,
               'tool_licence': 'GPL v3', 'point_km': POINT_KM, 'island_km': ISLAND_KM,
               'groups': {g: sorted(p for p in rows if group[p] == g) for g in sorted(GROUPS)},
               'entities': {p: {'name': rows[p][0], 'source': source[p], 'group': group[p],
                                'grids': rows[p][1]}
                            for p in sorted(rows)}}, fh, indent=1)
    fh.write('\n')
import glob                                          # leave only the zips behind: EVERY member of
for base in (NE, MINOR, US_SHP):                       # each one, not a hand-written list of
    if not os.path.exists(base + '.zip'): continue     # extensions (the Census zip also carries
    for p in glob.glob(base + '.*'):                   # two .xml files, which used to survive)
        if not p.endswith('.zip'): os.remove(p)
print('wrote %s and %s  (version %s, content %s)' % (OUT, os.path.basename(OUTJ), VERSION, CONTENT))
