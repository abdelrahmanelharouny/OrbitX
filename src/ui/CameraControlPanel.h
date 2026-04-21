#pragma once

#include <imgui.h>
#include <functional>

namespace agss {

/**
 * Camera Control Panel - UI-assisted camera controls
 * Features: Focus on selected, Reset camera, Smooth transitions
 */
class CameraControlPanel {
public:
    CameraControlPanel();

    // Set callbacks for camera control actions
    void SetOnFocusCallback(std::function<void()> onFocus) { m_onFocus = onFocus; }
    void SetOnResetCallback(std::function<void()> onReset) { m_onReset = onReset; }
    void SetOnNextBodyCallback(std::function<void()> onNext) { m_onNext = onNext; }
    void SetOnPrevBodyCallback(std::function<void()> onPrev) { m_onPrev = onPrev; }

    // Render the camera control panel
    void Render(bool& isOpen);

    // Get preferred size
    float GetPreferredWidth() const { return 200.0f; }

private:
    // Callbacks
    std::function<void()> m_onFocus;
    std::function<void()> m_onReset;
    std::function<void()> m_onNext;
    std::function<void()> m_onPrev;

    // UI State
    bool m_showCameraControls = true;
};

} // namespace agss
