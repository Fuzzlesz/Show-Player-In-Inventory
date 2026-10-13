#include "MenuCamera.h"
#include "APIManager.h"
#include "SettingsUI.h"

// windows.h macro
#undef GetObject

namespace
{
    using UEFlag = RE::ControlMap::UEFlag;
    using MouseKey = RE::BSWin32MouseDevice::Keys;

    // pov switch and wheel zoom replace the old ini overrides, movement and looking are the lock
    constexpr std::array CONTROLS{ UEFlag::kPOVSwitch, UEFlag::kWheelZoom, UEFlag::kMovement, UEFlag::kLooking };

    constexpr std::uint32_t WHEEL_UP = SKSE::InputMap::kMacro_MouseButtonOffset + std::to_underlying(MouseKey::kWheelUp);
    constexpr std::uint32_t WHEEL_DOWN = SKSE::InputMap::kMacro_MouseButtonOffset + std::to_underlying(MouseKey::kWheelDown);

    // flick's cache keys: sticks at its own offsets, gamepad buttons and triggers offset twice
    constexpr std::uint32_t GAMEPAD_OFFSET = SKSE::InputMap::kMacro_GamepadOffset;
    constexpr std::uint32_t LEFT_STICK_X = GAMEPAD_OFFSET + 32;
    constexpr std::uint32_t LEFT_STICK_Y = GAMEPAD_OFFSET + 33;
    constexpr std::uint32_t RIGHT_STICK_X = GAMEPAD_OFFSET + 34;
    constexpr std::uint32_t RIGHT_STICK_Y = GAMEPAD_OFFSET + 35;
    constexpr std::uint32_t LEFT_TRIGGER = GAMEPAD_OFFSET + SKSE::InputMap::kGamepadButtonOffset_LT;
    constexpr std::uint32_t RIGHT_TRIGGER = GAMEPAD_OFFSET + SKSE::InputMap::kGamepadButtonOffset_RT;

    constexpr float MOUSE_TURN = 0.01f;
    constexpr float MOUSE_PAN = 0.001f;
    constexpr float MOUSE_PITCH = 0.1f;
    constexpr float WHEEL_ZOOM = 10.0f;
    constexpr float WHEEL_FOV = 2.0f;
    constexpr float GAMEPAD_TURN = 3.0f;
    constexpr float GAMEPAD_PAN = 0.5f;
    constexpr float GAMEPAD_ZOOM = 100.0f;
    constexpr float GAMEPAD_FOV = 30.0f;
    constexpr float GAMEPAD_DEADZONE = 0.25f;
    constexpr float SCROLL_SMOOTH_SPEED = 15.0f;

    // FUCK-RACE's ApplyDeadzone
    float Axis(std::uint32_t key)
    {
        const float value = FUCK::GetAnalogInput(key);
        if (std::abs(value) < GAMEPAD_DEADZONE) {
            return 0.0f;
        }

        const float scaled = (std::abs(value) - GAMEPAD_DEADZONE) / (1.0f - GAMEPAD_DEADZONE);
        return std::copysign(scaled * scaled, value);
    }

    float ZoomScale()
    {
        return Settings::distance.GetValue() / Settings::distance.GetValueDefault();
    }

    // FUCK-RACE's DrainSmoothed
    float Drain(float& pending, float interval)
    {
        const float step = std::abs(pending) > 0.001f ? pending * (1.0f - std::exp(-SCROLL_SMOOTH_SPEED * interval)) : pending;
        pending -= step;
        return step;
    }

    // FUCK-RACE's prompt lock, plus quantity and book menus
    bool IsPromptOpen()
    {
        auto* ui = RE::UI::GetSingleton();
        return ui->IsMenuOpen(RE::MessageBoxMenu::MENU_NAME) || ui->IsMenuOpen(RE::InterfaceStrings::GetSingleton()->quantityMenu) || ui->IsMenuOpen(RE::BookMenu::MENU_NAME);
    }

    template <class T>
    bool IsCursorOverList(RE::UI* ui)
    {
        const auto menu = ui->GetMenu<T>();
        if (!menu || !menu->uiMovie || !menu->GetRuntimeData().itemList) {
            return false;
        }

        // menu alpha 0 means the list is hidden, so it never blocks the turn
        RE::GFxValue alpha;
        if (menu->uiMovie->GetVariable(&alpha, "_root._alpha") && alpha.IsNumber() && alpha.GetNumber() == 0.0) {
            return false;
        }

        RE::GFxValue x;
        RE::GFxValue y;
        menu->uiMovie->GetVariable(&x, "_root._xmouse");
        menu->uiMovie->GetVariable(&y, "_root._ymouse");

        const RE::GFxValue args[3]{ x, y, RE::GFxValue(false) };
        RE::GFxValue hit;
        return menu->GetRuntimeData().itemList->root.Invoke("hitTest", &hit, args, 3) && hit.GetBool();
    }

    bool IsCursorOverList()
    {
        auto* ui = RE::UI::GetSingleton();
        return IsCursorOverList<RE::InventoryMenu>(ui) || IsCursorOverList<RE::MagicMenu>(ui) || IsCursorOverList<RE::BarterMenu>(ui);
    }

    void PerformAction(RE::Actor* actor, RE::DEFAULT_OBJECT id)
    {
        std::unique_ptr<RE::TESActionData> data{ RE::TESActionData::Create() };
        data->source = RE::NiPointer<RE::TESObjectREFR>(actor);
        data->action = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSAction>(id);
        data->Process();
    }

    template <class T>
    struct PerspectiveHook
    {
        static void Update(T* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState)
        {
            _Update(a_this, a_nextState);
            MenuCamera::GetSingleton()->OnPerspectiveUpdate(std::is_same_v<T, RE::FirstPersonState>);
        }

        static void Install()
        {
            REL::Relocation<std::uintptr_t> vtbl{ T::VTABLE[0] };
            _Update = vtbl.write_vfunc(0x3, Update);
        }

        static inline REL::Relocation<decltype(Update)> _Update;
    };

    // FUCK-RACE's RaceSexMenu hook: runs every frame, even while the game is paused
    template <class T>
    struct AdvanceMovieHook
    {
        static void AdvanceMovie(T* a_this, float a_interval, std::uint32_t a_currentTime)
        {
            _AdvanceMovie(a_this, a_interval, a_currentTime);
            MenuCamera::GetSingleton()->Update(a_interval);
        }

        static void Install()
        {
            REL::Relocation<std::uintptr_t> vtbl{ T::VTABLE[0] };
            _AdvanceMovie = vtbl.write_vfunc(0x5, AdvanceMovie);
        }

        static inline REL::Relocation<decltype(AdvanceMovie)> _AdvanceMovie;
    };
}

void MenuCamera::InstallHooks()
{
    PerspectiveHook<RE::FirstPersonState>::Install();
    PerspectiveHook<RE::ThirdPersonState>::Install();
    AdvanceMovieHook<RE::InventoryMenu>::Install();
    AdvanceMovieHook<RE::MagicMenu>::Install();
    AdvanceMovieHook<RE::BarterMenu>::Install();

    logger::info("[MenuCamera] Installed camera state and menu hooks.");
}

void MenuCamera::SetCacheFrozen(bool frozen)
{
    cacheFrozen = frozen;
}

bool MenuCamera::IsActive() const
{
    return active;
}

bool MenuCamera::IsControlling() const
{
    return active && (turning || dragging || (!gamepadLocked && FUCK::IsInputDown(ToFlickKey(Settings::cameraButton.GetValue()))));
}

bool MenuCamera::TurnPlayer(float delta)
{
    if (!active) {
        return false;
    }

    Turn(delta);
    return true;
}

// clamped and saved on close like the player's own framing
bool MenuCamera::SetFraming(float screenX, float screenY, float distance, float pitch, float fov)
{
    if (!active) {
        return false;
    }

    Set(Settings::screenX, screenX, Settings::SCREEN_RANGE);
    Set(Settings::screenY, screenY, Settings::SCREEN_RANGE);
    Set(Settings::distance, distance, Settings::DISTANCE_RANGE);
    Set(Settings::pitch, pitch, Settings::PITCH_RANGE);
    Set(Settings::fov, fov, Settings::FOV_RANGE);
    return true;
}

// one owner at a time, cleared when the menu closes
bool MenuCamera::RequestCameraControl(SKSE::PluginHandle owner)
{
    if (!active || (cameraOwner && *cameraOwner != owner)) {
        return false;
    }

    cameraOwner = owner;
    return true;
}

bool MenuCamera::ReleaseCameraControl(SKSE::PluginHandle owner)
{
    if (cameraOwner != owner) {
        return false;
    }

    cameraOwner.reset();
    return true;
}

void MenuCamera::OnPerspectiveUpdate(bool firstPerson)
{
    if (!active && !cacheFrozen) {
        cachedFirstPerson = firstPerson;
    }

    // souls rewrites the camera every frame, framing goes on top
    if (active && !firstPerson && Settings::skyrimSouls) {
        ApplyTransform();
    }
}

void MenuCamera::Start()
{
    if (active) {
        return;
    }

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (player->IsInCombat() || player->IsOnMount()) {
        logger::info("[MenuCamera] Skipped start. Player is in combat or on horseback.");
        return;
    }

    auto* camera = RE::PlayerCamera::GetSingleton();

    if (APIs::SmoothCam && APIs::SmoothCam->IsCameraEnabled()) {
        const auto result = APIs::SmoothCam->RequestCameraControl(SKSE::GetPluginHandle());

        if (result == SmoothCamAPI::APIResult::OK || result == SmoothCamAPI::APIResult::AlreadyGiven) {
            APIs::SmoothCam->RequestInterpolatorUpdates(SKSE::GetPluginHandle(), true);
            smoothCamControl = true;
            logger::info("[MenuCamera] SmoothCam camera control acquired.");
        } else {
            logger::warn("[MenuCamera] SmoothCam camera control request failed: {}", static_cast<std::uint8_t>(result));
        }
    }

    playerAngleX = player->GetAngleX();
    playerAngleZ = player->GetAngleZ();
    allowAutoVanityMode = camera->allowAutoVanityMode;

    player->GetGraphVariableBool("IsNPC", headtrackingEnabled);
    player->GetGraphVariableBool("bHeadTrackSpine", headTrackSpineEnabled);
    player->GetGraphVariableBool("bUseEyeTracking", eyeTrackingEnabled);
    player->SetGraphVariableBool("IsNPC", false);
    player->SetGraphVariableBool("bHeadTrackSpine", false);
    player->SetGraphVariableBool("bUseEyeTracking", false);

    if (auto* process = player->GetActorRuntimeData().currentProcess) {
        process->ClearActionHeadtrackTarget(true);

        // zero the speed and play the game's own stop
        auto& speed = process->middleHigh->currentMovementSpeed;
        if (speed > 0.0f) {
            speed = 0.0f;
            RE::PlayerControls::GetSingleton()->data.autoMove = false;
            if (player->AsActorState()->IsSprinting()) {
                PerformAction(player, RE::DEFAULT_OBJECT::kActionSprintStop);
            }
            PerformAction(player, RE::DEFAULT_OBJECT::kActionMoveStop);
        }
    }

    auto* controlMap = RE::ControlMap::GetSingleton();
    for (const auto flag : CONTROLS) {
        const bool playerControl = flag == UEFlag::kMovement || flag == UEFlag::kLooking;
        if ((!playerControl || Settings::lockControls.GetValue()) && controlMap->AreControlsEnabled(flag)) {
            disabledControls.set(flag);
        }
    }

    if (disabledControls) {
        controlMap->ToggleControls(disabledControls.get(), false, false);
    }

    camera->allowAutoVanityMode = false;

    // EnterThirdPerson minus its zoom reset, fov read after the tab camera restores it
    if (camera->currentState->id < RE::CameraState::kThirdPerson) {
        camera->SetState(camera->cameraStates[RE::CameraState::kThirdPerson].get());
        camera->Update();
    }

    worldFOV = camera->worldFOV;
    player->data.angle.x = 0.1f;
    player->Update3DPosition(true);

    active = true;

    if (!Settings::tutorialSeen.GetValue()) {
        SettingsUI::ShowControls();
    }

    logger::info("[MenuCamera] Started. screenX={} screenY={} distance={} pitch={} fov={}", Settings::screenX.GetValue(), Settings::screenY.GetValue(), Settings::distance.GetValue(), Settings::pitch.GetValue(), Settings::fov.GetValue());
}

void MenuCamera::Stop()
{
    if (!active) {
        return;
    }

    active = false;
    turning = false;
    dragging = false;
    pendingZoom = 0.0f;
    pendingFOV = 0.0f;
    cameraOwner.reset();

    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* camera = RE::PlayerCamera::GetSingleton();

    if (smoothCamControl && APIs::SmoothCam && APIs::SmoothCam->IsCameraEnabled()) {
        APIs::SmoothCam->ReleaseCameraControl(SKSE::GetPluginHandle());
        logger::info("[MenuCamera] SmoothCam camera control released.");
    }
    smoothCamControl = false;

    if (disabledControls) {
        RE::ControlMap::GetSingleton()->ToggleControls(disabledControls.get(), true, false);
        disabledControls.reset();
    }

    if (cachedFirstPerson) {
        camera->ForceFirstPerson();
    }

    player->data.angle.x = playerAngleX;
    player->data.angle.z = playerAngleZ;
    player->SetGraphVariableBool("IsNPC", headtrackingEnabled);
    player->SetGraphVariableBool("bHeadTrackSpine", headTrackSpineEnabled);
    player->SetGraphVariableBool("bUseEyeTracking", eyeTrackingEnabled);

    camera->allowAutoVanityMode = allowAutoVanityMode;
    camera->worldFOV = worldFOV;
    camera->Update();
    player->Update3DPosition(true);

    if (framingChanged) {
        framingChanged = false;
        Settings::Save();
    }

    logger::info("[MenuCamera] Stopped and restored camera state.");
}

void MenuCamera::Update(float interval)
{
    if (!active) {
        return;
    }

    // FUCK-RACE's cap, so one long frame can't spin, zoom or skip the idle ahead
    interval = std::min(interval, 0.1f);

    const bool cameraDown = FUCK::IsInputDown(ToFlickKey(Settings::cameraButton.GetValue()));
    if (FUCK::IsMenuOpen() || IsPromptOpen()) {
        gamepadLocked = true;
    } else if (!cameraDown) {
        gamepadLocked = false;
    }

    if (!gamepadLocked && cameraDown) {
        Turn(Axis(RIGHT_STICK_X) * GAMEPAD_TURN * interval);
        Adjust(Settings::distance, -Axis(RIGHT_STICK_Y) * GAMEPAD_ZOOM * ZoomScale() * interval, Settings::DISTANCE_RANGE);
        Adjust(Settings::screenX, Axis(LEFT_STICK_X) * GAMEPAD_PAN * interval, Settings::SCREEN_RANGE);
        Adjust(Settings::screenY, -Axis(LEFT_STICK_Y) * GAMEPAD_PAN * interval, Settings::SCREEN_RANGE);
        Adjust(Settings::fov, (Axis(RIGHT_TRIGGER) - Axis(LEFT_TRIGGER)) * GAMEPAD_FOV * interval, Settings::FOV_RANGE);
    }

    Adjust(Settings::distance, Drain(pendingZoom, interval), Settings::DISTANCE_RANGE);
    Adjust(Settings::fov, Drain(pendingFOV, interval), Settings::FOV_RANGE);

    // paused world skips the player's animation and scene update
    if (Settings::idleWhilePaused.GetValue() && RE::UI::GetSingleton()->GameIsPaused()) {
        auto* player = RE::PlayerCharacter::GetSingleton();
        player->UpdateAnimation(interval);

        if (auto* root = player->Get3D()) {
            RE::NiUpdateData data{};
            root->Update(data);
        }
    }

    if (!Settings::skyrimSouls) {
        ApplyTransform();
    }
}

bool MenuCamera::ProcessInput(const RE::InputEvent* const* events)
{
    if (!active || FUCK::IsMenuOpen() || IsPromptOpen()) {
        turning = false;
        dragging = false;
        pendingZoom = 0.0f;
        pendingFOV = 0.0f;
        return false;
    }

    const auto rotateKey = Settings::rotateKey.GetValue();
    const auto cameraButton = Settings::cameraButton.GetValue();
    const bool ctrl = FUCK::IsModifierPressed(FUCK::Modifier::kCtrl);
    bool consume = FUCK::IsInputDown(ToFlickKey(cameraButton));

    for (auto* event = *events; event; event = event->next) {
        if (const auto* move = event->AsMouseMoveEvent()) {
            if (turning) {
                Turn(move->mouseInputX * MOUSE_TURN);
            } else if (dragging && ctrl) {
                Adjust(Settings::pitch, move->mouseInputY * MOUSE_PITCH, Settings::PITCH_RANGE);
            } else if (dragging) {
                Adjust(Settings::screenX, move->mouseInputX * MOUSE_PAN, Settings::SCREEN_RANGE);
                Adjust(Settings::screenY, move->mouseInputY * MOUSE_PAN, Settings::SCREEN_RANGE);
            }

            consume |= turning || dragging;
            continue;
        }

        const auto* button = event->AsButtonEvent();
        if (!button) {
            continue;
        }

        const auto code = GetKeyCode(button);
        if (code == cameraButton) {
            consume = true;
        } else if (code == rotateKey || code == Settings::MIDDLE_MOUSE) {
            bool& held = code == rotateKey ? turning : dragging;
            if (button->IsDown()) {
                held = !IsCursorOverList() && !FUCK::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
            }

            consume |= held;

            if (button->IsUp()) {
                held = false;
            }
        } else if ((code == WHEEL_UP || code == WHEEL_DOWN) && ctrl && button->IsDown()) {
            const float notch = code == WHEEL_UP ? -1.0f : 1.0f;
            if (FUCK::IsModifierPressed(FUCK::Modifier::kShift)) {
                pendingFOV += notch * WHEEL_FOV;
            } else {
                pendingZoom += notch * WHEEL_ZOOM * ZoomScale();
            }

            consume = true;
        }
    }

    return consume;
}

std::uint32_t MenuCamera::GetKeyCode(const RE::ButtonEvent* button)
{
    switch (button->GetDevice()) {
    case RE::INPUT_DEVICE::kMouse:
        return SKSE::InputMap::kMacro_MouseButtonOffset + button->GetIDCode();

    case RE::INPUT_DEVICE::kGamepad:
        return SKSE::InputMap::GamepadMaskToKeycode(button->GetIDCode());

    default:
        return button->GetIDCode();
    }
}

std::uint32_t MenuCamera::ToFlickKey(std::uint32_t code)
{
    return code >= GAMEPAD_OFFSET ? code + GAMEPAD_OFFSET : code;
}

void MenuCamera::Turn(float delta)
{
    if (delta == 0.0f) {
        return;
    }

    auto* player = RE::PlayerCharacter::GetSingleton();
    player->data.angle.z -= delta;
    player->Update3DPosition(true);
}

void MenuCamera::Adjust(REX::INI::F32<>& setting, float delta, Settings::Range range)
{
    Set(setting, setting.GetValue() + delta, range);
}

void MenuCamera::Set(REX::INI::F32<>& setting, float value, Settings::Range range)
{
    value = std::clamp(value, range.min, range.max);
    if (value == setting.GetValue()) {
        return;
    }

    setting.SetValue(value);
    framingChanged = true;
}

// screen position to camera shift via the live frustum, any aspect ratio
void MenuCamera::ApplyTransform()
{
    if (cameraOwner) {
        return;
    }

    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* camera = RE::PlayerCamera::GetSingleton();

    // frustum is a frame behind the fov
    const float builtFOV = RE::Main::WorldRootNode()->cameraFOV;
    const float renderFOV = builtFOV - camera->worldFOV + Settings::fov.GetValue();
    const float fovScale = std::tan(RE::deg_to_rad(renderFOV) * 0.5f) / std::tan(RE::deg_to_rad(builtFOV) * 0.5f);

    camera->worldFOV = Settings::fov.GetValue();

    const auto& frustum = RE::Main::WorldRootCamera()->GetRuntimeData2().viewFrustum;
    const float pitch = RE::deg_to_rad(Settings::pitch.GetValue());
    const float distance = Settings::distance.GetValue();
    const RE::NiPoint3 back{ std::sin(playerAngleZ) * std::cos(pitch), std::cos(playerAngleZ) * std::cos(pitch), std::sin(pitch) };
    const RE::NiPoint3 forward = -back;
    const RE::NiPoint3 right = forward.UnitCross(RE::NiPoint3{ 0.0f, 0.0f, 1.0f });
    const RE::NiPoint3 up = right.Cross(forward);
    const float shiftX = (Settings::screenX.GetValue() * 2.0f - 1.0f) * distance * frustum.fRight * fovScale;
    const float shiftY = (1.0f - Settings::screenY.GetValue() * 2.0f) * distance * frustum.fTop * fovScale;

    RE::NiPoint3 anchor = player->GetPosition();
    anchor.z += player->GetHeight() * 0.5f;

    auto* root = camera->cameraRoot.get();
    root->local.translate = anchor + back * distance - right * shiftX - up * shiftY;
    root->local.rotate = RE::NiMatrix3(RE::NiPoint3(right.x, forward.x, up.x), RE::NiPoint3(right.y, forward.y, up.y), RE::NiPoint3(right.z, forward.z, up.z));

    RE::NiUpdateData data{};
    root->Update(data);
}
