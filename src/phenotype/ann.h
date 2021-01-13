#ifndef ANN_PHENOTYPE_H
#define ANN_PHENOTYPE_H

#include "cppn.h"

namespace phenotype {

template <uint DIMENSIONS, uint DECIMALS>
class Point_t {
  static_assert(DIMENSIONS > 0, "Null points do not make sense");
  static_assert(DIMENSIONS > 1, "1D ANNs are far from fonctional");
  static_assert(DIMENSIONS < 3, "3D ANNs are far from fonctional");

  static constexpr uint MAX_DECIMALS = std::numeric_limits<int>::digits10-1;
  static_assert(DECIMALS <= MAX_DECIMALS, "Cannot represent such precision in"
                                          " fixed point type");

  static constexpr int RATIO = std::pow(10, DECIMALS);
  std::array<int, DIMENSIONS> _data;

  float get (uint i) const {
    return _data[i] / float(RATIO);
  }

  void set (uint i, float v) {
    _data[i] = std::round(RATIO * v);
  }

public:
  Point_t(std::initializer_list<float> &&v) {
    for (uint i=0; i<v.size(); i++) set(i, v[i]);
  }

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
    os << "{";
    for (auto c: p.data)  os << " " << c;
    return os << " }";
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
#ifndef CLUSTER_BUILD
    Point pos;
#endif
    enum Type { B, I, O, H };
    Type type;

    float value;

    using ptr = std::shared_ptr<Neuron>;
    using wptr = std::weak_ptr<Neuron>;
    struct Link {
      float weight;
      wptr in;
    };
    using Links = std::vector<Link>;
    Links links;

    bool isInput (void) const {   return type == B || type == I;  }
    bool isOutput (void) const {  return type == O; }
    bool isHidden (void) const {  return type == H; }
  };

  const auto& neurons (void) const {  return _neurons;  }

#ifdef WITH_GVC
  gvc::GraphWrapper build_gvc_graph (const char *ext = "png") const;
  void render_gvc_graph(const std::string &path) const;
#endif

  using Inputs = std::vector<float>;
  auto inputs (void) {  return Inputs(_inputs.size(), 0);  }

  using Outputs = Inputs;
  auto outputs (void) { return Outputs(_outputs.size(), 0);  }

  void reset (void);

  void operator() (const Inputs &inputs, Outputs &outputs, uint substeps);

  using Coordinates = std::vector<Point>;
  static ANN build (const Point &bias, const Coordinates &inputs,
                    const Coordinates &outputs,
                    const genotype::ES_HyperNEAT &genome,
                    const phenotype::CPPN &cppn);
private:
  using NeuronsMap = std::map<Point, Neuron::ptr, PointCMP>;
  NeuronsMap _neurons;

  std::vector<Neuron::ptr> _inputs, _outputs;
  bool _hasBias;

  ANN(void);

#if defined(WITH_GVC) || !defined(CLUSTER_BUILD)
  Neuron::ptr addNeuron (const Point &p, Neuron::Type t);
#else
  Neuron::ptr addNeuron (void);
#endif
};

} // namespace phenotype

#endif // ANN_PHENOTYPE_H
