#ifndef KGD_ANN_VIEWER_H
#define KGD_ANN_VIEWER_H

#include "../../phenotype/ann.h"
#include "../graphviewer.h"

namespace gui::ann {

struct Viewer : public GraphViewer {
  Q_OBJECT
  using Graph_t = phenotype::ANN;
public:
  Viewer (QWidget *parent = nullptr);

  void startAnimation (void);
  void updateAnimation (void);
  void stopAnimation (void);

  bool isAnimating (void) const {
    return _animating;
  }

signals:
  void neuronHovered (const QPointF &p);

private:
  QVector<QGraphicsItem*> _neurons;
  bool _animating;

  const char* gvc_layout (void) const override { return "nop"; }
  void processGraph (const gvc::Graph &g,
                     const gvc::GraphWrapper &gw) override;
};

} // end of namespace gui::ann

#endif // KGD_ANN_VIEWER_H
