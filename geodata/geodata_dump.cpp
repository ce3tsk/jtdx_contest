// A standalone dump used by geodata_check.py:
// prints every entity and every square exactly as the compiled tables see them, so the output can
// be diffed against geodata/*.json.  Not part of the program.
//   g++ -O1 -std=c++11 -o geodata_dump geodata_dump.cpp geodata.cpp dxcc_grids_data.cpp grid_states_data.cpp
#include "geodata.h"
#include "dxcc_grids_data.h"
#include "grid_states_data.h"
#include <cstdio>

int main ()
{
  char g[5];
  std::printf ("dxcc %s %s\n", geodata::dxccVersion (), geodata::dxccContent ());
  std::printf ("states %s %s\n", geodata::statesVersion (), geodata::statesContent ());
  for (std::size_t i = 0; i < geodata::dxcc::entityCount; ++i)
    {
      std::printf ("E %s %s %zu", geodata::dxcc::prefix[i], geodata::entitySource (int (i)),
                   geodata::entityGridCount (int (i)));
      for (std::uint32_t k = geodata::dxcc::offset[i]; k < geodata::dxcc::offset[i + 1]; ++k)
        { geodata::unpackGrid (geodata::dxcc::grid[k], g); std::printf (" %s", g); }
      std::printf ("\n");
    }
  for (std::size_t i = 0; i < geodata::states::squareCount; ++i)
    {
      geodata::unpackGrid (geodata::states::square[i], g);
      std::printf ("S %s %u", g, geodata::gridPopulation (g));
      geodata::StateShare sh[8];
      std::size_t const n = geodata::statesOfGrid (g, sh, 8);
      for (std::size_t k = 0; k < n; ++k)
        std::printf (" %s:%.3f:%.3f", sh[k].code, sh[k].population, sh[k].area);
      std::printf ("\n");
    }
  return 0;
}
