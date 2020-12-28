#include <QApplication>
#include <QPixmap>

#include "../phenotype/cppn.h"

#include <QDebug>

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
    1 -> 8 [+1];
    2 -> 9 [+1];
    3 -> 8 [-1];
    4 -> 9 [-1];
    5 -> 9 [+1];
    5 -> 7 [-1];
    9 -> 6 [+1];
    8 -> 7 [+1];
  )");

  phenotype::CPPN cppn = phenotype::CPPN::fromGenotype(genome);

  int s = 100;
  QImage i (s, s, QImage::Format_RGB32);

  auto inputs = cppn.inputs();
  auto outputs = cppn.outputs();

  inputs[0] = inputs[1] = 0;
  for (int r=0; r<s; r++) {
    QRgb* b = (QRgb*) i.scanLine(r);
    inputs[2] = 2.*r/(s-1) - 1;
    for (int c=0; c<s; c++) {
      inputs[3] = 2.*c/(s-1) - 1;
      cppn(inputs, outputs);
      qDebug() << inputs << outputs;
      b[c] = QColor::fromHsv(0, 0, outputs[0]*255).rgb();
      qDebug() << r << c << hex << b[c];
    }
  }

  i.save("cppn_phenotype.png");

  return 0;
}
