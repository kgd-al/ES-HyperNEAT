#ifndef KGD_ANN_GRAPHIC_EDGE_H
#define KGD_ANN_GRAPHIC_EDGE_H

#include <QGraphicsObject>

#include "../gvcqtinterface.h"
#include "../config.h"

namespace kgd::es_hyperneat::gui::ann {

#ifndef NDEBUG
std::ostream& operator<< (std::ostream &os, const QPointF &p);
std::ostream& operator<< (std::ostream &os, const QColor &c);
#endif

struct Node;
class Edge : public QGraphicsObject {
  Q_OBJECT

public:
  Edge (Agedge_t *edge, qreal scale);
  void updateIO (Node *i, Node *o);

  QRectF boundingRect(void) const override {    return _bounds; }

  void paint (QPainter *painter, const QStyleOptionGraphicsItem*,
              QWidget*) override;

  void updateAnimation (float v);
  void setHovered (bool h);

  void updateCustomColor (void);
  void clearCustomColor (void);

  const Node* input (void) const {  return in;  }
  const Node* output (void) const { return out; }

#ifndef NDEBUG
  friend std::ostream& operator<< (std::ostream &os, const Edge &e);
#endif

private:
#ifndef NDEBUG
  QString _name;
#endif

  Node *in, *out;

  QRectF _bounds;
  QPainterPath _edge, _arrow;
  float _width;
  QColor _color, _currentColor;
  config::ESHNGui::CustomColors _customColors;

  const float _weight;  /// TODO Change when implementing learning
  bool _hovered;

  void drawShape (const splines *spl, float scale, const QPointF &offset,
                  QPainterPath &edge, QPainterPath &arrow);
};

} // end of namespace kgd::es_hyperneat::gui::ann

#endif // KGD_ANN_GRAPHIC_EDGE_H
