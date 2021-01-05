#ifndef ANN_GRAPHIC_NODE_H
#define ANN_GRAPHIC_NODE_H

#include <QGraphicsObject>
#include <QTextDocument>

#include "../gvcqtinterface.h"

namespace gui::ann {

struct Edge;
class Node : public QGraphicsObject {
  Q_OBJECT
public:
  Node (Agnode_t *node, qreal scale);

  QRectF boundingRect(void) const override {    return _bounds; }

  void paint (QPainter *painter,
              const QStyleOptionGraphicsItem*, QWidget*) override;

  void hoverEnterEvent(QGraphicsSceneHoverEvent *e) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;

  std::vector<Edge*> in, out;

signals:
  void hovered (const QPointF &p);

private:
#ifndef NDEBUG
  QString _name;
#endif

  QPointF _spos;
  QRectF _bounds, _shape;
  bool _hovered;

  QColor _fill;
  QTextDocument _label;

  bool _srecurrent;

  void drawRichText(QPainter *painter);
};

} // end of namespace gui::ann

#endif // ANN_GRAPHIC_NODE_H
