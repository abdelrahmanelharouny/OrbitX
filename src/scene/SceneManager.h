#pragma once

#include "entities/SceneNode.h"

#include <memory>

namespace agss {

class SceneManager {
public:
  SceneManager();

  SceneNode* GetRoot() const { return m_root.get(); }

  void UpdateWorldTransforms();

private:
  std::unique_ptr<SceneNode> m_root;
};

} // namespace agss

