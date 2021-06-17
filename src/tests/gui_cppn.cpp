#include <QApplication>
#include <QHBoxLayout>

#include <QDebug>

#include "../gui/es_hyperneatpanel.h"
#include "../gui/ann/3d/viewer.h"

void make_sound (void) {

}

int main (int argc, char **argv) {
  QApplication app (argc, argv);

  config::EvolvableSubstrate::printConfig();

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
  phenotype::ANN ann;

  do {
    rng::FastDice dice (seed++);
    genome = genotype::ES_HyperNEAT::random(dice);

    for (uint i=0; i<1000; i++) genome.mutate(dice);

    phenotype::CPPN cppn = phenotype::CPPN::fromGenotype(genome);

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
  } while (ann.empty() && seed < baseSeed+100);


  phenotype::CPPN cppn = phenotype::CPPN::fromGenotype(genome);

  kgd::es_hyperneat::gui::ES_HyperNEATPanel p;
  p.setData(genome, cppn, ann);
  p.show();

  genome.cppn.render_gvc_graph("tmp/cppn_genotype.png");
  p.cppnViewer->render("tmp/cppn_qt.pdf");
  ann.render_gvc_graph("tmp/ann_phenotype.pdf");
  p.annViewer->render("tmp/ann_qt.pdf");

  kgd::es_hyperneat::gui::ann3d::Viewer ann3d;
  ann3d.setGraph(ann);
//  ann3d.show();

  QWidget *holder = new QWidget,
          *annholder = QWidget::createWindowContainer(&ann3d);
  QHBoxLayout *layout = new QHBoxLayout;
  layout->addWidget(&p);
  layout->addWidget(annholder);
  annholder->setMinimumWidth(100);
  layout->setStretch(0, 1);
  layout->setStretch(1, 1);
  holder->setLayout(layout);
  holder->setGeometry(0, 83, 1280, 997);
  holder->show();
  qDebug() << holder->geometry();

  auto r = app.exec();
  qDebug() << holder->geometry();
  return r;
}
