#include "es-hyperneat.h"

using CPPN = genotype::ES_HyperNEAT::CPPN;

namespace genotype {

#ifdef WITH_GVC
gvc::GraphWrapper CPPN::build_gvc_graph(const char *ext) const {
  using namespace gvc;

  static constexpr auto ilabel = [] (uint index, uint total) {
    if (index == total-1) return std::string("B");
    else {
      static constexpr std::array<char, 3> coords { 'x', 'y', 'z' };
      std::ostringstream oss;
      auto m = (total-1)/2;
      oss << coords.at(index%m) << "<sub>" << index/m << "</sub>";
      return oss.str();
    }
  };
  static constexpr std::array<char, 2> olabels { 'w', 'l' };

  static const auto img_exts = [] (const std::string &e) {
    static const std::map<std::string, std::string> m {
      { "png", ".png" },
      {  "ps", ".eps" }
    };
    auto it = m.find(e);
    if (it != m.end())  return it->second;
    return std::string(".png");
  };

  const auto img_ext = img_exts(ext);

  using NID = Node::ID;
  GraphWrapper g ("cppn");
  Agraph_t *g_i = g.add_graph("i"),
           *g_h = g.add_graph("h"),
           *g_o = g.add_graph("o");

  set(g.graph, "rankdir", "BT");
  set(g_i, "rank", "source");
  set(g_o, "rank", "same");
  std::map<NID, Agnode_t*> ag_nodes;

  // input nodes
  for (uint i=0; i<inputs; i++) {
    auto n = ag_nodes[NID(i)] = add_node(g_i, "I", i);
    set_html(g.graph, n, "label", ilabel(i, inputs));
    set(n, "shape", "plain");
  }

  // output nodes (and their labels)
  for (uint i=0; i<outputs; i++) {
    auto nid = NID(i+inputs);
    auto n = ag_nodes[nid] = add_node(g_o, "O", nid);
    set(n, "label", "");
    set(n, "kgdfunc", outputFunctions.at(i));
    set(n, "image", "ps/", outputFunctions.at(i), img_ext);
    set(n, "width", "0.5in");
    set(n, "height", "0.5in");
    set(n, "margin", "0");
    set(n, "fixedsize", "shape");
    set(n, "label", olabels.at(i));
  }

  // internal nodes
  for (const Node &n: nodes) {
    auto gn = ag_nodes[n.id] = add_node(g_h, "H", n.id);
    set(gn, "label", "");
    set(gn, "kgdfunc", n.func);
    set(gn, "image", "ps/", n.func, img_ext);
    set(gn, "width", "0.5in");
    set(gn, "height", "0.5in");
    set(gn, "margin", "0");
    set(gn, "fixedsize", "shape");
  }

  // cppn edges
  for (const Link &l: links) {
    if (l.weight == 0) continue;
    auto e = g.add_edge(ag_nodes.at(l.nid_src), ag_nodes.at(l.nid_dst),
                        "E", l.id);
    set(e, "color", l.weight < 0 ? "red" : "black");
    auto w = std::fabs(l.weight) / config_t::weightBounds().max;
    set(e, "penwidth", 3.75*w+.25);
//    set(e, "weight", w);
//    set(e, "label", w);
  }

  // enforce i/o order
  for (uint i=0; i<inputs-1; i++) {
    auto e = g.add_edge(ag_nodes.at(NID(i)), ag_nodes.at(NID(i+1)), "EIO", i);
    set(e, "style", "invis");
  }
  for (uint i=0; i<outputs-1; i++) {
    auto e = g.add_edge(ag_nodes.at(NID(i+inputs)),
                        ag_nodes.at(NID(i+1+inputs)),
                        "EIO", i);
    set(e, "style", "invis");
  }

  return g;
}

void CPPN::render_gvc_graph(const std::string &path) const {
  auto ext_i = path.find_last_of('.')+1;
  const char *ext = path.data()+ext_i;

  gvc::GraphWrapper g = build_gvc_graph(ext);

  g.layout("dot");
  gvRenderFilename(gvc::context(), g.graph, ext, path.c_str());
//  gvRender(gvc::context(), g.graph, "dot", stdout);

  auto dpath = path;
  dpath.replace(ext_i, 3, "dot");
  gvRenderFilename(gvc::context(), g.graph, "dot", dpath.c_str());
}

#endif

CPPN CPPN::fromDot(const std::string &data) {
  using NID = CPPN::Node::ID;
  using LID = CPPN::Link::ID;
  static const std::regex header(".*CPPN\\(([0-9]+),([0-9]+)\\).*");
  static const std::regex node(".*([0-9]+) *\\[([a-z]+)\\].*");
  static const std::regex link(".*([0-9]+) -> ([0-9]+) *\\[([+-]?[0-9]*.?[0-9]*)\\].*");
  CPPN cppn;
  std::istringstream iss (data);
  std::string line;
  std::smatch matches;
  while (std::getline(iss, line)) {
//    std::cout << "line: " << line << "\n";
    if (std::regex_match(line, matches, header)/* && matches.size() == 3*/) {
      std::istringstream (matches[1]) >> cppn.inputs;
      std::istringstream (matches[2]) >> cppn.outputs;
//      std::cout << "\tCPPN: i=" << cppn.inputs << ", o=" << cppn.outputs << "\n";
      cppn.nextNID = NID(cppn.inputs+cppn.outputs);
      cppn.nextLID = LID(0);

    } else if (std::regex_match(line, matches, node) && matches.size() > 1) {
      uint id;
      std::istringstream (matches[1]) >> id;
      id--; // NID increases id by 1, so decrement first
      auto func = CPPN::Node::FuncID(matches[2]);
      if (cppn.isOutput(id)) {
        id -= cppn.inputs;
        cppn.outputFunctions[id] = func;
      } else
        cppn.nodes.emplace(NID(id), func);
//      std::cout << "\tnode: " << matches[1] << ", f=" << matches[2] << "\n";

    } else if (std::regex_match(line, matches, link) && matches.size() > 2) {
      uint lid, rid;
      float w;
      std::istringstream (matches[1]) >> lid;
      std::istringstream (matches[2]) >> rid;
      std::istringstream (matches[3]) >> w;
      cppn.links.emplace(CPPN::Link::ID::next(cppn.nextLID),
                         NID(lid-1), NID(rid-1), w);
//      std::cout << "\tlink: " << matches[1] << " to " << matches[2]
//                << " w=" << matches[3] << "\n";
    }/* else
      std::cout << "unmatched line '" << line << "'\n";*/
  }
//  std::cout << std::endl;
  return cppn;
}

ES_HyperNEAT ES_HyperNEAT::fromGenomeFile(const std::string &path) {
  return genotype::EDNA<ES_HyperNEAT>::fromFile(path);
}

ES_HyperNEAT ES_HyperNEAT::fromDotFile(const std::string &path,
                                       rng::AbstractDice &dice) {

  std::string data = utils::readAll(path);
  ES_HyperNEAT genome = random(dice);
  genome.cppn = CPPN::fromDot(data);
  return genome;
}

std::ostream& operator<< (std::ostream &os, const CPPN &d) {
  os << "CPPN " << d.inputs << ":" << d.nodes.size() << ":" << d.outputs << " ("
     << d.links.size() << ")\n";
  for (const CPPN::Node &n: d.nodes)
    os << "\t" << n.id << " " << n.func << "\n";
  for (const CPPN::Link &l: d.links)
    os << "\t" << l.nid_src << " -> " << l.nid_dst << " (" << l.weight << ")\n";
  return os;
}

void to_json (nlohmann::json &j, const CPPN &d) {
  nlohmann::json jnodes, jlinks;
  for (const CPPN::Node &n: d.nodes)
    jnodes.push_back({n.id, n.func});
  for (const CPPN::Link &l: d.links)
    jlinks.push_back({ l.id, l.nid_src, l.nid_dst, l.weight });
  j = {
    d.inputs, d.outputs, d.nextNID, d.nextLID, jnodes, jlinks, d.outputFunctions
  };
}

void from_json (const nlohmann::json &j, CPPN &d) {
  uint i=0;
  d.inputs = j[i++];
  d.outputs = j[i++];
  d.nextNID = j[i++];
  d.nextLID = j[i++];
  for (const nlohmann::json &jn: j[i++])  d.nodes.emplace(jn[0], jn[1]);
  for (const nlohmann::json &jl: j[i++])
    d.links.emplace(jl[0], jl[1], jl[2], jl[3]);
  d.outputFunctions = j[i++];
}

bool operator== (const CPPN &lhs, const CPPN &rhs) {
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
  if (lhs.outputFunctions != rhs.outputFunctions) return false;
  return true;
}

void assertEqual (const CPPN &lhs, const CPPN &rhs,
                  bool deepcopy) {
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
  assertEqual(lhs.outputFunctions, rhs.outputFunctions, deepcopy);
}

} // end of namespace genotype

using namespace genotype;
#define GENOME ES_HyperNEAT
using Config = GENOME::config_t;
using NID = CPPN::Node::ID;
using LID = CPPN::Link::ID;

bool CPPN::isInput(NID::ut id) const {  return id < inputs; }

bool CPPN::isOutput(NID::ut id) const {
  return inputs <= id && id < inputs+outputs;
}

bool CPPN::isHidden(NID::ut id) const { return inputs+outputs <= id;  }

bool nid_ut (NID nid) {
  assert(NID::ut(nid.id) > 0);
  return NID::ut(nid.id)-1;
}

bool CPPN::isInput(NID nid) const {   return isInput(nid_ut(nid));  }
bool CPPN::isOutput(NID nid) const {  return isOutput(nid_ut(nid)); }
bool CPPN::isHidden(NID nid) const {  return isHidden(nid_ut(nid)); }

NID addNode (CPPN &data, rng::AbstractDice &dice) {
  auto id = NID::next(data.nextNID);
  data.nodes.emplace(id, *dice(Config::functions()));
  return id;
}

LID addLink (CPPN &data, NID src, NID dst, float weight) {
  auto id = LID::next(data.nextLID);
  data.links.emplace(id, src, dst, weight);
  return id;
}

LID addLink (CPPN &data, NID src, NID dst, rng::AbstractDice &dice) {
  static const auto &wb = Config::weightBounds();
  return addLink(data, src, dst, dice(wb.rndMin, wb.rndMax));
}

auto initialCPPN (rng::AbstractDice &dice) {
  CPPN d;
  d.inputs = Config::substrateDimension() * 2
           + Config::withBias();
  d.outputs = 1 + Config::withLEO();

  d.nextNID = NID(d.inputs+d.outputs);
  d.nextLID = LID(0);
  for (uint i=0; i<d.inputs; i++) {
    for (uint j=0; j<d.outputs; j++) {
      if (Config::cppnInit() == config::CPPNInitMethod::BIMODAL
          && dice(.5))  continue;
      addLink(d, NID(i), NID(d.inputs+j), dice);
    }
  }

  d.outputFunctions = Config::outputFunctions();
  return d;
}

void CPPN_add_node (CPPN &d, rng::AbstractDice &dice) {
  auto it = dice(d.links);
  const CPPN::Link &l = *it;
  NID nid = addNode(d, dice);
  addLink(d, l.nid_src, nid, 1);
  addLink(d, nid, l.nid_dst, l.weight);
//  std::cerr << "\n+ " << nid << " (" << d.nextNID << ")\n"
//            << "+ " << l.nid_src << " -> " << nid << "\n"
//            << "+ " << nid << " -> " << l.nid_dst << "\n"
//            << "- " << l.nid_src << " -> " << l.nid_dst << "\n";
  d.links.erase(it);
}

using MAddLCandidates = std::set<std::pair<NID,NID>>;
void CPPN_add_link (CPPN &d, const MAddLCandidates &candidates,
                    rng::AbstractDice &dice) {
  auto p = *dice(candidates);
  addLink(d, p.first, p.second, dice);
//  std::cerr << "\n+ " << p.first << " -> " << p.second << "\n";
}

using MDelNCandidates = std::set<NID>;
void CPPN_del_node (CPPN &d, const MDelNCandidates &candidates,
                    rng::AbstractDice &dice) {
  static constexpr uint target = 100;
  NID nid = *dice(candidates);
  auto lit = d.links.begin();
  uint found = 0;
  while (found < target && lit != d.links.end()) {
    if (lit->nid_src == nid || lit->nid_dst == nid) {
      lit = d.links.erase(lit);
      found++;
    } else
      ++lit;
  }
  assert(found == 2);
//  std::cerr << "\n- " << nid << "\n";
  d.nodes.erase(d.nodes.find(nid));
}

using MDelLCandidates = std::set<LID>;
void CPPN_del_link (CPPN &d, const MDelLCandidates &candidates,
                    rng::AbstractDice &dice) {
  auto it = dice(candidates);
//  std::cerr << "\n- " << it->nid_src << " -> " << it->nid_dst << "\n";
  d.links.erase(d.links.find(*it));
}

void CPPN_mutate_weight (CPPN &d, rng::AbstractDice &dice) {
  auto node = d.links.extract(dice(d.links));
  const auto &b = Config::weightBounds();
  using MO = config::MutationSettings::BoundsOperators<float>;
  MO::mutate(node.value().weight, b.min, b.max, dice);
  d.links.insert(std::move(node));
}

void CPPN_mutate_function (CPPN &d, rng::AbstractDice &dice) {
  auto node = d.nodes.extract(dice(d.nodes));
  CPPN::Node &n = node.value();
  auto f = n.func;
  while (f == n.func) n.func = *dice(Config::functions());
  d.nodes.insert(std::move(n));
}

void mutateCPPN (CPPN &d, rng::AbstractDice &dice) {
  auto rates = Config::cppn_mutationRates();

  struct N { NID id; enum T { I, O, H }; T type; };
  std::vector<N> allNodes;
  for (uint i=0; i<d.inputs; i++) allNodes.push_back({NID(i), N::I});
  for (uint i=0; i<d.outputs; i++)  allNodes.push_back({NID(i+d.inputs), N::O});
  for (const CPPN::Node &n: d.nodes)  allNodes.push_back({n.id, N::H});
  MAddLCandidates potentialLinks;
  for (const N &lhs: allNodes) {

    // no outputs ->
    if (lhs.type == N::O)  continue;

    for (const N &rhs: allNodes) {
      // no reflexive connections
      if (lhs.id == rhs.id) continue;

      // no -> inputs
      if (rhs.type == N::I) continue;

      // no recurrent connections

      potentialLinks.insert({lhs.id, rhs.id});
    }
  }

  struct Degree { uint in = 0, out = 0; };
  std::map<NID, Degree> nodeDegrees;
  for (const CPPN::Link &l: d.links) {
    nodeDegrees[l.nid_src].out++;
    nodeDegrees[l.nid_dst].in++;

    potentialLinks.erase({l.nid_src, l.nid_dst});
  }

  MDelLCandidates nonEssentialLinks;
  for (const CPPN::Link &l: d.links)
    if ((d.isInput(l.nid_src) && d.isOutput(l.nid_dst))
        || (nodeDegrees[l.nid_src].out > 1 && nodeDegrees[l.nid_dst].in > 1))
      nonEssentialLinks.insert(l.id);

  MDelNCandidates simpleNodes;
  for (const auto &p: nodeDegrees)
    if (p.second.in == 1 && p.second.out == 1)
      simpleNodes.insert(p.first);

  rates["add_l"] *= (potentialLinks.size() > 0);
  rates["del_l"] *= (nonEssentialLinks.size() > 0);
  rates["del_n"] *= (simpleNodes.size() > 0);
  rates["mut_f"] *= (d.nodes.size() > 0);
  auto type = dice.pickOne(rates);
  if (type == "add_l")
    CPPN_add_link(d, potentialLinks, dice);
  else if (type == "del_n")
    CPPN_del_node(d, simpleNodes, dice);
  else if (type == "del_l")
    CPPN_del_link(d, nonEssentialLinks, dice);
  else {
    static const std::map<std::string,
                          void(*)(CPPN&, rng::AbstractDice&)>
        mutators {
        { "add_n", &CPPN_add_node },
        { "mut_w", &CPPN_mutate_weight },
        { "mut_f", &CPPN_mutate_function },
    };
    mutators.at(type)(d, dice);
  }
}

DEFINE_GENOME_FIELD_WITH_FUNCTOR(CPPN, cppn, "", [] {
  GENOME_FIELD_FUNCTOR(CPPN, cppn) functor;

  functor.random = &initialCPPN;
  functor.mutate = &mutateCPPN;

  functor.cross = [] (auto &lhs, auto &rhs, auto &dice) {
    /// TODO Implement cppn crossing
    std::cerr << "CPPN not (yet) crossable" << std::endl;
    return dice.toss(lhs, rhs);
  };

  functor.distance = [] (auto &/*lhs*/, auto &/*rhs*/) {
    /// TODO Implement cppn genetic distance
    std::cerr << "CPPN not (yet) distance-aware" << std::endl;
    return 0;
  };

  functor.check = [] (auto &/*cppn*/) {
    /// TODO Confirm whether cppn requires a check
    std::cerr << "CPPN not (yet) checkable" << std::endl;
    return true;
  };

  return functor;
}())

template <>
struct genotype::MutationRatesPrinter<CPPN, GENOME, &GENOME::cppn> {
  static constexpr bool recursive = true;
  static void print (std::ostream &os, uint width, uint depth, float ratio) {
    for (const auto &p: Config::cppn_mutationRates())
      prettyPrintMutationRate(os, width, depth, ratio, p.second, p.first,
                              false);
  }
};

template <>
struct genotype::Extractor<CPPN> {
  std::string operator() (const CPPN &/*cppn*/, const std::string &/*field*/) {
    return "Not implemented";
  }
};

template <>
struct genotype::Aggregator<CPPN, GENOME> {
  void operator() (std::ostream &os, const std::vector<GENOME> &/*genomes*/,
                   std::function<const CPPN& (const GENOME&)> /*access*/,
                   uint /*verbosity*/) {
    os << "no aggregation (yet) for CPPNs\n";
  }
};

DEFINE_GENOME_FIELD_WITH_BOUNDS(uint, substeps, "n", 1u, 1u, 2u, 5u)

DEFINE_GENOME_MUTATION_RATES({
  EDNA_PAIR(       cppn, 95),
  EDNA_PAIR(   substeps, 5)
})
DEFINE_GENOME_DISTANCE_WEIGHTS({
  EDNA_PAIR(       cppn, 95),
  EDNA_PAIR(   substeps, 5)
})

#undef GENOME

#define CFILE genotype::ES_HyperNEAT::config_t

DEFINE_CONTAINER_PARAMETER(CFILE::FunctionSet, functions, {
//  "bsgm", "gaus", "line", "sin" // Risi function set
  "abs", "gaus", "id", "bsgm", "sin", "step"
})

DEFINE_PARAMETER(CFILE::Bounds<float>, weightBounds, -3.f, -1.f, 1.f, 3.f)

DEFINE_PARAMETER(int, substrateDimension, 2)
DEFINE_PARAMETER(bool, withBias, true)
DEFINE_PARAMETER(bool, withLEO, true)

DEFINE_PARAMETER(config::CPPNInitMethod, cppnInit,
                 config::CPPNInitMethod::BIMODAL)

DEFINE_CONTAINER_PARAMETER(CFILE::OFunctions, outputFunctions, {
                            "bsgm", "step" })

DEFINE_CONTAINER_PARAMETER(CFILE::MutationRates, cppn_mutationRates,
                           utils::normalizeRates({
  { "add_n",  .5f   },
  { "add_l",  .5f   },
  { "del_n",  .25f  },
  { "del_l",  .25f  },
  { "mut_w", 5.5f   },
  { "mut_f", 2.5f   },
}))

#undef CFILE
