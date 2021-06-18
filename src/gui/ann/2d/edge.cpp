#include <QPainter>
#include <QGraphicsScene>
#include <QPalette>

#include "edge.h"
#include "../../../phenotype/ann.h"

#include "node.h"
#include <iostream>
#include <QDebug>

namespace kgd::es_hyperneat::gui::ann2d {

#ifndef NDEBUG
std::ostream& operator<<(std::ostream &os, const Edge &e) {
  os << e._name.toStdString() << ": ";
  if (auto i = e.input())
    os << i->substratePosition().x() << " " << i->substratePosition().y();
  else
    os << "null";
  os << " -> ";
  if (auto o = e.output())
    os << o->substratePosition().x() << " " << o->substratePosition().y();
  else
    os << "null";
  return os;
}

std::ostream& operator<< (std::ostream &os, const QPointF &p) {
  return os << "{" << p.x() << ", " << p.y() << "}";
}

std::ostream& operator<< (std::ostream &os, const QColor &c) {
  return os << "QColor(" << c.redF() << ", " << c.greenF() << ", " << c.blueF()
            << ", " << c.alphaF() << ")";
}

//std::ostream& operator<< (std::ostream &os, const Edge &e) {
//  return os << "[" << e._name.toStdString() << "] " << e._edge.pointAtPercent(0)
//            << "->" << e._edge.pointAtPercent(1) << " " << e._color << " "
//            << e._currentColor << " " << e._width;
//}
#endif

Edge::Edge (Agedge_t *edge)
  : in(nullptr), out(nullptr), _weight(gvc::get(edge, "w", 0.f)) {
#ifndef NDEBUG
  _name = QString(agnameof(edge));
#endif

  setHovered(false);

  const splines* spl = ED_spl(edge);
  _bounds = kgd::gui::toQt(spl->bb);
  setPos(_bounds.center());
  _bounds.translate(-_bounds.center());

  drawShape(spl, pos(), _edge, _arrow);
  _width = gvc::get(edge, "penwidth", 0.f);
  _currentColor = _color =
      QColor(gvc::get(edge, "color", std::string()).c_str());
}

void Edge::updateIO(Node *i, Node *o) {
  if (i)  in = i;
  if (o)  out = o;
//  std::cerr << *this << std::endl;
}

void Edge::updateAnimation(float v) {
  static const auto& weightRange = config::EvolvableSubstrate::weightRange();
  if (!std::isnan(v))// Running
    _currentColor = kgd::gui::redBlackGradient(v * _weight / weightRange);

  else
    _currentColor = _color;

  update();
}

void Edge::updateCustomColor (void) {
  using Neuron = Node::Neuron;
  static const auto hasType = [] (const Node *n, Neuron::Type t) {
    return n->ntype() == t;
  };
//  static const auto hasCC = [] (const Node *n) {
//    return n->flags() != 0;
//  };

  updateAnimation(0);
  setZValue(-5);

  auto i = input(), o = output();
//  if (!hasCC(i) && !hasType(i, Neuron::I))  return;
//  if (!hasCC(o) && !hasType(o, Neuron::O))  return;

  decltype(phenotype::ANN::Neuron::flags) flag;
  if (hasType(i, Neuron::I)) {
    flag = o->flags();
//      qDebug() << "I -> {" << o->neuron().pos.x() << "," << o->neuron().pos.y()
//               << "}: " << ec;

  } else if (hasType(o, Neuron::O)) {
    flag = i->flags();
//      qDebug() << "{" << i->neuron().pos.x() << ","
//               << i->neuron().pos.y() << "} -> H: " << ec;

  } else {
    flag = i->flags() & o->flags();
//      qDebug() << "{" << i->neuron().pos.x() << "," << i->neuron().pos.y()
//               << "} -> {" << o->neuron().pos.x() << "," << o->neuron().pos.y()
//               << "}: " << i->customColors() << "*"
//               << o->customColors() << "=" << ec;
  }

  _customColors = config::ESHNGui::colorsForFlag (flag);
  setZValue(std::min(-1., zValue()+_customColors.size()));
  if (_customColors.isEmpty())  _customColors.push_back(QColor());
}

void Edge::clearCustomColor(void) {
  _customColors.clear();
}

void Edge::paint (QPainter *painter, const QStyleOptionGraphicsItem*,
                      QWidget*) {

  static constexpr auto dl = 2;

  painter->save();

    if (_hovered)
      painter->setPen(scene()->palette().color(QPalette::Highlight));
    else
      painter->setPen(_currentColor);

    if (!_customColors.empty()) {
      if (_customColors.size() == 1) {
        if (_customColors.front().isValid())
          painter->setPen(_customColors.front());
        else
          painter->setPen(Qt::gray);

      } else {
        QPen p = painter->pen();
        p.setDashPattern({dl, dl*(_customColors.size()-1.)});
        p.setColor(_customColors.front());
        painter->setPen(p);
      }
    }

    if (painter->pen().color().alphaF() != 0) {
      QPen p = painter->pen();
      p.setWidthF(_width * p.widthF());
      p.setCapStyle(Qt::FlatCap);

      if (_customColors.size() < 2) {
        painter->setPen(p);
        painter->drawPath(_edge);
      } else {
        for (int i=0; i<_customColors.size(); i++) {
          p.setDashOffset(i*dl);
          p.setColor(_customColors[i]);
          painter->setPen(p);
          painter->drawPath(_edge);
        }
      }

      p.setJoinStyle(Qt::MiterJoin);
      painter->setPen(p);
      if constexpr (false)
        painter->drawPath(_arrow);
      else
        painter->fillPath(_arrow, p.brush());
    }
  painter->restore();
}

void Edge::setHovered(bool h) {
  _hovered = h;
  setZValue(-2 + _hovered);
}

void Edge::drawShape (const splines *spl,
                      const QPointF &offset, QPainterPath &edge,
                      QPainterPath &arrow) {

  static constexpr float arrowLength = 5;
  static constexpr float arrowFolding = .25;

  auto convert = [offset] (pointf p) {
    return kgd::gui::toQt(p) - offset;
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

} // end of namespace kgd::es_hyperneat::gui::ann2d
