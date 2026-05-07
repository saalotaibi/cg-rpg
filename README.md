# Cozy Room RPG

Cozy Room RPG is a C++ OpenGL/GLUT application submitted for the CS2206 Computer Graphics project. The player navigates a textured cozy room, completes daily productivity tasks, and runs Pomodoro sessions while the on-screen statistics and health respond to progress.

## Screens

The game contains three primary screens:

1. Title screen. Entry point. Starts the game and can reset the saved profile.
2. Onboarding screen. Collects the player name, character selection, and task list.
3. Main gameplay screen. Renders the cozy room with the player and pet on the left, and the productivity panel on the right.

The main gameplay screen also exposes several visible state variations: idle, Pomodoro focus, Pomodoro break, partial task progress, and the task completion animation.

## Rubric coverage

* Project category: game.
* Multiple screens: title, onboarding, and gameplay.
* Multiple textured PNG assets across the room and characters.
* 2D viewing through `gluOrtho2D` for the entire game.
* Geometric transformations: `glTranslatef` and `glScalef` are applied to every sprite, and `glRotatef` is applied to the pet for a small wobble driven by its walk phase.
* Keyboard and mouse interaction across all screens.
* Custom bitmap text rendering.

## Dependencies

The project requires a C++17 compiler, GNU Make, OpenGL, GLU, GLEW, and GLUT (freeglut is supported).

### Arch Linux

```sh
sudo pacman -S base-devel mesa glu glew freeglut
```

### Ubuntu and Debian

```sh
sudo apt install build-essential libgl1-mesa-dev libglu1-mesa-dev libglew-dev freeglut3-dev
```

### Windows

Development was carried out on Linux. The provided `Makefile` uses GCC-style flags, so the recommended Windows toolchain is MSYS2.

1. Install MSYS2 from https://www.msys2.org and complete the post-install update steps documented on the MSYS2 website.
2. Open the shell named "MSYS2 MINGW64" from the Start menu. The standard "MSYS2" shell will not produce a Windows-native executable; the MINGW64 environment is required.
3. Install the toolchain and graphics libraries:

   ```sh
   pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-mesa mingw-w64-x86_64-glew mingw-w64-x86_64-freeglut make
   ```

## Build

### Linux

From inside the `game` folder:

```sh
make
```

The build produces a single executable named `cozy-room`.

### Windows (MSYS2 MINGW64)

From inside the `game` folder, in the MSYS2 MINGW64 shell:

```sh
make
```

The build produces a single executable named `cozy-room.exe` in the same folder.

## Run

### Linux

From inside the `game` folder:

```sh
./cozy-room
```

### Windows (MSYS2 MINGW64)

From inside the `game` folder, in the MSYS2 MINGW64 shell:

```sh
./cozy-room.exe
```

The application can also be launched by double-clicking `cozy-room.exe` in File Explorer. If Windows reports a missing DLL when launched outside of the MSYS2 shell, the required libraries are located in `C:\msys64\mingw64\bin`. Copy the following files into the same folder as `cozy-room.exe`:

```
freeglut.dll
glew32.dll
libstdc++-6.dll
libgcc_s_seh-1.dll
libwinpthread-1.dll
```

These are the libraries typically required. Filenames may vary between MSYS2 releases; if an additional DLL is reported as missing, copy that file from `C:\msys64\mingw64\bin` as well.

## Controls

### Title screen

* `Space` or `Enter`: start the game.
* `R`: reset the saved profile.
* `Esc`: quit.

### Onboarding

* Letter keys: type the player name or a task.
* `Left` and `Right`: change character selection.
* `Up` and `Down`: move between task rows.
* `Enter`: confirm the current step or the selected action.
* `Backspace`: edit text or remove the selected task.

### Main gameplay

* `W`, `A`, `S`, `D` or arrow keys: move the player.
* `P`: start or pause the Pomodoro timer.
* `R`: reset the Pomodoro timer.
* `H`: toggle the help overlay.
* `1` through `9` and `0`: mark the corresponding task as complete.
* Left mouse button on a task: mark the task as complete.
* Left mouse button on a Pomodoro button: start, pause, or reset the timer.
* `Esc`: quit.

After several seconds of input inactivity, an automatic wander behavior takes control of the character.

## Profile save file

The local player profile is stored at:

```
~/.cozy-room/profile.json
```

On Windows, this resolves to `C:\Users\<user>\.cozy-room\profile.json`. Pressing `R` on the title screen deletes the saved profile and triggers onboarding on the next run.

## Source layout

```
src/main.cpp        GLUT setup, main loop, input callbacks
src/scene.cpp       shared game state, tasks, statistics, Pomodoro
src/home.cpp        room rendering, walkable mask, player and pet draws (rotation applied to the pet)
src/panel.cpp       right-side UI panel and mouse hit testing
src/onboard.cpp     first-run onboarding wizard
src/profile.cpp     local profile save and load
src/ai.cpp          automatic wander behavior
src/player.cpp      player state
src/pet.cpp         pet state
src/texture.cpp     PNG loader wrapper
src/textures.cpp    global project textures
src/sprite.cpp      textured sprite draws using glTranslatef and glScalef
src/font.cpp        custom bitmap text renderer
src/ui.cpp          shared UI helpers
```

## Credits

* Pixel art for the room and characters is taken from LimeZu, *Modern Interiors*, available on itch.io at https://limezu.itch.io/moderninteriors.
* `stb_image.h` by Sean Barrett is used for PNG loading.
* `stb_image_write.h` by Sean Barrett is used for screenshot output.
* OpenGL, GLU, GLEW, and GLUT (freeglut) are used for rendering and windowing.

## Clean

```sh
make clean
```
