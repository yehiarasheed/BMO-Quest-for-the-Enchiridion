# BMO: Quest for the Enchiridion

A dual-realm 3D adventure game built with OpenGL, featuring BMO from Adventure Time on an epic quest through the Candy Kingdom and the Cursed Underworld Fortress.

## Game Description

In **BMO's Dual Realm Adventure**, the player begins in the vibrant Candy Kingdom, controlling BMO to navigate candy-themed obstacles such as lollipops, rolling donuts, jelly pits, and giant candy canes while collecting candy coins and cupcakes to increase their score. The main enemy, the Lich, challenges the player by appearing intermittently, requiring strategic movement and timing to avoid damage. 

The player's goal in this level is to rescue Finn, who marks the completion of the Candy Kingdom stage. Upon rescuing Finn, the Lich escapes with the Enchiridion, transporting the adventure into the **Cursed Underworld Fortress**, a fiery and hazardous realm. 

Here, the player faces a new set of dynamic obstacles including fire golems, fire rocks, and fire hammers, while evading or confronting the Fire Princess, the main enemy of this stage. The player must locate and collect the Demon Sword, which animates upon interaction, and use it to unlock Enchiridion. 

Along the way, animations, sound effects, and interactive transformations provide feedback for collisions, jumps, and collectibles, while dynamic lighting enhances the immersive experience. The game concludes when BMO obtains Enchiridion after collecting the Demon Sword, offering a seamless transition between levels with a clear story-driven objective.

## Game Environments

### Level 1: Candy Kingdom

#### Player Character
- **BMO** - Cartoon humanoid model

#### Enemy Character
- **Lich** - Cartoon low-poly villain model

#### Obstacles
- **Rolling Donut** - Rolling obstacle that moves dynamically
- **Giant Candy Cane** - Static obstacle blocking paths
- **Jelly Pit** - Animated wobbling hazard

#### Collectibles
- **Candy Coins** - Award points when collected
- **Cupcake** - Award points when collected

#### Target
- **Finn** - BMO's friend to rescue; marks level completion

### Level 2: Cursed Underworld Fortress

#### Player Character
- **BMO** - Same model as Level 1

#### Enemy Character
- **Fire Princess** - Humanoid fire enemy guarding key areas

#### Obstacles
- **Fire Rocks** - Falling or rolling hazards
- **Fire Hammer** - Swinging obstacle that damages BMO
- **Fire Golem** - Large, static obstacle blocking the path

#### Collectible
- **Demon Sword** - Main collectible needed to unlock the Enchiridion pedestal

#### Target
- **Enchiridion** - Final goal; placing Demon Sword here activates it

## Camera Perspectives

The game features two camera perspectives that can be toggled during gameplay:

- **First Person Point of View**: The camera is the player's eye, providing an immersive experience
- **Third Person Point of View**: The camera is behind and slightly above the player (the upper part of the player is visible to the camera)

## Controls

### Keyboard Controls
- **W / Up Arrow**: Move forward
- **S / Down Arrow**: Move backward
- **A / Left Arrow**: Move left (strafe)
- **D / Right Arrow**: Move right (strafe)
- **Shift**: Sprint / move faster
- **Spacebar**: Jump
- **ESC**: Toggle mouse look / show cursor
- **C**: Toggle camera mode

### Mouse Controls
- **Mouse Movement**: Rotate camera / look around (first-person or third-person perspective)
- **Left Mouse Button**: Jump (triggers jump action)
- **Right Mouse Button**: Toggle between first-person and third-person camera views
- **Middle Mouse Button**: Toggle mouse look on/off

### Debug Controls
- **U / O**: Rotate character left/right
- **P**: Print model positions and score
- **Z**: Wireframe mode
- **X**: Fill mode
- **Q**: Quit game

## Game Features

### Score Display
The score is displayed on the screen throughout gameplay, tracking collected items and progress.

### Animations & Interactive Elements

All game objects feature dynamic animations implemented in the code:

| Object | Transformation | Sound Effect |
|--------|---------------|--------------|
| **Coins** | Y-axis rotation (spin at 30° per frame) + vertical bounce (fly up 0.1 units per frame when collected) | Coin jingle |
| **Cupcake** | Y-axis rotation (spin at 20° per frame) + scale down effect (shrinks to zero over 50 frames when collected) | Pop/sparkle |
| **Demon Sword** | Y-axis rotation (spin at 20° per frame) + vertical float animation (rises 0.1 units per frame when collected) | Dark magical chime |
| **Fire Hammer** | X-axis pivot rotation (swings ±45° using sine wave) | Metallic clang |
| **Jelly** | Squash & stretch animation (Y-axis scales down while X/Z scales up to preserve volume) | Bubbling / squishy |
| **Donut** | Z-axis shake rotation (oscillates ±10° at 20Hz when hit) + Y-axis rotation | Thud / impact |
| **Fire Rocks** | Small vertical bounce (1 unit using sine wave when hit) | Thud / impact |
| **Fire Golem** | Z-axis push animation (translates 2 units back using sine wave when hit) | Roar / elemental |
| **Lich** | Translate back and forth movement | Roar / elemental |
| **Fire Princess** | Translate movement | Roar / elemental |
| **Finn** | Vertical bounce animation (moves up/down 2 units using sine wave on Y-axis when rescued) | Cheer / rescued |
| **Enchiridion Pedestal** | Pulsing scale animation (scales between 1.0 and 1.2 using sine wave when activated) | Magical unlocking chime |

### Dynamic Lighting

#### Candy Kingdom (Level 1) - Sunlight
- **Light Source**: Sunlight
- **Animation**:
  - Sun moves slowly across the sky (simulating day progression)
  - Light casts shadows that shift as sun moves
- **Color Intensity Change**:
  - Starts as bright white/yellow (midday)
  - Gradually dims to warmer orange/pink near "sunset"
  - Highlights candy textures and adds depth to the environment

#### Cursed Underworld Fortress (Level 2) - Lava
- **Light Source**: Lava glow
- **Animation**:
  - Lava pits flicker (light intensity oscillates)
- **Color Intensity Change**:
  - Lava oscillates between deep orange, red, and yellow to simulate heat and flicker

## Technical Details

### Project Structure
- **Language**: C++ with OpenGL
- **Graphics API**: OpenGL
- **Audio**: FMOD sound library
- **Model Formats**: 3DS and OBJ
- **Texture Format**: BMP

### Assets
- **Models**: Located in `/models` directory
- **Textures**: Located in `/textures` directory
- **Audio**: Located in `/audio` directory

## Building the Project

This is a Visual Studio project. Open `OpenGLMeshLoader.sln` in Visual Studio and build the solution.

### Requirements
- Visual Studio (2015 or later recommended)
- OpenGL
- GLUT library
- GLEW library
- FMOD audio library

## Credits

Based on the Adventure Time universe created by Pendleton Ward.

## License

This is an educational project.

---
