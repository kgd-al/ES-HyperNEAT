#include <QTextStream>

#include "neuronstate.h"

namespace kgd::es_hyperneat::gui::neurons {

NeuronStateViewer::NeuronStateViewer (void) {
  setReadOnly(true);
}

void NeuronStateViewer::noState(void) {
  _neuron = nullptr;
  updateState();
}

void NeuronStateViewer::displayState(const Neuron *n) {
  _neuron = n;
  updateState();
}

QTextStream& operator<< (QTextStream &qts, const phenotype::Point &p) {
  qts << "[" << p.x() << ", " << p.y();
#if ESHN_SUBSTRATE_DIMENSION > 2
  qts << ", " << p.z();
#endif
  qts << "]";
  return qts;
}

void NeuronStateViewer::updateState(void) {
  static const auto activation =
    QString::fromStdString(
      (std::string)config::EvolvableSubstrate::activationFunc());

  if (_neuron == nullptr) {
    setPlainText("N/A");
    return;
  }

  QString contents;
  QTextStream qts (&contents);
  qts.setRealNumberNotation(QTextStream::FixedNotation);
  qts.setRealNumberPrecision(3);
  qts.setNumberFlags(QTextStream::ForceSign);

  static constexpr auto D = ESHN_SUBSTRATE_DIMENSION;
  static constexpr auto NEURON_POS_LENGTH = 2+D*6+(D-1)*2;

  const Neuron &n = *_neuron;
  qts << (n.type == Neuron::I ? "Input"
                              : n.type == Neuron::O ? "Output"
                                                    : "Hidden")
      << " neuron\n"
      << "at " << n.pos << "\n"
      << "depth: " << n.depth << "\n----\n";

  float input = n.bias;
  if (!n.isInput()) {
    qts << " input:\n";

    qts << QString(NEURON_POS_LENGTH-3, ' ') << "[B] " << n.bias << "\n";
    for (const phenotype::ANN::Neuron::Link &l: n.links()) {
      const Neuron &in = *l.in.lock();
      qts << in.pos << " "
          << in.value * l.weight << " (" << in.value << " * " << l.weight
          << ")\n";
      input += in.value * l.weight;
    }

    qts << QString(NEURON_POS_LENGTH, ' ') << " = " << input << "\n";
  }

  qts << "\noutput: " << n.value;

  if (!n.isInput()) qts << " = " << activation << "(" << input << ")";

  setPlainText(contents);
}

} // end of namespace kgd::es_hyperneat::gui::neurons
