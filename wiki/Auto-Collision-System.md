# TMOD Auto-Collision System

In traditional Grand Theft Auto: San Andreas and Multi Theft Auto, adding custom maps or large 3D models requires exporting custom RenderWare `.dff` models and `.col` collision files. This legacy pipeline suffers from severe engine limitations:

1. **Vertex & Polygon Caps:** RenderWare meshes cannot exceed internal vertex count limits per model chunk without crashing or corrupting memory.
2. **Material & Texture Limits:** Complex scenes with modern PBR textures or dozens of submaterials cannot be represented cleanly in legacy `.txd` archives.
3. **Manual Collision Modeling:** Creating `.col` files requires specialized 3ds Max/Blender plugins, convex decomposition, and manual collision boundary generation.

---

## How TMOD Bypasses RenderWare Limitations

TMOD completely decouples 3D model rendering and collision from the legacy RenderWare engine pipeline. When a 3D model (such as an FBX or OBJ map) is loaded via `tmodLoadModel`:

```
+-------------------------------------------------------------+
| Model File (e.g. city_map.fbx)                              |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
| Assimp Importer (aiImportFile / Importer::ReadFile)         |
| Flags: aiProcess_Triangulate | aiProcess_GenSmoothNormals   |
+-------------------------------------------------------------+
         |                                           |
         v (Visual Mesh)                             v (Collision Data)
+-------------------------------+         +-------------------------------+
| Direct3D9 Vertex Buffer       |         | btTriangleIndexVertexArray    |
| Textured & Rendered via D3D9  |         | All vertices & triangle faces |
+-------------------------------+         +-------------------------------+
                                                         |
                                                         v
                                          +-------------------------------+
                                          | btBvhTriangleMeshShape        |
                                          | Bounding Volume Hierarchy     |
                                          +-------------------------------+
                                                         |
                                                         v
                                          +-------------------------------+
                                          | Bullet Discrete Dynamics World|
                                          | Real-time Physical Collision  |
                                          +-------------------------------+
```

---

## Technical Details

### 1. Vertex & Face Extraction
During `CTModAssetManager::LoadModel`, Assimp iterates through all `aiMesh` instances contained within the `aiScene`:
```cpp
for (unsigned int m = 0; m < scene->mNumMeshes; m++)
{
    aiMesh* mesh = scene->mMeshes[m];
    // Extracts position, normals, and UV coordinates
    for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
        // Populates vertex arrays
    }
    // Extracts triangulated indices
    for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
        // Populates index arrays
    }
}
```

### 2. Bullet Bvh Triangle Mesh Generation
For static map geometry (where `isStatic = true`), the extracted mesh indices and vertices are passed directly to `btTriangleIndexVertexArray`. Bullet compiles an accelerated **Bounding Volume Hierarchy (BVH)** tree:
```cpp
btTriangleIndexVertexArray* meshInterface = new btTriangleIndexVertexArray(
    totalTriangles,
    indices.data(),
    sizeof(int) * 3,
    totalVertices,
    (btScalar*)vertices.data(),
    sizeof(Vertex)
);

btBvhTriangleMeshShape* trimeshShape = new btBvhTriangleMeshShape(meshInterface, true);
```

### 3. Collision Benefits
- **Zero Collision Authoring Required:** Drag and drop an FBX map exported directly from Blender, Maya, or 3ds Max; collision is instantly generated.
- **Accurate Collision:** Complex architecture, stairs, slopes, and railings possess exact, pixel-perfect triangle collision.
- **High Performance:** Bullet's BVH tree performs fast AABB spatial queries, allowing hundred-thousand polygon maps to run at solid 60+ FPS without CPU bottlenecks.
