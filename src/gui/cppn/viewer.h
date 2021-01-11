#ifndef CPPN_VIEWER_H
#define CPPN_VIEWER_H

#include "../../genotype/es-hyperneat.h"
#include "../graphviewer.h"

namespace gui::cppn {

struct Viewer : public GraphViewer {
  using Graph_t = genotype::ES_HyperNEAT::CPPN;

  Viewer (QWidget *parent = nullptr);

private:
  const char* gvc_layout (void) const { return "dot"; }
  void processGraph (const gvc::GraphWrapper &graph);
};

} // end of namespace gui::cppn

#endif // CPPN_VIEWER_H
