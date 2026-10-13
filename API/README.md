# Show Player In Inventory API

Lets other SKSE plugins check whether Show Player In Inventory is showing the player and read the current camera framing.

## Setup

1. Copy `ShowPlayerInInventoryAPI.h` into your project.
2. Include it after CommonLibSSE (it uses `REX::W32`).
3. Call `SPII::Connect()` at `kPostLoad` or later. It returns `false` if Show Player In Inventory is not installed or is older than the header.

```cpp
case SKSE::MessagingInterface::kPostLoad:
    if (SPII::Connect()) {
        logger::info("Connected to Show Player In Inventory API");
    }
    break;
```

## Functions

| Function | Returns |
| --- | --- |
| `SPII::IsEnabled()` | `true` if `bEnable` is on. |
| `SPII::IsActive()` | `true` while the camera is showing the player in a menu. `false` when the menu opened in combat or on horseback, or the mod is turned off. |
| `SPII::GetActiveMenu()` | The menu the camera is running in (`InventoryMenu`, `MagicMenu` or `BarterMenu`), or `""` when it is not active. |
| `SPII::GetFraming()` | The current `screenX`, `screenY`, `distance`, `pitch` and `fov`, including changes the player is making right now. |

Every function returns `false`, `""` or zeros if `Connect()` was not called or failed.

## Example

```cpp
if (SPII::IsActive()) {
    const auto framing = SPII::GetFraming();
    logger::info("{} at screen x {}", SPII::GetActiveMenu(), framing.screenX);
}
```

## Notes

- Call the functions from the main thread.
- The API is read-only. Nothing you call changes the camera or the player's settings.
- `SPII_API_VERSION` is the version the header was written for. New versions only add to the end of `SPII_Interface`, so an older header keeps working with a newer Show Player In Inventory.
