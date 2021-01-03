#include <QApplication>

#include "gui/bwwindow.h"

int main (int argc, char **argv) {
  QApplication app (argc, argv);

  rng::FastDice dice (0);
  genotype::ES_HyperNEAT genome = genotype::ES_HyperNEAT::random(dice);

  genome.cppn = genotype::ES_HyperNEAT::CPPN::fromDot(R"(
  CPPN(5,2)
    6 [sigm]; # weight
    7 [step]; # leo
    8 [sin]; # G1 (should use gauss)
    9 [gaus]; # G2
    1 -> 8 [+3]; # x0 to G1 (should be 1)
    2 -> 9 [+.1]; # y0 to G2 (should be 1)
    3 -> 8 [-3]; # x1 to G1 (should be 1)
    4 -> 9 [-1]; # y1 to G2
    5 -> 9 [+1]; # Bias to G2
    5 -> 7 [-.75]; # Bias to leo
    9 -> 6 [+1]; # G2 to w
    8 -> 7 [+1]; # G1 to leo
  )");

  genome.recurrentDY = 2;

  phenotype::CPPN cppn = phenotype::CPPN::fromGenotype(genome);

  phenotype::ANN::Coordinates rnd_hidden;
  for (uint i=0; i<10; i++)
    rnd_hidden.push_back({dice(-1.f, 1.f), dice(-.8f, .8f)});
//  rnd_hidden = {{0,0}};

  phenotype::ANN ann = phenotype::ANN::build(
    { { -1, -1 }, { 0, -1}, { 1, -1 } },
    { { -.5, 1 }, { 0, 1}, { .5, 1 } },
    rnd_hidden,
    genome, cppn
  );

  gui::ES_HyperNEATPanel p;
  p.setData(genome, cppn, ann);
  p.show();

  gui::SoundGenerator sg;
//  sg.setInstrument(gui::SoundGenerator::SAW);
//  sg.vocalisation({ 0, 1, 0,
//                    0, 1, 1,
//                    0, 0, 1,
//                    1, 0, 1,
//                    1, 0, 0
//                  });
  sg.vocalisation({ 0, 1, 1, 0, 1 });

  return app.exec();
  return 0;
}