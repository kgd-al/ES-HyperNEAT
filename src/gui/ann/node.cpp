#include <QTextOption>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QGraphicsScene>

#include "node.h"
#include "edge.h"
#include "../../phenotype/cppn.h"

#include <QDebug>

namespace gui::ann {

const QTextOption TextAspect = QTextOption(Qt::AlignCenter);

const QFont BaseFont { "Courrier", 10 };
const QFont NodeFont { BaseFont.family(), BaseFont.pointSize()};
const QFont EdgeFont { BaseFont.family(), int(BaseFont.pointSize() * .8)};

static constexpr int MARGIN = 1;

Node::Node (Agnode_t *node, qreal scale) {
  setAcceptHoverEvents(true);
  _hovered = false;

  _name = QString(agnameof(node));

  auto b = ND_bb(node);
  _bounds = toQt(b, scale);
  setPos(_bounds.center());
  _bounds.translate(-_bounds.center());
  _shape = _bounds;
  _shape.adjust(+MARGIN, +MARGIN, -MARGIN, -MARGIN);

  _fill = QColor(QString::fromStdString(gvc::get(node, "fillcolor",
                                                 std::string())));

  {
    auto spos_str = gvc::get(node, "spos", std::string());
    float x, y;
    char c;
    std::istringstream (spos_str) >> x >> c >> y;
    _spos = QPointF(x, y);
  }

//  QString label (ND_label(node)->text);
//  if (!label.isEmpty()) {
//    if (label.indexOf('_') != -1) {
//      label.replace('_', "<sub>");
//      label.append("</sub>");
//    }
//    _label.setHtml(label);
//    _label.setDefaultTextOption(QTextOption(Qt::AlignCenter));

//    QFont font = NodeFont;
//    font.setPointSize(gvc::get(node, "fontsize", 10));
//    _label.setDefaultFont(font);
//  }
}


void Node::paint (QPainter *painter, const QStyleOptionGraphicsItem*,
                      QWidget*) {

  painter->save();
    if (_hovered)
      painter->setPen(scene()->palette().color(QPalette::Highlight));
    painter->setBrush(_fill);
    painter->drawEllipse(_shape);
  painter->restore();

  if (!_label.isEmpty())  drawRichText(painter);
}

void Node::drawRichText(QPainter *painter) {
  static const int flags = Qt::AlignCenter;

  QRectF rect = _bounds;
  if (fabs(_bounds.height() - _shape.height()) > MARGIN*2) {
    rect.adjust(0, _shape.height(), 0, 0);
//    painter->drawRect(rect);
  }

  _label.setPageSize(QSize(rect.width(), 1000));

  QAbstractTextDocumentLayout* layout = _label.documentLayout();

  QSizeF trect = layout->documentSize();
  const int width = qRound(trect.width());
  int x = rect.x();
  if (flags & Qt::AlignRight)
    x += (rect.width() - width);
  else if (flags & Qt::AlignHCenter)
    x += (rect.width() - width)/2;

  const int height = qRound(trect.height());
  int y = rect.y();
  if (flags & Qt::AlignBottom)
    y += (rect.height() - height);
  else if (flags & Qt::AlignVCenter)
    y += (rect.height() - height)/2;

  QAbstractTextDocumentLayout::PaintContext context;
  context.palette.setColor(QPalette::Text, painter->pen().color());

  painter->save();

  int dx = 0, dy = 0;
  painter->translate(x + dx, y + dy);
  layout->draw(painter, context);

  painter->restore();
}

void Node::hoverEnterEvent(QGraphicsSceneHoverEvent *e) {
  emit hovered(_spos);
  _hovered = true;
  for (auto e: in)  e->setHovered(true);
  for (auto e: out)  e->setHovered(true);
  QGraphicsObject::hoverEnterEvent(e);
}

void Node::hoverLeaveEvent(QGraphicsSceneHoverEvent *e) {
  _hovered = false;
  for (auto e: in)  e->setHovered(false);
  for (auto e: out)  e->setHovered(false);
  QGraphicsObject::hoverLeaveEvent(e);
}

} // end of namespace gui::ann
