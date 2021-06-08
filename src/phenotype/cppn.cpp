#include "cppn.h"

namespace phenotype {

#ifndef NDEBUG
//#define DEBUG
#endif

float ssgn(float x) {
  static constexpr float a = 1;
  return x < -a ? std::exp(-(x+a)*(x+a))-1
                : x > a ? 1 - std::exp(-(x-a)*(x-a))
                        : 0;
}

#define F(NAME, BODY) \
 { NAME, [] (float x) -> float { return BODY; } }
const std::map<genotype::ES_HyperNEAT::CPPN::Node::FuncID,
               CPPN::Function> CPPN::functions {

  // Function set from Risi
//  F("line", std::fabs(x)),  // Described as "linear"
//  F("gaus", 2*std::exp(-std::pow(2.5*x, 2))-1), // Negative gaussian
////  F(  "id", x), // None
//  F("ssgm", 1.f / (1.f + std::exp(-4.9*x))),  // Steepened sigmoid
//  F("bsgm", 2.f / (1.f + std::exp(-4.9*x)) - 1.f),  // Bipolar sigmoid
//  F( "sin", std::sin(2*x)),
//  F("step", x < 0 ? 0 : 1),

  // Personal take on the function set
  F( "abs", std::fabs(x)),
  F("gaus", std::exp(-6.25*x*x)), // Same steepness as Risi's but positive
  F(  "id", x),
  F("ssgm", 1.f / (1.f + std::exp(-4.9*x))),
  F("bsgm", 2.f / (1.f + std::exp(-4.9*x)) - 1.f),
  F( "sin", std::sin(2*x)),
  F("step", x <= 0 ? 0 : 1),

  // Custom-made activation function
  // kact(-inf) = 0, kact(0) = 0, kact(+inf) = 1
  // kact'(0^-) = kact'(0^+) = 2
  F("kact", x < 0 ? 4.f * x / (1.f + std::exp(-4*x)) : std::tanh(2*x)),

  // Another custom-made activation function
  F("ssgn", ssgn(x))
};
#undef F

#define F(NAME, MIN, MAX) { NAME, { MIN, MAX }}
const std::map<genotype::ES_HyperNEAT::CPPN::Node::FuncID,
               CPPN::Range> CPPN::functionRanges {

  // Risi function set bounds
//  F("line",  0, 1),
//  F("gaus", -1, 1),
//  F("ssgm",  0, 1),
//  F("bsgm", -1, 1),
//  F( "sin", -1, 1),

  F( "abs",  0, 1),
  F("gaus",  0, 1),
  F(  "id", -1, 1),
  F("ssgm",  0, 1),
  F("bsgm", -1, 1),
  F( "sin", -1, 1),
  F("step",  0, 1),

  F("kact", -1, 1), // Not really (min value ~ .278)
};
#undef F

CPPN::CPPN(){}

CPPN CPPN::fromGenotype(const genotype::ES_HyperNEAT &es_hyperneat) {
  using CPPN_g = genotype::ES_HyperNEAT::CPPN;
  const CPPN_g &cppn_g = es_hyperneat.cppn;
  using NID = CPPN_g::Node::ID;

  const auto &ofuncs = [] (uint index) {
    return genotype::ES_HyperNEAT::config_t::cppnOutputFuncs()[index];
  };

  CPPN cppn;

  auto fnode = [] (const CPPN_g::Node::FuncID &fid) {
    return std::make_shared<FNode>(functions.at(fid));
  };

  std::map<NID, Node_ptr> nodes;

  cppn._inputs.resize(CPPN_g::INPUTS);
  for (uint i=0; i<CPPN_g::INPUTS; i++) {
#ifdef DEBUG
    std::cerr << "(I) " << NID(i) << " " << i << std::endl;
#endif
    nodes[NID(i)] = cppn._inputs[i] = std::make_shared<INode>();
  }

  cppn._outputs.resize(CPPN_g::OUTPUTS);
  for (uint i=0; i<CPPN_g::OUTPUTS; i++) {
#ifdef DEBUG
    std::cerr << "(O) " << NID(i+cppn_g.inputs) << " " << i << " "
              << ofuncs[i] << std::endl;
#endif
    nodes[NID(i+CPPN_g::INPUTS)] = cppn._outputs[i] = fnode(ofuncs(i));
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
#ifdef DEBUG
  utils::IndentingOStreambuf indent (std::cout);
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
      data += l.weight * l.node.lock()->value();
#ifdef DEBUG
    std::cout << func(data) << " = func(" << data << ")\n";
#endif
    data = func(data);

#ifdef DEBUG
  } else
    std::cout << data << "\n";
#else
  }
#endif
  return data;
}

std::ostream& operator<< (std::ostream &os, const std::vector<float> &v) {
  os << "[";
  for (float f: v)  os << " " << f;
  return os << " ]";
}

void CPPN::pre_evaluation(const Point &src, const Point &dst) const {
  /// TODO Should replace this test by a constexpr enum check
  #ifdef DEBUG
  utils::IndentingOStreambuf indent (std::cout);
  std::cout << "compute step\n" << inputs << std::endl;
  #endif

  static constexpr auto N = SUBSTRATE_DIMENSION;
  for (uint i=0; i<N; i++)  _inputs[i]->data = src.get(i);
  for (uint i=0; i<N; i++)  _inputs[i+N]->data = dst.get(i);

#if CPPN_WITH_DISTANCE
  static const float norm = 2*std::sqrt(2);
  _inputs[2*N]->data = (src - dst).length() / norm;
#endif

  _inputs.back()->data = 1;

  for (auto &n: _hidden)  n->data = NAN;
  for (auto &n: _outputs)  n->data = NAN;
}

void CPPN::operator() (const Point &src, const Point &dst,
                       Outputs &outputs) const {
  assert(outputs.size() == _outputs.size());

  pre_evaluation(src, dst);
  for (uint i=0; i<outputs.size(); i++) outputs[i] = _outputs[i]->value();

#ifdef DEBUG
  std::cout << outputs << "\n" << std::endl;
#endif
}

void CPPN::operator() (const Point &src, const Point &dst, Outputs &outputs,
                       const OutputSubset &oset) const {
  assert(outputs.size() == _outputs.size());
  assert(oset.size() <= _outputs.size());

  pre_evaluation(src, dst);
  for (auto o: oset) outputs[uint(o)] = _outputs[uint(o)]->value();

#ifdef DEBUG
  std::cout << outputs << "\n" << std::endl;
#endif
}

float CPPN::operator() (const Point &src, const Point &dst,
                        genotype::cppn::Output o) const {
  pre_evaluation(src, dst);
  return _outputs[uint(o)]->value();
}

} // end of namespace phenotype
