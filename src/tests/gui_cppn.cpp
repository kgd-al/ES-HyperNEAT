#include <QApplication>
#include <QHBoxLayout>

#include <QDebug>

#include "../gui/es_hyperneatpanel.h"
#include "../gui/ann/3d/viewer.h"

int main (int argc, char **argv) {
  QApplication app (argc, argv);

  if (true) {
    // manual 2d cppn
    phenotype::CPPN cppn;
  }

//  config::EvolvableSubstrate::printConfig();

//  rng::FastDice dice (6);
//  genotype::ES_HyperNEAT genome = genotype::ES_HyperNEAT::random(dice);

//  genome.cppn = genotype::ES_HyperNEAT::CPPN::fromDot(R"(
//  CPPN(5,3)
//    6 [sigm]; # weight
//    7 [step]; # leo
//    8 [sin]; # G1 (should use gauss)
//    9 [gaus]; # G2
//    1 -> 8 [+3]; # x0 to G1 (should be 1)
//    2 -> 9 [+.1]; # y0 to G2 (should be 1)
//    3 -> 8 [-3]; # x1 to G1 (should be 1)
//    4 -> 9 [-1]; # y1 to G2
//    5 -> 9 [+1]; # Bias to G2
//    5 -> 7 [-.75]; # Bias to leo
//    9 -> 6 [+1]; # G2 to w
//    8 -> 7 [+1]; # G1 to leo
//  )", dice);
//  genome.cppn = genotype::ES_HyperNEAT::CPPN::fromDot(R"(
//  CPPN(5,2)
//    6 [id]; # weight
//    7 [id]; # leo
//    1 -> 6 [+1];
//    2 -> 7 [-1];
//  )");

  uint baseSeed = (argc > 1 ? atoi(argv[1]) : 0), seed = baseSeed;
  genotype::ES_HyperNEAT genome;
  phenotype::CPPN cppn;
  phenotype::ANN ann;

  do {
    rng::FastDice dice (seed++);
    genome = genotype::ES_HyperNEAT::random(dice);

    for (uint i=0; i<100; i++) genome.mutate(dice);

    cppn = phenotype::CPPN::fromGenotype(genome);

#if ESHN_SUBSTRATE_DIMENSION == 2
    // Assymetrical lower bound
//    ann = phenotype::ANN::build(
//      { { -.5, -.5 }, { 0, -.5}, { .5, -.5 }, {0,0} },
//      { { -.5, 1 }, { 0, 1}, { .5, 1 }, {1,1} },
//      cppn
//    );

    // Assymetrical upper bound
//    ann = phenotype::ANN::build(
//      { { -1, -1 }, { 0, -1}, { 1, -1 } },
//      { { -.5, .7 }, { 0, .8}, { .5, .7 } },
//      cppn
//    );

    // Plain assymetrical
//    ann = phenotype::ANN::build(
//      { { -.8, -.2 }, { 0, 0 }, { .5, -.7 } },
//      { { -.25, -.5 }, { 0, .5}, { .75, .6 } },
//      cppn
//    );

    // Regular and pretty
    ann = phenotype::ANN::build(
      { { -1, -1 }, { 0, -1 }, { 1, -1 } },
      { { -.5, 1 }, { 0,  1 }, { .5, 1 } },
      cppn
    );

#elif ESHN_SUBSTRATE_DIMENSION == 3
//    ann = phenotype::ANN::build(
//      { { -1, -1, -1 }, { -1, -1, 0 }, { -1, -1,  1 },
//        {  0, -1, 0 },
//        {  1, -1, -1 }, {  1, -1, 0 }, {  1, -1,  1 } },
//      { { -.5, 1, 0 }, { 0,  1, 0 }, { .5, 1, 0 }, { 0, 1, -1 } },
//      cppn
//    );
    ann = phenotype::ANN::build(
      { {  0, -1, -1 }, {  0, -1, +1 } },
      { { 0,  1, 0 } },
      cppn
    );
#endif

    std::cout << "Seed " << dice.getSeed() << ":" << (ann.empty() ? "" : " not")
              << " empty\n";

    if (!ann.empty()) {
      std::cout << "\t" << ann.neurons().size() << " neurons\n";
      uint e = 0;
      for (const auto &n: ann.neurons()) e += n->links().size();
      std::cout << "\t" << e << " connections\n";
    }

  } while (ann.empty() && seed < baseSeed+100);

  kgd::es_hyperneat::gui::ES_HyperNEATPanel p;
  p.setData(genome, cppn, ann);
  p.show();

  genome.cppn.render_gvc_graph("tmp/cppn_genotype.png");
  p.cppnViewer->render("tmp/cppn_qt.pdf");
#if ESHN_SUBSTRATE_DIMENSION == 2
  ann.render_gvc_graph("tmp/ann_phenotype.pdf");
  p.annViewer->render("tmp/ann_qt.pdf");
#endif

  auto r = app.exec();
  qDebug() << p.geometry();
  return r;
}
