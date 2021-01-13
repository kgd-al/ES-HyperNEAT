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

BWWindow::BWWindow(const stdfs::path &baseSavePath, uint seed, QWidget *parent)
  : QMainWindow(parent), _dice(seed), _baseSavePath(baseSavePath) {
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
      else if (s == ANIMATE) add(new QLabel ("Other"));

      QString name = QString(e.key(i)).replace("_", " ").toLower();
      name[0] = name[0].toUpper();

      QCheckBox *c;
      add(_settings[s] = c = new QCheckBox (name));
      c->setFocusPolicy(Qt::NoFocus);
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

  _animation.index = -1;
  _animation.step = -1;
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
  if (true || setting(LOCK_SELECTION))
        setSelectedIndividual(N*N/2);
  else  showIndividualDetails(N*N/2);
}

void BWWindow::nextGeneration(uint index) {
  setSelectedIndividual(-1);
  showIndividualDetails(-1);

  _generation++;
  std::cerr << "Processing generation" << _generation << "\n";
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

  const auto ix = j*N+k;
  simu::Individual &i = *(_individuals[ix] = std::move(in)).get();
  auto &ann = i.ann;
  auto &notes = i.phenotype;

  auto v = _visualizers[ix];

  auto inputs = ann.inputs();
  auto outputs = ann.outputs();
  assert(inputs.size() == outputs.size());

  std::fill(inputs.begin(), inputs.end(), 0);
  for (uint n=0; n<sound::Generator::NOTES; n++) {
    ann(inputs, outputs, i.genome.substeps);
    for (uint c=0; c<C; c++)  notes[n*C+c] = outputs[c];
    inputs = outputs;
  }
  v->setNoteSheet(notes);

  using utils::operator<<;
  std::cerr << "Note sheet for [" << j << "," << k << "]: " << notes << "\n";

  if (ix != N*N/2 || _generation == 0) {
    // Don't save last gen champion again
    std::ostringstream oss;
    oss << "i" << j << "_j" << k;
    logIndividual(ix, _currentSavePath / oss.str(), 3);
  }
}

void BWWindow::logIndividual(uint index, const stdfs::path &f,
                             int level) const {
  if (level <= 0) return;
  stdfs::create_directories(f);

  /// TODO Also log plain note sheet

  const auto &i = *_individuals[index];
  if (level >= 1) i.genome.toFile(f / "genome");
  if (level >= 3) i.genome.cppn.render_gvc_graph(f / "cppn_gvc.png");
  using GV = GraphViewer;
  if (level >= 2) GV::render<cppn::Viewer>(i.genome.cppn, f / "cppn_qt.pdf");
  if (level >= 3) i.ann.render_gvc_graph(f / "ann_gvc.png");
  if (level >= 2) GV::render<ann::Viewer>(i.ann, f / "ann_qt.pdf");

  const auto &v = *_visualizers[index];
  if (level >= 1) v.render(f / "song.png");
  if (level >= 2) v.sound().generateWav(f / "song.wav");
}

int BWWindow::indexOf (const sound::Visualizer *v) {
  int index = -1;
  for (uint i=0; i<_visualizers.size() && index < 0; i++)
    if (_visualizers[i] == v)
      index = i;
  return index;
}

bool BWWindow::eventFilter(QObject *watched, QEvent *event) {
  static const QList<QEvent::Type> types {
    QEvent::HoverEnter, QEvent::HoverLeave,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick
  };

  auto *v = dynamic_cast<sound::Visualizer*>(watched);
  if (!v) return false;

  if (!types.contains(event->type())) return false;

  int index = indexOf(v);
  if (index < 0)  return false;

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

  return false;
}

void BWWindow::keyPressEvent(QKeyEvent *e) {
  auto s = -1;
  if (e->modifiers() & Qt::CTRL)        s = PLAY;
  else if (e->modifiers() & Qt::SHIFT)  s = LOCK_SELECTION;
  else if (e->modifiers() & Qt::ALT)    s = SELECT_NEXT;
  if (s >= 0) _settings.value(Setting(s))->setChecked(true);

  auto v = _selection;
  if (!v) v = dynamic_cast<sound::Visualizer*>(childAt(QCursor::pos()));
  if (!v) return;

  int index = indexOf(v);
  if (index < 0)  return;

  switch (e->key()) {
  case Qt::Key_Space: startVocalisation(index); break;
  case Qt::Key_Escape: stopVocalisation(index); break;
  }
}

void BWWindow::individualHoverEnter(uint index) {
  showIndividualDetails(index);
  if (setting(AUTOPLAY))  startVocalisation(index);
}

void BWWindow::individualHoverLeave(uint index) {
  if (setting(AUTOPLAY) || _selection == nullptr)  stopVocalisation(index);
}

void BWWindow::individualMouseClick(uint index) {
  if (setting(LOCK_SELECTION))    setSelectedIndividual(index);
  else if (setting(SELECT_NEXT))  nextGeneration(index);
  else if (setting(PLAY) && !setting(AUTOPLAY))
    startVocalisation(index);
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

void BWWindow::startVocalisation(uint index) {
  auto v = _visualizers[index];
  if (_selection != nullptr && _selection != v) return;

  _animation.index = index;
  _animation.step = 0;
  startAnimateShownANN();

  if (setting(AUTOPLAY))
    v->vocaliseToAudioOut(sound::Generator::LOOP);
  else if (setting(MANUAL_PLAY))
    v->vocaliseToAudioOut(sound::Generator::ONE_PASS);
  else if (setting(STEP_PLAY))
    v->vocaliseToAudioOut(sound::Generator::ONE_NOTE);
}

void BWWindow::stopVocalisation(uint index) {
  auto v = _visualizers[index];
  if ((setting(PLAY) || setting(AUTOPLAY))
       && ((_selection == nullptr) || (_selection == v))) {
    v->stopVocalisationToAudioOut();
    if (setting(ANIMATE)) stopAnimateShownANN();
  }
}

void BWWindow::startAnimateShownANN(void) {
  if (!setting(ANIMATE))  return;

  connect(_visualizers[_animation.index], &sound::Visualizer::notifyNote,
          this, &BWWindow::animateShownANN);
  _individuals[_animation.index]->ann.reset();
  _details->annViewer->startAnimation();
  qDebug() << "Started animation" << _animation.index << _animation.step;
}

void BWWindow::animateShownANN(void) {
  if (!setting(ANIMATE))  return;

  auto &av = *_details->annViewer;
  assert(_animation.index >= 0);
  simu::Individual &i = *_individuals[_animation.index];

  auto inputs = i.ann.inputs(), outputs = i.ann.outputs();
  if (_animation.step > 0) {
    static constexpr auto C = sound::Generator::CHANNELS;
    uint off = (_animation.step-1)*C;
    for (uint c=0; c<C; c++)  inputs[c] = i.phenotype[off+c];
  }
  i.ann(inputs, outputs, i.genome.substeps);
  av.updateAnimation();
  qDebug() << "Stepped animation" << _animation.index << _animation.step;

  _animation.step = (_animation.step + 1) % sound::Generator::NOTES;
}

void BWWindow::stopAnimateShownANN(void) {
  if (!setting(ANIMATE))  return;

  auto &av = *_details->annViewer;
  if (av.isAnimating()) {
    assert(_animation.index >= 0);
    qDebug() << "Stopping animation" << _animation.index << _animation.step;
    disconnect(_visualizers[_animation.index], &sound::Visualizer::notifyNote,
               this, &BWWindow::animateShownANN);
    av.stopAnimation();
    _animation.index = -1;
    _animation.step = -1;
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
