#include "client.hpp"

#include <utility>

#include "simulation.hpp"
#include "stats.hpp"
#include "traffic.hpp"
#include "utils_netgen.hpp"

using namespace std;

client::client(double mht, double mnsc, traffic &tra):
  mht(mht), htd(1 / mht), htg(rng, htd),
  mnsc(mnsc), nscd(mnsc - 1), nscdg(rng, nscd),
  conn(g), st(stats::get()), tra(tra)
{
  // We try to setup the connection and reconfigure it.  We schedule
  // the connection to be torn down only when it was setup and
  // reconfigured.
  if (set_up() && reconfigure())
    {
      // Register the client with the traffic.
      tra.insert(this);
      // Holding time.
      double ht = htg();
      // Tear down time.
      tdt = now() + ht;
      schedule(tdt);
    }
  else
    tra.delete_me_later(this);
}

void client::operator()(double t)
{
  tear_down();
}

bool client::set_up()
{
  // The new demand.
  demand d;
  // The demand end nodes.
  d.first = random_node_pair(g, rng);
  // The number of slices the signal requires.  It's Poisson + 1.
  d.second = nscdg() + 1;

  // Set up the connection.
  bool status = conn.establish(d);

  // Report whether the connection was established or not.
  st.established(status);

  // If established, report the connection.
  if (status)
    st.established_conn(conn);

  return status;
}

bool
client::reconfigure()
{
  assert(conn.is_established());

  // Choose the next vertex.
  vertex new_src;
  
  // Reconfigure the connection for the new source vertex.
  auto result = conn.reconfigure(new_src);
  st.reconfigured(result.first);

  if (result.first)
    st.reconfigured(result.second);

  return result.first;
}

const connection &
client::get_connection() const
{
  return conn;
}

void client::tear_down()
{
  assert(conn.is_established());
  conn.tear_down();
  tra.erase(this);
  tra.delete_me_later(this);
}
