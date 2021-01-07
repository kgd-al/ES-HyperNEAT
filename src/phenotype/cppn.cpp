#include "cppn.h"

namespace phenotype {

#ifndef NDEBUG
//#define DEBUG
#endif

#define F(NAME, BODY) \
 { NAME, [] (float x) -> float { return BODY; } }
const std::map<genotype::ES_HyperNEAT::CPPN::Node::FuncID,
               CPPN::Function> CPPN::functions {
  F( "abs", std::fabs(x)),
  F("sigm", 2.f / (1.f + std::exp(-x)) - 1.f),
  F( "sin", std::sin(x)),
  F("step", x < 0 ? 0 : 1),
  F("gaus", std::exp(-(x*x)/2)),
  F(  "id", x)
};
#undef F

#define F(NAME, MIN, MAX) { NAME, { MIN, MAX }}
const std::map<genotype::ES_HyperNEAT::CPPN::Node::FuncID,
               CPPN::Range> CPPN::functionRanges {
  F( "abs",  0, 1),
  F("sigm", -1, 1),
  F( "sin", -1, 1),
  F("step",  0, 1),
  F("gaus",  0, 1),
  F(  "id", -1, 1),
};
#undef F

CPPN::CPPN(){}

CPPN CPPN::fromGenotype(const genotype::ES_HyperNEAT &es_hyperneat) {
  using CPPN_g = genotype::ES_HyperNEAT::CPPN;
  const CPPN_g &cppn_g = es_hyperneat.cppn;
  using NID = CPPN_g::Node::ID;

  const auto &ofuncs = cppn_g.outputFunctions;

  CPPN cppn;

  auto fnode = [] (const CPPN_g::Node::FuncID &fid) {
    return std::make_shared<FNode>(functions.at(fid));
  };

  std::map<NID, Node_ptr> nodes;

  cppn._inputs.resize(cppn_g.inputs);
  for (uint i=0; i<cppn_g.inputs; i++) {
#ifdef DEBUG
    std::cerr << "(I) " << NID(i) << " " << i << std::endl;
#endif
    nodes[NID(i)] = cppn._inputs[i] = std::make_shared<INode>();
  }

  cppn._outputs.resize(cppn_g.outputs);
  for (uint i=0; i<cppn_g.outputs; i++) {
#ifdef DEBUG
    std::cerr << "(O) " << NID(i+cppn_g.inputs) << " " << i << " "
              << ofuncs[i] << std::endl;
#endif
    nodes[NID(i+cppn_g.inputs)] = cppn._outputs[i] = fnode(ofuncs[i]);
  }

  uint i=0;
  cppn._hidden.resize(cppn_g.nodes.size());
  for (const CPPN_g::Node &n_g: cppn_g.nodes) {
#ifdef DEBUG
    std::cerr << "(H) " << n_g.id << " " << i << " " << n_g.func << std::endl;
#endif
    nodes[n_g.id] = cppn._hidden[i++] = fnode(n_g.func);
  }

  for (const CPPN_g::Link &l_g: cppn_g.links) {
    FNode &n = dynamic_cast<FNode&>(*nodes.at(l_g.nid_dst));
    n.links.push_back({l_g.weight, nodes.at(l_g.nid_src)});
  }

  return cppn;
}

float CPPN::INode::value (void) {
  utils::IndentingOStreambuf indent (std::cout);
#ifdef DEBUG
  std::cout << "I: " << data << std::endl;
#endif
  return data;
}

float CPPN::FNode::value (void) {
#ifdef DEBUG
  utils::IndentingOStreambuf indent (std::cout);
  std::cout << "F:\n";
#endif
  if (std::isnan(data)) {
    data = 0;
    for (Link &l: links)
      data += l.weight * l.node->value();
#ifdef DEBUG
    std::cout << func(data) << " = func(" << data << ")\n";
#endif
    data = func(data);
  }
#ifdef DEBUG
  std::cout << data << "\n";
#endif
  return data;
}

std::ostream& operator<< (std::ostream &os, const std::vector<float> &v) {
  os << "[";
  for (float f: v)  os << " " << f;
  return os << " ]";
}

void CPPN::operator() (const Inputs &inputs, Outputs &outputs) const {
  bool bias = ((_inputs.size() % 2) == 1);
  assert(inputs.size() == _inputs.size()-bias);
  assert(outputs.size() == _outputs.size());

//  utils::IndentingOStreambuf indent (std::cout);
//  std::cout << "compute step\n" << inputs << std::endl;

  for (uint i=0; i<inputs.size(); i++)  _inputs[i]->data = inputs[i];
  if (bias) _inputs.back()->data = 1;

  for (auto &n: _hidden)  n->data = NAN;
  for (auto &n: _outputs)  n->data = NAN;

  for (uint i=0; i<outputs.size(); i++) outputs[i] = _outputs[i]->value();

//  std::cout << outputs << "\n" << std::endl;
}

} // end of namespace phenotype
