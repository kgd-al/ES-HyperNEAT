#include "cppn.h"

namespace phenotype {

#define F(NAME, BODY) \
 { NAME, [] (float x) -> float { return BODY; } }
static const std::map<genotype::ES_HyperNEAT::CPPN::Node::FuncID,
                      CPPN::Function> functionsAdresses {
  F( "abs", std::fabs(x)),
  F("sigm", 1.f / (1.f + std::exp(-x))),
  F( "sin", std::sin(x)),
  F("step", x < 0 ? 0 : 1),
  F("gaus", std::exp(-(x*x)/2)),
};
#undef F

CPPN::CPPN(){}

CPPN CPPN::fromGenotype(const genotype::ES_HyperNEAT &es_hyperneat) {
  using CPPN_g = genotype::ES_HyperNEAT::CPPN;
  const CPPN_g &cppn_g = es_hyperneat.cppn;
  using NID = CPPN_g::Node::ID;

  static const auto &ofuncs = cppn_g.outputFunctions;

  CPPN cppn;

  auto fnode = [] (const CPPN_g::Node::FuncID &fid) {
    return std::make_shared<FNode>(functionsAdresses.at(fid));
  };

  std::map<NID, Node_ptr> nodes;

  cppn._inputs.resize(cppn_g.inputs);
  for (uint i=0; i<cppn_g.inputs; i++)
    nodes[NID(i)] = cppn._inputs[i] = std::make_shared<INode>();

  cppn._outputs.resize(cppn_g.outputs);
  for (uint i=0; i<cppn_g.outputs; i++)
    nodes[NID(i+cppn_g.outputs)] = cppn._outputs[i] = fnode(ofuncs[i]);

  uint i=0;
  cppn._hidden.resize(cppn_g.nodes.size());
  for (const CPPN_g::Node &n_g: cppn_g.nodes)
    nodes[n_g.id] = cppn._hidden[i++] = fnode(n_g.func);

  return cppn;
}

float CPPN::INode::value (void) {
  return data;
}

float CPPN::FNode::value (void) {
  if (std::isnan(data)) {
    data = 0;
    for (Link &l: links)
      data += l.weight * l.node->value();
  }
  return data;
}

void CPPN::operator() (const Inputs &inputs, Outputs &outputs) {
  bool bias = ((_inputs.size() % 2) == 1);
  assert(inputs.size() == _inputs.size()-bias);
  assert(outputs.size() == _outputs.size());

  for (uint i=0; i<inputs.size(); i++)  _inputs[i]->data = inputs[i];
  if (bias) _inputs.back()->data = 1;

  for (auto &n: _hidden)  n->data = NAN;
  for (auto &n: _outputs)  n->data = NAN;

  for (uint i=0; i<outputs.size(); i++) outputs[i] = _outputs[i]->value();
}

} // end of namespace phenotype
