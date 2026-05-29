# Robotron 2084 on FPGA

A hardware-accelerated recreation of the classic arcade game **Robotron 2084** running on a Nexys Video FPGA board.  
The game uses a MicroBlaze softcore processor for game logic, a custom SystemVerilog HDMI controller with dual-frame buffers, sprite ROM, and palette cycling, plus USB keyboard input via MAX3421E.

## Features

- **Full arcade gameplay** – Fight 9 enemy types (Grunt, Hulk, Spheroid, Enforcer, Brain, Prog, Quark, Tank, Electrode) and save humans.
- **Hardware-accelerated rendering** – Sprites are drawn by a dedicated state machine using dual-frame buffers (BRAM) and a sprite ROM; no CPU load for drawing.
- **Color cycling palette** – During waves, certain colours cycle through an 8-step sequence, giving a dynamic neon effect.
- **Dynamic HUD** – Shows player health, score (up to 9 digits), and current wave number.
- **Persistent leaderboard** – Top 5 scores with 4-character names, stored during a session.
- **Unlimited procedurally generated waves** – Enemy composition scales with wave number; wave 6+ repeats wave‑5 difficulty.
- **Dual‑stick controls** – Move with **WASD**, shoot with **arrow keys** (emulates original arcade layout).

## Hardware Requirements

- **FPGA board**: Digilent Nexys Video (or any board with HDMI, USB host, and sufficient BRAM)
- **HDMI monitor** (1080p or 720p, scaled to 320×240 active area)
- **USB keyboard** (boot protocol)

## Software & Tools

- **Vivado** 2018.3 (or later) – for hardware synthesis and bitstream generation
- **Vitis** IDE – for software development on MicroBlaze
- **Provided USB host stack** (MAX3421E driver, HID, transfer layer)

## Setup Instructions

1. **Unzip the project**  
   Extract the provided `robotron_archive.zip` – it contains the Vivado project folder `robotron/`.

2. **Open the Vivado project**  
   - Launch Vivado.  
   - Open `robotron.xpr` from the extracted folder.

3. **Generate the bitstream**  
   - Run synthesis, implementation, and bitstream generation.  
   - Export hardware (including the bitstream) as an `.xsa` file.

4. **Create a Vitis platform**  
   - Launch Vitis.  
   - Create a new application project.  
   - Choose **Create a new platform from hardware (XSA)** and point to the `.xsa` file.  
   - Select the “Hello World” template (you will replace the source files).

5. **Import software sources**  
   - Delete the default `hello_world.c`.  
   - Copy all files from the provided `vitis_src` folder into your Vitis project source directory.  
   - Ensure the folder structure matches (e.g., create a `lw_usb` subfolder for the USB stack files).

6. **Build and run**  
   - Build the project (Ctrl+B).  
   - Run as **Single Application Debug** on the connected Nexys Video board.

> **Note**: The `srcs_ref` folder contains all the source files for reference (SystemVerilog, C, headers), but these alone cannot rebuild the project – they are provided for code inspection only.

## Controls

| Action        | Keys               |
|---------------|--------------------|
| Move          | `W` `A` `S` `D`    |
| Shoot         | Arrow keys (↑ ↓ ← →) |
| Start / Continue | `Enter`         |

In menu screens, press **Enter** to start a new game or return to the start screen after game over.

## Implementation Details

### Hardware (SystemVerilog)

- **`hdmi_text_controller_v1_0`** – AXI4‑Lite slave that exposes memory‑mapped registers for:
  - Player / enemy positions and sprite addresses  
  - Game state, health, score, wave  
  - 150 static sprites (for menus and transition animation)  
  - 25 enemy projectiles  
- **`color_mapper`** – Finite state machine that:
  - Clears/draws border, HUD, enemies, player, bullets, and enemy projectiles.  
  - Reads sprite data from a single‑port BRAM ROM (16×16 tiles, 4‑bit index).  
  - Writes to one of two frame‑buffer BRAMs (single‑port RAM, 320×240 × 4bpp).  
- **Double buffering** – The display reads from one buffer while the CPU writes to the other; a `wea1`/`wea2` toggle swaps buffers on `vsync`.  
- **`robotronv4_palette`** – Palette lookup with cycle enable and frame counter; cycles colours for indices 1,4,6,11,12 based on `frameCt[5:3]`.

### Software (C on MicroBlaze)

- **Game loop** – Runs in `main()`; polls USB keyboard, updates game state (menus, enemy spawning, gameplay, wave transition, game over).
- **Enemy AI** – Each enemy type has its own movement, shooting, and collision logic:
  - *Grunt*: moves directly toward player.  
  - *Hulk*: slow, bullet‑proof (knocked back), changes direction randomly.  
  - *Spheroid*: random movement, spawns Enforcers.  
  - *Enforcer*: random float, shoots sparks.  
  - *Brain*: seeks nearest human, shoots homing projectiles.  
  - *Prog*: faster Grunt, appears when a human touches a Brain.  
  - *Quark*: random movement, spawns Tanks.  
  - *Tank*: slow, fires 20‑shot spreads.  
  - *Electrode*: stationary, damages on contact.  
  - *Humans*: wander, give bonus points when saved.
- **Wave system** – `initWave()` populates enemies based on wave number (1‑5, then repeats wave 5). Uses a fast pseudo‑random generator (`fastRand()`).
- **Leaderboard** – Top 5 scores stored in RAM; after game over, the player can enter a 4‑character name using the keyboard.
- **USB keyboard** – Boot protocol HID driver (MAX3421E SPI, interrupt polling).

## Credits

- **Original Robotron 2084** – Williams Electronics (1982)  
- **FPGA implementation** – ECE 385 team @ University of Illinois Urbana‑Champaign  
- **HDMI text controller framework** – Zuofu Cheng (University of Illinois)  
- **MAX3421E USB host stack** –  ECE 385 team @ University of Illinois Urbana‑Champaign 


## Contact
Tyler Li -- tylerli3@illinois.edu
Charles Chen -- cychen8@illinois.edu
