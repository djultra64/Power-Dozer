# PC Settings: complete section screens with the original game's appearance

Project attribution: djultra64.

## Intent and acceptance

Replace the four-row PC settings window and repeated MORE pages. Each section
must show all its options in one screen, including all fourteen input actions.
The user accepted the modern arrangement and specifically requires the game's
actual typeface, palette, background and shadow boxes. A generic modern panel
or substitute body typeface does not meet the request.

## Screens and interaction

Four persistent section tabs: Display, Graphics, Audio, Controls.

- Display: internal resolution, aspect ratio, presentation frame-rate target,
  window/fullscreen mode.
- Graphics: anti-aliasing, screen filter, texture filter.
- Audio: master volume and mute.
- Controls: keyboard/controller selector, automatic/manual controller family,
  fourteen logical actions in two columns of seven, restore device defaults and
  entry to the original Game Options. Capture appears as a modal over the same
  table; closing it restores the same row and device.

All rows remain visible in 4:3 and 16:9 at 720p and on resize. Use a 640×480
logical safe area, proportionately scaled and centered; wider windows expand
background margins. No scrolling or MORE/PREVIOUS pagination. The existing
fourteen actions and supported setting values remain the inventory.

Mouse hover/click focus rows, arrows adjust either direction, master volume can
be dragged and stays within 0–100. Keyboard Tab/Shift+Tab and arrows navigate;
left/right adjust, Enter selects, Escape cancels/returns. Controller navigation
uses stick/D-pad, shoulder shortcuts for tabs and logical confirm/back actions.
Keep a fixed keyboard recovery path even after remapping gameplay actions.

Prompts and glyphs follow the last active device and actual binding, through
InputModel and the existing Xbox/PlayStation/Nintendo Switch atlas. Automatic
family detection and manual fallback remain. Switching family does not remap
bindings; Nintendo A/B and X/Y positions remain explicitly distinguished.
Consume settings inputs before guest input; on close, suppress held inputs
until release so that selection does not trigger gameplay underneath.

## Original visual assets

Use the actual 32×32 I4 font from validated USA Rev1 ROM assets 0xF4C–0xF7E,
not a TTF approximation. The private extractor's 51 tiles were byte-compared
against the original codec-6 CPU decoder (26,112 bytes). Load these assets at
runtime from the user's ROM; no font or background ROM pixels enter the public
source or packages. Preserve original UV orientation and glyph advance.

Use original white/red/grey palette and original shadow-box mesh, extending its
layout behind each screen's text. Headers and compact labels share the game's
font; descriptions also use that font at a readable scale. Focus is red with an
additional original-style indicator. Preserve the game backdrop behind the
shadow, avoiding an invented metal frame, rounded cards or modern body font.

The local design previews under `.local/design-pc-settings/original-style/`
are private mockups, not implemented game screenshots or publishable assets.

## Integration

Implement a PC-owned model for sections, focus, editable rows and capture
modals. Reuse SettingsStore/merge_setting, ControlsStore, InputModel and capture
logic. Existing guest hooks open/close this view and continue to manage the
original menu transition; stop copying rows into the four-entry guest window.
The original Game Options entry hands control back to the unmodified game.

Use RT64's public SetRenderHooks(init, draw, deinit), installed before setup.
The draw hook executes after VI rendering and before PRESENT. A small Plume RHI
2D renderer draws textured triangles with alpha, using the font/glyph atlas and
original shadow geometry. Compile its shader using the existing DXC toolchain.
Publish an immutable view snapshot from SDL's main thread; render-thread code
must not read mutable input/settings state or call guest functions. Upload and
release GPU resources within the render hook lifecycle; resize uses the actual
swap-chain framebuffer extent. Existing presentation ordering remains intact.

## Persistence and feedback

Keep immediate per-field saving and the existing distinction between live and
next-launch changes. Mark internal resolution, aspect, frame-rate target and
MSAA as requiring restart. Volume/mute and supported filters retain live
application. Show saving/error state separately from the title. Failed writes
restore the saved value and retain focus. Do not introduce Apply/Cancel promises
without transactional support. Restore defaults applies only to the selected
input device and requires an explicit confirmation modal.

## Verification

Check row visibility and hitboxes in both aspect ratios, navigation in both
adjustment directions, volume bounds, modal focus restoration, held-input
suppression, save failure and restart semantics. Exercise equivalent changes
with mouse, keyboard and virtual controllers, then real controllers when
available. Inspect native GPU captures of every section at 720p and after
resize. Verify glyph and action together, especially Nintendo confirm/back.
Confirm no ROM assets, captured saves or private previews enter public files.

Audio root-cause investigation is independent and continues while this design
is reviewed. This document does not assert that the audio fault is fixed.
