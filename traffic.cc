#include "traffic.hpp"

using namespace std;

traffic::traffic(double mcat, double mht, double mbst,
                 double mdct, double mnsc):
  mcat(mcat), catd(1 / mcat), catg(rng, catd),
  mht(mht), mbst(mbst), mdct(mdct), mnsc(mnsc), idc()
{
  schedule(0);
}

traffic::~traffic()
{
  for(auto c: cs)
    delete c;
}

int
traffic::nr_clients() const
{
  return cs.size();
}

void
traffic::operator()(double t)
{
  client *c = new client(mht, mbst, mdct, mnsc, *this);
  cs.insert(c);
}

void
traffic::schedule_next(double t)
{
  double dt = catg();
  schedule(t + dt);
}

void
traffic::erase(client *c)
{
  cs.erase(c);
}
