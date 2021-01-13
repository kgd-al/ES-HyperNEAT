#ifndef KGD_WATCHMAKER_CONFIG_H
#define KGD_WATCHMAKER_CONFIG_H

#include "kgd/settings/configfile.h"
#include "../genotype/es-hyperneat.h"

DEFINE_NAMESPACE_SCOPED_PRETTY_ENUMERATION(
    kgd::watchmaker, TemporalInput,
        NONE, LINEAR, SINUSOIDAL)

DEFINE_NAMESPACE_SCOPED_PRETTY_ENUMERATION(
    kgd::watchmaker, Audition,
        NONE, SELF, COMMUNITY)

namespace config {

struct CONFIG_FILE(WatchMaker) {
  DECLARE_SUBCONFIG(genotype::ES_HyperNEAT::config_t, configESHN)

  DECLARE_PARAMETER(bool, withBias)
  DECLARE_PARAMETER(kgd::watchmaker::TemporalInput, tinput)
  DECLARE_PARAMETER(kgd::watchmaker::Audition, audition)

  DECLARE_PARAMETER(int, dataLogLevel)
};

} // end of namespace config

#endif // KGD_WATCHMAKER_CONFIG_H
