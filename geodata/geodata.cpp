// See geodata.h.  Part of JTDX_CONTEST, Tihomir Sokcevic CE3TSK, GPL v3 or later.

#include "geodata.h"
#include "dxcc_grids_data.h"
#include "grid_states_data.h"

#include <algorithm>
#include <cstring>

namespace
{
  int const FIELDS = 18;                 // A..R

  bool isField (char c) { return c >= 'A' && c <= 'R'; }
  bool isDigit (char c) { return c >= '0' && c <= '9'; }

  int squareIndex (std::uint16_t code)   // the states table, or -1
  {
    std::uint16_t const * b = geodata::states::square;
    std::uint16_t const * e = b + geodata::states::squareCount;
    std::uint16_t const * p = std::lower_bound (b, e, code);
    return (p != e && *p == code) ? static_cast<int> (p - b) : -1;
  }
}

namespace geodata
{
  std::uint16_t packGrid (char const * grid)
  {
    if (!grid || !isField (grid[0]) || !isField (grid[1])
        || !isDigit (grid[2]) || !isDigit (grid[3])) return InvalidGrid;
    return static_cast<std::uint16_t> (
             (((grid[0] - 'A') * FIELDS + (grid[1] - 'A')) * 10 + (grid[2] - '0')) * 10
             + (grid[3] - '0'));
  }

  void unpackGrid (std::uint16_t code, char * out)
  {
    if (!out) return;
    if (code >= FIELDS * FIELDS * 100) { out[0] = '\0'; return; }
    out[3] = static_cast<char> ('0' + code % 10); code /= 10;
    out[2] = static_cast<char> ('0' + code % 10); code /= 10;
    out[1] = static_cast<char> ('A' + code % FIELDS);
    out[0] = static_cast<char> ('A' + code / FIELDS);
    out[4] = '\0';
  }

  std::size_t gridNeighbours (std::uint16_t code, std::uint16_t * out)
  {
    if (code >= FIELDS * FIELDS * 100 || !out) return 0;
    int const latSq = code % 10, lonSq = (code / 10) % 10;
    int const latF = (code / 100) % FIELDS, lonF = (code / 100) / FIELDS;
    int const lon = lonF * 10 + lonSq, lat = latF * 10 + latSq;   // 0..179, 0..179
    std::size_t n = 0;
    for (int dlon = -1; dlon <= 1; ++dlon)
      for (int dlat = -1; dlat <= 1; ++dlat)
        {
          int const la = lat + dlat;
          if (la < 0 || la >= FIELDS * 10) continue;              // no square past a pole
          int const lo = (lon + dlon + FIELDS * 10) % (FIELDS * 10);   // the antimeridian wraps
          out[n++] = static_cast<std::uint16_t> (((lo / 10) * FIELDS + la / 10) * 100
                                                 + (lo % 10) * 10 + la % 10);
        }
    return n;
  }

  namespace                                      // ASCII only: these are cty.dat prefixes
  {
    char lower (char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char> (c - 'A' + 'a') : c; }

    bool sameIgnoringCase (char const * a, char const * b)
    {
      while (*a && lower (*a) == lower (*b)) { ++a; ++b; }
      return lower (*a) == lower (*b);
    }
  }

  int entityIndex (char const * primaryPrefix)
  {
    if (!primaryPrefix) return -1;
    // prefix[] is emitted sorted (the generator refuses to write it otherwise), so this is a
    // binary search over 346 short strings rather than a scan.
    std::size_t lo = 0, hi = dxcc::entityCount;
    while (lo < hi)
      {
        std::size_t const mid = lo + (hi - lo) / 2;
        int const c = std::strcmp (dxcc::prefix[mid], primaryPrefix);
        if (c == 0) return static_cast<int> (mid);
        if (c < 0) lo = mid + 1; else hi = mid;
      }
    // Twenty-nine prefixes carry a lower-case suffix - 3Y/b (Bouvet), VP8/o (South Orkney),
    // SV/a (Mount Athos), JD/m, FT/x ... - and the program's own resolver upper-cases what it
    // returns (CountryDat::_extractMasterPrefix).  Matching only exactly would hand back -1 for
    // precisely the rare entities a doubtful decode most often claims, and "no answer" means "do
    // not judge", so the test would be silently off where it is wanted most (review 2026-09-24).
    // The exact search above is the fast path; this scan runs only when it failed.  No two
    // prefixes differ by case alone, so a case-insensitive hit is still unambiguous.
    for (std::size_t i = 0; i < dxcc::entityCount; ++i)
      if (sameIgnoringCase (dxcc::prefix[i], primaryPrefix)) return static_cast<int> (i);
    return -1;
  }

  char const * entityPrefix (int entity)
  {
    return (entity < 0 || static_cast<std::size_t> (entity) >= dxcc::entityCount)
             ? nullptr : dxcc::prefix[entity];
  }

  char const * entityName (int entity)
  {
    return (entity < 0 || static_cast<std::size_t> (entity) >= dxcc::entityCount)
             ? nullptr : dxcc::name[entity];
  }

  char const * entitySource (int entity)
  {
    return (entity < 0 || static_cast<std::size_t> (entity) >= dxcc::entityCount)
             ? nullptr : dxcc::sourceName[dxcc::source[entity]];
  }

  char const * entityGroup (int entity)
  {
    return (entity < 0 || static_cast<std::size_t> (entity) >= dxcc::entityCount)
             ? "" : dxcc::groupName[dxcc::group[entity]];
  }

  std::size_t entityGridCount (int entity)
  {
    if (entity < 0 || static_cast<std::size_t> (entity) >= dxcc::entityCount) return 0;
    return dxcc::offset[entity + 1] - dxcc::offset[entity];
  }

  bool entityHasGrid (int entity, std::uint16_t grid)
  {
    if (entity < 0 || static_cast<std::size_t> (entity) >= dxcc::entityCount
        || grid == InvalidGrid) return false;
    std::uint16_t const * b = dxcc::grid + dxcc::offset[entity];
    std::uint16_t const * e = dxcc::grid + dxcc::offset[entity + 1];
    return std::binary_search (b, e, grid);
  }

  bool gridFitsEntity (char const * primaryPrefix, char const * grid, bool tolerant, bool sameAdmin)
  {
    int const entity = entityIndex (primaryPrefix);
    std::uint16_t const code = packGrid (grid);
    // Never judge what we cannot judge: an entity we do not know, a grid we cannot parse, and an
    // entity we hold no squares for.  The last one cannot happen with today's tables - the builder
    // refuses to emit an empty entity - but a new or hand-edited table could, and it must not turn
    // into "this station is impossible" (review 2026-09-24).
    if (entity < 0 || code == InvalidGrid || entityGridCount (entity) == 0) return true;

    std::uint16_t around[9];     // not `near`: that is a macro in the Windows SDK (minwindef.h)
    std::size_t const n = tolerant ? gridNeighbours (code, around) : 0;

    std::uint8_t const grp = dxcc::group[entity];
    for (std::size_t e = 0; e < dxcc::entityCount; ++e)
      {
        // just this entity, unless it belongs to an administration group and we were asked to
        // accept the whole of it
        if (static_cast<int> (e) != entity
            && !(sameAdmin && grp != 0 && dxcc::group[e] == grp)) continue;
        if (entityHasGrid (static_cast<int> (e), code)) return true;
        for (std::size_t i = 0; i < n; ++i)
          if (entityHasGrid (static_cast<int> (e), around[i])) return true;
      }
    return false;
  }

  std::size_t statesOfGrid (char const * grid, StateShare * out, std::size_t max)
  {
    int const i = squareIndex (packGrid (grid));
    if (i < 0 || !out) return 0;
    std::size_t n = 0;
    for (std::uint32_t k = states::offset[i]; k < states::offset[i + 1] && n < max; ++k, ++n)
      {
        out[n].code = states::code[states::entryState[k]];
        out[n].population = states::entryPop[k] / 255.0f;
        out[n].area = states::entryArea[k] / 255.0f;
      }
    return n;
  }

  std::uint32_t gridPopulation (char const * grid)
  {
    int const i = squareIndex (packGrid (grid));
    return i < 0 ? 0 : states::population[i];
  }

  char const * dxccVersion () { return dxcc::version; }
  char const * dxccContent () { return dxcc::content; }
  char const * statesVersion () { return states::version; }
  char const * statesContent () { return states::content; }
}
