#ifndef KGD_ESHN_GUI_CONFIG_H
#define KGD_ESHN_GUI_CONFIG_H

#include <QColor>
#include <QVector>

#include "../phenotype/ann.h"

namespace config {

struct CONFIG_FILE(ESHNGui) {

  using NFlag = decltype(phenotype::ANN::Neuron::flags);
  using ANNCustomColors = std::map<NFlag, QColor>;
  using CustomColors = QVector<QColor>;
  DECLARE_PARAMETER(ANNCustomColors, annColorMapping)

  static CustomColors colorsForFlag (NFlag flag);
};

} // end of namespace config

#endif // KGD_ESHN_GUI_CONFIG_H
