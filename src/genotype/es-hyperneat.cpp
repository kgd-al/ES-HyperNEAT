#include "genotype.h"

namespace genotype {

CPPN::CPPN(void) {
  data.inputs = data.outputs = 0;
}

std::ostream& operator<< (std::ostream &os, const CPPN::Data &d) {
  os << "CPPN " << d.inputs << ":" << d.nodes.size() << ":" << d.outputs << " ("
     << d.links.size() << ")\n";
  for (const CPPN::Node &n: d.nodes)
    os << "\t" << n.id << " " << n.func << "\n";
  for (const CPPN::Link &l: d.links)
    os << "\t" << l.nid_src << " -> " << l.nid_dst << " (" << l.weight << ")\n";
  return os;
}

void to_json (nlohmann::json &j, const CPPN::Data &d) {
  nlohmann::json jnodes, jlinks;
  for (const CPPN::Node &n: d.nodes) jnodes.push_back({n.id, n.func});
  for (const CPPN::Link &l: d.links)
    jlinks.push_back({ l.id, l.nid_src, l.nid_dst, l.weight });
  j = { d.inputs, d.outputs, d.nextNID, d.nextLID, jnodes, jlinks };
}

void from_json (const nlohmann::json &j, CPPN::Data &d) {
  uint i=0;
  d.inputs = j[i++];
  d.outputs = j[i++];
  d.nextNID = j[i++];
  d.nextLID = j[i++];
  for (const nlohmann::json &jn: j[i++])  d.nodes.emplace(jn[0], jn[1]);
  for (const nlohmann::json &jl: j[i++])
    d.links.emplace(jl[0], jl[1], jl[2], jl[3]);
}

bool operator== (const CPPN::Data &lhs, const CPPN::Data &rhs) {
  if (lhs.inputs != rhs.inputs) return false;
  if (lhs.outputs != rhs.outputs) return false;
  if (lhs.nextNID != rhs.nextNID) return false;
  if (lhs.nextLID != rhs.nextLID) return false;
  if (lhs.nodes.size() != rhs.nodes.size()) return false;
  for (auto itL = lhs.nodes.begin(), itR = rhs.nodes.begin();
       itL != lhs.nodes.end(); ++itL, ++itR) {
    const CPPN::Node &nL = *itL, nR = *itR;
    if (nL.id != nR.id) return false;
    if (nL.func != nR.func) return false;
  }
  if (lhs.links.size() != rhs.links.size()) return false;
  for (auto itL = lhs.links.begin(), itR = rhs.links.begin();
       itL != lhs.links.end(); ++itL, ++itR) {
    const CPPN::Link &lL = *itL, lR = *itR;
    if (lL.id != lR.id) return false;
    if (lL.nid_src != lR.nid_src) return false;
    if (lL.nid_dst != lR.nid_dst) return false;
    if (lL.weight != lR.weight) return false;
  }
  return true;
}

void assertEqual (const CPPN::Data &lhs, const CPPN::Data &rhs, bool deepcopy) {
  using utils::assertEqual;
  assertEqual(lhs.inputs, rhs.inputs, deepcopy);
  assertEqual(lhs.outputs, rhs.outputs, deepcopy);
  assertEqual(lhs.nextNID, rhs.nextNID, deepcopy);
  assertEqual(lhs.nextLID, rhs.nextLID, deepcopy);
  assertEqual(lhs.nodes.size(), rhs.nodes.size(), false);
  for (auto itL = lhs.nodes.begin(), itR = rhs.nodes.begin();
       itL != lhs.nodes.end(); ++itL, ++itR) {
    const CPPN::Node &nL = *itL, nR = *itR;
    assertEqual(nL.id, nR.id, deepcopy);
    assertEqual(nL.func, nR.func, deepcopy);
  }
  assertEqual(lhs.links.size(), rhs.links.size(), false);
  for (auto itL = lhs.links.begin(), itR = rhs.links.begin();
       itL != lhs.links.end(); ++itL, ++itR) {
    const CPPN::Link &lL = *itL, lR = *itR;
    assertEqual(lL.id, lR.id, deepcopy);
    assertEqual(lL.nid_src, lR.nid_src, deepcopy);
    assertEqual(lL.nid_dst, lR.nid_dst, deepcopy);
    assertEqual(lL.weight, lR.weight, deepcopy);
  }
}

} // end of namespace genotype

using namespace genotype;
#define GENOME CPPN
using Config = GENOME::config_t;
using NID = CPPN::Node::ID;
using LID = CPPN::Link::ID;

NID addNode (CPPN::Data &data, rng::AbstractDice &dice) {
  auto id = NID::next(data.nextNID);
  data.nodes.emplace(id, *dice(Config::functions()));
  return id;
}

LID addLink (CPPN::Data &data, NID src, NID dst, rng::AbstractDice &dice) {
  static const auto &wb = Config::weightBounds();
  auto id = LID::next(data.nextLID);
  data.links.emplace(id, src, dst, dice(wb.rndMin, wb.rndMax));
  return id;
}

auto initialCPPN (rng::AbstractDice &dice) {
  CPPN::Data d;
  d.inputs = Config::substrateDimension() * 2
           + Config::withBias();
  d.outputs = 1 + Config::withLEO();

  d.nextNID = NID(d.inputs+d.outputs);
  d.nextLID = LID(0);
  for (uint i=0; i<d.inputs; i++)
    for (uint j=0; j<d.outputs; j++)
      addLink(d, NID(i), NID(d.inputs+j), dice);
  return d;
}

void CPPN_add_node (CPPN::Data &d, rng::AbstractDice &dice) {
  const CPPN::Link &l = *dice(d.links);
  addNode(d, dice);
}

using MAddLCandidates = std::set<std::pair<NID,NID>>;
void CPPN_add_link (CPPN::Data &d, const MAddLCandidates &candidates,
                    rng::AbstractDice &dice) {
  auto p = *dice(candidates);
  addLink(d, p.first, p.second, dice);
}

using MDelNCandidates = std::set<NID>;
void CPPN_del_node (CPPN::Data &d, const MDelNCandidates &candidates,
                    rng::AbstractDice &dice) {
  static constexpr uint target = 100;
  NID nid = *dice(candidates);
  auto lit = d.links.begin();
  uint found = 0;
  while (found < target && lit != d.links.end()) {
    if (lit->nid_src == nid || lit->nid_dst == nid) {
      lit = d.links.erase(lit);
      found++;
    }
  }
  assert(found == 2);
  d.nodes.erase(d.nodes.find(nid));
}

void CPPN_del_link (CPPN::Data &d, rng::AbstractDice &dice) {
  d.links.erase(dice(d.links));
}

void CPPN_mutate_weight (CPPN::Data &d, rng::AbstractDice &dice) {
  auto node = d.links.extract(dice(d.links));
  const auto &b = Config::weightBounds();
  using MO = config::MutationSettings::BoundsOperators<float>;
  MO::mutate(node.value().weight, b.min, b.max, dice);
  d.links.insert(std::move(node));
}

void CPPN_mutate_function (CPPN::Data &d, rng::AbstractDice &dice) {
  auto node = d.nodes.extract(dice(d.nodes));
  CPPN::Node &n = node.value();
  auto f = n.func;
  while (f == n.func) n.func = *dice(Config::functions());
  d.nodes.insert(std::move(n));
}

void mutateCPPN (CPPN::Data &d, rng::AbstractDice &dice) {
  auto rates = Config::d_mutationRates();

  MAddLCandidates potentialLinks;
  for (const CPPN::Node &nl: d.nodes)
    for (const CPPN::Node &nr: d.nodes)
      if (nl.id != nr.id)
        potentialLinks.insert({nl.id, nr.id});

  std::map<NID, std::pair<uint,uint>> nodeDegrees;
  for (const CPPN::Link &l: d.links) {
    nodeDegrees[l.nid_src].second++;
    nodeDegrees[l.nid_dst].first++;

    potentialLinks.erase({l.nid_src, l.nid_dst});
  }

  MDelNCandidates simpleNodes;
  for (const auto &p: nodeDegrees)
    if (p.second.first == 1 && p.second.second == 1)
      simpleNodes.insert(p.first);

  rates["add_l"] *= (potentialLinks.size() > 0);
  rates["del_l"] *= (d.links.size() > 0);
  rates["del_n"] *= (simpleNodes.size() > 0);
  auto type = dice.pickOne(rates);
  if (type == "add_l")
    CPPN_add_link(d, potentialLinks, dice);
  else if (type == "del_n")
    CPPN_del_node(d, simpleNodes, dice);
  else {
    static const std::map<std::string,
                          void(*)(CPPN::Data&, rng::AbstractDice&)> mutators {
      { "add_n", &CPPN_add_node },
      { "del_l", &CPPN_del_link },
      { "mut_w", &CPPN_mutate_weight },
      { "mut_f", &CPPN_mutate_function },
    };
    mutators.at(type)(d, dice);
  }
}

DEFINE_GENOME_FIELD_WITH_FUNCTOR(CPPN::Data, data, "", [] {
  GENOME_FIELD_FUNCTOR(CPPN::Data, data) functor;

  functor.random = &initialCPPN;
  functor.mutate = &mutateCPPN;
  functor.cross = [] (auto &lhs, auto &rhs, auto &dice) {
    std::cerr << "CPPN not (yet) crossable" << std::endl;
    return dice.toss(lhs, rhs);
  };

  functor.distance = [] (auto &lhs, auto &rhs) {
    std::cerr << "CPPN not (yet) distance-aware" << std::endl;
    return 0;
  };

  functor.check = [] (auto &cppn) {
    std::cerr << "CPPN not (yet) checkable" << std::endl;
    return true;
  };

  return functor;
}())

template <>
struct genotype::MutationRatesPrinter<CPPN::Data, CPPN, &CPPN::data> {
  static constexpr bool recursive = true;
  static void print (std::ostream &os, uint width, uint depth, float ratio) {
    for (const auto &p: Config::d_mutationRates())
      prettyPrintMutationRate(os, width, depth, ratio, p.second, p.first,
                              false);
  }
};

template <>
struct genotype::Extractor<CPPN::Data> {
  std::string operator() (const CPPN::Data &d, const std::string &field) {
    return "Not implemented";
  }
};

template <>
struct genotype::Aggregator<CPPN::Data, CPPN> {
  void operator() (std::ostream &os, const std::vector<CPPN> &genomes,
                   std::function<const CPPN::Data& (const CPPN&)> access,
                   uint /*verbosity*/) {}
};

DEFINE_GENOME_MUTATION_RATES({ EDNA_PAIR(data, 1) })
DEFINE_GENOME_DISTANCE_WEIGHTS({ EDNA_PAIR(data, 1) })

#undef GENOME

#define CFILE genotype::CPPN::config_t

DEFINE_CONTAINER_PARAMETER(CFILE::FunctionSet, functions, {
  "gaus", "sin", "abs"
})

DEFINE_PARAMETER(CFILE::Bounds<float>, weightBounds, -3.f, -1.f, 1.f, 3.f)

DEFINE_PARAMETER(int, substrateDimension, 2)
DEFINE_PARAMETER(bool, withBias, true)
DEFINE_PARAMETER(bool, withLEO, true)

DEFINE_CONTAINER_PARAMETER(CFILE::MutationRates, d_mutationRates,
                           utils::normalizeRates({
  { "add_n",  .5f   },
  { "add_l",  .5f   },
  { "del_n",  .25f  },
  { "del_l",  .25f  },
  { "mut_w", 6.f    },
  { "mut_f", 2.5f   },
}))

#undef CFILE
