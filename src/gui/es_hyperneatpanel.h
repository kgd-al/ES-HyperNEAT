#ifndef KGD_ES_HYPERNEATPANEL_H
#define KGD_ES_HYPERNEATPANEL_H

#include <QLabel>

#include "cppn/viewer.h"
#include "cppn/outputsummary.h"

#include "ann/viewer.h"

namespace kgd::gui {

struct ES_HyperNEATPanel : public QWidget {
  cppn::Viewer *cppnViewer;
  cppn::OutputSummary *cppnOViewer;
  ann::Viewer *annViewer;
  std::map<std::string, QLabel*> otherFields;

  ES_HyperNEATPanel (QWidget *parent = nullptr);
  void setData (const genotype::ES_HyperNEAT &genome, phenotype::CPPN &cppn,
                phenotype::ANN &ann);
  void noData (void);

public slots:
  void showCPPNOutputsAt (const QPointF &p);

private:
  const genotype::ES_HyperNEAT *_genome;
  phenotype::CPPN *_cppn;
  const phenotype::ANN *_ann;
};

} // end of namespace kgd::gui

#endif // KGD_ES_HYPERNEATPANEL_H
