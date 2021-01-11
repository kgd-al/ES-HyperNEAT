#ifndef ANN_VIEWER_H
#define ANN_VIEWER_H

#include "../../phenotype/ann.h"
#include "../graphviewer.h"

namespace gui::ann {

struct Viewer : public GraphViewer {
  Q_OBJECT
  using Graph_t = phenotype::ANN;
public:
  Viewer (QWidget *parent = nullptr);

signals:
  void neuronHovered (const QPointF &p);

private:
  const char* gvc_layout (void) const { return "nop"; }
  void processGraph (const gvc::GraphWrapper &graph);
};

} // end of namespace gui::ann

#endif // ANN_VIEWER_H
