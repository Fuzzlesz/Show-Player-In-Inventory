#pragma once

#define SPII_API_VERSION 1

#pragma pack(push, 1)
struct SPII_Framing
{
    float screenX;
    float screenY;
    float distance;
    float pitch;
    float fov;
};

struct SPII_Interface
{
    unsigned int version;

    bool (*IsEnabled)();
    bool (*IsActive)();
    const char* (*GetActiveMenu)();
    void (*GetFraming)(SPII_Framing*);
    bool (*IsControlling)();
    bool (*Turn)(float);
    bool (*SetFraming)(const SPII_Framing*);
};
#pragma pack(pop)

// copy this file into your plugin, call SPII::Connect() from kPostLoad on, then use it from the main thread
namespace SPII
{
    inline SPII_Interface*& GetInterface()
    {
        static SPII_Interface* api = nullptr;
        return api;
    }

    inline bool Connect()
    {
        const auto module = REX::W32::GetModuleHandleW(L"ShowPlayerInInventory.dll");
        const auto request = module ? reinterpret_cast<SPII_Interface* (*)()>(REX::W32::GetProcAddress(module, "RequestSPII")) : nullptr;
        auto* api = request ? request() : nullptr;
        if (!api || api->version < SPII_API_VERSION) {
            return false;
        }

        GetInterface() = api;
        return true;
    }

    inline bool IsEnabled()
    {
        const auto* api = GetInterface();
        return api && api->IsEnabled();
    }

    inline bool IsActive()
    {
        const auto* api = GetInterface();
        return api && api->IsActive();
    }

    inline const char* GetActiveMenu()
    {
        const auto* api = GetInterface();
        return api ? api->GetActiveMenu() : "";
    }

    inline SPII_Framing GetFraming()
    {
        SPII_Framing framing{};
        if (const auto* api = GetInterface()) {
            api->GetFraming(&framing);
        }

        return framing;
    }

    // true while the player is turning, panning or holding the camera button, skip your own controls then
    inline bool IsControlling()
    {
        const auto* api = GetInterface();
        return api && api->IsControlling();
    }

    // turns the player in radians, same direction as the mouse turn, the camera stays put
    inline bool Turn(float delta)
    {
        const auto* api = GetInterface();
        return api && api->Turn(delta);
    }

    // clamped to the settings ranges and saved when the menu closes
    inline bool SetFraming(const SPII_Framing& framing)
    {
        const auto* api = GetInterface();
        return api && api->SetFraming(&framing);
    }
}
