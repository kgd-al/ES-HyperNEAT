#include <queue>

#include "ann.h"

#include "kgd/utils/indentingostream.h" /// TODO REMOVE

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
//#define DEBUG_ES 1
#endif

#if DEBUG_ES
//#define DEBUG_ES_QUADTREE 1
#endif

namespace evolvable_substrate {

using Config = config::EvolvableSubstrate;

using Coordinates = ANN::Coordinates;
using Coordinates_s = std::set<Coordinates::value_type>;

struct QOTreeNode {
  Point center;
  /// TODO remove
//  static_assert(ESHN_SUBSTRATE_DIMENSION == 2, "OctoTree not implemented");
  float radius;
  uint level;
  float weight;

  using ptr = std::shared_ptr<QOTreeNode>;
  std::vector<ptr> cs;

  QOTreeNode (const Point &p, float r, uint l)
    : center(p), radius(r), level(l), weight(NAN) {}

#if ESHN_SUBSTRATE_DIMENSION == 2
  QOTreeNode (float x, float y, float r, uint l)
    : QOTreeNode({x,y}, r, l) {}
#elif ESHN_SUBSTRATE_DIMENSION == 3
  QOTreeNode (float x, float y, float z, float r, uint l)
    : QOTreeNode({x,y,z}, r, l) {}
#endif

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
using QOTree = QOTreeNode::ptr;

template <typename... ARGS>
QOTreeNode::ptr node (ARGS... args) {
  return std::make_shared<QOTreeNode>(args...);
}


QOTree divisionAndInitialisation(const CPPN &cppn, const Point &p, bool out) {
  static const auto &initialDepth = Config::initialDepth();
  static const auto &maxDepth = Config::maxDepth();
  static const auto &divThr = Config::divThr();

  QOTree root = node(Point::null(), 1.f, 1);
  std::queue<QOTreeNode*> q;
  q.push(root.get());

  const auto weight = [&cppn] (const Point &p0, const Point &p1) {
    return cppn(p0, p1, genotype::cppn::Output::WEIGHT);
  };

#ifdef DEBUG_QUADTREE_DIVISION
  std::cout << "divisionAndInitialisation(" << p << ", " << out << ")\n";
#endif

  while (!q.empty()) {
    QOTreeNode &n = *q.front();
    q.pop();

    float cx = n.center.x(), cy = n.center.y();
#if ESHN_SUBSTRATE_DIMENSION == 3
    float cz = n.center.z();
#endif
    float hr = .5 * n.radius;
    float nl = n.level + 1;

    n.cs.resize(1 << ESHN_SUBSTRATE_DIMENSION);
    uint i=0;
    for (int x: {-1,1})
      for (int y: {-1, 1})
#if ESHN_SUBSTRATE_DIMENSION == 2
        n.cs[i++] = node(cx + x * hr, cy + y * hr, hr, nl);
#elif ESHN_SUBSTRATE_DIMENSION == 3
        for (int z: {-1,1})
          n.cs[i++] = node(cx + x * hr, cy + y * hr, cz + z * hr, hr, nl);
#endif

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
                      const QOTree &t, bool out) {

  static const auto &varThr = Config::varThr();
  static const auto &bndThr = Config::bndThr();
  static const auto leo = [] (const auto &cppn, auto i, auto o) {
    return (bool)cppn(i, o, genotype::cppn::Output::LEO);
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

      float r = c->radius;
      float bnd = 0;

      float cx = c->center.x(), cy = c->center.y();
      const auto dweight = [&cppn, &p, &c, out] (auto... coords) {
        Point src = out ? p : Point{coords...},
              dst = out ? Point{coords...} : p;
        return std::fabs(c->weight
                         - cppn(src, dst, genotype::cppn::Output::WEIGHT));
      };


#if ESHN_SUBSTRATE_DIMENSION == 2
      bnd = std::max(
        std::min(dweight(cx-r, cy), dweight(cx+r, cy)),
        std::min(dweight(cx, cy-r), dweight(cx, cy+r))
      );

#elif ESHN_SUBSTRATE_DIMENSION == 3
      float cz = c->center.z();
      bnd = std::max({
        std::min(dweight(cx-r, cy, cz), dweight(cx+r, cy, cz)),
        std::min(dweight(cx, cy-r, cz), dweight(cx, cy+r, cz)),
        std::min(dweight(cx, cy, cz-r), dweight(cx, cy, cz+r))
      });

#endif

#ifdef DEBUG_QUADTREE_PRUNING
      std::cout << "b> var = " << c->variance() << ", bnd = "
                << std::max(std::min(dl, dr), std::min(dt, db))
                << " = max(min(" << dl << ", " << dr << "), min(" << dt
                << ", " << db << ")) && leo = "
                << leoConnection(cppn, out ? p : c->center, out ? c->center : p)
                << "\n";
#endif

      if (bnd > bndThr
          && leo(cppn, out ? p : c->center, out ? c->center : p)
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
                               Coordinates_s &shidden,
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
  std::cerr << "\nconnections:\n";
  for (const auto &c: connections)
    std::cerr << "\t" << c.from << " -> " << c.to << "\n";
  std::cerr << "\n";
#endif

  const auto breadthFirstSearch =
      [] (const auto &src, auto &set, auto field) {
    std::queue<N*> q;
    typename std::remove_reference_t<decltype(set)> seen;
    for (const auto &n: src)  q.push(n), seen.insert(n);
    while (!q.empty()) {
      N *n = q.front();
      q.pop();

      for (auto &l: n->*field) {
        N *n_ = l.n;
        if (seen.find(n_) == seen.end()) {
          if (n_->t == Type::H) set.insert(n_);
          seen.insert(n_);
          q.push(n_);
        }
      }
    }
  };

  std::set<N*, CMP> iseen, oseen;
  breadthFirstSearch(inodes, iseen, &N::o);
  breadthFirstSearch(onodes, oseen, &N::i);

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

/// Collect new hidden nodes IFF they do not appear in provided coordinates
/// otherwise ignore them and remove their connection
void collectIfDiscovered (Connections &connections,
                          const Coordinates_s &io,
                          Coordinates_s &h) {
  for (auto it = connections.begin(); it != connections.end(); ) {
    auto c = *it;
    if (io.find(c.to) == io.end()) {
      h.insert(c.to);
      it++;
    } else
      it = connections.erase(it);
  }
}

bool connect (const CPPN &cppn,
              const Coordinates &inputs, const Coordinates &outputs,
              Coordinates &hidden, Connections &connections) {

  using utils::operator<<;
  static const auto &iterations = Config::iterations();
  static const auto overflow = [] (Coordinates_s &h, Connections &c) {
    static const auto &H = Config::neuronsUpperBound();
    static const auto &C = Config::connectionsUpperBound();
    if (H <= h.size() || C <= c.size()) {
      std::cerr << "[ANN BUILD] overflow: "
                << H << " < " << h.size() << " | "
                << C << " < " << c.size() << std::endl;
      return true;
    } else
      return false;
  };

  Coordinates_s sio;  // All fixed positions
  for (const auto &vec: {inputs, outputs}) {
    for (Point p: vec) {
      auto r = sio.insert(p);
      if (!r.second) {
        std::cerr << "inputs: " << inputs << "\noutputs: " << outputs
                  << std::endl;
        utils::Thrower("Unable to insert duplicate coordinate ", p);
      }
    }
  }

#if DEBUG_ES
  std::ostringstream oss;
  oss << "\n## --\nStarting evolvable substrate instantiation\n";
#endif

  Coordinates_s shidden;
  Connections tmpConnections;
  for (const Point &p: inputs) {
    auto t = divisionAndInitialisation(cppn, p, true);
    pruneAndExtract(cppn, p, tmpConnections, t, true);
    collectIfDiscovered(tmpConnections, sio, shidden);
  }

#if DEBUG_ES
  oss << "[I -> H] found " << shidden.size() << " hidden neurons";
#if DEBUG_ES >= 3
  oss << "\n\t" << shidden << "\n";
#endif
  oss << " and " << tmpConnections.size() << " connections";
#if DEBUG_ES >= 3
  oss << "\n\t" << tmpConnections;
#endif
  oss << "\n";
#endif

  connections.insert(connections.end(),
                     tmpConnections.begin(), tmpConnections.end());
  tmpConnections.clear();

  if (overflow(shidden, connections)) return false;

  bool converged = false;
  Coordinates_s unexploredHidden = shidden;
  for (uint i=0; i<iterations && !converged; i++) {
    for (const Point &p: unexploredHidden) {
      auto t = divisionAndInitialisation(cppn, p, true);
      pruneAndExtract(cppn, p, tmpConnections, t, true);
      collectIfDiscovered(tmpConnections, sio, shidden);
    }

    Coordinates_s tmpHidden;
    std::set_difference(shidden.begin(), shidden.end(),
                        unexploredHidden.begin(), unexploredHidden.end(),
                        std::inserter(tmpHidden, tmpHidden.end()));
    unexploredHidden = tmpHidden;

#if DEBUG_ES
  oss << "[H -> H] found " << unexploredHidden.size() << " hidden neurons";
#if DEBUG_ES >= 3
  oss << "\n\t" << unexploredHidden << "\n";
#endif
  oss << " and " << tmpConnections.size() << " connections";
#if DEBUG_ES >= 3
  oss << "\n\t" << tmpConnections;
#endif
  oss << "\n";
#endif

    converged = (unexploredHidden.empty() && tmpConnections.empty());
#if DEBUG_ES
    if (converged)
      oss << "\t> Premature convergence at iteration " << i << "\n";
#endif

    connections.insert(connections.end(),
                       tmpConnections.begin(), tmpConnections.end());
    tmpConnections.clear();

    if (overflow(shidden, connections)) return false;
  }

  for (const Point &p: outputs) {
    auto t = divisionAndInitialisation(cppn, p, false);
    pruneAndExtract(cppn, p, tmpConnections, t, false);
  }

#if DEBUG_ES
  oss << "[H -> O] found " << tmpConnections.size() << " connections";
#if DEBUG_ES >= 3
  oss << "\n\t" << tmpConnections << "\n";
#endif
  oss << "\n";
#endif

  connections.insert(connections.end(),
                     tmpConnections.begin(), tmpConnections.end());

  if (overflow(shidden, connections)) return false;

  Coordinates_s shidden2;
  removeUnconnectedNeurons(inputs, outputs, shidden2, connections);

#if DEBUG_ES
  oss << "[Filtrd] total " << shidden2.size() << " hidden neurons";
#if DEBUG_ES >= 3
  oss << "\n\t" << shidden2 << "\n";
#endif
  oss << " and " << connections.size() << " connections";
#if DEBUG_ES >= 3
  oss << "\n\t" << connections << "\n";
#endif
#endif

  std::copy(shidden2.begin(), shidden2.end(), std::back_inserter(hidden));

#if DEBUG_ES
  std::cerr << oss.str() << std::endl;
#endif

  return true;
}

} // end of namespace evolvable substrate

bool ANN::empty(void) const {
  return stats().edges == 0;
}

ANN ANN::build (const Coordinates &inputs,
                const Coordinates &outputs, const CPPN &cppn) {

  static const auto& weightRange = config::EvolvableSubstrate::weightRange();

  ANN ann;

  NeuronsMap &neurons = ann._neurons;

  const auto add = [&cppn, &ann] (auto p, auto t) {
    float bias = 0;
    if (t != Neuron::I)
      bias = cppn(p, Point::null(), genotype::cppn::Output::BIAS);
    return ann.addNeuron(p, t, bias);
  };

  uint i = 0;
  ann._inputs.resize(inputs.size());
  for (auto &p: inputs) neurons.insert(ann._inputs[i++] = add(p, Neuron::I));

  i = 0;
  ann._outputs.resize(outputs.size());
  for (auto &p: outputs) neurons.insert(ann._outputs[i++] = add(p, Neuron::O));

  Coordinates hidden;
  evolvable_substrate::Connections connections;
  if (evolvable_substrate::connect(cppn, inputs, outputs, hidden,
                                   connections)) {
    for (auto &p: hidden) neurons.insert(add(p, Neuron::H));
    for (auto &c: connections)
      ann.neuronAt(c.to)->addLink(c.weight * weightRange, ann.neuronAt(c.from));
  }

  ann.computeStats();

  return ann;
}

uint computeDepth (ANN &ann) {
  struct ReverseNeuron {
    ANN::Neuron &n;
    std::vector<ReverseNeuron*> o;
    ReverseNeuron (ANN::Neuron &n) : n(n) {}
  };

  std::map<Point, ReverseNeuron*> neurons;
  std::set<ReverseNeuron*> next;

  uint hcount = 0, hseen = 0;
  for (const ANN::Neuron::ptr &n: ann.neurons()) {
    auto p = neurons.emplace(std::make_pair(n->pos, new ReverseNeuron(*n)));
    if (n->type == ANN::Neuron::I) next.insert(p.first->second);
    else if (n->type == ANN::Neuron::H) hcount++;
  }

  for (const ANN::Neuron::ptr &n: ann.neurons())
    for (phenotype::ANN::Neuron::Link &l: n->links())
      neurons.at(l.in.lock()->pos)->o.push_back(neurons.at(n->pos));

  std::set<ReverseNeuron*> seen;
  uint depth = 0;
  while (!next.empty()) {
    auto current = next;
    next.clear();

    for (ReverseNeuron *n: current) {
      n->n.depth = depth;
//      std::cerr << n->n.pos << ": " << depth << "\n";
      seen.insert(n);
      if (n->n.type == ANN::Neuron::H)  hseen++;
      for (ReverseNeuron *o: n->o) next.insert(o);
    }

    decltype(seen) news;
    std::set_difference(next.begin(), next.end(),
                        seen.begin(), seen.end(),
                        std::inserter(news, news.end()));
    next = news;
    assert(hseen == hcount || next.size() > 0);

    depth++;
  }

  uint d = 0;
  for (auto &p: neurons) {
    d = std::max(d, p.second->n.depth);
    delete p.second;
  }
  return d;
}

void ANN::computeStats(void) {
  if (_neurons.size() == _inputs.size() + _outputs.size()) {
    for (Neuron::ptr &n: _inputs)   n->depth = 0;
    for (Neuron::ptr &n: _outputs)  n->depth = 1;
    _stats.depth = 1;
    _stats.edges = 0;
    _stats.axons = 0;
    return;
  }

  _stats.depth = computeDepth(*this);

  auto &e = _stats.edges = 0;
  float &l = _stats.axons = 0;
  for (const Neuron::ptr &n: _neurons) {
    e += n->links().size();
    for (const Neuron::Link &link: n->links())
      l += (n->pos - link.in.lock()->pos).length();
  }
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

  set(g.graph, "splines", "true");
  set(g.graph, "margin", "0,0");
  set(g.graph, "notranslate", "true");
  set(g.graph, "dim", ESHN_SUBSTRATE_DIMENSION);

  // Dot only -> useless here
//  set(g.graph, "concentrate", "true");

  // Moves nodes...
//  set(g.graph, "overlap", "false");

  for (const auto &p: _neurons) {
    Point pos = p->pos;
    auto &neuron = *p;
    auto n = gvnodes[p.get()] = add_node(g.graph, "N", i++);
    set(n, "label", "");
#if ESHN_SUBSTRATE_DIMENSION == 2
    set(n, "pos", scale*pos.x(), ",", scale*pos.y());
#elif ESHN_SUBSTRATE_DIMENSION == 3
    set(n, "pos", scale*pos.x(), ",", scale*pos.y(), ",", scale*pos.z());
#endif
    set(n, "pos", scale*pos.x(), ",", scale*pos.y());
    set(n, "width", ".1");
    set(n, "height", ".1");
    set(n, "margin", "0.01");
    set(n, "fixedsize", "true");
    set(n, "style", "filled");
    set(n, "fillcolor", (neuron.type != Neuron::H) ? "black" : "gray");
    set(n, "spos", pos);

    bool selfRecurrent = false;
    for (const auto &l: neuron.links()) {
      links.push_back({ p.get(), l });
      selfRecurrent |= (l.in.lock() == p);
      if (selfRecurrent) break;
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
    set(e, "penwidth", .15*sw+.05);
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
  for (auto &n: _neurons) n->reset();
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
      if (p->isInput()) continue;

      float v = p->bias;
      for (const auto &l: p->links()) {
#ifdef DEBUG_COMPUTE
        std::cerr << "        i> v = " << v + l.weight * l.in.lock()->value
                  << " = " << v << " + " << l.weight << " * "
                  << l.in.lock()->value << "\n";
#endif

        v += l.weight * l.in.lock()->value;
      }

      p->value = activation(v);
      assert(-1 <= p->value && p->value <= 1);

#ifdef DEBUG_COMPUTE
      std::cerr << "      <o " << p->pos << ": " << p->value << " = "
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

void from_json (const nlohmann::json &/*j*/, ANN::Neuron::ptr &/*n*/) {

}

void from_json (const nlohmann::json &/*j*/, ANN &/*ann*/) {
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
  ASRT(_ilinks);
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

// =============================================================================

struct AggregationCriterion {
  decltype(ModularANN::Neuron::flags) flags;
  uint depth;

  AggregationCriterion (const ModularANN::Neuron &n, uint d)
    : flags(n.flags), depth(d) {}

  friend bool operator< (const AggregationCriterion &lhs,
                         const AggregationCriterion &rhs) {
    if (lhs.depth != rhs.depth) return lhs.depth < rhs.depth;
    return lhs.flags < rhs.flags;
  }
};

template <typename T> using ModulePtrMap = std::map<ModularANN::Module*, T>;

static constexpr bool debugAgg = false;

inline Point2D convert (const Point2D &p) { return p; }
inline Point2D convert (const Point3D &p) { return {p.x(), p.y()}; }

ModularANN::ModularANN (const ANN &ann, bool withDepth) : _ann(ann) {
  std::map<AggregationCriterion, Module*> moduleMap;
  std::map<Neuron*, Module*> neuronToModuleMap;

  // First create all modules
  {
    std::vector<Module*> components;
    for (const auto &p: ann.neurons()) {
      const Neuron &n = *p;

      Module *m = nullptr;

      if (n.type != Neuron::H && n.flags == 0) {
        m = components.emplace_back(new Module(n.flags));

      } else {
        AggregationCriterion key (n, withDepth ? p->depth : 0);
        auto it = moduleMap.find(key);
        if (it == moduleMap.end()) {
          m = components.emplace_back(new Module(n.flags));
          moduleMap.emplace(key, m);
        } else
          m = it->second;
      }

      assert(m);
      m->center += convert(n.pos);
      m->neurons.push_back(std::ref(n));
      neuronToModuleMap.emplace(p.get(), m);

      if (debugAgg)
        std::cerr << "\t" << n.pos << " (f=" << n.flags
                  << ") -> " << m << "\n";
    }

    // Update module's position and size
    float hSize = neuronToModuleMap.size();
    for (Module *mptr: components) {
      Module &m = *mptr;
      auto size = m.neurons.size();
      assert(size > 0);

      m.center /= size;
      m.bl = m.ur = convert(m.neurons.front().get().pos);
      for (uint i=1; i<m.neurons.size(); i++) {
        auto p = m.neurons[i].get().pos;
        for (uint j=0; j<2; j++) {
          if (p.get(j) < m.bl.get(j)) m.bl.set(j, p.get(j));
          if (m.ur.get(j) < p.get(j)) m.ur.set(j, p.get(j));
        }
      }
      if (size == 1)  m.size = {1,1};
      else {
        /// TODO make up your mind about anisotropic modules
//        Module::Size s { m.ur.x() - m.bl.x(), m.ur.y() - m.bl.y() };
        float aratio = 1;// std::max(.5f, std::min(s.w / s.h, 2.f));
        float r = (1+5*size/hSize);
        m.size = { std::max(1.f, r * aratio), std::max(1.f, r / aratio) };
      }

      auto it = _components.find(mptr->center);
      while (it != _components.end()) {
        if (debugAgg)
          std::cerr << "/!\\Position " << mptr->center << " is already taken!"
                    << " Applying small shi(f)t\n";
        static constexpr auto M = .05f;
        mptr->center += {
         utils::sgn(mptr->center.x()) * M, utils::sgn(mptr->center.y()) * M,
        };
        it = _components.find(mptr->center);
      }
      _components[mptr->center] = mptr;


      if (debugAgg) {
        std::cerr << "Processed module " << m.center << "@" << mptr << " ("
                  << m.neurons.size() << " neurons):";
        for (const auto &n: m.neurons)
          std::cerr << " " << n.get().pos;
        std::cerr << "\n";
      }
    }
  }

  // Next (filter and) generate links using module coordinates
  struct ModuleLinkBuildData { uint count; float weight; };
  ModulePtrMap<ModulePtrMap<ModuleLinkBuildData>> uniqueLinks;
  for (const auto &p: ann.neurons()) {
    const Neuron &n = *p;
    if (n.type == Neuron::I)  continue;

    Module *dst = neuronToModuleMap.at(p.get());
    auto &mlist = uniqueLinks[dst];

    if (debugAgg)
      std::cerr << "Links for " << n.pos << " (module " << dst->center << ")\n";

    for (const Neuron::Link &l: n.links()) {
      Module* src = neuronToModuleMap.at(l.in.lock().get());
      if (dst == src) {
        if (debugAgg)
          std::cerr << "\tSkipping " << l.in.lock()->pos << " -> " << n.pos
                    << " (" << dst->center << " == " << src->center << ")\n";

        dst->recurrent = true;
        continue;
      }

      if (debugAgg)
        std::cerr << "\tnl: " << l.in.lock()->pos << " -> " << dst->center
                  << " [" << l.weight << "]\n";

      auto it = mlist.find(src);
      if (it == mlist.end())
        it = mlist.emplace(src, ModuleLinkBuildData{0,0}).first;
      it->second.count++;
      it->second.weight += std::fabs(l.weight);
    }
  }

//  float nSize = ann.neurons().size();
  for (auto &pm: uniqueLinks) {
    Module *dst = pm.first;
    for (auto &pw: pm.second) {
      if (debugAgg)
        std::cerr << "\tul: " << pw.first->center << " -> "
                  << dst->center << " [" << pw.second.count << ": "
                  << pw.second.weight << "]\n";

      Module *in = _components.at(pw.first->center);
      ModuleLinkBuildData d = pw.second;
      dst->links.push_back({ d.count, d.weight / d.count, in });
    }

    if (debugAgg) {
      std::cerr << "\t--\n";
      for (const Module::Link &l: dst->links)
        std::cerr << "\t\tml: " << l.in->center << " -> "
                  << dst->center << " [" << l.weight << "]\n";
      std::cerr << "----\n\n";
    }
  }

  // Modules are ready!
  update();

#ifndef NDEBUG
  for (const auto &p: _components)
    for (const Module::Link &l: p.second->links)
      assert(l.in);
#endif
}

ModularANN::~ModularANN (void) {
  for (auto &p: _components)  delete p.second;
}

void ModularANN::Module::update (void) {
  assert(neurons.size() > 0);
  if (neurons.size() == 1)
    valueCache = { std::fabs(neurons.front().get().value), 0 };
  else {
    valueCache.mean = 0;
    for (const auto &n: neurons)
      valueCache.mean += std::fabs(n.get().value);
    valueCache.mean /= neurons.size();
    valueCache.stddev = 0;
    for (const auto &n: neurons) {
      float v = valueCache.mean - n.get().value;
      valueCache.stddev += v*v;
    }
    valueCache.stddev = std::sqrt(valueCache.stddev / neurons.size());
  }
}

void ModularANN::update (void) {
  for (auto &p: _components)  p.second->update();
}

#ifdef WITH_GVC

gvc::GraphWrapper ModularANN::build_gvc_graph (void) const {
  using namespace gvc;

  GraphWrapper g ("mann");

  uint i = 0;
  std::map<const Module*, Agnode_t*> gvnodes;
  std::vector<std::pair<Module*, Module::Link>> links;

  set(g.graph, "splines", "false");
  set(g.graph, "notranslate", "true");

  // Dot only -> useless here (and crashes in debug mode)
//  set(g.graph, "concentrate", "true");

  for (const auto &p: _components) {
    Module &m = *p.second;
    Point pos = m.center;
    auto n = gvnodes[p.second] = add_node(g.graph, "N", i++);
    set(n, "label", "");

    set(n, "pos", scale*pos.x(), ",", scale*pos.y());

    set(n, "width", .1 * m.size.w);
    set(n, "height", .1 * m.size.h);
    set(n, "style", "filled");
    set(n, "fillcolor", (m.type() != Neuron::H) ? "black" : "gray");
    set(n, "spos", pos);
    set(n, "shape", "ellipse");//m.shape());
    set(n, "srecurrent", m.recurrent);

    for (const auto &l: m.links) {
      assert(p.second);
      assert(l.in);
      links.push_back({ p.second, l });
    }
  }

  for (const auto &p: links) {
    const Module *out = p.first;
    const Module *in = p.second.in;

    // Self-recurrent edges should not be possible
    assert(out != in);
    if (out == in) continue;

    auto e = add_edge(g.graph,
                      gvnodes.at(in), gvnodes.at(out),
                      "E", i++);
    auto w = p.second.weight;
    set(e, "color", w < 0 ? "red" : "black");
//    auto sw = std::fabs(w);// / config_t::weightBounds().max;
    auto sw = std::fabs(w) * .5 * p.second.count / _components.size();
    /// TODO: magic numbers...
    set(e, "penwidth", .9*sw+.1);
    set(e, "w", w);
  }

  return g;
}

void ModularANN::render_gvc_graph(const std::string &path) const {
  static constexpr auto engine = "nop";
  auto ext_i = path.find_last_of('.')+1;
  const char *ext = path.data()+ext_i;

  gvc::GraphWrapper g = build_gvc_graph();

  g.layout(engine);
  gvRenderFilename(gvc::context(), g.graph, ext, path.c_str());
//  gvRender(gvc::context(), g.graph, "dot", stdout);

  auto dpath = path;
  dpath.replace(ext_i, 3, engine);
  gvRenderFilename(gvc::context(), g.graph, "dot", dpath.c_str());
}
#endif

// =============================================================================

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

DEFINE_PARAMETER(uint, neuronsUpperBound, -1)
DEFINE_PARAMETER(uint, connectionsUpperBound, -1)

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
