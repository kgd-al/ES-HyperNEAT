#include <QPainter>
#include <QGraphicsScene>
#include <QPalette>

#include "edge.h"

#include <iostream>
#include <QDebug>

namespace gui::ann {

QColor redBlackGradient(float v) {
  static constexpr float minAlpha = 0;
  assert(-1 <= v && v <= 1);
  QColor c = QColor(v < 0 ? Qt::red : Qt::black);
  c.setAlphaF(minAlpha + (1.f-minAlpha)*std::fabs(v));
  return c;
}

std::ostream& operator<< (std::ostream &os, const QPointF &p) {
  return os << "{" << p.x() << ", " << p.y() << "}";
}

std::ostream& operator<< (std::ostream &os, const QColor &c) {
  return os << "QColor(" << c.redF() << ", " << c.greenF() << ", " << c.blueF()
            << ", " << c.alphaF() << ")";
}

std::ostream& operator<< (std::ostream &os, const Edge &e) {
  return os << "[" << e._name.toStdString() << "] " << e._edge.pointAtPercent(0)
            << "->" << e._edge.pointAtPercent(1) << " " << e._color << " "
            << e._currentColor << " " << e._width;
}

Edge::Edge (Agedge_t *edge, qreal scale)
  : _weight(gvc::get(edge, "weight", 0.f)) {
#ifndef NDEBUG
  _name = QString(agnameof(edge));
#endif

  setHovered(false);

  const splines* spl = ED_spl(edge);
  _bounds = toQt(spl->bb, scale);
  setPos(_bounds.center());
  _bounds.translate(-_bounds.center());

  drawShape(spl, scale, pos(), _edge, _arrow);
  _width = 1+gvc::get(edge, "penwidth", 0.f);
  _currentColor = _color =
      QColor(gvc::get(edge, "color", std::string()).c_str());
  std::cerr << "EDGE: " << *this << "\n";
}

void Edge::updateAnimation(float v) {
  std::cerr << "Updated color: " << *this << "\n\t";

  if (!std::isnan(v)) { // Running
    std::cerr << "\tred-black gradient of value " << v * _weight << " = "
              << v << " * " << _weight << "\n";
    /// TODO What is the synaptic weight range ?
    _currentColor = redBlackGradient(v * _weight);

  } else
    _currentColor = _color;

  std::cerr << "\t" << _currentColor << "\n";

  update();
}

void Edge::paint (QPainter *painter, const QStyleOptionGraphicsItem*,
                      QWidget*) {

  painter->save();

    if (_hovered)
      painter->setPen(scene()->palette().color(QPalette::Highlight));
    else
      painter->setPen(_currentColor);
    std::cerr << "paint(" << *this << ")\t" << painter->pen().color() << "\n";

    QPen p = painter->pen();
    p.setWidthF(_width * p.widthF());
    p.setCapStyle(Qt::FlatCap);
    painter->setPen(p);
    painter->drawPath(_edge);

    painter->drawEllipse(_edge.pointAtPercent(.5), 1, 1);

    p.setJoinStyle(Qt::MiterJoin);
    painter->setPen(p);
    if constexpr (false)
      painter->drawPath(_arrow);
    else
      painter->fillPath(_arrow, p.brush());
  painter->restore();
}

void Edge::setHovered(bool h) {
  _hovered = h;
  setZValue(-2 + _hovered);
}

void Edge::drawShape (const splines *spl, float scale,
                          const QPointF &offset, QPainterPath &edge,
                          QPainterPath &arrow) {

  static constexpr float arrowLength = 5;
  static constexpr float arrowFolding = .25;

  auto convert = [scale, offset] (pointf p) {
    return toQt(p, scale) - offset;
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

    float a = 0, b = .25;
    QPointF cross (-axis.y()/2, axis.x()/2);
    arrow.moveTo(end-a*axis-cross);
    arrow.lineTo(end-b*axis);
    arrow.lineTo(end-a*axis+cross);
//    arrow.lineTo(end);
    arrow.lineTo(end-axis);
    arrow.closeSubpath();
  }
}

} // end of namespace gui::ann
