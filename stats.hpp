#ifndef STATS_HPP
#define STATS_HPP

#include "sdi_args.hpp"

#include <vector>
#include <boost/optional.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

namespace ba = boost::accumulators;

class stats
{
  // The singleton of the class.
  static stats *singleton;

  // The accumulator type with double values.
  typedef ba::accumulator_set<double, ba::features<ba::tag::count,
                                                   ba::tag::mean>> dbl_acc;

  // The number of new links used in reconfiguration.
  std::vector<dbl_acc> m_newrc;
  // The number of old links used in reconfiguration.
  std::vector<dbl_acc> m_oldrc;
  // The number of links of the reconfigured connection.
  std::vector<dbl_acc> m_nolrc;

  // The probability of reconfiguring a connection.
  std::vector<dbl_acc> m_prc;
  // The length of the reconfigured connection.
  std::vector<dbl_acc> m_lenrc;
  // The number of slices of the reconfigured connection.
  std::vector<dbl_acc> m_nscrc;

  // The simulation arguments.
  sdi_args args;
  
public:
  stats(const sdi_args &args);
  ~stats();

  // Returns the singleton.
  static stats &
  get();

  // Report the reconfigured connection.
  void
  operator()(const connection &conn, int dist,
             boost::optional<std::pair<int, int>> result);
};

#endif
