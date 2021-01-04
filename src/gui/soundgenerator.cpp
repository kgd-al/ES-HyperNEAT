#include <cmath>

#include <QBuffer>
#include <QAudioOutput>

#include <QFile>
#include <QDataStream>

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

struct FormatData {
  static constexpr auto SS = 32;
  static constexpr auto ST = QAudioFormat::Float;
  static constexpr auto BO = QAudioFormat::LittleEndian;
  static constexpr auto B = QAudioFormat::LittleEndian;
};

class WavPcmFile : public QFile {
public:
  static void fromRawData (const QString &filename, const QByteArray &bytes,
                           const QAudioFormat &format) {
    WavPcmFile file (filename);

    if (!hasSupportedFormat(format)) {
      qWarning("Failed to write raw wav/pcm file: unsupported format");
      return;
    }

    if (!file.open(WriteOnly | Truncate)) {
      qWarning("Failed to write raw wav/pcm file '%s': %s",
               filename.toStdString().c_str(),
               file.errorString().toStdString().c_str());
      return;
    }

    file.writeHeader(format);

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out << bytes;

    file.close();
  }

private:
  WavPcmFile(const QString &name, QObject *parent = 0) : QFile(name, parent) {}

  void close(void) {
    // Fill the header size placeholders
    quint32 fileSize = size();

    QDataStream out(this);
    // Set the same ByteOrder like in writeHeader()
    out.setByteOrder(QDataStream::LittleEndian);
    // RIFF chunk size
    seek(4);
    out << quint32(fileSize - 8);

    // data chunk size
    seek(40);
    out << quint32(fileSize - 44);

    QFile::close();
  }

  void writeHeader(const QAudioFormat &format) {
    QDataStream out(this);
    out.setByteOrder(QDataStream::LittleEndian);

    // RIFF chunk
    out.writeRawData("RIFF", 4);
    out << quint32(0); // Placeholder for the RIFF chunk size (filled by close())
    out.writeRawData("WAVE", 4);

    // Format description chunk
    out.writeRawData("fmt ", 4);
    out << quint32(16); // "fmt " chunk size (always 16 for PCM)
    out << quint16(1); // data format (1 => PCM)
    out << quint16(format.channelCount());
    out << quint32(format.sampleRate());
    out << quint32(format.sampleRate() * format.channelCount()
    * format.sampleSize() / 8 ); // bytes per second
    out << quint16(format.channelCount() * format.sampleSize() / 8); // Block align
    out << quint16(format.sampleSize()); // Significant Bits Per Sample

    // Data chunk
    out.writeRawData("data", 4);
    out << quint32(0); // Placeholder for the data chunk size (filled by close())

    Q_ASSERT(pos() == 44); // Must be 44 for WAV PCM
  }

  static bool hasSupportedFormat(const QAudioFormat &format) {
//    return (format.sampleSize() == 8
//            && format.sampleType() == QAudioFormat::UnSignedInt)
//        || (format.sampleSize() > 8
//            && format.sampleType() == QAudioFormat::SignedInt
//            && format.byteOrder() == QAudioFormat::LittleEndian);
    return true;
  }
};

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
  static const QAudioFormat format;

  IFunc ifunc;
  std::array<std::array<float, SAMPLES_PER_NOTE>,
             SoundGenerator::CHANNELS> samples;

  Data (void) {
    bytes.resize(/*FormatData::B * */SAMPLE_RATE * SoundGenerator::DURATION);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    QAudioFormat currFormat = format;
    if (!info.isFormatSupported(format)) {
        qDebug() << "Requested format" << format << "is not supported";
        currFormat = info.nearestFormat(format);
        qDebug() << "Nearest alternative might work: " << currFormat << "\n"
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
    audio = new QAudioOutput (currFormat);
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

const QAudioFormat SoundGenerator::Data::format = [] {
  QAudioFormat format;
  format.setSampleRate(SAMPLE_RATE);
  format.setChannelCount(1);
  format.setSampleSize(FormatData::SS);
  format.setCodec("audio/pcm");
  format.setByteOrder(FormatData::BO);
  format.setSampleType(FormatData::ST);
  return format;
}();

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
//      a[s] = d->ifunc(s * f);
      a[s] = .5 * (d->ifunc(s * f) + d->ifunc(s * f * 3 / 2));
      ofs << s << " " << s*f << " " << a[s] << "\n";
    }
  }
}

void SoundGenerator::vocalisation(const std::vector<float> &input,
                                  const QString &filename) {
  assert(input.size() == CHANNELS * DURATION / STEP);

  std::array<std::ofstream, CHANNELS+1> streams;
  for (uint c=0; c<CHANNELS; c++) {
    std::ostringstream oss;
    oss << "tmp/soundwave_channel" << baseNotes[c%12].toStdString();
    streams[c].open(oss.str());
  }
  streams.back().open("tmp/soundwave");

  const auto transition = [&input] (uint i, uint c, uint s) -> float {
    static constexpr uint S = Data::SAMPLES_PER_NOTE;
    static constexpr uint ds = S * 0.25;

    return 1.f;

  //    if (0 < i
  //        && i < Data::NOTES_PER_VOCALISATION-1
  //        && int(input[i*CHANNELS+c]) == int(input[(i+1)*CHANNELS+c]))
  //      return 1.f;
  //    if (s < ds) return float(s) / ds;
  //    else if (S - s < ds)  return float(S - s) / ds;
  //    else  return 1.f;
      if (s < ds
          && ((0 == i) || int(input[(i-1)*CHANNELS+c]) != int(input[i*CHANNELS+c])))
        return float(s) / ds;
      if (S - s < ds
          && ((i == Data::NOTES_PER_VOCALISATION-1)
            || ((0 == i) || int(input[i*CHANNELS+c]) != int(input[(i+1)*CHANNELS+c]))))
        return float(S - s) / ds;
      return 1.f;
  };

  for (uint i=0; i<Data::NOTES_PER_VOCALISATION; i++) {
    for (uint j=0; j<Data::SAMPLES_PER_NOTE; j++) {
      float sample = 0;
      for (uint c=0; c<CHANNELS; c++) {
        float s = 0;
        auto a = input.at(i*CHANNELS+c);
        float b = transition(i, c, j);

        if (a != 0) s = a * b * d->samples[c][j];

        streams[c] << s << "\n";

        sample += s;
      }
      sample /= CHANNELS;
      sample *= .5;

      streams.back() << sample << "\n";

      char *ptr = (char*) (&sample);
      uint off = (i * Data::SAMPLES_PER_NOTE + j) * FormatData::B;
      for (uint k=0; k<FormatData::B; k++) {
        d->bytes[off + k] = *(ptr+k);
        qDebug() << "d->bytes[" << off << "+" << k << "] = " << (int)*(ptr+k);
      }
    }
  }

  qDebug() << d->bytes.size() << "bytes in current buffer";

  d->buffer.open(QIODevice::ReadOnly);
  d->audio->start(&d->buffer);

  if (!filename.isEmpty())
    WavPcmFile::fromRawData(filename, d->bytes, d->format);
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
