#ifndef CPPN_VIEWER_H
#define CPPN_VIEWER_H

#include <QGraphicsView>

#include "../../genotype/es-hyperneat.h"

namespace gui::cppn {

struct Viewer : public QGraphicsView {
  Viewer (QWidget *parent = nullptr);

  void setCPPN (const genotype::ES_HyperNEAT::CPPN &cppn);
  void noCPPN (void);

  void ensureFit (void);
  void resizeEvent(QResizeEvent *e);

  void render (const QString &filename);
};

} // end of namespace gui::cppn

#endif // CPPN_VIEWER_H
