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

  typedef std::vector<dbl_acc> acc_vec;

  // The number of new links used in reconfiguration.
  acc_vec m_newrc;
  // The number of old links used in reconfiguration.
  acc_vec m_oldrc;
  // The number of links of the reconfigured connection.
  acc_vec m_nolrc;

  // The probability of reconfiguring a connection.
  acc_vec m_prc;
  // The length of the reconfigured connection.
  acc_vec m_lenrc;
  // The number of slices of the reconfigured connection.
  acc_vec m_nscrc;

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

private:
  void
  print(const string &txt, const acc_vec &vec);
};

#endif
