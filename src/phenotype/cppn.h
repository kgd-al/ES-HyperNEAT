#ifndef KGD_CPPN_PHENOTYPE_H
#define KGD_CPPN_PHENOTYPE_H

#include "../genotype/es-hyperneat.h"
#include "point.hpp"

namespace phenotype {

class CPPN {
public:
  static constexpr auto DIMENSIONS = Point::DIMENSIONS;

  using FuncID = genotype::ES_HyperNEAT::CPPN::Node::FuncID;
  using Function = float (*) (float);
  static const std::map<FuncID, CPPN::Function> functions;
  static const std::map<CPPN::Function, FuncID> functionToName;

  struct Range { float min, max; };
  static const std::map<FuncID, Range> functionRanges;

private:
  struct Node_base {
    float data;

    virtual float value (void) = 0;
  };
  using Node_ptr = std::shared_ptr<Node_base>;
  using Node_wptr = std::weak_ptr<Node_base>;

  struct INode : public Node_base {
    float value (void) override;
  };

  struct Link {
    float weight;
    Node_wptr node;
  };

  struct FNode : public Node_base {
    float value (void) override;

    const Function func;

    std::vector<Link> links;

    FNode (Function f) : func(f) {}
  };

  std::vector<Node_ptr> _inputs, _hidden, _outputs;

public:
  CPPN(void);

  static CPPN fromGenotype (const genotype::ES_HyperNEAT &es_hyperneat);

  auto inputSize (void) const { return _inputs.size();  }
  auto outputSize (void) const {  return _outputs.size();  }

  using Outputs = std::array<float, genotype::ES_HyperNEAT::CPPN::OUTPUTS>;

  void operator() (const Point &src, const Point &dst, Outputs &outputs) const;

  float operator() (const Point &src, const Point &dst,
                    genotype::cppn::Output o) const;

  using OutputSubset = std::set<genotype::cppn::Output>;
  void operator() (const Point &src, const Point &dst, Outputs &outputs,
                   const OutputSubset &oset) const;

private:
  void pre_evaluation (const Point &src, const Point &dst) const;
};

} // end of namespace phenotype

#endif // KGD_CPPN_PHENOTYPE_H
