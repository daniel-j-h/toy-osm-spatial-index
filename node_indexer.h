#pragma once

#include <utility>

#include <osmium/osm/types.hpp>
#include <osmium/handler.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <tbb/parallel_sort.h>

#include "osm_point.h"
#include "parallel_rtree.h"

// Spatially indexes nodes for specific ways.
struct rtree_indexer_t : osmium::handler::Handler {
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


  // To use the packing implementation, store all nodes and then construct the index in parallel at once.
  void pack() {
    tbb::parallel_sort(points);
    boost::erase(points, boost::unique<boost::return_found_end>(points));

    tbb::parallel_sort(points, [](const auto& lhs, const auto& rhs) { return lhs.lon < rhs.lon; });
    parallel_rtree<osm_point_t> packed{points};

    using std::swap; // ADL
    swap(rtree, packed);

    // Reclaim memory. This also means until here we keep 1/ all points and 2/ the rtree in memory (hint: bad).
    decltype(points) empty{};
    swap(points, empty);
  }

  std::vector<osm_point_t> points;
  parallel_rtree<osm_point_t> rtree;
};
