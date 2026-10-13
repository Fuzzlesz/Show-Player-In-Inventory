#pragma once

namespace Settings
{
    struct Range
    {
        float min;
        float max;
    };

    inline constexpr const char* INI_PATH = "Data/SKSE/Plugins/ShowPlayerInInventory.ini";
    inline constexpr Range SCREEN_RANGE{ 0.0f, 1.0f };
    inline constexpr Range DISTANCE_RANGE{ 50.0f, 600.0f };
    inline constexpr Range PITCH_RANGE{ -60.0f, 60.0f };
    inline constexpr Range FOV_RANGE{ 20.0f, 120.0f };
    inline constexpr std::uint32_t RIGHT_MOUSE = SKSE::InputMap::kMacro_MouseButtonOffset + std::to_underlying(RE::BSWin32MouseDevice::Keys::kRightButton);
    inline constexpr std::uint32_t MIDDLE_MOUSE = SKSE::InputMap::kMacro_MouseButtonOffset + std::to_underlying(RE::BSWin32MouseDevice::Keys::kMiddleButton);

    inline bool skyrimSouls = false;

    inline REX::INI::Bool<> enabled{ "General", "bEnable", true };
    inline REX::INI::Bool<> barterEnabled{ "General", "bBarterMenu", false };
    inline REX::INI::Bool<> idleWhilePaused{ "General", "bIdleWhilePaused", true };
    inline REX::INI::Bool<> lockControls{ "General", "bLockControls", true };
    inline REX::INI::Bool<> fixWeaponsOnViewSwitch{ "General", "bFixWeaponsOnViewSwitch", false };
    inline REX::INI::Bool<> loggingEnabled{ "General", "bEnableLogging", false };
    inline REX::INI::Bool<> tutorialSeen{ "General", "bTutorialSeen", false };
    inline REX::INI::F32<> screenX{ "Camera", "fScreenX", 0.7f };
    inline REX::INI::F32<> screenY{ "Camera", "fScreenY", 0.5f };
    inline REX::INI::F32<> distance{ "Camera", "fDistance", 220.0f };
    inline REX::INI::F32<> pitch{ "Camera", "fPitch", 0.0f };
    inline REX::INI::F32<> fov{ "Camera", "fFOV", 60.0f };
    inline REX::INI::U32<> rotateKey{ "Controls", "iRotateKey", RIGHT_MOUSE };
    inline REX::INI::U32<> cameraButton{ "Controls", "iCameraButton", SKSE::InputMap::kGamepadButtonOffset_RIGHT_SHOULDER };

    void SetDefaults();
    void ApplyLogLevel();
    void Load();
    void Save();
    [[nodiscard]] bool IsWatchedMenu(const RE::BSFixedString& menuName);
}
