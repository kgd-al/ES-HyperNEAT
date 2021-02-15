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

auto sectionLabel (const QString &text) {
  auto l = new QLabel(text);
  l->setStyleSheet("font-weight: bold");
  return l;
}

OutputSummary::OutputSummary (QWidget *parent) : QWidget(parent) {
  auto layout = new QGridLayout;

  // To gobble up vertical space
  auto spacer = [] {
    return new QSpacerItem(10, 10,
                           QSizePolicy::Expanding, QSizePolicy::Expanding);
  };

  uint r = 0, c = 0;
  layout->addItem(spacer(), r++, c, 1, 3);

  layout->addWidget(sectionLabel("Neural parameters"), r++, c, 1, 3,
                    Qt::AlignCenter);

  layout->addWidget(keyLabel("b"), r++, 1);
  layout->addWidget(_nviewers[0] = new OutputViewer, r++, 1);

  layout->addItem(spacer(), r++, 0, 1, 3);

  layout->addWidget(_cheader = sectionLabel(""), r++, c, 1, 3, Qt::AlignCenter);
  _cheader->setAlignment(Qt::AlignCenter);

  using VLabel = kgd::gui::VLabel;
  layout->addWidget(keyLabel<VLabel>("a, b, x, y"), r+1, 0);
  layout->addWidget(keyLabel<VLabel>("x, y, a, b"), r+2, 0);
  layout->addWidget(keyLabel("w"), r, 1);
  layout->addWidget(keyLabel("l"), r, 2);
  for (uint i=0; i<2; i++)
    for (uint j=0; j<2; j++)
      layout->addWidget(_cviewers[i*2+j] = new OutputViewer, r+i+1, j+1);
  setLayout(layout);
}

void OutputSummary::phenotypes (const phenotype::CPPN &cppn, const QPointF &p,
                                ShowFlags flag) {
  using CPPN = phenotype::CPPN;
  using Range = phenotype::CPPN::Range;
  static const auto &functionRanges = phenotype::CPPN::functionRanges;
  static const auto &cppnOutputs =
      genotype::ES_HyperNEAT::config_t::cppnOutputs();
  static const auto scale = [] (const Range &in, const Range &out, float v) {
    return (out.max - out.min) * (v - in.min) / (in.max - in.min) + out.min;
  };
  static constexpr Range qImageRange { 0, 255 };

  {
    static const auto trunc = [] (float v) {
      return std::round(100 * v) / 100.f;
    };
    std::ostringstream oss;
    oss << "Connectivity at " << std::showpos
        << std::setw(5) << trunc(p.x()) << ", " << std::setw(5) << trunc(p.y());
    _cheader->setText(QString::fromStdString(oss.str()));
  }

  noPhenotypes(flag);


  std::array<CPPN::Inputs, 2> inputs;
  inputs.fill(cppn.inputs());

  std::array<CPPN::Outputs, 2> outputs;
  outputs.fill(cppn.outputs());

  std::vector<phenotype::CPPN::Range> ranges;
  ranges.resize(outputs.size());
  for (uint i=0; i<outputs.size(); i++)
    ranges[i] = functionRanges.at(cppnOutputs[i].function);

  static const auto rw_color = [] (float v) {
    assert(-1 <= v && v <= 1);
    return (v < 0) ? QColor::fromHsv(0, -v*255, -v*255).rgb()
                   : QColor::fromHsv(0, 0, v*255).rgb();
  };

  const auto w_color = [&outputs, &ranges] (uint i, uint j) {
    return rw_color(outputs[i][j]);
  };

  const auto l_color = [&outputs, &ranges] (uint i, uint j) {
    return QColor::fromHsv(0, 0, scale(ranges[j], qImageRange, outputs[i][j]))
        .rgb();
  };

  // Generate output for neural parameter /// TODO Only one for now
  if (flag & NEURAL) {
    CPPN::Inputs inputs = cppn.inputs();
    inputs[2] = inputs[3] = 0;
    for (int r=0; r<S; r++) {
      QRgb *bytes = (QRgb*)_nviewers[0]->image.scanLine(r);
      inputs[1] = -2.*r/(S-1) + 1;

      for (int c=0; c<S; c++) {
        inputs[0] = 2.*c/(S-1) - 1;
        bytes[c] = rw_color(utils::clip(-1.f,
                                        cppn(inputs, config::CPPNOutput::BIAS),
                                        1.f));
      }
    }
    _nviewers[0]->enabled = true;
    _nviewers[0]->update();
  }


  inputs[0][0] = p.x(); inputs[0][1] = p.y();
  inputs[1][2] = p.x(); inputs[1][3] = p.y();
  for (int r=0; r<S; r++) {
    std::array<QRgb*, 4> bytes;
    for (uint i=0; i<_cviewers.size(); i++)
      if ((i < 2 && (flag & OUTGOING))
       || (i >= 2 && (flag & INCOMING)))
        bytes[i] = (QRgb*) _cviewers[i]->image.scanLine(r);

    // Invert y to account for downward y axis windows
    inputs[0][3] = inputs[1][1] = -2.*r/(S-1) + 1;

    for (int c=0; c<S; c++) {
      inputs[0][2] = inputs[1][0] = 2.*c/(S-1) - 1;
      for (uint i=0; i<2; i++)  cppn(inputs[i], outputs[i]);

      if (flag & OUTGOING) {
        bytes[0][c] = w_color(0, 0);
        bytes[1][c] = l_color(0, 1);
      }

      if (flag & INCOMING) {
        bytes[2][c] = w_color(1, 0);
        bytes[3][c] = l_color(1, 1);
      }
    }
  }

  uint px = .5 * (S-1) * (p.x() + 1), py = .5 * (S-1) * (1 - p.y());
  for (uint i=0; i<_cviewers.size(); i++) {
    auto v = _cviewers[i];
    v->enabled = (i < 2 && (flag & OUTGOING))
              || (i >= 2 && (flag & INCOMING));

    if (v->enabled)
      v->image.setPixel(px, py, QColor(Qt::green).rgb());

    v->update();
  }
}

void OutputSummary::noPhenotypes(ShowFlags f) {
  if (f & NEURAL)
    for (auto v: _nviewers)
      v->image.fill(Qt::transparent);

  if (f & CONNECTIONS)
    for (auto v: _cviewers)
      v->image.fill(Qt::transparent);
}

} // end of namespace kgd::es_hyperneat::gui::cppn
