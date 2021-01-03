#ifndef BWWINDOW_H_H
#define BWWINDOW_H_H

#include <QMainWindow>

#include "es_hyperneatpanel.h"
#include "soundgenerator.h"

namespace gui {

class BWWindow : public QMainWindow {
public:
  BWWindow(QWidget *parent = nullptr);

private:
  ES_HyperNEATPanel *_details;
  SoundGenerator *_sounds;
};

} // end of namespace gui

#endif // BWWINDOW_H_H
