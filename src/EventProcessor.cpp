#include "EventProcessor.h"

#include "MenuCamera.h"
#include "Settings.h"

namespace
{
    std::atomic_bool blurClearQueued = false;

    void ClearVanillaMenuBlur()
    {
        auto* blur = RE::UIBlurManager::GetSingleton();
        while (blur->blurCount > 0) {
            blur->DecrementBlurCount();
        }
    }

    void QueueVanillaMenuBlurClear()
    {
        ClearVanillaMenuBlur();

        bool expected = false;
        if (!blurClearQueued.compare_exchange_strong(expected, true)) {
            return;
        }

        SKSE::GetTaskInterface()->AddUITask([] {
            ClearVanillaMenuBlur();
            blurClearQueued.store(false);
        });
    }
}

RE::BSEventNotifyControl EventProcessor::ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
    if (!event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (event->menuName == RE::TweenMenu::MENU_NAME) {
        tweenOpen = event->opening;
        MenuCamera::GetSingleton()->SetCacheFrozen(tweenOpen || menuOpen);
    }

    if (menuOpen) {
        QueueVanillaMenuBlurClear();
    }

    if (event->opening) {
        if (!Settings::IsWatchedMenu(event->menuName)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        menuOpen = true;
        MenuCamera::GetSingleton()->SetCacheFrozen(true);
        activeMenu = event->menuName;
        logger::info("[EventProcessor] Watched menu opened.");
        ApplyLiveSettings();
        return RE::BSEventNotifyControl::kContinue;
    }

    if (menuOpen && event->menuName == activeMenu) {
        logger::info("[EventProcessor] Watched menu closed.");
        MenuCamera::GetSingleton()->Stop();
        menuOpen = false;
        MenuCamera::GetSingleton()->SetCacheFrozen(tweenOpen);
        QueueVanillaMenuBlurClear();
    }

    return RE::BSEventNotifyControl::kContinue;
}

void EventProcessor::ApplyLiveSettings()
{
    if (!menuOpen) {
        return;
    }

    if (Settings::enabled.GetValue()) {
        MenuCamera::GetSingleton()->Start();
    } else {
        MenuCamera::GetSingleton()->Stop();
    }

    QueueVanillaMenuBlurClear();
}

const RE::BSFixedString& EventProcessor::GetActiveMenu() const
{
    return activeMenu;
}
