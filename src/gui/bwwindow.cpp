#include <QGridLayout>
#include <QHoverEvent>
#include <QSplitter>
#include <QMessageBox>
#include <QSettings>

#include "bwwindow.h"
#include "kgd/settings/configfile.h"

#include <QDebug>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QMetaEnum>

namespace gui {

namespace simu {

Individual::Individual(const Genome &g, const CPPN &c, const ANN &a)
  : genome(g), cppn(c), ann(a) {}

Individual::ptr Individual::fromGenome (const genotype::ES_HyperNEAT &genome) {
  auto cppn = phenotype::CPPN::fromGenotype(genome);

  phenotype::ANN::Point bias {0,-.9};
  phenotype::ANN::Coordinates inputs, outputs;
  static constexpr auto C = sound::Generator::CHANNELS;
  for (uint i=0; i<C; i++) {
    inputs.push_back( { float(i) / (C-1) - .5f, -1 });
    outputs.push_back({ float(i) / (C-1) - .5f, +1 });
  }

  auto ann = phenotype::ANN::build(bias, inputs, outputs, genome, cppn);

  return ptr(new Individual(genome, cppn, ann));
}

Individual::ptr Individual::random(rng::AbstractDice &d) {
  return fromGenome(genotype::ES_HyperNEAT::random(d));
}

Individual::ptr Individual::mutated (const Individual &i, rng::AbstractDice &d){
  auto genome = i.genome;
#ifndef NDEBUG
  assertEqual(i.genome, genome, true);
#endif
  genome.mutate(d);
  return fromGenome(genome);
}

} // end of namespace simu

BWWindow::BWWindow(QWidget *parent, const stdfs::path &baseSavePath)
  : QMainWindow(parent), _dice(1), _baseSavePath(baseSavePath) {
//  _sounds = new SoundGenerator(this);

  auto splitter = new QSplitter;
  auto holder = new QWidget;
  auto layout = new QGridLayout;

  for (uint i=0; i<N; i++)
    for (uint j=0; j<N; j++)
      layout->addWidget(_visualizers[i*N+j] = new sound::Visualizer, i, j);
  for (auto v: _visualizers)
    v->QWidget::installEventFilter(this);
  _shown = nullptr;
  _selection = nullptr;

  auto *clayout = new QVBoxLayout;
  clayout->addWidget(_autoplay = new QCheckBox ("Autoplay"));
  clayout->addWidget(_fastclose = new QCheckBox ("Fast close"));
  layout->addLayout(clayout, N, 0, 1, N, Qt::AlignCenter);

  holder->setLayout(layout);
  splitter->addWidget(holder);
  splitter->addWidget(_details = new ES_HyperNEATPanel);
  setCentralWidget(splitter);

  restoreSettings();

  firstGeneration();
}

void BWWindow::firstGeneration(void) {
  if (_baseSavePath.empty())  _baseSavePath = "./tmp/";

  std::ostringstream oss;
  oss << utils::CurrentTime("%Y-%m-%d_%H-%M-%S");
  _baseSavePath /= oss.str();
  stdfs::create_directories(_baseSavePath);
  _baseSavePath = stdfs::canonical(_baseSavePath);
  std::cerr << "baseSavePath: " << _baseSavePath << std::endl;

  _generation = 0;
  for (uint i=0; i<N; i++)
    for (uint j=0; j<N; j++)
      setIndividual(simu::Individual::random(_dice), i, j);
  showIndividualDetails(1);
  showIndividualDetails(0);
  showIndividualDetails(N*N/2);
}

void BWWindow::nextGeneration(uint index) {
  setSelectedIndividual(-1);
  showIndividualDetails(-1);
  IPtr parent = std::move(_individuals[index]);
  uint mid = N/2;
  for (uint i=0; i<N; i++) {
    for (uint j=0; j<N; j++) {
      if (i == mid && j == mid)  continue;
      setIndividual(simu::Individual::mutated(*parent, _dice), i, j);
    }
  }
  setIndividual(std::move(parent), mid, mid);
  showIndividualDetails(N*N/2);
}

void BWWindow::setIndividual(IPtr &&i, uint j, uint k) {
  static constexpr auto C = sound::Generator::CHANNELS;

  auto &ann = i->ann;
  _individuals[j*N+k] = std::move(i);

  auto v = _visualizers[j*N+k];
  sound::Visualizer::NoteSheet notes;

  auto inputs = ann.inputs();
  auto outputs = ann.outputs();
  assert(inputs.size() == outputs.size());

  std::fill(inputs.begin(), inputs.end(), 0);
  for (uint n=0; n<sound::Generator::NOTES; n++) {
    ann(inputs, outputs);
    for (uint c=0; c<C; c++)  notes[n*C+c] = outputs[c];
    inputs = outputs;
  }
  v->setNoteSheet(notes);
}

bool BWWindow::eventFilter(QObject *watched, QEvent *event) {
  static const QList<QEvent::Type> types {
    QEvent::HoverEnter, QEvent::HoverLeave,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick
  };

  auto *v = dynamic_cast<sound::Visualizer*>(watched);
  if (!v) return false;

  if (!types.contains(event->type())) return false;

  int index = -1;
  for (uint i=0; i<_visualizers.size() && index < 0; i++)
    if (_visualizers[i] == v)
      index = i;

  if (index >= 0) {
    switch (event->type()) {
    case QEvent::HoverEnter:
      showIndividualDetails(index);
      if (_autoplay->isChecked()) v->vocaliseToAudioOut();
      break;
    case QEvent::HoverLeave:
      if (_autoplay->isChecked()) v->stopVocalisationToAudioOut();
      break;
    case QEvent::MouseButtonRelease:
      if (auto *me = static_cast<QMouseEvent*>(event))
        if (me->modifiers() == Qt::ControlModifier)
          setSelectedIndividual(index);
      break;
    case QEvent::MouseButtonDblClick:
      nextGeneration(index);
      break;
    default:  break;
    }
  }

  return false;
}

void BWWindow::showIndividualDetails(int index) {
  if (_selection)  return;
  std::cerr << "Showing details of individual at index " << index << "\n";

  if (_shown) _shown->setHighlighted(false);

  if (index < 0)
    _details->noData();

  else {
    auto &i = _individuals.at(index);
    _details->setData(i->genome, i->cppn, i->ann);
    _shown = _visualizers[index];
    _shown->setHighlighted(true);
  }
}

void BWWindow::setSelectedIndividual(int index) {
  sound::Visualizer *v = (index >= 0) ? _visualizers[index] : nullptr;
  bool deselect = (v == nullptr || _selection == v);

  if (_selection) _selection = nullptr;
  if (!deselect) {
    showIndividualDetails(index);
    _selection = v;
  }
}

void BWWindow::saveSettings(void) const {
  QSettings settings;
  settings.setValue("geom", saveGeometry());
  settings.setValue("autoplay", _autoplay->isChecked());
  settings.setValue("fastclose", _fastclose->isChecked());
}

void BWWindow::restoreSettings(void) {
  QSettings settings;
  restoreGeometry(settings.value("geom").toByteArray());
  _autoplay->setChecked(settings.value("autoplay").toBool());
  _fastclose->setChecked(settings.value("fastclose").toBool());
}

void BWWindow::closeEvent(QCloseEvent *e) {
  if (_fastclose->isChecked()
      || QMessageBox::question(this, "Closing", "Confirm closing?",
                               QMessageBox::Yes, QMessageBox::No)
        == QMessageBox::Yes) {
    saveSettings();
    e->accept();

  } else
    e->ignore();
}

} // end of namespace gui
