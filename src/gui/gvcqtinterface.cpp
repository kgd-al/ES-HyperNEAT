#include "gvcqtinterface.h"

namespace kgd::gui {

QPointF toQt (const pointf &p) {
  return QPointF(p.x, -p.y);
}

QRectF toQt (const boxf &b) {
  return QRectF (b.LL.x, -b.UR.y, (b.UR.x - b.LL.x), (b.UR.y - b.LL.y));
}

QColor redBlackGradient(float v) {
  static constexpr float minAlpha = 0;
  assert(-1 <= v && v <= 1);
  QColor c = QColor(v < 0 ? Qt::red : Qt::black);
  c.setAlphaF(minAlpha + (1.f-minAlpha)*std::fabs(v));
  return c;
}

} // end of namespace kgd::gui
