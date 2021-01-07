#ifndef SOUND_GENERATOR_H
#define SOUND_GENERATOR_H

/*
 * Mostly thanks to
 * https://stackoverflow.com/questions/21310324/play-notification-frequency-x-
 * sound-in-qt-easiest-way
 */

#include <map>

#include <QString>
#include <QVector>

namespace sound {

class Generator : public QObject {
  Q_OBJECT
public:
  static constexpr uint CHANNELS = 2;
  static constexpr float STEP = 1./4.;
  static constexpr float DURATION = 1;

  static constexpr uint NOTES = DURATION / STEP;

  static constexpr uint BASE_OCTAVE = 3;

  static const QVector<QString> baseNotes;
  static const std::map<QString, float> notes;

  enum Instrument {
    SIN, SQUARE, TRI, SAW, ORGAN, CHIRP
  };
  Q_ENUM(Instrument);

  Generator(QObject *parent = nullptr);
  ~Generator (void);

  void setInstrument (Instrument i);

  using NoteSheet = std::array<float, NOTES*CHANNELS>;
  virtual void setNoteSheet (const NoteSheet &notes);

  virtual void vocaliseToAudioOut (void);
  void stopVocalisationToAudioOut (void);

  void vocaliseToFile (const QString &filename = "");

private:
  struct Data;
  Data *d;
};

} // end of namespace sound

#endif // SOUND_GENERATOR_H
