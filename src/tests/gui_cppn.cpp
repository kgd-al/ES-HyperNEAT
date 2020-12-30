#include <QApplication>

#include "../gui/es_hyperneatpanel.h"

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

  phenotype::CPPN cppn = phenotype::CPPN::fromGenotype(genome);

  gui::ES_HyperNEATPanel p;
  p.setData(genome, cppn);
  p.show();

  genome.cppn.graphviz_render_graph("cppn_genotype.png");
  p._cppnViewer->render("cppn_qt.pdf");

  return app.exec();
}
