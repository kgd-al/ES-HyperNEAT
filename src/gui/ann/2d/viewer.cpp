#include <QtPrintSupport/QPrinter>

#include "viewer.h"

#include "edge.h"

#include <QDebug>

namespace kgd::es_hyperneat::gui::ann2d {

struct Axis : public QGraphicsRectItem {
  static constexpr auto S = gvc::Graph::scale;
  Axis (void) : QGraphicsRectItem(-S, -S, 2*S, 2*S) {
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

void Viewer::setGraph(const phenotype::ANN &ann) {
  GraphViewer::setGraph(ann);
}

void Viewer::setGraph(const phenotype::ModularANN &ann) {
  GraphViewer::setGraph(ann);
}

struct NeuronData : public NeuralData {
  const Neuron &_neuron;
  NeuronData (const Neuron &n) : _neuron(n) {}
  const Neuron* neuron (void) const override {  return &_neuron;      }
  const Point& pos (void) const override {      return _neuron.pos;   }
  Neuron::Type type (void) const override {     return _neuron.type;  }
  Neuron::Flags_t flags (void) const override { return _neuron.flags; }
  float value (void) const override {           return _neuron.value; }
};

struct ModuleData : public NeuralData {
  using Module = phenotype::ModularANN::Module;
  const Module &_module;
  ModuleData (const Module &m) : _module(m) {}
  const Neuron* neuron (void) const override {  return nullptr;         }
  const Point& pos (void) const override {      return _module.center;  }
  Neuron::Type type (void) const override {     return _module.type();  }
  Neuron::Flags_t flags (void) const override { return _module.flags;   }
  float value (void) const override {
    return _module.value().mean;
  }
};

const char* Viewer::gvc_layout (void) const { return "nop"; }

void Viewer::processGraph(const gvc::Graph &g, const gvc::GraphWrapper &gw) {
  std::map<std::string, Edge*> edges;

  auto scene = this->scene();
  auto gvc = gw.graph;

  const auto getOrNew = [&edges, &scene] (Agedge_t *e, Node *i, Node *o) {
    auto ename = std::string(agnameof(e));
    auto it = edges.find(ename);

    Edge *qe = nullptr;
    if (it != edges.end())
      qe = it->second;
    else {
      qe = new Edge(e);
      scene->addItem(qe);
      edges[ename] = qe;
    }

    assert(qe);
    qe->updateIO(i, o);
    return qe;
  };

  std::function<NeuralData*(const phenotype::Point&)> neuralData;
  if (auto *ann = dynamic_cast<const phenotype::ANN*>(&g))
    neuralData = [ann] (auto p) {
      return new NeuronData (*ann->neuronAt(p));
    };

  else if (auto *mann = dynamic_cast<const phenotype::ModularANN*>(&g))
    neuralData = [mann] (auto p) {
      return new ModuleData (*mann->module(p));
    };

  else
    throw std::invalid_argument(
      "Provided graph is of wrong derived type. How did you do that?");

  _nodes.clear();
  for (auto *n = agfstnode(gvc); n != NULL; n = agnxtnode(gvc, n)) {
    auto p = gvc::get(n, "spos", phenotype::Point{NAN,NAN});

    auto qn = new Node(n, neuralData(p));
    scene->addItem(qn);
    _nodes.append(qn);
    connect(qn, &Node::hovered, this, &Viewer::neuronHovered);

    for (auto *e = agfstout(gvc, n); e != NULL; e = agnxtout(gvc, e))
      qn->out.push_back(getOrNew(e, qn, nullptr));

    for (auto *e = agfstin(gvc, n); e != NULL; e = agnxtin(gvc, e))
      qn->in.push_back(getOrNew(e, nullptr, qn));
  }

  scene->addItem(new Axis);
}

void Viewer::startAnimation(void) {
  _animating = true;
  for (QGraphicsItem *i: _nodes)
    dynamic_cast<Node*>(i)->updateAnimation(true);
}

void Viewer::updateAnimation(void) {
  for (QGraphicsItem *i: _nodes)
    dynamic_cast<Node*>(i)->updateAnimation(true);
}

void Viewer::stopAnimation(void) {
  _animating = false;
  for (QGraphicsItem *i: _nodes)
    dynamic_cast<Node*>(i)->updateAnimation(false);
}

void Viewer::updateCustomColors(void) {
  std::vector<Edge*> edges;
  for (QGraphicsItem *i: _nodes) {
    auto *n = dynamic_cast<Node*>(i);
    n->updateCustomColors();
    edges.insert(edges.end(), n->out.begin(), n->out.end());
  }

  for (Edge *e: edges)  e->updateCustomColor();
}

void Viewer::clearCustomColors (void) {
  for (QGraphicsItem *i: _nodes) {
    auto n = dynamic_cast<Node*>(i);
    for (Edge *e: n->out) e->clearCustomColor();
  }
}

void Viewer::depthDebugDraw (bool active) {
//  for ()
  qDebug() << "Depth debug drawing not implemented for 2d ANN";
}

} // end of namespace kgd::es_hyperneat::gui::ann2d
