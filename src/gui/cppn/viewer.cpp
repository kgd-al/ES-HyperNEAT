#include "viewer.h"

#include "node.h"
#include "edge.h"

namespace kgd::es_hyperneat::gui::cppn {

Viewer::Viewer (QWidget *parent) : GraphViewer(parent, "CPPN") {}

void Viewer::processGraph (const gvc::Graph &g, const gvc::GraphWrapper &gw) {
  auto scene = this->scene();
  auto gvc = gw.graph;
  qreal s = gvc::get(gvc, "dpi", 96.0) / 72.;
  for (auto *n = agfstnode(gvc); n != NULL; n = agnxtnode(gvc, n)) {
    scene->addItem(new Node(n, s));

    for (auto *e = agfstout(gvc, n); e != NULL; e = agnxtout(gvc, e)) {
      if (gvc::get(e, "style", std::string()) != "invis")
        scene->addItem(new Edge(e, s));
    }
  }
}

} // end of namespace kgd::es_hyperneat::gui::cppn
