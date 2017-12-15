#ifndef CHARGE_COMMON_OPTIONS_HPP
#define CHARGE_COMMON_OPTIONS_HPP

namespace charge::common {
class Options {
    Options() noexcept;

  public:
    static const Options &get();

    // CHARGE_TAIL_STATISTICS=On
    bool tail_statistics;
    // CHAGRE_TAIL_EXPERIMENTS=On
    bool tail_experiment;
    // CHARGE_TAIL_MEMORY=On
    bool tail_memory;
};
}

#endif
