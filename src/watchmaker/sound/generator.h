#ifndef KGD_SOUND_GENERATOR_H
#define KGD_SOUND_GENERATOR_H

/*
 * Mostly thanks to
 * https://stackoverflow.com/questions/21310324/play-notification-frequency-x-
 * sound-in-qt-easiest-way
 */

//#include <map>

//#include <QString>
//#include <QVector>
//#include <QAudio>

struct RtMidiOut;

namespace kgd::watchmaker::sound {

struct StaticData {
  static constexpr uint CHANNELS = 4;
  static constexpr float STEP = 1/4.;
  static constexpr float DURATION = 5;

  static constexpr uint NOTES = DURATION / STEP;

  static constexpr uint BASE_OCTAVE = 3;

  using NoteSheet = std::array<float, NOTES*CHANNELS>;

  enum PlaybackType {
    LOOP, ONE_PASS, ONE_NOTE
  };
};

struct MidiWrapper {
  static bool initialize (const std::string &preferredPort = "Timidity");
  static const bool initialized;

  static RtMidiOut& midiOut(void);

  static void sendMessage (const std::vector<uchar> &message);

  static void setInstrument (uchar channel, uchar i);
  static void noteOn (uchar channel, uchar note, uchar volume);
  static void notesOff (uchar channel);
};

//class Generator : public QObject {
//  Q_OBJECT
//public:
//  static const QVector<QString> baseNotes;
//  static const std::map<QString, float> notes;

//  enum Instrument {
//    SIN, SQUARE, TRI, SAW, ORGAN, CHIRP
//  };
//  Q_ENUM(Instrument);

//  Generator(QObject *parent = nullptr);
//  ~Generator (void);

//  void setInstrument (Instrument i);

//  void setNoteSheet (const NoteSheet &notes);

//  void start (PlaybackType t);
//  void stop (void);
//  void pause (void);
//  void resume (void);

//  void generateWav(const std::string &filename) const {
//    generateWav(QString::fromStdString(filename));
//  }
//  void generateWav (const QString &filename = "") const;

//  void setNotifyPeriod (int ms);

//  // In [0,1]
//  float progress (void) const;

//signals:
//  void notify (void);
//  void notifyNote (void); ///< A full note has been played back
//  void stateChanged (QAudio::State s);

//private:
//  struct Data;
//  Data *d;

//  void onNoteConsumed (void);
//};

} // end of namespace kgd::watchmaker::sound

#endif // KGD_SOUND_GENERATOR_H
