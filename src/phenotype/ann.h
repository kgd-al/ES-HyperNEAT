#ifndef ANN_PHENOTYPE_H
#define ANN_PHENOTYPE_H

#include "cppn.h"

namespace phenotype {

template <uint D>
struct Point_t {
  static_assert(D > 0, "Null points do not make sense");
  static_assert(D > 1, "1D ANNs are far from fonctional");
  static_assert(D < 3, "3D ANNs are far from fonctional");

  std::array<float, D> data;
  float x (void) const {  return data[0]; }

  float y (void) const {
    static_assert(D >= 1, "Current point type is mono-dimensional");
    return data[1];
  }
};

class ANN {
public:
  using Point = Point_t<2>;

private:
  struct RangeFinder {
    float min;
  };
  struct PointCMP {
    using is_transparent = void;
    bool operator() (const Point &lhs, const Point &rhs) const {
      if (lhs.y() != rhs.y()) return lhs.y() < rhs.y();
      return lhs.x() < rhs.x();
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
#if defined(WITH_GVC) || !defined(CLUSTER_BUILD)
    enum Type { I, O, H };
    Type type;
#endif
    float value;

    using ptr = std::shared_ptr<Neuron>;
    struct Link {
      float weight;
      ptr dst;
    };
    using Links = std::vector<Link>;
    Links dests;
  };

#ifndef CLUSTER_BUILD
  const auto& neurons (void) const {  return _neurons;  }
#endif

#ifdef WITH_GVC
  gvc::GraphWrapper graphviz_build_graph (const char *ext = "png") const;
  void graphviz_render_graph(const std::string &path) const;
#endif

  using Coordinates = std::vector<Point>;
  static ANN build (const Coordinates &inputs, const Coordinates &outputs,
                    const Coordinates &hidden,
                    const genotype::ES_HyperNEAT &genome,
                    const phenotype::CPPN &cppn);
private:
  using NeuronsMap = std::map<Point, Neuron::ptr, PointCMP>;
#ifndef CLUSTER_BUILD
  NeuronsMap _neurons;
#endif
  std::vector<Neuron::ptr> _inputs, _outputs;

  ANN(void);

#if defined(WITH_GVC) || !defined(CLUSTER_BUILD)
  Neuron::ptr addNeuron (const Point &p, Neuron::Type t);
#else
  Neuron::ptr addNeuron (void);
#endif
};

} // namespace phenotype

#endif // ANN_PHENOTYPE_H
