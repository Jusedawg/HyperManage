# HyperManage

**Factory Building Toolkit for Satisfactory — by Jusedawg**

One toolkit for precise placement, editing existing builds, and bulk factory operations. HyperManage aims to cover the workflows associated with Micro Manage, Infinite Nudge, Rearchitector and Area Actions through shared controls and settings.

## What you can do

- Move, rotate and scale selected objects.
- Choose movement and rotation increments for fine adjustments or larger changes.
- Snap selections to the world XY grid, snap rotation and level objects.
- Use anchors and targets to align your builds.
- Undo and redo edits and access controls from a panel that slides out from the right edge of the screen.
- Keep a clipboard of keyboard shortcuts visible while you work.

HyperManage is currently a preview for Satisfactory 1.2. Object coverage, save persistence and multiplayer support are still being tested. Hologram editing, bulk copy and fill, persistent selection groups and bulk machine settings are planned features.

## Getting started

Use a packaged HyperManage build that matches your game version; downloading this repository alone does not install the mod.

With the tool equipped, aim at an object and press **Ctrl + left click** to select it. Select additional objects the same way, or use **Ctrl + right click** to remove an object from the selection.

Press **right click** to open the tools panel. Pick your movement and rotation increments, then use the action buttons or the shortcuts below. Hover over a button to see its description.

## Default controls

| Action | Shortcut |
| --- | --- |
| Select object | Ctrl + left click |
| Deselect object | Ctrl + right click |
| Open tools panel | Right click |
| Toggle anchor | Shift + left click |
| Toggle target | Shift + right click |
| Undo | Ctrl + Z |
| Redo | Ctrl + Y |
| Move up / down | Ctrl + I / K |
| Rotate left / right | Ctrl + J / L |
| Move away / toward you | Alt + I / K |
| Move left / right | Alt + J / L |
| Pitch | Shift + I / K |
| Roll | Shift + J / L |
| Shrink / grow | Ctrl + Alt + J / L |
| Change increment preset | Ctrl + Alt + I |

## Aligning objects

Choose **Move (m)** for the distance of each movement and **Rotate (deg)** for the angle of each rotation. Type an exact value beside a preset—for example, **0.375 m** or **22.5°**—then press Enter or leave the field to save. Movement and rotation apply to the active increment profile; the grid setting is shared. The panel shows which profile you are editing.

Set **Grid XY (m)**, then use **Snap XY** to align the selection to that world grid. XY snapping adjusts horizontal placement; it does not snap elevation. Use **Snap rotation** to align rotation to the selected angle increment, or **Level** to remove pitch and roll.

Start with larger increments for positioning, then switch to smaller increments for the finishing adjustments.

## Undo and redo

Use **Undo** and **Redo** in the tools panel, or **Ctrl + Z** and **Ctrl + Y**. The buttons show the number of recorded edits. History covers transforms, matching paint and box selection, with up to 1,000 steps for the current session. A new recorded edit clears redo; **Clear history** empties both lists. Connections and individual selection clicks are not recorded.

History pauses while a lightweight building edit is awaiting acknowledgement. History is not saved with your game.

## Feedback

Report problems and suggest features through [GitHub issues](https://github.com/Jusedawg/HyperManage/issues). Include your game and HyperManage versions, what you selected, and the steps that reproduce the problem. Screenshots help with selection and interface issues.
