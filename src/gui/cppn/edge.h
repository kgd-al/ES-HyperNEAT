#ifndef KGD_CPPN_GRAPHIC_EDGE_H
#define KGD_CPPN_GRAPHIC_EDGE_H

#include <QGraphicsObject>

#include "../gvcqtinterface.h"

namespace kgd::es_hyperneat::gui::cppn {

class Edge : public QGraphicsObject {
  Q_OBJECT

public:
  Edge (Agedge_t *edge);

  QRectF boundingRect(void) const override {    return _bounds; }

  void paint (QPainter *painter, const QStyleOptionGraphicsItem*,
              QWidget*) override;

private:
#ifndef NDEBUG
  QString _name;
#endif

  QRectF _bounds;
  QPainterPath _edge, _arrow;
  float _width;
  QColor _color;

  void drawShape (const splines *spl, const QPointF &offset,
                  QPainterPath &edge, QPainterPath &arrow);
};

} // end of namespace kgd::es_hyperneat::gui::cppn

#endif // KGD_CPPN_GRAPHIC_EDGE_H
