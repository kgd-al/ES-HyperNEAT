#ifndef BWWINDOW_H
#define BWWINDOW_H

#include <QMainWindow>
#include <QCheckBox>

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
};

} // end of namespace simu

class BWWindow : public QMainWindow {
  Q_OBJECT
public:
  BWWindow(QWidget *parent = nullptr);

private:
  ES_HyperNEATPanel *_details;

  static constexpr uint N = 3;
  using IPtr = simu::Individual::ptr;
  std::array<IPtr, N*N> _individuals;
  uint _generation;

  std::array<sound::Visualizer*, N*N> _visualizers;
  sound::Visualizer *_selection;

  QCheckBox *_autoplay, *_fastclose;

  rng::FastDice _dice;

  void firstGeneration (void);
  void nextGeneration (void);

  void setIndividual(IPtr &&i, uint j, uint k);

  bool eventFilter(QObject *watched, QEvent *event) override;

  void showIndividualDetails (uint index);
  void setSelectedIndividual (uint index);

  void saveSettings (void) const;
  void restoreSettings (void);

  void closeEvent(QCloseEvent *e);
};

} // end of namespace gui

#endif // BWWINDOW_H
