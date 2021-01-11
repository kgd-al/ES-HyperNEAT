#ifndef BWWINDOW_H
#define BWWINDOW_H

#include <QMainWindow>
#include <QCheckBox>
#include <QMap>

#include "es_hyperneatpanel.h"
#include "../sound/visualizer.h"

namespace gui {

namespace simu {

struct Individual {
  using Genome = genotype::ES_HyperNEAT;
  Genome genome;

  using CPPN = phenotype::CPPN;
  CPPN cppn;

  using ANN = phenotype::ANN;
  ANN ann;

  using ptr = std::unique_ptr<Individual>;
  static ptr random (rng::AbstractDice &d);
  static ptr mutated (const Individual &i, rng::AbstractDice &d);

private:
  Individual (const Genome &g, const CPPN &c, const ANN &a);

  static ptr fromGenome (const Genome &g);
};

} // end of namespace simu

class BWWindow : public QMainWindow {
  Q_OBJECT
public:
  BWWindow(QWidget *parent = nullptr, const stdfs::path &baseSavePath = "");

  enum Setting {
    AUTOPLAY, MANUAL_PLAY, STEP_PLAY,
    LOCK_SELECTION, SELECT_NEXT, PLAY,
    FAST_CLOSE
  };
  Q_ENUM(Setting)

private:
  ES_HyperNEATPanel *_details;

  static constexpr uint N = 3;
  static_assert(N%2 == 1, "Grid size must be odd");

  using IPtr = simu::Individual::ptr;
  std::array<IPtr, N*N> _individuals;
  uint _generation;

  std::array<sound::Visualizer*, N*N> _visualizers;
  sound::Visualizer *_shown, *_selection;

  QMap<Setting, QCheckBox*> _settings;

  rng::FastDice _dice;

  stdfs::path _baseSavePath, _currentSavePath;

  void firstGeneration (void);
  void nextGeneration (uint index);
  void updateSavePath (void);

  void setIndividual(IPtr &&in, uint j, uint k);

  bool eventFilter(QObject *watched, QEvent *event) override;
  void individualHoverEnter (uint index);
  void individualHoverLeave (uint index);
  void individualMouseClick (uint index);
  void individualMouseDoubleClick (uint index);

  void showIndividualDetails (int index);
  void setSelectedIndividual (int index);

  bool setting (Setting s) const;
  void saveSettings (void) const;
  void restoreSettings (void);

  void closeEvent(QCloseEvent *e);
};

} // end of namespace gui

#endif // BWWINDOW_H
