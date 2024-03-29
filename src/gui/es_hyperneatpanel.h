#ifndef KGD_ES_HYPERNEATPANEL_H
#define KGD_ES_HYPERNEATPANEL_H

#include <QLabel>
#include <QSplitter>
#include <QSettings>

#include "cppn/viewer.h"
#include "cppn/outputsummary.h"

#if ESHN_SUBSTRATE_DIMENSION == 2
#include "ann/2d/viewer.h"
#elif ESHN_SUBSTRATE_DIMENSION == 3
#include "ann/3d/viewer.h"
#endif

#include "ann/neuronstate.h"

namespace kgd::gui {

void save (QSettings &settings, const QSplitter *splitter);
void restore (const QSettings &settings, QSplitter *splitter);

} // end of namespace gui

namespace kgd::es_hyperneat::gui {

#if ESHN_SUBSTRATE_DIMENSION == 2
namespace ann = ann2d;
#elif ESHN_SUBSTRATE_DIMENSION == 3
namespace ann = ann3d;
#endif

struct ES_HyperNEATPanel : public QWidget {
  using NeuronType = phenotype::ANN::Neuron::Type;

  cppn::Viewer *cppnViewer;
  cppn::OutputSummary *cppnOViewer;
  ann::Viewer *annViewer;
  neurons::NeuronStateViewer *neuronViewer;
  std::map<std::string, QLabel*> otherFields;

#if ESHN_SUBSTRATE_DIMENSION == 3
  QWidget *annViewerWidget;
#endif

  ES_HyperNEATPanel (QWidget *parent = nullptr);
  ~ES_HyperNEATPanel (void);

  void setData (const genotype::ES_HyperNEAT &genome,
                const phenotype::CPPN &cppn,
                const phenotype::ANN &ann);
  void noData (void);

private:
  const genotype::ES_HyperNEAT *_genome;
  const phenotype::CPPN *_cppn;
  const phenotype::ANN *_ann;
  QSplitter *_mainSplitter, *_cppnSplitter, *_annSplitter;

  bool _settingsLoaded;

  void neuronHovered(const phenotype::ANN::Neuron *n);

  void showEvent(QShowEvent *e) override;
  void hideEvent(QHideEvent *e) override;
};

} // end of namespace kgd::es_hyperneat::gui

#endif // KGD_ES_HYPERNEATPANEL_H
