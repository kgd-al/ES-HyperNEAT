#include <QGridLayout>

#include "es_hyperneatpanel.h"

namespace gui {

ES_HyperNEATPanel::ES_HyperNEATPanel (QWidget *parent) : QWidget(parent) {
  auto *layout = new QGridLayout;
  layout->addWidget(_cppnViewer = new cppn::Viewer, 0, 0);
  layout->addWidget(_cppnOViewer = new cppn::OutputSummary, 0, 1);
  setLayout(layout);
}

void ES_HyperNEATPanel::setData (const genotype::ES_HyperNEAT &genome,
                                 phenotype::CPPN &cppn) {
  _cppnViewer->setCPPN(genome.cppn);
  _cppnOViewer->phenotypes(cppn, {0,0});
}

} // end of namespace gui
