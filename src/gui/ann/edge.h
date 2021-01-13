#ifndef KGD_ANN_GRAPHIC_EDGE_H
#define KGD_ANN_GRAPHIC_EDGE_H

#include <QGraphicsObject>

#include "../gvcqtinterface.h"

namespace kgd::gui::ann {

QColor redBlackGradient (float v);

#ifndef NDEBUG
std::ostream& operator<< (std::ostream &os, const QPointF &p);
std::ostream& operator<< (std::ostream &os, const QColor &c);
#endif

class Edge : public QGraphicsObject {
  Q_OBJECT

public:
  Edge (Agedge_t *edge, qreal scale);

  QRectF boundingRect(void) const override {    return _bounds; }

  void paint (QPainter *painter, const QStyleOptionGraphicsItem*,
              QWidget*) override;

  void updateAnimation (float v);
  void setHovered (bool h);

#ifndef NDEBUG
  friend std::ostream& operator<< (std::ostream &os, const Edge &e);
#endif

private:
#ifndef NDEBUG
  QString _name;
#endif

  QRectF _bounds;
  QPainterPath _edge, _arrow;
  float _width;
  QColor _color, _currentColor;

  const float _weight;  /// TODO Change when implementing learning
  bool _hovered;

  void drawShape (const splines *spl, float scale, const QPointF &offset,
                  QPainterPath &edge, QPainterPath &arrow);
};

} // end of namespace kgd::gui::ann

#endif // KGD_ANN_GRAPHIC_EDGE_H
