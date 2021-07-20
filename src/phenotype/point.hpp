#ifndef ES_HYPERNEAT_POINT_HPP
#define ES_HYPERNEAT_POINT_HPP

namespace phenotype {

template <uint DI, uint DE>
class Point_t {
  static_assert(DI > 0, "Null points do not make sense");
  static_assert(DI > 1, "1D ANNs do not make much sense, right?");
  static_assert(DI < 4, "n-D ANNs are really not a good idea");

  static constexpr uint MAX_DECIMALS = std::numeric_limits<int>::digits10-1;
  static_assert(DE <= MAX_DECIMALS,
                "Cannot represent such precision in fixed point type");

  static constexpr int RATIO = std::pow(10, DE);
  std::array<int, DI> _data;

public:
  static constexpr auto DIMENSIONS = DI;
  static constexpr auto DECIMALS = DE;

  Point_t(std::initializer_list<float> &&flist) {
    uint i=0;
    for (float f: flist) set(i++, f);
    for (; i<DIMENSIONS; i++) set(i, 0);
  }

  Point_t(void) { set(0); }

  static Point_t null (void) { return Point_t(); }

  float x (void) const {  return get(0); }

  template <uint DI_ = DI>
  std::enable_if_t<DI_ >= 2, float> y (void) const {
    return get(1);
  }

  template <uint DI_ = DI>
  std::enable_if_t<DI_ >= 3, float> z (void) const {
    return get(2);
  }

  float get (uint i) const {
    return _data[i] / float(RATIO);
  }

  void set (uint i, float v) {
    _data[i] = std::round(RATIO * v);
  }

  void set (float v) {
    for (uint i=0; i<DIMENSIONS; i++) set(i, v);
  }

  Point_t& operator+= (const Point_t &that) {
    for (uint i=0; i<DIMENSIONS; i++)
      set(i, get(i) + that.get(i));
    return *this;
  }

  Point_t& operator/= (float v) {
    for (uint i=0; i<DIMENSIONS; i++)
      set(i, get(i) / v);
    return *this;
  }

  float length (void) const {
    float sum = 0;
    for (uint i=0; i<DIMENSIONS; i++)
      sum += get(i)*get(i);
    return std::sqrt(sum);
  }

  friend Point_t operator- (const Point_t &lhs, const Point_t &rhs) {
    Point_t res;
    for (uint i=0; i<DIMENSIONS; i++)
      res.set(i, lhs.get(i) - rhs.get(i));
    return res;
  }

  friend bool operator< (const Point_t &lhs, const Point_t &rhs) {
    /// TODO Restore
    return lhs._data < rhs._data;
    /// TODO Remove
//    if (lhs.y() != rhs.y()) return lhs.y() < rhs.y();
//    return lhs.x() < rhs.x();
  }

  friend bool operator== (const Point_t &lhs, const Point_t &rhs) {
    return lhs._data == rhs._data;
  }

  friend bool operator!= (const Point_t &lhs, const Point_t &rhs) {
    return lhs._data != rhs._data;
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
using Point = Point_t<ESHN_SUBSTRATE_DIMENSION, 3>;

} // end of namespace phenotype

#endif // ES_HYPERNEAT_POINT_HPP
