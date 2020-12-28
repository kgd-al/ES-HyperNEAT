#include <ostream>

#include "../phenotype/cppn.h"

int main (void) {
  rng::FastDice dice (0);

  using Genotype = genotype::ES_HyperNEAT;
  using CPPN = Genotype::CPPN;
  Genotype::printMutationRates(std::cout, 3);

  auto genotype = Genotype::random(dice);
  std::cout << "random genotype: " << genotype << std::endl;
//  genotype.cppn.graphviz_render("random.pdf");
//  genotype.cppn.graphviz_render("random.png");
  genotype.cppn.graphviz_render("random.ps");

//  std::cout << cppn.dump(1) << std::endl;

  {
    nlohmann::json j = genotype;
    Genotype genotype2 = j;

    assertEqual(genotype, genotype2, true);
  }

  for (uint i=0; i<100; i++) {
    genotype.mutate(dice);
    std::cout << ".";
  }
  std::cout << std::endl;

  {
    nlohmann::json j = genotype;
    Genotype genotype2 = j;

    assertEqual(genotype, genotype2, true);
  }

//  std::cout << cppn.dump(1) << std::endl;
//  genotype.cppn.graphviz_render("mutated.pdf");
//  genotype.cppn.graphviz_render("mutated.png");
  genotype.cppn.graphviz_render("mutated.ps");

  auto manual_cppn = CPPN::fromDot(R"(
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
  manual_cppn.graphviz_render("manual.ps");

  return 0;
}
