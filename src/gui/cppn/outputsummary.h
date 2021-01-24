#ifndef KGD_CPPN_OUTPUTSUMMARY_H
#define KGD_CPPN_OUTPUTSUMMARY_H

#include <QLabel>

#include "../../phenotype/cppn.h"

namespace kgd::es_hyperneat::gui::cppn {

struct OutputViewer;

struct OutputSummary : public QWidget {
  enum ShowFlags {
    SHOW_OUTGOING = 0x1,
    SHOW_INCOMING = 0x2,

    SHOW_NONE = 0, SHOW_ALL = SHOW_OUTGOING | SHOW_INCOMING
  };

  OutputSummary (QWidget *parent = nullptr);

  void phenotypes (const genotype::ES_HyperNEAT &genome, phenotype::CPPN &cppn,
                   const QPointF &p, ShowFlags f);
  void noPhenotypes (void);

public slots:
  void phenotypeAt(const QPointF &p);

private:
  std::array<OutputViewer*, 4> _viewers;
  QLabel *_header;
};

} // end of namespace kgd::es_hyperneat::gui::cppn

#endif // KGD_CPPN_OUTPUTSUMMARY_H
