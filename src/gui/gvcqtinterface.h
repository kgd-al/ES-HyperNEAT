#ifndef GVCQTINTERFACE_H
#define GVCQTINTERFACE_H

#include <QPointF>
#include <QRectF>

#include "../misc/gvc_wrapper.h"

namespace gui {

QPointF toQt (const pointf &p, float scale);
QRectF toQt (const boxf &b, float scale);

} // end of namespace gui

#endif // GVCQTINTERFACE_H
