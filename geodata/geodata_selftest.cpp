// Exercises the geodata API: packing round-trips, the neighbour set at the corners and across the
// antimeridian, and a table of call-entity/grid pairs.  Prints one line per check; geodata_check.py
// compiles it and judges the output.  Not part of the program.
//   g++ -O1 -std=c++11 -o selftest geodata_selftest.cpp geodata.cpp dxcc_grids_data.cpp grid_states_data.cpp
//
// Part of JTDX_CONTEST, Tihomir Sokcevic CE3TSK, GPL v3 or later.
#include "geodata.h"
#include <cstdio>

int main ()
{
  char g[5], b[5];
  for (int c = 0; c < 18 * 18 * 100; ++c)          // every square must round-trip
    {
      geodata::unpackGrid (static_cast<std::uint16_t> (c), g);
      if (geodata::packGrid (g) != c) { std::printf ("ROUNDTRIP %d %s\n", c, g); return 1; }
    }
  std::printf ("roundtrip ok\n");

  std::printf ("bad %d %d %d %d\n",
               geodata::packGrid ("") == geodata::InvalidGrid,
               geodata::packGrid ("S000") == geodata::InvalidGrid,   // S is not a field
               geodata::packGrid ("FF4") == geodata::InvalidGrid,
               geodata::packGrid ("FF46mi") != geodata::InvalidGrid); // 6 characters: take the first 4

  std::uint16_t n[9];
  std::size_t k = geodata::gridNeighbours (geodata::packGrid ("AA00"), n);   // the south-west corner:
  // longitude wraps west into the R field, latitude stops at the pole, so six squares, not nine
  std::printf ("corner %zu", k);
  for (std::size_t i = 0; i < k; ++i) { geodata::unpackGrid (n[i], b); std::printf (" %s", b); }
  std::printf ("\n");

  k = geodata::gridNeighbours (geodata::packGrid ("RJ95"), n);               // at the antimeridian
  std::printf ("wrap %zu", k);
  for (std::size_t i = 0; i < k; ++i) { geodata::unpackGrid (n[i], b); std::printf (" %s", b); }
  std::printf ("\n");

  static char const * const pairs[][3] = {
    {"CE", "FF46", "1"}, {"DL", "JO62", "1"}, {"EA8", "IL18", "1"}, {"KL", "BP51", "1"},
    {"3D2", "RH91", "1"}, {"UA9", "AP16", "1"}, {"F", "JN18", "1"}, {"OZ", "JO65", "1"},
    {"OM", "JN88", "1"}, {"9M2", "OJ03", "1"}, {"PJ2", "FK52", "1"}, {"KP2", "FK77", "1"},
    {"DL", "FF46", "0"}, {"CE", "JO62", "0"}, {"F", "LG79", "0"},   // Reunion is its own call area: an F call there would re-sign
    {"3A", "KM72", "0"},
    {"ZZZ", "FF46", "1"},        // an entity we do not know: never judge what we cannot judge
    {"CE", "ZZ99", "1"}          // not a grid square: likewise
  };
  for (std::size_t i = 0; i < sizeof pairs / sizeof pairs[0]; ++i)
    std::printf ("fit %s %s %d want %s\n", pairs[i][0], pairs[i][1],
                 geodata::gridFitsEntity (pairs[i][0], pairs[i][1]) ? 1 : 0, pairs[i][2]);

  // cty.dat spells twenty-nine prefixes with a lower-case suffix while the program's own resolver
  // upper-cases what it returns, so the two spellings must give the same answer (review 2026-09-24)
  // K / KH6 / KL are here because one prefix is a strict prefix of the others, and *GM/s because
  // its first byte is punctuation: the two shapes a sloppy comparison gets wrong.
  static char const * const case_pairs[][2] = {
    {"VP8/o", "VP8/O"}, {"3Y/b", "3Y/B"}, {"SV/a", "SV/A"}, {"JD/m", "jd/M"}, {"CE", "ce"},
    {"K", "k"}, {"KH6", "kh6"}, {"KL", "kl"}, {"*GM/s", "*gm/S"}, {"HK0/a", "HK0/A"} };
  for (std::size_t i = 0; i < sizeof case_pairs / sizeof case_pairs[0]; ++i)
    {
      int const a = geodata::entityIndex (case_pairs[i][0]);
      int const b = geodata::entityIndex (case_pairs[i][1]);
      // the prefix found, so the checker can see WHICH entity was matched, not just that the two
      // spellings agree (a consistently wrong index would otherwise pass)
      std::printf ("case %s %d %d %s\n", case_pairs[i][0], a, b,
                   b < 0 ? "-" : geodata::entityPrefix (b));
    }
  std::printf ("case-unknown %d %d\n", geodata::entityIndex ("VP8/zz"),
               geodata::entityIndex ("KL7"));                     // a call area, not an entity

  // the administration groups: a KL7 holder in Ohio, an Asiatic-Russia call west of the Urals,
  // a Spanish operator on the Canaries - accepted with sameAdmin, rejected without it
  // Cases the GROUP decides - the entity's own polygon does not hold the square.  (Spain and Italy
  // would be useless here: their map units already contain the Canaries and Sicily, so those pass
  // either way - see "the permissive rule" in geodata/README.md.)
  static char const * const admin[][2] = {
    {"KL", "FN20"}, {"K", "BP51"}, {"KH6", "EM12"}, {"UA9", "LO74"}, {"CT", "IM12"}, {"OZ", "GP60"} };
  for (std::size_t i = 0; i < sizeof admin / sizeof admin[0]; ++i)
    std::printf ("admin %s %s %d %d\n", admin[i][0], admin[i][1],
                 geodata::gridFitsEntity (admin[i][0], admin[i][1], true, true) ? 1 : 0,
                 geodata::gridFitsEntity (admin[i][0], admin[i][1], true, false) ? 1 : 0);
  std::printf ("group [%s] [%s] [%s] [%s]\n",
               geodata::entityGroup (geodata::entityIndex ("KL")),
               geodata::entityGroup (geodata::entityIndex ("UA9")),
               geodata::entityGroup (geodata::entityIndex ("CE")),
               geodata::entityGroup (-1));
  // a different administration must still be rejected, group or no group
  std::printf ("cross %d %d\n", geodata::gridFitsEntity ("DL", "FF46", true, true) ? 1 : 0,
               geodata::gridFitsEntity ("CE", "JO62", true, true) ? 1 : 0);

  std::printf ("neighbour-tolerance %d %d\n",
               geodata::gridFitsEntity ("3A", "JN23", true),    // JN23 touches Monaco's JN33
               geodata::gridFitsEntity ("3A", "JN23", false));
  return 0;
}
