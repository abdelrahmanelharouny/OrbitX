#include "scene/SceneManager.h"

namespace agss {

SceneManager::SceneManager() : m_root(std::make_unique<SceneNode>("Root")) {}

void SceneManager::UpdateWorldTransforms() {
  if (m_root) {
    m_root->UpdateWorldTransform(glm::mat4(1.0f));
  }
}

} // namespace agss

