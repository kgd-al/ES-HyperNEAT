#include <cmath>

#include <QPainter>

#include "visualizer.h"

#include <iostream>
#include "kgd/utils/utils.h"

namespace kgd::watchmaker::sound {

static constexpr uint MS_PER_NOTE = StaticData::STEP * 1000.;
static constexpr uint UPDATE_PERIOD_MS = 1000. / 25;
static constexpr uint DURATION_MS = StaticData::DURATION * 1000;

Visualizer::Visualizer(uchar channel, QWidget *parent)
  : QWidget(parent), _channel(channel) {
//  setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  QSizePolicy sizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred );
  sizePolicy.setHeightForWidth(true);
  setSizePolicy(sizePolicy);
  setAttribute(Qt::WA_Hover, true);

  _highlight = false;

  _notes = nullptr;
  _prevNote = _currNote = -1;

  _currentMS = 0;
  _playback = StaticData::LOOP;

  connect(&_timer, &QTimer::timeout, this, &Visualizer::timeout);
  _sliderPos = 0;

  _data = QImage(StaticData::NOTES, StaticData::CHANNELS, QImage::Format_RGB32);

//  _sound.setNotifyPeriod(25);
//  connect(&_sound, &Generator::notify, this, &Visualizer::updateSlider);
//  connect(&_sound, &Generator::stateChanged, this, &Visualizer::stateChanged);
//  connect(&_sound, &Generator::notifyNote, this, &Visualizer::notifyNote);
}

Visualizer::~Visualizer(void) {}

void Visualizer::setNoteSheet(const StaticData::NoteSheet &notes) {
//  _sound.setNoteSheet(notes);
  _notes = &notes;

  for (uint c=0; c<StaticData::CHANNELS; c++) {
    auto b = (QRgb*) _data.scanLine(c);
    for (uint n=0; n<StaticData::NOTES; n++)
      b[n] = QColor::fromHsv(
        0, 0, 255*std::max(0.f, notes[n*StaticData::CHANNELS+c])).rgb();
  }

  update();
}

void Visualizer::vocaliseToAudioOut (StaticData::PlaybackType t) {
  _playback = t;
  if (_playback == t && _currentMS > 0)
    resume();
  else
    start();
}

void Visualizer::stopVocalisationToAudioOut (void) {
  stop();
}

void Visualizer::start (void) {
  _timer.start(UPDATE_PERIOD_MS);
  _currentMS = 0;
  _sliderPos = 0;
  _currNote = 0;
  nextNote(false);
}

void Visualizer::pause (void) {
  MidiWrapper::notesOff(_channel);
  _timer.stop();
}

void Visualizer::resume (void) {
  _timer.start(UPDATE_PERIOD_MS);
  nextNote(false);
}

void Visualizer::stop (void) {
//  _sound.stop();
  MidiWrapper::notesOff(_channel);
  _timer.stop();
  _currentMS = 0;
//  _sliderPos = 0;
}

void Visualizer::setHighlighted(bool h) {
  _highlight = h;
  update();
}

void Visualizer::timeout(void) {
  uint note = _currentMS / MS_PER_NOTE;
  _currentMS += UPDATE_PERIOD_MS;
  if (note < _currentMS / MS_PER_NOTE)  nextNote(true);
  _currentMS %= DURATION_MS;
  updateSlider();
}

void Visualizer::updateSlider(void) {
//  _sliderPos = std::fmod(_sound.progress(), 1);
//  qDebug() << "Underlying sound object has progressed to" << _sliderPos;
  _sliderPos = float(_currentMS) / DURATION_MS;
  update();
}

//void Visualizer::stateChanged(QAudio::State s) {
//  _playing = (s == QAudio::ActiveState);
////  qDebug() << "Underlying sound object changed stated to" << s;
//}

void Visualizer::nextNote(bool spontaneous) {
  _currNote = (_currentMS / MS_PER_NOTE);
  if (_playback == StaticData::ONE_NOTE && spontaneous) {
      pause();
      return;

  } else if (_playback == StaticData::ONE_PASS &&
             _currNote == StaticData::NOTES) {
    stopVocalisationToAudioOut();
    return;

  } else if (_playback == StaticData::LOOP)
    _currNote %= StaticData::NOTES;

  std::cerr << utils::CurrentTime{} << " playing next notes: "
            << _currNote << "\n";
  std::cout << "sending notes [";
  static constexpr auto C = StaticData::CHANNELS;
  for (uint i=0; i<C; i++)
    std::cout << " " << (*_notes)[i+C*_currNote];
  std::cout << " ]\n";

  for (uint i=0; i<C; i++) {
    float n = (*_notes)[i+C*_currNote];
    if (_prevNote < 0 || (*_notes)[i+C*_prevNote] != n)
      MidiWrapper::noteOn(_channel, 60+i, std::round(127*std::max(0.f, n)));
  }

  _prevNote = _currNote;
}

void Visualizer::paintEvent(QPaintEvent *e) {
  QWidget::paintEvent(e);

  isActiveWindow();
  QPainter p (this);
  p.setPen(palette().color(_highlight ? QPalette::Highlight
                                      : QPalette::Light));
  p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 1, 1);
  p.drawImage(rect().adjusted(1, 1, -1, -1), _data);

  if (_timer.isActive() || _currentMS != 0) {
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

} // end of namespace kgd::watchmaker::sound
