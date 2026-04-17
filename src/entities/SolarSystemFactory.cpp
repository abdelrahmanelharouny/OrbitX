#include "entities/SolarSystemFactory.h"

#include "entities/CelestialBody.h"
#include "entities/SceneNode.h"
#include "physics/Units.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace agss {

static glm::dvec3 ReadDVec3(const nlohmann::json& j) {
  return glm::dvec3(j.at(0).get<double>(), j.at(1).get<double>(), j.at(2).get<double>());
}

static glm::vec3 ReadVec3(const nlohmann::json& j) {
  return glm::vec3(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>());
}

SolarSystemBuildResult SolarSystemFactory::LoadFromJsonFile(const std::string& path) {
  std::ifstream f(path);
  if (!f) {
    throw std::runtime_error("Failed to open solar system json: " + path);
  }

  nlohmann::json rootJson;
  f >> rootJson;

  SolarSystemBuildResult out;
  out.root = std::make_unique<SceneNode>("SolarSystem");

  const auto& bodies = rootJson.at("bodies");
  out.bodiesOwned.reserve(bodies.size());
  out.bodiesRaw.reserve(bodies.size());

  for (const auto& b : bodies) {
    auto body = std::make_unique<CelestialBody>(b.at("name").get<std::string>());
    body->radius_km = b.at("radius_km").get<double>();
    body->Physics().mass = b.at("mass").get<double>();
    body->Physics().position = ReadDVec3(b.at("position"));
    body->Physics().velocity = ReadDVec3(b.at("velocity"));
    body->Physics().radiusAu = units::KmToAu(body->radius_km);
    body->CommitPreviousPhysicsState();
    if (b.contains("color")) {
      body->baseColor = ReadVec3(b.at("color"));
    }

    out.bodiesRaw.push_back(body.get());
    out.bodiesOwned.emplace_back(std::move(body));
  }

  return out;
}

} // namespace agss

