#include "connection.hpp"
#include "routing.hpp"
#include "utils.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

#include <boost/none.hpp>

using namespace std;

int connection::counter = 0;

connection::re_t connection::m_re = connection::re_t::none;

// Returns a pair of integers, where the first is the number of links
// in new_path not found in old_path, and the second is the number of
// links in new_path also found in old_path.
std::pair<int, int>
calc_links(const sscpath &new_path, const sscpath &old_path)
{
  int newlinks = 0;
  int oldlinks = 0;

  // Now we need to calculate the number of new links to configure,
  // which depends on the SSC.  Check whether the SSC of the new path
  // is the same as the SSC of the old path.
  if (new_path.second == old_path.second)
    {
      // Calculate the number of links to configure, i.e. those links
      // that are in the new path, but are missing in the old path.
      // Iterate over the new path, and calculate those links that are
      // not present in the old path.
      for(path::const_iterator i = new_path.first.begin();
          i != new_path.first.end(); ++i)
        {
          edge e = *i;
          path::const_iterator j = std::find(old_path.first.begin(),
                                             old_path.first.end(), e);
          if (j == old_path.first.end())
            ++newlinks;
          else
            ++oldlinks;
        }
    }
  else
    // Since it's a different SSC, we have to configure all links.
    newlinks = new_path.first.size();

  // Make sure that the newlinks and oldlinks add up.
  assert(newlinks + oldlinks == new_path.first.size());
  
  return std::make_pair(newlinks, oldlinks);
}

connection::connection(graph &g): m_g(g), m_id(counter++)
{
}

connection::~connection()
{
  if (is_established())
    tear_down();
}

const demand &
connection::get_demand() const
{
  return m_d;
}

bool
connection::is_established() const
{
  return m_p != boost::none;
}

int
connection::get_length() const
{
  assert(is_established());

  boost::property_map<graph, boost::edge_weight_t>::type
    wm = get(boost::edge_weight_t(), m_g);

  int length = 0;
  const path &p = m_p.get().first;
  
  for(const edge &e: p)
    length += get(wm, e);

  return length;
}

int
connection::get_hops() const
{
  assert(is_established());
  return m_p.get().first.size();
}

int
connection::get_nsc() const
{
  assert(is_established());
  return m_p.get().second.size();
}

// Set the reconfiguration type.
void
connection::set_re(const std::string &re)
{
  map <string, connection::re_t> re_map;
  re_map["complete"] = connection::re_t::complete;
  re_map["curtailing"] = connection::re_t::curtailing;
  re_map["proposed"] = connection::re_t::proposed;
  m_re = interpret ("reconfiguration type", re, re_map);
}

// Get the reconfiguration type.
connection::re_t
connection::get_re()
{
  return m_re;
}

bool
connection::establish(const demand &d)
{
  // Make sure the connection is not established.
  assert(!is_established());

  // Route the demand.
  m_p = routing::route(m_g, d);

  // If successful, remember the demand.
  if (m_p)
    m_d = d;

  return is_established();
}

boost::optional<std::pair<int, int> >
connection::reconfigure(vertex new_dst)
{
  assert(is_established());

  boost::optional<std::pair<int, int> > result;

  vertex src = m_d.first.first;
  // That's the new demand.
  demand nd = demand(npair(src, new_dst), m_d.second);

  // The old path.
  sscpath old_p = m_p.get();

  bool status;

  switch(m_re)
    {
    case re_t::complete:
      // The complete reconfiguration.
      status = reconfigure_complete(nd);
      break;

    case re_t::curtailing:
      // The curtailing reconfiguration.
      status = reconfigure_curtailing(nd);
      break;

    case re_t::proposed:
      // The proposed reconfiguration.
      status = reconfigure_proposed(nd);
      break;

    default:
      assert(false);
    }

  if (status)
    {
      result = calc_links(m_p.get(), old_p);
      m_d = nd;
    }

  return result;
}

bool
connection::reconfigure_complete(const demand &nd)
{
  assert(is_established());

  // Remember the previous path.
  auto pp = m_p;

  // First we need to tear down the existing path.  We might need its
  // slices to establish a new connection.
  tear_down();

  bool status = establish(nd);

  // If failed, we have to establish the connection as before.
  if (!status)
    {
      m_p = pp;
      bool status = routing::set_up_path(m_g, m_p.get());
      assert(status);
    }

  return status;
}

bool
connection::reconfigure_curtailing_worker(const demand &nd)
{
  assert(is_established());

  // The number of slices requested.
  int nsc = m_d.second;
  // The SSC that we request.
  const SSC &ssc = m_p.get().second;

  // This is the best result found.  The pair of integers: the first
  // is the number of hops in the bridging part of the path, while the
  // second is the number of hops in the reused part of the path.
  boost::optional<pair<sscpath, pair<int, int> > > rp;

  // The new destination node of the connection.
  vertex new_dst = nd.first.second;

  // Iterate over the edges of the path, and take them down.
  for (path p = m_p.get().first; !p.empty(); p.pop_back())
    {
      // The last edge.
      edge e = p.back();
      // The intermediate vertex to which we bridge.
      vertex iv = boost::target(e, m_g);
      // The new bridging demand.
      demand bd(npair(iv, new_dst), nsc);

      // The bridging path.
      boost::optional<sscpath> bp; // = routing::route(bd, ssc);

      // Did we find a result?
      if (bp != boost::none)
        {
          // The candidate cost.
          pair<int, int> cc(bp.get().first.size(), p.size());

          // Is this result better than some previously found?
          if (rp == boost::none || cc < rp.get().second)
            {
              path cp = p;
              cp.insert(cp.end(),
                        bp.get().first.begin(), bp.get().first.end());
              // Remember this as the best result.
              rp = make_pair(sscpath(cp, ssc), cc);
            }
        }

      // Tear down the edge.
      routing::tear_down(m_g, sscpath(path{e}, ssc));
    }

  // Set up the reconfigured path if we found one.
  if (rp != boost::none)
    {
      bool status = routing::set_up_path(m_g, rp.get().first);
      assert(status);
      m_p = rp.get().first;
    }

  return rp != boost::none;
}

bool
connection::reconfigure_curtailing(const demand &nd)
{
}

bool
connection::reconfigure_proposed(const demand &nd)
{
  return false;
}

void
connection::tear_down()
{
  assert(is_established());
  routing::tear_down(m_g, m_p.get());
  m_p = boost::none;
}

/*
          // Build the new path np from p and bp.
          np = p;
          // Take the SSC from bp if p is empty.  This can happen in
          // the case we examine the destination node as the end point
          // of the briding connection.
          if (np.second.first.empty())
            np.second.second = bp.second.second;

          // We want the SSC in the additional path to be the same as
          // in the existing path.
          assert(!np.second.second.empty());
          np.second.first.insert(np.second.first.begin(),
                                 bp.second.first.begin(),
                                 bp.second.first.end());

          assert(new_src == source(np.second.first.front(), g));
        }

      // This is the condition for breaking the loop which should go
      // exactly here.  This allows us to consider also the dst node
      // as the end node for the briding path, and prevents the
      // execution of the code below.
      if (p.second.first.empty())
        break;

      // Take down the leading edge in the path.
      edge ettd = p.second.first.front();
      p.second.first.pop_front();
      // Retrace one node.
      assert(source(ettd, g) == int_src);
      int_src = target(ettd, g);
      // Take down that edge!
      sscpath sscpathttd;
      sscpathttd.first.push_back(ettd);
      sscpathttd.second = p.second.second;
      dijkstra::tear_down_path(g, sscpathttd);
    }
*/
