#include <cmath>

#include <QBuffer>
#include <QAudioOutput>

#include "soundgenerator.h"

#include <QDebug>

#include <fstream>
#include <sstream>
#include <cassert>
#include <QDateTime>

namespace gui {

const QVector<QString> SoundGenerator::baseNotes {
  "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#",
};

using IFunc = float (*) (float);
#define F(T, X) { SoundGenerator::T, [] (float x) -> float { X; } }
const std::map<SoundGenerator::Instrument, IFunc> SoundGenerator::instruments {
  F(   SIN, return std::sin(x)),
  F(SQUARE, return (std::fmod(x, 2*M_PI) > M_PI) ? 1.0 : -1.0),
  F(   SAW, qreal ret = fmod(x, M_PI) / (M_PI/2);
            if (fmod(x, 2*M_PI) > M_PI) {
                ret = -ret + 1;
            } else {
                ret = ret - 1;
            }
            return ret),
  F(   TRI, return (std::fmod(x, 2*M_PI) / M_PI) - 1),
};
#undef F

const std::map<QString, float> SoundGenerator::notes = [] {
  #define TWO_ROOT_12         1.059463094359295
  #define A0                  27.50
  int steps = 64;
  int start = 0;

  std::map<QString, float> m;
  for (int i=0; i<steps; ++i) {
    float freq = A0 * std::pow(TWO_ROOT_12, start + i);
    int oct = ((start + i) / 12);
    QString name = SoundGenerator::baseNotes[(start + i)%12]
                  + QString::number(oct);
    m[name] = freq;
  }
  #undef TWO_ROOT_12
  #undef A0

  return m;
}();

struct SoundGenerator::Data {
  static constexpr auto CHANNELS = SoundGenerator::CHANNELS;
  static constexpr auto NOTE_DURATION = SoundGenerator::STEP;
  static constexpr auto DURATION = SoundGenerator::DURATION;

  static constexpr float SAMPLE_RATE = 8000;
  static constexpr float FREQ_CONST  = ((2.0 * M_PI) / SAMPLE_RATE);

  static constexpr uint SAMPLES_PER_NOTE = SAMPLE_RATE * NOTE_DURATION;
  static constexpr uint NOTES_PER_VOCALISATION = DURATION / STEP;
  static_assert(DURATION / NOTE_DURATION == NOTES_PER_VOCALISATION,
                "Requested durations lead to rounding errors");

  static constexpr auto TWO_ROOT_12 = 1.059463094359295;
  static constexpr auto A0 = 27.50;
  static const auto baseFrequency = BASE_OCTAVE * 12;
  static const std::array<float, CHANNELS> frequencies;

  QByteArray bytes;
  QBuffer buffer;
  QAudioOutput *audio;

  IFunc ifunc;
  std::array<std::array<float, SAMPLES_PER_NOTE>,
             SoundGenerator::CHANNELS> samples;

  Data (void) {
    bytes.resize(SAMPLE_RATE * SoundGenerator::DURATION);

    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(1);
    format.setSampleSize(32);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::Float);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qDebug() << "Requested format" << format << "is not supported";
        format = info.nearestFormat(format);
        qDebug() << "Nearest alternative might work: " << format << "\n"
                 << "Please use the following:\n"
                 << "\t    ByteOrder: " << info.supportedByteOrders() << "\n"
                 << "\tChannelCounts: " << info.supportedChannelCounts() << "\n"
                 << "\t       Codecs: " << info.supportedCodecs() << "\n"
                 << "\t  SampleRates: " << info.supportedSampleRates() << "\n"
                 << "\t  SampleSizes: " << info.supportedSampleSizes() << "\n"
                 << "\t  SampleTypes: " << info.supportedSampleTypes() << "\n";
    }

    buffer.setBuffer(&bytes);

    // Create an output with our premade QAudioFormat (See example in QAudioOutput)
    audio = new QAudioOutput (format);
    QObject::connect(audio, &QAudioOutput::stateChanged,
                     [this] (QAudio::State s) {
      qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss,zzz")
               << audio << "changed state to" << s;
      if (s == QAudio::IdleState) {
        audio->stop();
        buffer.close();
  //      audio->deleteLater();
      }
    });
  }

  ~Data (void) {
    delete audio;
  }
};

const std::array<float, SoundGenerator::CHANNELS>
SoundGenerator::Data::frequencies = [] {
  std::array<float, CHANNELS> a;
  for (uint i=0; i<CHANNELS; i++)
    a[i] = A0 * std::pow(TWO_ROOT_12, baseFrequency + i);
  return a;
}();


SoundGenerator::SoundGenerator(QObject *parent)
  : QObject(parent), d(new Data) {
  setInstrument(SAW);
}
SoundGenerator::~SoundGenerator(void) { delete d; }

void SoundGenerator::setInstrument(Instrument i) {
  d->ifunc = instruments.at(i);
  for (uint c=0; c<CHANNELS; c++) {
    std::ostringstream oss;
    oss << "tmp/sample" << baseNotes[c%12].toStdString();
    std::ofstream ofs (oss.str());
    auto &a = d->samples[c];
    float f = Data::frequencies.at(c) * Data::FREQ_CONST;
    for (uint s=0; s<Data::SAMPLES_PER_NOTE; s++) {
      a[s] = d->ifunc(s * f);
      ofs << s << " " << s*f << " " << a[s] << "\n";
    }
  }
}

void SoundGenerator::vocalisation(const std::vector<float> &input) {
  assert(input.size() == CHANNELS * DURATION / STEP);

  std::array<std::ofstream, CHANNELS+1> streams;
  for (uint c=0; c<CHANNELS; c++) {
    std::ostringstream oss;
    oss << "tmp/soundwave_channel" << baseNotes[c%12].toStdString();
    streams[c].open(oss.str());
  }
  streams.back().open("tmp/soundwave");

  for (uint i=0; i<Data::NOTES_PER_VOCALISATION; i++) {
    for (uint j=0; j<Data::SAMPLES_PER_NOTE; j++) {
      float sample = 0;
      for (uint c=0; c<CHANNELS; c++) {
        float s = 0;
        auto a = input.at(i*CHANNELS+c);
        if (a != 0) s = a * d->samples[c][j];

        streams[c] << a * d->samples[c][j] << "\n";

        sample += s;
      }
      sample /= CHANNELS;

      streams.back() << sample << "\n";

      char *ptr = (char*) (&sample);
      uint off = (i * Data::SAMPLES_PER_NOTE + j) * 4;
      for (uint k=0; k<4; k++)
        d->bytes[off + k] = *(ptr+k);
    }
  }

  qDebug() << d->bytes.size() << "bytes in current buffer";

  d->buffer.open(QIODevice::ReadOnly);
  d->audio->start(&d->buffer);
}

//void SoundGenerator::make_sound(Instrument ins, float frequency, float seconds) {
////  qDebug() << "generating sound of frequency" << frequency << "for"
////           << seconds << "seconds";

//  const auto samples = Data::SAMPLE_RATE * seconds;

//  d->bytes.resize(sizeof(float) * samples);

//  std::ofstream ofs ("tmp/soundwave");
//  for (int i=0; i<samples; i++) {
//    float sample = instruments.at(ins)(frequency * i * Data::FREQ_CONST);
//    ofs << sample << "\n";
//    char *ptr = (char*) (&sample);
//    for (uint j=0; j<4; j++)
//      d->bytes[4 * i + j] = *(ptr+j);
//  }

////  // Make a QBuffer from our QByteArray
////  QBuffer* input = new QBuffer(byteBuffer);
////  input->open(QIODevice::ReadOnly);

////  {
////    auto q = qDebug();
////    q << "buffer:" << input->buffer();
////  }



//  qDebug() << d->audio << "processing" << d->buffer.size() << "bytes";
//  d->buffer.open(QIODevice::ReadOnly);
//  d->audio->start(&d->buffer);
//}

//void SoundGenerator::make_sound(float frequency, float seconds) {
//  try {
//    RtMidiOut midi;
//  } catch (RtMidiError &error) {
//    // Handle the exception here
//    error.printMessage();
//  }
//}

} // end of namespace gui
