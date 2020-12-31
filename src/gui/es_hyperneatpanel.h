#ifndef ES_HYPERNEATPANEL_H
#define ES_HYPERNEATPANEL_H

#include <QLabel>

#include "cppn/viewer.h"
#include "cppn/outputsummary.h"

#include "ann/viewer.h"

namespace gui {

struct ES_HyperNEATPanel : public QWidget {
  cppn::Viewer *_cppnViewer;
  cppn::OutputSummary *_cppnOViewer;
  ann::Viewer *_annViewer;
  std::map<std::string, QLabel*> _otherFields;

  ES_HyperNEATPanel (QWidget *parent = nullptr);
  void setData (const genotype::ES_HyperNEAT &genome, phenotype::CPPN &cppn,
                phenotype::ANN &ann);
};

} // end of namespace gui

#endif // ES_HYPERNEATPANEL_H
