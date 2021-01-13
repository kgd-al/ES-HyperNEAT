#include <QtPrintSupport/QPrinter>

#include "graphviewer.h"

namespace kgd::gui {

GraphViewer::GraphViewer(QWidget *parent, const QString &type)
  : QGraphicsView(parent), _type(type) {
  setScene(new QGraphicsScene(this));
  setRenderHint(QPainter::Antialiasing);
  clearGraph();
}

void GraphViewer::clearGraph(void) {
  scene()->clear();
  scene()->addText("No " + _type + "\nto display");
  scale(1, 1);
}

void GraphViewer::setGraph(const gvc::Graph &graph) {
  auto gw = graph.build_gvc_graph();
  gw.layout(gvc_layout());
  gvRender(gvc::context(), gw.graph, "dot", NULL);

  auto scene = this->scene();
  scene->clear();

  processGraph(graph, gw);

  scene->setSceneRect(scene->itemsBoundingRect());
  ensureFit();
}

void GraphViewer::ensureFit (void) {
  if (scene()->items().size() > 1)  fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void GraphViewer::resizeEvent(QResizeEvent *e) {
  QWidget::resizeEvent(e);
  ensureFit();
}

void GraphViewer::render (const QString &filename) {
  QPrinter printer(QPrinter::HighResolution);
  printer.setPageSizeMM(sceneRect().size());
  printer.setOutputFormat(QPrinter::PdfFormat);
  printer.setOutputFileName(filename);

  QPainter p(&printer);
  scene()->render(&p);
}


} // end of namespace kgd::gui
