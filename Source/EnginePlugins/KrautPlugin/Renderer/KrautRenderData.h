#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <RendererCore/Pipeline/RenderData.h>

using xiiMeshResourceHandle = xiiTypedResourceHandle<class xiiMeshResource>;

class XII_KRAUTPLUGIN_DLL xiiKrautRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiKrautRenderData, xiiRenderData);

public:
  xiiMeshResourceHandle m_hMesh;
  xiiUInt32             m_uiUniqueID = 0;
  float                 m_fLodDistanceMinSQR;
  float                 m_fLodDistanceMaxSQR;
  xiiVec3               m_vLeafCenter;

  xiiUInt8 m_uiSubMeshIndex = 0;
  xiiUInt8 m_uiThisLodIndex = 0;
  bool     m_bCastShadows   = false;
  xiiVec3  m_vWindTrunk     = xiiVec3::ZeroVector();
  xiiVec3  m_vWindBranches  = xiiVec3::ZeroVector();
};
