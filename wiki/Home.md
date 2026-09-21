# Welcome to the TMOD Wiki

Welcome to the official developer documentation for **Targut's Mod (TMOD)**.

TMOD is a modernized multiplayer sandbox engine developed as a major architectural fork of Multi Theft Auto: San Andreas (MTASA). By integrating modern physics, advanced locomotion models, arbitrary 3D asset pipelines, and custom rendering pipelines directly into the native client, TMOD transforms San Andreas into a flexible, high-performance sandbox environment reminiscent of Garry's Mod and Source Engine titles.

---

## Documentation Index

### 1. [Lua Scripting API Reference](Lua-API-Reference.md)
Complete specification of all Lua scripting functions introduced by TMOD:
- **Bullet Physics API:** Rigid body creation, forces, impulses, element attachment, velocity management, freezing.
- **Locomotion API:** Source movement modes, air-acceleration tuning, auto-bunnyhopping.
- **Camera API:** First-person viewmodel perspective, Source free-look, offsets, and field of view.
- **Asset & Auto-Collision API:** Dynamic FBX/OBJ loading, automatic triangle mesh collision generation, prop spawning.

### 2. [Engine Architecture](Architecture.md)
Detailed walkthrough of the internal C++ subsystems, manager classes, lifecycle hooks, and integration points with the San Andreas game loop and Direct3D9 rendering pipeline.

### 3. [Auto-Collision System](Auto-Collision-System.md)
Technical explanation of how TMOD circumvents legacy RenderWare DFF/COL limitations, parses 3D models with Assimp, and constructs runtime `btBvhTriangleMeshShape` collision objects in Bullet Physics.

### 4. [Getting Started & Development](Getting-Started.md)
Step-by-step instructions for setting up the development environment, compiling client and server binaries from source, launching a local test server, and utilizing the `tmod_tester` sandbox resource.

---

## High-Level Capabilities

| Subsystem | Underlying Technology | Description |
| :--- | :--- | :--- |
| **Physics Simulation** | Bullet Physics 3 (`btDiscreteDynamicsWorld`) | True rigid body simulation with collision manifolds, forces, impulses, and synchronization with GTA world entities. |
| **Player Locomotion** | Source Physics (`CTModMovementManager`) | Air-strafing, auto-bhop, ground friction, and decoupled character heading for directional movement. |
| **3D Asset Pipeline** | Assimp 5 (`aiScene`, `aiMesh`) | Runtime importing of FBX, OBJ, and DAE assets with materials and textures. |
| **Map Collision** | Bullet Triangle Mesh (`btBvhTriangleMeshShape`) | Arbitrary high-polygon collision without RenderWare limits or manual COL collision files. |
| **First Person Viewmodel** | Direct3D9 Vertex/Pixel Pipeline | First-person viewmodel arms and weapons attached to player camera viewport. |
| **Atmosphere Shaders** | Direct3D9 HLSL Post-Processing | Source Engine color grading, tone mapping, and lighting calibration. |
