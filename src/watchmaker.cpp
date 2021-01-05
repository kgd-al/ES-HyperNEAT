#include <QApplication>

#include "gui/bwwindow.h"

int main (int argc, char **argv) {
  QApplication app (argc, argv);

  QApplication::setOrganizationName("kgd-al");
  QApplication::setApplicationName("blind-songmaker");
  QApplication::setApplicationDisplayName("Blind SongMaker");
  QApplication::setApplicationDisplayName("Blind SongMaker");
  QApplication::setApplicationVersion("0.0.0");
  QApplication::setDesktopFileName(
    QApplication::organizationName() + "-" + QApplication::applicationName()
    + ".desktop");

  gui::BWWindow w;
  w.show();

  return app.exec();
}
