#ifndef KGD_ANN_PHENOTYPE_H
#define KGD_ANN_PHENOTYPE_H

#include "cppn.h"

//#define DEBUG_QUADTREE
#ifdef DEBUG_QUADTREE
namespace quadtree_debug {
const stdfs::path& debugFilePrefix (const stdfs::path &path = "");
}
#endif

namespace phenotype {

class ANN : public gvc::Graph {
  struct RangeFinder {
    float min;
  };
  struct PointCMP {
    using is_transparent = void;
    bool operator() (const Point &lhs, const Point &rhs) const {
      return lhs < rhs;
    }

    bool operator() (const Point &lhs, const RangeFinder &rhs) const {
      return lhs.y() < rhs.min;
    }

    bool operator() (const RangeFinder &lhs, const Point &rhs) const {
      return lhs.min < rhs.y();
    }
  };

public:
  static constexpr auto DIMENSIONS = CPPN::DIMENSIONS;

  struct Neuron {
    const Point pos;
    enum Type { I, O, H };
    const Type type;

    const float bias;
    float value;

    // For clustering purposes (bitwise mask)
    using Flags_t = uint;
    Flags_t flags;

    Neuron (const Point &p, Type t, float b)
      : pos(p), type(t), bias(b), value(0), flags(0) {}

    bool isInput (void) const {   return type == I;  }
    bool isOutput (void) const {  return type == O; }
    bool isHidden (void) const {  return type == H; }

    void reset (void) { value = 0;  }

    using ptr = std::shared_ptr<Neuron>;
    using wptr = std::weak_ptr<Neuron>;
    struct Link {
      float weight;
      wptr in;
    };
    using Links = std::vector<Link>;

    const Links& links (void) const {   return _links;            }
    Links& links (void) {   return _links;            }

    void addLink (float w, wptr n) {    _links.push_back({w,n});  }

    friend void assertEqual (const Neuron &lhs, const Neuron &rhs,
                             bool deepcopy);

    friend void assertEqual (const Link &lhs, const Link &rhs, bool deepcopy);

  private:
    Links _links;
  };

  ANN(void) = default;

  const auto& neurons (void) const {  return _neurons;  }
  auto& neurons (void) {  return _neurons;  }

#ifdef WITH_GVC
  gvc::GraphWrapper build_gvc_graph (void) const;
  void render_gvc_graph(const std::string &path) const;
#endif

  using Inputs = std::vector<float>;
  auto inputs (void) {  return Inputs(_inputs.size(), 0);  }

  using Outputs = Inputs;
  auto outputs (void) { return Outputs(_outputs.size(), 0);  }

  void reset (void);

  void operator() (const Inputs &inputs, Outputs &outputs, uint substeps);

  bool empty (void) const;

  using Coordinates = std::vector<Point>;
  static ANN build (const Coordinates &inputs,
                    const Coordinates &outputs, const phenotype::CPPN &cppn);

  friend void to_json (nlohmann::json &j, const ANN &ann);
  friend void from_json (const nlohmann::json &j, ANN &ann);

  friend void assertEqual (const ANN &lhs, const ANN &rhs, bool deepcopy);

private:
  using NeuronsMap = std::map<Point, Neuron::ptr, PointCMP>;
  NeuronsMap _neurons;

  std::vector<Neuron::ptr> _inputs, _outputs;

  Neuron::ptr addNeuron (const Point &p, Neuron::Type t, float bias);
};

struct ModularANN : public gvc::Graph {
  using Neuron = ANN::Neuron;
  struct Module {
    Point center, bl, ur;
    struct Size { float w, h; } size;
    std::vector<std::reference_wrapper<const Neuron>> neurons;
    bool recurrent;

    struct Link {
      float weight;
      Module *in;
    };

    struct Value {
      float mean, stddev;
    } valueCache;

    using Links = std::vector<Link>;
    Links links;

    Neuron::Flags_t flags;

    Module (Neuron::Flags_t f) : recurrent(false), flags(f) {}

    bool isModule (void) const {
      return neurons.size() > 1;
    }

    Neuron::Type type (void) const {
      return neurons.front().get().type;
    }

    Value value (void) const {
      return valueCache;
    }

    void update (void);
  };

  ModularANN (const ANN &ann, bool withDepth = false);
  ~ModularANN (void);

  Module* module (const Point &p) { return _components.at(p); }
  const Module* module (const Point &p) const { return _components.at(p); }

  const auto& modules (void) const {
    return _components;
  }

#ifdef WITH_GVC
  gvc::GraphWrapper build_gvc_graph (void) const;
  void render_gvc_graph(const std::string &path) const;
#endif

private:
  const ANN &_ann;
  std::map<Point, Module*> _components;
};

} // namespace phenotype

namespace config {

struct CONFIG_FILE(EvolvableSubstrate) {
  DECLARE_PARAMETER(uint, initialDepth)
  DECLARE_PARAMETER(uint, maxDepth)
  DECLARE_PARAMETER(uint, iterations)

  DECLARE_PARAMETER(float, divThr)  // division
  DECLARE_PARAMETER(float, varThr)  // variance
  DECLARE_PARAMETER(float, bndThr)  // band-pruning

  DECLARE_PARAMETER(float, weightRange)
  DECLARE_CONST_PARAMETER(genotype::ES_HyperNEAT::CPPN::Node::FuncID,
                          activationFunc)

  DECLARE_SUBCONFIG(genotype::ES_HyperNEAT::config_t, configGenotype)
};

} // end of namespace config

#endif // KGD_ANN_PHENOTYPE_H
