#ifndef WORLDMAP_H
#define WORLDMAP_H

// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md stage 1): the world the Reception map window draws - its
// rules, free of Qt, so that they can be tested without a window:
//
//   - the projection: Web Mercator on the unit square, as pskreporter's map - x 0 at 180 W and 1 at 180 E, y 0 at
//     85.05 N and 1 at 85.05 S; the world repeats sideways, so any longitude can be the middle;
//   - a locator's place: the middle of its square, 4 to 10 characters, either case;
//   - the night: the subsolar point from the UTC clock (the Astronomical Almanac's low-precision formulae, good to
//     0.01 degree between 1950 and 2050), the sun's altitude at a place, and a shade that grows from the terminator
//     to full at -12 degrees - a soft edge, as twilight is;
//   - a great circle, for the line to a station (stage 3), and the copy of the world a point of it belongs to;
//   - the view: its middle, its zoom (1: the world's side is the window's longer side, so no edge of the world ever
//     shows; 32 at most), the place it opens at (our longitude in the middle, 78 N to 60 S in view - decision 4), a
//     zoom about the mouse, a drag, and the level of detail a zoom asks for;
//   - the map file (tools/make_world_map.py writes it and describes it; Natural Earth, public domain): its header, and
//     one level decoded and CHECKED - a file that is not what it says is refused, never drawn in part;
//   - the map's colours, one table with a light and a dark column (5.8): nothing of the map is coloured in the code.
//
// test_worldmap has the rules.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace world_map
{
  double const pi {3.14159265358979323846};
  double const maxLatitude {85.0511287798066};   // where Web Mercator's square ends

  inline double radians (double degrees) {return degrees * pi / 180.0;}
  inline double degrees (double radians) {return radians * 180.0 / pi;}

  // ------------------------------------------------------------------------------------------------ the projection
  inline double xOfLongitude (double lon) {return (lon + 180.0) / 360.0;}
  inline double longitudeOfX (double x) {return x * 360.0 - 180.0;}
  inline double yOfLatitude (double lat)
  {
    if (lat > maxLatitude) lat = maxLatitude;
    if (lat < -maxLatitude) lat = -maxLatitude;
    return 0.5 - std::log (std::tan (pi / 4 + radians (lat) / 2)) / (2 * pi);
  }
  inline double latitudeOfY (double y) {return degrees (2 * std::atan (std::exp ((0.5 - y) * 2 * pi)) - pi / 2);}
  inline double wrapX (double x) {return x - std::floor (x);}   // into [0, 1)

  // ---------------------------------------------------------------------------------------------- a locator's place
  struct Place
  {
    bool ok;
    double lat;
    double lon;
  };

  // the middle of the square a Maidenhead locator names: fields A-R, squares 0-9, subsquares A-X, then 0-9 and A-X
  // again; 4, 6, 8 or 10 characters, either case. Anything else is no place.
  inline Place place (std::string const& locator)
  {
    Place const none {false, 0.0, 0.0};
    std::size_t const n {locator.size ()};
    if (n < 4 || n > 10 || n % 2) return none;
    double lon {-180.0}, lat {-90.0}, dlon {20.0}, dlat {10.0};
    for (std::size_t pair = 0; 2 * pair < n; ++pair)
      {
        int const count {pair % 2 ? 10 : pair ? 24 : 18};
        int a {static_cast<unsigned char> (locator[2 * pair])}, b {static_cast<unsigned char> (locator[2 * pair + 1])};
        if (pair % 2)
          {
            a -= '0'; b -= '0';
          }
        else
          {
            if (a >= 'a' && a <= 'z') a -= 'a' - 'A';
            if (b >= 'a' && b <= 'z') b -= 'a' - 'A';
            a -= 'A'; b -= 'A';
          }
        if (a < 0 || a >= count || b < 0 || b >= count) return none;
        if (pair)
          {
            dlon /= count; dlat /= count;
          }
        lon += a * dlon; lat += b * dlat;
      }
    return {true, lat + dlat / 2, lon + dlon / 2};
  }

  // (the review of 2026-11-16, TODO.md 3.-261) A locator as the operator may have typed it - the Settings take 5, 7
  // and 9 characters as well, FF46q - is the locator without its odd last character: the part of it that names a
  // square. "" where that is no place either.
  inline std::string wholeLocator (std::string const& locator)
  {
    std::string l {locator.size () > 10 ? locator.substr (0, 10) : locator};
    if (l.size () % 2) l.erase (l.size () - 1);
    return place (l).ok ? l : std::string {};
  }

  // (stage 4) the other way: the locator of a place, 4 or 6 characters - the field and the square in capitals, the
  // subsquare small ("FF46qp"). Any longitude; a latitude past a pole is the pole's.
  inline std::string locator (double lat, double lon, int characters)
  {
    double x {lon + 180.0}, y {lat + 90.0};
    x -= 360.0 * std::floor (x / 360.0);
    if (!(y >= 0.0)) y = 0.0;
    if (y >= 180.0) y = 180.0 - 1e-9;
    std::string out;
    out += static_cast<char> ('A' + static_cast<int> (x / 20.0));
    out += static_cast<char> ('A' + static_cast<int> (y / 10.0));
    out += static_cast<char> ('0' + static_cast<int> (std::fmod (x, 20.0) / 2.0));
    out += static_cast<char> ('0' + static_cast<int> (std::fmod (y, 10.0)));
    if (characters >= 6)
      {
        out += static_cast<char> ('a' + static_cast<int> (std::fmod (x, 2.0) * 12.0));
        out += static_cast<char> ('a' + static_cast<int> (std::fmod (y, 1.0) * 24.0));
      }
    return out;
  }

  // -------------------------------------------------------------------------------------------------------- the night
  struct Sun
  {
    double lat;   // the place the sun stands over
    double lon;
  };

  inline Sun subsolar (double unixSeconds)
  {
    double const n {unixSeconds / 86400.0 + 2440587.5 - 2451545.0};   // days from J2000.0
    double const meanLongitude {280.460 + 0.9856474 * n};
    double const anomaly {radians (std::fmod (357.528 + 0.9856003 * n, 360.0))};
    double const lambda {radians (std::fmod (meanLongitude + 1.915 * std::sin (anomaly) + 0.020 * std::sin (2 * anomaly), 360.0))};
    double const obliquity {radians (23.439 - 0.0000004 * n)};
    double const declination {std::asin (std::sin (obliquity) * std::sin (lambda))};
    double const rightAscension {degrees (std::atan2 (std::cos (obliquity) * std::sin (lambda), std::cos (lambda)))};
    double const greenwichSiderealTime {std::fmod (280.46061837 + 360.98564736629 * n, 360.0)};
    double lon {std::fmod (rightAscension - greenwichSiderealTime, 360.0)};
    if (lon < -180.0) lon += 360.0;
    if (lon >= 180.0) lon -= 360.0;
    return {degrees (declination), lon};
  }

  // the sun's altitude at a place, degrees: 90 under it, 0 on the terminator, negative in the night
  inline double sunAltitude (double lat, double lon, Sun const& sun)
  {
    double const s {std::sin (radians (lat)) * std::sin (radians (sun.lat))
                    + std::cos (radians (lat)) * std::cos (radians (sun.lat)) * std::cos (radians (lon - sun.lon))};
    return degrees (std::asin (s > 1.0 ? 1.0 : s < -1.0 ? -1.0 : s));
  }

  double const fullNightAltitude {-12.0};   // the end of nautical twilight

  // how much of the night's shade a place gets: 0 in daylight, 1 from -12 degrees down, growing evenly between
  inline double nightShade (double altitude)
  {
    return altitude >= 0.0 ? 0.0 : altitude <= fullNightAltitude ? 1.0 : altitude / fullNightAltitude;
  }

  // (the review of 2026-11-16, TODO.md 3.-261: the night was worked out place by place - six sines, cosines and arc
  // sines for every pixel of it, at every step of a drag) The night's shade over a grid of places. The sine of the
  // sun's altitude is  sin lat sin sunLat + cos lat cos sunLat cos (lon - sunLon): the last factor is a column's, the
  // others a row's - each worked out once; and the arc sine is asked only in the twilight, where the shade grows.
  //   Shade shade {sun, the columns' longitudes};  shade.row (lat);  shade.at (column)  - nightShade (sunAltitude ()) of it
  class Shade
  {
  public:
    Shade (Sun const& sun, std::vector<double> const& lons)
      : m_sinSun {std::sin (radians (sun.lat))}, m_cosSun {std::cos (radians (sun.lat))}, m_sinFull {std::sin (radians (fullNightAltitude))}
    {
      m_cosLon.reserve (lons.size ());
      for (double lon : lons) m_cosLon.push_back (std::cos (radians (lon - sun.lon)));
    }
    void row (double lat)
    {
      m_a = std::sin (radians (lat)) * m_sinSun;
      m_b = std::cos (radians (lat)) * m_cosSun;
    }
    double at (std::size_t column) const
    {
      double const s {m_a + m_b * m_cosLon[column]};
      if (s >= 0.0) return 0.0;            // daylight
      if (s <= m_sinFull) return 1.0;      // the full night
      return nightShade (degrees (std::asin (s)));
    }
  private:
    double m_sinSun, m_cosSun, m_sinFull;
    double m_a {0.0}, m_b {0.0};
    std::vector<double> m_cosLon;
  };

  // -------------------------------------------------------------------------------------------------- a great circle
  struct LatLon
  {
    double lat;
    double lon;
  };

  // the short way from one place to another in `steps` steps (steps + 1 points, the two places first and last). The
  // same place twice is that place; for two places exactly opposite, where every great circle is as short, the way
  // over the north.
  inline std::vector<LatLon> greatCircle (double lat1, double lon1, double lat2, double lon2, int steps)
  {
    if (steps < 1) steps = 1;
    double const p1 {radians (lat1)}, l1 {radians (lon1)}, p2 {radians (lat2)}, l2 {radians (lon2)};
    double const a[3] {std::cos (p1) * std::cos (l1), std::cos (p1) * std::sin (l1), std::sin (p1)};
    double const b[3] {std::cos (p2) * std::cos (l2), std::cos (p2) * std::sin (l2), std::sin (p2)};
    double dot {a[0] * b[0] + a[1] * b[1] + a[2] * b[2]};
    if (dot > 1.0) dot = 1.0;
    if (dot < -1.0) dot = -1.0;
    double const omega {std::acos (dot)};
    std::vector<LatLon> path;
    path.reserve (steps + 1);
    for (int i = 0; i <= steps; ++i)
      {
        if (0 == i) {path.push_back ({lat1, lon1}); continue;}
        if (steps == i) {path.push_back ({lat2, lon2}); continue;}
        double const u {static_cast<double> (i) / steps};
        double x, y, z;
        if (omega < 1e-9)   // the same place
          {
            x = a[0]; y = a[1]; z = a[2];
          }
        else if (pi - omega < 1e-9)   // opposite places: from the first up to the pole and down to the second
          {
            double const lat {u < 0.5 ? lat1 + (90.0 - lat1) * 2 * u : 90.0 - (90.0 - lat2) * (2 * u - 1)};
            path.push_back ({lat, u < 0.5 ? lon1 : lon2});
            continue;
          }
        else
          {
            double const s1 {std::sin ((1 - u) * omega) / std::sin (omega)}, s2 {std::sin (u * omega) / std::sin (omega)};
            x = s1 * a[0] + s2 * b[0]; y = s1 * a[1] + s2 * b[1]; z = s1 * a[2] + s2 * b[2];
          }
        path.push_back ({degrees (std::atan2 (z, std::sqrt (x * x + y * y))), degrees (std::atan2 (y, x))});
      }
    return path;
  }

  // a point's x beside the point before it: the copy of the world nearest to that one, so that a line crossing
  // 180 degrees goes on across the edge instead of running back over the whole map
  inline double unwrappedX (double x, double previous) {return x + std::round (previous - x);}

  // --------------------------------------------------------------------------------------------------------- the view
  double const maxZoom {32.0};
  double const defaultNorth {78.0};    // the band the map opens with: the inhabited world
  double const defaultSouth {-60.0};
  double const detailPixelsPerDegree {12.0};   // from here on the 1:10m level is drawn

  struct View
  {
    double cx {0.5};    // the world's point in the middle of the window, on the unit square
    double cy {0.5};
    double zoom {1.0};  // 1: the world's side is the window's longer side
    int w {1};          // the window, in pixels
    int h {1};

    double side () const {return zoom * (w > h ? w : h);}   // the world square's side, pixels
    double screenX (double x) const {return (x - cx) * side () + w / 2.0;}
    double screenY (double y) const {return (y - cy) * side () + h / 2.0;}
    double worldX (double sx) const {return (sx - w / 2.0) / side () + cx;}
    double worldY (double sy) const {return (sy - h / 2.0) / side () + cy;}
  };

  // a view made sound: the zoom within 1 and 32, the middle wrapped sideways, and held so that nothing beyond the
  // world's top or bottom is in the window. A value that is no number goes back to its start.
  inline void settle (View& v)
  {
    if (v.w < 1) v.w = 1;
    if (v.h < 1) v.h = 1;
    if (!(v.zoom >= 1.0)) v.zoom = 1.0;
    if (v.zoom > maxZoom) v.zoom = maxZoom;
    if (!std::isfinite (v.cx)) v.cx = 0.5;
    if (!std::isfinite (v.cy)) v.cy = 0.5;
    v.cx = wrapX (v.cx);
    double const half {v.h / 2.0 / v.side ()};   // half the window's height on the unit square; never over 0.5
    if (v.cy < half) v.cy = half;
    if (v.cy > 1.0 - half) v.cy = 1.0 - half;
  }

  // where the map opens, and where Home returns: our longitude in the middle (Greenwich's with no locator of ours),
  // the band 78 N to 60 S in view - not our latitude: from FF46 that would cut the picture at 58 N
  inline View defaultView (int w, int h, bool haveOwn, double ownLon)
  {
    View v;
    v.w = w; v.h = h; v.zoom = 1.0;
    v.cx = xOfLongitude (haveOwn ? ownLon : 0.0);
    v.cy = (yOfLatitude (defaultNorth) + yOfLatitude (defaultSouth)) / 2;
    settle (v);
    return v;
  }

  // zoom by a factor, the world's point under (sx, sy) staying under it - as far as the world's top and bottom allow
  inline void zoomAbout (View& v, double factor, double sx, double sy)
  {
    double const x {v.worldX (sx)}, y {v.worldY (sy)};
    v.zoom *= factor;
    if (!(v.zoom >= 1.0)) v.zoom = 1.0;
    if (v.zoom > maxZoom) v.zoom = maxZoom;
    v.cx = x - (sx - v.w / 2.0) / v.side ();
    v.cy = y - (sy - v.h / 2.0) / v.side ();
    settle (v);
  }

  // the map dragged by so many pixels
  inline void drag (View& v, double dx, double dy)
  {
    v.cx -= dx / v.side ();
    v.cy -= dy / v.side ();
    settle (v);
  }

  // (the review of 2026-11-16, TODO.md 3.-261: the whole map was drawn again at every step of a drag) Is the view
  // `to` the view `from` moved by whole pixels - the same window, the same zoom - and by how many: what was drawn at
  // (x, y) is then at (x + dx, y + dy), and only what came into the window is to be drawn. Round the world the
  // shorter way. Not when nothing of the old drawing would be left.
  inline bool pan (View const& from, View const& to, int& dx, int& dy)
  {
    if (from.zoom != to.zoom || from.w != to.w || from.h != to.h) return false;
    double const side {to.side ()};
    double x {(from.cx - to.cx) * side}, y {(from.cy - to.cy) * side};
    x -= side * std::floor (x / side + 0.5);
    double const wholeX {std::floor (x + 0.5)}, wholeY {std::floor (y + 0.5)};
    if (std::fabs (x - wholeX) > 1e-6 || std::fabs (y - wholeY) > 1e-6) return false;
    if (std::fabs (wholeX) >= to.w || std::fabs (wholeY) >= to.h) return false;
    dx = static_cast<int> (wholeX); dy = static_cast<int> (wholeY);
    return true;
  }

  // (stage 4) The view that shows a set of places as large as the window allows - "Zoom to the stations that hear
  // me": each place in the copy of the world nearest `refLon` (our own longitude), so many pixels left free at the
  // top, the right, the foot and the left (a mark's label stands above it, the status line lies over the foot). One
  // place: the zoom's limit, about it. None: the view as it is.
  struct Margins
  {
    int top;
    int right;
    int bottom;
    int left;
  };

  inline View fitTo (View const& now, std::vector<Place> const& places, double refLon, Margins const& m)
  {
    View v {now};
    double const x0 {xOfLongitude (refLon)};
    bool any {false};
    double minX {0}, maxX {0}, minY {0}, maxY {0};
    for (auto const& p : places)
      {
        if (!p.ok) continue;
        double const x {unwrappedX (xOfLongitude (p.lon), x0)}, y {yOfLatitude (p.lat)};
        if (!any)
          {
            minX = maxX = x; minY = maxY = y; any = true;
            continue;
          }
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
      }
    if (!any) return v;
    double const freeW {v.w - m.left - m.right > 1 ? double (v.w - m.left - m.right) : 1.0};
    double const freeH {v.h - m.top - m.bottom > 1 ? double (v.h - m.top - m.bottom) : 1.0};
    double const longer {double (v.w > v.h ? v.w : v.h)};
    double zoom {maxZoom};
    if (maxX > minX && freeW / ((maxX - minX) * longer) < zoom) zoom = freeW / ((maxX - minX) * longer);
    if (maxY > minY && freeH / ((maxY - minY) * longer) < zoom) zoom = freeH / ((maxY - minY) * longer);
    if (!(zoom >= 1.0)) zoom = 1.0;
    v.zoom = zoom;
    // the places' middle in the middle of what the margins leave free
    v.cx = (minX + maxX) / 2 - (m.left - m.right) / 2.0 / v.side ();
    v.cy = (minY + maxY) / 2 - (m.top - m.bottom) / 2.0 / v.side ();
    settle (v);
    return v;
  }

  // ------------------------------------------------------------------------------------------------------ labels
  // (the review of 2026-11-16, TODO.md 3.-261: the ages of the stations and the entities' prefixes each had a box
  // and a placing rule of their own) A label's box, and which of a list of them are written: they are given their
  // place one after the other - from the last to the first, or from the first - and one that would meet a label
  // already given is left out; `gap` pixels must stay free round it. With a map's width and height, a box of no size
  // or one that reaches out of the map is left out as well.
  struct Box
  {
    double x, y, w, h;
    bool meets (Box const& o) const {return x < o.x + o.w && o.x < x + w && y < o.y + o.h && o.y < y + h;}
  };

  inline std::vector<bool> placeBoxes (std::vector<Box> const& boxes, bool lastFirst, double gap, bool bounded = false, double mapWidth = 0, double mapHeight = 0)
  {
    std::vector<bool> written (boxes.size (), false);
    std::vector<Box> given;
    for (std::size_t k = 0; k < boxes.size (); ++k)
      {
        std::size_t const i {lastFirst ? boxes.size () - 1 - k : k};
        Box const& b = boxes[i];
        if (bounded && (b.w <= 0 || b.h <= 0 || b.x < 0 || b.y < 0 || b.x + b.w > mapWidth || b.y + b.h > mapHeight)) continue;
        bool free {true};
        for (auto const& g : given) if (g.meets (Box {b.x - gap, b.y - gap, b.w + 2 * gap, b.h + 2 * gap})) {free = false; break;}
        if (!free) continue;
        written[i] = true;
        given.push_back (b);
      }
    return written;
  }

  // the level of detail a view asks for: 0 the 1:50m level, 1 the 1:10m level
  inline int level (View const& v) {return v.side () / 360.0 >= detailPixelsPerDegree ? 1 : 0;}

  // ----------------------------------------------------------------------------------------------------- the map file
  int const fileVersion {1};
  std::uint32_t const noName {0xFFFFFFFFu};
  double const unitsPerDegree {10000.0};
  enum Kind {Land50 = 0, Land10 = 1, Lakes = 2};

  struct Ring
  {
    std::uint32_t first;   // into Layer::lon and Layer::lat
    std::uint32_t count;
  };

  struct Polygon
  {
    std::int32_t minLon, minLat, maxLon, maxLat;   // 1e-4 degree
    std::uint32_t name;                            // into File::names, or noName
    std::uint32_t firstRing;                       // into Layer::rings: the outline, then its holes
    std::uint32_t rings;
  };

  struct Layer
  {
    int kind {-1};
    std::vector<Polygon> polygons;
    std::vector<Ring> rings;
    std::vector<std::int32_t> lon, lat;            // 1e-4 degree
  };

  struct File
  {
    struct Entry
    {
      int kind;
      std::uint32_t polygons, rings, points;
      std::size_t offset, bytes;                   // where the layer's polygons stand in the file
    };
    std::string note;                              // the source and the rights
    std::vector<std::string> names;                // Natural Earth's GEOUNIT of each map unit
    std::vector<Entry> layers;
  };

  namespace detail
  {
    struct Cursor
    {
      unsigned char const * p;
      unsigned char const * end;
      bool ok {true};

      Cursor (unsigned char const * from, unsigned char const * to) : p {from}, end {to} {}

      std::size_t left () const {return static_cast<std::size_t> (end - p);}
      bool take (std::size_t n) {if (!ok || left () < n) {ok = false; return false;} return true;}
      std::uint32_t u8 () {if (!take (1)) return 0; return *p++;}
      std::uint32_t u16 () {if (!take (2)) return 0; std::uint32_t const v {static_cast<std::uint32_t> (p[0]) | static_cast<std::uint32_t> (p[1]) << 8}; p += 2; return v;}
      std::uint32_t u32 ()
      {
        if (!take (4)) return 0;
        std::uint32_t const v {static_cast<std::uint32_t> (p[0]) | static_cast<std::uint32_t> (p[1]) << 8
                               | static_cast<std::uint32_t> (p[2]) << 16 | static_cast<std::uint32_t> (p[3]) << 24};
        p += 4;
        return v;
      }
      std::int32_t i32 () {return static_cast<std::int32_t> (u32 ());}
      std::uint32_t varint ()   // 32 bits at most, so five bytes at most, the fifth with no more than four bits
      {
        std::uint32_t v {0};
        for (int shift = 0; shift <= 28; shift += 7)
          {
            if (!take (1)) return 0;
            std::uint32_t const b {*p++};
            if (28 == shift && (b & 0xF0u)) {ok = false; return 0;}
            v |= (b & 0x7Fu) << shift;
            if (!(b & 0x80u)) return v;
          }
        ok = false;
        return 0;
      }
      std::int32_t step () {std::uint32_t const z {varint ()}; return static_cast<std::int32_t> (z >> 1) ^ -static_cast<std::int32_t> (z & 1u);}
    };
  }

  // what a file holds. False, with the reason, when it is not a world map of this version or its parts do not add up
  // to its size - nothing of `file` is to be used then.
  inline bool readHeader (unsigned char const * data, std::size_t size, File& file, std::string& error)
  {
    file = File {};
    detail::Cursor c {data, data + size};
    static char const magic[] {"JTDXWMAP"};
    if (!c.take (8) || std::string (reinterpret_cast<char const *> (c.p), 8) != magic) {error = "not a world map file"; return false;}
    c.p += 8;
    if (c.u32 () != static_cast<std::uint32_t> (fileVersion) || !c.ok) {error = "a world map file of another version"; return false;}
    std::uint32_t const noteBytes {c.u16 ()};
    if (!c.take (noteBytes)) {error = "the file ends in its note"; return false;}
    file.note.assign (reinterpret_cast<char const *> (c.p), noteBytes);
    c.p += noteBytes;
    std::uint32_t const names {c.u32 ()};
    if (!c.ok || names > c.left () / 2) {error = "the file ends in its names"; return false;}
    file.names.reserve (names);
    for (std::uint32_t i = 0; i < names; ++i)
      {
        std::uint32_t const n {c.u16 ()};
        if (!c.take (n)) {error = "the file ends in its names"; return false;}
        file.names.emplace_back (reinterpret_cast<char const *> (c.p), n);
        c.p += n;
      }
    std::uint32_t const layers {c.u32 ()};
    if (!c.ok || layers < 1 || layers > 16) {error = "the file names no layers, or too many"; return false;}
    for (std::uint32_t i = 0; i < layers; ++i)
      {
        File::Entry e;
        e.kind = static_cast<int> (c.u8 ());
        e.polygons = c.u32 (); e.rings = c.u32 (); e.points = c.u32 ();
        e.bytes = c.u32 ();
        if (!c.take (e.bytes)) {error = "the file ends in a layer"; return false;}
        e.offset = static_cast<std::size_t> (c.p - data);
        c.p += e.bytes;
        // the least a polygon, a ring and a point take: no count may promise more than the layer's bytes can hold
        if (e.polygons > e.bytes / 22 || e.rings > e.bytes / 3 || e.points > e.bytes / 2 || e.rings < e.polygons || e.points < 3 * static_cast<std::uint64_t> (e.rings))
          {error = "a layer's counts do not fit its size"; return false;}
        file.layers.push_back (e);
      }
    if (c.left ()) {error = "the file goes on after its last layer"; return false;}
    return true;
  }

  // one layer of a file whose header was read, decoded and checked: as many polygons, rings and points as the header
  // says, in exactly its bytes; every ring of three points or more; every point in the world and inside its polygon's
  // box; every name one of the file's. False, with the reason, otherwise - and `layer` is then empty.
  inline bool readLayer (unsigned char const * data, std::size_t size, File const& file, std::size_t index, Layer& layer, std::string& error)
  {
    layer = Layer {};
    if (index >= file.layers.size ()) {error = "no such layer"; return false;}
    File::Entry const& e {file.layers[index]};
    if (e.offset > size || e.bytes > size - e.offset) {error = "the layer is not in the file"; return false;}
    detail::Cursor c {data + e.offset, data + e.offset + e.bytes};
    Layer l;
    l.kind = e.kind;
    l.polygons.reserve (e.polygons); l.rings.reserve (e.rings); l.lon.reserve (e.points); l.lat.reserve (e.points);
    std::int32_t const lonEdge {static_cast<std::int32_t> (180 * unitsPerDegree)}, latEdge {static_cast<std::int32_t> (90 * unitsPerDegree)};
    for (std::uint32_t i = 0; i < e.polygons; ++i)
      {
        Polygon p;
        p.minLon = c.i32 (); p.minLat = c.i32 (); p.maxLon = c.i32 (); p.maxLat = c.i32 ();
        p.name = c.u32 ();
        p.rings = c.varint ();
        p.firstRing = static_cast<std::uint32_t> (l.rings.size ());
        if (!c.ok) {error = "a layer ends in a polygon"; return false;}
        if (p.minLon < -lonEdge || p.maxLon > lonEdge || p.minLat < -latEdge || p.maxLat > latEdge || p.minLon > p.maxLon || p.minLat > p.maxLat)
          {error = "a polygon's box is not in the world"; return false;}
        if (p.name != noName && p.name >= file.names.size ()) {error = "a polygon names no map unit of the file"; return false;}
        if (p.rings < 1 || p.rings > e.rings - l.rings.size ()) {error = "a polygon has more rings than the layer"; return false;}
        for (std::uint32_t r = 0; r < p.rings; ++r)
          {
            std::uint32_t const n {c.varint ()};
            if (!c.ok || n < 3 || n > e.points - l.lon.size ()) {error = "a ring has fewer than three points, or more than the layer"; return false;}
            l.rings.push_back ({static_cast<std::uint32_t> (l.lon.size ()), n});
            std::int64_t x {p.minLon}, y {p.minLat};
            for (std::uint32_t k = 0; k < n; ++k)
              {
                x += c.step (); y += c.step ();
                if (!c.ok) {error = "a layer ends in a ring"; return false;}
                if (x < p.minLon || x > p.maxLon || y < p.minLat || y > p.maxLat) {error = "a point outside its polygon's box"; return false;}
                l.lon.push_back (static_cast<std::int32_t> (x)); l.lat.push_back (static_cast<std::int32_t> (y));
              }
          }
        l.polygons.push_back (p);
      }
    if (c.left ()) {error = "a layer is longer than its polygons"; return false;}
    if (l.rings.size () != e.rings || l.lon.size () != e.points) {error = "a layer holds other counts than its header says"; return false;}
    layer = std::move (l);
    return true;
  }

  // ------------------------------------------------------------------------------------------------------ the colours
  // ONE table, a light and a dark column (RECEPTION_MAP_PLAN.md 5.8): a neutral grey map (decision 5), so that the
  // stations of the later stages carry the colour. `night` is the black laid over the map at full night.
  struct Colours
  {
    char const * sea;
    char const * land;
    char const * border;
    double night;
    char const * ink;       // our station's mark, the line to a station, a tooltip's text
    char const * surface;   // the ring that sets a mark off from the map, the status line's ground
    // stage 3, the stations:
    char const * ink2;      // the outline of a mark, the status line's text
    char const * other;     // a station that only reports
    char const * ramp[4];   // one that hears us, by the SNR it gave us: -18 and below, to -10, to -1, from 0 - the
                            // strongest the farthest from the map's own colours in both columns
    char const * weHear;    // one we hear; the ring and the frame of one heard both ways
  };

  inline Colours colours (bool dark)
  {
    return dark ? Colours {"#161b21", "#2d333b", "#4f5660", 0.55, "#f0efec", "#1a1a19", "#c3c2b7", "#a3acb6", {"#1c5cab", "#3987e5", "#86b6ef", "#cde2fb"}, "#d95926"}
                : Colours {"#e3e7eb", "#fafaf8", "#b4b8bd", 0.30, "#0b0b0b", "#fcfcfb", "#52514e", "#5c6570", {"#86b6ef", "#5598e7", "#256abf", "#104281"}, "#eb6834"};
  }
}

#endif
