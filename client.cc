#include "client.hpp"

#include <utility>

#include "dijkstra.hpp"
#include "simulation.hpp"
#include "stats.hpp"
#include "traffic.hpp"
#include "utils_netgen.hpp"

using namespace std;

client::client(double mht, double mbst, double mdct, double mnsc,
               traffic &tra):
  mht(mht), htd(mht), htg(rng, htd),
  mbst(mbst), bstd(mbst), bstg(rng, bstd),
  mdct(mdct), dctd(mdct), dctg(rng, dctd),
  mnsc(mnsc - 1), nscd(mnsc), nscdg(rng, nscd),
  conn(g), st(stats::get()), tra(tra)
{
  // Tear down time.
  tdt = now() + htg();
  schedule();
}

void client::operator()(double t)
{
  if (!conn.is_established())
    set_up();
  else
    {
      if (t != tdt)
        reconfigure();
      else
        tear_down();
    }
}

pair<bool, int>
client::set_up()
{
  // The new demand.
  demand d;
  // The demand end nodes.
  d.first = random_node_pair(g, rng);
  // The number of subcarriers the signal requires.  It's Poisson + 1.
  d.second = nscdg() + 1;

  // The conection client is now idle, and should get busy now.
  pair<bool, int> result = conn.set_up(d);

  st.established(result.first);

  if (result.first)
    {
      st.established_length(result.second);
      schedule_next();
    }
  else
    destroy();
}

std::pair<bool, int>
client::reconfigure()
{
  /*
  // It's time now to reconfigure.
  pair<bool, int> result = reconfigure();
  if (result.first)
    {
      --nc_left;
      st->reconfigured_links(result.second);
    }
  else
    {
      nc_left = 0;
      idle = true;
      conn.tear_down();
      st->completed(false);
    }
  
  // We change the source node, and the destination node stays
  // unchanged.  We choose the new source node from one of the
  // neighbours of the current source node.
  vertex old_src = conn.get_demand().first.first;

  // From these vertexes we can reach the current source node.
  set<vertex> sov;
  boost::graph_traits<graph>::in_edge_iterator ei, eei;
  for(boost::tie(ei, eei) = boost::in_edges(old_src, g); ei != eei; ++ei)
    sov.insert(source(*ei, g));

  // Now we chose at random from one of the found nodes.
  vertex new_src = get_random_element(sov, rng);

  return conn.reconfigure(new_src);
  */
}

void client::tear_down()
{
  conn.tear_down();
  st.completed(true);
}

// Schedule the next event based on the current time t.
void client::schedule_next()
{
  // The time to switch to the next base station.
  double bst = now() + bstg();

  // Next time.
  double nt = bst < tdt ? bst : tdt;

  schedule(nt);
}

void client::destroy()
{
  tra.erase(this);
  delete this;
}
