#ifndef KGD_ES_HYPERNEATPANEL_H
#define KGD_ES_HYPERNEATPANEL_H

#include <QLabel>
#include <QSplitter>

#include "cppn/viewer.h"
#include "cppn/outputsummary.h"

#include "ann/viewer.h"

namespace kgd::es_hyperneat::gui {

struct ES_HyperNEATPanel : public QWidget {
  using NeuronType = phenotype::ANN::Neuron::Type;

  cppn::Viewer *cppnViewer;
  cppn::OutputSummary *cppnOViewer;
  ann::Viewer *annViewer;
  ann::NeuronStateViewer *neuronViewer;
  std::map<std::string, QLabel*> otherFields;

  ES_HyperNEATPanel (QWidget *parent = nullptr);
  void setData (const genotype::ES_HyperNEAT &genome, phenotype::CPPN &cppn,
                phenotype::ANN &ann);
  void noData (void);

private:
  const genotype::ES_HyperNEAT *_genome;
  phenotype::CPPN *_cppn;
  const phenotype::ANN *_ann;
  QSplitter *_mainSplitter, *_cppnSplitter, *_annSplitter;

  bool _settingsLoaded;

  void neuronHovered(const phenotype::ANN::Neuron &n);

  void showEvent(QShowEvent *e) override;
  void hideEvent(QHideEvent *e) override;
};

} // end of namespace kgd::es_hyperneat::gui

#endif // KGD_ES_HYPERNEATPANEL_H
