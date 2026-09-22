# TMOD Lua Scripting API Reference

This document provides technical documentation for all native Lua functions introduced by TMOD. All functions are available in the client-side Lua environment.

---

## 1. Physics API (`CLuaTModPhysicsDefs`)

The Physics API controls the embedded Bullet Physics 3 simulation world.

### `tmodCreateRigidBody`
Creates an independent dynamic or static rigid body in the Bullet Physics simulation.

```lua
int bodyId = tmodCreateRigidBody(float x, float y, float z, float sx, float sy, float sz, float mass)
```
- **Parameters:**
  - `x, y, z`: Initial world coordinates.
  - `sx, sy, sz`: Box half-extents (dimensions of collision box).
  - `mass`: Mass of the body. Set to `0.0` for an immovable static body.
- **Returns:**
  - `int`: Unique integer identifier for the rigid body, or `-1` on failure.

---

### `tmodAttachPhysics`
Attaches a Bullet rigid body to an existing GTA client entity (object, vehicle, ped) and keeps their transforms synchronized.

```lua
int bodyId = tmodAttachPhysics(element pEntity [, float mass = 0.0, float sx = 0.0, float sy = 0.0, float sz = 0.0])
```
- **Parameters:**
  - `pEntity`: Target GTA element (`CClientObject`, `CClientVehicle`, etc.).
  - `mass`: Body mass (optional, default: `0.0`).
  - `sx, sy, sz`: Optional bounding box overrides. If omitted, the element's GTA bounding box is utilized.
- **Returns:**
  - `int`: Allocated body ID.

---

### `tmodDetachPhysics`
Detaches and destroys the physics body attached to a GTA element.

```lua
bool success = tmodDetachPhysics(element pEntity)
```

---

### `tmodBindBodyToElement`
Binds an existing Bullet rigid body ID to a GTA visual element.

```lua
bool success = tmodBindBodyToElement(int bodyId, element pEntity)
```

---

### `tmodDestroyBody`
Destroys a Bullet rigid body and frees its memory.

```lua
bool success = tmodDestroyBody(int bodyId)
```

---

### `tmodSetBodyPosition` / `tmodGetBodyPosition`
Sets or queries the world position of a rigid body.

```lua
bool success = tmodSetBodyPosition(int bodyId, float x, float y, float z)
float x, float y, float z = tmodGetBodyPosition(int bodyId)
```

---

### `tmodGetBodyRotation`
Queries the Euler rotation (in degrees) of a rigid body.

```lua
float rx, float ry, float rz = tmodGetBodyRotation(int bodyId)
```

---

### `tmodGetBodyVelocity` / `tmodSetBodyVelocity`
Gets or sets the linear velocity vector of a rigid body.

```lua
float vx, float vy, float vz = tmodGetBodyVelocity(int bodyId)
bool success = tmodSetBodyVelocity(int bodyId, float vx, float vy, float vz)
```

---

### `tmodApplyForce`
Applies a directional force to a rigid body at a specific relative position (impulse or continuous push).

```lua
bool success = tmodApplyForce(int bodyId, float fx, float fy, float fz, float px, float py, float pz)
```
- **Parameters:**
  - `fx, fy, fz`: Force vector components.
  - `px, py, pz`: Relative point of force application from the center of mass.

---

### `tmodFreezeBody` / `tmodIsBodyFrozen`
Freezes or unfreezes a rigid body in place (disables or enables motion integration).

```lua
bool success = tmodFreezeBody(int bodyId, bool freeze)
bool frozen = tmodIsBodyFrozen(int bodyId)
```

---

## 2. Movement API (`CLuaTModMovementDefs`)

The Movement API governs the Source Engine locomotion controller.

### `tmodSetMovementMode`
Switches the active locomotion physics model.

```lua
bool success = tmodSetMovementMode(string mode)
```
- **Parameters:**
  - `mode`: `"source"` (enables Source movement, air-strafing, bunnyhop) or `"default"` (reverts to GTA native movement).

---

### `tmodGetMovementMode`
Returns the currently active movement controller mode.

```lua
string mode = tmodGetMovementMode()
```

---

### `tmodSetAirAccelerate`
Sets the air acceleration coefficient (`sv_airaccelerate`), determining the sharpness and speed gain of air-strafing.

```lua
bool success = tmodSetAirAccelerate(float value)
```
- **Recommended Values:**
  - `100.0`: Standard Counter-Strike / Half-Life 2 style air control.
  - `300.0`: Enhanced agility.
  - `800.0`: High-speed surf physics.
  - `1500.0`: Ultra-responsive extreme air-control.

---

### `tmodSetAutoBhop`
Enables or disables continuous bunnyhopping when holding the Jump key.

```lua
bool success = tmodSetAutoBhop(bool enabled)
```

---

## 3. Camera API (`CLuaTModCameraDefs`)

The Camera API provides decoupled viewing modes and first-person viewmodel perspective.

### `tmodSetCameraMode`
Changes the active camera perspective.

```lua
bool success = tmodSetCameraMode(string mode)
```
- **Parameters:**
  - `"firstperson"`: Locks the camera to the player's head with visible viewmodel arms.
  - `"source"` or `"thirdperson"`: Source/GMod style third-person orbital camera with decoupled player look direction.
  - `"gta"`: Native GTA camera handling.

---

### `tmodGetCameraMode`
Returns the currently active camera mode name.

```lua
string mode = tmodGetCameraMode()
```

---

### `tmodSetCameraOffset`
Configures local position offset relative to the target attachment point.

```lua
bool success = tmodSetCameraOffset(float offsetX, float offsetY, float offsetZ)
```

---

### `tmodSetCameraDistance`
Configures third-person orbital camera distance.

```lua
bool success = tmodSetCameraDistance(float distance)
```

---

### `tmodSetWeaponViewmodel`
Binds a custom FBX viewmodel and texture to a specific GTA weapon type ID (e.g. 22 for Colt45, 24 for Deagle, 31 for M4). When the player equips that weapon, TMOD automatically switches to that viewmodel.

```lua
bool success = tmodSetWeaponViewmodel(int weaponType, string fbxPath, string texturePath)
```

---

### `tmodSetViewmodelOffset`
Configures the first-person viewmodel position and pitch tilt in camera space.

```lua
bool success = tmodSetViewmodelOffset(float right, float forward, float down, float pitchDeg)
```
- **Default values:** `right = 0.0`, `forward = 0.38`, `down = -0.22`, `pitchDeg = -12.0`.

---

### `tmodSetViewmodelScale`
Scales the viewmodel along forward length, vertical height, and lateral width. Used to elongate short arms to prevent cutoff clipping.

```lua
bool success = tmodSetViewmodelScale(float scaleForward, float scaleUp, float scaleRight)
```
- **Default values:** `scaleForward = 1.45`, `scaleUp = 1.15`, `scaleRight = 1.15`.

---

### `tmodSetViewmodelAnimInterval`
Configures how often the viewmodel animation triggers while in First Person mode (default: 3.0 seconds).

```lua
bool success = tmodSetViewmodelAnimInterval(float intervalSeconds)
```

---

### `tmodSetViewmodelAnimName`
Sets the name of the skeletal animation clip to play from the FBX file (default: `"CINEMA_4D_Main"`).

```lua
bool success = tmodSetViewmodelAnimName(string animName)
```

---

## 4. Asset & Map API (`CLuaTModAssetDefs`)

The Asset API handles dynamic importing of 3D models and automatic collision generation.

### `tmodLoadModel`
Parses and caches a 3D model file (FBX, OBJ, DAE, GLTF) using Assimp.

```lua
string handle = tmodLoadModel(string filePath)
```
- **Parameters:**
  - `filePath`: Relative or absolute path to the 3D model file (e.g. `"map_fbx/city_map.fbx"`).
- **Returns:**
  - `string`: Cached model handle string, or empty string on load failure.

---

### `tmodSpawnProp`
Spawns a loaded 3D model into the world with automatic Bullet collision geometry.

```lua
int propId = tmodSpawnProp(string modelHandle, float x, float y, float z, float rx, float ry, float rz, float mass, bool isStatic)
```
- **Parameters:**
  - `modelHandle`: Model identifier returned by `tmodLoadModel`.
  - `x, y, z`: World position.
  - `rx, ry, rz`: Rotation angles in degrees.
  - `mass`: Mass for dynamic simulation, or `0.0` for static map geometry.
  - `isStatic`: When `true`, constructs an optimized static `btBvhTriangleMeshShape` collision mesh.
- **Returns:**
  - `int`: Allocated prop identifier, or `-1` on error.

---

### `tmodSetPropPerma`
Marks a spawned prop or map element as permanent, protecting it from automatic garbage collection or reset routines.

```lua
bool success = tmodSetPropPerma(int propId, bool isPermanent)
```
