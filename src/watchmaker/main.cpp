#include <QApplication>

#include "kgd/external/cxxopts.hpp"

#include "bwwindow.h"

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

  using Verbosity = config::Verbosity;

  std::string configFile = "auto";  // Default to auto-config
  Verbosity verbosity = Verbosity::QUIET;

  std::string outputFolder = "";
  int seed = -1;

  cxxopts::Options options("Splinoids (visualisation)",
                             "2D simulation of critters in a changing environment");
  options.add_options()
    ("h,help", "Display help")
//    ("a,auto-config", "Load configuration data from default location")
//    ("c,config", "File containing configuration data",
//     cxxopts::value(configFile))
//    ("v,verbosity", "Verbosity level. " + config::verbosityValues(),
//     cxxopts::value(verbosity))

    ("s,seed", "Seed value for simulation's RNG", cxxopts::value(seed))
    ("o,out-folder", "Folder under which to store the computational outputs",
     cxxopts::value(outputFolder));

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if (result.count("auto-config") && result["auto-config"].as<bool>())
    configFile = "auto";

  gui::BWWindow w (outputFolder, seed);
  w.show();

  return app.exec();
}
