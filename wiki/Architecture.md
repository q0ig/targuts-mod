# TMOD Engine Architecture

TMOD is designed around modular, high-cohesion manager singletons that integrate seamlessly into the core Multi Theft Auto client cycle (`CClientGame::DoPulse`). This document details each subsystem's technical implementation, lifecycle, and memory management.

---

## 1. Lifecycle & Frame Synchronization

Every frame, the San Andreas client runs through `CClientGame::DoPulse`. TMOD injects update hooks in the following sequence:

```
+-------------------------------------------------------------+
| CClientGame::DoPulse (Main Client Tick)                     |
+-------------------------------------------------------------+
   |
   +--> 1. CTModMovementManager::Update(deltaTime)
   |       - Reads input keystrokes and mouse delta
   |       - Calculates air acceleration / ground friction
   |       - Injects velocity directly into player ped
   |
   +--> 2. CTModPhysicsManager::Update(deltaTime)
   |       - Steps the Bullet physics world (btDiscreteDynamicsWorld)
   |       - Synchronizes transforms of active rigid bodies to GTA elements
   |
   +--> 3. CTModCameraManager::Update(deltaTime)
   |       - Computes camera matrix based on active mode (FP / Free-look)
   |       - Overrides native GTA camera orientation and matrix
   |
   +--> 4. Direct3D9 Scene End / Hook (CProxyDirect3D9)
           - CTModViewmodelManager::Render(pDevice)
           - CTModPostFXManager::Render(pDevice)
```

---

## 2. Subsystem Breakdown

### `CTModPhysicsManager`
- **Location:** `Client/mods/deathmatch/logic/physics/`
- **Engine:** Bullet Physics 3 SDK
- **Responsibilities:**
  - Manages `btDefaultCollisionConfiguration`, `btCollisionDispatcher`, `btDbvtBroadphase`, and `btSequentialImpulseConstraintSolver`.
  - Maintains `std::unordered_map<int, btRigidBody*>` tracking active rigid bodies.
  - Maintains `std::unordered_map<CClientEntity*, int>` binding GTA entities (`CClientObject`, `CClientVehicle`) to Bullet bodies.
  - Converts between GTA coordinates (Z-up) and Bullet coordinates seamlessly.

### `CTModMovementManager`
- **Location:** `Client/mods/deathmatch/logic/movement/`
- **Physics Model:** Source Engine Locomotion
- **Responsibilities:**
  - Re-implements Quake/Source `PM_AirAccelerate` and `PM_Friction` algorithms.
  - Overrides default GTA turning behavior: character orientation matches player aim vector, while `A`, `S`, `D` movements apply orthogonal strafe vectors without rotating the character mesh.
  - Manages automatic jump triggers on ground contact when `m_bAutoBhop` is enabled.

### `CTModCameraManager`
- **Location:** `Client/mods/deathmatch/logic/camera/`
- **Responsibilities:**
  - Manages decoupled camera states: First Person, Source Third Person, GTA Native.
  - Intercepts mouse look angles and applies pitch/yaw clamping.
  - Aligns head bone position in first person while attaching viewmodel arms to the view frustum.

### `CTModAssetManager`
- **Location:** `Client/mods/deathmatch/logic/assets/`
- **Engine:** Open Asset Import Library (Assimp 5)
- **Responsibilities:**
  - Parses 3D model formats (`.fbx`, `.obj`, `.dae`, `.gltf`) into GPU vertex buffers (`VertexPosNormalTex`).
  - Automatically converts mesh geometry into Bullet `btBvhTriangleMeshShape` collision objects.
  - Manages scale normalization heuristics to handle varying coordinate scales (e.g. centimeters vs meters in DCC software).

### `CTModViewmodelManager` & `CTModPostFXManager`
- **Location:** `Client/mods/deathmatch/logic/rendering/`
- **Responsibilities:**
  - **Viewmodel:** Renders animated first-person arms and weapon attachments directly in the D3D9 viewport with dedicated FOV and near-plane clipping.
  - **PostFX:** Fullscreen quad rendering executing HLSL pixel shaders for color correction, tone mapping, and ambient exposure control.

---

## 3. Lua Bridge Pattern (`CLuaTMod*Defs`)

All C++ functionality is exposed to Lua using MTASA's type-safe `ArgumentParser<Function>` template system. Each definition file registers its functions into `CLuaCFunctions` at startup through `CLuaManager::CLuaManager()`:
- `CLuaTModPhysicsDefs::LoadFunctions()`
- `CLuaTModMovementDefs::LoadFunctions()`
- `CLuaTModCameraDefs::LoadFunctions()`
- `CLuaTModAssetDefs::LoadFunctions()`
