#ifndef KGD_ANN_2D_VIEWER_H
#define KGD_ANN_2D_VIEWER_H

#include <QTextEdit>

#include "../../../phenotype/ann.h"
#include "../../graphviewer.h"
#include "node.h"

namespace kgd::es_hyperneat::gui::ann2d {

struct NeuronStateViewer : public QTextEdit {
  using Neuron = phenotype::ANN::Neuron;
  NeuronStateViewer (void);

  void noState (void);
  void displayState (const Neuron *n);
  void updateState (void);

private:
  const Neuron *_neuron;
};

struct Viewer : public GraphViewer {
  Q_OBJECT
  using Graph_t = phenotype::ANN;
public:
  Viewer (QWidget *parent = nullptr);

  void setGraph (const phenotype::ANN &ann);
  void setGraph (const phenotype::ModularANN &ann);

  void startAnimation (void);
  void updateAnimation (void);
  void stopAnimation (void);

  bool isAnimating (void) const {
    return _animating;
  }

  void updateCustomColors (void);
  void clearCustomColors (void);

signals:
  void neuronHovered (const phenotype::ANN::Neuron *n);

private:
  QVector<QGraphicsItem*> _nodes;
  bool _animating;

  const char* gvc_layout (void) const override;
  void processGraph (const gvc::Graph &g,
                     const gvc::GraphWrapper &gw) override;
};

} // end of namespace kgd::es_hyperneat::gui::ann2d

#endif // KGD_ANN_2D_VIEWER_H
