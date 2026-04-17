#pragma once

namespace agss::units {

constexpr double KM_PER_AU = 149597870.7;

inline double KmToAu(double km) { return km / KM_PER_AU; }
inline double AuToKm(double au) { return au * KM_PER_AU; }

} // namespace agss::units

