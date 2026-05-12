# AR Flashcards

Augmented Reality flashcards using ArUco markers and OpenCV in C++.

CIN0135 — Estruturas de Dados Orientadas a Objetos
Centro de Informática, UFPE — 2026.1

## Team

- Kaíque Bonfim — [ksb3@cin.ufpe.br](mailto:ksb3@cin.ufpe.br)
- Luiz Miguel — [lmgra@cin.ufpe.br](mailto:lmgra@cin.ufpe.br)
- Luiz Taiguara — [ltog@cin.ufpe.br](mailto:ltog@cin.ufpe.br)

## What it does

Point a camera at a printed flashcard containing an ArUco marker. The
application detects the marker, computes its pose in 3D space, and overlays
educational content (an image of the human skeleton, a chemical structure,
a textured 3D wireframe, ...) onto the marker plane in real time.

## Requirements

- Linux (tested on Ubuntu)
- OpenCV 4 with `opencv_contrib` modules (for ArUco)
- SDL2, GLEW, OpenGL, and Assimp (for OBJ rendering)
- CMake ≥ 3.16
- A C++17 compiler

Install dependencies on Ubuntu:

```bash
sudo apt install libopencv-dev libopencv-contrib-dev libsdl2-dev libglew-dev libassimp-dev
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

Smoke test with an Android phone running the **IP Webcam** app:

```bash
./build/ar_flashcards http://<phone-ip>:8080/video config/assets.json
```

With a local USB webcam:

```bash
./build/ar_flashcards webcam:0 config/assets.json
```

With a recorded video clip:

```bash
./build/ar_flashcards file:assets/test_clip.mp4 config/assets.json
```

Press `ESC` to quit.

The default `config/assets.json` uses a `default` entry, so the skull model is
rendered on any detected marker from the `DICT_4X4_50` dictionary. Add numeric
marker IDs to the JSON only when you want specific cards to override the
default model.

## Architecture

See [CLAUDE.md](CLAUDE.md) for the architectural overview, design pattern
choices, and code conventions.

## Project layout

```
include/    Public headers (one class per file)
src/        Implementations
assets/     Flashcard images and 3D models
config/     Camera calibration YAML and asset-mapping JSON
```
