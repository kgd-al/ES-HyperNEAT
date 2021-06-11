#ifndef KGD_GVCQTINTERFACE_H
#define KGD_GVCQTINTERFACE_H

#include <QPointF>
#include <QRectF>
#include <QColor>

#include "../misc/gvc_wrapper.h"

namespace kgd::gui {

QPointF toQt (const pointf &p);
QRectF toQt (const boxf &b);

QColor redBlackGradient(float v);

} // end of namespace kgd::gui

#endif // KGD_GVCQTINTERFACE_H
