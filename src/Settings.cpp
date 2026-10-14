#include "Settings.h"

namespace
{
    // the settings and their ini keys. defaults come from Values, and only values that differ are written
    const FUCK::INI::Fields<Settings::Values> FIELDS{ "General", {
        { "bEnable", &Settings::Values::enabled },
        { "bBarterMenu", &Settings::Values::barterMenu },
        { "bAnimateCharacter", &Settings::Values::animateCharacter },
        { "bRememberCamera", &Settings::Values::rememberCamera },
        { "bCameraControls", &Settings::Values::cameraControls },
        { "bFixWeaponsOnViewSwitch", &Settings::Values::fixWeaponsOnViewSwitch },
        { "bTutorialSeen", &Settings::Values::tutorialSeen },
    } };

    FUCK::PluginSettings& GetINI()
    {
        static FUCK::PluginSettings ini;
        return ini;
    }
}

namespace Settings
{
    void SetDefaults()
    {
        // the tutorial has been seen either way
        const bool seen = values.tutorialSeen;
        values = {};
        values.tutorialSeen = seen;
    }

    void Load()
    {
        GetINI().Load(FIELDS, values);
        logger::info("[Settings] enabled={} animateCharacter={}", values.enabled, values.animateCharacter);
    }

    // only what differs from the defaults is written
    void Save()
    {
        GetINI().Save(FIELDS, values);
    }

    bool IsWatchedMenu(std::string_view menuName)
    {
        return menuName == RE::InventoryMenu::MENU_NAME || menuName == RE::MagicMenu::MENU_NAME || (values.barterMenu && menuName == RE::BarterMenu::MENU_NAME);
    }
}
