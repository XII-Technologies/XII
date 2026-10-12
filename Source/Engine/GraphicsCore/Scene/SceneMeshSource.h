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
  xiiUInt32            m_uiSectionIndex = xiiInvalidIndex;
  xiiUInt32            m_uiRenderDataId = 0U;
};
