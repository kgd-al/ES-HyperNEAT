#include <QImage>
#include <QGridLayout>
#include <QPainter>

#include "outputsummary.h"
#include "../verticalpanel.hpp"

namespace kgd::es_hyperneat::gui::cppn {

static constexpr int S = 25;

struct OutputViewer : public QWidget {
  QImage image;
  bool enabled;

  OutputViewer (QWidget *parent = nullptr) : QWidget(parent) {
    image = QImage(S, S, QImage::Format_ARGB32);
    enabled = false;
  }

  QSize minimumSizeHint(void) const override {
    return QSize(100,100);
  }

  void paintEvent(QPaintEvent *e) override {
    QWidget::paintEvent(e);
    QPainter painter (this);

    if (enabled)
      painter.drawImage(rect(), image.scaled(size(), Qt::KeepAspectRatio));

    else {
      painter.setRenderHint(QPainter::Antialiasing, true);
      auto r = rect().adjusted(0, 0, -1, -1);
      painter.drawRect(r);
      painter.drawLine(r.topLeft(), r.bottomRight());
      painter.drawLine(r.bottomLeft(), r.topRight());
    }
  }
};

template <typename L = QLabel>
L* keyLabel (const QString &text) {
  auto *l = new L (text);
  l->setAlignment(Qt::AlignCenter);
  auto f = l->font();
  f.setItalic(true);
  l->setFont(f);
  return l;
}

OutputSummary::OutputSummary (QWidget *parent) : QWidget(parent) {
  auto layout = new QGridLayout;
  QWidget *spacer = new QWidget;
  layout->addWidget(spacer, 0, 0, 1, 3);
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  layout->addWidget(_header = new QLabel, 1, 0, 1, 3, Qt::AlignCenter);
  _header->setAlignment(Qt::AlignCenter);

  using VLabel = kgd::gui::VLabel;
  layout->addWidget(keyLabel<VLabel>("a, b, ., ."), 3, 0);
  layout->addWidget(keyLabel<VLabel>("., ., a, b"), 4, 0);
  layout->addWidget(keyLabel("w"), 2, 1);
  layout->addWidget(keyLabel("l"), 2, 2);
  for (uint i=0; i<2; i++)
    for (uint j=0; j<2; j++)
      layout->addWidget(_viewers[i*2+j] = new OutputViewer, i+3, j+1);
  setLayout(layout);
}

void OutputSummary::phenotypes (const genotype::ES_HyperNEAT &genome,
                                phenotype::CPPN &cppn, const QPointF &p,
                                ShowFlags flag) {
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

  noPhenotypes();

  std::array<CPPN::Inputs, 2> inputs;
  inputs.fill(cppn.inputs());

  std::array<CPPN::Outputs, 2> outputs;
  outputs.fill(cppn.outputs());

  std::vector<phenotype::CPPN::Range> ranges;
  ranges.resize(outputs.size());
  for (uint i=0; i<outputs.size(); i++)
    ranges[i] = functionRanges.at(genome.cppn.outputFunctions.at(i));

  const auto w_color = [&outputs, &ranges] (uint i, uint j) {
    auto o = outputs[i][j];
    return (o < 0) ? QColor::fromHsv(0, -o*255, -o*255).rgb()
                   : QColor::fromHsv(0, 0, o*255).rgb();
  };

  const auto l_color = [&outputs, &ranges] (uint i, uint j) {
    return QColor::fromHsv(0, 0, scale(ranges[j], qImageRange, outputs[i][j]))
        .rgb();
  };

  inputs[0][0] = p.x(); inputs[0][1] = p.y();
  inputs[1][2] = p.x(); inputs[1][3] = p.y();
  for (int r=0; r<S; r++) {
    std::array<QRgb*, 4> bytes;
    for (uint i=0; i<_viewers.size(); i++)
      if ((i < 2 && (flag & SHOW_OUTGOING))
       || (i >= 2 && (flag & SHOW_INCOMING)))
        bytes[i] = (QRgb*) _viewers[i]->image.scanLine(r);

    // Invert y to account for downward y axis windows
    inputs[0][3] = inputs[1][1] = -2.*r/(S-1) + 1;

    for (int c=0; c<S; c++) {
      inputs[0][2] = inputs[1][0] = 2.*c/(S-1) - 1;
      for (uint i=0; i<2; i++)  cppn(inputs[i], outputs[i]);

      if (flag & SHOW_OUTGOING) {
        bytes[0][c] = w_color(0, 0);
        bytes[1][c] = l_color(0, 1);
      }

      if (flag & SHOW_INCOMING) {
        bytes[2][c] = w_color(1, 0);
        bytes[3][c] = l_color(1, 1);
      }
    }
  }

  uint px = .5 * (S-1) * (p.x() + 1), py = .5 * (S-1) * (1 - p.y());
  for (uint i=0; i<_viewers.size(); i++) {
    auto v = _viewers[i];
    v->enabled = (i < 2 && (flag & SHOW_OUTGOING))
              || (i >= 2 && (flag & SHOW_INCOMING));

    if (v->enabled)
      v->image.setPixel(px, py, QColor(Qt::green).rgb());

    v->update();
  }
}

void OutputSummary::noPhenotypes(void) {
  for (auto v: _viewers)  v->image.fill(Qt::transparent);
}

} // end of namespace kgd::es_hyperneat::gui::cppn
