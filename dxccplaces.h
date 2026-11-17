#ifndef DXCCPLACES_H
#define DXCCPLACES_H

// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md stage 4, decision 7): the DXCC entities on the Reception
// map - where each one's prefix is written, and which entities are under the mouse. Qt-free.
//
//   - entities ()   cty.dat's entities with their positions: a record is "Name: CQ: ITU: Continent: Lat: Lon: UTC:
//                   Prefix:" and then its prefixes up to ';'. The longitude there is WEST positive; here it is east
//                   positive. An entry whose prefix begins with '*' is none of DXCC's (the six of the WAE and CQ
//                   lists - Sicily, the Shetlands ...) and is left out. The numbers are read by this file's own
//                   rule, not the C library's: the program runs in the operator's locale, where a decimal point may
//                   be a comma. The larger entity first - by the squares it occupies in the program's own table
//                   (geodata.h) - so that, where two labels would meet, the one of the larger is written;
//   - at ()         the entities whose territory has the 4-character square of a place, as that table has them - the
//                   same table that judges a decode's grid against its call - the nearest by its cty.dat position
//                   first. A square is 2 by 1 degrees: on a border it has more than one entity, and all are named;
//                   a parent keeps its child's squares there (Spain the Canaries'), so the child, nearer, comes first;
//   - firstCome ()  which labels are written when each is written unless it meets one written before it.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "worldmap.h"
#include "geodata/geodata.h"

namespace dxcc_places
{
  struct Entity
  {
    std::string prefix;   // cty.dat's primary prefix: "CE", "VP8/o"
    std::string name;     // as cty.dat spells it; the caller may put the program's own name for it
    double lat;
    double lon;           // east positive
    int table;            // its row in the program's table of squares, -1 when it has none
    int squares;          // how many squares it occupies there
  };

  namespace detail
  {
    inline std::string trimmed (std::string const& s)
    {
      std::size_t a {0}, b {s.size ()};
      while (a < b && (' ' == s[a] || '\t' == s[a] || '\r' == s[a] || '\n' == s[a])) ++a;
      while (b > a && (' ' == s[b - 1] || '\t' == s[b - 1] || '\r' == s[b - 1] || '\n' == s[b - 1])) --b;
      return s.substr (a, b - a);
    }

    // "-12.43", "9", "41.9": a sign, digits, a point and digits - whatever the locale
    inline bool number (std::string const& s, double& value)
    {
      std::size_t i {0};
      bool const minus {!s.empty () && '-' == s[0]};
      if (minus || (!s.empty () && '+' == s[0])) ++i;
      double v {0}, scale {1};
      bool digits {false}, point {false};
      for (; i < s.size (); ++i)
        {
          if ('.' == s[i] && !point) {point = true; continue;}
          if (s[i] < '0' || s[i] > '9') return false;
          digits = true;
          if (point) {scale /= 10; v += (s[i] - '0') * scale;}
          else v = v * 10 + (s[i] - '0');
        }
      if (!digits) return false;
      value = minus ? -v : v;
      return true;
    }
  }

  inline std::vector<Entity> entities (std::string const& cty)
  {
    std::vector<Entity> out;
    std::size_t at {0};
    while (at < cty.size ())
      {
        std::size_t const end {cty.find (';', at)};
        if (end == std::string::npos) break;
        std::string const record {cty.substr (at, end - at)};
        at = end + 1;
        std::vector<std::string> f;
        std::size_t a {0};
        for (int i = 0; i < 8; ++i)
          {
            std::size_t const colon {record.find (':', a)};
            if (colon == std::string::npos) break;
            f.push_back (detail::trimmed (record.substr (a, colon - a)));
            a = colon + 1;
          }
        if (f.size () != 8 || f[0].empty () || f[7].empty () || '*' == f[7][0]) continue;
        double lat, west;
        if (!detail::number (f[4], lat) || !detail::number (f[5], west) || lat < -90.0 || lat > 90.0 || west < -180.0 || west > 180.0) continue;
        Entity e;
        e.prefix = f[7];
        e.name = f[0];
        e.lat = lat;
        e.lon = -west;
        e.table = geodata::entityIndex (e.prefix.c_str ());
        e.squares = e.table >= 0 ? static_cast<int> (geodata::entityGridCount (e.table)) : 0;
        out.push_back (e);
      }
    std::stable_sort (out.begin (), out.end (), [] (Entity const& a, Entity const& b) {return a.squares != b.squares ? a.squares > b.squares : a.prefix < b.prefix;});
    return out;
  }

  // how far two places are apart, in degrees of the great circle
  inline double apart (double lat1, double lon1, double lat2, double lon2)
  {
    using world_map::radians;
    double const c {std::sin (radians (lat1)) * std::sin (radians (lat2)) + std::cos (radians (lat1)) * std::cos (radians (lat2)) * std::cos (radians (lon1 - lon2))};
    return world_map::degrees (std::acos (c > 1.0 ? 1.0 : c < -1.0 ? -1.0 : c));
  }

  // indices into `all`
  inline std::vector<std::size_t> at (std::vector<Entity> const& all, double lat, double lon)
  {
    std::vector<std::size_t> out;
    std::uint16_t const square {geodata::packGrid (world_map::locator (lat, lon, 4).c_str ())};
    if (geodata::InvalidGrid == square) return out;
    for (std::size_t i = 0; i < all.size (); ++i)
      if (all[i].table >= 0 && geodata::entityHasGrid (all[i].table, square)) out.push_back (i);
    std::stable_sort (out.begin (), out.end (), [&all, lat, lon] (std::size_t a, std::size_t b) {
        return apart (lat, lon, all[a].lat, all[a].lon) < apart (lat, lon, all[b].lat, all[b].lon);
      });
    return out;
  }

  typedef world_map::Box Box;   // (3.-261) the one box and the one placing rule of worldmap.h

  inline std::vector<bool> firstCome (std::vector<Box> const& boxes)
  {
    return world_map::placeBoxes (boxes, false, 0.0);   // from the first, touching allowed, the map's edge no bound
  }
}

#endif
