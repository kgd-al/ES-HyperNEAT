#ifndef CPPN_GENOTYPE_H
#define CPPN_GENOTYPE_H

#include "kgd/genotype/selfawaregenome.hpp"

#include "../misc/fixed_size_string.hpp"

namespace genotype {

class CPPN : public genotype::EDNA<CPPN> {
  APT_EDNA()

public:

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

  struct Data {
    Nodes nodes;
    Links links;
    uint inputs, outputs;
    Node::ID nextNID;
    Link::ID nextLID;
  } data;

  CPPN(void);

private:
  friend std::ostream& operator<< (std::ostream &os, const Data &d);

  friend void to_json (json &j, const Data &d);
  friend void from_json (const json &j, Data &d);

  friend bool operator== (const Data &lhs, const Data &rhs);
  friend void assertEqual(const Data &lhs, const Data &rhs, bool deepcopy);
};
DECLARE_GENOME_FIELD(CPPN, CPPN::Data, data)

} // end of namespace genotype

namespace config {

template <>
struct EDNA_CONFIG_FILE(CPPN) {
  using FunctionSet = std::set<genotype::CPPN::Node::FuncID>;
  DECLARE_PARAMETER(FunctionSet, functions)
  DECLARE_PARAMETER(Bounds<float>, weightBounds)

  DECLARE_PARAMETER(int, substrateDimension)
  DECLARE_PARAMETER(bool, withBias)

  DECLARE_PARAMETER(bool, withLEO)

  DECLARE_PARAMETER(MutationRates, d_mutationRates)
  DECLARE_PARAMETER(DistanceWeights, d_distanceWeights)

  DECLARE_PARAMETER(MutationRates, mutationRates)
  DECLARE_PARAMETER(DistanceWeights, distanceWeights)
};


} // end of namespace config

#endif // CPPN_GENOTYPE_H
