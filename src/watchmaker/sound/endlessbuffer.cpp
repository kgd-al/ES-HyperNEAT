#include "endlessbuffer.h"

//#define DEBUG_EB 1
#ifdef DEBUG_EB
#include <QDebug>
#endif

namespace kgd::watchmaker::utils {

EndlessBuffer::EndlessBuffer (QByteArray &array, QObject *parent)
  : QIODevice(parent), _data(array), _index(0), _looping(false),
    _notificationPeriod(0), _notificationBuffer(0) {}

bool EndlessBuffer::isSequential(void) const {
  return false;
}

void EndlessBuffer::open (OpenMode openMode, bool looping) {
  QIODevice::open(openMode | QIODevice::Unbuffered);
  _looping = looping;
  _index = 0;
  _notificationBuffer = 0;
}

void EndlessBuffer::setNotifyPeriod(qint64 period) {
  _notificationPeriod = period;
}

bool EndlessBuffer::atEnd(void) const {
  return !_looping && _data.size() <= _index;
}

qint64 EndlessBuffer::bytesAvailable(void) const {
  if (_looping)
    return std::numeric_limits<quint64>::max();
  else
    return QIODevice::bytesAvailable() + _data.size() - _index;
}

qint64 EndlessBuffer::readData(char *data, qint64 len) {
#if DEBUG_EB
  qDebug() << "readData(" << len << ")";
#endif

  // Loop mode -> always more data to read
  if (_looping) {
    // But maybe in two chunks
    qint64 rlen = std::min(len, qint64(_data.size() - _index));
    memcpy(data, _data.constData() + _index, rlen);
#if DEBUG_EB > 1
    qDebug() << "\tread" << _index << "+" << rlen << "bytes";
#endif
    if (rlen < len) {
#if DEBUG_EB > 1
      qDebug() << "\tand then" << len-rlen << "from the start";
#endif
      memcpy(data+rlen, _data.constData(), len-rlen);
    }

    _index = (_index+len)%_data.size();

  } else {
    if ((len = std::min(len, qint64(_data.size()) - _index)) <= 0)
          len = 0;
    else  memcpy(data, _data.constData() + _index, len);
    _index += len;
  }

#if DEBUG_EB
  qDebug() << len << "bytes read. " << bytesAvailable() << "remaining";
#endif

  if (_notificationPeriod > 0) {
    _notificationBuffer += len;
    if (_notificationPeriod <= _notificationBuffer) {
      _notificationBuffer %= _notificationPeriod;
      emit notify();
    }
  }

  return len;
}

qint64 EndlessBuffer::writeData(const char* /*data*/, qint64 /*len*/) {
  return -1;
}

bool EndlessBuffer::open(OpenMode) {
  return false;
}

} // end of namespace kgd::watchmaker::utils
