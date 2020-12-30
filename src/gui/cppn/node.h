#ifndef CPPN_NODE_H
#define CPPN_NODE_H

#include <QGraphicsObject>
#include <QTextDocument>

#include "../gvcqtinterface.h"

namespace gui::cppn {

class Node : public QGraphicsObject {

public:
  Node (Agnode_t *node, qreal scale);

  QRectF boundingRect(void) const override {    return _bounds; }

  void paint (QPainter *painter,
              const QStyleOptionGraphicsItem*, QWidget*) override;
private:
#ifndef NDEBUG
  QString _name;
#endif

  QRectF _bounds, _shape;

  bool _border;
  QTextDocument _label;
  const QPainterPath *_fPath;

  void drawRichText(QPainter *painter);
};

} // end of namespace gui::cppn

#endif // CPPN_NODE_H
