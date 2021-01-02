#include <QGridLayout>
#include <QFormLayout>

#include "es_hyperneatpanel.h"

#include <QDebug>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QToolButton>
#include "soundgenerator.h"
#include <QMetaEnum>

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
  layout->addWidget(_cppnViewer = new cppn::Viewer, r, c++);
  layout->addWidget(_cppnOViewer = new cppn::OutputSummary, r++, c);

  c = 0;
  layout->addWidget(new VLabel ("ANN"), r, c++, Qt::AlignCenter);
  layout->addWidget(_annViewer = new ann::Viewer, r++, c, 1, 2);

  c = 0;
  auto otherFields = new QFormLayout;
  layout->addWidget(new VLabel ("Misc"), r, c++, Qt::AlignCenter);
  layout->addLayout(otherFields, r++, c, 1, 2, Qt::AlignCenter);

  for (const auto &p: genotype::ES_HyperNEAT::iterator())
    if (!p.second.get().isRecursive())
      otherFields->addRow(QString::fromStdString(p.first) + ":",
                          _otherFields[p.first] = new QLabel);

  connect(_annViewer, &ann::Viewer::neuronHovered,
          this, &ES_HyperNEATPanel::showCPPNOutputsAt);

  setLayout(layout);
}

void ES_HyperNEATPanel::setData (const genotype::ES_HyperNEAT &genome,
                                 phenotype::CPPN &cppn,
                                 phenotype::ANN &ann) {
  _genome = &genome;
  _cppn = &cppn;
  _ann = &ann;

  _cppnViewer->setCPPN(genome.cppn);
  _cppnOViewer->phenotypes(genome, cppn, {0,0});
  _annViewer->setANN(ann);

  for (const auto &p: _otherFields)
    p.second->setText(QString::fromStdString(genome.getField(p.first)));
}

void ES_HyperNEATPanel::showCPPNOutputsAt(const QPointF &p) {
  _cppnOViewer->phenotypes(*_genome, *_cppn, p);
}

} // end of namespace gui
