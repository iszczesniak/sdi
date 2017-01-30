#include "client.hpp"
#include "stats.hpp"
#include "utils_netgen.hpp"

#include <iostream>

using namespace std;

// The singleton of the class.  The compiler initializes it to null.
stats *stats::singleton;

stats::stats(const sdi_args &args, const traffic &tra):
  args(args), tra(tra)
{
  assert(!singleton);
  singleton = this;

  // The seed of the simulation.
  cout << "seed" << " ";
  // The hash of the simulation.
  cout << "hash" << " ";
  // The measured value.
  cout << "value" << " ";
  // Distance
  cout << "d1 d2 d3 d4 d5 d6 d7 d8 d9 d10";

  // That's it.  Thank you.
  cout << endl;
}

stats &
stats::get()
{
  return *singleton;
}

void
stats::operator()(const connection &conn, int hops, bool status,
                  int newrc, int oldrc)
{
  assert(hops >= 1);

  // We care about the stats only for the hops <= 10.
  if (hops <= 10)
    {
      m_prc[hops](status);
  
      if (status)
        {
          int len = conn.get_len();
          int nol = conn.get_nol();
          int nsc = conn.get_nsc();

          m_newrc[hops](newrc);
          m_oldrc[hops](oldrc);
          m_nolrc[hops](nol);

          m_lenrc[hops](len);
          m_nscrc[hops](nsc);
        }
    }
}
