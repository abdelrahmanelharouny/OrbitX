#pragma once

#include <memory>
#include <string>
#include <vector>

namespace agss {

class CelestialBody;
class SceneNode;

struct SolarSystemBuildResult {
  std::unique_ptr<SceneNode> root;
  std::vector<std::unique_ptr<CelestialBody>> bodiesOwned;
  std::vector<CelestialBody*> bodiesRaw;
};

class SolarSystemFactory {
public:
  static SolarSystemBuildResult LoadFromJsonFile(const std::string& path);
};

} // namespace agss

