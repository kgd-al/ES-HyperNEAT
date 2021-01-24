#include <cmath>

#include <QBuffer>
#include <QAudioOutput>

#include <QFile>
#include <QDataStream>

#include "generator.h"
#include "../config.h"

#define __RTMIDI_DEBUG__
#include "rtmidi/RtMidi.h"

#include <QDebug>
#include <iomanip>

#include <fstream>
#include <sstream>
#include <cassert>
#include <QDateTime>
#include <QTimer>

#ifndef NDEBUG
#define DEBUG_AUDIO 0
#define DEBUG_MIDI_AUDIO 100
#endif

#include "../../../midi/midifile/include/MidiFile.h"

namespace kgd::watchmaker::sound {

uchar key (uchar index) {   return StaticData::BASE_A+index;  }
uchar vel (float volume)  { return std::round(127*std::max(0.f, volume)); }

RtMidiOut& MidiWrapper::midiOut (void) {
  static RtMidiOut *o = new RtMidiOut(RtMidi::LINUX_ALSA);
  return *o;
}

bool MidiWrapper::initialize (std::string preferredPort) {
  if (preferredPort.empty())  preferredPort = config::WatchMaker::midiPort();

  auto &midi = midiOut();
  uint n = midi.getPortCount();
  int index = -1;
  std::cout << "\nPreferred midi port: " << preferredPort << "\n";
  std::cout << "\t" << n << " output ports available.\n";
  for (uint i=0; i<n; i++) {
    std::string portName = midi.getPortName(i);
    bool match = false;
    if (index < 0) {
      auto it = std::search(portName.begin(), portName.end(),
                            preferredPort.begin(), preferredPort.end(),
                            [] (char lhs, char rhs) {
        return std::toupper(lhs) == std::toupper(rhs);
      });
      match = (it != portName.end());
      if (match)  index = i;
    }
    std::cout << (match? '*' : ' ') << " Output Port #" << i+1 << ": "
              << portName << '\n';
  }
  std::cout << '\n';

  if (index < 0) {
    std::cout << "Did not find a port matching pattern '" << preferredPort
              << "'.\nDefaulting to first available port\n";
    index = 0;
  }

  std::cout << "Using port #" << index+1 << ": " << midi.getPortName(index)
            << "\n";
  midi.openPort(index);

  return true;
}

void MidiWrapper::sendMessage (const std::vector<uchar> &message) {
//  std::cout << "Sending Midi message:";
//  for (uchar c: message)
//    std::cout << " " << std::hex << std::setw(2) << uint(c);
//  std::cout << std::endl;

  midiOut().sendMessage(&message);
}

void MidiWrapper::setInstrument(uchar channel, uchar i) {
  static std::vector<uchar> buffer ({0x90, 0x00});
  buffer[0] = 0xC0 | (channel & 0x0F);
  buffer[1] = i & 0x7F;
  sendMessage(buffer);
}

void MidiWrapper::noteOn(uchar channel, uchar note, uchar volume) {
  static std::vector<uchar> buffer ({0x90, 0x00, 0x00});
  buffer[0] = 0x90 | (channel & 0x0F);
  buffer[1] = note & 0x7F;
  buffer[2] = volume & 0x7F;
  sendMessage(buffer);
}

void MidiWrapper::noteOn(uchar channel, uchar note_index, float volume) {
  noteOn(channel, key(note_index), vel(volume));
}

void MidiWrapper::notesOff(uchar channel) {
  static std::vector<uchar> buffer ({0x90, 0x00, 0x00});
  buffer[0] = 0xB0 | (channel & 0x0F);
  buffer[1] = 0x7B;
  sendMessage(buffer);
}

bool MidiWrapper::writeMidi(const StaticData::NoteSheet &notes,
                            const std::string &path) {

  static constexpr auto N = StaticData::NOTES;
//  static constexpr auto S = StaticData::STEP;
  static constexpr auto C = StaticData::CHANNELS;

//  std::ofstream ofs (path, std::ios::out | std::ios::binary);
//  if (!ofs) return false;

////  uint16_t tickdiv = StaticData::STEP;

//  // header
//  ofs << "MThd"     // Identifier
//      << 0x00000006 // Chunklen (fixed to 6)
//      << 0x0000     // Format 0 -> single track midi
//      << 0x0001     // 1 track
//      << 0x0060;    // 96 ppqn (TODO improve)

//  // data
//  ofs << "MTrk";  // Identifier

  smf::MidiFile midifile;
  midifile.addTempo(0, 0, 120);
  midifile.setTicksPerQuarterNote(96);
  midifile.addTimbre(0, 0, 0, config::WatchMaker::midiInstrument());

  int tpq = midifile.getTPQ();
  for (uint n=0; n<N; n++) {
    int t = tpq * n;  /// TODO Debug
//    std::cerr << "t(" << n << ") = " << t << "\n";
    for (uint c=0; c<C; c++) {
      float v = 2 * (notes[c+C*n] - .5);
      if (n < 1 || notes[c+C*(n-1)] != v) {
        if (n > 0)  midifile.addNoteOff(0, t, 0, 60+c);
        midifile.addNoteOn(0, t, 0, key(c), vel(v));
      }
    }
  }
  midifile.sortTracks();

  midifile.write(path);

  return true;
}

} // end of namespace kgd::watchmaker::sound
