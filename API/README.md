# Show Player In Inventory API

Lets other SKSE plugins read and drive the Show Player In Inventory menu camera.

## Setup

1. Copy `ShowPlayerInInventoryAPI.h` into your plugin. It needs CommonLibSSE-NG for `REX::W32`.
2. Call `SPII::Connect()` from `kPostLoad` or later. It returns `false` when Show Player In Inventory is not installed or is too old.
3. Use the functions from the main thread.

Every function is safe to call when `Connect()` failed. It returns `false`, an empty string or zeroed framing.

## Functions

| Function | Description |
| --- | --- |
| `bool IsEnabled()` | The mod is turned on in its settings. |
| `bool IsActive()` | The camera is framing the player right now. |
| `const char* GetActiveMenu()` | The menu the camera is framing in, such as `InventoryMenu`. Empty when not active. |
| `SPII_Framing GetFraming()` | The current screen position, distance, pitch and field of view. |
| `bool IsControlling()` | The player is turning or moving the character with the mouse, or holding the controller camera button. Skip your own camera controls while this is `true`. |
| `bool Turn(float delta)` | Turns the player by `delta` radians, the same direction as the mouse turn. The camera stays where it is. Returns `false` when not active. |
| `bool SetFraming(const SPII_Framing& framing)` | Sets the framing. Values are clamped to the settings ranges and saved when the menu closes. Returns `false` when not active. |

## Framing ranges

| Field | Range |
| --- | --- |
| `screenX` | 0 (left) to 1 (right) |
| `screenY` | 0 (top) to 1 (bottom) |
| `distance` | 50 to 600 |
| `pitch` | -60 to 60 degrees |
| `fov` | 20 to 120 degrees |

## Example

Turning and zooming with the right stick without fighting the player's own controls:

```cpp
if (SPII::IsActive() && !SPII::IsControlling()) {
    SPII::Turn(stickX * 0.05f);

    auto framing = SPII::GetFraming();
    framing.distance -= stickY * 5.0f;
    SPII::SetFraming(framing);
}
```
