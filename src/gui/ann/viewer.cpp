#include <QtPrintSupport/QPrinter>

#include "viewer.h"

#include "node.h"
#include "edge.h"

namespace gui::ann {

Viewer::Viewer(QWidget *parent) : QGraphicsView(parent) {
  setScene(new QGraphicsScene(this));
  setRenderHint(QPainter::Antialiasing);
  setCursor(Qt::CrossCursor);
}

void Viewer::setANN(const phenotype::ANN &ann) {
  auto gw = ann.graphviz_build_graph();
  gw.layout("nop");

  gvRender(gvc::context(), gw.graph, "dot", NULL);

  auto scene = this->scene();
  scene->clear();

  std::map<std::string, Edge*> edges;

  auto g = gw.graph;
  qreal s = gvc::get(g, "dpi", 96.0) / 72.;

  const auto getOrNew = [&edges, &scene, s] (Agedge_t *e) {
    auto ename = std::string(agnameof(e));
    auto it = edges.find(ename);
    if (it != edges.end())
      return it->second;
    else {
      auto qe = new Edge(e, s);
      scene->addItem(qe);
      return qe;
    }
  };

  for (auto *n = agfstnode(g); n != NULL; n = agnxtnode(g, n)) {
    auto qn = new Node(n, s);
    scene->addItem(qn);
    connect(qn, &Node::hovered, this, &Viewer::neuronHovered);

    for (auto *e = agfstout(g, n); e != NULL; e = agnxtout(g, e)) {
      if (gvc::get(e, "style", std::string()) == "invis") continue;
      qn->out.push_back(getOrNew(e));
    }

    for (auto *e = agfstin(g, n); e != NULL; e = agnxtin(g, e)) {
      if (gvc::get(e, "style", std::string()) == "invis") continue;
      qn->in.push_back(getOrNew(e));
    }
  }

  scene->setSceneRect(scene->itemsBoundingRect());
  ensureFit();
}

void Viewer::ensureFit (void) {
  fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void Viewer::resizeEvent(QResizeEvent *e) {
  QWidget::resizeEvent(e);
  ensureFit();
}

void Viewer::render (const QString &filename) {
  QPrinter printer(QPrinter::HighResolution);
  printer.setPageSizeMM(sceneRect().size());
  printer.setOutputFormat(QPrinter::PdfFormat);
  printer.setOutputFileName(filename);

  QPainter p(&printer);
  scene()->render(&p);
}

} // end of namespace gui::ann
