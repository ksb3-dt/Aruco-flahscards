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
- CMake ≥ 3.16
- A C++17 compiler

Install OpenCV on Ubuntu:

```bash
sudo apt install libopencv-dev libopencv-contrib-dev
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

Smoke test with an Android phone running the **IP Webcam** app:

```bash
./build/ar_flashcards http://<phone-ip>:8080/video
```

With a local USB webcam:

```bash
./build/ar_flashcards webcam:0
```

With a recorded video clip:

```bash
./build/ar_flashcards file:assets/test_clip.mp4
```

Press `ESC` to quit.

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
