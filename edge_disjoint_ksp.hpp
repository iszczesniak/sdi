//=======================================================================
// Copyright 2015 by Ireneusz Szcześniak
// Authors: Ireneusz Szcześniak
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

//=======================================================================
// Function edge_disjoint_ksp finds the k-shortest paths which are
// edge-disjoint, i.e. the paths do not have common edges.  The graph
// can have parallel edges.  The algorithm employs the Dijkstra
// algorithm to find the shortest path.  Next the edges of the found
// path are disabled, and Dijkstra finds the second shortest path.
// This process is repeated until k paths are found or there are no
// more paths to be found.
// =======================================================================

#ifndef BOOST_GRAPH_EDGE_DISJOINT_KSP
#define BOOST_GRAPH_EDGE_DISJOINT_KSP

#include <list>
#include <set>

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/optional.hpp>   
#include <boost/property_map/vector_property_map.hpp>
#include <boost/utility/value_init.hpp>

namespace boost {

  // Exclude edge filter
  template <typename Graph>
  struct edksp_filter
  {
    typedef typename Graph::edge_descriptor edge_descriptor;
    typedef typename std::set<edge_descriptor> edge_set;

    // The filter must be default-constructible, so it is.
    edksp_filter(): m_excluded() {};
    
    edksp_filter(const edge_set *excluded): m_excluded(excluded) {};

    inline bool operator()(const edge_descriptor &e) const
    {
      return m_excluded->count(e) == 0;
    }

    const edge_set *m_excluded;
  };

  template <typename Graph, typename Weight>
  optional<std::pair<typename Weight::value_type,
                     std::list<typename Graph::edge_descriptor>>>
  custom_dijkstra_call(const Graph &g,
                       typename graph_traits<Graph>::vertex_descriptor s,
                       typename graph_traits<Graph>::vertex_descriptor t,
                       Weight wm)
  {
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
    typedef typename std::list<typename Graph::edge_descriptor> path_type;
    typedef typename Weight::value_type weight_type;
    typedef typename std::pair<weight_type, path_type> kr_type;
    
    vector_property_map<edge_descriptor> pred(num_vertices(g));
  
    dijkstra_shortest_paths
      (g, s, weight_map(wm).
       visitor(make_dijkstra_visitor(record_edge_predecessors(pred, on_edge_relaxed()))));

    optional<kr_type> result;

    // Was the solution found?
    if (pred[t] != edge_descriptor())
      {
        // The cost of the shortest path.
        value_initialized<weight_type> cost;
        // The path found.
        path_type path;

        // Trace the solution to the source.
        vertex_descriptor c = t;
        while (c != s)
          {
            const edge_descriptor &e = pred[c];
            // Build the path.
            path.push_front(e);
            // Calculate the cost of the path.
            cost += get(wm, e);
            // Find the predecessing vertex.
            c = source(e, g);
          }

        result = std::make_pair(cost, path);
      }

    return result;
  }
  
  template <typename Graph, typename Weight>
  std::list<std::pair<typename Weight::value_type,
                      std::list<typename Graph::edge_descriptor>>>
  edge_disjoint_ksp(const Graph& g,
                    typename graph_traits<Graph>::vertex_descriptor s,
                    typename graph_traits<Graph>::vertex_descriptor t,
                    Weight wm,
                    optional<unsigned int> K)
  {
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
    typedef typename std::list<typename Graph::edge_descriptor> path_type;
    typedef typename Weight::value_type weight_type;
    typedef typename std::pair<weight_type, path_type> kr_type;

    // The result.
    std::list<std::pair<weight_type, path_type>> result;

    // The set of excluded edges.
    std::set<edge_descriptor> excluded;
    // The filter for excluding edges.
    edksp_filter<Graph> f(&excluded);
    // The filtered graph type.
    typedef filtered_graph<Graph, edksp_filter<Graph> > fg_type;
    // The filtered graph.
    fg_type fg(g, f);

    // In each iteration, we try to find a shortest path.
    do
      {
        // If required, stop at the K-th shortest path.
        if (K && result.size() == K.get())
          break;
          
        // This is the optional k-th result.
        optional<kr_type> okr = custom_dijkstra_call(g, s, t, wm);

        if (!okr)
          break;

        // This is the k-th result.
        const kr_type &kr = okr.get();
        
        // Exclude the edges from the k-th result found.
        for(auto const &e: kr.second)
          excluded.insert(e);

        result.push_back(kr);

      } while(true);
      
    return result;
  }

  template <typename Graph>
  std::list<std::pair<typename property_map<Graph, edge_weight_t>::value_type,
                      std::list<typename Graph::edge_descriptor>>>
  edge_disjoint_ksp(const Graph& g,
                    typename graph_traits<Graph>::vertex_descriptor s,
                    typename graph_traits<Graph>::vertex_descriptor t,
                    optional<unsigned int> K = optional<unsigned int>())
  {
    return edge_disjoint_ksp(g, s, t, get(edge_weight_t(), g), K);
  }

} // boost

#endif /* BOOST_GRAPH_EDGE_DISJOINT_KSP */
