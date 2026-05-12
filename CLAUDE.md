# AR Flashcards — CIN0135 OOP Project

## Project context

Augmented Reality flashcards application for the CIN0135 (Estruturas de Dados
Orientadas a Objetos) practical project at CIn-UFPE, 2026.1 semester. Detects
ArUco markers on physical flashcards via a smartphone camera and overlays
educational content (skeleton image, molecule diagram, globe, etc.) onto the
marker plane in real time.

**Team:** Kaíque Bonfim (ksb3), Luiz Miguel (lmgra), Luiz Taiguara (ltog).

**Course requirements that drive the design:**
- Must demonstrate classes, inheritance, polymorphism, access modifiers,
  abstract classes
- Must use pointers and references
- Code must be commented and organized into classes
- Report must explain which OOP features are used and *where* in the code

## Architecture

Three polymorphic axes, each justified by genuine runtime variation:

| Hierarchy        | Why polymorphic                            | Concrete subclasses                              |
|------------------|--------------------------------------------|--------------------------------------------------|
| `Camera`         | Multiple physical camera sources           | `IPCamera`, `WebcamCamera`, `VideoFileCamera`    |
| `Asset`          | Different content types per marker         | `TextureAsset`, `WireframeAsset`                 |
| `Renderer`       | Different drawing strategies per asset     | `TextureRenderer`, `WireframeRenderer`           |

**Composition:** `ARApplication` owns one `Camera`, one `MarkerDetector`, one
`PoseEstimator`, and one `AssetManager`. The asset manager owns
`(Asset, Renderer)` pairs keyed by marker ID.

**Per-frame pipeline:**

```
Camera::nextFrame()
    -> MarkerDetector::detect(frame)
    -> for each detected marker:
           PoseEstimator::estimate(marker)
           AssetManager::findAsset/findRenderer(marker.id)
           Renderer::draw(frame, pose, asset, estimator)
    -> imshow
```

### Design patterns deliberately used

1. **Factory Method on `Camera`** — `Camera::create(uri)` dispatches on URI
   scheme. Adding a new camera type means editing one switch, not every call
   site.
2. **Factory Method on `Asset`** — `Asset::createFromFile(path)` dispatches on
   file extension.
3. **Factory Method (instance) on `Asset`** — `Asset::createRenderer()` returns
   the matching `Renderer`. Each `Asset` subclass knows its `Renderer`; clients
   don't need to.

## Code conventions

- **Language:** English throughout (identifiers, comments, commit messages,
  error messages).
- **Naming:** `PascalCase` for types, `camelCase` for methods and variables,
  `snake_case_` (trailing underscore) for private members.
- **Layout:** headers in `include/`, sources in `src/`. One class per file.
  Header name matches class name.
- **`#pragma once`** at the top of every header.
- **Smart pointers:** `std::unique_ptr` for ownership, raw pointers or
  references for non-owning access. No `new` / `delete` in client code.
- **Parameter passing:** `const T&` for non-trivial inputs (especially
  `cv::Mat`), `T&` for outputs that the function mutates.
- **`override`** on every virtual method in subclasses. **`final`** when
  appropriate.
- **Virtual destructor** on every abstract base class.
- **Forward declarations** in headers where possible to keep compile times
  down. Heavy includes (e.g. `<opencv2/aruco.hpp>`) only when needed.
- **Doxygen-style** `///` comments on every public method.
- **No exceptions across module boundaries** for recoverable errors — return
  empty `cv::Mat` or `std::optional`. Constructors may throw for unrecoverable
  setup failures (e.g. camera won't open).

## Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Smoke test with phone (IP Webcam app):

```bash
./build/ar_flashcards http://192.168.X.X:8080/video
```

With local webcam:

```bash
./build/ar_flashcards webcam:0
```

With a recorded clip (useful for iterating on rendering without juggling the
phone):

```bash
./build/ar_flashcards file:assets/test_clip.mp4
```

## Camera URI scheme

The factory in `Camera::create(uri)` recognizes:

| URI prefix              | Concrete class      | Example                                  |
|-------------------------|---------------------|------------------------------------------|
| `http://`, `https://`   | `IPCamera`          | `http://192.168.0.42:8080/video`         |
| `webcam:`               | `WebcamCamera`      | `webcam:0`                               |
| `file:`                 | `VideoFileCamera`   | `file:assets/test.mp4`                   |

## Smartphone camera setup

Using the **IP Webcam** Android app. Start the server, note the LAN IP, append
`/video` to the URL. Camera intrinsics differ per device — calibrate once
using OpenCV's chessboard tutorial and save to `config/calibration.yaml` via
`cv::FileStorage`.

For initial development without calibration, the `PoseEstimator` accepts
approximate intrinsics:
- `fx = fy = image_width`
- `cx = width / 2`, `cy = height / 2`
- `distCoeffs = [0, 0, 0, 0, 0]`

ArUco pose works surprisingly well with these defaults; calibrate later for
stability.

## Implementation status

Already implemented (smoke test runs):
- `Camera` hierarchy (factory + `IPCamera` + `WebcamCamera` + `VideoFileCamera`)
- `main.cpp` smoke test (live video display window)

To implement (one class per Claude Code session):
- `MarkerDetector` — wrap `cv::aruco::ArucoDetector`
- `PoseEstimator` — load calibration YAML, run `cv::solvePnP` per marker
- `Asset` hierarchy — `TextureAsset` + `WireframeAsset` + `createFromFile`
- `Renderer` hierarchy — `TextureRenderer` (`cv::warpPerspective`) +
  `WireframeRenderer` (`cv::projectPoints` + `cv::line`)
- `AssetManager` — JSON config loading, owns `(Asset, Renderer)` pairs
- `ARApplication` — wires the pipeline; refactor `main.cpp` to delegate to it

## Session workflow (please follow)

Each Claude Code session should:
1. Implement **one** class (or one closely-related pair).
2. End with `cmake --build build` succeeding.
3. End with a manual run that confirms the new behavior.
4. End with a focused commit (`feat: PoseEstimator with YAML calibration`).

Do not bundle unrelated changes. Do not leave the build broken between
sessions. If a header needs to change, change it explicitly and explain why
in the commit body.

## Out of scope

- Full 3D mesh rendering with OpenGL. `WireframeRenderer` using
  `cv::projectPoints` + `cv::line` is sufficient for the demo and keeps
  OpenCV as the only dependency.
- Cross-platform packaging. Linux only.
- Unit tests. Manual verification per session is sufficient for project scope.
- GUI controls. Keyboard (`ESC` to quit) is enough.
