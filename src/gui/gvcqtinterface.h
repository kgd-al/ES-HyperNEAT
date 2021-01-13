#ifndef KGD_GVCQTINTERFACE_H
#define KGD_GVCQTINTERFACE_H

#include <QPointF>
#include <QRectF>

#include "../misc/gvc_wrapper.h"

namespace kgd::gui {

QPointF toQt (const pointf &p, float scale);
QRectF toQt (const boxf &b, float scale);

} // end of namespace kgd::gui

#endif // KGD_GVCQTINTERFACE_H
