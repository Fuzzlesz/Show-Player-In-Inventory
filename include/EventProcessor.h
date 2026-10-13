#pragma once

class EventProcessor final : public REX::Singleton<EventProcessor>, public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* source) override;

    void ApplyLiveSettings();
    [[nodiscard]] const RE::BSFixedString& GetActiveMenu() const;

private:
    RE::BSFixedString activeMenu;
    bool menuOpen = false;
    bool tweenOpen = false;
};
