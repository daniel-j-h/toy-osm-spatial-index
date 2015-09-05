#pragma once

#include <cstdint>
#include <cmath>
#include <ostream>
#include <iomanip>
#include <limits>

#include <osmium/osm/location.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/io/ios_state.hpp>

namespace {

// Represents a OSM node-based (lon, lat)-tuple that in addition stores its OSM ID.
struct osm_point_t {
  static const constexpr auto precision = osmium::Location::coordinate_precision;
  static const constexpr auto digits = std::floor(std::log10(osm_point_t::precision)); // after comma

  using oid_t = std::uint32_t;
  using degree_t = std::int32_t;

  oid_t oid;
  degree_t lon, lat;
};

inline std::ostream& operator<<(std::ostream& out, const osm_point_t& point) {
  const auto lon = static_cast<double>(point.lon) / osm_point_t::precision;
  const auto lat = static_cast<double>(point.lat) / osm_point_t::precision;

  boost::io::ios_flags_saver defer{out};
  out << std::fixed << std::setprecision(osm_point_t::digits);

  return out << "{oid: " << point.oid << ", lon: " << lon << ", lat: " << lat << "}";
}

inline bool operator==(const osm_point_t& lhs, const osm_point_t& rhs) { return lhs.oid == rhs.oid; }
inline bool operator<(const osm_point_t& lhs, const osm_point_t& rhs) { return lhs.oid < rhs.oid; }
}

// BOOST_GEOMETRY_REGISTER_POINT_2D(osm_point_t, osm_point_t::degree_t, cs::geographic<degree>, lon, lat)
// see: https://svn.boost.org/trac/boost/ticket/9758

BOOST_GEOMETRY_REGISTER_POINT_2D(osm_point_t, osm_point_t::degree_t, cs::spherical_equatorial<degree>, lon, lat)
