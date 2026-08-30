# Console Tetris C++

A lightweight, high-performance command-line implementation of classic Tetris built in C++ using the Windows Console API (`Win32`). 

Features real-time input polling, smooth double-buffered rendering, line-clearing animations, dynamic piece rotation, and automatic aspect-ratio/buffer adjustment for modern Windows Terminal displays.

---

## Controls

| Key | Action | Description |
| :--- | :--- | :--- |
| **`←` (Left Arrow)** | Move Left | Shift piece left by 1 cell |
| **`→` (Right Arrow)** | Move Right | Shift piece right by 1 cell |
| **`↓` (Down Arrow)** | Soft Drop | Fast-fall active piece |
| **`Z`** | Rotate Clockwise | Rotates piece 90° clockwise (if collision permits) |

---

## Key Technical Features

- **Flicker-Free Rendering:** Uses `CreateConsoleScreenBuffer` and `WriteConsoleOutputCharacterW` to handle double-buffering rather than clearing stdout (`system("cls")`).
- **Aspect Ratio Fix:** Renders 2 horizontal characters per block cell (`AA`, `BB`, `##`) to prevent text vertical stretching in console fonts.
- **Dynamic Window Buffer Adjustment:** Reads console window bounds at launch to scale buffer sizes and stop line wrapping glitching.
- **Index-based Rotation Engine:** Rotates 4x4 array tiles algorithmically on-the-fly via mathematical index translation (`0°`, `90°`, `180°`, `270°`).

---

## How to Build & Run

### Method 1: Command Line (MinGW / GCC)
1. Open Command Prompt or PowerShell in the code directory.
2. Compile with `g++`:
   ```cmd
   g++ Tetris.cpp -o Tetris.exe -O2