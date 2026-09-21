/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/Declarations.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <GraphicsCore/Declarations.h>

/// Base class for components to push generic render data.
class XII_GRAPHICSCORE_DLL xiiRenderData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderData, xiiReflectedClass);

public:
  struct Caching
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Never = 0U, ///< This render data should never be cached. It will be extracted every frame and not stored in the static cache.
      IfStatic,   ///< This render data can be cached if it is detected to be static. It will be extracted every frame until it is detected as static, then stored in the static cache and reused until invalidated.

      ENUM_COUNT,

      Default = Never
    };
  };

  xiiRenderData()          = default;
  virtual ~xiiRenderData() = default;

public:
  xiiTransform         m_GlobalTransform = xiiTransform::MakeIdentity();
  xiiBoundingBoxSphere m_GlobalBounds    = xiiBoundingBoxSphere::MakeZero();

  xiiGameObjectHandle m_hOwnerObject;
  xiiComponentHandle  m_hOwnerComponent;

  xiiUInt64 m_uiSortingKey = 0;
};
