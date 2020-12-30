#ifndef CPPN_OUTPUTSUMMARY_H
#define CPPN_OUTPUTSUMMARY_H

#include <QLabel>

#include "../../phenotype/cppn.h"

namespace gui::cppn {

struct OutputViewer;

struct OutputSummary : public QWidget {
  OutputSummary (QWidget *parent = nullptr);

  void phenotypes (phenotype::CPPN &cppn, const QPointF &p);

private:
  std::array<OutputViewer*, 4> _viewers;
  QLabel *_header;

  QLabel* keyLabel (const QString &text);
};

} // end of namespace gui::cppn

#endif // CPPN_OUTPUTSUMMARY_H
