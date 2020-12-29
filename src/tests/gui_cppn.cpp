#include <QApplication>

#include <QImage>
#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QGridLayout>
#include <QLabel>

#include "../phenotype/cppn.h"

#include <QDebug>

static constexpr int S = 5;
using CPPN = phenotype::CPPN;

#ifdef WITH_GVC
#include <gvc.h>
struct CPPNViewer : public QWidget {

};

#else

#endif

struct CPPNOutputViewer : public QWidget {
  QImage image;

  CPPNOutputViewer (QWidget *parent = nullptr) : QWidget(parent) {
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

struct CPPNOutputSummary : public QWidget {
  std::array<CPPNOutputViewer*, 4> viewers;
  QLabel *header;

  CPPNOutputSummary (QWidget *parent = nullptr) : QWidget(parent) {
    auto layout = new QGridLayout;
    layout->addWidget(header = new QLabel, 0, 0, 1, 3, Qt::AlignCenter);
    header->setAlignment(Qt::AlignCenter);
    layout->addWidget(keyLabel("in"), 2, 0);
    layout->addWidget(keyLabel("out"), 3, 0);
    layout->addWidget(keyLabel("w"), 1, 1);
    layout->addWidget(keyLabel("l"), 1, 2);
    for (uint i=0; i<2; i++)
      for (uint j=0; j<2; j++)
        layout->addWidget(viewers[i*2+j] = new CPPNOutputViewer, i+2, j+1);
    setLayout(layout);
  }

  QLabel* keyLabel (const QString &text) {
    auto *l = new QLabel (text);
    l->setAlignment(Qt::AlignCenter);
    auto f = l->font();
    f.setItalic(true);
    l->setFont(f);
    return l;
  }

  void phenotypes (CPPN &cppn, const QPointF &p) {
    header->setText(QString("connectivity at %1,%2").arg(p.x()).arg(p.y()));

    std::array<CPPN::Inputs, 2> inputs;
    inputs.fill(cppn.inputs());

    std::array<CPPN::Outputs, 2> outputs;
    outputs.fill(cppn.outputs());

    inputs[0][0] = p.x(); inputs[0][1] = p.y();
    inputs[1][2] = p.x(); inputs[1][3] = p.y();
    for (int r=0; r<S; r++) {
      std::array<QRgb*, 4> bytes;
      for (uint i=0; i<viewers.size(); i++)
        bytes[i] = (QRgb*) viewers[i]->image.scanLine(r);

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
};

int main (int argc, char **argv) {
  QApplication app (argc, argv);

  rng::FastDice dice (0);
  genotype::ES_HyperNEAT genome = genotype::ES_HyperNEAT::random(dice);

  genome.cppn = genotype::ES_HyperNEAT::CPPN::fromDot(R"(
  CPPN(5,2)
    6 [sigm]; # weight
    7 [step]; # leo
    8 [gaus]; # G1
    9 [gaus]; # G2
    1 -> 8 [+1]; # x0 to G1
    2 -> 9 [+1]; # y0 to G2
    3 -> 8 [-1]; # x1 to G1
    4 -> 9 [-1]; # y1 to G2
    5 -> 9 [+1]; # Bias to G2
    5 -> 7 [-.75]; # Bias to leo
    9 -> 6 [+1]; # G2 to w
    8 -> 7 [+1]; # G1 to leo
  )");
  genome.cppn.graphviz_render("cppn_genotype.ps");

  phenotype::CPPN cppn = phenotype::CPPN::fromGenotype(genome);

  CPPNOutputSummary s;
  s.phenotypes(cppn, {0,0});
  s.show();

  return app.exec();
}
