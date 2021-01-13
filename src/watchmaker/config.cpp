#include "config.h"

namespace config {
#define CFILE WatchMaker
using namespace kgd::watchmaker;

DEFINE_PARAMETER(bool, withBias, true)
DEFINE_PARAMETER(TemporalInput, tinput, TemporalInput::NONE)
DEFINE_PARAMETER(Audition, audition, Audition::SELF)

DEFINE_PARAMETER(int, dataLogLevel, 1)

DEFINE_SUBCONFIG(genotype::ES_HyperNEAT::config_t, configESHN)

#undef CFILE
} // end of namespace config
