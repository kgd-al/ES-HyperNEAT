#include <QImage>
#include <QGridLayout>
#include <QPainter>

#include "outputsummary.h"

namespace gui::cppn {

static constexpr int S = 25;

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

void OutputSummary::phenotypes (const genotype::ES_HyperNEAT &genome,
                                phenotype::CPPN &cppn, const QPointF &p) {
  using CPPN = phenotype::CPPN;
  using Range = phenotype::CPPN::Range;
  static const auto &functionRanges = phenotype::CPPN::functionRanges;
  static const auto scale = [] (const Range &in, const Range &out, float v) {
    return (out.max - out.min) * (v - in.min) / (in.max - in.min) + out.min;
  };
  static constexpr Range qImageRange { 0, 255 };

  {
    static const auto trunc = [] (float v) {
      return std::round(100 * v) / 100.f;
    };
    std::ostringstream oss;
    oss << "connectivity at " << std::showpos
        << std::setw(5) << trunc(p.x()) << ", " << std::setw(5) << trunc(p.y());
    _header->setText(QString::fromStdString(oss.str()));
  }

  std::array<CPPN::Inputs, 2> inputs;
  inputs.fill(cppn.inputs());

  std::array<CPPN::Outputs, 2> outputs;
  outputs.fill(cppn.outputs());

  std::vector<phenotype::CPPN::Range> ranges;
  ranges.resize(outputs.size());
  for (uint i=0; i<outputs.size(); i++)
    ranges[i] = functionRanges.at(genome.cppn.outputFunctions.at(i));

  const auto scale_o = [&outputs, &ranges] (uint i, uint j) {
    auto r = scale(ranges[i], qImageRange, outputs[i][j]);
    assert(0 <= r && r <= 255);
    return r;
  };

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

      bytes[0][c] = QColor::fromHsv(0, 0, scale_o(0, 0)).rgb();
      bytes[1][c] = QColor::fromHsv(0, 0, scale_o(0, 1)).rgb();
      bytes[2][c] = QColor::fromHsv(0, 0, scale_o(1, 0)).rgb();
      bytes[3][c] = QColor::fromHsv(0, 0, scale_o(1, 1)).rgb();
    }
  }

  for (auto v: _viewers)  v->update();
}

}
