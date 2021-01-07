#ifndef SOUND_VISUALIZER_H
#define SOUND_VISUALIZER_H

#include <QWidget>

#include "generator.h"

namespace sound {

class Visualizer : public QWidget, public Generator {
  QImage _data;
  bool _highlight;

public:
  Visualizer(QWidget *parent = nullptr);
  ~Visualizer (void);

  QSize minimumSizeHint(void) const override {
    return QSize(100,100);
  }

  bool hasHeightForWidth(void) const override {
    return true;
  }

  int heightForWidth(int w) const override {
    return w;
  }

  void setNoteSheet(const NoteSheet &notes) override;

  void setHighlighted (bool s);

  void paintEvent(QPaintEvent *e) override;
};

} // end of namespace sound
#endif // SOUND_VISUALIZER_H
