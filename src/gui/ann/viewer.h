#ifndef ANN_VIEWER_H
#define ANN_VIEWER_H

#include <QGraphicsView>

#include "../../phenotype/ann.h"

namespace gui::ann {

struct Viewer : public QGraphicsView {
  Q_OBJECT
public:
  Viewer (QWidget *parent = nullptr);

  void setANN (const phenotype::ANN &ann);
  void noANN (void);

  void ensureFit (void);
  void resizeEvent(QResizeEvent *e);

  void render (const QString &filename);

signals:
  void neuronHovered (const QPointF &p);
};

} // end of namespace gui::ann

#endif // ANN_VIEWER_H
