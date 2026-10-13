#include "APIManager.h"
#include "EventProcessor.h"
#include "MenuCamera.h"
#include "Settings.h"
#include "SettingsUI.h"
#include "ShowPlayerInInventoryAPI.h"
#include "ViewSwitchFix.h"

namespace
{
    void OnMessage(SKSE::MessagingInterface::Message* message)
    {
        switch (message->type) {
        case SKSE::MessagingInterface::kPostLoad:
            APIs::RegisterCallbacks();
            break;

        case SKSE::MessagingInterface::kPostPostLoad:
            APIs::RequestAPIs();
            break;

        case SKSE::MessagingInterface::kDataLoaded:
            Settings::Load();
            SettingsUI::Register();
            MenuCamera::InstallHooks();
            ViewSwitchFix::Install();
            RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(EventProcessor::GetSingleton());
            logger::info("[Show Player In Inventory] Registered menu watcher.");
            break;

        case SKSE::MessagingInterface::kNewGame:
        case SKSE::MessagingInterface::kPostLoadGame:
            ViewSwitchFix::Reset();
            break;

        default:
            break;
        }
    }

    bool IsEnabled()
    {
        return Settings::enabled.GetValue();
    }

    bool IsActive()
    {
        return MenuCamera::GetSingleton()->IsActive();
    }

    const char* GetActiveMenu()
    {
        return IsActive() ? EventProcessor::GetSingleton()->GetActiveMenu().c_str() : "";
    }

    void GetFraming(SPII_Framing* framing)
    {
        *framing = { Settings::screenX.GetValue(), Settings::screenY.GetValue(), Settings::distance.GetValue(), Settings::pitch.GetValue(), Settings::fov.GetValue() };
    }
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    Settings::ApplyLogLevel();

    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    logger::info("[Show Player In Inventory] Plugin loaded.");

    return true;
}

SKSE_EXPORT SPII_Interface* RequestSPII()
{
    static SPII_Interface api{ .version = SPII_API_VERSION, .IsEnabled = IsEnabled, .IsActive = IsActive, .GetActiveMenu = GetActiveMenu, .GetFraming = GetFraming };
    return &api;
}
