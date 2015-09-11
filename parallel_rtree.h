#include <boost/assert.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/nth_element.hpp>
#include <boost/range/algorithm_ext/copy_n.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/index/parameters.hpp>
#include <boost/geometry/algorithms/comparable_distance.hpp>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/spin_mutex.h>

#include <vector>
#include <iterator>
#include <utility>
#include <algorithm>
#include <type_traits>

#include <cstdint>

#include "osm_point.h"

// Parallel packing assumption making implementation of an rtree to speed up construction.
// Assumption: user passes in a geographically sorted range (e.g. by longitude) we can chunk on.
template <typename Value, std::size_t ChunkSize = 1'000'000>
class parallel_rtree final {
public:
  using value_type = Value;
  using parameters_type = boost::geometry::index::rstar<32>;
  using rtree_type = boost::geometry::index::rtree<value_type, parameters_type>;
  using size_type = typename rtree_type::size_type;

  parallel_rtree() = default;

  template <typename Iter>
  parallel_rtree(Iter first, Iter last);

  template <typename Range>
  parallel_rtree(const Range& rng)
      : parallel_rtree(begin(rng), end(rng)) {}

  size_type size() const;

  bool empty() const;

  void clear();

  template <typename Predicates, typename OutIter>
  size_type query(const Predicates& predicates, OutIter out) const;

private:
  std::vector<rtree_type> forest;
};


template <typename Value, std::size_t ChunkSize>
template <typename Iter>
parallel_rtree<Value, ChunkSize>::parallel_rtree(Iter first, Iter last) {
  BOOST_ASSERT_MSG(first <= last, "backwards range not supported");

  // Keep locking during construction local out of the class's interface.
  // Idea: create rtrees locally per thread, acquire lock only for adding them to the forest.

  // Spinlock, because adding a rtree is quick (just a move), so threads can spin just as well.
  using mutex_type = tbb::spin_mutex;

  // Populate forest locally and then switch to member attribute for exception safety.
  decltype(forest) preForest;
  mutex_type preForestLock;

  using range_type = tbb::blocked_range<Iter>;
  range_type range{first, last, ChunkSize};

  tbb::parallel_for(range, [&preForest, &preForestLock](const range_type& chunk) {
    // Build rtrees locally in parallel on chunks without holding a lock.
    rtree_type localRtree{chunk};

    // Only acquire lock for quickly adding an rtree to the forest.
    mutex_type::scoped_lock guard{preForestLock};
    preForest.push_back(std::move(localRtree));
  });

  // All rtrees built, good to switch in.
  std::swap(preForest, forest);
}


template <typename Value, std::size_t ChunkSize>
typename parallel_rtree<Value, ChunkSize>::size_type parallel_rtree<Value, ChunkSize>::size() const {
  return boost::accumulate(forest, size_type(0), [](const auto n, const auto& rtree) { return n + rtree.size(); });
}

template <typename Value, std::size_t ChunkSize>
bool parallel_rtree<Value, ChunkSize>::empty() const {
  return !boost::algorithm::any_of(forest, [](const auto& rtree) { return !rtree.empty(); });
}


template <typename Value, std::size_t ChunkSize>
void parallel_rtree<Value, ChunkSize>::clear() {
  boost::for_each(forest, [](const auto& rtree) { rtree.clear(); });
}


// Queries are so fast we don't need parallelism here, would only introduce overhead.
template <typename Value, std::size_t ChunkSize>
template <typename Predicates, typename OutIter>
typename parallel_rtree<Value, ChunkSize>::size_type parallel_rtree<Value, ChunkSize>::query(
    const Predicates& predicates, OutIter out) const {

  // For all nearest neighbors, we query all rtrees and manually pick the nearest k items.
  // TODO: implement this in a more elegant and abstract way.
  using nearest_type = boost::geometry::index::detail::nearest<Value>; // XXX: hack, is impl. detail
  static_assert(std::is_same<Predicates, nearest_type>(), "only nearest neighbor query merging supported right now");

  const auto k = static_cast<std::size_t>(predicates.count);

  std::vector<value_type> all;
  all.reserve(k * forest.size());

  const auto gather = [&all, &predicates](const auto& rtree) { rtree.query(predicates, std::back_inserter(all)); };
  boost::for_each(forest, gather);

  const auto n = std::min(k, all.size());

  const auto dist = [&predicates](const auto& lhs, const auto& rhs) {
    return boost::geometry::comparable_distance(predicates.point_or_relation, lhs) <
           boost::geometry::comparable_distance(predicates.point_or_relation, rhs);
  };

  boost::nth_element(all, begin(all) + n, dist);
  boost::copy_n(all, n, out);

  return n;
}
