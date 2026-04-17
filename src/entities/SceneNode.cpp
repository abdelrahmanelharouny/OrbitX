#include "entities/SceneNode.h"

namespace agss {

SceneNode::SceneNode(std::string name) : m_name(std::move(name)) {}

void SceneNode::AddChild(std::unique_ptr<SceneNode> child) {
  if (!child) {
    return;
  }
  child->m_parent = this;
  m_children.emplace_back(std::move(child));
}

void SceneNode::UpdateWorldTransform(const glm::mat4& parentWorld) {
  m_worldTransform = parentWorld * m_localTransform;
  for (auto& c : m_children) {
    c->UpdateWorldTransform(m_worldTransform);
  }
}

} // namespace agss

