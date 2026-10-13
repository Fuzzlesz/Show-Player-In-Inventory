# Show Player In Inventory

**Show Player In Inventory** adjusts the camera so you can see your character in the inventory menu. It also includes real-time camera position options through **FLICK**.

## Why does this mod exist when Show Player In Menus is already a thing?

**Show Player In Menus** is a bit old and does too much while relying on Papyrus and a SkyUI MCM menu. That means you have to open the MCM menu, adjust a value, go back, and check whether it looks right or not.

I wanted something much simpler that only does the basics, which has kind of become a trend with my mods lately.

## Requirements

- Skyrim Script Extender (SKSE64)
- Address Library for SKSE Plugins
- FLICK 1.5.0 or newer
- ImGui Icons

## Recommended

- **Skyrim Souls RE - Updated** - For the unpaused menu. It looks nice and clean and is meant to be used with this.
- **SmoothCam** - Has built-in support for SmoothCam, and it just looks nice.

## Not Compatible With

- **Show Player In Menus** - It does the same thing.
- **VR** - If someone wants to port this mod to VR, be my guest. Check the source code.

## Controls

| Input | Action |
| --- | --- |
| Right-drag (not over the item list) | Turns your character. |
| Middle-drag | Moves your character on screen. |
| Ctrl + middle-drag | Changes the camera pitch. |
| Ctrl + scroll | Zooms the camera. |
| Ctrl + Shift + scroll | Changes the field of view. |
| Hold RB + right stick | Turns your character and zooms. |
| Hold RB + left stick | Moves your character on screen. |
| Hold RB + triggers | Changes the field of view. |

The rotate button and the controller button can be rebound in FLICK. Settings save as you change them, and framing changes save when the menu closes.

## Configuration

| Setting | Description |
| --- | --- |
| `bEnable` | Turns the mod on or off. |
| `bBarterMenu` | Shows your character in the barter menu. |
| `bIdleWhilePaused` | Keeps your character's idle animation playing in paused menus. Turned off and locked when Skyrim Souls RE is installed. |
| `bLockControls` | Stops your character from walking or looking around while the menu is open. |
| `bFixWeaponsOnViewSwitch` | Puts your drawn weapons and shield back in your hands when you switch between first and third person. |
| `bEnableLogging` | Writes detailed activity to ShowPlayerInInventory.log. |
| `bTutorialSeen` | Set to 0 to show the controls popup again. |
| `fScreenX` | Where your character sits across the screen, from 0 (left) to 1 (right). |
| `fScreenY` | Where your character sits on the screen, from 0 (top) to 1 (bottom). |
| `fDistance` | How far the camera is from your character. |
| `fPitch` | How far above or below your character the camera looks from, in degrees. |
| `fFOV` | Adjusts the camera field of view. |
| `iRotateKey` | Mouse or keyboard button that turns your character. |
| `iCameraButton` | Controller button that switches the sticks and triggers to the camera. |

## Source

- [GitHub Page](#)

## Credits

- **JPSteel2** for **Character Menu SE** as the base. I basically yoinked the Character Menu SE camera code, so this would not really be possible without it.
- **myztikrice** for **Show Player In Menus**, because technically JP took inspiration from Show Player In Menus, so it indirectly helped me here.
- **mwilsnd** for **SmoothCam** and its API.
- **Fuzzlesz** for **FLICK** and **FUCK-RACE**, whose per-frame menu camera and controls this camera is built on.

## Check Out My Other Mods

- [My Other Mods](#)

## Community

Join Discord to interact with the community, ask for support, and more.
