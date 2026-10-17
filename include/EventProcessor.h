#pragma once

// watches for the menus the player is shown in, through FUCK's menu listener
class EventProcessor final : public REX::Singleton<EventProcessor>
{
public:
    void Register();
    void ApplyLiveSettings();

private:
    void OnMenu(std::string_view menuName, bool opening);
    void QueueStop();

    FUCK::MenuEventListener listener;
    std::string activeMenu;
    std::uint32_t stopGeneration = 0;
    bool menuOpen = false;
    bool tweenOpen = false;
};
