#include "es-hyperneat.h"

using CPPN = genotype::ES_HyperNEAT::CPPN;

namespace genotype {

auto randomNodeFunction (rng::AbstractDice &dice) {
  return *dice(genotype::ES_HyperNEAT::config_t::functions());
}

auto randomLinkWeight (rng::AbstractDice &dice) {
  static const auto &wb = genotype::ES_HyperNEAT::config_t::weightBounds();
  return dice(wb.rndMin, wb.rndMax);
}

#ifdef WITH_GVC
static constexpr std::array<const char*, CPPN::INPUTS> ilabels = {{
  "x_0", "y_0",
#if ESHN_SUBSTRATE_DIMENSION == 3
  "z_0",
#endif
  "x_1", "y_1",
#if ESHN_SUBSTRATE_DIMENSION == 3
  "z_1",
#endif
#if CPPN_WITH_DISTANCE
  "l",
#endif
  "b"
}};

static constexpr std::array<const char*, CPPN::OUTPUTS> olabels = {{
  "w", "l",
#if ANN_TYPE == Float
  "b"
#endif
}};

gvc::GraphWrapper CPPN::build_gvc_graph(void) const {
  using namespace gvc;

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
  for (uint i=0; i<CPPN::INPUTS; i++) {
    auto n = ag_nodes[NID(i)] = add_node(g_i, "I", i);
    set_html(g.graph, n, "label", ilabels[i]);
    set(n, "shape", "plain");
  }

  // output nodes (and their labels)
  for (uint i=0; i<CPPN::OUTPUTS; i++) {
    auto nid = NID(i+CPPN::INPUTS);
    auto n = ag_nodes[nid] = add_node(g_o, "O", nid);

    const auto &f = config_t::cppnOutputFuncs()[i];
    set(n, "kgdfunc", f);
    set(n, "image", "ps/", f, ".png");

    set(n, "width", "0.5in");
    set(n, "height", "0.5in");
    set(n, "margin", "0");
    set(n, "fixedsize", "shape");
    set(n, "label", olabels[i]);
  }

  // internal nodes
  for (const Node &n: nodes) {
    auto gn = ag_nodes[n.id] = add_node(g_h, "H", n.id);
//    set(gn, "label", "");
    set(gn, "kgdfunc", n.func);
    set(gn, "image", "ps/", n.func, ".png");
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
  for (uint i=0; i<CPPN::INPUTS-1; i++) {
    auto e = g.add_edge(ag_nodes.at(NID(i)), ag_nodes.at(NID(i+1)), "EIO", i);
    set(e, "style", "invis");
  }
  for (uint i=0; i<CPPN::OUTPUTS-1; i++) {
    auto e = g.add_edge(ag_nodes.at(NID(i+CPPN::INPUTS)),
                        ag_nodes.at(NID(i+1+CPPN::INPUTS)),
                        "EIO", i);
    set(e, "style", "invis");
  }

  return g;
}

void CPPN::render_gvc_graph(const std::string &path) const {
  auto ext_i = path.find_last_of('.')+1;
  const char *ext = path.data()+ext_i;

  gvc::GraphWrapper g = build_gvc_graph();

  g.layout("dot");
  gvRenderFilename(gvc::context(), g.graph, ext, path.c_str());
//  gvRender(gvc::context(), g.graph, "dot", stdout);

  auto dpath = path;
  dpath.replace(ext_i, 3, "dot");
  gvRenderFilename(gvc::context(), g.graph, "dot", dpath.c_str());
}

#endif

template <typename T>
void parseOrExcept (const std::string &str, T &field,
                    uint row, const std::string &line) {
  std::istringstream iss (str);
  iss >> field;
  if (!iss)
    utils::Thrower("Could not parse '", str, "' as a value of type ",
                    utils::className<T>(), " in ", line, " (row ", row, ")");
}

CPPN CPPN::fromDot(const std::string &data, rng::AbstractDice &dice) {
  using NID = CPPN::Node::ID;
  using LID = CPPN::Link::ID;
  static const std::regex header(".*CPPN\\(([0-9]+),([0-9]+)\\).*");

  static const std::regex node(
    " *([0-9]+) *(\\[([a-z]*)\\])?;.*"
  );

  static const std::regex link(
    " *([0-9]+) *-> *([0-9]+) *(\\[([+-]?[0-9]*.?[0-9]*)\\])?;.*");

  static const auto &functions = config_t::functions();

  CPPN cppn;
  std::istringstream iss (data);
  std::string line;
  std::smatch matches;
  uint row = 0;
  const auto parseOrExceptL = [&row, &line] (const std::string &s, auto &v) {
    parseOrExcept(s, v, row, line);
  };

  while (std::getline(iss, line)) {
//    std::cout << "line: " << line << "\n";

    if (std::regex_match(line, matches, header)/* && matches.size() == 3*/) {
      uint inputs, outputs;
      parseOrExceptL(matches[1], inputs);
      parseOrExceptL(matches[2], outputs);
      cppn.nextLID = LID(0);

//      std::cout << "\tCPPN: i=" << cppn.inputs << ", o=" << cppn.outputs
//                << "\n";

      if (inputs != CPPN::INPUTS)
        utils::Thrower("Header declares ", inputs, " inputs while ",
                        CPPN::INPUTS, " were expected");

      if (outputs != CPPN::OUTPUTS)
        utils::Thrower("Header declares ", outputs, " outputs while ",
                        CPPN::OUTPUTS, " were expected");

    } else if (std::regex_match(line, matches, node) && matches.size() > 1) {
      bool funcProvided = (matches.size() == 4 && matches[3].length() > 0);
      uint id;
      parseOrExceptL(matches[1], id);
      id--; // NID increases id by 1, so decrement first

      if (!cppn.isOutput(id)) {

        CPPN::Node::FuncID func;
        if (funcProvided) {
          func = CPPN::Node::FuncID(matches[3]);

          if (functions.find(func) == functions.end())
            utils::Thrower("Function ", func,
                           " is not a member of the current set: ", functions);

        } else
          func = randomNodeFunction(dice);

        cppn.nodes.emplace(NID(id), func);
//        std::cout << "\tnode" << NID(id) << ": " << func << "\n";

      } else if (funcProvided)
          std::cout << "Ignoring provided function '" << matches[3]
                      << "' for output node " << id+1 << "\n";

    } else if (std::regex_match(line, matches, link) && matches.size() > 2) {
      bool weightProvided = (matches.size() == 5 && matches[4].length() > 0);

      uint lid, rid;
      float w;
      parseOrExceptL(matches[1], lid);
      parseOrExceptL(matches[2], rid);

      if (weightProvided)
        parseOrExceptL(matches[4], w);
      else
        w = randomLinkWeight(dice);

      cppn.links.emplace(CPPN::Link::ID::next(cppn.nextLID),
                         NID(lid-1), NID(rid-1), w);

//      std::cout << "\tlink: " << NID(lid-1) << " to " << NID(rid-1)
//                << " w=" << w << "\n";

    }/* else
      std::cout << "unmatched line '" << line << "'\n";*/

    row++;
  }

//  std::cout << std::endl;
  cppn.nextNID = NID(CPPN::INPUTS+CPPN::OUTPUTS+cppn.nodes.size());

//  std::cout << cppn << std::endl;

  return cppn;
}

ES_HyperNEAT ES_HyperNEAT::fromGenomeFile(const std::string &path) {
  return genotype::EDNA<ES_HyperNEAT>::fromFile(path);
}

ES_HyperNEAT ES_HyperNEAT::fromDotFile(const std::string &path,
                                       rng::AbstractDice &dice) {

  std::string data = utils::readAll(path);
  ES_HyperNEAT genome = random(dice);
  genome.cppn = CPPN::fromDot(data, dice);
  return genome;
}

std::ostream& operator<< (std::ostream &os, const CPPN &d) {
  os << "CPPN " << CPPN::INPUTS << ":" << d.nodes.size() << ":"
     << CPPN::OUTPUTS << " (" << d.links.size() << ")\n";
  os << "  nextNID: " << d.nextNID << "\n";
  os << "  nextLID: " << d.nextLID << "\n";
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

  std::array<CPPN::Node::FuncID, CPPN::OUTPUTS> outputFunctions;
  for (uint i=0; i<outputFunctions.size(); i++)
    outputFunctions[i] = ES_HyperNEAT::config_t::cppnOutputFuncs()[i];

  j = {
    CPPN::INPUTS, CPPN::OUTPUTS,
    d.nextNID, d.nextLID,
    jnodes, jlinks, outputFunctions
  };
}

void from_json (const nlohmann::json &j, CPPN &d) {
  uint i=0;
  uint inputs = j[i++];
  uint outputs = j[i++];
  d.nextNID = j[i++];
  d.nextLID = j[i++];
  for (const nlohmann::json &jn: j[i++])  d.nodes.emplace(jn[0], jn[1]);
  for (const nlohmann::json &jl: j[i++])
    d.links.emplace(jl[0], jl[1], jl[2], jl[3]);

  static const auto &outputFuncs = ES_HyperNEAT::config_t::cppnOutputFuncs();
  std::array<CPPN::Node::FuncID, CPPN::OUTPUTS> outputFunctions = j[i++];

  if (inputs != CPPN::INPUTS)
    utils::Thrower("Parsed cppn has wrong input size! Expected ",
                   CPPN::INPUTS, " got ", inputs);
  if (outputs != CPPN::OUTPUTS)
    utils::Thrower("Parsed cppn has wrong output size! Expected ",
                   CPPN::OUTPUTS, " got ", outputs);
  for (uint i=0; i<CPPN::OUTPUTS; i++)
    if (outputFunctions[i] != outputFuncs[i])
      utils::Thrower("Wrong output function for output ", i, "! Expected ",
                      outputFuncs[i], " got ", outputFunctions[i]);
}

bool operator== (const CPPN &lhs, const CPPN &rhs) {
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

void assertEqual (const CPPN &lhs, const CPPN &rhs,
                  bool deepcopy) {
  using utils::assertEqual;
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
#define GENOME ES_HyperNEAT
using Config = GENOME::config_t;
using NID = CPPN::Node::ID;
using LID = CPPN::Link::ID;

static constexpr NID::ut HID_OFFSET = CPPN::INPUTS + CPPN::OUTPUTS;

bool CPPN::isInput(NID::ut id) {  return id < CPPN::INPUTS; }

bool CPPN::isOutput(NID::ut id) {
  return CPPN::INPUTS <= id && id < HID_OFFSET;
}

bool CPPN::isHidden(NID::ut id) { return HID_OFFSET <= id;  }

NID::ut nid_ut (NID nid) {
  assert(NID::ut(nid.id) > 0);
  return NID::ut(nid.id)-1;
}

bool CPPN::isInput(NID nid) {   return isInput(nid_ut(nid));  }
bool CPPN::isOutput(NID nid) {  return isOutput(nid_ut(nid)); }
bool CPPN::isHidden(NID nid) {  return isHidden(nid_ut(nid)); }

NID addNode (CPPN &data, rng::AbstractDice &dice) {
  auto id = NID::next(data.nextNID);
  data.nodes.emplace(id, randomNodeFunction(dice));
  return id;
}

LID addLink (CPPN &data, NID src, NID dst, float weight) {
  auto id = LID::next(data.nextLID);
  data.links.emplace(id, src, dst, weight);
  return id;
}

LID addLink (CPPN &data, NID src, NID dst, rng::AbstractDice &dice) {
  return addLink(data, src, dst, randomLinkWeight(dice));
}

auto initialCPPN (rng::AbstractDice &dice) {
  CPPN d;
  d.nextNID = NID(HID_OFFSET);
  d.nextLID = LID(0);
  for (uint i=0; i<CPPN::INPUTS; i++) {
    for (uint j=0; j<CPPN::OUTPUTS; j++) {
      if (Config::cppnInit() == config::CPPNInitMethod::BIMODAL
          && dice(.5))  continue;
      addLink(d, NID(i), NID(CPPN::INPUTS+j), dice);

//      assert(CPPN::isInput(i));
      assert(CPPN::isInput(NID(i)));
//      assert(CPPN::isOutput(CPPN::INPUTS+j));
      assert(CPPN::isOutput(NID(CPPN::INPUTS+j)));
    }
  }

//  d.outputFunctions = Config::outputFunctions();
  return d;
}


void CPPN_add_node (CPPN &d, rng::AbstractDice &dice, bool log) {
  auto it = dice(d.links);
  const CPPN::Link &l = *it;
  NID nid = addNode(d, dice);
  addLink(d, l.nid_src, nid, 1);
  addLink(d, nid, l.nid_dst, l.weight);
  if (log)
    std::cerr << " created node " << nid << " in " << l.nid_src << " -> "
              << l.nid_dst;
  d.links.erase(it);
}

using MAddLCandidates = std::set<std::pair<NID,NID>>;
void CPPN_add_link (CPPN &d, const MAddLCandidates &candidates,
                    rng::AbstractDice &dice, bool log) {
  auto p = *dice(candidates);
  auto lid = addLink(d, p.first, p.second, dice);

  if (log)
    std::cerr << " added " << p.first << " -(" << d.links.find(lid)->weight
              << ")-> " << p.second;
}

using MDelNCandidates = std::set<NID>;
void CPPN_del_node (CPPN &d, const MDelNCandidates &candidates,
                    rng::AbstractDice &dice, bool log) {
  static constexpr uint target = 100;
  NID nid = *dice(candidates);
  auto lit = d.links.begin();
  uint found = 0;
  NID src, dst;
  float weight = 0;
  while (found < target && lit != d.links.end()) {
    bool inLink = (lit->nid_dst == nid), outLink = (lit->nid_src == nid);
    if (inLink)   src = lit->nid_src;
    if (outLink)  dst = lit->nid_dst, weight = lit->weight;
    if (inLink || outLink) {
      lit = d.links.erase(lit);
      found++;
    } else
      ++lit;
  }
  assert(found == 2);

  if (log)
    std::cerr << " removed node " << nid;

  d.nodes.erase(d.nodes.find(nid));
  addLink(d, src, dst, weight);
}

using MDelLCandidates = std::set<LID>;
void CPPN_del_link (CPPN &d, const MDelLCandidates &candidates,
                    rng::AbstractDice &dice, bool log) {
  auto it = dice(candidates);
  auto lit = d.links.find(*it);
  if (log)
    std::cerr << " removed link " << lit->nid_src << " -> " << lit->nid_dst;

  d.links.erase(d.links.find(*it));
}

void CPPN_mutate_weight (CPPN &d, rng::AbstractDice &dice, bool log) {
  auto node = d.links.extract(dice(d.links));
  const auto &b = Config::weightBounds();
  using MO = config::MutationSettings::BoundsOperators<float>;
  if (log)
    std::cerr << " mutated weight in " << node.value().nid_src << " -> "
              << node.value().nid_dst << " from " << node.value().weight;

  MO::mutate(node.value().weight, b.min, b.max, b.stddev, dice);
  if (log)
    std::cerr << " to " << node.value().weight;

  d.links.insert(std::move(node));
}

void CPPN_mutate_function (CPPN &d, rng::AbstractDice &dice, bool log) {
  auto node = d.nodes.extract(dice(d.nodes));
  CPPN::Node &n = node.value();
  auto f = n.func;
  if (log)
    std::cerr << " mutated node " << n.id << " function from " << f;
  while (f == n.func) n.func = randomNodeFunction(dice);
  if (log)
    std::cerr << " to " << n.func;
  d.nodes.insert(std::move(n));
}

void mutateCPPN (CPPN &d, rng::AbstractDice &dice) {
  auto rates = Config::cppn_mutationRates();
  bool log = config::EDNAConfigFile_common::autologMutations();

  struct N { NID id; enum T { I, O, H }; T type; };
  std::vector<N> allNodes;
  for (uint i=0; i<CPPN::INPUTS; i++) allNodes.push_back({NID(i), N::I});
  for (uint i=0; i<CPPN::OUTPUTS; i++)
    allNodes.push_back({NID(i+CPPN::INPUTS), N::O});
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

      // keep recurrent connections

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
    CPPN_add_link(d, potentialLinks, dice, log);
  else if (type == "del_n")
    CPPN_del_node(d, simpleNodes, dice, log);
  else if (type == "del_l")
    CPPN_del_link(d, nonEssentialLinks, dice, log);
  else {
    static const std::map<std::string,
                          void(*)(CPPN&, rng::AbstractDice&, bool)>
        mutators {
        { "add_n", &CPPN_add_node },
        { "mut_w", &CPPN_mutate_weight },
        { "mut_f", &CPPN_mutate_function },
    };
    mutators.at(type)(d, dice, log);
  }
}

DEFINE_GENOME_FIELD_WITH_FUNCTOR(CPPN, cppn, "", [] {
  GENOME_FIELD_FUNCTOR(CPPN, cppn) functor;

  functor.random = &initialCPPN;

  functor.mutate = &mutateCPPN;
  (void)log;  // Retrieved from inside the aforementioned function

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
//    std::cerr << "CPPN not (yet) checkable" << std::endl;
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

//DEFINE_PARAMETER(int, substrateDimension, 2)
//DEFINE_PARAMETER(bool, withBias, true)
//DEFINE_PARAMETER(bool, withLEO, true)
//DEFINE_CONTAINER_PARAMETER(CFILE::OFunctions, outputFunctions, {
//                            "bsgm", "step" })

DEFINE_CONST_PARAMETER(CFILE::OutputFuncs, cppnOutputFuncs, CFILE::OutputFuncs{{
  "bsgm", "step", "id"
}})

DEFINE_PARAMETER(config::CPPNInitMethod, cppnInit,
                 config::CPPNInitMethod::BIMODAL)

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
