#include <QtPrintSupport/QPrinter>

#include "viewer.h"

namespace gui::ann {

Viewer::Viewer(QWidget *parent)
  : QGraphicsView(new QGraphicsScene, parent) {
  setRenderHint(QPainter::Antialiasing);
}

void Viewer::setANN(const phenotype::ANN &ann) {
  auto gw = ann.graphviz_build_graph();
  gw.layout("dot");

  gvRender(gvc::context(), gw.graph, "dot", NULL);

  auto scene = this->scene();
  scene->clear();

//  auto g = gw.graph;
//  qreal s = gvc::get(g, "dpi", 96.0) / 72.;
//  for (auto *n = agfstnode(g); n != NULL; n = agnxtnode(g, n)) {
//    scene->addItem(new Node(n, s));

//    for (auto *e = agfstout(g, n); e != NULL; e = agnxtout(g, e)) {
//      if (gvc::get(e, "style", std::string()) != "invis")
//        scene->addItem(new Edge(e, s));
//    }
//  }

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