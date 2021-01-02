#ifndef CPPN_PHENOTYPE_H
#define CPPN_PHENOTYPE_H

#include "../genotype/es-hyperneat.h"

namespace phenotype {

class CPPN {
public:
  using FuncID = genotype::ES_HyperNEAT::CPPN::Node::FuncID;
  using Function = float (*) (float);
  static const std::map<FuncID, CPPN::Function> functions;

  struct Range { float min, max; };
  static const std::map<FuncID, Range> functionRanges;

private:
  struct Node_base {
    float data;

    virtual float value (void) = 0;
  };
  using Node_ptr = std::shared_ptr<Node_base>;

  struct INode : public Node_base {
    float value (void) override;
  };

  struct Link {
    float weight;
    Node_ptr node;
  };

  struct FNode : public Node_base {
    float value (void) override;

    const Function func;

    std::vector<Link> links;

    FNode (Function f) : func(f) {}
  };

  std::vector<Node_ptr> _inputs, _hidden, _outputs;

  CPPN(void);

public:
  static CPPN fromGenotype (const genotype::ES_HyperNEAT &es_hyperneat);

  auto inputs (void) const {  return Inputs((_inputs.size()>>2)<<2); }
  auto outputs (void) const {  return Outputs(_outputs.size()); }

  auto inputSize (void) const { return _inputs.size();  }
  auto outputSize (void) const {  return _outputs.size();  }

  using Inputs = std::vector<float>;
  using Outputs = Inputs;
  void operator() (const Inputs &inputs, Outputs &outputs) const;
};

} // end of namespace phenotype

#endif // CPPN_PHENOTYPE_H
