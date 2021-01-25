#ifndef KGD_SOUND_GENERATOR_H
#define KGD_SOUND_GENERATOR_H

#include <string>

struct RtMidiOut;

namespace kgd::watchmaker::sound {

struct StaticData {
  static constexpr uint CHANNELS = 4;
  static constexpr uint TEMPO = 240;  // BPM
  static constexpr uint NOTES = 10;

  static constexpr float NOTE_DURATION = 60.f / TEMPO;
  static constexpr float SONG_DURATION = NOTES * 60.f / TEMPO;

  static constexpr uint BASE_OCTAVE = 3;
  static constexpr uint BASE_A = 21 + 12 * BASE_OCTAVE;

  using NoteSheet = std::array<float, NOTES*CHANNELS>;

  enum PlaybackType {
    LOOP, ONE_PASS, ONE_NOTE
  };
};

struct MidiWrapper {
  static bool initialize (std::string preferredPort = "");

  static RtMidiOut& midiOut(void);

  static void sendMessage (const std::vector<uchar> &message);

  static uchar velocity (float volume);
  static uchar key (int index);

  static void setInstrument (uchar channel, uchar i);
  static void noteOn (uchar channel, uchar note, uchar volume);
  static void notesOff (uchar channel);

  static bool writeMidi (const StaticData::NoteSheet &notes,
                         const std::string &path);
};

} // end of namespace kgd::watchmaker::sound

#endif // KGD_SOUND_GENERATOR_H
