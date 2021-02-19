#include <queue>

#include "ann.h"

/// TODO Find out which LEO implementation is best
/// TODO No direct input -> output connections (feature?)

#ifdef DEBUG_QUADTREE
//#define DEBUG_QUADTREE_DIVISION
//#define DEBUG_QUADTREE_PRUNING
namespace phenotype::evolvable_substrate { struct QuadTreeNode; }
namespace quadtree_debug {
void debugGenerateImages (const phenotype::evolvable_substrate::QuadTreeNode &t,
                          const phenotype::ANN::Point &p, bool in);
} // end of namespace quadtree_debug
#endif

namespace phenotype {

#ifndef NDEBUG
//#define DEBUG
//#define DEBUG_COMPUTE 1
//#define DEBUG_ES 3
#endif

#if DEBUG_ES
//#define DEBUG_ES_QUADTREE 1
#endif

namespace evolvable_substrate {

using Config = config::EvolvableSubstrate;

using Point = ANN::Point;
using Coordinates = ANN::Coordinates;

struct QuadTreeNode {
  Point center;
  float radius;
  uint level;
  float weight;

  using ptr = std::shared_ptr<QuadTreeNode>;
  std::vector<ptr> cs;

  QuadTreeNode (const Point &p, float r, uint l)
    : center(p), radius(r), level(l), weight(NAN) {}

  QuadTreeNode (float x, float y, float r, uint l)
    : QuadTreeNode({x,y}, r, l) {}

  float variance (void) const {
    if (cs.empty()) return 0;
    float mean = 0;
    for (auto &c: cs) mean += c->weight;
    mean /= cs.size();
    float var = 0;
    for (auto &c: cs) var += std::pow(c->weight - mean, 2);
    return var / cs.size();
  }

#if DEBUG_ES_QUADTREE
  friend std::ostream& operator<< (std::ostream &os, const QuadTreeNode &n) {
    utils::IndentingOStreambuf indent (os);
    os << "QTN " << n.center << " " << n.radius << " " << n.level << " "
              << n.weight << "\n";
    for (const auto &c: n.cs) os << *c;
    return os;
  }
#endif
};
using QuadTree = QuadTreeNode::ptr;

template <typename... ARGS>
QuadTreeNode::ptr node (ARGS... args) {
  return std::make_shared<QuadTreeNode>(args...);
}


QuadTree divisionAndInitialisation(const CPPN &cppn, const Point &p, bool out) {
  static const auto &initialDepth = Config::initialDepth();
  static const auto &maxDepth = Config::maxDepth();
  static const auto &divThr = Config::divThr();

  QuadTree root = node(0.f, 0.f, 1.f, 1);
  std::queue<QuadTreeNode*> q;
  q.push(root.get());

  const auto weight = [&cppn] (const Point &p0, const Point &p1) {
    auto cppn_inputs = cppn.inputs();
    cppn_inputs[0] = p0.x();
    cppn_inputs[1] = p0.y();
    cppn_inputs[2] = p1.x();
    cppn_inputs[3] = p1.y();
    return cppn(cppn_inputs, config::CPPNOutput::WEIGHT);
  };

#ifdef DEBUG_QUADTREE_DIVISION
  std::cout << "divisionAndInitialisation(" << p << ", " << out << ")\n";
#endif

  while (!q.empty()) {
    QuadTreeNode &n = *q.front();
    q.pop();

    float cx = n.center.x(), cy = n.center.y();
    float hr = .5 * n.radius;
    float nl = n.level + 1;
    n.cs.resize(4);
    n.cs[0] = node(cx - hr, cy - hr, hr, nl);
    n.cs[1] = node(cx - hr, cy + hr, hr, nl);
    n.cs[2] = node(cx + hr, cy - hr, hr, nl);
    n.cs[3] = node(cx + hr, cy + hr, hr, nl);

    for (auto &c: n.cs)
      c->weight = out ? weight(p, c->center) : weight(c->center, p);

#ifdef DEBUG_QUADTREE_DIVISION
    std::string indent (2*n.level, ' ');
    std::cout << indent << n.center << ", r=" << n.radius << ", l=" << n.level
              << ":";
    for (auto &c: n.cs) std::cout << " " << c->weight;
    std::cout << "\n" << indent << "> var = " << n.variance() << "\n";
#endif

    if (n.level < initialDepth || (n.level < maxDepth && n.variance() > divThr))
      for (auto &c: n.cs) q.push(c.get());
  }

#if DEBUG_ES_QUADTREE
  std::cerr << *root;
#endif

#ifdef DEBUG_QUADTREE
  quadtree_debug::debugGenerateImages(*root, p, !out);
#endif

  return root;
}

struct Connection {
  Point from, to;
  float weight;
#if DEBUG_ES
  friend std::ostream& operator<< (std::ostream &os, const Connection &c) {
    return os << "{ " << c.from << " -> " << c.to << " [" << c.weight << "]}";
  }
#endif
};
using Connections = std::vector<Connection>;
void pruneAndExtract (const CPPN &cppn, const Point &p, Connections &con,
                      const QuadTree &t, bool out) {

  static const auto &varThr = Config::varThr();
  static const auto &bndThr = Config::bndThr();
  static const auto leo = [] (const auto &cppn, auto i, auto o) {
    auto inputs = cppn.inputs();
    inputs[0] = i.x();
    inputs[1] = i.y();
    inputs[2] = o.x();
    inputs[3] = o.y();
    return (bool)cppn(inputs, config::CPPNOutput::LEO);
  };
  static const auto leoConnection = [] (const auto &cppn, auto i, auto o) {
    return leo(cppn, i, o);
  };

#ifdef DEBUG_QUADTREE_PRUNING
  if (t->level == 1)  std::cout << "\n---\n";
  utils::IndentingOStreambuf indent (std::cout);
  std::cout << "pruneAndExtract(" << p << ", " << t->center << ", "
            << t->radius << ", " << t->level << ", " << out << ") {\n";
#endif

  for (auto &c: t->cs) {
#ifdef DEBUG_QUADTREE_PRUNING
    utils::IndentingOStreambuf indent1 (std::cout);
    std::cout << "processing " << c->center << "\n";
    utils::IndentingOStreambuf indent2 (std::cout);
#endif

    if (c->variance() >= varThr) {
      // More information at lower resolution -> explore
#ifdef DEBUG_QUADTREE_PRUNING
      std::cout << "a> " << c->variance() << " >= " << varThr
                << " >> digging\n";
#endif
      pruneAndExtract(cppn, p, con, c, out);

    } else {
      // Not enough information at lower resolution -> test if part of band

      const auto weight = [&cppn, &p, &c, out] (float x, float y) {
        auto cppn_inputs = cppn.inputs();
        cppn_inputs[0] = out ? p.x() : x;
        cppn_inputs[1] = out ? p.y() : y;
        cppn_inputs[2] = out ? x : p.x();
        cppn_inputs[3] = out ? y : p.y();
        return std::fabs(c->weight
                         - cppn(cppn_inputs, config::CPPNOutput::WEIGHT));
      };
      float cx = c->center.x(), cy = c->center.y();
      float r = c->radius;
      float dl = weight(cx-r, cy), dr = weight(cx+r, cy),
            dt = weight(cx, cy-r), db = weight(cx, cy+r);

#ifdef DEBUG_QUADTREE_PRUNING
      std::cout << "b> var = " << c->variance() << ", bnd = "
                << std::max(std::min(dl, dr), std::min(dt, db))
                << " = max(min(" << dl << ", " << dr << "), min(" << dt
                << ", " << db << ")) && leo = "
                << leoConnection(cppn, out ? p : c->center, out ? c->center : p)
                << "\n";
#endif

      if (std::max(std::min(dl, dr), std::min(dt, db)) > bndThr
          && leoConnection(cppn, out ? p : c->center, out ? c->center : p)
          && c->weight != 0) {
        con.push_back({
          out ? p : c->center, out ? c->center : p, c->weight
        });
#ifdef DEBUG_QUADTREE_PRUNING
        std::cout << " < created " << (out ? p : c->center) << " -> "
                  << (out ? c->center : p) << " [" << c->weight << "]\n";
#endif
      }
    }
  }

#ifdef DEBUG_QUADTREE_PRUNING
  std::cout << "}\n";
#endif
}

void removeUnconnectedNeurons (const Coordinates &inputs,
                               const Coordinates &outputs,
                               std::set<Point> &shidden,
                               Connections &connections) {
  using Type = ANN::Neuron::Type;
  struct L;
  struct N {
    Point p;
    Type t;
    std::vector<L> i, o;
    N (const Point &p, Type t = Type::H) : p(p), t(t) {}
  };
  struct L {
    N *n;
    float w;
  };
  struct CMP {
    using is_transparent = void;
    bool operator() (const N *lhs, const Point &rhs) const {
      return lhs->p < rhs;
    }
    bool operator() (const Point &lhs, const N *rhs) const {
      return lhs < rhs->p;
    }
    bool operator() (const N *lhs, const N *rhs) const {
      return lhs->p < rhs->p;
    }
  };

  std::set<N*, CMP> nodes, inodes, onodes;
  for (const Point &p: inputs)
    nodes.insert(*inodes.insert(new N(p, Type::I)).first);
  for (const Point &p: outputs)
    nodes.insert(*onodes.insert(new N(p, Type::O)).first);

  const auto getOrCreate = [&nodes] (const Point &p) {
    auto it = nodes.find(p);
    if (it != nodes.end())  return *it;
    else                    return *nodes.insert(new N(p)).first;
  };
  for (Connection &c: connections) {
    N *i = getOrCreate(c.from), *o = getOrCreate(c.to);
    i->o.push_back({o,c.weight});
    o->i.push_back({i,c.weight});
  }

#if DEBUG_ES >= 2
  std::cerr << "\ninodes:\n";
  for (const auto &n: inodes) std::cerr << "\t" << n->p << "\n";
  std::cerr << "\nonodes:\n";
  for (const auto &n: onodes) std::cerr << "\t" << n->p << "\n";
  std::cerr << "\n";
#endif

  std::queue<N*> q;
  const auto breathFirstSearch =
      [&q] (const auto &src, auto &set, auto field) {
    for (const auto &n: src)  q.push(n);
    while (!q.empty()) {
      N *n = q.front();
      q.pop();

      if (n->t == Type::H) set.insert(n);
      for (auto &l: n->*field) if (set.find(l.n) == set.end()) q.push(l.n);
    }
  };
  std::set<N*, CMP> iseen, oseen;
  breathFirstSearch(inodes, iseen, &N::o);
  breathFirstSearch(onodes, oseen, &N::i);

#if DEBUG_ES >= 2
  std::cerr << "hidden nodes:\n\tiseen:\n";
  for (const auto *n: iseen)  std::cerr << "\t\t" << n->p << "\n";
  std::cerr << "\n\toseen:\n";
  for (const auto *n: oseen)  std::cerr << "\t\t" << n->p << "\n";
  std::cerr << "\n";
#endif

  CMP cmp;
  std::vector<N*> hiddenNodes;
  std::set_intersection(iseen.begin(), iseen.end(), oseen.begin(), oseen.end(),
                        std::back_inserter(hiddenNodes), cmp);

  connections.clear();
  for (const N *n: hiddenNodes) {
#if DEBUG_ES >= 3
    std::cerr << "\t" << n->p << "\n";
#endif
    shidden.insert(n->p);
    for (const L &l: n->i)  connections.push_back({l.n->p, n->p, l.w});
    for (const L &l: n->o)
      if (l.n->t == Type::O)
        connections.push_back({n->p, l.n->p, l.w});
  }

  for (auto it=nodes.begin(); it!=nodes.end();) {
    delete *it;
    it = nodes.erase(it);
  }
}

void connect (const CPPN &cppn,
              const Coordinates &inputs, const Coordinates &outputs,
              Coordinates &hidden, Connections &connections) {

  static const auto &iterations = Config::iterations();

#if DEBUG_ES
  using utils::operator<<;
  std::ostringstream oss;
  oss << "\n## --\nStarting evolvable substrate instantiation\n";
#endif

  std::set<Point> shidden;
  Connections tmpConnections;
  for (const Point &p: inputs) {
    auto t = divisionAndInitialisation(cppn, p, true);
    pruneAndExtract(cppn, p, tmpConnections, t, true);
    for (auto c: tmpConnections)  shidden.insert(c.to);
  }

#if DEBUG_ES
  oss << "[I -> H] found " << shidden.size() << " hidden neurons\n\t"
      << shidden << "\n"
      << " and " << tmpConnections.size() << " connections\n\t"
      << tmpConnections << "\n";
#endif

  connections.insert(connections.end(),
                     tmpConnections.begin(), tmpConnections.end());
  tmpConnections.clear();

  std::set<Point> unexploredHidden = shidden;
  for (uint i=0; i<iterations; i++) {
    for (const Point &p: unexploredHidden) {
      auto t = divisionAndInitialisation(cppn, p, true);
      pruneAndExtract(cppn, p, tmpConnections, t, true);
      for (auto c: tmpConnections)  shidden.insert(c.to);
    }

    std::set<Point> tmpHidden;
    std::set_difference(shidden.begin(), shidden.end(),
                        unexploredHidden.begin(), unexploredHidden.end(),
                        std::inserter(tmpHidden, tmpHidden.end()));
    unexploredHidden = tmpHidden;

#if DEBUG_ES
  oss << "[H -> H] found " << unexploredHidden.size()
      << " hidden neurons\n\t" << unexploredHidden << "\n"
      << " and " << tmpConnections.size() << " connections\n\t"
      << tmpConnections << "\n";
#endif

    connections.insert(connections.end(),
                       tmpConnections.begin(), tmpConnections.end());
    tmpConnections.clear();
  }

  for (const Point &p: outputs) {
    auto t = divisionAndInitialisation(cppn, p, false);
    pruneAndExtract(cppn, p, tmpConnections, t, false);
  }

#if DEBUG_ES
  oss << "[H -> O] found " << tmpConnections.size() << " connections\n\t"
      << tmpConnections << "\n";
#endif

  connections.insert(connections.end(),
                     tmpConnections.begin(), tmpConnections.end());

  std::set<Point> shidden2;
  removeUnconnectedNeurons(inputs, outputs, shidden2, connections);

#if DEBUG_ES
  oss << "[Filtrd] total " << shidden2.size()
      << " hidden neurons\n\t" << shidden2 << "\n"
      << " and " << connections.size() << " connections\n\t"
      << connections << "\n";
#endif

  std::copy(shidden2.begin(), shidden2.end(), std::back_inserter(hidden));

#if DEBUG_ES
  std::cerr << oss.rdbuf() << std::endl;;
#endif
}

} // end of namespace evolvable substrate

bool ANN::empty(void) const {
  for (const auto &p: _neurons)
    if (!p.second->links().empty())
      return false;
  return true;
}

ANN ANN::build (const Coordinates &inputs,
                const Coordinates &outputs, const CPPN &cppn) {

  static const auto& weightRange = config::EvolvableSubstrate::weightRange();

  ANN ann;

  NeuronsMap &neurons = ann._neurons;

  const auto add = [&cppn, &ann] (auto p, auto t) {
    auto inputs = cppn.inputs();
    float bias = 0;
    if (t != Neuron::I) {
      inputs[0] = p.x();
      inputs[1] = p.y();
      inputs[2] = inputs[3] = 0;
      bias = cppn(inputs, config::CPPNOutput::BIAS);
    }
    return ann.addNeuron(p, t, bias);
  };

  uint i = 0;
  ann._inputs.resize(inputs.size());
  for (auto &p: inputs) neurons[p] = ann._inputs[i++] = add(p, Neuron::I);

  i = 0;
  ann._outputs.resize(outputs.size());
  for (auto &p: outputs) neurons[p] = ann._outputs[i++] = add(p, Neuron::O);

  Coordinates hidden;
  evolvable_substrate::Connections connections;
  evolvable_substrate::connect(cppn, inputs, outputs, hidden,
                               connections);

  for (auto &p: hidden) neurons[p] = add(p, Neuron::H);
  for (auto &c: connections)
    neurons.at(c.to)->addLink(c.weight * weightRange, neurons.at(c.from));

  return ann;
}

ANN::Neuron::ptr ANN::addNeuron(const Point &p, Neuron::Type t, float bias) {
  return std::make_shared<Neuron>(p, t, bias);
}

#ifdef WITH_GVC
gvc::GraphWrapper ANN::build_gvc_graph (void) const {
  using namespace gvc;

  GraphWrapper g ("ann");

  uint i = 0;
  std::map<Neuron*, Agnode_t*> gvnodes;
  std::vector<std::pair<Neuron*, Neuron::Link>> links;

  /*
   * In non-debug builds (otherwise everything is fine)
   *      default -> ok
   *   false/line -> ok
   * true/splines -> segfault
   *     polyline -> segfault
   *       curved -> ok
   *        ortho -> ok
   */
  set(g.graph, "splines", "line");

  // Dot only -> useless here
//  set(g.graph, "concentrate", "true");

  // Moves nodes...
//  set(g.graph, "overlap", "false");

  for (const auto &p: _neurons) {
    Point pos = p.first;
    auto neuron = p.second;
    auto n = gvnodes[neuron.get()] = add_node(g.graph, "N", i++);
    set(n, "label", "");
    set(n, "pos", scale*pos.x(), ",", scale*pos.y());
    set(n, "width", ".1");
    set(n, "height", ".1");
    set(n, "style", "filled");
    set(n, "fillcolor", (neuron->type != Neuron::H) ? "black" : "gray");
    set(n, "spos", p.first);

    bool selfRecurrent = false;
    for (const auto &l: neuron->links()) {
      links.push_back({ neuron.get(), l });
      selfRecurrent |= (l.in.lock() == neuron);
    }

    set(n, "srecurrent", selfRecurrent);
  }

  for (const auto &p: links) {
    Neuron *out = p.first;
    Neuron *in = p.second.in.lock().get();

    // Self-recurrent edges are hidden
    if (out == in) continue;

    auto e = add_edge(g.graph, gvnodes.at(in), gvnodes.at(out), "E", i++);
    auto w = p.second.weight;
    set(e, "color", w < 0 ? "red" : "black");
    auto sw = std::fabs(w);// / config_t::weightBounds().max;
    set(e, "penwidth", .875*sw+.125);
    set(e, "w", w);
  }

  return g;
}

void ANN::render_gvc_graph(const std::string &path) const {
  auto ext_i = path.find_last_of('.')+1;
  const char *ext = path.data()+ext_i;

  gvc::GraphWrapper g = build_gvc_graph();

  g.layout("nop");
  gvRenderFilename(gvc::context(), g.graph, ext, path.c_str());
//  gvRender(gvc::context(), g.graph, "dot", stdout);

  auto dpath = path;
  dpath.replace(ext_i, 3, "nop");
  gvRenderFilename(gvc::context(), g.graph, "dot", dpath.c_str());
}
#endif

void ANN::reset(void) {
  for (auto &p: _neurons) p.second->reset();
}

void ANN::operator() (const Inputs &inputs, Outputs &outputs, uint substeps) {
  static const auto &activation =
    phenotype::CPPN::functions.at(config::EvolvableSubstrate::activationFunc());
  assert(inputs.size() == _inputs.size());
  assert(outputs.size() == outputs.size());

  for (uint i=0; i<inputs.size(); i++) _inputs[i]->value = inputs[i];

#ifdef DEBUG_COMPUTE
  using utils::operator<<;
  std::cerr << "## Compute step --\n inputs:\t" << inputs << "\n";
#endif

  for (uint s = 0; s < substeps; s++) {
#ifdef DEBUG_COMPUTE
    std::cerr << "#### Substep " << s+1 << " / " << substeps << "\n";
#endif

    for (const auto &p: _neurons) {
      if (p.second->isInput()) continue;

      float v = p.second->bias;
      for (const auto &l: p.second->links()) {
#ifdef DEBUG_COMPUTE
        std::cerr << "        i> v = " << v + l.weight * l.in.lock()->value
                  << " = " << v << " + " << l.weight << " * "
                  << l.in.lock()->value << "\n";
#endif

        v += l.weight * l.in.lock()->value;
      }

      p.second->value = activation(v);
      assert(-1 <= p.second->value && p.second->value <= 1);

#ifdef DEBUG_COMPUTE
      std::cerr << "      <o " << p.first << ": " << p.second->value << " = "
                << config::EvolvableSubstrate::activationFunc() << "("
                << v << ")\n";
#endif
    }
  }

  for (uint i=0; i<_outputs.size(); i++)  outputs[i] = _outputs[i]->value;

#ifdef DEBUG_COMPUTE
  std::cerr << "outputs:\t" << outputs << "\n## --\n";
#endif
}

// =============================================================================

void to_json (nlohmann::json &j, const ANN::Neuron::ptr &n) {
  nlohmann::json jl;
  for (const ANN::Neuron::Link &l: n->links())
    jl.push_back({l.weight, l.in.lock()->pos});
  j = { n->pos, n->type, n->bias, n->value, jl };
}

void to_json (nlohmann::json &j, const ANN &ann) {
  nlohmann::json jn, ji, jo;
  jn = ann._neurons;
  for (const auto &i: ann._inputs)  ji.push_back(i->pos);
  for (const auto &o: ann._outputs) jo.push_back(o->pos);
  j = { jn, ji, jo };
}

// =============================================================================

void from_json (const nlohmann::json &j, ANN::Neuron::ptr &n) {

}

void from_json (const nlohmann::json &j, ANN &ann) {
  assert(false);
//  ann = ANN();

//  uint i = 0;
//  ann._neurons = j[i++];
//  for (const ANN::Point &p: j[i++])
//    ann._inputs.push_back(ann._neurons.at(p));
//  for (const ANN::Point &p: j[i++])
//    ann._outputs.push_back(ann._neurons.at(p));
}

// =============================================================================

#define ASRT(X) assertEqual(lhs.X, rhs.X, deepcopy)
void assertEqual (const ANN::Neuron &lhs, const ANN::Neuron &rhs,
                  bool deepcopy) {

  using utils::assertEqual;
  ASRT(pos);
  ASRT(type);
  ASRT(bias);
  ASRT(value);
  ASRT(_links);
}

void assertEqual (const ANN::Neuron::Link &lhs, const ANN::Neuron::Link &rhs,
                  bool deepcopy) {
  using utils::assertEqual;
  ASRT(weight);
  ASRT(in.lock()->pos);
}

void assertEqual (const ANN &lhs, const ANN &rhs, bool deepcopy) {
  using utils::assertEqual;
  ASRT(_neurons);
  ASRT(_inputs);
  ASRT(_outputs);
}
#undef ASRT

} // end of namespace phenotype

#define CFILE config::EvolvableSubstrate

DEFINE_PARAMETER(uint, initialDepth, 3)
DEFINE_PARAMETER(uint, maxDepth, 3)
DEFINE_PARAMETER(uint, iterations, 1)

DEFINE_PARAMETER(float, divThr, .03) // division
DEFINE_PARAMETER(float, varThr, .03)  // variance
DEFINE_PARAMETER(float, bndThr, .15)  // band-pruning

DEFINE_PARAMETER(float, weightRange, 3)
DEFINE_CONST_PARAMETER(genotype::ES_HyperNEAT::CPPN::Node::FuncID,
                       activationFunc, "ssgn")

DEFINE_SUBCONFIG(genotype::ES_HyperNEAT::config_t, configGenotype)

#undef CFILE


#ifdef DEBUG_QUADTREE
namespace quadtree_debug {

const stdfs::path& debugFilePrefix(const stdfs::path &path) {
  static stdfs::path p;
  if (!path.empty())  p = path;
  return p;
}

float trunc (float x) {
  return std::round(100 * x) / 100.f;
}

using QTree = phenotype::evolvable_substrate::QuadTreeNode;
void debugGenerateImages (const QTree &t,
                          const phenotype::ANN::Point &p, bool in) {
  if (debugFilePrefix().empty())
    throw std::invalid_argument("debug file prefix is empty");

  std::ostringstream oss;
  oss << debugFilePrefix() << "_" << trunc(p.x()) << "_" << trunc(p.y()) << "_"
      << (in ? "i" : "o") << ".png";
  std::string output = oss.str();
  std::cerr << "Writing quadtree-detected cppn pattern for " << p
            << " to " << output << "\n";

  oss.str("");
  oss << "gnuplot -e \"\n"
      << "  set term pngcairo size 1050,1050 font ',24' "
        << "transparent;\n"
      << "  set output '" << output << "';\n"
      << "  set xrange [-1:1]; set yrange [-1:1];\n"
//      << "  unset xtics; unset ytics; set margins 0,0,0,0;\n"
      << "  set palette defined (0 'red', 1 'black', 2 'white');\n"
      << "  set style rect lc rgb '#0000FF';"
      << "  unset colorbox;\n"
      << "\n";


  using F = void (*) (std::ostream&, const QTree&);
  static const F worker = [] (std::ostream &os, const QTree &t) {
    if (t.cs.empty())
      os << "  set object rect from " << t.center.x()-t.radius << ","
         << t.center.y()-t.radius << " to " << t.center.x()+t.radius << ","
         << t.center.y()+t.radius << " fc palette frac "
         << (.5*t.weight + .5) << ";\n";
    else  for (auto &c: t.cs)  worker(os, *c);
  };
//  oss << "  set object rect from -1,-1 to 0,0 fc palette frac 0.0;\n";
//  oss << "  set object rect from 0,0 to 1,1 fc palette frac 0.5;\n";
//  oss << "  set object rect from -1,0 to 0,1 fc palette frac 1.0;\n";
  worker(oss, t);

  oss << "  \nplot x linecolor '#FF000000' notitle;\"";

  auto r = system(oss.str().c_str());
//  std::cerr << "Executed\n" << oss.str() << "\nwith exit code " << r
//            << std::endl;
}

} // end of namespace quadtree_debug
#endif
