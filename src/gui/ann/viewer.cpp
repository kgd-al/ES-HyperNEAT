#include <QtPrintSupport/QPrinter>

#include "viewer.h"

#include "node.h"
#include "edge.h"

#include <QDebug>

namespace gui::ann {

struct Axis : public QGraphicsRectItem {
  Axis (const QRectF &bounds) : QGraphicsRectItem(bounds) {
    setZValue(-10);
  }

  void paint (QPainter *painter, const QStyleOptionGraphicsItem *option,
              QWidget *widget) override {

    qreal x, y, w, h;
    boundingRect().getRect(&x, &y, &w, &h);

    painter->save();
      QPen p = painter->pen();
      p.setColor(Qt::darkGray);
      p.setStyle(Qt::DashLine);
      p.setWidthF(.5*p.widthF());
      painter->setPen(p);
      painter->drawLine(QPointF(x, y+h/2), QPointF(x+w, y+h/2));
      painter->drawLine(QPointF(x+w/2, y), QPointF(x+w/2, y+h));

      p.setStyle(Qt::DotLine);
      p.setWidthF(.5*p.widthF());
      painter->setPen(p);
      painter->drawLine(QPointF(x, y+1*h/4), QPointF(x+w, y+1*h/4));
      painter->drawLine(QPointF(x+1*w/4, y), QPointF(x+1*w/4, y+h));
      painter->drawLine(QPointF(x, y+3*h/4), QPointF(x+w, y+3*h/4));
      painter->drawLine(QPointF(x+3*w/4, y), QPointF(x+3*w/4, y+h));

    painter->restore();

    QGraphicsRectItem::paint(painter, option, widget);
  }
};

Viewer::Viewer(QWidget *parent) : GraphViewer(parent, "ANN") {
  setCursor(Qt::CrossCursor);
  _animating = false;
}

void Viewer::processGraph(const gvc::Graph &g, const gvc::GraphWrapper &gw) {
  std::map<std::string, Edge*> edges;

  auto scene = this->scene();
  auto gvc = gw.graph;
  qreal s = gvc::get(gvc, "dpi", 96.0) / 72.;

  const auto getOrNew = [&edges, &scene, s] (Agedge_t *e) {
    auto ename = std::string(agnameof(e));
    auto it = edges.find(ename);

    if (it != edges.end())
      return it->second;
    else {
      auto qe = new Edge(e, s);
      scene->addItem(qe);
      edges[ename] = qe;
      return qe;
    }
  };

  const auto &ann = dynamic_cast<const phenotype::ANN&>(g);
  const auto &neurons = ann.neurons();

  std::cerr << "neurons:" << std::setprecision(25);
  for (const auto &p: neurons)
    std::cerr << "\t" << p.first << "\n";
  std::cerr << "\n";

  QRectF qt_bounds (.5*INT_MAX, -.5*INT_MAX, -INT_MAX, INT_MAX), sb_bounds;

  _neurons.clear();
  for (auto *n = agfstnode(gvc); n != NULL; n = agnxtnode(gvc, n)) {
    phenotype::ANN::Point p;
    {
      auto spos_str = gvc::get(n, "spos", std::string());
      char c;
      std::istringstream (spos_str) >> p.data[0] >> c >> p.data[1];
    }

    std::cerr << "Querying " << p << "\n";
    auto qn = new Node(n, *neurons.at(p), s);
    scene->addItem(qn);
    _neurons.append(qn);
    connect(qn, &Node::hovered, this, &Viewer::neuronHovered);

    auto qn_b = qn->boundingRect().translated(qn->pos()).center();
    auto sb_p = qn->substratePosition();
    if (qn_b.x() < qt_bounds.left())
      qt_bounds.setLeft(qn_b.x()), sb_bounds.setLeft(sb_p.x());
    if (qt_bounds.right() < qn_b.x())
      qt_bounds.setRight(qn_b.x()), sb_bounds.setRight(sb_p.x());
    if (qn_b.y() < qt_bounds.bottom())
      qt_bounds.setBottom(qn_b.y()), sb_bounds.setBottom(sb_p.y());
    if (qt_bounds.top() < qn_b.y())
      qt_bounds.setTop(qn_b.y()), sb_bounds.setTop(sb_p.y());

    for (auto *e = agfstout(gvc, n); e != NULL; e = agnxtout(gvc, e)) {
      if (gvc::get(e, "style", std::string()) == "invis") continue;
      qn->out.push_back(getOrNew(e));
    }

    for (auto *e = agfstin(gvc, n); e != NULL; e = agnxtin(gvc, e)) {
      if (gvc::get(e, "style", std::string()) == "invis") continue;
      qn->in.push_back(getOrNew(e));
    }
  }

  QPointF qc = qt_bounds.center();
  float sx = qt_bounds.width() / sb_bounds.width(),
        sy = qt_bounds.height() / sb_bounds.height();
  QRectF bounds (
    qc.x() - sx * (1 + .5 * (sb_bounds.left() + sb_bounds.right())),
    qc.y() + sy * (1 + .5 * (sb_bounds.top() + sb_bounds.bottom())),
    2*sx, -2*sy);
  scene->addItem(new Axis(bounds));
}

void Viewer::startAnimation(void) {
  _animating = true;
  qDebug() << "ANN animation started -> notify";
  for (QGraphicsItem *i: _neurons)
    dynamic_cast<Node*>(i)->updateAnimation(true);
}

void Viewer::updateAnimation(void) {
  qDebug() << "ANN state changed -> update";
  for (QGraphicsItem *i: _neurons)
    dynamic_cast<Node*>(i)->updateAnimation(true);
}

void Viewer::stopAnimation(void) {
  _animating = false;
  qDebug() << "ANN animation stopped -> notify";
  for (QGraphicsItem *i: _neurons)
    dynamic_cast<Node*>(i)->updateAnimation(false);
}

} // end of namespace gui::ann
