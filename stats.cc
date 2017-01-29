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
stats::reconfigured(bool status)
{
  m_prc(status);
}

void
stats::reconfigured_conn(const connection &conn, int newrc, int oldrc)
{
  int len = conn.get_len();
  int nol = conn.get_nol();
  int nsc = conn.get_nsc();

  m_lenrc(len);
  m_nolrc(nol);
  m_nscrc(nsc);
  m_newrc(newrc);
  m_oldrc(oldrc);
}
