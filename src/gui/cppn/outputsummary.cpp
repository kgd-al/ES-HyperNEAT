#include <QImage>
#include <QGridLayout>
#include <QPainter>

#include "outputsummary.h"

namespace gui::cppn {

static constexpr int S = 5;

struct OutputViewer : public QWidget {
  QImage image;

  OutputViewer (QWidget *parent = nullptr) : QWidget(parent) {
    image = QImage(S, S, QImage::Format_RGB32);
  }

  QSize minimumSizeHint(void) const override {
    return QSize(100,100);
  }

  void paintEvent(QPaintEvent *e) override {
    QWidget::paintEvent(e);
    QPainter painter (this);
    painter.drawImage(rect(), image);
  }
};

OutputSummary::OutputSummary (QWidget *parent) : QWidget(parent) {
  auto layout = new QGridLayout;
  layout->addWidget(_header = new QLabel, 0, 0, 1, 3, Qt::AlignCenter);
  _header->setAlignment(Qt::AlignCenter);
  layout->addWidget(keyLabel("in"), 2, 0);
  layout->addWidget(keyLabel("out"), 3, 0);
  layout->addWidget(keyLabel("w"), 1, 1);
  layout->addWidget(keyLabel("l"), 1, 2);
  for (uint i=0; i<2; i++)
    for (uint j=0; j<2; j++)
      layout->addWidget(_viewers[i*2+j] = new OutputViewer, i+2, j+1);
  setLayout(layout);
}

QLabel* OutputSummary::keyLabel (const QString &text) {
  auto *l = new QLabel (text);
  l->setAlignment(Qt::AlignCenter);
  auto f = l->font();
  f.setItalic(true);
  l->setFont(f);
  return l;
}

void OutputSummary::phenotypes (phenotype::CPPN &cppn, const QPointF &p) {
  using CPPN = phenotype::CPPN;

  _header->setText(QString("connectivity at %1,%2").arg(p.x()).arg(p.y()));

  std::array<CPPN::Inputs, 2> inputs;
  inputs.fill(cppn.inputs());

  std::array<CPPN::Outputs, 2> outputs;
  outputs.fill(cppn.outputs());

  inputs[0][0] = p.x(); inputs[0][1] = p.y();
  inputs[1][2] = p.x(); inputs[1][3] = p.y();
  for (int r=0; r<S; r++) {
    std::array<QRgb*, 4> bytes;
    for (uint i=0; i<_viewers.size(); i++)
      bytes[i] = (QRgb*) _viewers[i]->image.scanLine(r);

    inputs[0][3] = inputs[1][1] = 2.*r/(S-1) - 1;

    for (int c=0; c<S; c++) {
      inputs[0][2] = inputs[1][0] = 2.*c/(S-1) - 1;
      for (uint i=0; i<2; i++)  cppn(inputs[i], outputs[i]);

      bytes[0][c] = QColor::fromHsv(0, 0, outputs[0][0]*255).rgb();
      bytes[1][c] = QColor::fromHsv(0, 0, outputs[0][1]*255).rgb();
      bytes[2][c] = QColor::fromHsv(0, 0, outputs[1][0]*255).rgb();
      bytes[3][c] = QColor::fromHsv(0, 0, outputs[1][1]*255).rgb();
    }
  }
}

}
