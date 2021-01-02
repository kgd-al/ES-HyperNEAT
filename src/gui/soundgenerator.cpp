#include <cmath>

#include <QBuffer>
#include <QAudioOutput>

#include "soundgenerator.h"

#include <QDebug>

#include <fstream>

namespace gui {

const QVector<QString> SoundGenerator::baseNotes {
  "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#",
};

#define F(T, X) { SoundGenerator::T, [] (float x) -> float { X; } }
const std::map<SoundGenerator::Instrument, float (*) (float)>
SoundGenerator::instruments {
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

  return m;
}();

struct SoundGenerator::Data {
  static constexpr float SAMPLE_RATE = 8000;
  static constexpr float FREQ_CONST  = ((2.0 * M_PI) / SAMPLE_RATE);
  static constexpr float SAMPLE_SIZE = 32;
  static constexpr auto  SAMPLE_TYPE = QAudioFormat::Float;
  static constexpr auto  CODEC_TYPE  = "audio/pcm";
  static constexpr auto  CHANNELS    = 1;

  QByteArray bytes;
  QBuffer buffer;
  QAudioOutput *audio;

  Data (void) {
    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(CHANNELS);
    format.setSampleSize(SAMPLE_SIZE);
    format.setCodec(CODEC_TYPE);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(SAMPLE_TYPE);

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
      qDebug() << audio << "changed state to" << s;
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

SoundGenerator::SoundGenerator(QObject *parent)
  : QObject(parent), d(new Data) {}
SoundGenerator::~SoundGenerator(void) { delete d; }

void SoundGenerator::make_sound(Instrument ins, float frequency, float seconds) {
//  qDebug() << "generating sound of frequency" << frequency << "for"
//           << seconds << "seconds";

  const auto samples = Data::SAMPLE_RATE * seconds;

  d->bytes.resize(sizeof(float) * samples);

  std::ofstream ofs ("tmp/soundwave");
  for (int i=0; i<samples; i++) {
    float sample = instruments.at(ins)(frequency * i * Data::FREQ_CONST);
    ofs << sample << "\n";
    char *ptr = (char*) (&sample);
    for (uint j=0; j<4; j++)
      d->bytes[4 * i + j] = *(ptr+j);
  }

//  // Make a QBuffer from our QByteArray
//  QBuffer* input = new QBuffer(byteBuffer);
//  input->open(QIODevice::ReadOnly);

//  {
//    auto q = qDebug();
//    q << "buffer:" << input->buffer();
//  }



  qDebug() << d->audio << "processing" << d->buffer.size() << "bytes";
  d->buffer.open(QIODevice::ReadOnly);
  d->audio->start(&d->buffer);
}

//void SoundGenerator::make_sound(float frequency, float seconds) {
//  try {
//    RtMidiOut midi;
//  } catch (RtMidiError &error) {
//    // Handle the exception here
//    error.printMessage();
//  }
//}

} // end of namespace gui
