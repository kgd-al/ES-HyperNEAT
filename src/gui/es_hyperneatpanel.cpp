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

  _cppnSplitter = new QSplitter;
  _cppnSplitter->addWidget(cppnViewer = new cppn::Viewer);
  _cppnSplitter->addWidget(cppnOViewer = new cppn::OutputSummary);
  _mainSplitter->addWidget(row("CPPN", _cppnSplitter));

  _annSplitter = new QSplitter;
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
                                 phenotype::CPPN &cppn,
                                 phenotype::ANN &ann) {
  _genome = &genome;
  _cppn = &cppn;
  _ann = &ann;

  cppnViewer->setGraph(genome.cppn);
  cppnOViewer->phenotypes(genome, cppn, {0,0}, cppn::OutputSummary::SHOW_ALL);
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
      (n.type != T::O ? S::SHOW_OUTGOING : S::SHOW_NONE)
    | ((n.type != T::I && n.type != T::B) ? S::SHOW_INCOMING : S::SHOW_NONE)
  );
  cppnOViewer->phenotypes(*_genome, *_cppn,
                          QPointF(n.pos.x(), n.pos.y()),
                          flag);

  neuronViewer->displayState(n);
}

template <uint n>
auto slist (const QSettings &s, const QString &key) {
  static const auto def =
      QVariant::fromValue(QList<int>::fromStdList(std::list<int>(n, 1)));
  return s.value(key, def).value<QList<int>>();
}

auto slist (const QSplitter *s) {
  return QVariant::fromValue(s->sizes());
}

void ES_HyperNEATPanel::showEvent(QShowEvent *e) {
  if (!_settingsLoaded) {
    QSettings settings;
    settings.beginGroup("eshn_panel");
    settings.beginGroup("splitters");
    _mainSplitter->setSizes(slist<3>(settings, "main"));
    _cppnSplitter->setSizes(slist<2>(settings, "cppn"));
    _annSplitter->setSizes(slist<1>(settings, "ann"));
    _settingsLoaded = true;
  }
  QWidget::showEvent(e);
}

void ES_HyperNEATPanel::hideEvent(QHideEvent *e) {
  QSettings settings;
  settings.beginGroup("eshn_panel");
  settings.beginGroup("splitters");
  settings.setValue("main", slist(_mainSplitter));
  settings.setValue("cppn", slist(_cppnSplitter));
  settings.setValue("ann", slist(_annSplitter));
  QWidget::hideEvent(e);
}

} // end of namespace kgd::es_hyperneat::gui
