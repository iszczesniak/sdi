#ifndef STATS_HPP
#define STATS_HPP

#include "event.hpp"
#include "connection.hpp"
#include "graph.hpp"
#include "sdi_args.hpp"
#include "traffic.hpp"

#include <vector>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/timer/timer.hpp>

namespace ba = boost::accumulators;
using boost::timer::cpu_timer;
using boost::timer::cpu_times;

class client;

class stats
{
  // The singleton of the class.
  static stats *singleton;

  // The traffic of the run.
  const traffic &tra;

  // The accumulator type with double values.
  typedef ba::accumulator_set<double, ba::features<ba::tag::count,
                                                   ba::tag::mean>> dbl_acc;

  // The probability of establishing a connection.
  dbl_acc m_pec;
  // The length of the established connection.
  dbl_acc m_lenec;
  // The number of links of the established connection.
  dbl_acc m_nolec;
  // The number of slices of the established connection.
  dbl_acc m_nscec;

  // The probability of reconfiguring a connection.
  dbl_acc m_prc;
  // The length of the reconfigured connection.
  dbl_acc m_lenrc;
  // The number of links of the reconfigured connection.
  dbl_acc m_nolrc;
  // The number of slices of the reconfigured connection.
  dbl_acc m_nscrc;
  
  // The number of new links used in reconfiguration.
  dbl_acc m_newrc;
  // The number of old links used in reconfiguration.
  dbl_acc m_oldrc;

  // The arguments of the run.
  sdi_args args;
  // The total timer, i.e., keeps track from the beginning of the run.
  cpu_timer ttimer;
  // The delta timer, i.e., keeps track from the last report time.
  cpu_timer dtimer;
  
public:
  stats(const sdi_args &, const traffic &);

  static stats &
  get();

  // Report the reconfigured connection.
  void
  operator()(const connection &conn, int dist, bool status,
             int newrc, int oldrc);
};

#endif
