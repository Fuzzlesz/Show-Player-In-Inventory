#include "SettingsUI.h"
#include "EventProcessor.h"
#include "MenuCamera.h"
#include "Settings.h"

#include <IconsFontAwesome6.h>

namespace
{
    constexpr std::uint32_t NO_KEY = SKSE::InputMap::kMaxMacros;
    constexpr auto BIND_POPUP = "Bind Button";
    constexpr float POPUP_WIDTH = 560.0f;

    // flick's key icons have no wheel or drag, so these come straight from imgui icons
    constexpr auto DRAG_ICON = "Data/Interface/ImGuiIcons/Icons/MouseMove.png";
    constexpr auto WHEEL_ICON = "Data/Interface/ImGuiIcons/Icons/Wheel.png";
    constexpr std::uint32_t LEFT_CTRL = std::to_underlying(RE::BSWin32KeyboardDevice::Key::kLeftControl);
    constexpr std::uint32_t LEFT_SHIFT = std::to_underlying(RE::BSWin32KeyboardDevice::Key::kLeftShift);
    constexpr std::array<std::uint32_t, 4> CLOSE_KEYS{ std::to_underlying(RE::BSWin32KeyboardDevice::Key::kEnter), std::to_underlying(RE::BSWin32KeyboardDevice::Key::kEscape), SKSE::InputMap::kGamepadButtonOffset_A, SKSE::InputMap::kGamepadButtonOffset_B };

    std::atomic<REX::INI::U32<>*> bindTarget = nullptr;
    std::atomic<std::uint32_t> heldKey = NO_KEY;

    bool Checkbox(const char* label, REX::INI::Bool<>& setting, const char* help)
    {
        bool value = setting.GetValue();
        const bool changed = FUCK::Checkbox(label, &value);
        if (changed) {
            setting.SetValue(value);
            Settings::Save();
        }

        FUCK::SetTooltip(help);
        return changed;
    }

    void Slider(const char* label, REX::INI::F32<>& setting, Settings::Range range, const char* format, const char* help)
    {
        float value = setting.GetValue();
        if (FUCK::SliderFloat(label, &value, range.min, range.max, format)) {
            setting.SetValue(value);
        }

        if (FUCK::IsItemDeactivatedAfterEdit()) {
            Settings::Save();
        }

        FUCK::SetTooltip(help);
    }

    void BindRow(const char* label, REX::INI::U32<>& setting, const char* help)
    {
        std::string keyName = SKSE::InputMap::GetKeyName(setting.GetValue());
        if (keyName.empty()) {
            keyName = std::to_string(setting.GetValue());
        }

        FUCK::LeftLabel(label);
        if (FUCK::Button(std::format("{}###{}", keyName, label).c_str())) {
            FUCK::OpenPopup(BIND_POPUP);
            bindTarget = &setting;
        }
        FUCK::SetTooltip(help);
    }

    [[nodiscard]] bool IsBindable(std::uint32_t code, const REX::INI::U32<>* target)
    {
        if (target == &Settings::cameraButton) {
            return code >= SKSE::InputMap::kMacro_GamepadOffset && code < SKSE::InputMap::kMaxMacros;
        }

        return code < SKSE::InputMap::kMacro_MouseWheelOffset;
    }

    struct Icon
    {
        ImTextureID texture = 0;
        ImVec2 size{};
        const char* name = "";
    };

    Icon KeyIcon(std::uint32_t code)
    {
        const auto key = MenuCamera::ToFlickKey(code);
        Icon icon;
        icon.texture = FUCK::GetIconForKey(key, &icon.size);
        if (!icon.texture) {
            icon.name = FUCK::GetKeyName(key);
        }

        return icon;
    }

    void DrawIcon(const Icon& icon)
    {
        if (!icon.texture) {
            FUCK::AlignTextToFramePadding();
            FUCK::TextUnformatted(icon.name);
            return;
        }

        FUCK::DrawImage(icon.texture, icon.size);
    }

    void Row(const char* action, std::initializer_list<Icon> icons)
    {
        FUCK::LeftLabel(action);
        for (const auto& icon : icons) {
            if (&icon != icons.begin()) {
                FUCK::SameLine();
            }

            DrawIcon(icon);
        }
    }

    // FUCK-RACE's window pattern
    class ControlsWindow final : public FUCK::IWindow, public REX::Singleton<ControlsWindow>
    {
    public:
        const char* Id() const override
        {
            return "Controls";
        }

        const char* Title() const override
        {
            return "Show Player In Inventory";
        }

        void Draw() override
        {
            const bool appearing = FUCK::IsWindowAppearing();
            if (appearing && !drag.IsLoaded()) {
                drag = FUCK::Image(DRAG_ICON);
                wheel = FUCK::Image(WHEEL_ICON);
            }

            // flick's window background follows the user's transparency, this box stays solid
            const ImVec2 pos = FUCK::GetWindowPos();
            const ImVec2 size = FUCK::GetWindowSize();
            const float border = FUCK::GetStyleVar(ImGuiStyleVar_WindowBorderSize);
            ImVec4 background = FUCK::GetStyleColorVec4(ImGuiCol_WindowBg);
            background.w = 1.0f;
            FUCK::DrawRectFilled(ImVec2(pos.x + border, pos.y + border), ImVec2(pos.x + size.x - border, pos.y + size.y - border), background, FUCK::GetStyleVar(ImGuiStyleVar_WindowRounding));
            lastPos = pos;
            lastSize = size;

            const bool gamepad = FUCK::GetInputDevice() == FUCK::InputDevice::kGamepad;
            const float height = KeyIcon(LEFT_CTRL).size.y;
            const Icon dragIcon{ drag.GetID(), ImVec2(height * drag.GetWidth() / drag.GetHeight(), height) };
            const Icon wheelIcon{ wheel.GetID(), ImVec2(height * wheel.GetWidth() / wheel.GetHeight(), height) };

            FUCK::Dummy(ImVec2(FUCK::Scale(POPUP_WIDTH), 0.0f));
            FUCK::PushFontScaled(FUCK::GetFont(FUCK::Font::kLarge), 1.0f);
            FUCK::CenteredText("Show Player In Inventory Controls");
            FUCK::PopFont();
            FUCK::Separator();

            if (FUCK::BeginTabBar("Controls")) {
                if (FUCK::BeginTabItem(ICON_FA_COMPUTER_MOUSE "  Mouse & Keyboard", appearing && !gamepad ? ImGuiTabItemFlags_SetSelected : 0)) {
                    Row("Turn your character", { KeyIcon(Settings::rotateKey.GetValue()), dragIcon });
                    Row("Move your character", { KeyIcon(Settings::MIDDLE_MOUSE), dragIcon });
                    Row("Tilt the camera", { KeyIcon(LEFT_CTRL), KeyIcon(Settings::MIDDLE_MOUSE), dragIcon });
                    Row("Zoom", { KeyIcon(LEFT_CTRL), wheelIcon });
                    Row("Field of view", { KeyIcon(LEFT_CTRL), KeyIcon(LEFT_SHIFT), wheelIcon });
                    FUCK::EndTabItem();
                }

                if (FUCK::BeginTabItem(ICON_FA_GAMEPAD "  Controller", appearing && gamepad ? ImGuiTabItemFlags_SetSelected : 0)) {
                    const Icon hold = KeyIcon(Settings::cameraButton.GetValue());
                    Row("Turn and zoom", { hold, KeyIcon(SKSE::InputMap::kGamepadButtonOffset_RIGHT_THUMB) });
                    Row("Move your character", { hold, KeyIcon(SKSE::InputMap::kGamepadButtonOffset_LEFT_THUMB) });
                    Row("Field of view", { hold, KeyIcon(SKSE::InputMap::kGamepadButtonOffset_LT), KeyIcon(SKSE::InputMap::kGamepadButtonOffset_RT) });
                    FUCK::EndTabItem();
                }

                FUCK::EndTabBar();
            }

            FUCK::Spacing();

            const float width = FUCK::CalcTextSize("OK").x + FUCK::GetStyleVarVec(ImGuiStyleVar_FramePadding).x * 2.0f;
            FUCK::SetCursorPosX((size.x - width) * 0.5f);
            if (FUCK::Button("OK")) {
                SetOpen(false);
            }

            FUCK::SetItemDefaultFocus();
        }

        bool IsOpen() const override
        {
            return visible;
        }

        void SetOpen(bool a_open) override
        {
            visible = a_open;
            if (!a_open && !Settings::tutorialSeen.GetValue()) {
                Settings::tutorialSeen.SetValue(true);
                Settings::Save();
            }
        }

        FUCK::WindowFlags GetFlags() const override
        {
            auto flags = FUCK::WindowFlags::kNoDecoration | FUCK::WindowFlags::kAutoResize | FUCK::WindowFlags::kNoResize | FUCK::WindowFlags::kCloseOnEsc;

            // FUCK-RACE's hover check, input only goes to the box while the cursor is on it
            const ImVec2 mouse = FUCK::GetMousePos();
            const bool hovered = mouse.x >= lastPos.x && mouse.x <= lastPos.x + lastSize.x && mouse.y >= lastPos.y && mouse.y <= lastPos.y + lastSize.y;
            if (!hovered && !FUCK::IsAnyItemActive()) {
                flags = flags | FUCK::WindowFlags::kPassInputToGame;
            }

            return flags;
        }

        ImVec2 GetDefaultPos() const override
        {
            return ImVec2((FUCK::GetDisplaySize().x - FUCK::Scale(POPUP_WIDTH)) * 0.5f, FUCK::Scale(100.0f));
        }

        bool OnAsyncInput(const void* a_events) override
        {
            for (const auto key : CLOSE_KEYS) {
                if (FUCK::IsInputPressed(a_events, MenuCamera::ToFlickKey(key))) {
                    SetOpen(false);
                    return true;
                }
            }

            return false;
        }

    private:
        FUCK::Image drag;
        FUCK::Image wheel;
        ImVec2 lastPos{};
        ImVec2 lastSize{};
        std::atomic_bool visible = false;
    };

    class SettingsPage final : public FUCK::ITool, public REX::Singleton<SettingsPage>
    {
    public:
        const char* Name() const override
        {
            return "Settings";
        }

        const char* Group() const override
        {
            return "Show Player In Inventory";
        }

        void Draw() override
        {
            FUCK::SeparatorText("GENERAL");

            if (Checkbox("Enable", Settings::enabled, "Toggles Show Player In Inventory ON and OFF.")) {
                EventProcessor::GetSingleton()->ApplyLiveSettings();
            }

            Checkbox("Barter Menu", Settings::barterEnabled, "Shows your character in the barter menu.");

            FUCK::BeginDisabled(Settings::skyrimSouls);
            Checkbox("Idle While Paused", Settings::idleWhilePaused, "Keeps your character's idle animation playing in paused menus.");
            FUCK::EndDisabled();

            Checkbox("Lock Controls", Settings::lockControls, "Stops your character from walking or looking around while the menu is open.");
            Checkbox("Fix Weapons When Switching View", Settings::fixWeaponsOnViewSwitch, "Puts your drawn weapons and shield back in your hands when you switch between first and third person.");

            if (Checkbox("Enable Logging", Settings::loggingEnabled, "Writes detailed activity to ShowPlayerInInventory.log.")) {
                Settings::ApplyLogLevel();
            }

            FUCK::Spacing();
            FUCK::SeparatorText("CAMERA");

            Slider("Screen X", Settings::screenX, Settings::SCREEN_RANGE, "%.2f", "Where your character sits across the screen, from left to right.");
            Slider("Screen Y", Settings::screenY, Settings::SCREEN_RANGE, "%.2f", "Where your character sits on the screen, from top to bottom.");
            Slider("Distance", Settings::distance, Settings::DISTANCE_RANGE, "%.0f", "How far the camera is from your character.");
            Slider("Pitch", Settings::pitch, Settings::PITCH_RANGE, "%.0f", "How far above or below your character the camera looks from.");
            Slider("FOV", Settings::fov, Settings::FOV_RANGE, "%.0f", "Adjusts the camera field of view.");

            FUCK::Spacing();
            FUCK::SeparatorText("CONTROLS");

            BindRow("Rotate Button", Settings::rotateKey, "Hold and drag the mouse to turn your character; over the item list the button keeps its normal use.");
            BindRow("Controller Button", Settings::cameraButton, "Hold to turn, zoom and move your character with the sticks and triggers.");

            if (FUCK::Button("Show Controls")) {
                SettingsUI::ShowControls();
            }
            FUCK::SetTooltip("Shows the controls popup.");

            if (FUCK::BeginPopupModal(BIND_POPUP)) {
                const auto* target = bindTarget.load();
                if (!target) {
                    FUCK::CloseCurrentPopup();
                } else if (target == &Settings::cameraButton) {
                    FUCK::TextUnformatted("Press any controller button to bind.");
                } else {
                    FUCK::TextUnformatted("Press any key or mouse button to bind.");
                    FUCK::TextUnformatted("(Esc binds Escape.)");
                }

                FUCK::EndPopup();
            }

            FUCK::Spacing();

            if (FUCK::Button("Reset Defaults")) {
                Settings::SetDefaults();
                Settings::Save();
                Settings::ApplyLogLevel();
                EventProcessor::GetSingleton()->ApplyLiveSettings();
            }
            FUCK::SetTooltip("Restores the built-in defaults.");
        }

        void OnClose() override
        {
            bindTarget = nullptr;
            heldKey = NO_KEY;
        }

        bool OnAsyncInput(const void* a_events) override
        {
            const auto* events = static_cast<const RE::InputEvent* const*>(a_events);
            if (!events) {
                return false;
            }

            auto* target = bindTarget.load();
            if (!target && heldKey == NO_KEY) {
                return MenuCamera::GetSingleton()->ProcessInput(events);
            }

            for (auto* current = *events; current; current = current->next) {
                const auto* button = current->AsButtonEvent();
                if (!button) {
                    continue;
                }

                const auto code = MenuCamera::GetKeyCode(button);
                if (button->IsUp() && code == heldKey) {
                    heldKey = NO_KEY;
                } else if (target && button->IsDown() && IsBindable(code, target)) {
                    target->SetValue(code);
                    Settings::Save();
                    heldKey = code;
                    bindTarget = nullptr;
                    target = nullptr;
                }
            }

            return true;
        }
    };
}

namespace SettingsUI
{
    void Register()
    {
        if (!FUCK::Connect(SKSE::GetPluginName().data())) {
            SKSE::stl::report_and_fail("FLICK 1.5.0 or newer is required.");
        }

        if (!std::filesystem::exists(DRAG_ICON) || !std::filesystem::exists(WHEEL_ICON)) {
            SKSE::stl::report_and_fail("ImGui Icons is required.");
        }

        FUCK::RegisterTool(SettingsPage::GetSingleton());
        FUCK::RegisterWindow(ControlsWindow::GetSingleton());
        logger::info("[Show Player In Inventory] Registered FLICK settings and controls.");
    }

    void ShowControls()
    {
        ControlsWindow::GetSingleton()->SetOpen(true);
    }
}
