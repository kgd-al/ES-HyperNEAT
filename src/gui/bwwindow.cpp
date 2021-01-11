#include <QGridLayout>
#include <QHoverEvent>
#include <QSplitter>
#include <QMessageBox>
#include <QSettings>
#include <QButtonGroup>

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

  auto *clayout = new QGridLayout;
  {
    uint r = 0, c = 0;
    auto add = [&r, &c, clayout] (QWidget *w) {
      clayout->addWidget(w, r++, c);
      if (r == 4) r = 0, c++;
    };
    QMetaEnum e = QMetaEnum::fromType<Setting>();
    for (int i=0; i<e.keyCount(); i++) {
      Setting s = Setting(e.value(i));
      if (s == AUTOPLAY)  add(new QLabel("Sound"));
      else if (s == LOCK_SELECTION) add(new QLabel ("LMouse"));
      else if (s == FAST_CLOSE) add(new QLabel ("Other"));

      QString name = QString(e.key(i)).replace("_", " ").toLower();
      name[0] = name[0].toUpper();
      add(_settings[s] = new QCheckBox (name));
    }

    QButtonGroup *gp = new QButtonGroup(this);
    for (Setting s: {AUTOPLAY,MANUAL_PLAY,STEP_PLAY})
      gp->addButton(_settings.value(s));

    QButtonGroup *gm = new QButtonGroup(this);
    for (Setting s: {LOCK_SELECTION,SELECT_NEXT,PLAY})
      gm->addButton(_settings.value(s));
  }
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
  auto link = _baseSavePath / "last";
  _baseSavePath /= oss.str();
  stdfs::create_directories(_baseSavePath);
  _baseSavePath = stdfs::canonical(_baseSavePath);

  if (stdfs::exists(link))  stdfs::remove(link);
  stdfs::create_directory_symlink(_baseSavePath, link);

  std::cerr << "baseSavePath: " << _baseSavePath << std::endl;

  _generation = 0;
  updateSavePath();

  for (uint i=0; i<N; i++)
    for (uint j=0; j<N; j++)
      setIndividual(simu::Individual::random(_dice), i, j);
  showIndividualDetails(1);
  showIndividualDetails(0);
  showIndividualDetails(N*N/2);
  showIndividualDetails(7);
}

void BWWindow::nextGeneration(uint index) {
  setSelectedIndividual(-1);
  showIndividualDetails(-1);

  _generation++;
  updateSavePath();

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

void BWWindow::updateSavePath(void) {
  std::ostringstream oss;
  oss << "gen" << _generation;
  _currentSavePath = _baseSavePath / oss.str();
  stdfs::create_directories(_currentSavePath);
}

void BWWindow::setIndividual(IPtr &&in, uint j, uint k) {
  static constexpr auto C = sound::Generator::CHANNELS;


  simu::Individual &i = *(_individuals[j*N+k] = std::move(in)).get();
  auto &ann = i.ann;

  auto v = _visualizers[j*N+k];
  sound::Generator::NoteSheet notes;

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

  if (j*N+k != N*N/2 || _generation == 0) {
    // Don't save last gen champion again
    std::ostringstream oss;
    oss << "i" << j << "_j" << k;
    stdfs::path saveFolder = _currentSavePath / oss.str();
    stdfs::create_directories(saveFolder);
    i.genome.toFile(saveFolder / "genome");
    i.genome.cppn.render_gvc_graph(saveFolder / "cppn_gvc.png");
    using GV = GraphViewer;
    GV::render<cppn::Viewer>(i.genome.cppn, saveFolder / "cppn_qt.pdf");
    i.ann.render_gvc_graph(saveFolder / "ann_gvc.png");
    GV::render<ann::Viewer>(i.ann, saveFolder / "ann_qt.pdf");
    v->render(saveFolder / "song.png");
    v->sound().generateWav(saveFolder / "song.wav");
  }
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
      individualHoverEnter(index);
      break;
    case QEvent::HoverLeave:
      individualHoverLeave(index);
      break;
    case QEvent::MouseButtonRelease:
      individualMouseClick(index);
      break;
    case QEvent::MouseButtonDblClick:
      individualMouseDoubleClick(index);
      break;
    default:  break;
    }
  }

  return false;
}

void BWWindow::individualHoverEnter(uint index) {
  showIndividualDetails(index);
  if (setting(AUTOPLAY))
    _visualizers[index]->vocaliseToAudioOut(sound::Generator::LOOP);
}

void BWWindow::individualHoverLeave(uint index) {
  if (setting(AUTOPLAY))
    _visualizers[index]->stopVocalisationToAudioOut();
}

void BWWindow::individualMouseClick(uint index) {
  if (setting(LOCK_SELECTION))  setSelectedIndividual(index);
  else if (setting(SELECT_NEXT))  nextGeneration(index);
  else if (setting(PLAY)) {
    auto v = _visualizers[index];
    if (setting(MANUAL_PLAY))
      v->vocaliseToAudioOut(sound::Generator::ONE_PASS);
    else
      v->vocaliseToAudioOut(sound::Generator::ONE_NOTE);
  }
}

void BWWindow::individualMouseDoubleClick(uint index) {
//  nextGeneration(index);
}

void BWWindow::showIndividualDetails(int index) {
  if (_selection)  return;
  std::cerr << "\nShowing details of individual at index " << index << "\n";

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

bool BWWindow::setting(Setting s) const {
  return _settings.value(s)->isChecked();
}

void BWWindow::saveSettings(void) const {
  QSettings settings;
  settings.setValue("geom", saveGeometry());

  settings.beginGroup("settings");
  QMetaEnum e = QMetaEnum::fromType<Setting>();
  for (int i=0; i<e.keyCount(); i++)
    settings.setValue(e.key(i), setting(Setting(e.value(i))));
  settings.endGroup();
}

void BWWindow::restoreSettings(void) {
  QSettings settings;
  restoreGeometry(settings.value("geom").toByteArray());

  settings.beginGroup("settings");
  QMetaEnum e = QMetaEnum::fromType<Setting>();
  for (int i=0; i<e.keyCount(); i++)
    _settings[Setting(e.value(i))]->setChecked(settings.value(e.key(i)).toBool());
  settings.endGroup();
}

void BWWindow::closeEvent(QCloseEvent *e) {
  if (setting(FAST_CLOSE)
      || QMessageBox::question(this, "Closing", "Confirm closing?",
                               QMessageBox::Yes, QMessageBox::No)
        == QMessageBox::Yes) {
    saveSettings();
    e->accept();

  } else
    e->ignore();
}

} // end of namespace gui
