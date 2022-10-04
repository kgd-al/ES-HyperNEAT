#include <cmath>

#include "cppn.h"

namespace phenotype {

#ifndef NDEBUG
//#define DEBUG
#endif

// =============================================================================
// -- Ugly solution to numerical non-determinism
// Divergences observed on local/remote machines can be solved by requesting
//  value in double precision (accurate within 1ULP?) and truncating back to
//  float. Computationally more expensive but ensures bitwise identical CPPN/ANN

float fd_exp(float x) {
  return static_cast<double(*)(double)>(std::exp)(x);
}

float fd_sin(float x) {
  return static_cast<double(*)(double)>(std::sin)(x);
}

#define KGD_EXP fd_exp
#define KGD_EXP_STR "dexp"

// =============================================================================

float ssgn(float x) {
  static constexpr float a = 1;
  return x < -a ? KGD_EXP(-(x+a)*(x+a))-1
                : x > a ? 1 - KGD_EXP(-(x-a)*(x-a))
                        : 0;
}

float act2(float x) {
  return x <= 0 ? 0
                : 1 - KGD_EXP(
                        -x*x /* / (2*b*b) */
                      );
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
  F("gaus", KGD_EXP(-6.25f*x*x)), // Same steepness as Risi's but positive
  F(  "id", x),
  F("ssgm", 1.f / (1.f + KGD_EXP(-4.9f*x))),
  F("bsgm", 2.f / (1.f + KGD_EXP(-4.9f*x)) - 1.f),
  F( "sin", fd_sin(2.f*x)),
  F("step", x <= 0.f ? 0.f : 1.f),

  // Custom-made activation function
  // kact(-inf) = 0, kact(0) = 0, kact(+inf) = 1
  // kact'(0^-) = kact'(0^+) = 2
//  F("kact", x < 0 ? 4.f * x / (1.f + std::exp(-4*x)) : std::tanh(2*x)),

  // Another custom-made activation function
  F("ssgn", ssgn(x)),

  // Positive activation function
  // forall x <=0, act2(x) = 0
  // lim x -> inf act2(x) = 1
//  F("act2", act2(x)),
};
#undef F

template <typename K, typename V>
std::map<V, K> reverse (const std::map<K, V> &m) {
  std::map<V, K> m_;
  for (const auto &p: m) m_.emplace(p.second, p.first);
  return m_;
}

const std::map<CPPN::Function, genotype::ES_HyperNEAT::CPPN::Node::FuncID>
  CPPN::functionToName = reverse(CPPN::functions);


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

//  F("kact", -1, 1), // Not really (min value ~ .278)
};
#undef F

CPPN::CPPN (void){}

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
    std::cerr << "(O) " << NID(i+CPPN_g::INPUTS) << " " << i << " "
              << ofuncs(i) << std::endl;
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

#ifdef DEBUG
  i=0;
  std::map<Node_ptr, uint> map;
  printf("Built CPPN:\n");
  for (const auto &v: {cppn._inputs, cppn._outputs, cppn._hidden})
    for (const Node_ptr &n: v)
      map[n] = i++;

  for (const auto &v: {cppn._hidden, cppn._outputs}) {
    for (const Node_ptr &n: v) {
      FNode &fn = *static_cast<FNode*>(n.get());
      printf("\t[%d]\n", map.at(n));
      std::string fname (functionToName.at(fn.func));
      printf("\t\t%s\n", fname.c_str());
      for (const Link &l: fn.links)
        printf("\t\t[%d]\t%g %a\n", map.at(l.node.lock()), l.weight, l.weight);
    }
  }
#endif

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
    auto val = func(data);
    std::cout << val << " = " << functionToName.at(func)
              << "(" << data << ")\n";
    data = val;
#else
    data = func(data);
#endif

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
  static constexpr auto N = DIMENSIONS;
  for (uint i=0; i<N; i++)  _inputs[i]->data = src.get(i);
  for (uint i=0; i<N; i++)  _inputs[i+N]->data = dst.get(i);

#if ESHN_WITH_DISTANCE
  static const float norm = 2*std::sqrt(2);
  _inputs[2*N]->data = (src - dst).length() / norm;
#endif

  _inputs.back()->data = 1;

  for (auto &n: _hidden)  n->data = NAN;
  for (auto &n: _outputs)  n->data = NAN;

#ifdef DEBUG
  utils::IndentingOStreambuf indent (std::cout);
  std::cout << "compute step\n\tInputs:"
            << std::setprecision(std::numeric_limits<float>::max_digits10);
  for (auto &i: _inputs) std::cout << " " << i->data;
  std::cout << "\n";
#endif
}

void CPPN::operator() (const Point &src, const Point &dst,
                       Outputs &outputs) const {
  assert(outputs.size() == _outputs.size());

  pre_evaluation(src, dst);
  for (uint i=0; i<outputs.size(); i++) outputs[i] = _outputs[i]->value();

#ifdef DEBUG
  using utils::operator<<;
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
  using utils::operator<<;
  std::cout << outputs << "\n" << std::endl;
#endif
}

float CPPN::operator() (const Point &src, const Point &dst,
                        genotype::cppn::Output o) const {
  pre_evaluation(src, dst);
  return _outputs[uint(o)]->value();
}

} // end of namespace phenotype
