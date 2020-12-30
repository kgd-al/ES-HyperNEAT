#include "gvcqtinterface.h"

namespace gui {

QPointF toQt (const pointf &p, float scale) {
    return QPointF(p.x * scale, -p.y * scale);
}

QRectF toQt (const boxf &b, float scale) {
  return QRectF (
      b.LL.x * scale, -b.UR.y * scale,
      (b.UR.x - b.LL.x) * scale, (b.UR.y - b.LL.y) * scale
  );
}

}
