#include "viewer.h"

#include "node.h"
#include "edge.h"

namespace gui::cppn {

Viewer::Viewer (QWidget *parent) : GraphViewer(parent, "CPPN") {}

void Viewer::processGraph (const gvc::GraphWrapper &cppn) {
  auto scene = this->scene();
  auto g = cppn.graph;
  qreal s = gvc::get(g, "dpi", 96.0) / 72.;
  for (auto *n = agfstnode(g); n != NULL; n = agnxtnode(g, n)) {
    scene->addItem(new Node(n, s));

    for (auto *e = agfstout(g, n); e != NULL; e = agnxtout(g, e)) {
      if (gvc::get(e, "style", std::string()) != "invis")
        scene->addItem(new Edge(e, s));
    }
  }
}

} // end of namespace gui::cppn
