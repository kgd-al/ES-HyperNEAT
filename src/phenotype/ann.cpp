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
                const Coordinates &hidden,
                const genotype::ES_HyperNEAT &genome, const CPPN &cppn) {
  ANN ann;
#ifndef CLUSTER_BUILD
  NeuronsMap &neurons = ann._neurons;
#else
  NeuronsMap neurons;
#endif

#if defined(WITH_GVC) || !defined(CLUSTER_BUILD)
  const auto add = [&ann] (auto p, auto t) { return ann.addNeuron(p, t); };
  const auto type = [] (auto n) -> Neuron::Type { return n->type; };
#else
  const auto add = [&ann] (auto, auto) { return ann.addNeuron(); };
#endif

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
//    std::cerr << "neuron at " << p.first << "\n";
    if (type(p.second) == Neuron::O) {
//      std::cerr << "\tno outgoing connections for output node\n";
      continue;
    }

    auto lb = neurons.lower_bound(RangeFinder{minY});
//    std::cerr << "\t" << std::distance(lb, neurons.end())
//              << " link candidates in [" << minY << ", 1]\n";
    while (lb != neurons.end()) {
      if (type(lb->second) == Neuron::I) {
//        std::cerr << "\t\tNo incoming connection for input node at "
//                  << lb->first << "\n";
      } else {
//        std::cerr << "\t\t-> " << lb->first << ": ";

        cppn_inputs = {
          p.first.x(), p.first.y(), lb->first.x(), lb->first.y() };
        cppn(cppn_inputs, cppn_outputs);

//        std::cerr << " {" << cppn_outputs[0] << " " << cppn_outputs[1] << "}\n";
        if (cppn_outputs[1] > 0)
          p.second->links.push_back({cppn_outputs[0], lb->second});
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

    for (const auto &l: neuron->links)  links.push_back({ neuron.get(), l });
  }

  for (const auto &p: links) {
    auto e = add_edge(g.graph, gvnodes.at(p.first),
                      gvnodes.at(p.second.dst.lock().get()), "E", i++);
    auto w = p.second.weight;
    set(e, "color", w < 0 ? "red" : "black");
    auto sw = std::fabs(w);// / config_t::weightBounds().max;
    set(e, "penwidth", 3.75*sw+.25);
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

} // end of namespace phenotype
