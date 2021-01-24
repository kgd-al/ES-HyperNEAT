#include <QPainter>

#include "edge.h"

namespace kgd::es_hyperneat::gui::cppn {

Edge::Edge (Agedge_t *edge, qreal scale) {
  setZValue(-1);

#ifndef NDEBUG
  _name = QString(agnameof(edge));
#endif

  const splines* spl = ED_spl(edge);
  _bounds = kgd::gui::toQt(spl->bb, scale);
  setPos(_bounds.center());
  _bounds.translate(-_bounds.center());

  drawShape(spl, scale, pos(), _edge, _arrow);

  _width = gvc::get(edge, "penwidth", 1.f);
  _color = QColor(gvc::get(edge, "color", std::string()).c_str());
}

void Edge::paint (QPainter *painter, const QStyleOptionGraphicsItem*,
                      QWidget*) {

  QPen p = painter->pen();
  p.setColor(_color);
  painter->setPen(p);

  painter->save();
    p.setWidthF(_width * p.widthF());
    p.setCapStyle(Qt::FlatCap);
    painter->setPen(p);
    painter->drawPath(_edge);
  painter->restore();

  painter->save();
//    p = painter->pen();
//    p.setJoinStyle(Qt::MiterJoin);
//    painter->setPen(p);
//    painter->drawPath(_arrow);
    painter->fillPath(_arrow, p.brush());
  painter->restore();
}

void Edge::drawShape (const splines *spl, float scale,
                          const QPointF &offset, QPainterPath &edge,
                          QPainterPath &arrow) {

  static constexpr float arrowLength = 7.5;
  static constexpr float arrowFolding = .25;

  auto convert = [scale, offset] (pointf p) {
    return kgd::gui::toQt(p, scale) - offset;
  };

  if ((spl->list != 0) && (spl->list->size%3 == 1)) {
    bezier bez = spl->list[0];
    // If there is a starting point,
    //  draw a line from it to the first curve point
    if (bez.sflag) {
      edge.moveTo(convert(bez.sp));
      edge.lineTo(convert(bez.list[0]));

    } else
      edge.moveTo(convert(bez.list[0]));

    // Loop over the curve points
    for (int i=1; i<bez.size; i+=3)
      edge.cubicTo(convert(bez.list[i]), convert(bez.list[i+1]),
                   convert(bez.list[i+2]));

    // If there is an ending point, draw a line to it
    if (bez.eflag)  edge.lineTo(convert(bez.ep));

    QPointF end = edge.currentPosition();
    QPointF axis = end - edge.pointAtPercent(.99);
    float length = sqrt(axis.x()*axis.x()+axis.y()*axis.y());
    axis = arrowLength * axis / length;
    end -= .05 * axis;

    QPointF lastPoint = edge.elementAt(edge.elementCount()-1);
    lastPoint -= (1-arrowFolding)*axis;
    edge.setElementPositionAt(edge.elementCount()-1,
                              lastPoint.x(), lastPoint.y());

    QPointF cross (-axis.y()/2, axis.x()/2);
    arrow.moveTo(end-axis-cross);
    arrow.lineTo(end);
    arrow.lineTo(end-axis+cross);
//    arrow.lineTo(end);
    arrow.lineTo(end-(1-arrowFolding)*axis);
    arrow.closeSubpath();
  }
}

} // end of namespace kgd::es_hyperneat::gui::cppn
