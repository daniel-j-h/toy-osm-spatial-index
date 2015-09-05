#pragma once

#include <utility>

#include <osmium/osm/types.hpp>
#include <osmium/handler.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/index/parameters.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/range/algorithm.hpp>

#include "osm_point.h"

namespace {

// Spatially indexes nodes for specific ways.
struct rtree_indexer_t : osmium::handler::Handler {
  // using rtree_param_t = boost::geometry::index::linear<64>;
  // using rtree_param_t = boost::geometry::index::quadratic<32>;
  using rtree_param_t = boost::geometry::index::rstar<32>;

  using rtree_t = boost::geometry::index::rtree<osm_point_t, rtree_param_t>;

  void way(const osmium::Way& way) {
    // Just an example here, we should also look into osmium's filter features
    if (!way.get_value_by_key("highway"))
      return;

    const auto& nodes = way.nodes();

    for (auto&& node : nodes) {
      const auto osm_id = node.positive_ref();
      const auto& loc = node.location();

      if (loc) {
        points.push_back({boost::numeric_cast<osm_point_t::oid_t>(osm_id),
                          boost::numeric_cast<osm_point_t::degree_t>(loc.x()),
                          boost::numeric_cast<osm_point_t::degree_t>(loc.y())});
      }
    }
  }


  // To use the packing implementation, store all nodes and then construct the index at once.
  void pack() {
    rtree_t packed{boost::unique(boost::sort(points))};

    using std::swap; // ADL
    swap(rtree, packed);

    // Reclaim memory
    decltype(points) empty{};
    swap(points, empty);
  }

  std::vector<osm_point_t> points;
  rtree_t rtree;
};
}
