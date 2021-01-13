#ifndef KGD_ENDLESSBUFFER_H
#define KGD_ENDLESSBUFFER_H

#include <QIODevice>

namespace watchmaker::utils {

class EndlessBuffer : public QIODevice {
  Q_OBJECT

  QByteArray &_data;
  qint64 _index;

  bool _looping;

  qint64 _notificationPeriod, _notificationBuffer;

public:
  EndlessBuffer (QByteArray &array, QObject *parent = nullptr);

  bool isSequential(void) const override;

  bool looping (void) const {
    return _looping;
  }

  void open (OpenMode openMode, bool looping);
  void setNotifyPeriod (qint64 period);

  bool atEnd(void) const override;

  qint64 bytesAvailable(void) const override;

  qint64 readData(char *data, qint64 len);

  qint64 writeData(const char* /*data*/, qint64 /*len*/) override;

signals:
  void notify (void);

private:
  bool open (OpenMode /*openMode*/) override;
};

} // end of namespace watchmaker::utils

#endif // KGD_ENDLESSBUFFER_H
