#pragma once

namespace Settings
{
    struct Values
    {
        bool enabled = true;
        bool barterMenu = false;
        bool animateCharacter = true;
        bool rememberCamera = true;
        bool cameraControls = true;
        bool fixWeaponsOnViewSwitch = false;
        bool tutorialSeen = false;
    };

    // as they stand, kept in FUCK's settings for this plugin (Data/FUCKs/ShowPlayerInInventory)
    inline Values values;

    void SetDefaults();
    void Load();
    void Save();
    [[nodiscard]] bool IsWatchedMenu(std::string_view menuName);
}
