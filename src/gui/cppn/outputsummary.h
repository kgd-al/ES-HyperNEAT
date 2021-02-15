#ifndef KGD_CPPN_OUTPUTSUMMARY_H
#define KGD_CPPN_OUTPUTSUMMARY_H

#include <QLabel>

#include "../../phenotype/cppn.h"

namespace kgd::es_hyperneat::gui::cppn {

struct OutputViewer;

struct OutputSummary : public QWidget {
  enum ShowFlags {
    NEURAL   = 0x1,
    OUTGOING = 0x2,
    INCOMING = 0x4,

    NONE = 0,
    CONNECTIONS = OUTGOING | INCOMING,
    ALL = NEURAL | CONNECTIONS
  };

  OutputSummary (QWidget *parent = nullptr);

  void phenotypes (const phenotype::CPPN &cppn, const QPointF &p, ShowFlags f);
  void noPhenotypes (ShowFlags f = ALL);

public slots:
  void phenotypeAt(const QPointF &p);

private:
  std::array<OutputViewer*, 1> _nviewers;
  std::array<OutputViewer*, 4> _cviewers;
  QLabel *_cheader;
};

} // end of namespace kgd::es_hyperneat::gui::cppn

#endif // KGD_CPPN_OUTPUTSUMMARY_H
