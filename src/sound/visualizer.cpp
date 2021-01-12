#include <cmath>

#include <QPainter>

#include "visualizer.h"

#include <QDebug>

namespace sound {

Visualizer::Visualizer(QWidget *parent) : QWidget(parent) {
//  setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  QSizePolicy sizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred );
  sizePolicy.setHeightForWidth(true);
  setSizePolicy(sizePolicy);
  setAttribute(Qt::WA_Hover, true);

  _highlight = false;
  _playing = false;
  _sliderPos = 0;

  _data = QImage(Generator::NOTES, Generator::CHANNELS, QImage::Format_RGB32);

  _sound.setNotifyPeriod(25);
  connect(&_sound, &Generator::notify, this, &Visualizer::updateSlider);
  connect(&_sound, &Generator::stateChanged, this, &Visualizer::stateChanged);
  connect(&_sound, &Generator::notifyNote, this, &Visualizer::notifyNote);
}

Visualizer::~Visualizer(void) {}

void Visualizer::setNoteSheet(const Generator::NoteSheet &notes) {
  _sound.setNoteSheet(notes);

  for (uint c=0; c<Generator::CHANNELS; c++) {
    auto b = (QRgb*) _data.scanLine(c);
    for (uint n=0; n<Generator::NOTES; n++)
      b[n] = QColor::fromHsv(
        0, 0, 255*std::max(0.f, notes[n*Generator::CHANNELS+c])).rgb();
  }

  update();
}

void Visualizer::vocaliseToAudioOut (Generator::PlaybackType t) {
  _sound.start(t);
  _playing = true;
  _sliderPos = 0;
}

void Visualizer::stopVocalisationToAudioOut (void) {
  _sound.stop();
  _playing = false;
//  _sliderPos = 0;
}

void Visualizer::setHighlighted(bool h) {
  _highlight = h;
  update();
}

void Visualizer::updateSlider(void) {
  _sliderPos = std::fmod(_sound.progress(), 1);
//  qDebug() << "Underlying sound object has progressed to" << _sliderPos;
  update();
}

void Visualizer::stateChanged(QAudio::State s) {
  _playing = (s == QAudio::ActiveState);
//  qDebug() << "Underlying sound object changed stated to" << s;
}

void Visualizer::paintEvent(QPaintEvent *e) {
  QWidget::paintEvent(e);

  isActiveWindow();
  QPainter p (this);
  p.setPen(palette().color(_highlight ? QPalette::Highlight
                                      : QPalette::Light));
  p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 1, 1);
  p.drawImage(rect().adjusted(1, 1, -1, -1), _data);

  if (_playing) {
    int x = width() * _sliderPos;
    p.setPen(Qt::yellow);
    p.drawLine(x, 0, x, height());
  }
}

void Visualizer::render (const QString &filename) const {
  static const QRect DRAW_RECT (0, 0, 100, 100);
  QImage i (DRAW_RECT.size(), QImage::Format_Grayscale8);
  QPainter p (&i);
  p.drawImage(DRAW_RECT, _data);
  i.save(filename);
}

} // end of namespace sound
