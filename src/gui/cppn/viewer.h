#ifndef KGD_CPPN_VIEWER_H
#define KGD_CPPN_VIEWER_H

#include "../../genotype/es-hyperneat.h"
#include "../graphviewer.h"

namespace kgd::gui::cppn {

struct Viewer : public GraphViewer {
  using Graph_t = genotype::ES_HyperNEAT::CPPN;

  Viewer (QWidget *parent = nullptr);

private:
  const char* gvc_layout (void) const override { return "dot"; }
  void processGraph (const gvc::Graph &g,
                     const gvc::GraphWrapper &gw);
};

} // end of namespace kgd::gui::cppn

#endif // KGD_CPPN_VIEWER_H
