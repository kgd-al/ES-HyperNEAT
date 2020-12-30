#ifndef ES_HYPERNEATPANEL_H
#define ES_HYPERNEATPANEL_H

#include <QWidget>

#include "cppn/viewer.h"
#include "cppn/outputsummary.h"

namespace gui {

struct ES_HyperNEATPanel : public QWidget {
  cppn::Viewer *_cppnViewer;
  cppn::OutputSummary *_cppnOViewer;

  ES_HyperNEATPanel (QWidget *parent = nullptr);
  void setData (const genotype::ES_HyperNEAT &genome, phenotype::CPPN &cppn);
};

} // end of namespace gui

#endif // ES_HYPERNEATPANEL_H
