#include <QGridLayout>
#include <QFormLayout>
#include <QSettings>

#include "es_hyperneatpanel.h"
#include "verticalpanel.hpp"

namespace kgd::es_hyperneat::gui {

auto row (const QString &label, QWidget *contents) {
  auto holder = new QWidget;
  auto layout = new QGridLayout;

  QLabel *l;

  layout->addWidget(l = new kgd::gui::VLabel (label), 0, 0, Qt::AlignCenter);
  layout->addWidget(contents, 0, 1);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setColumnStretch(0, 0);
  layout->setColumnStretch(1, 1);

  holder->setLayout(layout);
  return holder;
}

auto row (const QString &label, QLayout *contents) {
  auto holder = new QWidget;
  holder->setLayout(contents);
  return row(label, holder);
}

ES_HyperNEATPanel::ES_HyperNEATPanel (QWidget *parent) : QWidget(parent) {
  setWindowTitle("ES-HyperNEAT Panel");

  _mainSplitter = new QSplitter (Qt::Vertical);
  _mainSplitter->setObjectName("eshn::splitters::main");

  _cppnSplitter = new QSplitter;
  _cppnSplitter->setObjectName("eshn::splitters::cppn");
  _cppnSplitter->addWidget(cppnViewer = new cppn::Viewer);
  _cppnSplitter->addWidget(cppnOViewer = new cppn::OutputSummary);
  _cppnSplitter->setStretchFactor(0, 1);
  _mainSplitter->addWidget(row("CPPN", _cppnSplitter));

  _annSplitter = new QSplitter;
  _annSplitter->setObjectName("eshn::splitters::ann");
  _annSplitter->addWidget(annViewer = new ann::Viewer);
  _annSplitter->addWidget(neuronViewer = new ann::NeuronStateViewer);
  _mainSplitter->addWidget(row("ANN", _annSplitter));

  auto lotherFields = new QFormLayout;
  _mainSplitter->addWidget(row("Misc", lotherFields));

  for (const auto &p: genotype::ES_HyperNEAT::iterator())
    if (!p.second.get().isRecursive())
      lotherFields->addRow(QString::fromStdString(p.first) + ":",
                           otherFields[p.first] = new QLabel);

//  layout->setRowStretch(0, 1);
//  layout->setRowStretch(1, 2);
//  layout->setRowStretch(2, 0);

  connect(annViewer, &ann::Viewer::neuronHovered,
          this, &ES_HyperNEATPanel::neuronHovered);

  auto layout = new QHBoxLayout;
  layout->addWidget(_mainSplitter);
  setLayout(layout);

  _settingsLoaded = false;
  noData();
}

void ES_HyperNEATPanel::setData (const genotype::ES_HyperNEAT &genome,
                                 const phenotype::CPPN &cppn,
                                 const phenotype::ANN &ann) {
  _genome = &genome;
  _cppn = &cppn;
  _ann = &ann;

  cppnViewer->setGraph(genome.cppn);
  cppnOViewer->phenotypes(cppn, {0,0}, cppn::OutputSummary::ALL);
  annViewer->setGraph(ann);

  for (const auto &p: otherFields)
    p.second->setText(QString::fromStdString(genome.getField(p.first)));
}

void ES_HyperNEATPanel::noData(void) {
  cppnViewer->clearGraph();
  cppnOViewer->noPhenotypes();
  annViewer->clearGraph();
  neuronViewer->noState();
  for (const auto &p: otherFields) p.second->setText("N/A");
}

void ES_HyperNEATPanel::neuronHovered(const phenotype::ANN::Neuron &n) {
  using T = phenotype::ANN::Neuron::Type;
  using S = cppn::OutputSummary::ShowFlags;
  S flag = S(
      (n.type != T::O ? S::OUTGOING : S::NONE)
    | ((n.type != T::I) ? S::INCOMING : S::NONE)
  );
  cppnOViewer->phenotypes(*_cppn, QPointF(n.pos.x(), n.pos.y()), flag);

  neuronViewer->displayState(&n);
}

void ES_HyperNEATPanel::showEvent(QShowEvent *e) {
  if (!_settingsLoaded) {
    QSettings settings;
    for (QSplitter *s: { _mainSplitter, _cppnSplitter, _annSplitter })
      kgd::gui::restore(settings, s);
    _settingsLoaded = true;
  }
  QWidget::showEvent(e);
}

void ES_HyperNEATPanel::hideEvent(QHideEvent *e) {
  QSettings settings;
  for (QSplitter *s: { _mainSplitter, _cppnSplitter, _annSplitter })
    kgd::gui::save(settings, s);
  QWidget::hideEvent(e);
}

} // end of namespace kgd::es_hyperneat::gui

namespace kgd::gui {

void save (QSettings &settings, const QSplitter *splitter) {
  Q_ASSERT(!splitter->objectName().isEmpty());
  QVariantList l;
  bool allNull = true;
  for (int s: splitter->sizes()) {
    l.append(s);
    allNull &= (s == 0);
  }

  if (!allNull)  settings.setValue(splitter->objectName(), l);
}

void restore (const QSettings &settings, QSplitter *splitter) {
  Q_ASSERT(!splitter->objectName().isEmpty());
  QList<int> l;
  for (const QVariant &v: settings.value(splitter->objectName()).toList())
    l.append(v.toInt());
  splitter->setSizes(l);
}

} // end of namespace kgd::gui
