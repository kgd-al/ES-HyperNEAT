#ifndef KGD_ANN_NEURONSTATE_H
#define KGD_ANN_NEURONSTATE_H

#include <QTextEdit>

#include "../../phenotype/ann.h"

namespace kgd::es_hyperneat::gui::neurons {

struct NeuronStateViewer : public QTextEdit {
  using Neuron = phenotype::ANN::Neuron;
  NeuronStateViewer (void);

  void noState (void);
  void displayState (const Neuron *n);
  void updateState (void);

private:
  const Neuron *_neuron;
};

} // end of namespace kgd::es_hyperneat::gui::neurons

#endif // KGD_ANN_NEURONSTATE_H
