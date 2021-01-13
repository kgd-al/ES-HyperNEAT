#include <cmath>

#include <QBuffer>
#include <QAudioOutput>

#include <QFile>
#include <QDataStream>

#include "generator.h"
#include "endlessbuffer.h"

#include <QDebug>

#include <fstream>
#include <sstream>
#include <cassert>
#include <QDateTime>
#include <QTimer>

#ifndef NDEBUG
#define DEBUG_AUDIO 0
#endif

namespace kgd::watchmaker::sound {

const QVector<QString> Generator::baseNotes {
  "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#",
};

float chirp (float x) {
  static constexpr float theta_0 = 0;
  static constexpr float c = 1;
  static constexpr float f_0 = 0;

  return std::sin(theta_0 + (.5*c*x*x+f_0*x));
}

using IFunc = float (*) (float);
#define F(T, X) { Generator::T, [] (float t) -> float { X; } }
const std::map<Generator::Instrument, IFunc> instruments {
  F(   SIN, return std::sin(t)),
  F(SQUARE, return (std::fmod(t, 2*M_PI) > M_PI) ? 1.0 : -1.0),
  F(   SAW, qreal ret = fmod(t, M_PI) / (M_PI/2);
            return (fmod(t, 2*M_PI) > M_PI) ? -ret + 1 : ret = ret - 1;),
  F(   TRI, return (std::fmod(t, 2*M_PI) / M_PI) - 1),
  F( ORGAN, return .5 * (std::sin(t) + std::sin(t * 3 / 2))),
  F( CHIRP, return chirp(t)),
};
#undef F

const std::map<QString, float> Generator::notes = [] {
  #define TWO_ROOT_12         1.059463094359295
  #define A0                  27.50
  int steps = 64;
  int start = 0;

  std::map<QString, float> m;
  for (int i=0; i<steps; ++i) {
    float freq = A0 * std::pow(TWO_ROOT_12, start + i);
    int oct = ((start + i) / 12);
    QString name = Generator::baseNotes[(start + i)%12]
                  + QString::number(oct);
    m[name] = freq;
  }
  #undef TWO_ROOT_12
  #undef A0

  return m;
}();

#if 0
struct FormatData {
  static constexpr auto SS = 32;
  static constexpr auto ST = QAudioFormat::Float;
  static constexpr auto BO = QAudioFormat::LittleEndian;
  static constexpr auto B = 4;
};
#elif 0
struct FormatData {
  static constexpr auto SS = 16;
  static constexpr auto ST = QAudioFormat::SignedInt;
  static constexpr auto BO = QAudioFormat::LittleEndian;
  static constexpr auto B = 2;
};
#else
struct FormatData {
  static constexpr auto SS = 8;
  static constexpr auto ST = QAudioFormat::UnSignedInt;
  static constexpr auto BO = QAudioFormat::LittleEndian;
  static constexpr auto B = 1;
};
#endif

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
    return (format.sampleSize() == 8
            && format.sampleType() == QAudioFormat::UnSignedInt)
        || (format.sampleSize() > 8
            && format.sampleType() == QAudioFormat::SignedInt
            && format.byteOrder() == QAudioFormat::LittleEndian);
    return true;
  }
};

static constexpr auto CHANNELS = Generator::CHANNELS;
static constexpr auto NOTE_DURATION = Generator::STEP;
static constexpr auto DURATION = Generator::DURATION;

static constexpr float SAMPLE_RATE = 8000;
static constexpr float FREQ_CONST  = ((2.0 * M_PI) / SAMPLE_RATE);

static constexpr uint SAMPLES_PER_NOTE = SAMPLE_RATE * NOTE_DURATION;
static constexpr uint NOTES_PER_VOCALISATION = Generator::NOTES;
static constexpr qint64 TOTAL_SAMPLES = DURATION * SAMPLE_RATE;
static constexpr qint64 BYTES_PER_NOTE = SAMPLES_PER_NOTE * FormatData::B;
static constexpr qint64 TOTAL_BYTES_COUNT = TOTAL_SAMPLES * FormatData::B;

static_assert(DURATION / NOTE_DURATION == NOTES_PER_VOCALISATION,
              "Requested durations lead to rounding errors");

static constexpr auto TWO_ROOT_12 = 1.059463094359295;
static constexpr auto A0 = 27.50;
static const auto baseFrequency = Generator::BASE_OCTAVE * 12;
static const std::array<float, CHANNELS> frequencies = [] {
  std::array<float, CHANNELS> a;
  for (uint i=0; i<CHANNELS; i++)
    a[i] = A0 * std::pow(TWO_ROOT_12, baseFrequency + i);
  return a;
}();

struct Generator::Data {
  QByteArray bytes;
  utils::EndlessBuffer buffer;
  QAudioOutput *audio;
  static const QAudioFormat format;

  IFunc ifunc;

  Generator::PlaybackType currentPlayback;

  Data (void) : buffer(bytes) {
    bytes.resize(FormatData::B * SAMPLE_RATE * Generator::DURATION);
//    qDebug() << bytes.size() << "bytes in current buffer";

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

    // Create an output with our premade QAudioFormat (See example in QAudioOutput)
    audio = new QAudioOutput (currFormat);

    currentPlayback = LOOP;
  }

  ~Data (void) {
    delete audio;
  }
};

const QAudioFormat Generator::Data::format = [] {
  QAudioFormat format;
  format.setSampleRate(SAMPLE_RATE);
  format.setChannelCount(1);
  format.setSampleSize(FormatData::SS);
  format.setCodec("audio/pcm");
  format.setByteOrder(FormatData::BO);
  format.setSampleType(FormatData::ST);
  return format;
}();

#define TIME (QDateTime::currentDateTime().toString("hh:mm:ss,zzz"))

Generator::Generator(QObject *parent) : QObject(parent), d(new Data) {
  setInstrument(ORGAN);

  connect(d->audio, &QAudioOutput::stateChanged, [this] (QAudio::State s) {
#if DEBUG_AUDIO
    qDebug() << TIME << d->audio << "changed state to" << s;
#endif
    if (s == QAudio::IdleState) stop();
  });

  // Forwarding
  connect(d->audio, &QAudioOutput::notify, this, &Generator::notify);
  connect(d->audio, &QAudioOutput::stateChanged,
                   this, &Generator::stateChanged);

  d->buffer.setNotifyPeriod(BYTES_PER_NOTE);
  connect(&d->buffer, &utils::EndlessBuffer::notify,
          this, &Generator::onNoteConsumed);
}

Generator::~Generator(void) { delete d; }

void Generator::setInstrument(Instrument i) {
  d->ifunc = instruments.at(i);

//  for (uint c=0; c<CHANNELS; c++) {
//    std::ostringstream oss;
//    oss << "tmp/sample" << baseNotes[c%12].toStdString() << BASE_OCTAVE + c/12;
//    std::ofstream ofs (oss.str());
//    float f = frequencies.at(c) * FREQ_CONST;
//    for (uint s=0; s<SAMPLES_PER_NOTE; s++)
//      ofs << s << " " << d->ifunc(s * f) << "\n";
//  }
}

void Generator::setNoteSheet(const NoteSheet &notes) {
  assert(notes.size() == CHANNELS * NOTES);

//  std::array<std::ofstream, CHANNELS+1> streams;
//  for (uint c=0; c<CHANNELS; c++) {
//    std::ostringstream oss;
//    oss << "tmp/soundwave_channel"
//        << baseNotes[c%12].toStdString()
//        << BASE_OCTAVE + c/12;
//    streams[c].open(oss.str());
//  }
//  streams.back().open("tmp/soundwave");

  for (uint i=0; i<NOTES_PER_VOCALISATION; i++) {
    for (uint j=0; j<SAMPLES_PER_NOTE; j++) {
      float t = float(i * SAMPLES_PER_NOTE + j) * FREQ_CONST;
      float sample = 0;
      for (uint c=0; c<CHANNELS; c++) {
        float s = 0;
        auto a = notes.at(i*CHANNELS+c);

        if (a != 0)
          s = a * d->ifunc(t * frequencies.at(c));

//        streams[c] << t << " " << s << "\n";

        sample += s;
      }
      sample /= CHANNELS;
//      sample *= .5;

//      streams.back() << t << " " << sample << "\n";

      if constexpr (FormatData::B == 1)
        d->bytes[i * SAMPLES_PER_NOTE + j] = 255.f * (sample+1) / 2;

      else if (FormatData::B == 2) {
        qint16 ssample = sample * (2<<15);
        char *ptr = (char*) (&ssample);
        uint off = (i * SAMPLES_PER_NOTE + j) * FormatData::B;
        for (uint k=0; k<FormatData::B; k++)
          d->bytes[off + k] = *(ptr+k);

      } else {
        char *ptr = (char*) (&sample);
        uint off = (i * SAMPLES_PER_NOTE + j) * FormatData::B;
        for (uint k=0; k<FormatData::B; k++)
          d->bytes[off + k] = *(ptr+k);
      }
    }
  }
}

void Generator::start(PlaybackType t) {
#if DEBUG_AUDIO
  qDebug() << "\n\n" << TIME << "Starting vocalisation (looping=" << (t==LOOP)
           << ") on audio device" << d->audio << " (" << d->audio->state()
           << ")";
#endif
  d->currentPlayback = t;

  if (d->audio->state() != QAudio::StoppedState && t == ONE_NOTE)
    resume();
  else {
    d->buffer.open(QIODevice::ReadOnly, (t==LOOP));
    d->audio->start(&d->buffer);
  }
}

void Generator::stop(void) {
  d->audio->stop();
  d->buffer.close();
#if DEBUG_AUDIO
  qDebug() << TIME << "Stopped vocalisation";
#endif
}

void Generator::pause(void) {
  d->audio->suspend();
#if DEBUG_AUDIO
  qDebug() << TIME << "Suspended vocalisation";
#endif
}

void Generator::resume (void) {
  if (d->audio->state() != QAudio::SuspendedState)  d->audio->suspend();
  d->audio->resume();
#if DEBUG_AUDIO
  qDebug() << TIME << "Resuming vocalisation";
#endif
}

void Generator::generateWav(const QString &filename) const {
  if (!filename.isEmpty())
    WavPcmFile::fromRawData(filename, d->bytes, d->format);
}

void Generator::setNotifyPeriod(int ms) {
  d->audio->setNotifyInterval(ms);
}

float Generator::progress(void) const {
  static constexpr float US_DURATION = DURATION * 1000000.;
  return d->audio->processedUSecs() / US_DURATION;
}

void Generator::onNoteConsumed(void) {
#if DEBUG_AUDIO
  qDebug() << TIME << "Note consumed. Emitting signals and disconnecting";
#endif
  emit notifyNote();
  if (d->currentPlayback == ONE_NOTE) pause();
}

} // end of namespace kgd::watchmaker::sound
