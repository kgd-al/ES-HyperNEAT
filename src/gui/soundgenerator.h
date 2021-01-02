#ifndef SOUNDGENERATOR_H
#define SOUNDGENERATOR_H

/*
 * Mostly thanks to
 * https://stackoverflow.com/questions/21310324/play-notification-frequency-x-
 * sound-in-qt-easiest-way
 */

#include <map>

#include <QString>
#include <QVector>

namespace gui {


class SoundGenerator : public QObject {
  Q_OBJECT
public:
  static constexpr uint CHANNELS = 8;
  static constexpr float LENGTH = .5;
  static const QVector<QString> baseNotes;
  static const std::map<QString, float> notes;

  enum Instrument {
    SIN, SQUARE, TRI, SAW
  };
  Q_ENUM(Instrument);
  static const std::map<Instrument, float (*)(float)> instruments;

  SoundGenerator(QObject *parent = nullptr);
  ~SoundGenerator (void);

  void make_sound (Instrument i, float frequency, float seconds);

private:
  struct Data;
  Data *d;
};

} // end of namespace gui

#endif // SOUNDGENERATOR_H
