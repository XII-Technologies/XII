#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Types/TagSet.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiWorld;
using xiiCpuMeshResourceHandle = xiiTypedResourceHandle<class xiiCpuMeshResource>;

/// \brief A utility to gather raw geometry from a world
///
/// The utility sends xiiMsgExtractGeometry to world components and they may fill out the geometry information.
/// \a ExtractionMode defines what the geometry is needed for. This ranges from finding geometry that is used to generate the navmesh from
/// to exporting the geometry to a file for use in another program, e.g. a modeling software.
class XII_RENDERERCORE_DLL xiiWorldGeoExtractionUtil
{
public:
  struct MeshObject
  {
    xiiTransform             m_GlobalTransform;
    xiiCpuMeshResourceHandle m_hMeshResource;
  };

  using MeshObjectList = xiiDeque<MeshObject>;

  /// \brief Describes what the geometry is needed for
  enum class ExtractionMode
  {
    RenderMesh,        ///< The render geometry is desired. Typically for exporting it to file.
    CollisionMesh,     ///< The collision geometry is desired. Typically for exporting it to file.
    NavMeshGeneration, ///< The geometry that participates in navmesh generation is desired.
  };

  /// \brief Extracts the desired geometry from all objects in a world
  ///
  /// The geometry object is not cleared, so this can be called repeatedly to append more data.
  static void ExtractWorldGeometry(MeshObjectList& ref_objects, const xiiWorld& world, ExtractionMode mode, xiiTagSet* pExcludeTags = nullptr);

  /// \brief Extracts the desired geometry from a specified subset of objects in a world
  ///
  /// The geometry object is not cleared, so this can be called repeatedly to append more data.
  static void ExtractWorldGeometry(MeshObjectList& ref_objects, const xiiWorld& world, ExtractionMode mode, const xiiDeque<xiiGameObjectHandle>& selection);

  /// \brief Writes the given geometry in .obj format to file
  static void WriteWorldGeometryToOBJ(const char* szFile, const MeshObjectList& objects, const xiiMat3& mTransform);
};

/// \brief Sent by xiiWorldGeoExtractionUtil to gather geometry information about objects in a world
///
/// The mode defines what the geometry is needed for, thus components should decide to participate or not
/// and how detailed the geometry is they return.
struct XII_RENDERERCORE_DLL xiiMsgExtractGeometry : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractGeometry, xiiMessage);

  /// \brief Specifies what the geometry is extracted for, and thus what the message handler should write back
  xiiWorldGeoExtractionUtil::ExtractionMode m_Mode = xiiWorldGeoExtractionUtil::ExtractionMode::RenderMesh;

  /// \brief Append mesh objects to this to describe the requested world geometry
  xiiWorldGeoExtractionUtil::MeshObjectList* m_pMeshObjects = nullptr;

  void AddMeshObject(const xiiTransform& transform, xiiCpuMeshResourceHandle hMeshResource);
  void AddBox(const xiiTransform& transform, xiiVec3 vExtents);
};
