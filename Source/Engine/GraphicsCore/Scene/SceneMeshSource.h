/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <GraphicsCore/Scene/SceneTypes.h>

/// Native extraction identity for one GPU scene mesh packet. These handles do not own world
/// objects. Retain the producing scene/view identity with asynchronous results and validate
/// world handles before use. Section indices describe mesh sections, not individual instances.
struct XII_GRAPHICSCORE_DLL xiiSceneMeshSource
{
  xiiSceneObjectHandle m_hSceneObject;
  xiiGameObjectHandle  m_hOwnerObject;
  xiiComponentHandle   m_hOwnerComponent;
  xiiUInt32            m_uiLodIndex      = xiiInvalidIndex; ///< Authored LOD for mesh-section candidates; invalid for unrestricted custom ranges.
  xiiUInt32            m_uiSectionIndex  = xiiInvalidIndex;
  xiiUInt32            m_uiMaterialIndex = 0U; ///< Mesh material slot, which can differ from the section index.
  xiiUInt32            m_uiRenderDataId  = 0U;
};
