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

template <uint DIMENSIONS, uint DECIMALS>
class Point_t {
  static_assert(DIMENSIONS > 0, "Null points do not make sense");
  static_assert(DIMENSIONS > 1, "1D ANNs are far from fonctional");
  static_assert(DIMENSIONS < 3, "3D ANNs are far from fonctional");

  static constexpr uint MAX_DECIMALS = std::numeric_limits<int>::digits10-1;
  static_assert(DECIMALS <= MAX_DECIMALS,
                "Cannot represent such precision in fixed point type");

  static constexpr int RATIO = std::pow(10, DECIMALS);
  std::array<int, DIMENSIONS> _data;

  float get (uint i) const {
    return _data[i] / float(RATIO);
  }

  void set (uint i, float v) {
    _data[i] = std::round(RATIO * v);
  }

public:
  Point_t(std::initializer_list<float> &&flist) {
    uint i=0;
    for (float f: flist) set(i++, f);
  }
  Point_t(void) : Point_t{0,0} {}

  float x (void) const {  return get(0); }

  float y (void) const {
    static_assert(DIMENSIONS >= 1, "Current point type is mono-dimensional");
    return get(1);
  }

  friend bool operator< (const Point_t &lhs, const Point_t &rhs) {
    if (lhs.y() != rhs.y()) return lhs.y() < rhs.y();
    return lhs.x() < rhs.x();
  }

  friend std::ostream& operator<< (std::ostream &os, const Point_t &p) {
    os << p.get(0);
    for (uint i=1; i<DIMENSIONS; i++)  os << "," << p.get(i);
    return os;
  }

  friend std::istream& operator>> (std::istream &is, Point_t &p) {
    char c;
    float f;
    is >> f;
    p.set(0, f);
    for (uint i=1; i<DIMENSIONS; i++) {
      is >> c >> f;
      p.set(i, f);
    }
    return is;
  }

  friend void to_json (nlohmann::json &j, const Point_t &p) {
    j = { p._data };
  }

  friend void from_json (const nlohmann::json &j, Point_t &p) {
    p._data = j;
  }

  friend void assertEqual (const Point_t &lhs, const Point_t &rhs,
                           bool deepcopy) {
    using utils::assertEqual;
    assertEqual(lhs._data, rhs._data, deepcopy);
  }
};

class ANN : public gvc::Graph {
public:
  using Point = Point_t<2, 3>;

private:
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
  struct Neuron {
    const Point pos;
    enum Type { I, O, H };
    const Type type;

    const float bias;
    float value;

    Neuron (const Point &p, Type t, float b)
      : pos(p), type(t), bias(b), value(0) {}

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
    void addLink (float w, wptr n) {    _links.push_back({w,n});  }

    friend void assertEqual (const Neuron &lhs, const Neuron &rhs,
                             bool deepcopy);

    friend void assertEqual (const Link &lhs, const Link &rhs, bool deepcopy);

  private:
    Links _links;
  };

  ANN(void) = default;

  const auto& neurons (void) const {  return _neurons;  }

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
