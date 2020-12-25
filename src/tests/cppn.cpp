#include <ostream>

#include "phenotype.h"

int main (void) {
  rng::FastDice dice (0);
  auto cppn = genotype::CPPN::random(dice);
  std::cout << "random cppn: " << cppn << std::endl;

  std::cout << cppn.dump() << std::endl;

  nlohmann::json j = cppn;
  genotype::CPPN cppn2 = j;

  assertEqual(cppn, cppn2, true);

  return 0;
}
