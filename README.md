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

Press **right click** to open the tools panel. Pick your movement and rotation increments, then use the action buttons or the shortcuts below. Snap, level, anchor-match and undo/redo controls use compact icons; hover for their names and detailed tooltips. Anchor-match icons include X/Y/Z, and undo/redo tooltips include history counts. Other action tiles retain short captions. Field adjustment tiles have a **-** side on the left and a **+** side on the right; click the side for the direction you want. Drag the right scroll rail to reach more controls. Scrolling over a field-adjustment icon changes the selected objects instead of scrolling the panel.

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
| Cycle notes display | Ctrl + Alt + K |

## Aligning objects

Choose **Move (m)** for the distance of each movement and **Rotate (deg)** for the angle of each rotation. Type an exact value beside a preset—for example, **0.375 m** or **22.5°**—then press Enter or leave the field to save. Movement and rotation apply to the active increment profile; the XY and Z grid settings are shared. The panel shows which profile you are editing.

Set **Grid XY (m)**, then use **Snap XY** to align the selection to that world grid. XY snapping adjusts horizontal placement. Set **Grid Z (m)** and use **Snap Z** to snap origin heights independently of the horizontal grid; the default height grid is 1 m. Both grids are measured from world zero. In group mode, snapping moves the group using the anchor (or first selected origin) and preserves its offsets. In individual mode, each origin snaps separately. Use **Snap angle** to align rotation to the selected angle increment, or **Level** to remove pitch and roll.

Start with larger increments for positioning, then switch to smaller increments for the finishing adjustments.

The clipboard shows selection controls, Undo/Redo, movement diagrams, shrink/grow shortcuts and the current increment profile. Selection, history, scale and notes shortcuts follow your configured bindings; the I/J/K/L movement diagram is marked as the default layout.

## Match an anchor coordinate

Select your reference object and press **Shift + left click** to make it the anchor. Select the other objects, then use **Match X**, **Match Y**, or **Match Z** near the grid controls. Each object moves only along that world axis to match the anchor's origin. **Match Z** is useful for equal origin heights.

The anchor and target stay stationary; other coordinates, rotation and scale are preserved. Each Apply is undoable. These commands align object origins, not mesh edges or visible surfaces, and move objects independently regardless of group mode.

The exact-edit sections start collapsed to keep the panel compact. Click a section heading to reveal its fields and action buttons. Collapsing a section keeps the entered values while the panel remains open.

## Exact world offsets

Under **Offset (m)** with **World axes** selected, enter X, Y and Z distances, then click **Apply**. For example, X **0.375**, Y **-1.25**, Z **2** moves the selection diagonally and raises it two meters. Zero leaves an axis unchanged. Each axis accepts up to 1,000 meters in either direction.

Offsets always use world axes and move the selected objects together, preserving spacing, rotation and scale. The target stays in place. This requires a selection; it does not move an object merely because you are looking at it. Values stay in the fields while the panel is open so you can repeat the move. **Zero fields** clears the values without changing any objects. Undo and redo treat each Apply as one edit.

## Offsets along a rotated object

In **Offset (m)** choose **Object axes** to move along the selected anchor's local X/Y/Z directions. With one selected object, its own orientation supplies the axes. Multiple objects require a selected anchor. All selected objects receive the same movement, preserving spacing, rotation and scale; the target stays in place. This ignores Together/Individual mode and does not change keyboard movement settings.

For example, when a reference has yaw 90 degrees, X **1** moves one meter along its rotated forward direction. Reference scale does not multiply that distance. On a tilted reference, local Z is not world height; choose World axes for vertical moves. Both input components and the resulting world-axis movement are limited to 1,000 m per axis. Apply records one **Object-axis offset** history step. The picker starts on World axes whenever the panel opens.

## Exact world position

Expand **World position (m)** and use **Read position** to fill X/Y/Z from the current selection. Edit the destination coordinates, then click the Apply icon. Unlike offsets, **zero means world zero**. For example, read the position and change only Z to **150** to place the reference at 150 meters high.

The checkboxes beside X/Y/Z choose which coordinates to apply. Uncheck X and Y to set only height; uncheck Z to keep the current height while moving horizontally. Unchecked fields are dimmed and ignored. All three start checked when the panel opens. Read position fills the fields without changing your axis choices. These switches affect only World position, not other movement controls.

The reference is the selected anchor, or the center of the selected object origins if no anchor is selected. For one object, it is that object's origin. The entire selection moves together, preserving spacing, rotation and scale, regardless of Together/Individual mode. The target stays in place. Coordinates accept -10,000 to 10,000 meters, with a maximum move of 1,000 meters per axis per Apply. Typing and Read position do not change objects. Each Apply is one undoable **World position** edit; applying the same destination again does nothing.

## Exact rotation offsets

Under **World rotation offset (deg)**, enter yaw, pitch and roll changes, then click **Apply**. Each field accepts -180 to 180 degrees. For example, yaw **90** turns the selection a quarter turn around world Z. These are changes to the current rotation, not absolute orientation values. Combined values form one world-space rotation (roll, then pitch, then yaw).

In **Together** mode, objects rotate around the selected anchor, or the center of the selected object origins if no anchor is selected. In **Individual** mode, each object rotates in place. The target stays stationary and scale is preserved. The panel describes the active pivot behavior. **Zero fields** clears only the inputs; each Apply is one undoable edit.

## Exact world orientation

Expand **World orientation (deg)** and click **Read orientation** to fill yaw (Y), pitch (P) and roll (R). Enter the desired angles from -180 to 180 degrees, then Apply. These are absolute angles: zero means a zero angle, unlike an offset. Applying the same orientation again does nothing.

Checkboxes beside Y/P/R choose which reference angles to set. For heading only, leave Y checked and uncheck P and R. Unchecked fields are dimmed and retain the reference's current Euler angles; they do not lock each member of a group independently. All angles start checked when the panel opens, and Read orientation preserves your choices. These controls affect only World orientation. Near vertical pitch, yaw and roll are coupled; retained values use the same canonical Euler representation shown by Read orientation.

One selected object rotates in place. For multiple objects, select an anchor with **Shift + left click** first. The anchor reaches the requested orientation while the group rotates around its origin, preserving spacing, relative orientations and scale. This operates on the group regardless of Together/Individual mode; the target remains excluded. Each Apply creates one **World orientation** history step. Read orientation changes only the fields. Equivalent Euler angles can look different after reading them back, especially near vertical pitch, while describing the same orientation.

## Exact scale

Under **Exact local scale (%)**, enter X/Y/Z percentages and click **Apply**. **100%** is original size, **50%** is half size, and **200%** is double size. Values are absolute rather than repeated multipliers, so applying 125% twice still leaves an object at 125%. Each axis accepts 1-1,000%.

**Read scale** fills the fields from your selected anchor, or the only selected object. Select an anchor first when working with a group. Reading does not resize anything or add history. It is unavailable while edits are pending or the reference scale is outside 1-1,000%. Applying the resulting values sets every selected object to those percentages; it does not preserve differences in scale between group members.

A **Uniform preset** fills all three fields. **100%** prepares a return to original size; click Apply to make the change. Editing fields or choosing a preset alone does not alter objects. Scaling uses each object's local axes and preserves its origin and rotation regardless of group mode. The target is excluded, and the edit supports undo/redo.

## Measure between references

Set an anchor and target, then expand **Reference measurements**. The panel shows signed world X/Y/Z offsets from the anchor to the target, horizontal **XY** distance, and straight-line **Total** distance, in meters. Positive Z means the target is higher. Measurements use object origins rather than visible surfaces, so they are not necessarily the gap between buildings.

The readout updates as the references move, pauses while an edit is pending, and never changes objects or history. It starts collapsed.

## Even spacing

Expand **Even origin spacing** and choose X, Y or Z to distribute 3-1,024 selected objects along that world axis. The two outermost origins stay fixed; the others move to equal intervals between them. Other coordinates, rotation and scale stay unchanged. The target is excluded. The anchor participates and can move if it is between the endpoints; Together/Individual mode does not affect this command.

Spacing uses object origins, not visible edges, so objects of different sizes may still have unequal gaps. Already-even or coincident origins do nothing. A move larger than 1,000 meters rejects the operation. Each operation is one undoable edit, including supported lightweight objects. The section starts collapsed.

## Filter a mixed selection by type

Set an anchor to choose a reference type, then expand **Selection type filter**. **Keep type** deselects other types; **Remove type** deselects matching types. Both retain the current anchor and target, even when those references would otherwise be filtered out. Nothing is dismantled, and each change supports Undo/Redo.

Matching uses the exact object or building class, including the real building type of lightweight foundations and walls. Different building variants may be distinct types; this is not a broad category or paint filter. The commands operate only on your current selection, pause while edits are pending, and do nothing without an anchor. Repeating a filter with no changes adds no history. The section starts collapsed.

## Add or remove an area from a selection

**Edges** and **Centers** add objects inside the anchor/target region to the current selection. The anchor stays assigned. The target marker clears, leaving that object selected and included in subsequent transforms. Undo restores the previous selection and both markers.

Set an anchor and target at the region boundaries, then use **Remove edges** or **Remove centers** in the Selection controls. These subtract objects from the current selection without moving or dismantling anything. The anchor and target stay selected so you can reuse the same region.

**Remove centers** uses the region between the reference centers; **Remove edges** extends it using their bounds. As with the additive Edges/Centers controls, reference orientations and selection tolerance define the region, and candidate bounds centers determine inclusion. The operation works on selected normal and lightweight objects, and one Undo restores the removed selection.

## Remember selection groups

Choose **Slot 1-10** above the selection controls, then click **Remember** to store the current selection, anchor and target. Remember replaces only the chosen slot. Switch slots and click **Recall** to return to a group; switching the dropdown alone leaves the current selection unchanged.

Give each slot an optional name in the field beneath its picker, such as **Roof supports**. Enter or leave the field to save; names appear beside slot numbers. Names accept up to 24 characters and remain when Remember replaces a group. In single-player they are stored with the game save. Clear a name to return to its numbered label. Naming a slot does not store a selection or add undo history.

**Add slot** merges the chosen slot into your current selection. It preserves your current anchor and target, skips the saved target and unavailable objects, and avoids duplicates. The saved anchor joins as an ordinary selected object unless it is already your current reference. Undo removes only the newly added objects; adding the same slot again creates no extra history. Use Recall when you want to replace the selection and restore the saved references instead.

**Remove slot** deselects the remembered objects from your current selection. It protects your current anchor and target, excludes the saved target, and leaves the remembered slot intact. Nothing is dismantled. Undo restores the removed selection; repeated clicks with nothing left to remove add no history.

The slot status shows the number of available editable objects. Unused slots cannot be recalled, and removed objects are skipped. Recall is one undoable selection change. Remembering an empty selection intentionally stores an empty group. Slots follow the objects as you edit them. Single-player slots containing ordinary saved actors can survive reloads; lightweight and multiplayer slots remain session-only. See Selection slots across reloads below.

## Undo and redo

Use **Undo** and **Redo** in the tools panel, or **Ctrl + Z** and **Ctrl + Y**. The buttons show the number of recorded edits. History covers transforms, matching paint, selection clicks, anchor/target changes, box selection, Clear selection and Recall, with up to 1,000 steps for the current session. A new recorded edit clears redo; **Clear history** empties both lists. Connections are not recorded. Repeating a selection click that changes nothing does not add a history step. Clearing or recalling a selection is one step, including its anchor and target.

Expand **Recent history** below Undo/Redo to see the five newest undo records and the next five redo records. Hover Undo or Redo to see the next operation, such as World offset, Snap Z or Exact scale. The list is a read-only preview; the buttons replay one step at a time. Unavailable objects are skipped during replay.

History pauses while a lightweight building edit is awaiting acknowledgement. History is not saved with your game.

## Feedback

Report problems and suggest features through [GitHub issues](https://github.com/Jusedawg/HyperManage/issues). Include your game and HyperManage versions, what you selected, and the steps that reproduce the problem. Screenshots help with selection and interface issues.

### Automatic anchor

With **Auto anchor** enabled in the selection controls, the first Ctrl-clicked object becomes the anchor when the editable selection is empty. Later clicks preserve that anchor. Selection and automatic anchoring share one undo/redo step. Disable the checkbox to choose anchors manually; the preference is saved, and changing it leaves the current selection untouched. A target by itself does not count as an editable selection.

### Transform the object you are pointing at

With no editable objects selected, using the movement, rotation, scaling or world-snap controls selects the supported object under your crosshair first. Its highlight and selected count stay visible, and later adjustments operate on that selection. Auto anchor applies if enabled. The target is protected and cannot be transformed this way. No object under the crosshair means no edit.

Undo reverses the transform while keeping the object selected; a second Undo removes the automatic selection. Redo restores selection before the transform. Exact numeric fields continue to require an existing selection.

### Beam scaling

Beams store length separately from object scale. Incremental scaling, exact scale and matching scale are blocked when the editable selection includes a beam. A message explains how to proceed: deselect the beams before resizing other objects. Mixed selections are not partially resized. Movement, rotation and history remain available. A beam used only as the target does not block resizing other selected objects.

### Select a placed blueprint

Point at a member of a placed blueprint and press **Ctrl + Shift + left-click** to add its registered members to the selection. Membership comes from the placed blueprint instance, so an adjacent copy is not included. Normal buildings and lightweight pieces are supported. Existing anchor and target markers stay assigned; Auto anchor can assign the pointed member when starting with an empty editable selection. The target remains excluded from transforms.

The selection change is one undo step. Repeating it adds no history when nothing changes. This does not create a persistent group, export a blueprint or save selection slots. Streaming and multiplayer membership coverage still require gameplay validation.

### Selection slots across reloads

In single-player, slots containing only ordinary saved actors now store their members, anchor, target and name inside the game save. The slot status shows **game-save** when eligible. Save the game after Remember or renaming; changes made after the last game save are not retained. Deleted or unavailable members are skipped on recall.

Slots containing lightweight pieces (such as many foundations and walls) remain **this session**, as do multiplayer slots. A mixed slot is never partially persisted. Replacing a previously persistent slot with a session-only selection clears its stored membership on the next game save; its name is retained. This first persistence pass still needs live save/reload validation.

Stored slots are cleaned before saving and after loading: deleted members are removed, duplicate references are collapsed, and anchor/target markers are cleared if their objects are no longer members. References from another world are rejected. An intentionally empty slot stays occupied.

Use **Forget slot** beside the slot-name field to clear that remembered group while keeping its name. It leaves buildings, current selection, anchor/target and undo/redo history untouched. Forgetting is not undoable; Remember can store the current selection again. Save the game to retain removal of a persistent slot. **Remove slot** instead deselects the remembered objects and keeps the slot.

### Remember a whole blueprint

Set an anchor on a member of a placed blueprint, choose a slot, then click **Blueprint slot** beside the name. This stores that placed instance, rather than the current selection snapshot. In single-player its identity is stored with your game save, including when its members are lightweight buildings. Save the game after remembering it. Multiplayer blueprint slots remain session-only.

Recall selects the blueprint's registered members and clears anchor/target markers; Add slot and Remove slot operate on those members while preserving current markers. Membership is resolved afresh for each command. Adjacent copies are separate instances. Missing or unavailable blueprint membership leaves the current selection unchanged. The slot does not recreate dismantled pieces. Regular Remember replaces blueprint mode with a selection snapshot; Forget slot clears either mode. Live blueprint save/reload and lightweight membership validation are still pending.

Blueprint selection and blueprint-slot operations check every registered member before changing selection. If any normal or lightweight member is unavailable, the operation leaves selection and history unchanged and explains why. Move closer, wait for loading, and retry. Blueprint slots show **Blueprint unavailable** after a failed attempt; successful retry clears that status. This avoids deliberately selecting only the available portion of a registered group; multiplayer registration timing still needs gameplay validation.
