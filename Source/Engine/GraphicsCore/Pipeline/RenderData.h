#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Base class for components to push generic render data.
class XII_GRAPHICSCORE_DLL xiiRenderData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderData, xiiReflectedClass);

public:
  enum class Category
  {
    Opaque,
    Masked,
    Transparent,
    ENUM_COUNT
  };

  xiiRenderData()          = default;
  virtual ~xiiRenderData() = default;

  xiiMat4              m_GlobalTransform = xiiMat4::MakeIdentity();
  xiiBoundingBoxSphere m_GlobalBounds;

  xiiUInt64            m_uiSortingKey = 0;
  Category             m_Category     = Category::Opaque;
};
