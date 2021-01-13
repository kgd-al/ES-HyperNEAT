#ifndef KGD_SOUND_VISUALIZER_H
#define KGD_SOUND_VISUALIZER_H

#include <QWidget>

#include "generator.h"

namespace kgd::watchmaker::sound {

class Visualizer : public QWidget {
  Q_OBJECT

  Generator _sound;
  QImage _data;
  bool _highlight;

  bool _playing;
  float _sliderPos;

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

  void setNoteSheet(const Generator::NoteSheet &notes);
  void vocaliseToAudioOut (Generator::PlaybackType t);
  void stopVocalisationToAudioOut (void);
  const Generator& sound (void) const {
    return _sound;
  }

  void setHighlighted (bool s);

  void paintEvent(QPaintEvent *e) override;

  void render (const std::string &filename) const {
    render(QString::fromStdString(filename));
  }
  void render (const QString &filename) const;

signals:
  void notifyNote (void);

private:
  void updateSlider (void);
  void stateChanged (QAudio::State s);
};

} // end of namespace kgd::watchmaker::sound

#endif // KGD_SOUND_VISUALIZER_H
