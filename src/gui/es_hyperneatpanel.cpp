#include <QGridLayout>
#include <QFormLayout>

#include "es_hyperneatpanel.h"

namespace gui {

struct VLabel : public QLabel {
  VLabel (const QString &text = "", QWidget *parent = nullptr)
    : QLabel(text, parent) {}

  QSize minimumSizeHint(void) const override {
    return QLabel::minimumSizeHint().transposed();
  }

  QSize sizeHint(void) const override {
    return QLabel::sizeHint().transposed();
  }

  void paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.translate(rect().center());
    painter.rotate(-90);
    painter.translate(-rect().transposed().center());
    painter.drawText(rect().transposed(), alignment(), text());
  }
};

ES_HyperNEATPanel::ES_HyperNEATPanel (QWidget *parent) : QWidget(parent) {
  auto *layout = new QGridLayout;

  uint r = 0, c = 0;
  layout->addWidget(new VLabel ("CPPN"), r, c++, Qt::AlignCenter);
  layout->addWidget(cppnViewer = new cppn::Viewer, r, c++);
  layout->addWidget(cppnOViewer = new cppn::OutputSummary, r++, c);

  c = 0;
  layout->addWidget(new VLabel ("ANN"), r, c++, Qt::AlignCenter);
  layout->addWidget(annViewer = new ann::Viewer, r++, c, 1, 2);

  c = 0;
  auto lotherFields = new QFormLayout;
  layout->addWidget(new VLabel ("Misc"), r, c++, Qt::AlignCenter);
  layout->addLayout(lotherFields, r++, c, 1, 2, Qt::AlignCenter);

  for (const auto &p: genotype::ES_HyperNEAT::iterator())
    if (!p.second.get().isRecursive())
      lotherFields->addRow(QString::fromStdString(p.first) + ":",
                           otherFields[p.first] = new QLabel);

  connect(annViewer, &ann::Viewer::neuronHovered,
          this, &ES_HyperNEATPanel::showCPPNOutputsAt);

  setLayout(layout);

  noData();
}

void ES_HyperNEATPanel::setData (const genotype::ES_HyperNEAT &genome,
                                 phenotype::CPPN &cppn,
                                 phenotype::ANN &ann) {
  _genome = &genome;
  _cppn = &cppn;
  _ann = &ann;

  cppnViewer->setGraph(genome.cppn);
  cppnOViewer->phenotypes(genome, cppn, {0,0});
  annViewer->setGraph(ann);

  for (const auto &p: otherFields)
    p.second->setText(QString::fromStdString(genome.getField(p.first)));
}

void ES_HyperNEATPanel::noData(void) {
  cppnViewer->clearGraph();
  cppnOViewer->noPhenotypes();
  annViewer->clearGraph();
  for (const auto &p: otherFields) p.second->setText("N/A");
}

void ES_HyperNEATPanel::showCPPNOutputsAt(const QPointF &p) {
  cppnOViewer->phenotypes(*_genome, *_cppn, p);
}

} // end of namespace gui
