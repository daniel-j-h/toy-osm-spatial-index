#include <cstdio>
#include <ios>
#include <locale>
#include <iostream>
#include <vector>
#include <utility>
#include <iterator>
#include <stdexcept>

#include <osmium/io/pbf_input.hpp>
#include <osmium/visitor.hpp>

#define BOOST_SCOPE_EXIT_CONFIG_USE_LAMBDAS
#include <boost/scope_exit.hpp>
#include <boost/timer/timer.hpp>

#include "osm_point.h"
#include "node_cache.h"
#include "node_indexer.h"

int main(int argc, char** argv) try {
  std::locale::global(std::locale(""));
  std::ios_base::sync_with_stdio(false);

  GOOGLE_PROTOBUF_VERIFY_VERSION;
  BOOST_SCOPE_EXIT(void) { google::protobuf::ShutdownProtobufLibrary(); };

  if (argc != 2)
    return EXIT_FAILURE;

  const auto types = osmium::osm_entity_bits::node | osmium::osm_entity_bits::way;
  osmium::io::Reader reader(argv[1], types);

  node_cache_t cache;
  rtree_indexer_t indexer;

  { // Time file reading and node-way association
    boost::timer::auto_cpu_timer t;
    osmium::apply(reader, cache.handler, indexer);
    std::cout << "Reading file and caching nodes:" << std::endl;
  }

  { // Time spatial index construction using packing algorithm
    boost::timer::auto_cpu_timer t;
    indexer.pack();
    std::cout << "Spatial index creation:" << std::endl;
  }

  const auto size = indexer.rtree.size();

  // Query the three nearest neighbors of a coordinate somewhere in Berlin
  const constexpr auto k = 3u;
  osm_point_t p{0, 13'4075810, 52'5197930};

  std::vector<osm_point_t> knn(k);

  { // Time nearest neighbor query on spatial index
    boost::timer::auto_cpu_timer t;
    indexer.rtree.query(boost::geometry::index::nearest(p, k), std::begin(knn));
    std::cout << "Nearest neighbor query on " << size << " items:" << std::endl;
  }

  std::cout << "Nearest three neighbors:" << std::endl;
  for (auto&& point : knn)
    std::cout << ' ' << point << '\n';

} catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << '\n';
  return EXIT_FAILURE;
}
