# nspGL — WebGL + HTML viewer for the TI-Nspire CX (Ndless)

nspGL opens `.html` files on the calculator, runs their JavaScript, and renders
WebGL 1.0 (and basic 2D canvas) in software. It is a single Ndless program
(`nspgl.tns`).

Compatibility targets:

- **Original TI-Nspire Clickpad/Touchpad (including CAS):** OS 3.9.0.463 with
  the matching classic Ndless installer. The monochrome LCD is 4-bit grayscale;
  nspGL converts its RGB565 render buffer to the calculator's native format.
- **Color TI-Nspire CX (non-II):** OS 4.5.5.79 with the matching CX Ndless build.
  The existing RGB565 display path is retained.

The classic Clickpad uses its legacy timer hardware rather than the CX SP804
timer. nspGL selects the correct display and timer paths at runtime. These are
compatibility targets; actual-device validation is still required for each OS
and hardware revision.

## What's inside

| Part | What it does |
|---|---|
| `src/js/qjs/` | QuickJS (ES2023 JavaScript engine: `let/const`, arrow functions, classes, typed arrays, modules, promises) |
| `src/js/prelude.js` | Mini browser runtime: DOM tree, events, `querySelector`, CSS cascade, timers, `requestAnimationFrame`, `fetch`/`XMLHttpRequest` for local files, `Image`, `localStorage` (in memory) |
| `src/gl/glsl_*.c` | GLSL ES 1.00 compiler: preprocessor (`#define`, function macros, `#if`), structs, arrays, loops, user functions (inlined), all common built-ins, `texture2D`/`textureCube` |
| `src/gl/sgl.c` | Software WebGL 1.0 state machine + rasterizer: clipping, perspective-correct varyings, depth test, blending, culling, points/lines, textures (nearest/bilinear, repeat/clamp/mirror, cube maps), framebuffers/renderbuffers, `readPixels` |
| `src/js/webgl_js.c` | Full `WebGLRenderingContext` API for JS (all WebGL 1 calls and constants) |
| `src/js/canvas2d.c` | Small `CanvasRenderingContext2D`: rects, paths, arcs, fill/stroke, text, `drawImage`, `get/putImageData`, transforms |
| `src/app/` | HTML parser, layout (text wrapping, headings, lists, tables, links, images, canvases, `position:absolute/fixed` overlays), browser UI |

## Installing

1. Install the Ndless release that matches the calculator hardware and OS.
   For an original Clickpad/Touchpad on OS 3.9.0.463, use the classic OS 3.9.0
   installer. Do not use the CX installer; CX models use their separate OS
   3.9.1 release line. For a color CX on OS 4.5.5, use the matching CX installer.
2. Send `nspgl.tns` to the calculator (any folder; the `ndless` folder is fine).
3. Send your pages with a `.tns` suffix (use TiLP or TI-Nspire Computer Link;
   some versions of the Student Software refuse non-document `.tns` files), e.g. `cube.html.tns`. Scripts, images and
   other files a page loads also need `.tns`: `app.js.tns`, `tex.png.tns`.
   (TI-Nspire Computer Link / Student Software only transfers `.tns` files.)
4. Run `nspgl` once. It registers itself for `.html` and `.htm`, so afterwards
   you can open a page directly from the OS file browser.

Links and `src=` attributes are written normally in the HTML (`href="page2.html"`);
nspGL tries the name with `.tns` appended first, then the name as written.

## Keys

| Key | Action |
|---|---|
| arrows | scroll (or sent to the page as `ArrowUp`... when it listens for keys; then use `ctrl`+up/down to scroll) |
| `tab` | jump between links/buttons; `enter` follows |
| touchpad models only | moves a mouse cursor; click = `mousedown/up/click` + pointer events |
| Clickpad | use the arrow keys to navigate; `enter` activates the focused link/button (the Clickpad has no pointer surface) |
| letters, digits, `space`, `enter`, `del`(Backspace), `shift`, `ctrl` | delivered to JS as `KeyboardEvent`s with standard `key`/`code`/`keyCode` |
| `esc` | back / close page |
| `menu` | Back, Reload, 3D quality, fullscreen canvas, FPS counter, console, open file, quit |
| `doc` | JavaScript console (errors, `console.log`) |
| `on` | emergency stop (interrupts runaway scripts, quits) |

A red **!** in the corner means the page logged an error — open the console.

## Performance notes

The original Clickpad/CX family uses an ARM9 with no FPU and no GPU; every
pixel runs your fragment shader in an interpreter. On the monochrome Clickpad,
the display conversion is applied only at presentation time, so the renderer
and its RGB565 framebuffer remain shared with color CX builds. nspGL renders
WebGL canvases at a reduced
internal resolution (menu → *3D quality*, default 50 %, i.e. 160×120 for a
full-screen canvas) and scales up. It runs the CPU at 150 MHz while open.
`gl_FragCoord`, `drawingBufferWidth` and `viewport` keep using the canvas's
CSS-pixel size, so pages don't need changes.

Measured cost per frame (ARM926 instruction counts under emulation, default
50 % quality; real frame rates depend on cache behaviour, expect roughly these):

| Demo | Instructions/frame | Expected speed |
|---|---|---|
| `cube.html` (vertex colors) | ~5 M | ~15–25 fps |
| `texture.html` (bilinear texture) | ~9 M | ~10–15 fps |
| `lit.html` (lit torus, 400 triangles) | ~14 M | ~7–9 fps |
| three.js Phong cube | ~38 M | ~3 fps |
| three.js PBR scene (`three/std.html`) | ~60–100 M | ~1–2 fps |
| `plasma.html` (6 trig calls per pixel) | ~97 M | ~1 fps |
| `raymarch.html` | ~200 M | <1 fps |

Page start-up: ~0.2 s for small pages; three.js (600 KB of JS) takes ~3–4 s to
parse, and its first frame another 2–3 s while it compiles shaders.

Tips for fast pages: small canvases, simple fragment shaders (do work per
vertex), avoid `discard`, prefer `drawElements` with shared vertices.

## Limits

- No network: only local files (bare module imports like `import 'three'` won't
  resolve; vendor the library and import it by relative path).
- No WebGL2, no stencil buffer, no mipmaps (mip filters fall back to linear/nearest),
  `dFdx/dFdy/fwidth` return 0, no `gl_FragDepth`, no antialiasing.
- CSS support is basic (colors, display, fonts, text-align, margin/padding, simple
  absolute/fixed positioning). No flexbox/grid layout beyond centering.
- No audio/video, no WebAssembly.

## Building

Requirements: Ndless SDK on `PATH` (`nspire-gcc`, `nspire-ld`, `genzehn`, `make-prg`), Python 3 with Pillow
(font generation), `stb_image.h` (`STB=/path/to/stb`).

```sh
make          # -> nspgl.tns
make host     # -> build/nspgl-host, headless desktop harness for testing
```

Host harness:
`NSPGL_FRAMES=60 NSPGL_SHOTS=10,59 NSPGL_OUT=/tmp ./build/nspgl-host samples/cube.html`
writes PPM screenshots; `NSPGL_KEYS=5:tab,6:enter` presses keys at given frames.

## License

nspGL code: MIT. QuickJS: MIT (see `src/js/qjs/LICENSE`). Fonts: DejaVu (Bitstream Vera license).
stb_image: public domain / MIT.
