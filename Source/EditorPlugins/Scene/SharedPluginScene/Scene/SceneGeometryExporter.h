/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Types/Status.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class xiiWorld;

/// Tooling export modes. Values preserve the existing scene export IPC field.
enum class xiiSceneGeometryExportMode : xiiInt32
{
  RenderMesh       = 0,
  CollisionMesh    = 1,
  NavmeshObstacles = 2,
};

/// Exports native mesh components as OBJ triangle geometry, without creating a rendering pipeline.
/// Includes LOD zero, selected sections, local instances and current skinning matrices. OBJ output
/// contains positions and faces; materials, UVs and normals are not exported. GPU-only geometry is
/// read back synchronously, so this operation belongs on the engine-process thread between frames.
/// The caller must hold the world's write marker. Empty selection exports no objects when bSelectionOnly
/// is true. Editor-tagged and inactive objects are excluded. Validation failures discard the pending
/// output before opening the destination. Final disk writes use xiiDeferredFileWriter and report I/O
/// failures; this writer does not guarantee atomic replacement against disk failure.
/// Collision providers use Core's xiiMsgBuildStaticMesh world-space description. Navmesh export requires
/// the native geometry module and supports whole-world queries only, since its interface has no object IDs.
class XII_SHAREDPLUGINSCENE_DLL xiiSceneGeometryExporter
{
public:
  static xiiStatus ExportOBJ(xiiWorld& world, xiiStringView sFile, xiiSceneGeometryExportMode mode,
                             const xiiMat3& coordinateTransform, bool bSelectionOnly = false,
                             xiiArrayPtr<const xiiGameObjectHandle> selection = {});
};
