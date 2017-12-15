#ifndef CHARGE_EV_FILES_HPP
#define CHARGE_EV_FILES_HPP

#include "ev/charging_model.hpp"

#include "common/binary.hpp"
#include "common/serialization.hpp"

namespace charge::ev::files {
inline auto read_charger(const std::string &base_path) {
    std::vector<double> charger;
    common::BinaryReader reader(base_path + "/charger");
    common::serialization::read(reader, charger);
    return charger;
}
inline void write_charger(const std::string &base_path, const std::vector<double> &charger) {
    common::BinaryWriter writer(base_path + "/charger");
    common::serialization::write(writer, charger);
}
}

#endif
