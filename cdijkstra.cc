#include "cdijkstra.hpp"
#include "graph.hpp"
#include "utils.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <queue>
#include <map>
#include <utility>

using namespace std;

sscpath
cdijkstra::route_w(graph &g, const demand &d)
{
  // We allow to allocate the signal on any of the slices.
  V2C2S r = search(g, d);
  sscpath result = trace(g, r, d);
  if (!result.first.empty())
    {
      bool status = set_up_path(g, result);
      assert(status);
    }

  return result;
}

bool
cdijkstra::has_better_or_equal(const C2S &c2s, const COST &cost, const SSC &ssc)
{
  // We examine the existing results with the cost lower or equal to
  // "cost".
  for (auto const &e: c2s)
    {
      // Stop searching when we reach a result with the cost higher
      // than "cost".
      if (get<0>(e.first) > cost)
        break;

      // Check whether the existing result includes "ssc".
      if (includes(e.second, ssc))
        return true;
    }

  return false;
}

// I tried implementing this function with the reverse iterators and
// with regular iterators iterated backwards, but it was a mess, so I
// stuck to regular iterators iterated forward.  I was also thinking
// about using upper_bound to locate the first element to consider,
// but it actually might turn out less effective.
void
cdijkstra::purge_worse(pqueue &q, C2S &c2s, const COST &cost, const SSC &ssc)
{
  auto i = c2s.begin();

  while (i != c2s.end())
    {
      // We're advancing the iterator now, because at the end of the
      // loop it already can be invalid because of the erasure.
      auto i2 = i++;

      const CEV &cev = i2->first;

      // We consider only worse results.
      if (get<0>(cev) > cost)
        {
          // The SSC of a worse result.
          SSC &wr_ssc = i2->second;

          // If wr_ssc is included in ssc, discard the CEV from both q
          // and c2s.
          if (includes(ssc, wr_ssc))
            {
              c2s.erase(i2);
              // Remove the corresponding element from the queue, only
              // if there is no another cev in c2s.
              if (c2s.find(cev) == c2s.end())
                q.erase(cev);
            }
        }
    }
}

void
cdijkstra::relax(pqueue &q, C2S &c2s, const CEV &cev, const SSC &ssc)
{
  const COST &cost = get<0>(cev);

  // We can ignore the new result (given as arguments cev and ssc) if
  // in c2s there is already a better or equal result.
  if (!has_better_or_equal(c2s, cost, ssc))
    {
      // OK, this is the best result so far, so let's use it.
      q.insert(cev);
      c2s[cev].insert(ssc);

      if (c2s[cev].size() > 1)
        cout << "size = " << c2s[cev].size() << endl;

      // There might be worse results from previous relaxations, and
      // so we need to remove them, because they are useless.
      purge_worse(q, c2s, cost, ssc);
    }
}

V2C2S
cdijkstra::search(const graph &g, const demand &d)
{
  SSC ssc;
  vertex src = d.first.first;

  // Itereate over the out edges of the vertex.
  graph::out_edge_iterator ei, eei;
  for(boost::tie(ei, eei) = boost::out_edges(src, g); ei != eei; ++ei)
    {
      // The edge that we examine in this iteration.
      const edge &e = *ei;
      // The slices available on the edge.
      const SSC &e_ssc = boost::get(boost::edge_ssc, g, e);

      // Add e_ssc to ssc.
      include(ssc, e_ssc);
    }

  return search(g, d, ssc);
}

V2C2S
cdijkstra::search(const graph &g, const demand &d, const SSC &src_ssc)
{
  V2C2S r;

  vertex src = d.first.first;
  vertex dst = d.first.second;
  int nsc = d.second;

  // The null edge.
  const edge &ne = *(boost::edges(g).second);

  // We have to filter ssc to exclude slices that can't support
  // the signal with p slices.
  SSC src_ssc_nsc = exclude(src_ssc, nsc);

  if (!src_ssc_nsc.empty())
    {
      // We put here the information that allows us to process the
      // source node in the loop below.  We say that we reach source
      // node src with cost (0, 0) on the slices passed in the
      // ssc argument along the null edge.  The null edge signals the
      // beginning of the path.
      r[src][CEV(COST(0, 0), ne, src)].insert(src_ssc_nsc);

      // The priority queue.
      pqueue q;
      // We reach vertex src with null cost along the null edge.
      q.insert(CEV(COST(0, 0), ne, src));

      while(!q.empty())
        {
          const CEV &cev = *(q.begin());
          const COST &cost = get<0>(cev);
          int c = cost.first;
          int h = cost.second;
          vertex v = get<2>(cev);
          q.erase(q.begin());

          // Stop searching when we reach the destination node.
          if (v == dst)
            break;

          // C2S for node v.
          const C2S &c2s = r[v];

          // The CEV that we process in this loop has to be in the
          // C2S.
          C2S::const_iterator j = c2s.find(cev);
          assert(j != c2s.end());

          // This SSSC is now available at node v for further search.
          // There might be other slices available in the c2s,
          // but we care only about the one that we got with edge e at
          // cost c.
          const SSSC &v_sssc = j->second;

          // Itereate over the out edges of the vertex.
          graph::out_edge_iterator ei, eei;
          for(boost::tie(ei, eei) = boost::out_edges(v, g); ei != eei; ++ei)
            {
              // The edge that we examine in this iteration.
              const edge &e = *ei;

              // The cost of the edge.
              int ec = boost::get(boost::edge_weight, g, e);
              // Candidate cost.
              int c_c = c + ec;

              // Consider that path when there is no maximal length
              // given or when the new lenght is not greater than the
              // limit.
              if (!m_ml || c_c <= m_ml.get())
                {
                  // The slices available on the edge.
                  const SSC &e_ssc = boost::get(boost::edge_ssc, g, e);
                  // Candidate SSC: the ssc available at node v that
                  // can be carried by edge e, and that has at least
                  // nsc contiguous slices.
                  SSSC c_sssc = exclude(intersection(v_sssc, e_ssc), nsc);

                  if (!c_sssc.empty())
                    {
                      // Candidate number of hops.
                      int c_h = h + 1;
                      // Candidate COST.
                      COST c_cost = COST(c_c, c_h);
                      // The target vertex of the edge.
                      vertex t = boost::target(e, g);
                      // Candidate CEV.
                      CEV c_cev = CEV(c_cost, e, t);

                      // Deal with the new solution.
                      relax(q, r[t], c_cev, c_sssc);
                    }
                }
            }
        }
    }

  return r;
}

sscpath
cdijkstra::trace(const graph &g, const V2C2S &r, const demand &d)
{
  sscpath p;

  vertex src = d.first.first;
  vertex dst = d.first.second;
  int nsc = d.second;

  // Find the path from the back.  This is the current node.
  V2C2S::const_iterator dst_i = r.find(dst);

  // Make sure there is some data for node dst in r.
  if (dst_i != r.end())
    {
      // The C2S of the destination node.
      const C2S &dst_c2s = dst_i->second;

      if (!dst_c2s.empty())
        {
          // For dst, the first element in C2S is the best result,
          // i.e. of the lowest cost.
          C2S::const_iterator bri = dst_c2s.begin();

          // This is the cost of the whole path.
          COST c = get<0>(bri->first);

          // This is the path SSC.
          const SSC ssc = select_ssc(bri->second, nsc);
          // And we remember this as the final result.
          p.second = ssc;

          // We start with the destination node.
          vertex crt = dst;
          while(crt != src)
            {
              V2C2S::const_iterator i = r.find(crt);
              assert(i != r.end());
              const C2S &c2s = i->second;

              // We look for the solution that costs c and that
              // contains SSC.
              C2S::const_iterator j;
              for(j = c2s.begin(); j != c2s.end(); ++j)
                if (get<0>(j->first) == c && includes(j->second, ssc))
                  break;

              // Make sure we found the right CEV.
              assert(j != c2s.end());

              const edge &e = get<1>(j->first);
              const SSC &essc = boost::get(boost::edge_ssc, g, e);
              assert(includes(essc, ssc));
              p.first.push_front(e);
              c.first -= boost::get(boost::edge_weight, g, e);
              --c.second;
              assert(crt == boost::target(e, g));

              crt = boost::source(e, g);
            }

          // The cost at the source node, that we should have reached
          // by now, should be 0.
          assert(c.first == 0);
          assert(c.second == 0);
        }
    }

  return p;
}
