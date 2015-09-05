#pragma once

#include <osmium/osm/types.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>

namespace {

// Provides a node cache handler that internally stores nodes for their associated ways.
struct node_cache_t {
  using index_positive_t = osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>;
  using handler_t = osmium::handler::NodeLocationsForWays<index_positive_t>;

  index_positive_t index_positive;
  handler_t handler{index_positive};
};
}
