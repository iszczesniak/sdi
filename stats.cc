#include "stats.hpp"
#include "utils_netgen.hpp"

#include <iostream>

using namespace std;

stats *stats::singleton;

stats::stats(graph &g, pqueue &q):
  g(g), module(q)
{
  assert(!singleton);
  singleton = this;
  schedule(0);
}

stats::~stats()
{
  cout << "stats at exit: " << endl;
  cout << "mean network load = " << ba::mean(nla) << endl;
  cout << "mean probability of establishing a connection = "
       << ba::mean(tcea) << endl;
  cout << "mean probability of completing a connection = "
       << ba::mean(tcca) << endl;
}

stats *
stats::get()
{
  return singleton;
}

void stats::operator()(double t)
{
  cout << "stats at t = " << t << ": " << endl;

  // The network load.
  double nl = calculate_load(g);
  nla(nl);
  cout << "load = " << nl;
  cout << endl;

  // The probability of establishing a connection.
  double cep = ba::mean(cea);
  cea = dbl_acc();
  cout << "probability of connection establishment = " << cep << endl;

  // The probability of completing a connection.
  double ccp = ba::mean(cca);
  cca = dbl_acc();
  cout << "probability of connection completion = " << ccp << endl;

  schedule(t);
}

// Schedule the next event based on the current time 0.
void stats::schedule(double t)
{
  // We call the stats every second.
  module::schedule(t + 1);
}

void stats::established(bool status)
{
  cea(status);
  tcea(status);
}

void stats::completed(bool status)
{
  cca(status);
  tcca(status);
}
