#ifndef ANN_GRAPHIC_EDGE_H
#define ANN_GRAPHIC_EDGE_H

#include <QGraphicsObject>

#include "../gvcqtinterface.h"

namespace gui::ann {

class Edge : public QGraphicsObject {
  Q_OBJECT

public:
  Edge (Agedge_t *edge, qreal scale);

  QRectF boundingRect(void) const override {    return _bounds; }

  void paint (QPainter *painter, const QStyleOptionGraphicsItem*,
              QWidget*) override;

  void setHovered (bool h);

private:
#ifndef NDEBUG
  QString _name;
#endif

  QRectF _bounds;
  QPainterPath _edge, _arrow;
  float _width;
  QColor _color;

  bool _hovered;

  void drawShape (const splines *spl, float scale, const QPointF &offset,
                  QPainterPath &edge, QPainterPath &arrow);
};

} // end of namespace gui::ann

#endif // ANN_GRAPHIC_EDGE_H
