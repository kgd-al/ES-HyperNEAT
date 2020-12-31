#include "ann.h"

namespace phenotype {

template <uint D>
std::ostream& operator<< (std::ostream &os, const Point_t<D> &p) {
  os << "{";
  for (auto c: p.data)  os << " " << c;
  return os << " }";
}

ANN::ANN(void){}

ANN ANN::build (const Coordinates &inputs, const Coordinates &outputs,
                const Coordinates &hidden, const CPPN &cppn) {
  ANN ann;
#ifndef CLUSTER_BUILD
  NeuronsMap &neurons = ann._neurons;
#else
  NeuronsMap neurons;
#endif

#if defined(WITH_GVC) || !defined(CLUSTER_BUILD)
#define ADD(P, T) ann.addNeuron(P, T)
#else
#define ADD(P, T) ann.addNeuron()
#endif

  uint i = 0;
  ann._inputs.resize(inputs.size());
  for (auto &p: inputs) neurons[p] = ann._inputs[i++] = ADD(p, Neuron::I);

  i = 0;
  ann._outputs.resize(outputs.size());
  for (auto &p: outputs) neurons[p] = ann._outputs[i++] = ADD(p, Neuron::O);

  for (auto &p: hidden) neurons[p] = ADD(p, Neuron::O);

  for (const auto &p: neurons) {
    std::cerr << "neuron at " << p.first << "\n";

  }

  return ann;
}

#if defined(WITH_GVC) || !defined(CLUSTER_BUILD)
ANN::Neuron::ptr ANN::addNeuron(const Point &p, Neuron::Type t) {
#else
ANN::Neuron::ptr ANN::addNeuron(void) {
#endif
  auto n = std::make_shared<Neuron>();
  n->value = NAN;

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
  for (const auto &p: _neurons) {
    auto n = add_node(g.graph, i++);
    set(n, "label", "");
    set(n, "pos", scale*p.first.x(), ",", scale*p.first.y());
    set(n, "width", ".1");
    set(n, "height", ".1");
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

} // end of namespace phenotype
