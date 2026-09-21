# Targut's Mod (TMOD)

Targut's Mod (TMOD) is a next-generation physics and sandbox game engine built on top of the open-source Multi Theft Auto: San Andreas (MTASA) core. It modernizes the classic San Andreas engine by integrating **Bullet Physics 3**, **Source Engine locomotion mechanics**, an **Assimp-powered FBX/3D model pipeline with automatic triangle mesh collision**, a **decoupled first-person viewmodel and camera system**, and **Source post-processing color grading**.

---

## Key Features

### 1. Bullet Physics 3 Integration
- **True Rigid Body Simulation:** Real-time physics simulation using Bullet Physics 3 alongside native San Andreas elements.
- **Dynamic Entities & Forces:** Create custom dynamic rigid bodies with customizable mass, friction, restitution, and inertia tensors.
- **GTA Element Synchronization:** Real-time bi-directional transform synchronization between Bullet rigid bodies and client visual entities (`CClientObject`, `CClientVehicle`).
- **Interactive Manipulation:** Full support for impulse application, continuous directional forces, velocity setters/getters, and freeze/unfreeze states.

### 2. Source Engine Locomotion & Physics
- **Source Physics Simulation:** Decouples player locomotion from native GTA animations and physics to deliver authentic Source Engine movement.
- **Air Acceleration & Air-Strafing:** Smooth air-strafing with configurable acceleration parameters (`sv_airaccelerate`).
- **Bunnyhopping (Bhop):** Built-in continuous momentum preservation and optional auto-bhop mechanics.
- **Directional Decoupling:** Player character movement aligns with look direction; strafing left/right/backwards executes directional movement animations without rotating the player body unnaturally.

### 3. Assimp 3D Pipeline & Auto-Collision System
- **Universal 3D Asset Loading:** Native support for FBX, OBJ, DAE, and other 3D formats directly inside the client engine via the Open Asset Import Library (Assimp).
- **RenderWare Limit Bypass:** Bypasses legacy RenderWare engine restrictions (DFF hierarchy limits, collision chunk bounds, material count restrictions).
- **Automatic Collision Mesh Generation:** Automatically extracts mesh vertex and triangle index data to construct static Bullet `btBvhTriangleMeshShape` collision geometry at runtime.
- **City-Scale Maps:** Seamlessly renders large external maps and environments with high polygon counts and custom PBR textures.

### 4. Custom Camera & Viewmodel System
- **Multiple Camera Perspectives:**
  - **First Person Mode:** Head-mounted camera with decoupled body rotation and viewmodel hands.
  - **Source / GMod Free-Look:** Smooth orbital and third-person perspectives with decoupled viewing angles.
  - **GTA Standard Mode:** Seamless fallback to native third-person behavior.
- **Counter-Strike / GMod Style Viewmodels:** Real-time Direct3D9 rendering of animated first-person arms and weapon models attached to the camera viewport.

### 5. Source Atmosphere & PostFX
- **Custom D3D9 Shaders:** Integrated post-processing pipeline featuring color grading, tone mapping, and contrast curve adjustments to match the visual atmosphere of Source Engine titles.
- **Dynamic Lighting Calibration:** Glare reduction and daylight calibration specifically designed for high-resolution external FBX textures.

---

## Architecture Overview

```
+-------------------------------------------------------------+
|                      TMOD Lua Scripting                     |
|  (tmodCreateRigidBody, tmodSetMovementMode, tmodLoadModel)  |
+-------------------------------------------------------------+
                              |
+-------------------------------------------------------------+
|                  CLuaTMod* Definition Layer                 |
|  CLuaTModPhysicsDefs | CLuaTModMovementDefs | CLuaTModCamera|
+-------------------------------------------------------------+
                              |
+-------------------------------------------------------------+
|                    TMOD Core C++ Managers                   |
|  CTModPhysicsManager       |  CTModMovementManager          |
|  CTModAssetManager         |  CTModCameraManager            |
|  CTModViewmodelManager     |  CTModPostFXManager            |
+-------------------------------------------------------------+
           |                         |               |
+----------------------+  +--------------------+  +-----------+
| Bullet 3 Physics SDK |  | Assimp 3D Importer |  | Direct3D9 |
+----------------------+  +--------------------+  +-----------+
```

---

## Quick Start & In-Game Controls

When running with the included `tmod_tester` resource:

| Key / Command | Action |
| :--- | :--- |
| `F5` | Open the TMOD Developer & Testing Control Panel |
| `/tmod` | Toggle the in-game debug HUD and developer info |
| `Space` (Hold) | Continuous Auto-Bunnyhop (when enabled in Movement tab) |
| `W, A, S, D` + Mouse | Source air-strafing locomotion |
| `V` | Cycle camera modes (First Person / Source Third Person / GTA) |

---

## Building from Source

### Prerequisites (Windows)
- **Visual Studio 2022** (v143/v145 MSVC toolset) with:
  - Desktop development with C++
  - C++ MFC for latest build tools (x86 & x64)
- **Microsoft DirectX SDK (June 2010)**
- **Git**

### Build Steps
1. Clone the repository:
   ```cmd
   git clone https://github.com/q0ig/targuts-mod.git
   cd targuts-mod
   ```
2. Generate Visual Studio project files:
   ```cmd
   win-create-projects.bat
   ```
3. Open `Build/MTASA.sln` in Visual Studio 2022.
4. Set configuration to **Release** and platform to **Win32** for Client, or **x64** for Server.
5. Build Solution (`Ctrl + Shift + B`).
6. Run `win-install-data.bat` to install required runtime assets.

---

## Documentation & Wiki

Detailed documentation is available in the [`wiki/`](wiki/) directory:
- [Architecture & Engine Design](wiki/Architecture.md)
- [Lua Scripting API Reference](wiki/Lua-API-Reference.md)
- [Auto-Collision System Guide](wiki/Auto-Collision-System.md)
- [Getting Started & Setup Guide](wiki/Getting-Started.md)

---

## License

Targut's Mod is based on Multi Theft Auto: San Andreas (MTA:SA) and is licensed under the **GPLv3** (GNU General Public License version 3). See [LICENSE](LICENSE) for details. Third-party libraries (Bullet Physics 3, Assimp, BASS) retain their respective open-source licenses.
