#include <QApplication>

#include "kgd/external/cxxopts.hpp"

#include "bwwindow.h"
#include "config.h"

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

  stdfs::path outputFolder = "";
  int seed = -1;

  stdfs::path baseCPPN = "";

  cxxopts::Options options("SongMaker (ES-HyperNEAT)",
                           "Test environment for personnal implementation of "
                           "the ES-HyperNEAT core algorithm (i.e. ann "
                           "generation without evolutionary algorithmic "
                           "components) on a sound generation user-driven "
                           "task");
  options.add_options()
    ("h,help", "Display help")
    ("a,auto-config", "Load configuration data from default location")
    ("c,config", "File containing configuration data",
     cxxopts::value(configFile))
    ("v,verbosity", "Verbosity level. " + config::verbosityValues(),
     cxxopts::value(verbosity))

    ("s,seed", "Seed value for simulation's RNG", cxxopts::value(seed))
    ("o,out-folder", "Folder under which to store the computational outputs",
     cxxopts::value(outputFolder))
    ("b,basis", "Basis for initial generation (either full genome or dot "
                "file describing the cppn)",
     cxxopts::value(baseCPPN))
    ;

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if (result.count("auto-config") && result["auto-config"].as<bool>())
    configFile = "auto";

  using namespace kgd::watchmaker::gui;
  outputFolder = BWWindow::generateOutputFolder (outputFolder);

  if (verbosity != Verbosity::QUIET) {
    genotype::ES_HyperNEAT::printMutationRates(std::cout, 3);
    config::WatchMaker::printConfig(outputFolder / "config/");
    config::WatchMaker::printConfig();
  }

  if (seed < 0) seed = rng::FastDice::currentMilliTime();

  kgd::watchmaker::sound::MidiWrapper::initialize();


  BWWindow w (outputFolder, seed);
  w.firstGeneration(baseCPPN);
  w.show();

  return app.exec();
}
