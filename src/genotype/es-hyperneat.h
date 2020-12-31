#ifndef CPPN_GENOTYPE_H
#define CPPN_GENOTYPE_H

#include "kgd/genotype/selfawaregenome.hpp"

#include "../misc/fixed_size_string.hpp"
#include "../misc/gvc_wrapper.h"

namespace genotype {

class ES_HyperNEAT : public genotype::EDNA<ES_HyperNEAT> {
  APT_EDNA()

public:

  struct CPPN {

    template <typename T>
    struct ID_CMP {
      using ID = decltype(T::id);
      using is_transparent = void;
      bool operator() (ID lhs, ID rhs) const {  return lhs < rhs; }

      bool operator() (const T &lhs, ID rhs) const {
       return operator() (lhs.id, rhs);
      }

      bool operator() (ID lhs, const T &rhs) const {
       return operator() (lhs, rhs.id);
      }

      bool operator() (const T &lhs, const T &rhs) const {
       return operator() (lhs.id, rhs.id);
      }
    };

    struct Node {
      HAS_GENOME_ID(Node)
      using FuncID = utils::fixed_string<4>;
      FuncID func;

      Node (ID id, FuncID f) : id(id), func(f) {}
    };
    using Nodes = std::set<Node, ID_CMP<Node>>;

    struct Link {
      HAS_GENOME_ID(Link)
      Node::ID nid_src, nid_dst;
      float weight;

      Link (ID id, Node::ID in, Node::ID out, float w)
       : id(id), nid_src(in), nid_dst(out), weight(w) {}
    };
    using Links = std::set<Link, ID_CMP<Link>>;

    Nodes nodes;
    Links links;
    uint inputs, outputs;
    Node::ID nextNID;
    Link::ID nextLID;
    std::array<Node::FuncID, 2> outputFunctions;

    CPPN (void) : inputs(0), outputs(0) {}

#ifdef WITH_GVC
    gvc::GraphWrapper graphviz_build_graph (const char *ext = "png") const;
    void graphviz_render_graph (const std::string &path) const;
#endif

    static CPPN fromDot (const std::string &data);

    friend std::ostream& operator<< (std::ostream &os, const CPPN &d);

    friend void to_json (json &j, const CPPN &d);
    friend void from_json (const json &j, CPPN &d);

    friend bool operator== (const CPPN &lhs, const CPPN &rhs);
    friend void assertEqual(const CPPN &lhs, const CPPN &rhs, bool deepcopy);
  } cppn;

  ES_HyperNEAT(void) {}
};
DECLARE_GENOME_FIELD(ES_HyperNEAT, ES_HyperNEAT::CPPN, cppn)

} // end of namespace genotype

namespace config {

template <>
struct EDNA_CONFIG_FILE(ES_HyperNEAT) {
  using FuncID = genotype::ES_HyperNEAT::CPPN::Node::FuncID;
  using FunctionSet = std::set<FuncID>;
  DECLARE_PARAMETER(FunctionSet, functions)
  DECLARE_PARAMETER(Bounds<float>, weightBounds)

  DECLARE_PARAMETER(int, substrateDimension)
  DECLARE_PARAMETER(bool, withBias)

  DECLARE_PARAMETER(bool, withLEO)

  using OFunctions = decltype(genotype::ES_HyperNEAT::CPPN::outputFunctions);
  DECLARE_PARAMETER(OFunctions, outputFunctions)

  DECLARE_PARAMETER(MutationRates, cppn_mutationRates)
  DECLARE_PARAMETER(DistanceWeights, cppn_distanceWeights)

  DECLARE_PARAMETER(MutationRates, mutationRates)
  DECLARE_PARAMETER(DistanceWeights, distanceWeights)
};


} // end of namespace config

#endif // CPPN_GENOTYPE_H
