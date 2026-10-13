#include "Settings.h"

namespace Settings
{
    void SetDefaults()
    {
        enabled.SetValue(enabled.GetValueDefault());
        barterEnabled.SetValue(barterEnabled.GetValueDefault());
        idleWhilePaused.SetValue(!skyrimSouls);
        lockControls.SetValue(lockControls.GetValueDefault());
        fixWeaponsOnViewSwitch.SetValue(fixWeaponsOnViewSwitch.GetValueDefault());
        loggingEnabled.SetValue(loggingEnabled.GetValueDefault());
        screenX.SetValue(screenX.GetValueDefault());
        screenY.SetValue(screenY.GetValueDefault());
        distance.SetValue(distance.GetValueDefault());
        pitch.SetValue(pitch.GetValueDefault());
        fov.SetValue(fov.GetValueDefault());
        rotateKey.SetValue(rotateKey.GetValueDefault());
        cameraButton.SetValue(cameraButton.GetValueDefault());
    }

    void ApplyLogLevel()
    {
        const auto level = loggingEnabled.GetValue() ? spdlog::level::info : spdlog::level::warn;
        spdlog::set_level(level);
        spdlog::flush_on(level);
    }

    void Load()
    {
        if (!std::filesystem::exists(INI_PATH)) {
            logger::warn("[Settings] Could not open {}. Defaults will be used.", INI_PATH);
        }

        const auto ini = REX::INI::SettingStore::GetSingleton();
        ini->Init(INI_PATH, "");
        ini->Load();

        // FUCK-RACE's compat check: unpaused menus already animate the player
        skyrimSouls = REX::W32::GetModuleHandleW(L"SkyrimSoulsRE.dll") != nullptr;
        if (skyrimSouls && idleWhilePaused.GetValue()) {
            idleWhilePaused.SetValue(false);
            Save();
        }

        ApplyLogLevel();

        logger::info("[Settings] enabled={} idleWhilePaused={} skyrimSouls={} screenX={} screenY={} distance={} pitch={} fov={}", enabled.GetValue(), idleWhilePaused.GetValue(), skyrimSouls, screenX.GetValue(), screenY.GetValue(), distance.GetValue(), pitch.GetValue(), fov.GetValue());
    }

    void Save()
    {
        REX::INI::SettingStore::GetSingleton()->Save();
    }

    bool IsWatchedMenu(const RE::BSFixedString& menuName)
    {
        return menuName == RE::InventoryMenu::MENU_NAME || menuName == RE::MagicMenu::MENU_NAME || (barterEnabled.GetValue() && menuName == RE::BarterMenu::MENU_NAME);
    }
}
