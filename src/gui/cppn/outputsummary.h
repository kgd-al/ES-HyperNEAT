#ifndef KGD_CPPN_OUTPUTSUMMARY_H
#define KGD_CPPN_OUTPUTSUMMARY_H

#include <QLabel>

#include "../../phenotype/cppn.h"

namespace kgd::gui::cppn {

struct OutputViewer;

struct OutputSummary : public QWidget {
  OutputSummary (QWidget *parent = nullptr);

  void phenotypes (const genotype::ES_HyperNEAT &genome, phenotype::CPPN &cppn,
                   const QPointF &p);
  void noPhenotypes (void);

public slots:
  void phenotypeAt(const QPointF &p);

private:
  std::array<OutputViewer*, 4> _viewers;
  QLabel *_header;

  QLabel* keyLabel (const QString &text);
};

} // end of namespace kgd::gui::cppn

#endif // KGD_CPPN_OUTPUTSUMMARY_H
