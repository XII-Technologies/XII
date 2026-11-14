/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/Declarations.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Strings/String.h>
#include <MujocoPlugin/System/MuJoCoState.h>

/// Compiled metadata. Native identifiers are valid only for m_Id's generation and world.
struct xiiMuJoCoElementInfo
{
  xiiMuJoCoElementId  m_Id;
  xiiComponentHandle  m_hModel;
  xiiComponentHandle  m_hBinding;
  xiiGameObjectHandle m_hObject;
  xiiString           m_sName;
  xiiString           m_sDescription;
  xiiString           m_sTarget;
  xiiString           m_sType;
  xiiInt32            m_iBody            = -1;
  xiiInt32            m_iParentBody      = -1;
  xiiInt32            m_iNativeType      = -1;
  xiiInt32            m_iTarget          = -1;
  xiiInt32            m_iTargetType      = -1;
  xiiVec3             m_vAxis            = xiiVec3::MakeZero(); ///< Authored body-local joint axis.
  xiiUInt32           m_uiReferenceCount = 0;
  bool                m_bBindable        = false;
  double              m_Reference[7]     = {}; ///< qpos0 coordinates, including native W,X,Y,Z quaternion ordering.
};

struct xiiMuJoCoModelInstanceInfo
{
  xiiComponentHandle  m_hComponent;
  xiiGameObjectHandle m_hObject;
  xiiString           m_sResource;
  xiiString           m_sNamespace;
  bool                m_bPending   = false;
  bool                m_bMissing   = false;
  bool                m_bInstalled = false;
};
