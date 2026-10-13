#pragma once

#include "Settings.h"

class MenuCamera : public REX::Singleton<MenuCamera>
{
public:
    static void InstallHooks();

    void Start();
    void Stop();
    void Update(float interval);
    [[nodiscard]] bool ProcessInput(const RE::InputEvent* const* events);
    void OnPerspectiveUpdate(bool firstPerson);
    void SetCacheFrozen(bool frozen);
    [[nodiscard]] bool IsActive() const;
    [[nodiscard]] bool IsControlling() const;
    bool TurnPlayer(float delta);
    bool SetFraming(float screenX, float screenY, float distance, float pitch, float fov);

    [[nodiscard]] static std::uint32_t GetKeyCode(const RE::ButtonEvent* button);
    [[nodiscard]] static std::uint32_t ToFlickKey(std::uint32_t code);

private:
    void Turn(float delta);
    void Adjust(REX::INI::F32<>& setting, float delta, Settings::Range range);
    void Set(REX::INI::F32<>& setting, float value, Settings::Range range);
    void ApplyTransform();

    REX::EnumSet<RE::ControlMap::UEFlag, std::uint32_t> disabledControls;

    float playerAngleX = 0.0f;
    float playerAngleZ = 0.0f;
    float worldFOV = 0.0f;
    float pendingZoom = 0.0f;
    float pendingFOV = 0.0f;

    bool active = false;
    bool smoothCamControl = false;
    bool cachedFirstPerson = false;
    bool cacheFrozen = false;
    bool headtrackingEnabled = false;
    bool headTrackSpineEnabled = false;
    bool eyeTrackingEnabled = false;
    bool allowAutoVanityMode = false;
    bool framingChanged = false;
    bool turning = false;
    bool dragging = false;
    bool gamepadLocked = false;
};
