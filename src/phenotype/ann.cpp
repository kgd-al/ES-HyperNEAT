#include <queue>

#include "ann.h"

/// TODO Find out which LEO implementation is best
/// TODO No direct input -> output connections (feature?)

namespace phenotype {

#ifndef NDEBUG
//#define DEBUG
//#define DEBUG_COMPUTE 0
//#define DEBUG_ES 1
#endif

#if DEBUG_ES
//#define DEBUG_ES_QUADTREE 1
#endif

namespace evolvable_substrate {
static constexpr uint initialDepth = 3;
static constexpr uint maxDepth = 5;
static constexpr uint iterations = 1;
static constexpr float varThr = .03;  // variance
static constexpr float bndThr = .03;  // band
static constexpr float bprThr = .3; // band-pruning
static constexpr float divThr = .5; // division
static constexpr bool useLEO = true;
static constexpr bool discoverLEONodes = useLEO && true;

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

  /// TODO Is this computed often enough?
  float variance (void) const {
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

  QuadTree root = node(0.f, 0.f, 1.f, 1);
  std::queue<QuadTreeNode*> q;
  q.push(root.get());

  static auto cppn_inputs = cppn.inputs();
  static auto cppn_outputs = cppn.outputs();
  const auto weight = [&cppn] (const Point &p0, const Point &p1) {
    cppn_inputs[0] = p0.x();
    cppn_inputs[1] = p0.y();
    cppn_inputs[2] = p1.x();
    cppn_inputs[3] = p1.y();
    cppn(cppn_inputs, cppn_outputs);
    auto r = cppn_outputs[0];
    if (discoverLEONodes && cppn_outputs.size() >= 2)
      r *= cppn_outputs[1];
    return r;
  };

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

    if (n.level < initialDepth || (n.level < maxDepth && n.variance() > divThr))
      for (auto &c: n.cs) q.push(c.get());
  }

#if DEBUG_ES_QUADTREE
  std::cerr << *root;
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

  static const auto leo = [] (const auto &cppn, auto i, auto o) {
    static auto inputs = cppn.inputs();
    static auto outputs = cppn.outputs();
    inputs[0] = i.x();
    inputs[1] = i.y();
    inputs[2] = o.x();
    inputs[3] = o.y();
    cppn(inputs, outputs);
    return (bool)outputs[1];
  };
  static const auto leoConnection = [] (const auto &cppn, auto i, auto o) {
    return !useLEO || discoverLEONodes || leo(cppn, i, o);
  };

  for (auto &c: t->cs) {
    if (c->variance() >= varThr)
      pruneAndExtract(cppn, p, con, c, out);
    else {
      static auto cppn_inputs = cppn.inputs();
      static auto cppn_outputs = cppn.outputs();
      const auto weight = [&cppn, &p, &c, out] (float x, float y) {
        cppn_inputs[0] = out ? p.x() : x;
        cppn_inputs[1] = out ? p.y() : y;
        cppn_inputs[2] = out ? x : p.x();
        cppn_inputs[3] = out ? y : p.y();
        cppn(cppn_inputs, cppn_outputs);
        return std::fabs(c->weight - cppn_outputs[0]);
      };
      float cx = c->center.x(), cy = c->center.y();
      float r = c->radius;
      float dl = weight(cx-r, cy), dr = weight(cx+r, cy),
            dt = weight(cx, cy-r), db = weight(cx, cy+r);

      if (std::max(std::min(dl, dr), std::min(dt, db)) > bndThr
          && leoConnection(cppn, out ? p : c->center, out ? c->center : p)) {
        con.push_back({
          out ? p : c->center, out ? c->center : p, c->weight
        });
      }
    }
  }
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

  std::set<N*> iseen, oseen;
  std::queue<N*> q;
  const auto breathFirstSearch =
      [&q] (const auto &src, std::set<N*> &set, auto field) {
    for (const auto &n: src)  q.push(n);
    while (!q.empty()) {
      N *n = q.front();
      q.pop();

      if (n->t == Type::H) set.insert(n);
      for (auto &l: n->*field) if (set.find(l.n) == set.end()) q.push(l.n);
    }
  };
  breathFirstSearch(inodes, iseen, &N::o);
  breathFirstSearch(onodes, oseen, &N::i);

  std::vector<N*> hiddenNodes;
  std::set_intersection(iseen.begin(), iseen.end(), oseen.begin(), oseen.end(),
                        std::back_inserter(hiddenNodes));

  connections.clear();
  for (const N *n: hiddenNodes) {
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

#if DEBUG_ES
  using utils::operator<<;
  std::cerr << "\n## --\nStarting evolvable substrate instantiation\n";
#endif

  std::set<Point> shidden;
  Connections tmpConnections;
  for (const Point &p: inputs) {
    auto t = divisionAndInitialisation(cppn, p, true);
    pruneAndExtract(cppn, p, tmpConnections, t, true);
    for (auto c: tmpConnections)  shidden.insert(c.to);
  }

#if DEBUG_ES
  std::cerr << "[I -> H] found " << shidden.size() << " hidden neurons\n\t"
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
  std::cerr << "[H -> H] found " << unexploredHidden.size()
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
  std::cerr << "[H -> O] found " << tmpConnections.size() << " connections\n\t"
            << tmpConnections << "\n";
#endif

  connections.insert(connections.end(),
                     tmpConnections.begin(), tmpConnections.end());

  std::set<Point> shidden2;
  removeUnconnectedNeurons(inputs, outputs, shidden2, connections);

#if DEBUG_ES
  std::cerr << "[Filtrd] total " << shidden2.size()
            << " hidden neurons\n\t" << shidden2 << "\n"
            << " and " << connections.size() << " connections\n\t"
            << connections << "\n";
#endif

  std::copy(shidden2.begin(), shidden2.end(), std::back_inserter(hidden));
}

} // end of namespace evolvable substrate


ANN::ANN(void){}

ANN ANN::build (const Point &bias, const Coordinates &inputs,
                const Coordinates &outputs,
                const genotype::ES_HyperNEAT &genome, const CPPN &cppn) {
  ANN ann;
  ann._hasBias = (!std::isnan(bias.x()) && !std::isnan(bias.y()));

  NeuronsMap &neurons = ann._neurons;

#if defined(WITH_GVC) || !defined(CLUSTER_BUILD)
  const auto add = [&ann] (auto p, auto t) { return ann.addNeuron(p, t); };
#else
  const auto add = [&ann] (auto, auto) { return ann.addNeuron(); };
#endif

  if (ann._hasBias) neurons[bias] = add(bias, Neuron::B);

  uint i = 0;
  ann._inputs.resize(inputs.size());
  for (auto &p: inputs) neurons[p] = ann._inputs[i++] = add(p, Neuron::I);

  i = 0;
  ann._outputs.resize(outputs.size());
  for (auto &p: outputs) neurons[p] = ann._outputs[i++] = add(p, Neuron::O);

  Coordinates hidden;
  evolvable_substrate::Connections connections;
  auto inputsWithBias = inputs;
  if (ann._hasBias) inputsWithBias.push_back(bias);
  evolvable_substrate::connect(cppn, inputsWithBias, outputs, hidden,
                               connections);

  for (auto &p: hidden) neurons[p] = add(p, Neuron::H);
  for (auto &c: connections) {
    auto &src = neurons.at(c.from), &dst = neurons.at(c.to);
    dst->links.push_back({c.weight,src});
  }

  return ann;
}

#if defined(WITH_GVC) || !defined(CLUSTER_BUILD)
ANN::Neuron::ptr ANN::addNeuron(const Point &p, Neuron::Type t) {
#else
ANN::Neuron::ptr ANN::addNeuron(void) {
#endif
  auto n = std::make_shared<Neuron>();

  n->value = (t == Neuron::B) ? 1 : 0;

#ifndef CLUSTER_BUILD
  n->pos = p;
#endif
#if defined(WITH_GVC) || !defined(CLUSTER_BUILD)
  n->type = t;
#endif

  return n;
}

#ifdef WITH_GVC
gvc::GraphWrapper ANN::build_gvc_graph (const char *ext) const {
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
    for (const auto &l: neuron->links) {
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
    set(e, "weight", w);
  }

  return g;
}

void ANN::render_gvc_graph(const std::string &path) const {
  auto ext_i = path.find_last_of('.')+1;
  const char *ext = path.data()+ext_i;

  gvc::GraphWrapper g = build_gvc_graph(ext);

  g.layout("nop");
  gvRenderFilename(gvc::context(), g.graph, ext, path.c_str());
//  gvRender(gvc::context(), g.graph, "dot", stdout);

  auto dpath = path;
  dpath.replace(ext_i, 3, "nop");
  gvRenderFilename(gvc::context(), g.graph, "dot", dpath.c_str());
}
#endif

void ANN::reset(void) {
  for (auto &p: _neurons) if (p.second->type != Neuron::B)  p.second->value = 0;
}

void ANN::operator() (const Inputs &inputs, Outputs &outputs, uint substeps) {
  static const auto activation = phenotype::CPPN::functions.at("sigm");
  assert(inputs.size() == _inputs.size());
  assert(outputs.size() == outputs.size());

  for (uint i=0; i<inputs.size(); i++)  _inputs[i]->value = inputs[i];

#ifdef DEBUG_COMPUTE
  using utils::operator<<;
  std::cerr << "## Compute step --\n inputs:\t" << inputs << "\n";
#endif

  for (uint s = 0; s < substeps; s++) {
    for (const auto &p: _neurons) {
      if (p.second->isInput()) continue;

      float v = 0;
      for (const auto &l: p.second->links) {
#ifdef DEBUG_COMPUTE
        std::cerr << "\t\tv = " << v + l.weight * l.in.lock()->value
                  << " = " << v << " + " << l.weight << " * "
                  << l.in.lock()->value << "\n";
#endif

        v += l.weight * l.in.lock()->value;
      }

#ifdef DEBUG_COMPUTE
      std::cerr << "\t" << p.first << ": " << activation(v) << " = sigm(" << v
                << ")\n";
#endif

      p.second->value = activation(v);
    }
  }

  for (uint i=0; i<_outputs.size(); i++)  outputs[i] = _outputs[i]->value;

#ifdef DEBUG_COMPUTE
  std::cerr << "outputs:\t" << outputs << "\n## --\n";
#endif
}

} // end of namespace phenotype
