#include "ann.h"

namespace phenotype {

#ifndef NDEBUG
//#define DEBUG
//#define DEBUG_COMPUTE
#endif

struct QuadTreeNode {
  ANN::Point center;
  float radius;
  uint level;

  using ptr = std::shared_ptr<QuadTreeNode>;
  std::array<ptr, 4> children;
};

template <uint D>
std::ostream& operator<< (std::ostream &os, const Point_t<D> &p) {
  os << "{";
  for (auto c: p.data)  os << " " << c;
  return os << " }";
}

ANN::ANN(void){}

ANN ANN::build (const Point &bias, const Coordinates &inputs, const Coordinates &outputs,
                const Coordinates &hidden,
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

  for (auto &p: hidden) neurons[p] = add(p, Neuron::H);

  auto cppn_inputs = cppn.inputs();
  auto cppn_outputs = cppn.outputs();
  for (const auto &p: neurons) {
    float minY = std::max(-1.f, p.first.y() - genome.recurrentDY);
#ifdef DEBUG
    std::cerr << "neuron at " << p.first << "\n";
#endif
    if (p.second->type == Neuron::O) {
#ifdef DEBUG
      std::cerr << "\tno outgoing connections for output node\n";
#endif
      continue;
    }

    auto lb = neurons.lower_bound(RangeFinder{minY});
#ifdef DEBUG
    std::cerr << "\t" << std::distance(lb, neurons.end())
              << " link candidates in [" << minY << ", 1]\n";
#endif
    while (lb != neurons.end()) {
      if (lb->second->isInput()) {
#ifdef DEBUG
        std::cerr << "\t\tNo incoming connection for input node at "
                  << lb->first << "\n";
#endif
      } else {
#ifdef DEBUG
        std::cerr << "\t\t-> " << lb->first << ": ";
#endif

        cppn_inputs = {
          p.first.x(), p.first.y(), lb->first.x(), lb->first.y() };
        cppn(cppn_inputs, cppn_outputs);

#ifdef DEBUG
        std::cerr << " {" << cppn_outputs[0] << " " << cppn_outputs[1] << "}\n";
#endif
        if (cppn_outputs[1] > 0 && cppn_outputs[0] != 0)
          lb->second->links.push_back({cppn_outputs[0], p.second});
      }
      ++lb;
    }
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
gvc::GraphWrapper ANN::graphviz_build_graph (const char *ext) const {
  using namespace gvc;
  static constexpr float scale = 100;

  GraphWrapper g ("ann");

  uint i = 0;
  std::map<Neuron*, Agnode_t*> gvnodes;
  std::vector<std::pair<Neuron*, Neuron::Link>> links;

  set(g.graph, "splines", "true");

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
    set(n, "spos", p.first.x(), ",", p.first.y());

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

void ANN::graphviz_render_graph(const std::string &path) const {
  auto ext_i = path.find_last_of('.')+1;
  const char *ext = path.data()+ext_i;

  gvc::GraphWrapper g = graphviz_build_graph(ext);

  g.layout("nop");
  gvRenderFilename(gvc::context(), g.graph, ext, path.c_str());
//  gvRender(gvc::context(), g.graph, "dot", stdout);

  auto dpath = path;
  dpath.replace(ext_i, 3, "nop");
  gvRenderFilename(gvc::context(), g.graph, "dot", dpath.c_str());
}
#endif

void ANN::operator() (const Inputs &inputs, Outputs &outputs) {
  static const auto activation = phenotype::CPPN::functions.at("sigm");
  assert(inputs.size() == _inputs.size());
  assert(outputs.size() == outputs.size());

  for (uint i=0; i<inputs.size(); i++)  _inputs[i]->value = inputs[i];

#ifdef DEBUG_COMPUTE
  using utils::operator<<;
  std::cerr << "## Compute step --\n inputs:\t" << inputs << "\n";
#endif

  for (const auto &p: _neurons) {
    if (p.second->isInput()) continue;

    float v = 0;
    for (const auto &l: p.second->links) {
#ifdef DEBUG_COMPUTE
      std::cerr << "\t\tv = " << v + l.weight * l.in.lock()->value
                << " = " << v << " + " << l.weight * l.in.lock()->value
                << "\n";
#endif

      v += l.weight * l.in.lock()->value;
    }

#ifdef DEBUG_COMPUTE
    std::cerr << "\t" << p.first << ": " << activation(v) << " = sigm(" << v
              << ")\n";
#endif

    p.second->value = activation(v);
  }

  for (uint i=0; i<_outputs.size(); i++)  outputs[i] = _outputs[i]->value;

#ifdef DEBUG_COMPUTE
  std::cerr << "outputs:\t" << outputs << "\n## --\n";
#endif
}

} // end of namespace phenotype
