#ifndef KGD_CPPN_GENOTYPE_H
#define KGD_CPPN_GENOTYPE_H

#include "kgd/genotype/selfawaregenome.hpp"

#include "../misc/fixed_size_string.hpp"
#include "../misc/gvc_wrapper.h"

namespace genotype {

class ES_HyperNEAT : public genotype::EDNA<ES_HyperNEAT> {
  APT_EDNA()

public:

  struct CPPN : gvc::Graph {

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

    CPPN (void) : inputs(0), outputs(0) {}

#ifdef WITH_GVC
    gvc::GraphWrapper build_gvc_graph (void) const;
    void render_gvc_graph (const std::string &path) const;
#endif

    bool isInput (Node::ID nid) const;
    bool isOutput (Node::ID nid) const;
    bool isHidden (Node::ID nid) const;

    static CPPN fromDot (const std::string &data, rng::AbstractDice &dice);

    friend std::ostream& operator<< (std::ostream &os, const CPPN &d);

    friend void to_json (json &j, const CPPN &d);
    friend void from_json (const json &j, CPPN &d);

    friend bool operator== (const CPPN &lhs, const CPPN &rhs);
    friend void assertEqual(const CPPN &lhs, const CPPN &rhs, bool deepcopy);

  private:
    bool isInput (Node::ID::ut nid) const;
    bool isOutput (Node::ID::ut nid) const;
    bool isHidden (Node::ID::ut nid) const;
  } cppn;

  uint substeps;

  ES_HyperNEAT(void) {}

  std::string extension(void) const {
    return ".eshn.json";
  }

  static ES_HyperNEAT fromGenomeFile (const std::string &path);
  static ES_HyperNEAT fromDotFile (const std::string &path,
                                   rng::AbstractDice &dice);

private:
  using genotype::EDNA<ES_HyperNEAT>::fromFile;
};
DECLARE_GENOME_FIELD(ES_HyperNEAT, ES_HyperNEAT::CPPN, cppn)
DECLARE_GENOME_FIELD(ES_HyperNEAT, uint, substeps)

} // end of namespace genotype

DEFINE_NAMESPACE_SCOPED_PRETTY_ENUMERATION(
  config, CPPNInput, X0, Y0, X1, Y1, BIAS)

DEFINE_NAMESPACE_SCOPED_PRETTY_ENUMERATION(
    config, CPPNOutput, WEIGHT, LEO, BIAS)

DEFINE_NAMESPACE_SCOPED_PRETTY_ENUMERATION(
  config, CPPNInitMethod,
    PERCEPTRON, BIMODAL)

namespace config {

template <>
struct EDNA_CONFIG_FILE(ES_HyperNEAT) {
  using FuncID = genotype::ES_HyperNEAT::CPPN::Node::FuncID;
  using FunctionSet = std::set<FuncID>;
  DECLARE_PARAMETER(FunctionSet, functions)

  using FBounds = Bounds<float>;
  DECLARE_PARAMETER(FBounds, weightBounds)

  static constexpr auto INPUTS_COUNT = EnumUtils<CPPNInput>::size();
  struct InputData {
    std::string name;
  };
  using Inputs = std::array<InputData, INPUTS_COUNT>;
  DECLARE_CONST_PARAMETER(Inputs, cppnInputs)

  static constexpr auto OUTPUTS_COUNT = EnumUtils<CPPNOutput>::size();
  struct OutputData {
    std::string name;
    FuncID function;
  };
  using Outputs = std::array<OutputData, OUTPUTS_COUNT>;
  DECLARE_CONST_PARAMETER(Outputs, cppnOutputs)

  DECLARE_PARAMETER(CPPNInitMethod, cppnInit)

  DECLARE_PARAMETER(MutationRates, cppn_mutationRates)
  DECLARE_PARAMETER(DistanceWeights, cppn_distanceWeights)

  using UBounds = Bounds<uint>;
  DECLARE_PARAMETER(UBounds, substepsBounds)

  DECLARE_PARAMETER(MutationRates, mutationRates)
  DECLARE_PARAMETER(DistanceWeights, distanceWeights)
};


} // end of namespace config

#endif // KGD_CPPN_GENOTYPE_H
