#pragma once

#include <glm/mat4x4.hpp>
#include <memory>
#include <string>
#include <vector>

namespace agss {

class SceneNode {
public:
  explicit SceneNode(std::string name = "");
  virtual ~SceneNode() = default;

  SceneNode(const SceneNode&) = delete;
  SceneNode& operator=(const SceneNode&) = delete;

  const std::string& GetName() const { return m_name; }

  void AddChild(std::unique_ptr<SceneNode> child);
  const std::vector<std::unique_ptr<SceneNode>>& GetChildren() const { return m_children; }

  SceneNode* GetParent() const { return m_parent; }

  const glm::mat4& GetLocalTransform() const { return m_localTransform; }
  const glm::mat4& GetWorldTransform() const { return m_worldTransform; }

  void SetLocalTransform(const glm::mat4& t) { m_localTransform = t; }
  void UpdateWorldTransform(const glm::mat4& parentWorld);

  virtual void Update(double dt) { (void)dt; }

private:
  std::string m_name;
  glm::mat4 m_localTransform{1.0f};
  glm::mat4 m_worldTransform{1.0f};
  SceneNode* m_parent = nullptr;
  std::vector<std::unique_ptr<SceneNode>> m_children;
};

} // namespace agss

