#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/Declarations.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Base class for components to push generic render data.
class XII_GRAPHICSCORE_DLL xiiRenderData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderData, xiiReflectedClass);

public:
  struct Caching
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Never,
      IfStatic,

      Default = Never
    };
  };

  xiiRenderData()          = default;
  virtual ~xiiRenderData() = default;

  xiiMat4              m_GlobalTransform = xiiMat4::MakeIdentity();
  xiiBoundingBoxSphere m_GlobalBounds;

  xiiGameObjectHandle m_hOwnerObject;
  xiiComponentHandle  m_hOwnerComponent;

  xiiUInt64 m_uiSortingKey = 0;
};
