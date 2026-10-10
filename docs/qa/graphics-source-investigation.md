# Graphics source investigation — 2026-10-10

Current disposition: the user explicitly accepted the present graphics and
closed this investigation on 2026-10-10. The evidence below is retained for
reference; these appearance differences no longer block the requested beta.
Do not resume graphical experiments as part of the remaining launch work.

Independent source review started at `6bf0e2d` and continued through the current
lifecycle checkpoint. This reviewer made no production changes, rebuilds,
native execution, GPU work or dependency modifications. The local
original-game source and generated files were read, along with the pinned RT64
source under Tetrisphere's `options-beta/.local/deps/rt64`.

## Evidence and limits

The production `gameplay-c3qp5vrt/route-building-3-compositor.png` and private
shadow-omission `gameplay-k1c8jl0c/route-building-3-compositor.png` show wide dark
arcs and large translucent polygons. The independent Angrylion/CXD4 reference
`reference-emulator/profile/screenshots/original-major-2.png` does not show the
same fault, although its timing and camera are different.

Root subsequently supplied `gameplay-qwuqrzgz/route-building-3-compositor.png`
from a private variant replacing only the track routine's two triangle command
constants with SPNOOP. Large arcs are absent there, but large polygons containing
displaced scene content, including a `$230` popup, remain. This connects the arcs
to track drawing and motivates checking a separate source for the displaced
scene polygons. It does not establish abnormal track widths or identify the
faulty polygon draw call.

## Track command decoding is consistent with the SDK

Original source `37530.c:341–350` sets one-cycle translucent rendering, clears
geometry state, enables shade/smooth shade, disables textures, and combines
primitive RGB with shade alpha. Primitive RGB is black. Generated
`func_8027C4C8` contains the corresponding `BB000000/FFFFFFFF`,
`FCFFFFFF/FFFDF8FC` and black primitive-color commands. Scenery visible through
these strips is not by itself evidence that the strips sample a texture.

SDK `include/2.0D/PR/gbi.h:1737–1745` packs original F3D vertex count as `n-1`
in bits 20–23. RT64 `gbi/rt64_gbi_f3d.cpp:72–74` decodes those bits plus one.
SDK `gbi.h:1900–1908` multiplies triangle indices by ten; RT64 F3D at `93–95`
divides by ten. Texture on/off is the low byte in both SDK `gbi.h:2557–2571`
and RT64 F3D `143–149`. No mismatch was found in these boundaries.

## The geometry boundary can distinguish CPU coordinates from HLE transforms

The original track ring contains 80 entries of 0x1C bytes at `8036D3D0`.
`37530.c:253,271–282` rotates the half-width and writes each wheel's two edges
as center plus/minus truncated rotated width. Generated callers use widths
3, 4 or 5. For example, `funcs_16.c:196–203` passes width 4 at caller stack
offset 0x24 before calling `8027BE7C`. The single-track caller `802CB224` also
passes width 5.

A useful snapshot records tail/head at `8036DC90/91`, all ring entries, and
the generated track vertex array before RT64 reads it. Pair left edges at
entry offsets 0/+6 and right edges at +0xC/+0x12; each XYZ point is three signed
16-bit values. Heading zero with width 4 gives same-wheel x difference 8,
equal y and equal z. More general headings should retain a small width after
integer truncation. Root reported original-emulator live widths around 8–8.94.
The later native snapshot and effective-clip results below now provide the
native comparison.

If native ring widths are already large, inspect the CPU producer and its
arguments. If ring widths are small but generated vertex widths differ,
inspect `8027C4C8`. If both are small, inspect the transform active at the first
track vertex and its transformed positions. `hd.c:2578–2580` loads the main
projection/modelview, but `2633–2636` draws a static sublist before tracks;
the track routine itself does not load a modelview matrix. Log matrix address,
matrix values and stack depth at that boundary rather than assuming the
inherited transform is correct.

## Confirmed culling omission, unproven causal relevance

RT64 F3D `rt64_gbi_f3d.cpp:106–108` and F3DEX `rt64_gbi_f3dex.cpp:41–43`
leave `cullDl` as TODO. The track routine emits an eight-corner bounding box
and `gSPCullDisplayList(0,7)` for runs containing at least 41 vertices
(`37530.c:526–530`). This omission can execute a sublist the original RSP
would reject. It does not inherently multiply widths of visible geometry.

A future CPU-only semantic fixture can load eight corners sharing one outside
clip plane, execute cull, then a sentinel triangle within a nested display
list. The expected result is return from that sublist without the sentinel;
mixed-plane and intersecting boxes should continue. An actual affected run's
corners and transform are needed to connect this fixture to the screenshot.

## Framebuffer candidate for the displaced scene polygons

RT64 `hle/rt64_rdp.cpp:133–183` chooses a framebuffer for a texture-load range,
tags TMEM and schedules a GPU tile copy. The picker
`rt64_framebuffer_manager.cpp:64–87` accepts any overlapping range despite its
`findMostRecentContaining` name and prioritizes newer write timestamps. Its
equal-timestamp full-containment branch at 75 is unreachable after the `>=`
comparison at 71. These are source facts, not proof of this fault.

`FramebufferManager::get`, lines 33–50, derives the tracked range from width,
pixel size and maximum observed height. `makeFramebufferTile`, lines 396–470,
can clip a partially overlapping load to the framebuffer range. Original
`hd.c:4363–4365` assigns main buffers at 0x400 and 0x25C00, each 320x240 RGBA16;
their contiguous end is 0x4B400, the level heap start. Depth is at 0x21ED00.
Object shadow textures start around 0x3580C0 and are generated as separate
64x64 CI8 targets (`13A70.c:169–175`), then sampled as IA8
(`hd.c:2894`). These known addresses do not establish a main/shadow overlap.

For the offending polygon, capture its command address and triangle vertices,
texture-load range, matched framebuffer address bounds/dimensions/timestamp,
TMEM range and tile-copy ID, then the draw's combiner and texture usage. A valid
asset load redirected to an unrelated old main framebuffer would confirm
the alias boundary. A texture load explicitly addressing the main buffer would
instead move the investigation back to guest command generation/address
translation. If no framebuffer tile is used, inspect blending or another
source of the scene content.

Simple stale TMEM invalidation is weakened by the source: `rt64_rdp.cpp:488–498`
and `526–534` discard overlapping framebuffer TMEM regions before recording
each new tile/block load. Ordinary motion blur is also weakened: the original
draw code checks mode nonzero at `34430.c:461`, and the generated branch at
`funcs_5.c:10042–10050` skips drawing when the sampled mode is zero.

### Subsequent source-bounded evidence

Root's original-refresh run `gameplay-ifza76_e` reached three route targets.
Its third-building image still contains the dark arcs. That excludes requiring
interpolation for these arcs. I independently decoded its unchanged 8 MiB
`native-third-building.ram`: 77 live ring entries, widths 8–8.944 for the
nonzero entries, and matching widths in referenced track VTX commands. Both
frame contexts contain 208 track TRI1 commands with valid loaded vertex indices;
the largest triangle edge is 30.887 world units. No enormous strip bridge or
bad F3D vertex-index decoding appeared in this snapshot.

The stored main modelview is identity. The perspective matrix is consistent
with 45-degree FOV, near 10 and far 10000, and the camera matrix is plausible.
Multiplying the stored camera matrix by perspective projects pairs 65–76 to
6.39–11.65 pixels wide at 320x240, with positive clip w of approximately
284–385. At 1920 pixels wide, this is approximately 38–70 pixels. Thus the
visible band thickness alone does not prove excessive width. Most ring alphas
are 50, which is approximately 19.6% opacity. An unmatched reference screenshot
cannot establish that these source-generated widths or opacities are wrong.
The matrix actually inherited by RSP at each VTX load remains the useful check.

Private `graphics-source-tools/track_state.py` decodes snapshots or an existing
read-only StateReader using only `pread`. Its word-swap/width fixture passed.
Private `track-tracer.cpp` wraps `RSP::setVertex`, calling the real function once
and then logging copied coordinates, alpha, effective matrices, stack depths,
addresses and clip XYZW. Root subsequently compiled this private wrapper; this
reviewer did not build or execute it. Its stderr logging changes timing and
must be considered when investigating races.

The finished framebuffer tracer run `gameplay-v73v379s` reported only eight
shadow matches and no main framebuffer matches after VI 2500. A later run was
still ongoing. The observed shadow tuple is a legitimate 64-wide CI8 image
at 0x3580C0; tracked height 63 gives a 4032-byte range while the texture load
requests 4096 bytes. This partial final-row overlap explains why the overlap
picker is exercised, but does not connect it to old scene content.

A concrete main-framebuffer range-overrun scenario would require the second
320-wide RGBA16 buffer at 0x25C00 to be tracked at height at least 241. Its
end would become 0x4B680, covering the first 640 bytes of the heap at 0x4B400.
Alternatively, height 240 with width 321 would end at 0x4B5E0. These are
conditional arithmetic examples, not observed dimensions. Source fixes main
width to 320, fills through row 239, clips triangle bounds against viewport and
scissor (`rt64_rsp.cpp:1151–1155`), and resets max-height tracking each workload
(`rt64_state.cpp:1831`) and render batch (`rt64_workload_queue.cpp:366`). There
is currently no evidence for such an overrun in Blast Corps.

No culling, texture, framebuffer-selection, interpolation or geometry
workaround is justified by this review alone. Capture the effective transforms
and offending polygon draw state before choosing a fix.

### Effective clips and MEMORY blending

Root executed the combined private tracer as `gameplay-zrmi0y9n`. I parsed
its 1024 vertex-load events (VI 4501–4775). Entire loads sharing an outside
homogeneous clip plane include 816 to the right, 644 below, 138 before near
and 33 above; these counts overlap. Only 163 loads lack a common rejecting
plane. There are 828 pairs containing a nonpositive w. When both pair
vertices lie inside the homogeneous frustum, 886 pairs remain, with median
width 5.646 and maximum 7.649 pixels at 320x240. The unrestricted projected
maximum of thousands of pixels is consequently not evidence of visible
inflated widths. Logging exhausted its cap before the third-building view.

`RSPProcessCS.hlsl:123–132` converts clip positions to screen coordinates
but retains w (with a special case only for exactly zero). `RasterVS.hlsl:18–21`
reconstructs homogeneous XY and multiplies XYZ by this w. Thus the GPU can
clip offscreen geometry even if `cullDl` is omitted. Near clipping is governed
by `NoN`, independently from depth testing: `rt64_state.cpp:1017–1025` uses
the draw's NoN for projections using viewports and forces NoN for those without
one; `rt64_raster_shader.cpp:315` sets depthClipEnabled to !NoN;
`RasterPS.hlsl:78–88` also enforces depth bounds. Capture effective viewport,
scissor and NoN to distinguish a wrong projection classification from merely
large projected positions outside the frustum.

The SDK's `RM_AA_XLU_SURF`, `gbi.h:773–776`, blends input color times input
alpha with MEMORY times one-minus-alpha. In the raster path, MEMORY is the
current framebuffer attachment at the same pixel, through fixed-function
dual-source blending. `rt64_blender.h:416–423` generates the combiner RGB and
factor; `RasterPS.hlsl:226–239` emits that factor in its secondary output;
`rt64_raster_shader.cpp:334–338` configures SRC1_ALPHA and INV_SRC1_ALPHA.
There is no MEMORY texture, UV offset or guest framebuffer-texture load here.
`gBackgroundColor` is declared in `FbRendererRT.hlsli` for the raytracing path,
not this raster shader. Old scene content from MEMORY would require stale
attachment contents, attachment selection or incomplete clearing. It is not
established merely by the ghost popup. A GPU capture of the polygon draw's
bound attachment, blend state and preceding clears is the narrow discriminant.

### Main-image coverage at wide aspect

The ifza76_e snapshot identifies segment 1 at physical 0x3FF600. Its main
initialization sublist at offset 0x38 restores scissor with
`ED000000/005003C0`, meaning 0..320 by 0..240, before the main image draws.
`hd.c:2574` fills the ground from its horizon through row 239, across the
full original width. `2D810.c:146–157` sets the horizon to zero if sky drawing
is disabled, so that case fills the entire image. Otherwise its sky uses
opaque rendering above the horizon (`2D810.c:164–171`).

RT64 explicitly expands fill rectangles spanning the pair's whole scissor:
`rt64_framebuffer_renderer.cpp:1538–1550` sets inverse aspect scale to one;
`convertFixedRect`, lines 75–78, then multiplies the horizontal bounds by the
wide resolution scale. Submission at lines 600–609 clears either the whole
target or the converted subrectangle. Thus simple missing 16:9 border coverage
is not established by source; the code includes a path intended to cover it.
The next discriminant is the actual attachment dimensions and converted
clear/scissor rectangles in the faulty frame, plus an otherwise equivalent
original-aspect run. Source-level intentions do not prove GPU execution.

### Private diagnostic prepared for the final draw boundary

`graphics-source-tools/draw-tracer.cpp` wraps the externally referenced
three-index `RSP::drawIndexedTri` overload and `FramebufferRenderer::addFramebuffer`.
The exact mangled names were checked against undefined references and definitions
in the pinned RT64 archive. This avoids relying on wrapping `State::flush`, whose
calls inside its own translation unit may bypass GNU linker wrapping.

The RSP event records the first triangle of each pending call by default,
with its framebuffer pair, combiner, blender flags, texture/copy IDs, effective
viewport, scissor, NoN and CPU vertices/clips. The renderer event records final
shader flags, attachment identities/dimensions, converted clears/scissors and
bounded indexed faces or raw vertices. Join on workload submission, pair index
and pending call index. `State::flush`, line 474, assigns this index from the
then-current workload gameCallCount; the renderer copies it to instanceIndex.
Live VI/gfx counters describe observation time and are not the join keys.

Both wrappers invoke the original exactly once and inspect host data; they do
not write guest memory or submit extra GPU operations. Logging exceptions are
contained after the original returns. The source passed CPU syntax checks. No
object compilation, linking or GPU execution was performed by this reviewer.
The prepared commands JSON is a recommendation based on the earlier private
link recipe; use the current build's object list if that recipe has changed.

Sampling uses immutable workload submission on both host threads, controlled
by `BC_QA_DRAW_SUBMISSION_STRIDE` (or the earlier GFX_STRIDE name as fallback),
with default stride 30. MIN_VI/MAX_VI gate the observation window. DRAW_LIMIT,
DRAW_RENDER_LIMIT and DRAW_FACE_LIMIT bound output, while ALL_TRIANGLES enables
every triangle instead of the first of a pending call. CPU clip data is not a
GPU readback; converted renderer state is not proof of fragment execution.

### Final captured draw state: z934h0pt

Root compiled the final private wrapper against current lifecycle-build objects
and executed `gameplay-z934h0pt`; this reviewer only read its `native.log`,
screenshots and source. The run ended normally at VI 10801, with 10439 graphics
tasks and 5397 audio tasks. Its scripted route reached no objectives, so it
provides neither a third-building comparison nor a successful gameplay verdict.

The log contains 8192 pending draws and 128 render-attachment events, with the
attachment observations at VI 4023–5301. Both caps were exhausted. Sampling
stride 30 selected 21 main workloads; each has a native 320x240 pass and an
upscaled 1920x1080 pass. All 42 selected main attachments use color address
0x25C00 and depth address 0x21ED00. Selecting one main-buffer parity with an even
stride does not show that the other buffer is unused.

All 42 main passes begin with a black RGBA(0,0,0,1) clear covering the full
selected attachment: [0,0,320,240] or [0,0,1920,1080]. The native and upscaled
color targets have distinct identities. Captured scissors stay within their
attachments. This weakens incomplete wide-image clearing and retained main
MEMORY content for these samples. It remains CPU command/attachment evidence,
not verification that the clear or subsequent GPU draws executed correctly.

Joining `(submission, pair_index, pending_call_index)` to the renderer's
`call_index` yields 16344 matches, counting the two render passes separately.
No joined source other-mode or combiner mismatch was found. All 17728 main
raster calls carrying the shader NoN flag have it set, and all 2062 main calls
classified as alpha blending use the simple blender path. NoN is not a defect
by itself and does not disable horizontal/vertical clipping. Its exact game
microcode semantics still need a discriminating fixture before any change.

The 210 main tile-copy references consist of five 64x63 shadow copies per pass,
IDs 3 through 7, sampled as 64x64 IA8. No sampled main-framebuffer copy was
observed. Pending source copy IDs 8 through 12 become renderer IDs 3 through 7
because `rt64_state.cpp:736–753` hashes and schedules reinterpretation, then
replaces each call tile's ID with the destination ID. The change is expected;
it is not a pending/render identity mismatch.

The bulldozer shadow uses texture-image address 0x359100. In submission 2400,
pair 2, call 153, its two complete faces cover approximately x102–225,
y88–201 at 320x240, with positive clip w and UVs 0–62.999. Vertex RGB is 10
and alpha is 140, matching the original shadow quad generation at
`hd.c:2898–2918`. Its 19 pending samples remain within the XY frustum.

The train shadow at texture-image address 0x35D200 is also present: it was not
removed by the earlier private variant skipping `8024E4F4` and `80258B78`.
Original `hd.c:2930–3022` draws it separately in `8024F520`. In submission
2400 it is call 244, with both complete faces at approximately y=-270..-274
and positive w 1376..1440. All 42 main render samples place all four train
vertices above the common homogeneous clip plane y>w. Thus those samples do
not support the train's shadow producing the visible central polygons. They
do not establish its position in earlier, uncaptured mission-start frames.

The 43 selected CI8 shadow attachments use address 0x359100: 22 native targets
of 64x63 and 21 upscaled targets of 384x284. Native clears/scissors are
[0,0,63,63]; upscaled clears/scissors are [0,0,378,284]. These follow the guest's
63-exclusive scissor and leave a narrow right border, while the requested
64x64 load also extends one row beyond the tracked framebuffer height.
This partial boundary warrants inspecting sampled border texels if visible
edges correlate with it; it does not explain an entire copied old scene.
The shadow geometry calls are untextured, use white primitive color and do
not blend. No scene-texture draw into the selected shadow target was logged.

CI8-to-IA8 is supported by `FbReinterpretCS.hlsl:81–89`: `ANY8toUINT8`, at
42–49, selects the red or green channel, and `ANY8toIA8`, at 56–59, decodes
the high nibble as intensity and low nibble as alpha. `IA8ToFloat4` in
`Formats.hlsli:75–81` replicates intensity into RGB. A normal binary 0/255
shadow therefore becomes transparent black/opaque white, not colored scenery.
The read-only final RAM analysis supplied by the stability reviewer found
3457 zero and 639 FF bytes for the bulldozer mask, and 3534 zero and 562 FF
bytes for the train mask. It was not synchronized to the mission-start image.
These facts support checking the actual GPU tile-copy, reinterpretation output
and descriptor binding if the suspect polygon is identified as a shadow.

`FramebufferRenderer::createGPUTiles`, at 236–252, resolves the final copy ID,
assigns its GPU texture index and dimensions and sets alphaIsCvg false for
reinterpretations. `getTextureIndex(TileCopy)`, at 300–305, adds that actual
texture to the dynamic views; `updateShaderDescriptorSet`, at 411–414, binds
the views. The present trace stops before this mapping and does not read GPU
texels. Its next precise source boundary is the externally called
`createGPUTiles` (called from `rt64_workload_queue.cpp:653`) after the real call:
record copy ID, texture identity, used/allocated dimensions, final textureIndex,
TC/UL scale and matching dynamic view. This can verify descriptor selection
without adding guest writes or GPU commands; texture contents still require
a GPU capture or correctly synchronized readback.

`mission-start-compositor.png` and the independent original
`original-route-before.png` now show the same tutorial prompt and similar
initial camera. The native image has hard polygon boundaries around the
vehicle, while the original has a softer dark shadow. That is a more useful
comparison than the unmatched demolition images. The native fault is already
visible near VI 1950, before this trace's MIN_VI 4000 window. An exact frame's
draw/copy capture should target that initial view, rather than increasing a
later route cap. Equivalent original-refresh and original-aspect runs also
showed displaced scene polygons according to root's evidence; neither setting
is established as a necessary trigger.

Verdict: the capture narrows the investigation but proves no production fix.
No abnormal visible track width, sampled main-image alias, wrong pending/render
mode, incomplete main clear or onscreen train shadow was demonstrated in its
sampled interval. The actual initial polygon's GPU attachment, texture and
contents remain the decisive missing evidence. No effects were disabled and
no culling, geometry, blending or framebuffer workaround is recommended.

## Initial-frame GPU pixel history and bound resources

The subsequent private RenderDoc 1.42 capture
`initial-original-scale_frame2230.rdc` was taken at original resolution/aspect
and original refresh, before the producer-fairness correction. Successful
replays inspect actual GPU resources, rather than inferring their contents
from the earlier CPU trace. The reports are retained under
`.local/qa/renderdoc/initial-original-scale-resources`,
`initial-terrain-resources` and `initial-terrain-mesh`.

Pixel history for color resource 951 identifies terrain draw 1501, primitive
11, at (190,205), and primitive 12 at (200,190). At (130,190) it identifies
terrain draw 1496, primitive 27. At (160,155), terrain primitive 11 is followed
by shadow draw 1787, primitive 0; the latter passes but leaves the color
unchanged at that particular pixel. These observations identify the sampled
colored patches as terrain. They do not prove that the entire shadow draw
is transparent or that all visible discrepancies share this source.

Draw 1501 has render index 35, instance 36 and 17 triangles starting at index
2079. Actual GPU tile 49 selects descriptor-array element 308, a 64x32 soil
texture; its TC/UL scales are both [1,1]. The RDP tile uses RGBA16, stride 128,
64x32 wrapping and unit shifts. Primitive 11 projects to approximately
(204.10,233.75), (142.19,69.73), (81.65,110.86), with positive clip w and
UVs (62.999,30.9995), (0,0), (0,30.9995). Primitive 12 completes the adjacent
triangle of that large ground quad. These dimensions and UVs are evidence,
not a demonstrated projection or texture-selection defect. The sparse
earlier source trace does not contain this exact index/face-count pair, so
it cannot yet identify this call's source geometry producer.

Shadow draw 1787 has render index 150 and GPU tile 220. It selects element
1562/resource 46005, with the expected 64x64 IA8 dimensions. The actual
reinterpretation output contains 3443 transparent black texels and 653 opaque
white texels. Its vertex RGB 10 and alpha 140 match the guest shadow quad.
Raw pre-reinterpretation copies have different channel meaning and must not
be confused with this bound IA8 output. The actual selected output is a binary
mask; it does not contain a copied colored scene. Comparison with the original
shadow mask remains necessary before considering a production correction.

### The initial reference shadow has a renderer regression

Offline extraction of the original Mupen64Plus state validates its M64+SAVE
header/version and locates RDRAM at byte 444 from the pinned core's state
reader. The owner-4 mask at 0x80359100 contains 2587 zero bytes, 853 bytes of
value 8 and 656 FF bytes. The native mask contains 3443 zero and 653 FF bytes,
consistent with the GPU IA8 output. Every original byte 8 lies at x modulo
4 equal to 3, before the unfilled final row. The white silhouettes overlap
closely (640 common texels, 669 union), although frame/camera equivalence
is not established for every sample.

This difference is attributable to the reference renderer's fill expression.
Angrylion commit `4cc07ca8094f` changed the grouped byte-extraction shift into
`(fill_color >> ((fb & 3)^3) << 3) & 0xff`. For guest fill 0x00010001, this
produces [0,0,0,8], matching the original-state pattern exactly. The expression
before that commit, and the independently grouped shifts in parallel-rdp and
MAME, produce [0,1,0,1]. Thus the stronger broad darkening in this Angrylion
reference cannot be treated as the required hardware appearance. A private
reference with the grouping restored must regenerate its cached masks before
a new visual comparison; loading the old state alone preserves its bad bytes.
Production must not be changed to reproduce the reference's value-8 artifact.

The private offline report and extracted masks are under
`.local/qa/graphics-source-tools/terrain-shadow-cpu`. It also compares every
captured vertex of draws 1496/1501/1787: post-VS screen positions differ from
their input screen positions only by the expected +0.5 pixel center offset,
within approximately 0.00011 pixel. No additional vertex expansion by the
raster vertex shader is demonstrated in these draws. The visible route arcs
and displaced scene fragments remain separate open defects requiring their
own captured draw/resource evidence.
