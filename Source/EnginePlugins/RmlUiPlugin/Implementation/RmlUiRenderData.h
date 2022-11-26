#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Pipeline/RenderData.h>

using xiiTexture2DResourceHandle = xiiTypedResourceHandle<class xiiTexture2DResource>;

namespace xiiRmlUiInternal
{
  struct Vertex
  {
    XII_DECLARE_POD_TYPE();

    xiiVec3          m_Position;
    xiiVec2          m_TexCoord;
    xiiColorLinearUB m_Color;
  };

  struct CompiledGeometry
  {
    xiiUInt32                  m_uiTriangleCount = 0;
    xiiGALBufferHandle         m_hVertexBuffer;
    xiiGALBufferHandle         m_hIndexBuffer;
    xiiTexture2DResourceHandle m_hTexture;
  };

  struct Batch
  {
    xiiMat4          m_Transform   = xiiMat4::IdentityMatrix();
    xiiVec2          m_Translation = xiiVec2(0);
    CompiledGeometry m_CompiledGeometry;
    xiiRectFloat     m_ScissorRect           = xiiRectFloat(0, 0);
    bool             m_bEnableScissorRect    = false;
    bool             m_bTransformScissorRect = false;
  };
} // namespace xiiRmlUiInternal

class xiiRmlUiRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRmlUiRenderData, xiiRenderData);

public:
  xiiRmlUiRenderData(xiiAllocatorBase* pAllocator) :
    m_Batches(pAllocator)
  {
  }

  xiiDynamicArray<xiiRmlUiInternal::Batch> m_Batches;
};
