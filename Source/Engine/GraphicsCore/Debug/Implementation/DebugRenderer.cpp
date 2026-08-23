/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Debug/SimpleASCIIFont.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Types.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

xiiCVarFloat cvar_DebugTextScale("Debug.TextScale", 1.0f, xiiCVarFlags::Save, "Global scale for debug text.");

//////////////////////////////////////////////////////////////////////////

xiiDebugRendererContext::xiiDebugRendererContext(const xiiWorld* pWorld) :
  m_uiId(pWorld != nullptr ? pWorld->GetIndex() : 0)
{
}

xiiDebugRendererContext::xiiDebugRendererContext(const xiiViewHandle& hView) :
  m_uiId(hView.GetInternalID().m_Data)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiDebugTextHAlign, 1)
  XII_ENUM_CONSTANTS(xiiDebugTextHAlign::Left, xiiDebugTextHAlign::Center, xiiDebugTextHAlign::Right)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiDebugTextVAlign, 1)
  XII_ENUM_CONSTANTS(xiiDebugTextVAlign::Top, xiiDebugTextVAlign::Center, xiiDebugTextVAlign::Bottom)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiDebugTextPlacement, 1)
  XII_ENUM_CONSTANTS(xiiDebugTextPlacement::TopLeft, xiiDebugTextPlacement::TopCenter, xiiDebugTextPlacement::TopRight)
  XII_ENUM_CONSTANTS(xiiDebugTextPlacement::BottomLeft, xiiDebugTextPlacement::BottomCenter, xiiDebugTextPlacement::BottomRight)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

namespace
{
  struct alignas(16) Vertex
  {
    xiiVec3          m_vPosition;
    xiiColorLinearUB m_Color;
  };

  static_assert(sizeof(Vertex) == 16);

  struct alignas(16) TexVertex
  {
    xiiVec3          m_vPosition;
    xiiColorLinearUB m_Color;
    xiiVec2          m_fTexCoord;
    float            padding[2];
  };

  static_assert(sizeof(TexVertex) == 32);

  struct alignas(16) BoxData
  {
    xiiShaderTransform m_Transform;
    xiiColor           m_Color;
  };

  static_assert(sizeof(BoxData) == 64);

  struct alignas(16) GlyphData
  {
    xiiVec2          m_vTopLeftCorner;
    xiiColorLinearUB m_Color;
    xiiUInt16        m_uiGlyphIndex;
    xiiUInt16        m_uiSizeInPixel;
  };

  static_assert(sizeof(GlyphData) == 16);

  struct TextLineData2D
  {
    xiiString        m_sText;
    xiiVec2          m_vTopLeftCorner;
    xiiColorLinearUB m_Color;
    xiiUInt32        m_uiSizeInPixel;
  };

  struct TextLineData3D : public TextLineData2D
  {
    xiiVec3 m_vPosition;
  };

  struct InfoTextData
  {
    xiiString m_sGroup;
    xiiString m_sText;
    xiiColor  m_Color;
  };

  struct PerContextData
  {
    xiiDynamicArray<Vertex, xiiAlignedAllocatorWrapper>                                             m_LineVertices;
    xiiDynamicArray<Vertex, xiiAlignedAllocatorWrapper>                                             m_TriangleVertices;
    xiiDynamicArray<Vertex, xiiAlignedAllocatorWrapper>                                             m_Triangle2DVertices;
    xiiDynamicArray<Vertex, xiiAlignedAllocatorWrapper>                                             m_Line2DVertices;
    xiiDynamicArray<BoxData, xiiAlignedAllocatorWrapper>                                            m_LineBoxes;
    xiiDynamicArray<BoxData, xiiAlignedAllocatorWrapper>                                            m_SolidBoxes;
    xiiMap<xiiSharedPtr<xiiGALTextureView>, xiiDynamicArray<TexVertex, xiiAlignedAllocatorWrapper>> m_TexturedTriangle2DVertices;
    xiiMap<xiiSharedPtr<xiiGALTextureView>, xiiDynamicArray<TexVertex, xiiAlignedAllocatorWrapper>> m_TexturedTriangle3DVertices;

    xiiDynamicArray<InfoTextData>                          m_InfoTextData[(xiiUInt8)xiiDebugTextPlacement::ENUM_COUNT];
    xiiDynamicArray<TextLineData2D>                        m_sTextLines2D;
    xiiDynamicArray<TextLineData3D>                        m_sTextLines3D;
    xiiDynamicArray<GlyphData, xiiAlignedAllocatorWrapper> m_Glyphs;
  };

  struct DoubleBufferedPerContextData
  {
    xiiUInt32                    m_uiCurrentDataIndex = 0U;
    xiiUniquePtr<PerContextData> m_pData[2];
  };

  static xiiHashTable<xiiDebugRendererContext, DoubleBufferedPerContextData> s_PerContextData;
  static xiiMutex                                                            s_Mutex;

  static void ClearPerContextData(PerContextData& data)
  {
    data.m_LineVertices.Clear();
    data.m_Line2DVertices.Clear();
    data.m_LineBoxes.Clear();
    data.m_SolidBoxes.Clear();
    data.m_TriangleVertices.Clear();
    data.m_Triangle2DVertices.Clear();
    data.m_TexturedTriangle2DVertices.Clear();
    data.m_TexturedTriangle3DVertices.Clear();
    data.m_sTextLines2D.Clear();
    data.m_sTextLines3D.Clear();
    data.m_Glyphs.Clear();

    for (xiiUInt32 i = 0; i < static_cast<xiiUInt32>(xiiDebugTextPlacement::ENUM_COUNT); ++i)
    {
      data.m_InfoTextData[i].Clear();
    }
  }

  static PerContextData& GetDataForExtraction(const xiiDebugRendererContext& context)
  {
    DoubleBufferedPerContextData& doubleBufferedData = s_PerContextData[context];

    xiiUniquePtr<PerContextData>& pData = doubleBufferedData.m_pData[doubleBufferedData.m_uiCurrentDataIndex];
    if (pData == nullptr)
    {
      pData = XII_DEFAULT_NEW(PerContextData);
    }

    return *pData;
  }

  struct BufferType
  {
    enum Enum
    {
      LineBoxes,
      SolidBoxes,
      Glyphs,

      Count
    };
  };

  struct DynamicMeshBufferKind
  {
    enum Enum
    {
      Line,
      Triangle,
      TexturedTriangle,

      Count
    };
  };

  static xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>>  s_DataBufferPages[BufferType::Count];
  static xiiDynamicArray<xiiMeshBufferResourceHandle> s_DynamicMeshBufferPages[DynamicMeshBufferKind::Count];
  static xiiMeshBufferResourceHandle                  s_hLineBoxMeshBuffer;
  static xiiMeshBufferResourceHandle                  s_hSolidBoxMeshBuffer;
  static xiiTexture2DResourceHandle                   s_hDebugFontTexture;
  static xiiSharedPtr<xiiGALBuffer>                   s_pGlobalConstantsBuffer;

  static xiiSharedPtr<xiiGALInputLayout> s_pPositionOnlyInputLayout;
  static xiiSharedPtr<xiiGALInputLayout> s_pVertexInputLayout;
  static xiiSharedPtr<xiiGALInputLayout> s_pTexVertexInputLayout;

  static xiiShaderResourceHandle s_hDebugGeometryShader;
  static xiiShaderResourceHandle s_hDebugPrimitiveShader;
  static xiiShaderResourceHandle s_hDebugTexturedPrimitiveShader;
  static xiiShaderResourceHandle s_hDebugTextShader;

  struct DebugRenderPassKey
  {
    xiiEnum<xiiGALResourceFormat> m_ColorFormat       = xiiGALResourceFormat::Unknown;
    xiiEnum<xiiGALResourceFormat> m_DepthFormat       = xiiGALResourceFormat::Unknown;
    xiiUInt8                      m_uiSampleCount     = 1U;
    xiiUInt8                      m_uiArraySliceCount = 1U;

    XII_ALWAYS_INLINE bool operator<(const DebugRenderPassKey& rhs) const
    {
      if (m_ColorFormat != rhs.m_ColorFormat)
        return m_ColorFormat < rhs.m_ColorFormat;
      if (m_DepthFormat != rhs.m_DepthFormat)
        return m_DepthFormat < rhs.m_DepthFormat;
      if (m_uiSampleCount != rhs.m_uiSampleCount)
        return m_uiSampleCount < rhs.m_uiSampleCount;
      return m_uiArraySliceCount < rhs.m_uiArraySliceCount;
    }

    XII_ALWAYS_INLINE bool operator==(const DebugRenderPassKey& rhs) const
    {
      return m_ColorFormat == rhs.m_ColorFormat && m_DepthFormat == rhs.m_DepthFormat && m_uiSampleCount == rhs.m_uiSampleCount && m_uiArraySliceCount == rhs.m_uiArraySliceCount;
    }
  };

  enum class DebugPipelineKind : xiiUInt8
  {
    Geometry,
    Primitive,
    TexturedPrimitive,
    Text,
  };

  enum class DebugCameraMode : xiiUInt8
  {
    Perspective,
    Stereo,
    Orthographic,
  };

  struct DebugPipelineKey
  {
    DebugPipelineKind                m_Kind            = DebugPipelineKind::Primitive;
    DebugCameraMode                  m_CameraMode      = DebugCameraMode::Perspective;
    xiiEnum<xiiGALPrimitiveTopology> m_Topology        = xiiGALPrimitiveTopology::TriangleList;
    bool                             m_bPreTransformed = false;
    bool                             m_bMonochrome     = false;
    xiiGALRenderPass*                m_pRenderPass     = nullptr;

    XII_ALWAYS_INLINE bool operator<(const DebugPipelineKey& rhs) const
    {
      if (m_Kind != rhs.m_Kind)
        return m_Kind < rhs.m_Kind;
      if (m_CameraMode != rhs.m_CameraMode)
        return m_CameraMode < rhs.m_CameraMode;
      if (m_Topology != rhs.m_Topology)
        return m_Topology < rhs.m_Topology;
      if (m_bPreTransformed != rhs.m_bPreTransformed)
        return m_bPreTransformed < rhs.m_bPreTransformed;
      if (m_bMonochrome != rhs.m_bMonochrome)
        return m_bMonochrome < rhs.m_bMonochrome;
      return m_pRenderPass < rhs.m_pRenderPass;
    }

    XII_ALWAYS_INLINE bool operator==(const DebugPipelineKey& rhs) const
    {
      return m_Kind == rhs.m_Kind && m_CameraMode == rhs.m_CameraMode && m_Topology == rhs.m_Topology && m_bPreTransformed == rhs.m_bPreTransformed && m_bMonochrome == rhs.m_bMonochrome && m_pRenderPass == rhs.m_pRenderPass;
    }
  };

  static xiiMap<DebugRenderPassKey, xiiSharedPtr<xiiGALRenderPass>>          s_RenderPassCache;
  static xiiMap<DebugPipelineKey, xiiSharedPtr<xiiGALGraphicsPipelineState>> s_GraphicsPipelineCache;

  enum
  {
    DEBUG_BUFFER_SIZE               = 1024 * 256,
    BOXES_PER_BATCH                 = DEBUG_BUFFER_SIZE / sizeof(BoxData),
    LINE_VERTICES_PER_BATCH         = DEBUG_BUFFER_SIZE / sizeof(Vertex),
    TRIANGLE_VERTICES_PER_BATCH     = (DEBUG_BUFFER_SIZE / sizeof(Vertex) / 3) * 3,
    TEX_TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(TexVertex) / 3) * 3,
    GLYPHS_PER_BATCH                = DEBUG_BUFFER_SIZE / sizeof(GlyphData),
  };

  static xiiSharedPtr<xiiGALBuffer> EnsureDataBufferPage(BufferType::Enum bufferType, xiiUInt32 uiPageIndex, xiiUInt32 uiStructSize)
  {
    auto& bufferPages = s_DataBufferPages[bufferType];
    bufferPages.EnsureCount(uiPageIndex + 1U);

    if (bufferPages[uiPageIndex] == nullptr)
    {
      xiiGALBufferCreationDescription bufferDescription;
      bufferDescription.m_uiElementByteStride = uiStructSize;
      bufferDescription.m_uiSize              = DEBUG_BUFFER_SIZE;
      bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
      bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
      bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
      bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

      bufferPages[uiPageIndex] = xiiGALDevice::GetDefaultDevice()->CreateBuffer(bufferDescription);
    }

    return bufferPages[uiPageIndex];
  }

  static void DestroyDataBuffers(BufferType::Enum bufferType)
  {
    s_DataBufferPages[bufferType].Clear();
  }

  static xiiMeshBufferResourceHandle EnsureDynamicMeshBufferPage(DynamicMeshBufferKind::Enum kind, xiiUInt32 uiPageIndex)
  {
    auto& meshBufferPages = s_DynamicMeshBufferPages[kind];
    meshBufferPages.EnsureCount(uiPageIndex + 1U);

    if (meshBufferPages[uiPageIndex].IsValid())
      return meshBufferPages[uiPageIndex];

    xiiMeshBufferResourceDescriptor desc;
    desc.Clear();
    desc.m_ResourceUsage       = xiiGALResourceUsage::Mutable;
    desc.m_bAllowGpuDrivenDraw = false;

    xiiStringView sBaseName;

    switch (kind)
    {
      case DynamicMeshBufferKind::Line:
        desc.m_Topology = xiiGALPrimitiveTopology::LineList;
        desc.AddStream(xiiMeshVertexSemantic::Position, xiiMeshVertexStreamFormat::Float3, static_cast<xiiUInt16>(offsetof(Vertex, m_vPosition)), static_cast<xiiUInt16>(sizeof(Vertex)));
        desc.AddStream(xiiMeshVertexSemantic::Color0, xiiMeshVertexStreamFormat::UByte4Normalized, static_cast<xiiUInt16>(offsetof(Vertex, m_Color)), static_cast<xiiUInt16>(sizeof(Vertex)));
        desc.AllocateStreams(LINE_VERTICES_PER_BATCH, 0U, false);
        sBaseName = "DebugDynamicLines"_xiisv;
        break;

      case DynamicMeshBufferKind::Triangle:
        desc.m_Topology = xiiGALPrimitiveTopology::TriangleList;
        desc.AddStream(xiiMeshVertexSemantic::Position, xiiMeshVertexStreamFormat::Float3, static_cast<xiiUInt16>(offsetof(Vertex, m_vPosition)), static_cast<xiiUInt16>(sizeof(Vertex)));
        desc.AddStream(xiiMeshVertexSemantic::Color0, xiiMeshVertexStreamFormat::UByte4Normalized, static_cast<xiiUInt16>(offsetof(Vertex, m_Color)), static_cast<xiiUInt16>(sizeof(Vertex)));
        desc.AllocateStreams(TRIANGLE_VERTICES_PER_BATCH, 0U, false);
        sBaseName = "DebugDynamicTriangles"_xiisv;
        break;

      case DynamicMeshBufferKind::TexturedTriangle:
        desc.m_Topology = xiiGALPrimitiveTopology::TriangleList;
        desc.AddStream(xiiMeshVertexSemantic::Position, xiiMeshVertexStreamFormat::Float3, static_cast<xiiUInt16>(offsetof(TexVertex, m_vPosition)), static_cast<xiiUInt16>(sizeof(TexVertex)));
        desc.AddStream(xiiMeshVertexSemantic::Color0, xiiMeshVertexStreamFormat::UByte4Normalized, static_cast<xiiUInt16>(offsetof(TexVertex, m_Color)), static_cast<xiiUInt16>(sizeof(TexVertex)));
        desc.AddStream(xiiMeshVertexSemantic::TexCoord0, xiiMeshVertexStreamFormat::Float2, static_cast<xiiUInt16>(offsetof(TexVertex, m_fTexCoord)), static_cast<xiiUInt16>(sizeof(TexVertex)));
        desc.AllocateStreams(TEX_TRIANGLE_VERTICES_PER_BATCH, 0U, false);
        sBaseName = "DebugDynamicTexturedTriangles"_xiisv;
        break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    xiiStringBuilder resourceName;
    resourceName.SetFormat("{}_{}", sBaseName, uiPageIndex);

    meshBufferPages[uiPageIndex] = xiiResourceManager::CreateResource<xiiMeshBufferResource>(resourceName.GetView(), std::move(desc), resourceName.GetView());
    return meshBufferPages[uiPageIndex];
  }

  template <typename AddFunc>
  static xiiUInt32 AddTextLines(const xiiDebugRendererContext& context, const xiiFormatString& text0, const xiiVec2I32& vPositionInPixel, xiiUInt32 uiSizeInPixel, xiiDebugTextHAlign::Enum horizontalAlignment, xiiDebugTextVAlign::Enum verticalAlignment, AddFunc func)
  {
    if (text0.IsEmpty())
      return 0;

    xiiStringBuilder tmp;
    xiiStringView    text = text0.GetText(tmp);

    xiiHybridArray<xiiStringView, 8> lines;
    xiiUInt32                        maxLineLength = 0;

    xiiHybridArray<xiiUInt32, 8> maxColumWidth;
    bool                         isTabular = false;

    xiiStringBuilder sb;
    if (text.FindSubString("\n"))
    {
      sb = text;
      sb.Split(true, lines, "\n");

      for (auto& line : lines)
      {
        xiiUInt32 uiColIdx = 0;

        const char* colPtrCur = line.GetStartPointer();

        while (const char* colPtrNext = line.FindSubString("\t", colPtrCur))
        {
          isTabular = true;

          const xiiUInt32 colLen = xiiMath::RoundUp(1 + static_cast<xiiUInt32>(colPtrNext - colPtrCur), 4);

          maxColumWidth.EnsureCount(uiColIdx + 1);
          maxColumWidth[uiColIdx] = xiiMath::Max(maxColumWidth[uiColIdx], colLen);

          colPtrCur = colPtrNext + 1;
          ++uiColIdx;
        }

        // length of the last column (that wasn't counted)
        maxLineLength = xiiMath::Max(maxLineLength, xiiStringUtils::GetStringElementCount(colPtrCur, line.GetEndPointer()));
      }

      for (xiiUInt32 columnWidth : maxColumWidth)
      {
        maxLineLength += columnWidth;
      }
    }
    else
    {
      lines.PushBack(text);
      maxLineLength = text.GetElementCount();
      maxColumWidth.PushBack(maxLineLength);
    }

    const float fGlyphWidth  = xiiDebugRenderer::GetTextGlyphWidth(uiSizeInPixel);
    const float fGlyphHeight = xiiMath::Ceil(uiSizeInPixel * cvar_DebugTextScale);
    const float fLineHeight  = xiiDebugRenderer::GetTextLineHeight(uiSizeInPixel);
    const float fLineSpacing = fLineHeight - fGlyphHeight;

    float screenPosX = (float)vPositionInPixel.x;
    if (horizontalAlignment == xiiDebugTextHAlign::Right)
      screenPosX -= maxLineLength * fGlyphWidth;

    float screenPosY = (float)vPositionInPixel.y;
    if (verticalAlignment == xiiDebugTextVAlign::Center)
      screenPosY -= xiiMath::Ceil(lines.GetCount() * fLineHeight * 0.5f) - fLineSpacing * 0.5f;
    else if (verticalAlignment == xiiDebugTextVAlign::Bottom)
      screenPosY -= lines.GetCount() * fLineHeight - fLineSpacing;

    {
      XII_LOCK(s_Mutex);

      auto& data = GetDataForExtraction(context);

      xiiVec2 currentPos(screenPosX, screenPosY);

      for (xiiStringView line : lines)
      {
        currentPos.x = screenPosX;
        if (horizontalAlignment == xiiDebugTextHAlign::Center)
          currentPos.x -= xiiMath::Ceil(line.GetElementCount() * fGlyphWidth * 0.5f);

        if (isTabular)
        {
          xiiUInt32 uiColIdx = 0;

          const char* colPtrCur = line.GetStartPointer();

          xiiUInt32 addWidth = 0;

          while (const char* colPtrNext = line.FindSubString("\t", colPtrCur))
          {
            const xiiVec2 tabOff(addWidth * fGlyphWidth, 0);
            func(data, xiiStringView(colPtrCur, colPtrNext), currentPos + tabOff);

            addWidth += maxColumWidth[uiColIdx];

            colPtrCur = colPtrNext + 1;
            ++uiColIdx;
          }

          // last column
          {
            const xiiVec2 tabOff(addWidth * fGlyphWidth, 0);
            func(data, xiiStringView(colPtrCur, line.GetEndPointer()), currentPos + tabOff);
          }
        }
        else
        {
          func(data, line, currentPos);
        }

        currentPos.y += fLineHeight;
      }
    }

    return lines.GetCount();
  }

  static void AppendGlyphs(xiiDynamicArray<GlyphData, xiiAlignedAllocatorWrapper>& ref_glyphs, const TextLineData2D& textLine)
  {
    xiiVec2     currentPos  = textLine.m_vTopLeftCorner;
    const float fGlyphWidth = xiiDebugRenderer::GetTextGlyphWidth(textLine.m_uiSizeInPixel);

    for (xiiUInt32 uiCharacter : textLine.m_sText)
    {
      auto& glyphData            = ref_glyphs.ExpandAndGetRef();
      glyphData.m_vTopLeftCorner = currentPos;
      glyphData.m_Color          = textLine.m_Color;
      glyphData.m_uiGlyphIndex   = uiCharacter < 128 ? static_cast<xiiUInt16>(uiCharacter) : 0;
      glyphData.m_uiSizeInPixel  = (xiiUInt16)xiiMath::Ceil(textLine.m_uiSizeInPixel * cvar_DebugTextScale);

      currentPos.x += fGlyphWidth;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Persistent Items

  struct PersistentCrossData
  {
    float    m_fSize;
    xiiColor m_Color;
    xiiMat4  m_Transform;
    xiiTime  m_Timeout;
  };

  struct PersistentSphereData
  {
    float    m_fRadius;
    xiiColor m_Color;
    xiiMat4  m_Transform;
    xiiTime  m_Timeout;
  };

  struct PersistentBoxData
  {
    xiiVec3  m_vHalfSize;
    xiiColor m_Color;
    xiiMat4  m_Transform;
    xiiTime  m_Timeout;
  };

  struct PersistentLineData
  {
    xiiHybridArray<xiiDebugRendererLine, 32> m_Lines;
    xiiColor                                 m_Color;
    xiiMat4                                  m_Transform;
    xiiTime                                  m_Timeout;
  };

  struct PersistentInfoTextData
  {
    xiiString                   m_sText;
    xiiDebugTextPlacement::Enum m_Placement;
    xiiColor                    m_Color;
    xiiTime                     m_Timeout;
  };

  struct PersistentPerContextData
  {
    xiiTime                          m_Now;
    xiiDeque<PersistentCrossData>    m_Crosses;
    xiiDeque<PersistentSphereData>   m_Spheres;
    xiiDeque<PersistentBoxData>      m_Boxes;
    xiiDeque<PersistentLineData>     m_Lines;
    xiiDeque<PersistentInfoTextData> m_InfoText;
  };

  static xiiHashTable<xiiDebugRendererContext, PersistentPerContextData> s_PersistentPerContextData;

} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, DebugRenderer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiDebugRenderer::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiDebugRenderer::OnEngineShutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

// static
void xiiDebugRenderer::DrawLines(const xiiDebugRendererContext& context, xiiArrayPtr<const xiiDebugRendererLine> lines, const xiiColor& color, xiiMatOrTransform mTransform /*= xiiMat4::MakeIdentity()*/)
{
  if (lines.IsEmpty())
    return;

  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    const xiiVec3*  pPositions = &line.m_vStart;
    const xiiColor* pColors    = &line.m_StartColor;

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex       = data.m_LineVertices.ExpandAndGetRef();
      vertex.m_vPosition = mTransform.m_Mat4.TransformPosition(pPositions[i]);
      vertex.m_Color     = pColors[i] * color;
    }
  }
}

void xiiDebugRenderer::Draw2DLines(const xiiDebugRendererContext& context, xiiArrayPtr<const xiiDebugRendererLine> lines, const xiiColor& color)
{
  if (lines.IsEmpty())
    return;

  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    const xiiVec3* pPositions = &line.m_vStart;

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex       = data.m_Line2DVertices.ExpandAndGetRef();
      vertex.m_vPosition = pPositions[i];
      vertex.m_Color     = color;
    }
  }
}

// static
void xiiDebugRenderer::DrawCross(const xiiDebugRendererContext& context, const xiiVec3& vGlobalPosition, float fLineLength, const xiiColor& color, xiiMatOrTransform mTransform /*= xiiMat4::MakeIdentity()*/)
{
  if (fLineLength <= 0.0f)
    return;

  const float   fHalfLineLength = fLineLength * 0.5f;
  const xiiVec3 xAxis           = xiiVec3::MakeAxisX() * fHalfLineLength;
  const xiiVec3 yAxis           = xiiVec3::MakeAxisY() * fHalfLineLength;
  const xiiVec3 zAxis           = xiiVec3::MakeAxisZ() * fHalfLineLength;

  const xiiMat4& transform = mTransform.m_Mat4;

  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_LineVertices.PushBack({transform.TransformPosition(vGlobalPosition - xAxis), color});
  data.m_LineVertices.PushBack({transform.TransformPosition(vGlobalPosition + xAxis), color});

  data.m_LineVertices.PushBack({transform.TransformPosition(vGlobalPosition - yAxis), color});
  data.m_LineVertices.PushBack({transform.TransformPosition(vGlobalPosition + yAxis), color});

  data.m_LineVertices.PushBack({transform.TransformPosition(vGlobalPosition - zAxis), color});
  data.m_LineVertices.PushBack({transform.TransformPosition(vGlobalPosition + zAxis), color});
}

// static
void xiiDebugRenderer::DrawLineBox(const xiiDebugRendererContext& context, const xiiBoundingBox& box, const xiiColor& color, xiiMatOrTransform mTransform)
{
  XII_LOCK(s_Mutex);

  const xiiMat4& transform = mTransform.m_Mat4;

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_LineBoxes.ExpandAndGetRef();

  xiiTransform boxTransform(box.GetCenter(), xiiQuat::MakeIdentity(), box.GetHalfExtents());

  boxData.m_Transform = transform * boxTransform.GetAsMat4();
  boxData.m_Color     = color;
}

// static
void xiiDebugRenderer::DrawLineBoxCorners(const xiiDebugRendererContext& context, const xiiBoundingBox& box, float fCornerFraction, const xiiColor& color, xiiMatOrTransform mTransform)
{
  const xiiMat4& transform = mTransform.m_Mat4;

  fCornerFraction = xiiMath::Clamp(fCornerFraction, 0.0f, 1.0f) * 0.5f;

  xiiVec3 corners[8];
  box.GetCorners(corners);

  for (xiiUInt32 i = 0; i < 8; ++i)
  {
    corners[i] = transform * corners[i];
  }

  xiiVec3 edgeEnds[12];
  edgeEnds[0]  = corners[1]; // 0 -> 1
  edgeEnds[1]  = corners[3]; // 1 -> 3
  edgeEnds[2]  = corners[0]; // 2 -> 0
  edgeEnds[3]  = corners[2]; // 3 -> 2
  edgeEnds[4]  = corners[5]; // 4 -> 5
  edgeEnds[5]  = corners[7]; // 5 -> 7
  edgeEnds[6]  = corners[4]; // 6 -> 4
  edgeEnds[7]  = corners[6]; // 7 -> 6
  edgeEnds[8]  = corners[4]; // 0 -> 4
  edgeEnds[9]  = corners[5]; // 1 -> 5
  edgeEnds[10] = corners[6]; // 2 -> 6
  edgeEnds[11] = corners[7]; // 3 -> 7

  xiiDebugRendererLine lines[24];
  for (xiiUInt32 i = 0; i < 12; ++i)
  {
    xiiVec3 edgeStart = corners[i % 8];
    xiiVec3 edgeEnd   = edgeEnds[i];
    xiiVec3 edgeDir   = edgeEnd - edgeStart;

    lines[i * 2 + 0].m_vStart = edgeStart;
    lines[i * 2 + 0].m_vEnd   = edgeStart + edgeDir * fCornerFraction;

    lines[i * 2 + 1].m_vStart = edgeEnd;
    lines[i * 2 + 1].m_vEnd   = edgeEnd - edgeDir * fCornerFraction;
  }

  DrawLines(context, lines, color);
}

// static
void xiiDebugRenderer::DrawLineSphere(const xiiDebugRendererContext& context, const xiiBoundingSphere& sphere, const xiiColor& color, xiiMatOrTransform mTransform /*= xiiMat4::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS = 32
  };

  const xiiVec3  vCenter   = sphere.m_vCenter;
  const float    fRadius   = sphere.m_fRadius;
  const xiiAngle stepAngle = xiiAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  const xiiMat4& transform = mTransform.m_Mat4;

  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (xiiUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = xiiMath::Cos(fS1 * stepAngle);
    const float fCos2 = xiiMath::Cos(fS2 * stepAngle);

    const float fSin1 = xiiMath::Sin(fS1 * stepAngle);
    const float fSin2 = xiiMath::Sin(fS2 * stepAngle);

    data.m_LineVertices.PushBack({transform * (vCenter + xiiVec3(0.0f, fCos1, fSin1) * fRadius), color});
    data.m_LineVertices.PushBack({transform * (vCenter + xiiVec3(0.0f, fCos2, fSin2) * fRadius), color});

    data.m_LineVertices.PushBack({transform * (vCenter + xiiVec3(fCos1, 0.0f, fSin1) * fRadius), color});
    data.m_LineVertices.PushBack({transform * (vCenter + xiiVec3(fCos2, 0.0f, fSin2) * fRadius), color});

    data.m_LineVertices.PushBack({transform * (vCenter + xiiVec3(fCos1, fSin1, 0.0f) * fRadius), color});
    data.m_LineVertices.PushBack({transform * (vCenter + xiiVec3(fCos2, fSin2, 0.0f) * fRadius), color});
  }
}


void xiiDebugRenderer::DrawLineCapsuleZ(const xiiDebugRendererContext& context, float fLength, float fRadius, const xiiColor& color, xiiMatOrTransform mTransform /*= xiiMat4::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS      = 32,
    NUM_HALF_SEGMENTS = 16,
    NUM_LINES         = NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + 4,
  };

  const xiiMat4& transform = mTransform.m_Mat4;

  const xiiAngle stepAngle = xiiAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  xiiDebugRendererLine lines[NUM_LINES];

  const float fOffsetZ = fLength * 0.5f;

  xiiUInt32 curLine = 0;

  // render 4 straight lines
  lines[curLine].m_vStart = transform * xiiVec3(-fRadius, 0, fOffsetZ);
  lines[curLine].m_vEnd   = transform * xiiVec3(-fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_vStart = transform * xiiVec3(+fRadius, 0, fOffsetZ);
  lines[curLine].m_vEnd   = transform * xiiVec3(+fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_vStart = transform * xiiVec3(0, -fRadius, fOffsetZ);
  lines[curLine].m_vEnd   = transform * xiiVec3(0, -fRadius, -fOffsetZ);
  ++curLine;

  lines[curLine].m_vStart = transform * xiiVec3(0, +fRadius, fOffsetZ);
  lines[curLine].m_vEnd   = transform * xiiVec3(0, +fRadius, -fOffsetZ);
  ++curLine;

  // render top and bottom circle
  for (xiiUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = xiiMath::Cos(fS1 * stepAngle);
    const float fCos2 = xiiMath::Cos(fS2 * stepAngle);

    const float fSin1 = xiiMath::Sin(fS1 * stepAngle);
    const float fSin2 = xiiMath::Sin(fS2 * stepAngle);

    lines[curLine].m_vStart = transform * xiiVec3(fCos1 * fRadius, fSin1 * fRadius, fOffsetZ);
    lines[curLine].m_vEnd   = transform * xiiVec3(fCos2 * fRadius, fSin2 * fRadius, fOffsetZ);
    ++curLine;

    lines[curLine].m_vStart = transform * xiiVec3(fCos1 * fRadius, fSin1 * fRadius, -fOffsetZ);
    lines[curLine].m_vEnd   = transform * xiiVec3(fCos2 * fRadius, fSin2 * fRadius, -fOffsetZ);
    ++curLine;
  }

  // render top and bottom half circles
  for (xiiUInt32 s = 0; s < NUM_HALF_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = xiiMath::Cos(fS1 * stepAngle);
    const float fCos2 = xiiMath::Cos(fS2 * stepAngle);

    const float fSin1 = xiiMath::Sin(fS1 * stepAngle);
    const float fSin2 = xiiMath::Sin(fS2 * stepAngle);

    // top two bows
    lines[curLine].m_vStart = transform * xiiVec3(0.0f, fCos1 * fRadius, fSin1 * fRadius + fOffsetZ);
    lines[curLine].m_vEnd   = transform * xiiVec3(0.0f, fCos2 * fRadius, fSin2 * fRadius + fOffsetZ);
    ++curLine;

    lines[curLine].m_vStart = transform * xiiVec3(fCos1 * fRadius, 0.0f, fSin1 * fRadius + fOffsetZ);
    lines[curLine].m_vEnd   = transform * xiiVec3(fCos2 * fRadius, 0.0f, fSin2 * fRadius + fOffsetZ);
    ++curLine;

    // bottom two bows
    lines[curLine].m_vStart = transform * xiiVec3(0.0f, fCos1 * fRadius, -fSin1 * fRadius - fOffsetZ);
    lines[curLine].m_vEnd   = transform * xiiVec3(0.0f, fCos2 * fRadius, -fSin2 * fRadius - fOffsetZ);
    ++curLine;

    lines[curLine].m_vStart = transform * xiiVec3(fCos1 * fRadius, 0.0f, -fSin1 * fRadius - fOffsetZ);
    lines[curLine].m_vEnd   = transform * xiiVec3(fCos2 * fRadius, 0.0f, -fSin2 * fRadius - fOffsetZ);
    ++curLine;
  }

  XII_ASSERT_DEBUG(curLine == NUM_LINES, "Invalid line count");
  DrawLines(context, lines, color);
}

void xiiDebugRenderer::DrawLineCylinderZ(const xiiDebugRendererContext& context, float fLength, float fRadius, const xiiColor& color, xiiMatOrTransform mTransform /*= xiiMat4::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS      = 32,
    NUM_HALF_SEGMENTS = 16,
    NUM_LINES         = NUM_SEGMENTS + NUM_SEGMENTS + 4,
  };

  const xiiMat4& transform = mTransform.m_Mat4;

  const xiiAngle stepAngle = xiiAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  xiiDebugRendererLine lines[NUM_LINES];

  const float fOffsetZ = fLength * 0.5f;

  xiiUInt32 curLine = 0;

  // render 4 straight lines
  lines[curLine].m_vStart = transform * xiiVec3(-fRadius, 0, fOffsetZ);
  lines[curLine].m_vEnd   = transform * xiiVec3(-fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_vStart = transform * xiiVec3(+fRadius, 0, fOffsetZ);
  lines[curLine].m_vEnd   = transform * xiiVec3(+fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_vStart = transform * xiiVec3(0, -fRadius, fOffsetZ);
  lines[curLine].m_vEnd   = transform * xiiVec3(0, -fRadius, -fOffsetZ);
  ++curLine;

  lines[curLine].m_vStart = transform * xiiVec3(0, +fRadius, fOffsetZ);
  lines[curLine].m_vEnd   = transform * xiiVec3(0, +fRadius, -fOffsetZ);
  ++curLine;

  // render top and bottom circle
  for (xiiUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = xiiMath::Cos(fS1 * stepAngle);
    const float fCos2 = xiiMath::Cos(fS2 * stepAngle);

    const float fSin1 = xiiMath::Sin(fS1 * stepAngle);
    const float fSin2 = xiiMath::Sin(fS2 * stepAngle);

    lines[curLine].m_vStart = transform * xiiVec3(fCos1 * fRadius, fSin1 * fRadius, fOffsetZ);
    lines[curLine].m_vEnd   = transform * xiiVec3(fCos2 * fRadius, fSin2 * fRadius, fOffsetZ);
    ++curLine;

    lines[curLine].m_vStart = transform * xiiVec3(fCos1 * fRadius, fSin1 * fRadius, -fOffsetZ);
    lines[curLine].m_vEnd   = transform * xiiVec3(fCos2 * fRadius, fSin2 * fRadius, -fOffsetZ);
    ++curLine;
  }

  XII_ASSERT_DEBUG(curLine == NUM_LINES, "Invalid line count");
  DrawLines(context, lines, color);
}

// static
void xiiDebugRenderer::DrawLineFrustum(const xiiDebugRendererContext& context, const xiiFrustum& frustum, const xiiColor& color, bool bDrawPlaneNormals /*= false*/)
{
  xiiVec3 cornerPoints[8];
  if (frustum.ComputeCornerPoints(cornerPoints).Failed())
    return;

  xiiDebugRendererLine lines[12] = {
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomLeft], cornerPoints[xiiFrustum::FrustumCorner::FarBottomLeft]),
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomRight], cornerPoints[xiiFrustum::FrustumCorner::FarBottomRight]),
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopLeft], cornerPoints[xiiFrustum::FrustumCorner::FarTopLeft]),
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopRight], cornerPoints[xiiFrustum::FrustumCorner::FarTopRight]),

    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomLeft], cornerPoints[xiiFrustum::FrustumCorner::NearBottomRight]),
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomRight], cornerPoints[xiiFrustum::FrustumCorner::NearTopRight]),
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopRight], cornerPoints[xiiFrustum::FrustumCorner::NearTopLeft]),
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopLeft], cornerPoints[xiiFrustum::FrustumCorner::NearBottomLeft]),

    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarBottomLeft], cornerPoints[xiiFrustum::FrustumCorner::FarBottomRight]),
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarBottomRight], cornerPoints[xiiFrustum::FrustumCorner::FarTopRight]),
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarTopRight], cornerPoints[xiiFrustum::FrustumCorner::FarTopLeft]),
    xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarTopLeft], cornerPoints[xiiFrustum::FrustumCorner::FarBottomLeft]),
  };

  DrawLines(context, lines, color);

  if (bDrawPlaneNormals)
  {
    xiiColor normalColor = color + xiiColor(0.4f, 0.4f, 0.4f);
    float    fDrawLength = 0.5f;

    const xiiVec3 nearPlaneNormal   = frustum.GetPlane(0).m_vNormal * fDrawLength;
    const xiiVec3 farPlaneNormal    = frustum.GetPlane(1).m_vNormal * fDrawLength;
    const xiiVec3 leftPlaneNormal   = frustum.GetPlane(2).m_vNormal * fDrawLength;
    const xiiVec3 rightPlaneNormal  = frustum.GetPlane(3).m_vNormal * fDrawLength;
    const xiiVec3 bottomPlaneNormal = frustum.GetPlane(4).m_vNormal * fDrawLength;
    const xiiVec3 topPlaneNormal    = frustum.GetPlane(5).m_vNormal * fDrawLength;

    xiiDebugRendererLine normalLines[24] = {
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomLeft], cornerPoints[xiiFrustum::FrustumCorner::NearBottomLeft] + nearPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomRight], cornerPoints[xiiFrustum::FrustumCorner::NearBottomRight] + nearPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopLeft], cornerPoints[xiiFrustum::FrustumCorner::NearTopLeft] + nearPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopRight], cornerPoints[xiiFrustum::FrustumCorner::NearTopRight] + nearPlaneNormal),

      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarBottomLeft], cornerPoints[xiiFrustum::FrustumCorner::FarBottomLeft] + farPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarBottomRight], cornerPoints[xiiFrustum::FrustumCorner::FarBottomRight] + farPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarTopLeft], cornerPoints[xiiFrustum::FrustumCorner::FarTopLeft] + farPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarTopRight], cornerPoints[xiiFrustum::FrustumCorner::FarTopRight] + farPlaneNormal),

      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomLeft], cornerPoints[xiiFrustum::FrustumCorner::NearBottomLeft] + leftPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopLeft], cornerPoints[xiiFrustum::FrustumCorner::NearTopLeft] + leftPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarBottomLeft], cornerPoints[xiiFrustum::FrustumCorner::FarBottomLeft] + leftPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarTopLeft], cornerPoints[xiiFrustum::FrustumCorner::FarTopLeft] + leftPlaneNormal),

      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomRight], cornerPoints[xiiFrustum::FrustumCorner::NearBottomRight] + rightPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopRight], cornerPoints[xiiFrustum::FrustumCorner::NearTopRight] + rightPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarBottomRight], cornerPoints[xiiFrustum::FrustumCorner::FarBottomRight] + rightPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarTopRight], cornerPoints[xiiFrustum::FrustumCorner::FarTopRight] + rightPlaneNormal),

      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomLeft], cornerPoints[xiiFrustum::FrustumCorner::NearBottomLeft] + bottomPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearBottomRight], cornerPoints[xiiFrustum::FrustumCorner::NearBottomRight] + bottomPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarBottomLeft], cornerPoints[xiiFrustum::FrustumCorner::FarBottomLeft] + bottomPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarBottomRight], cornerPoints[xiiFrustum::FrustumCorner::FarBottomRight] + bottomPlaneNormal),

      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopLeft], cornerPoints[xiiFrustum::FrustumCorner::NearTopLeft] + topPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::NearTopRight], cornerPoints[xiiFrustum::FrustumCorner::NearTopRight] + topPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarTopLeft], cornerPoints[xiiFrustum::FrustumCorner::FarTopLeft] + topPlaneNormal),
      xiiDebugRendererLine(cornerPoints[xiiFrustum::FrustumCorner::FarTopRight], cornerPoints[xiiFrustum::FrustumCorner::FarTopRight] + topPlaneNormal),
    };

    DrawLines(context, normalLines, normalColor);
  }
}

// static
void xiiDebugRenderer::DrawSolidBox(const xiiDebugRendererContext& context, const xiiBoundingBox& box, const xiiColor& color, xiiMatOrTransform mTransform)
{
  const xiiMat4& transform = mTransform.m_Mat4;

  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_SolidBoxes.ExpandAndGetRef();

  xiiTransform boxTransform(box.GetCenter(), xiiQuat::MakeIdentity(), box.GetHalfExtents());

  boxData.m_Transform = transform * boxTransform.GetAsMat4();
  boxData.m_Color     = color;
}

// static
void xiiDebugRenderer::DrawSolidTriangles(const xiiDebugRendererContext& context, xiiArrayPtr<xiiDebugRendererTriangle> triangles, const xiiColor& color)
{
  if (triangles.IsEmpty())
    return;

  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& triangle : triangles)
  {
    const xiiColorLinearUB col = triangle.m_Color * color;

    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      auto& vertex       = data.m_TriangleVertices.ExpandAndGetRef();
      vertex.m_vPosition = triangle.m_vPosition[i];
      vertex.m_Color     = col;
    }
  }
}

void xiiDebugRenderer::DrawTexturedTriangles(const xiiDebugRendererContext& context, xiiArrayPtr<xiiDebugRendererTexturedTriangle> triangles, const xiiColor& color, const xiiTexture2DResourceHandle& hTexture)
{
  if (triangles.IsEmpty())
    return;

  xiiResourceLock<xiiTexture2DResource> pTexture(hTexture, xiiResourceAcquireMode::AllowLoadingFallback);
  auto                                  pResourceView = pTexture->GetGALTexture()->GetDefaultView(xiiGALTextureViewType::ShaderResource);

  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context).m_TexturedTriangle3DVertices[pResourceView];

  for (auto& triangle : triangles)
  {
    const xiiColorLinearUB col = triangle.m_Color * color;

    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      auto& vertex       = data.ExpandAndGetRef();
      vertex.m_vPosition = triangle.m_vPosition[i];
      vertex.m_fTexCoord = triangle.m_vTexCoord[i];
      vertex.m_Color     = col;
    }
  }
}

void xiiDebugRenderer::Draw2DRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color)
{
  Vertex vertices[6];

  vertices[0].m_vPosition = xiiVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[1].m_vPosition = xiiVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[2].m_vPosition = xiiVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);
  vertices[3].m_vPosition = xiiVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[4].m_vPosition = xiiVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);
  vertices[5].m_vPosition = xiiVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(vertices); ++i)
  {
    vertices[i].m_Color = color;
  }

  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_Triangle2DVertices.PushBackRange(xiiMakeArrayPtr(vertices));
}

void xiiDebugRenderer::Draw2DRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color, const xiiTexture2DResourceHandle& hTexture, xiiVec2 vScale)
{
  xiiResourceLock<xiiTexture2DResource> pTexture(hTexture, xiiResourceAcquireMode::AllowLoadingFallback);
  Draw2DRectangle(context, rectInPixel, fDepth, color, pTexture->GetGALTexture()->GetDefaultView(xiiGALTextureViewType::ShaderResource), vScale);
}

void xiiDebugRenderer::Draw2DRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color, xiiSharedPtr<xiiGALTextureView> pTextureView, xiiVec2 vScale)
{
  TexVertex vertices[6];

  vertices[0].m_vPosition = xiiVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[0].m_fTexCoord = xiiVec2(0, 0).CompMul(vScale);
  vertices[1].m_vPosition = xiiVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[1].m_fTexCoord = xiiVec2(1, 1).CompMul(vScale);
  vertices[2].m_vPosition = xiiVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);
  vertices[2].m_fTexCoord = xiiVec2(0, 1).CompMul(vScale);
  vertices[3].m_vPosition = xiiVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[3].m_fTexCoord = xiiVec2(0, 0).CompMul(vScale);
  vertices[4].m_vPosition = xiiVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);
  vertices[4].m_fTexCoord = xiiVec2(1, 0).CompMul(vScale);
  vertices[5].m_vPosition = xiiVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[5].m_fTexCoord = xiiVec2(1, 1).CompMul(vScale);

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(vertices); ++i)
  {
    vertices[i].m_Color = color;
  }

  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_TexturedTriangle2DVertices[pTextureView].PushBackRange(xiiMakeArrayPtr(vertices));
}

void xiiDebugRenderer::Draw2DLineRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color)
{
  xiiDebugRendererLine lines[4];

  lines[0].m_vStart = xiiVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  lines[0].m_vEnd   = xiiVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);

  lines[1].m_vStart = lines[0].m_vEnd;
  lines[1].m_vEnd   = xiiVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);

  lines[2].m_vStart = lines[1].m_vEnd;
  lines[2].m_vEnd   = xiiVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);

  lines[3].m_vStart = lines[2].m_vEnd;
  lines[3].m_vEnd   = xiiVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);

  Draw2DLines(context, lines, color);
}

xiiUInt32 xiiDebugRenderer::Draw2DText(const xiiDebugRendererContext& context, const xiiFormatString& text, const xiiVec2I32& vPositionInPixel, const xiiColor& color, xiiUInt32 uiSizeInPixel /*= 16*/, xiiDebugTextHAlign::Enum horizontalAlignment /*= xiiDebugTextHAlign::Left*/, xiiDebugTextVAlign::Enum verticalAlignment /*= xiiDebugTextVAlign::Top*/)
{
  return AddTextLines(context, text, vPositionInPixel, uiSizeInPixel, horizontalAlignment, verticalAlignment, [=](PerContextData& ref_data, xiiStringView sLine, xiiVec2 vTopLeftCorner) {
    auto& textLine            = ref_data.m_sTextLines2D.ExpandAndGetRef();
    textLine.m_sText          = sLine;
    textLine.m_vTopLeftCorner = vTopLeftCorner;
    textLine.m_Color          = color;
    textLine.m_uiSizeInPixel  = uiSizeInPixel;
  });
}

void xiiDebugRenderer::DrawInfoText(const xiiDebugRendererContext& context, xiiDebugTextPlacement::Enum placement, xiiStringView sGroupName, const xiiFormatString& text, const xiiColor& color)
{
  XII_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  xiiStringBuilder tmp;

  auto& e    = data.m_InfoTextData[(xiiUInt8)placement].ExpandAndGetRef();
  e.m_sGroup = sGroupName;
  e.m_sText  = text.GetText(tmp);
  e.m_Color  = color;
}

void xiiDebugRenderer::AddPersistentInfoText(const xiiDebugRendererContext& context, xiiDebugTextPlacement::Enum placement, const xiiFormatString& text, xiiTime duration, const xiiColor& color)
{
  XII_LOCK(s_Mutex);

  xiiStringBuilder tmp;

  auto& data       = s_PersistentPerContextData[context];
  auto& item       = data.m_InfoText.ExpandAndGetRef();
  item.m_sText     = text.GetText(tmp);
  item.m_Placement = placement;
  item.m_Color     = color;
  item.m_Timeout   = data.m_Now + duration;
}

xiiUInt32 xiiDebugRenderer::Draw3DText(const xiiDebugRendererContext& context, const xiiFormatString& text, const xiiVec3& vGlobalPosition, const xiiColor& color, xiiUInt32 uiSizeInPixel /*= 16*/, xiiDebugTextHAlign::Enum horizontalAlignment /*= xiiDebugTextHAlign::Center*/, xiiDebugTextVAlign::Enum verticalAlignment /*= xiiDebugTextVAlign::Bottom*/)
{
  return AddTextLines(context, text, xiiVec2I32(0), uiSizeInPixel, horizontalAlignment, verticalAlignment, [&](PerContextData& ref_data, xiiStringView sLine, xiiVec2 vTopLeftCorner) {
    auto& textLine            = ref_data.m_sTextLines3D.ExpandAndGetRef();
    textLine.m_sText          = sLine;
    textLine.m_vTopLeftCorner = vTopLeftCorner;
    textLine.m_Color          = color;
    textLine.m_uiSizeInPixel  = uiSizeInPixel;
    textLine.m_vPosition      = vGlobalPosition;
  });
}

void xiiDebugRenderer::AddPersistentCross(const xiiDebugRendererContext& context, float fSize, const xiiColor& color, xiiMatOrTransform mTransform, xiiTime duration)
{
  XII_LOCK(s_Mutex);

  auto& data       = s_PersistentPerContextData[context];
  auto& item       = data.m_Crosses.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color     = color;
  item.m_fSize     = fSize;
  item.m_Timeout   = data.m_Now + duration;
}

void xiiDebugRenderer::AddPersistentLineSphere(const xiiDebugRendererContext& context, float fRadius, const xiiColor& color, xiiMatOrTransform mTransform, xiiTime duration)
{
  XII_LOCK(s_Mutex);

  auto& data       = s_PersistentPerContextData[context];
  auto& item       = data.m_Spheres.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color     = color;
  item.m_fRadius   = fRadius;
  item.m_Timeout   = data.m_Now + duration;
}

void xiiDebugRenderer::AddPersistentLineBox(const xiiDebugRendererContext& context, const xiiVec3& vHalfSize, const xiiColor& color, xiiMatOrTransform mTransform, xiiTime duration)
{
  XII_LOCK(s_Mutex);

  auto& data       = s_PersistentPerContextData[context];
  auto& item       = data.m_Boxes.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color     = color;
  item.m_vHalfSize = vHalfSize;
  item.m_Timeout   = data.m_Now + duration;
}

void xiiDebugRenderer::AddPersistentLines(const xiiDebugRendererContext& context, xiiArrayPtr<const xiiDebugRendererLine> lines, const xiiColor& color, xiiMatOrTransform mTransform, xiiTime duration)
{
  XII_LOCK(s_Mutex);

  auto& data       = s_PersistentPerContextData[context];
  auto& item       = data.m_Lines.ExpandAndGetRef();
  item.m_Transform = mTransform.m_Mat4;
  item.m_Color     = color;
  item.m_Lines     = lines;
  item.m_Timeout   = data.m_Now + duration;
}

void xiiDebugRenderer::DrawAngle(const xiiDebugRendererContext& context, xiiAngle startAngle, xiiAngle endAngle, const xiiColor& solidColor, const xiiColor& lineColor, xiiMatOrTransform mTransform, xiiVec3 vForwardAxis /*= xiiVec3::MakeAxisX()*/, xiiVec3 vRotationAxis /*= xiiVec3::MakeAxisZ()*/)
{
  const xiiMat4& transform = mTransform.m_Mat4;

  xiiHybridArray<xiiDebugRendererTriangle, 64> tris;
  xiiHybridArray<xiiDebugRendererLine, 64>     lines;

  startAngle.NormalizeRange();
  endAngle.NormalizeRange();

  if (startAngle > endAngle)
    startAngle -= xiiAngle::MakeFromDegree(360);

  const xiiAngle  range         = endAngle - startAngle;
  const xiiUInt32 uiTesselation = xiiMath::Max(1u, (xiiUInt32)(range / xiiAngle::MakeFromDegree(5)));
  const xiiAngle  step          = range / (float)uiTesselation;

  xiiQuat qStart = xiiQuat::MakeFromAxisAndAngle(vRotationAxis, startAngle);

  xiiQuat qStep = xiiQuat::MakeFromAxisAndAngle(vRotationAxis, step);

  xiiVec3 vCurDir = qStart * vForwardAxis;

  if (lineColor.a > 0)
  {
    xiiDebugRendererLine& l1 = lines.ExpandAndGetRef();
    l1.m_vStart.SetZero();
    l1.m_vEnd = vCurDir;
  }

  for (xiiUInt32 i = 0; i < uiTesselation; ++i)
  {
    const xiiVec3 vNextDir = qStep * vCurDir;

    if (solidColor.a > 0)
    {
      xiiDebugRendererTriangle& tri1 = tris.ExpandAndGetRef();
      tri1.m_vPosition[0]            = transform.GetTranslationVector();
      tri1.m_vPosition[1]            = transform.TransformPosition(vNextDir);
      tri1.m_vPosition[2]            = transform.TransformPosition(vCurDir);

      xiiDebugRendererTriangle& tri2 = tris.ExpandAndGetRef();
      tri2.m_vPosition[0]            = transform.GetTranslationVector();
      tri2.m_vPosition[1]            = transform.TransformPosition(vCurDir);
      tri2.m_vPosition[2]            = transform.TransformPosition(vNextDir);
    }

    if (lineColor.a > 0)
    {
      xiiDebugRendererLine& l1 = lines.ExpandAndGetRef();
      l1.m_vStart.SetZero();
      l1.m_vEnd = vNextDir;

      xiiDebugRendererLine& l2 = lines.ExpandAndGetRef();
      l2.m_vStart              = vCurDir;
      l2.m_vEnd                = vNextDir;
    }

    vCurDir = vNextDir;
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void xiiDebugRenderer::DrawOpeningCone(const xiiDebugRendererContext& context, xiiAngle halfAngle, const xiiColor& colorInside, const xiiColor& colorOutside, xiiMatOrTransform mTransform, xiiVec3 vForwardAxis /*= xiiVec3::MakeAxisX()*/)
{
  const xiiMat4& transform = mTransform.m_Mat4;

  xiiHybridArray<xiiDebugRendererTriangle, 64> trisInside;
  xiiHybridArray<xiiDebugRendererTriangle, 64> trisOutside;

  halfAngle = xiiMath::Clamp(halfAngle, xiiAngle(), xiiAngle::MakeFromDegree(180));

  const xiiAngle  refAngle      = halfAngle <= xiiAngle::MakeFromDegree(90) ? halfAngle : xiiAngle::MakeFromDegree(180) - halfAngle;
  const xiiUInt32 uiTesselation = xiiMath::Max(8u, (xiiUInt32)(refAngle / xiiAngle::MakeFromDegree(2)));

  const xiiVec3 tangentAxis = vForwardAxis.GetOrthogonalVector().GetNormalized();

  xiiQuat tilt = xiiQuat::MakeFromAxisAndAngle(tangentAxis, halfAngle);

  xiiQuat step = xiiQuat::MakeFromAxisAndAngle(vForwardAxis, xiiAngle::MakeFromDegree(360) / (float)uiTesselation);

  xiiVec3 vCurDir = tilt * vForwardAxis;

  for (xiiUInt32 i = 0; i < uiTesselation; ++i)
  {
    const xiiVec3 vNextDir = step * vCurDir;

    if (colorInside.a > 0)
    {
      xiiDebugRendererTriangle& tri = trisInside.ExpandAndGetRef();
      tri.m_vPosition[0]            = transform.GetTranslationVector();
      tri.m_vPosition[1]            = transform.TransformPosition(vCurDir);
      tri.m_vPosition[2]            = transform.TransformPosition(vNextDir);
    }

    if (colorOutside.a > 0)
    {
      xiiDebugRendererTriangle& tri = trisOutside.ExpandAndGetRef();
      tri.m_vPosition[0]            = transform.GetTranslationVector();
      tri.m_vPosition[1]            = transform.TransformPosition(vNextDir);
      tri.m_vPosition[2]            = transform.TransformPosition(vCurDir);
    }

    vCurDir = vNextDir;
  }


  DrawSolidTriangles(context, trisInside, colorInside);
  DrawSolidTriangles(context, trisOutside, colorOutside);
}

void xiiDebugRenderer::DrawLimitCone(const xiiDebugRendererContext& context, xiiAngle halfAngle1, xiiAngle halfAngle2, const xiiColor& solidColor, const xiiColor& lineColor, xiiMatOrTransform mTransform)
{
  const xiiMat4& transform = mTransform.m_Mat4;

  constexpr xiiUInt32 NUM_LINES = 32U;

  xiiHybridArray<xiiDebugRendererLine, NUM_LINES * 2>     lines;
  xiiHybridArray<xiiDebugRendererTriangle, NUM_LINES * 2> tris;

  // no clue how this works
  // copied 1:1 from NVIDIA's PhysX SDK: Cm::visualizeLimitCone
  {
    float scale = 1.0f;

    const float tanQSwingZ = xiiMath::Tan(halfAngle1 / 4.0f);
    const float tanQSwingY = xiiMath::Tan(halfAngle2 / 4.0f);

    xiiVec3 prev(0);
    for (xiiUInt32 i = 0; i <= NUM_LINES; i++)
    {
      const float   angle = 2 * xiiMath::Pi<float>() / (float)NUM_LINES * i;
      const float   c = xiiMath::Cos(xiiAngle::MakeFromRadian(angle)), s = xiiMath::Sin(xiiAngle::MakeFromRadian(angle));
      const xiiVec3 rv(0, -tanQSwingZ * s, tanQSwingY * c);
      const float   rv2 = rv.GetLengthSquared();
      const float   r   = (1 / (1 + rv2));
      const xiiQuat q   = xiiQuat(0, r * 2 * rv.y, r * 2 * rv.z, r * (1 - rv2));
      const xiiVec3 a   = q * xiiVec3(1.0f, 0, 0) * scale;

      if (lineColor.a > 0)
      {
        auto& l1    = lines.ExpandAndGetRef();
        l1.m_vStart = prev;
        l1.m_vEnd   = a;

        auto& l2 = lines.ExpandAndGetRef();
        l2.m_vStart.SetZero();
        l2.m_vEnd = a;
      }

      if (solidColor.a > 0)
      {
        auto& t1          = tris.ExpandAndGetRef();
        t1.m_vPosition[0] = transform.GetTranslationVector();
        t1.m_vPosition[1] = transform.TransformPosition(prev);
        t1.m_vPosition[2] = transform.TransformPosition(a);

        auto& t2          = tris.ExpandAndGetRef();
        t2.m_vPosition[0] = transform.GetTranslationVector();
        t2.m_vPosition[1] = transform.TransformPosition(a);
        t2.m_vPosition[2] = transform.TransformPosition(prev);
      }

      prev = a;
    }
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void xiiDebugRenderer::DrawCylinder(const xiiDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const xiiColor& solidColor, const xiiColor& lineColor, xiiMatOrTransform mTransform0, bool bCapStart /*= false*/, bool bCapEnd /*= false*/, xiiBasisAxis::Enum cylinderAxis /*= xiiBasisAxis::PositiveX*/)
{
  constexpr xiiUInt32 NUM_SEGMENTS = 16U;

  const xiiQuat tilt      = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveX, cylinderAxis);
  const xiiMat4 transform = mTransform0.m_Mat4 * tilt.GetAsMat4();

  xiiHybridArray<xiiDebugRendererLine, NUM_SEGMENTS * 3>         lines;
  xiiHybridArray<xiiDebugRendererTriangle, NUM_SEGMENTS * 2 * 2> tris;

  const xiiAngle step  = xiiAngle::MakeFromDegree(360) / (float)NUM_SEGMENTS;
  xiiAngle       angle = {};

  xiiVec3 vCurCircle(0, 1 /*xiiMath::Cos(angle)*/, 0 /*xiiMath::Sin(angle)*/);

  const bool bSolid = solidColor.a > 0;
  const bool bLine  = lineColor.a > 0;

  const xiiVec3 vLastCircle(0, xiiMath::Cos(-step), xiiMath::Sin(-step));
  const xiiVec3 vLastStart = transform.TransformPosition(xiiVec3(0, vLastCircle.y * fRadiusStart, vLastCircle.z * fRadiusStart));
  const xiiVec3 vLastEnd   = transform.TransformPosition(xiiVec3(fLength, vLastCircle.y * fRadiusEnd, vLastCircle.z * fRadiusEnd));

  for (xiiUInt32 i = 0; i < NUM_SEGMENTS; ++i)
  {
    angle += step;
    const xiiVec3 vNextCircle(0, xiiMath::Cos(angle), xiiMath::Sin(angle));

    xiiVec3 vCurStart  = vCurCircle * fRadiusStart;
    xiiVec3 vNextStart = vNextCircle * fRadiusStart;

    xiiVec3 vCurEnd(fLength, vCurCircle.y * fRadiusEnd, vCurCircle.z * fRadiusEnd);
    xiiVec3 vNextEnd(fLength, vNextCircle.y * fRadiusEnd, vNextCircle.z * fRadiusEnd);

    if (bLine)
    {
      lines.PushBack({vCurStart, vNextStart});
      lines.PushBack({vCurEnd, vNextEnd});
      lines.PushBack({vCurStart, vCurEnd});
    }

    if (bSolid)
    {
      vCurStart  = transform.TransformPosition(vCurStart);
      vCurEnd    = transform.TransformPosition(vCurEnd);
      vNextStart = transform.TransformPosition(vNextStart);
      vNextEnd   = transform.TransformPosition(vNextEnd);

      tris.PushBack({vCurStart, vNextStart, vNextEnd});
      tris.PushBack({vCurStart, vNextEnd, vCurEnd});

      if (bCapStart)
        tris.PushBack({vLastStart, vNextStart, vCurStart});

      if (bCapEnd)
        tris.PushBack({vLastEnd, vCurEnd, vNextEnd});
    }

    vCurCircle = vNextCircle;
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void xiiDebugRenderer::DrawArrow(const xiiDebugRendererContext& context, float fSize, const xiiColor& color, xiiMatOrTransform mTransform, xiiVec3 vForwardAxis /*= xiiVec3::MakeAxisX()*/)
{
  vForwardAxis.Normalize();
  const xiiVec3 right     = vForwardAxis.GetOrthogonalVector().GetNormalized();
  const xiiVec3 up        = vForwardAxis.CrossRH(right).GetNormalized();
  const xiiVec3 endPoint  = vForwardAxis * fSize;
  const xiiVec3 endPoint2 = vForwardAxis * fSize * 0.9f;
  const float   tipSize   = fSize * 0.1f;

  xiiDebugRendererLine lines[9];
  lines[0] = xiiDebugRendererLine(xiiVec3::MakeZero(), endPoint);
  lines[1] = xiiDebugRendererLine(endPoint, endPoint2 + right * tipSize);
  lines[2] = xiiDebugRendererLine(endPoint, endPoint2 + up * tipSize);
  lines[3] = xiiDebugRendererLine(endPoint, endPoint2 - right * tipSize);
  lines[4] = xiiDebugRendererLine(endPoint, endPoint2 - up * tipSize);
  lines[5] = xiiDebugRendererLine(lines[1].m_vEnd, lines[2].m_vEnd);
  lines[6] = xiiDebugRendererLine(lines[2].m_vEnd, lines[3].m_vEnd);
  lines[7] = xiiDebugRendererLine(lines[3].m_vEnd, lines[4].m_vEnd);
  lines[8] = xiiDebugRendererLine(lines[4].m_vEnd, lines[1].m_vEnd);

  DrawLines(context, lines, color, mTransform);
}

// static
float xiiDebugRenderer::GetTextGlyphWidth(xiiUInt32 uiSizeInPixel /*= 16*/)
{
  // Glyphs only use 8x10 pixels in their 16x16 pixel block, thus we don't advance by full size here.
  return xiiMath::Ceil(uiSizeInPixel * cvar_DebugTextScale * (8.0f / 16.0f));
}

// static
float xiiDebugRenderer::GetTextLineHeight(xiiUInt32 uiSizeInPixel /*= 16*/)
{
  return xiiMath::Ceil(uiSizeInPixel * cvar_DebugTextScale * (20.0f / 16.0f));
}

// static
float xiiDebugRenderer::GetTextScale()
{
  return cvar_DebugTextScale;
}

// static
void xiiDebugRenderer::SetTextScale(float fScale)
{
  cvar_DebugTextScale = fScale;
}

struct xiiDebugUploadData
{
  XII_DECLARE_POD_TYPE();

  xiiRGBufferHandle m_hSyncToken;
};

struct xiiDebugVisualizationData
{
  XII_DECLARE_POD_TYPE();

  xiiRGBufferHandle  m_hSyncToken;
  xiiRGTextureHandle m_hSceneColor;
  xiiRGTextureHandle m_hSceneDepth;
};

namespace
{
  struct alignas(16) DebugGlobalConstants
  {
    XII_DECLARE_POD_TYPE();

    xiiShaderMat4 m_CameraToScreenMatrix[2];
    xiiShaderMat4 m_ScreenToCameraMatrix[2];
    xiiShaderMat4 m_WorldToCameraMatrix[2];
    xiiShaderMat4 m_CameraToWorldMatrix[2];
    xiiShaderMat4 m_WorldToScreenMatrix[2];
    xiiShaderMat4 m_ScreenToWorldMatrix[2];

    xiiVec4   m_ViewportSize = xiiVec4(1.0f, 1.0f, 1.0f, 1.0f);
    xiiVec4   m_ClipPlanes   = xiiVec4(0.1f, 1000.0f, 0.001f, 0.0f);
    float     m_fMaxZValue   = 0.0f;
    float     m_fDeltaTime   = 0.0f;
    float     m_fGlobalTime  = 0.0f;
    float     m_fWorldTime   = 0.0f;
    float     m_fExposure    = 1.0f;
    xiiInt32  m_iRenderPass  = 0;
    xiiUInt32 m_uiPadding[2] = {};
  };

  struct DebugUploadAllocator
  {
    xiiUInt32 m_uiNextDynamicMeshPage[DynamicMeshBufferKind::Count] = {};
    xiiUInt32 m_uiNextDataBufferPage[BufferType::Count]             = {};
  };

  struct PreparedVertexBatch
  {
    XII_DECLARE_POD_TYPE();

    xiiGALBuffer* m_pVertexBuffer = nullptr;
    xiiUInt32     m_uiVertexCount = 0U;
  };

  struct PreparedTexturedVertexBatch
  {
    xiiGALBuffer*                   m_pVertexBuffer = nullptr;
    xiiUInt32                       m_uiVertexCount = 0U;
    bool                            m_bMonochrome   = false;
    xiiSharedPtr<xiiGALTextureView> m_pTextureView;
  };

  struct PreparedBoxBatch
  {
    XII_DECLARE_POD_TYPE();

    xiiGALBuffer*            m_pVertexBuffer   = nullptr;
    xiiGALBuffer*            m_pIndexBuffer    = nullptr;
    xiiGALBuffer*            m_pInstanceData   = nullptr;
    xiiUInt32                m_uiIndexCount    = 0U;
    xiiEnum<xiiGALValueType> m_IndexType       = xiiGALValueType::Undefined;
    xiiUInt32                m_uiInstanceCount = 0U;
  };

  struct PreparedGlyphBatch
  {
    XII_DECLARE_POD_TYPE();

    xiiGALBuffer* m_pGlyphDataBuffer = nullptr;
    xiiUInt32     m_uiGlyphCount     = 0U;
  };

  struct PreparedContextData
  {
    xiiDynamicArray<PreparedBoxBatch>            m_SolidBoxes;
    xiiDynamicArray<PreparedVertexBatch>         m_Triangles3D;
    xiiDynamicArray<PreparedTexturedVertexBatch> m_TexturedTriangles3D;
    xiiDynamicArray<PreparedVertexBatch>         m_Lines3D;
    xiiDynamicArray<PreparedBoxBatch>            m_LineBoxes;
    xiiDynamicArray<PreparedGlyphBatch>          m_Text3D;

    xiiDynamicArray<PreparedVertexBatch>         m_Triangles2D;
    xiiDynamicArray<PreparedTexturedVertexBatch> m_TexturedTriangles2D;
    xiiDynamicArray<PreparedVertexBatch>         m_Lines2D;
    xiiDynamicArray<PreparedGlyphBatch>          m_Text2D;
  };

  struct PreparedDebugViewData
  {
    PreparedContextData m_WorldContext;
    PreparedContextData m_ViewContext;
  };

  struct DebugTransitionCollector
  {
    xiiHybridArray<xiiGALStateTransitionDescription, 32>                 m_Transitions;
    xiiHashTable<xiiGALResource*, xiiBitflags<xiiGALResourceStateFlags>> m_LastRequestedState;
  };

  static xiiHashTable<xiiUInt32, PreparedDebugViewData> s_PreparedViewData;

  struct DebugUploadState
  {
    const xiiView&     m_View;
    xiiGALCommandList& m_CommandList;
    xiiUInt32          m_uiViewportWidth  = 1U;
    xiiUInt32          m_uiViewportHeight = 1U;
  };

  struct DebugDrawState
  {
    const xiiView&                 m_View;
    xiiGALCommandList&             m_CommandList;
    xiiSharedPtr<xiiGALRenderPass> m_pRenderPass;
    DebugCameraMode                m_CameraMode       = DebugCameraMode::Perspective;
    xiiUInt32                      m_uiEyeCount       = 1U;
    xiiUInt32                      m_uiViewportWidth  = 1U;
    xiiUInt32                      m_uiViewportHeight = 1U;
  };

  static const char* GetCameraModePermutationValue(DebugCameraMode cameraMode)
  {
    switch (cameraMode)
    {
      case DebugCameraMode::Stereo:
        return "CAMERA_MODE_STEREO";
      case DebugCameraMode::Orthographic:
        return "CAMERA_MODE_ORTHO";
      case DebugCameraMode::Perspective:
      default:
        return "CAMERA_MODE_PERSPECTIVE";
    }
  }

  static const char* GetTopologyPermutationValue(xiiGALPrimitiveTopology::Enum topology)
  {
    switch (topology)
    {
      case xiiGALPrimitiveTopology::LineList:
        return "TOPOLOGY_LINE_LIST";
      case xiiGALPrimitiveTopology::TriangleList:
      default:
        return "TOPOLOGY_TRIANGLE_LIST";
    }
  }

  static xiiUInt32 GetEyeCount(const xiiView& view)
  {
    const xiiCamera* pCamera = view.GetCamera();
    return pCamera != nullptr && pCamera->IsStereoscopic() ? 2U : 1U;
  }

  static DebugCameraMode GetCameraMode(const xiiView& view)
  {
    const xiiCamera* pCamera = view.GetCamera();
    if (pCamera != nullptr)
    {
      if (pCamera->IsStereoscopic())
        return DebugCameraMode::Stereo;

      if (pCamera->IsOrthographic())
        return DebugCameraMode::Orthographic;
    }

    return DebugCameraMode::Perspective;
  }

  static void EnsureGlobalConstantsBuffer()
  {
    if (s_pGlobalConstantsBuffer == nullptr)
    {
      s_pGlobalConstantsBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(xiiGALDevice::GetDefaultDevice(), sizeof(DebugGlobalConstants), "DebugRenderer Global Constants");
    }
  }

  static void UpdateGlobalConstants(const xiiView& view, xiiGALCommandList& commandList, xiiUInt32 uiViewportWidth, xiiUInt32 uiViewportHeight)
  {
    EnsureGlobalConstantsBuffer();

    if (s_pGlobalConstantsBuffer == nullptr)
      return;

    const xiiCamera* pCamera = view.GetCamera();
    const float      fNear   = pCamera != nullptr ? pCamera->GetNearPlane() : 0.1f;
    const float      fFar    = pCamera != nullptr ? pCamera->GetFarPlane() : 1000.0f;

    xiiGALMapHelper<DebugGlobalConstants> pConstants(commandList, s_pGlobalConstantsBuffer.Borrow(), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    DebugGlobalConstants&                 constants = *pConstants;

    for (xiiUInt32 uiEyeIndex = 0; uiEyeIndex < 2U; ++uiEyeIndex)
    {
      const xiiCameraEye eye = uiEyeIndex == 1U && pCamera != nullptr && pCamera->IsStereoscopic() ? xiiCameraEye::Right : xiiCameraEye::Left;

      constants.m_CameraToScreenMatrix[uiEyeIndex] = view.GetProjectionMatrix(eye);
      constants.m_ScreenToCameraMatrix[uiEyeIndex] = view.GetInverseProjectionMatrix(eye);
      constants.m_WorldToCameraMatrix[uiEyeIndex]  = view.GetViewMatrix(eye);
      constants.m_CameraToWorldMatrix[uiEyeIndex]  = view.GetInverseViewMatrix(eye);
      constants.m_WorldToScreenMatrix[uiEyeIndex]  = view.GetViewProjectionMatrix(eye);
      constants.m_ScreenToWorldMatrix[uiEyeIndex]  = view.GetInverseViewProjectionMatrix(eye);
    }

    const float fInvViewportWidth  = uiViewportWidth > 0U ? 1.0f / static_cast<float>(uiViewportWidth) : 0.0f;
    const float fInvViewportHeight = uiViewportHeight > 0U ? 1.0f / static_cast<float>(uiViewportHeight) : 0.0f;
    const float fInvFarPlane       = fFar > 0.0f ? 1.0f / fFar : 0.0f;

    constants.m_ViewportSize = xiiVec4(static_cast<float>(uiViewportWidth), static_cast<float>(uiViewportHeight), fInvViewportWidth, fInvViewportHeight);
    constants.m_ClipPlanes   = xiiVec4(fNear, fFar, fInvFarPlane, 0.0f);
    constants.m_fMaxZValue   = 0.0f;
    constants.m_fExposure    = 1.0f;
    constants.m_iRenderPass  = 0;

    if (const xiiClock* pClock = xiiClock::GetGlobalClock())
    {
      constants.m_fDeltaTime  = static_cast<float>(pClock->GetTimeDiff().GetSeconds());
      constants.m_fGlobalTime = static_cast<float>(pClock->GetAccumulatedTime().GetSeconds());
      constants.m_fWorldTime  = constants.m_fGlobalTime;
    }
  }

  static void BindGlobalConstants(xiiGALCommandList& commandList)
  {
    if (s_pGlobalConstantsBuffer != nullptr)
    {
      commandList.ResolveAndSetConstantBuffer("xiiGlobalConstants", s_pGlobalConstantsBuffer.Borrow(), xiiGALShaderType::Vertex | xiiGALShaderType::Pixel);
    }
  }

  static xiiSharedPtr<xiiGALInputLayout> CreateInputLayout(xiiGALShader& vertexShader, std::initializer_list<xiiGALLayoutElement> layoutElements)
  {
    xiiGALInputLayoutCreationDescription description;
    for (const xiiGALLayoutElement& element : layoutElements)
    {
      description.m_LayoutElements.PushBack(element);
    }

    return vertexShader.CreateInputLayout(description);
  }

  static xiiSharedPtr<xiiGALInputLayout> EnsurePositionOnlyInputLayout(const xiiShaderPermutationResource& permutation)
  {
    if (s_pPositionOnlyInputLayout == nullptr)
    {
      xiiGALShader* pVertexShader = permutation.GetGALShader(xiiGALShaderType::Vertex).Borrow();
      XII_ASSERT_DEV(pVertexShader != nullptr, "Debug geometry permutation is missing a vertex shader.");

      s_pPositionOnlyInputLayout = CreateInputLayout(*pVertexShader, {
                                                                       xiiGALLayoutElement(xiiGALInputLayoutSemantic::Position, 0U, xiiGALResourceFormat::RGB32Float, 0U, sizeof(xiiMeshPackedVertex), xiiGALInputElementFrequency::PerVertex, 1U),
                                                                     });
    }

    return s_pPositionOnlyInputLayout;
  }

  static xiiSharedPtr<xiiGALInputLayout> EnsureVertexInputLayout(const xiiShaderPermutationResource& permutation)
  {
    if (s_pVertexInputLayout == nullptr)
    {
      xiiGALShader* pVertexShader = permutation.GetGALShader(xiiGALShaderType::Vertex).Borrow();
      XII_ASSERT_DEV(pVertexShader != nullptr, "Debug primitive permutation is missing a vertex shader.");

      s_pVertexInputLayout = CreateInputLayout(*pVertexShader, {
                                                                 xiiGALLayoutElement(xiiGALInputLayoutSemantic::Position, 0U, xiiGALResourceFormat::RGB32Float, 0U, sizeof(Vertex), xiiGALInputElementFrequency::PerVertex, 1U),
                                                                 xiiGALLayoutElement(xiiGALInputLayoutSemantic::Color0, 0U, xiiGALResourceFormat::RGBA8UNormalized, 12U, sizeof(Vertex), xiiGALInputElementFrequency::PerVertex, 1U),
                                                               });
    }

    return s_pVertexInputLayout;
  }

  static xiiSharedPtr<xiiGALInputLayout> EnsureTextureVertexInputLayout(const xiiShaderPermutationResource& permutation)
  {
    if (s_pTexVertexInputLayout == nullptr)
    {
      xiiGALShader* pVertexShader = permutation.GetGALShader(xiiGALShaderType::Vertex).Borrow();
      XII_ASSERT_DEV(pVertexShader != nullptr, "Debug textured primitive permutation is missing a vertex shader.");

      s_pTexVertexInputLayout = CreateInputLayout(*pVertexShader, {
                                                                    xiiGALLayoutElement(xiiGALInputLayoutSemantic::Position, 0U, xiiGALResourceFormat::RGB32Float, 0U, sizeof(TexVertex), xiiGALInputElementFrequency::PerVertex, 1U),
                                                                    xiiGALLayoutElement(xiiGALInputLayoutSemantic::Color0, 0U, xiiGALResourceFormat::RGBA8UNormalized, 12U, sizeof(TexVertex), xiiGALInputElementFrequency::PerVertex, 1U),
                                                                    xiiGALLayoutElement(xiiGALInputLayoutSemantic::TexCoord0, 0U, xiiGALResourceFormat::RG32Float, 16U, sizeof(TexVertex), xiiGALInputElementFrequency::PerVertex, 1U),
                                                                  });
    }

    return s_pTexVertexInputLayout;
  }

  static xiiSharedPtr<xiiGALRenderPass> GetOrCreateRenderPass(const xiiGALTexture& sceneColor, const xiiGALTexture& sceneDepth)
  {
    const xiiGALTextureCreationDescription& colorDescription = sceneColor.GetDescription();
    const xiiGALTextureCreationDescription& depthDescription = sceneDepth.GetDescription();

    DebugRenderPassKey key;
    key.m_ColorFormat       = colorDescription.m_Format;
    key.m_DepthFormat       = depthDescription.m_Format;
    key.m_uiSampleCount     = static_cast<xiiUInt8>(xiiMath::Max(1U, colorDescription.m_uiSampleCount));
    key.m_uiArraySliceCount = static_cast<xiiUInt8>(xiiMath::Max(colorDescription.m_uiArraySizeOrDepth, depthDescription.m_uiArraySizeOrDepth));

    auto it = s_RenderPassCache.Find(key);
    if (it.IsValid())
      return it.Value();

    xiiGALRenderPassCreationDescription renderPassDescription;

    xiiGALRenderPassAttachmentDescription& colorAttachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
    colorAttachment.m_Format                               = colorDescription.m_Format;
    colorAttachment.m_uiSampleCount                        = static_cast<xiiUInt8>(xiiMath::Max(1U, colorDescription.m_uiSampleCount));
    colorAttachment.m_LoadOperation                        = xiiGALAttachmentLoadOperation::Load;
    colorAttachment.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;
    colorAttachment.m_InitialStateFlags                    = xiiGALResourceStateFlags::RenderTarget;
    colorAttachment.m_FinalStateFlags                      = xiiGALResourceStateFlags::RenderTarget;

    xiiGALRenderPassAttachmentDescription& depthAttachment = renderPassDescription.m_Attachments.ExpandAndGetRef();
    depthAttachment.m_Format                               = depthDescription.m_Format;
    depthAttachment.m_uiSampleCount                        = static_cast<xiiUInt8>(xiiMath::Max(1U, depthDescription.m_uiSampleCount));
    depthAttachment.m_LoadOperation                        = xiiGALAttachmentLoadOperation::Load;
    depthAttachment.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;
    depthAttachment.m_StencilLoadOperation                 = xiiGALAttachmentLoadOperation::Load;
    depthAttachment.m_StencilStoreOperation                = xiiGALAttachmentStoreOperation::Store;
    depthAttachment.m_InitialStateFlags                    = xiiGALResourceStateFlags::DepthWrite;
    depthAttachment.m_FinalStateFlags                      = xiiGALResourceStateFlags::DepthWrite;

    xiiGALSubPassDescription& subPass = renderPassDescription.m_SubPasses.ExpandAndGetRef();
    subPass.m_RenderTargetAttachments.PushBack({0U, xiiGALResourceStateFlags::RenderTarget});
    subPass.m_DepthStencilAttachment.PushBack({1U, xiiGALResourceStateFlags::DepthWrite});

    xiiSharedPtr<xiiGALRenderPass> pRenderPass = xiiGALDevice::GetDefaultDevice()->CreateRenderPass(renderPassDescription);
    XII_ASSERT_DEV(pRenderPass != nullptr, "Failed to create render pass for the debug renderer.");

    s_RenderPassCache.Insert(key, pRenderPass);
    return pRenderPass;
  }

  static xiiSharedPtr<xiiGALGraphicsPipelineState> GetOrCreatePipeline(const DebugDrawState& drawState, DebugPipelineKind pipelineKind, xiiGALPrimitiveTopology::Enum topology, bool bPreTransformed, bool bMonochrome)
  {
    DebugPipelineKey key;
    key.m_Kind            = pipelineKind;
    key.m_CameraMode      = drawState.m_CameraMode;
    key.m_Topology        = topology;
    key.m_bPreTransformed = bPreTransformed;
    key.m_bMonochrome     = bMonochrome;
    key.m_pRenderPass     = drawState.m_pRenderPass.Borrow();

    auto it = s_GraphicsPipelineCache.Find(key);
    if (it.IsValid())
      return it.Value();

    xiiShaderResourceHandle hShader;
    switch (pipelineKind)
    {
      case DebugPipelineKind::Geometry:
        hShader = s_hDebugGeometryShader;
        break;
      case DebugPipelineKind::Primitive:
        hShader = s_hDebugPrimitiveShader;
        break;
      case DebugPipelineKind::TexturedPrimitive:
        hShader = s_hDebugTexturedPrimitiveShader;
        break;
      case DebugPipelineKind::Text:
        hShader = s_hDebugTextShader;
        break;
    }

    xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
    xiiHashedString                                permutationName;
    xiiHashedString                                permutationValue;

    permutationName.Assign("CAMERA_MODE");
    permutationValue.Assign(GetCameraModePermutationValue(drawState.m_CameraMode));
    permutationVariables.Insert(permutationName, permutationValue);

    permutationName.Assign("TOPOLOGY");
    permutationValue.Assign(GetTopologyPermutationValue(topology));
    permutationVariables.Insert(permutationName, permutationValue);

    if (pipelineKind == DebugPipelineKind::Primitive || pipelineKind == DebugPipelineKind::TexturedPrimitive)
    {
      permutationName.Assign("PRE_TRANSFORMED_VERTICES");
      permutationValue.Assign(bPreTransformed ? "TRUE" : "FALSE");
      permutationVariables.Insert(permutationName, permutationValue);
    }

    if (pipelineKind == DebugPipelineKind::TexturedPrimitive)
    {
      permutationName.Assign("MONOCHROME");
      permutationValue.Assign(bMonochrome ? "TRUE" : "FALSE");
      permutationVariables.Insert(permutationName, permutationValue);
    }

    xiiShaderPermutationResourceHandle            hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, true);
    xiiResourceLock<xiiShaderPermutationResource> pPermutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    XII_ASSERT_DEV(pPermutation.IsValid(), "Failed to load the required debug shader permutation.");

    xiiGALGraphicsPipelineStateCreationDescription pipelineDescription;
    pipelineDescription.m_pPipelineResourceSignature                     = pPermutation->GetPipelineResourceSignature();
    pipelineDescription.m_pVertexShader                                  = pPermutation->GetGALShader(xiiGALShaderType::Vertex);
    pipelineDescription.m_pPixelShader                                   = pPermutation->GetGALShader(xiiGALShaderType::Pixel);
    pipelineDescription.m_GraphicsPipeline.m_pBlendState                 = pPermutation->GetBlendState();
    pipelineDescription.m_GraphicsPipeline.m_pDepthStencilState          = pPermutation->GetDepthStencilState();
    pipelineDescription.m_GraphicsPipeline.m_pRasterizerState            = pPermutation->GetRasterizerState();
    pipelineDescription.m_GraphicsPipeline.m_pRenderPass                 = drawState.m_pRenderPass;
    pipelineDescription.m_GraphicsPipeline.m_PrimitiveTopology           = topology;
    pipelineDescription.m_GraphicsPipeline.m_uiViewportCount             = 1U;
    pipelineDescription.m_GraphicsPipeline.m_uiSubpassIndex              = 0U;
    pipelineDescription.m_GraphicsPipeline.m_SampleDescription.m_uiCount = static_cast<xiiUInt8>(xiiMath::Max(xiiUInt8{1U}, drawState.m_pRenderPass->GetDescription().m_Attachments[0].m_uiSampleCount));

    switch (pipelineKind)
    {
      case DebugPipelineKind::Geometry:
        pipelineDescription.m_GraphicsPipeline.m_pInputLayout = EnsurePositionOnlyInputLayout(*pPermutation.GetPointer());
        break;
      case DebugPipelineKind::Primitive:
        pipelineDescription.m_GraphicsPipeline.m_pInputLayout = EnsureVertexInputLayout(*pPermutation.GetPointer());
        break;
      case DebugPipelineKind::TexturedPrimitive:
        pipelineDescription.m_GraphicsPipeline.m_pInputLayout = EnsureTextureVertexInputLayout(*pPermutation.GetPointer());
        break;
      case DebugPipelineKind::Text:
        pipelineDescription.m_GraphicsPipeline.m_pInputLayout.Clear();
        break;
    }

    xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline = xiiGALPipelineCache::GetPipeline(pipelineDescription);
    XII_ASSERT_DEV(pPipeline != nullptr, "Failed to create a graphics pipeline for the debug renderer.");

    s_GraphicsPipelineCache.Insert(key, pPipeline);
    return pPipeline;
  }

  static xiiUInt32 GetViewStorageKey(const xiiView& view)
  {
    return view.GetHandle().GetInternalID().m_Data;
  }

  static xiiMeshBufferResourceHandle AcquireDynamicMeshBufferPage(DebugUploadAllocator& allocator, DynamicMeshBufferKind::Enum kind)
  {
    return EnsureDynamicMeshBufferPage(kind, allocator.m_uiNextDynamicMeshPage[kind]++);
  }

  static xiiSharedPtr<xiiGALBuffer> AcquireDataBufferPage(DebugUploadAllocator& allocator, BufferType::Enum bufferType, xiiUInt32 uiStructSize)
  {
    return EnsureDataBufferPage(bufferType, allocator.m_uiNextDataBufferPage[bufferType]++, uiStructSize);
  }

  static void QueueTransition(DebugTransitionCollector& collector, xiiGALResource* pResource, xiiBitflags<xiiGALResourceStateFlags> newState)
  {
    if (pResource == nullptr)
      return;

    if (const auto* pLastState = collector.m_LastRequestedState.GetValue(pResource); pLastState != nullptr && *pLastState == newState)
      return;

    collector.m_LastRequestedState.Remove(pResource);
    collector.m_LastRequestedState.Insert(pResource, newState);

    xiiGALStateTransitionDescription& transition = collector.m_Transitions.ExpandAndGetRef();
    transition.m_pResource                       = pResource;
    transition.m_OldState                        = xiiGALResourceStateFlags::Unknown;
    transition.m_NewState                        = newState;
    transition.m_TransitionFlags                 = xiiGALStateTransitionFlags::UpdateState;
    transition.m_TransitionType                  = xiiGALStateTransitionType::Immediate;
    transition.m_uiMipLevelCount                 = XII_GAL_REMAINING_MIP_LEVELS;
    transition.m_uiArraySliceCount               = XII_GAL_REMAINING_ARRAY_SLICES;
  }

  static void FlushTransitions(xiiGALCommandList& commandList, DebugTransitionCollector& collector)
  {
    if (collector.m_Transitions.IsEmpty())
      return;

    commandList.TransitionResourceStates(collector.m_Transitions);
    collector.m_Transitions.Clear();
    collector.m_LastRequestedState.Clear();
  }

  static PerContextData* GetRenderingData(const xiiDebugRendererContext& context)
  {
    DoubleBufferedPerContextData* pDoubleBufferedContextData = nullptr;
    if (!s_PerContextData.TryGetValue(context, pDoubleBufferedContextData))
      return nullptr;

    return pDoubleBufferedContextData->m_pData[pDoubleBufferedContextData->m_uiCurrentDataIndex].Borrow();
  }

  static void AdvanceToNextExtractionBuffer(const xiiDebugRendererContext& context)
  {
    XII_LOCK(s_Mutex);

    DoubleBufferedPerContextData* pDoubleBufferedContextData = nullptr;
    if (!s_PerContextData.TryGetValue(context, pDoubleBufferedContextData) || pDoubleBufferedContextData == nullptr)
      return;

    const xiiUInt32 uiNextDataIndex = 1U - pDoubleBufferedContextData->m_uiCurrentDataIndex;

    xiiUniquePtr<PerContextData>& pNextData = pDoubleBufferedContextData->m_pData[uiNextDataIndex];
    if (pNextData == nullptr)
    {
      pNextData = XII_DEFAULT_NEW(PerContextData);
    }

    ClearPerContextData(*pNextData);
    pDoubleBufferedContextData->m_uiCurrentDataIndex = uiNextDataIndex;
  }

  static PerContextData* PrepareWorldSpaceData(const xiiDebugRendererContext& context)
  {
    {
      XII_LOCK(s_Mutex);

      auto& data = s_PersistentPerContextData[context];
      data.m_Now = xiiClock::GetGlobalClock() != nullptr ? xiiClock::GetGlobalClock()->GetLastUpdateTime() : xiiTime();

      xiiUInt32 uiNumItems = data.m_Crosses.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Crosses[i];
        if (data.m_Now > item.m_Timeout)
        {
          data.m_Crosses.RemoveAtAndSwap(i);
          --uiNumItems;
          continue;
        }

        xiiDebugRenderer::DrawCross(context, xiiVec3::MakeZero(), item.m_fSize, item.m_Color, item.m_Transform);
        ++i;
      }

      uiNumItems = data.m_Spheres.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Spheres[i];
        if (data.m_Now > item.m_Timeout)
        {
          data.m_Spheres.RemoveAtAndSwap(i);
          --uiNumItems;
          continue;
        }

        xiiDebugRenderer::DrawLineSphere(context, xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), item.m_fRadius), item.m_Color, item.m_Transform);
        ++i;
      }

      uiNumItems = data.m_Boxes.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Boxes[i];
        if (data.m_Now > item.m_Timeout)
        {
          data.m_Boxes.RemoveAtAndSwap(i);
          --uiNumItems;
          continue;
        }

        xiiDebugRenderer::DrawLineBox(context, xiiBoundingBox::MakeFromMinMax(-item.m_vHalfSize, item.m_vHalfSize), item.m_Color, item.m_Transform);
        ++i;
      }

      uiNumItems = data.m_Lines.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Lines[i];
        if (data.m_Now > item.m_Timeout)
        {
          data.m_Lines.RemoveAtAndSwap(i);
          --uiNumItems;
          continue;
        }

        xiiDebugRenderer::DrawLines(context, item.m_Lines.GetArrayPtr(), item.m_Color, item.m_Transform);
        ++i;
      }
    }

    return GetRenderingData(context);
  }

  static PerContextData* PrepareScreenSpaceData(const xiiDebugRendererContext& context, xiiUInt32 uiViewportWidth, xiiUInt32 uiViewportHeight)
  {
    {
      XII_LOCK(s_Mutex);

      auto& data = s_PersistentPerContextData[context];
      data.m_Now = xiiClock::GetGlobalClock() != nullptr ? xiiClock::GetGlobalClock()->GetLastUpdateTime() : xiiTime();

      xiiUInt32 uiNumItems = data.m_InfoText.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_InfoText[i];
        if (data.m_Now > item.m_Timeout)
        {
          data.m_InfoText.RemoveAtAndSwap(i);
          --uiNumItems;
          continue;
        }

        xiiDebugRenderer::DrawInfoText(context, item.m_Placement, "__Persistent", item.m_sText.GetView(), item.m_Color);
        ++i;
      }
    }

    PerContextData* pData = GetRenderingData(context);
    if (pData == nullptr)
      return nullptr;

    static_assert(static_cast<xiiUInt8>(xiiDebugTextPlacement::ENUM_COUNT) == 6U);

    constexpr xiiDebugTextHAlign::Enum horizontalAlignment[static_cast<xiiUInt8>(xiiDebugTextPlacement::ENUM_COUNT)] = {
      xiiDebugTextHAlign::Left,
      xiiDebugTextHAlign::Center,
      xiiDebugTextHAlign::Right,
      xiiDebugTextHAlign::Left,
      xiiDebugTextHAlign::Center,
      xiiDebugTextHAlign::Right,
    };

    constexpr xiiDebugTextVAlign::Enum verticalAlignment[static_cast<xiiUInt8>(xiiDebugTextPlacement::ENUM_COUNT)] = {
      xiiDebugTextVAlign::Top,
      xiiDebugTextVAlign::Top,
      xiiDebugTextVAlign::Top,
      xiiDebugTextVAlign::Bottom,
      xiiDebugTextVAlign::Bottom,
      xiiDebugTextVAlign::Bottom,
    };

    const xiiInt32 lineHeight                                                       = static_cast<xiiInt32>(xiiDebugRenderer::GetTextLineHeight());
    xiiVec2I32     anchor[static_cast<xiiUInt8>(xiiDebugTextPlacement::ENUM_COUNT)] = {
      xiiVec2I32(10, 10),
      xiiVec2I32(static_cast<xiiInt32>(uiViewportWidth / 2U), 10),
      xiiVec2I32(static_cast<xiiInt32>(uiViewportWidth) - 10, 10),
      xiiVec2I32(10, static_cast<xiiInt32>(uiViewportHeight) - 10),
      xiiVec2I32(static_cast<xiiInt32>(uiViewportWidth / 2U), static_cast<xiiInt32>(uiViewportHeight) - 10),
      xiiVec2I32(static_cast<xiiInt32>(uiViewportWidth) - 10, static_cast<xiiInt32>(uiViewportHeight) - 10),
    };

    for (xiiUInt32 uiCorner = 0; uiCorner < static_cast<xiiUInt32>(xiiDebugTextPlacement::ENUM_COUNT); ++uiCorner)
    {
      auto& cornerData = pData->m_InfoTextData[uiCorner];
      xiiSorting::InsertionSort(cornerData, [](const InfoTextData& lhs, const InfoTextData& rhs) -> bool { return lhs.m_sGroup < rhs.m_sGroup; });

      xiiVec2I32     currentPosition = anchor[uiCorner];
      const xiiInt32 offset          = verticalAlignment[uiCorner] == xiiDebugTextVAlign::Top ? lineHeight : -lineHeight;

      for (xiiUInt32 i = 0; i < cornerData.GetCount(); ++i)
      {
        if (i > 0U && cornerData[i - 1U].m_sGroup != cornerData[i].m_sGroup)
        {
          currentPosition.y += offset;
        }

        currentPosition.y += offset * xiiDebugRenderer::Draw2DText(context, cornerData[i].m_sText.GetData(), currentPosition, cornerData[i].m_Color, 16U, horizontalAlignment[uiCorner], verticalAlignment[uiCorner]);
      }
    }

    AdvanceToNextExtractionBuffer(context);
    return pData;
  }

  static void UploadVertexArray(const DebugUploadState& uploadState, DebugUploadAllocator& allocator, DebugTransitionCollector& transitions, xiiArrayPtr<const Vertex> vertices, xiiGALPrimitiveTopology::Enum topology, xiiDynamicArray<PreparedVertexBatch>& out_batches)
  {
    if (vertices.IsEmpty())
      return;

    const DynamicMeshBufferKind::Enum bufferKind  = topology == xiiGALPrimitiveTopology::LineList ? DynamicMeshBufferKind::Line : DynamicMeshBufferKind::Triangle;
    const xiiUInt32                   uiBatchSize = topology == xiiGALPrimitiveTopology::LineList ? LINE_VERTICES_PER_BATCH : TRIANGLE_VERTICES_PER_BATCH;

    for (xiiUInt32 uiOffset = 0; uiOffset < vertices.GetCount(); uiOffset += uiBatchSize)
    {
      const xiiUInt32                        uiBatchCount = xiiMath::Min(vertices.GetCount() - uiOffset, uiBatchSize);
      const xiiMeshBufferResourceHandle      hMeshBuffer  = AcquireDynamicMeshBufferPage(allocator, bufferKind);
      xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::BlockTillLoaded);
      if (!pMeshBuffer.IsValid() || pMeshBuffer->GetVertexBuffer() == nullptr)
        continue;

      xiiGALDeviceUtilities::MapAndUpdateBuffer(&uploadState.m_CommandList, pMeshBuffer->GetVertexBuffer(), 0U, xiiMakeArrayPtr(vertices.GetPtr() + uiOffset, uiBatchCount).ToByteArray()).AssertSuccess();

      PreparedVertexBatch& batch = out_batches.ExpandAndGetRef();
      batch.m_pVertexBuffer      = pMeshBuffer->GetVertexBuffer().Borrow();
      batch.m_uiVertexCount      = uiBatchCount;

      QueueTransition(transitions, batch.m_pVertexBuffer, xiiGALResourceStateFlags::VertexBuffer);
    }
  }

  static void UploadTexturedVertexBatches(const DebugUploadState& uploadState, DebugUploadAllocator& allocator, DebugTransitionCollector& transitions, const xiiMap<xiiSharedPtr<xiiGALTextureView>, xiiDynamicArray<TexVertex, xiiAlignedAllocatorWrapper>>& vertexGroups, xiiDynamicArray<PreparedTexturedVertexBatch>& out_batches)
  {
    for (auto it = vertexGroups.GetIterator(); it.IsValid(); ++it)
    {
      const xiiSharedPtr<xiiGALTextureView>& pTextureView = it.Key();
      const auto&                            vertices     = it.Value();
      if (pTextureView == nullptr || vertices.IsEmpty())
        continue;

      const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(pTextureView->GetTexture()->GetDescription().m_Format);

      for (xiiUInt32 uiOffset = 0; uiOffset < vertices.GetCount(); uiOffset += TEX_TRIANGLE_VERTICES_PER_BATCH)
      {
        const xiiUInt32                        uiBatchCount = xiiMath::Min(vertices.GetCount() - uiOffset, xiiUInt32{TEX_TRIANGLE_VERTICES_PER_BATCH});
        const xiiMeshBufferResourceHandle      hMeshBuffer  = AcquireDynamicMeshBufferPage(allocator, DynamicMeshBufferKind::TexturedTriangle);
        xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::BlockTillLoaded);
        if (!pMeshBuffer.IsValid() || pMeshBuffer->GetVertexBuffer() == nullptr)
          continue;

        xiiGALDeviceUtilities::MapAndUpdateBuffer(&uploadState.m_CommandList, pMeshBuffer->GetVertexBuffer(), 0U, xiiMakeArrayPtr(vertices.GetData() + uiOffset, uiBatchCount).ToByteArray()).AssertSuccess();

        PreparedTexturedVertexBatch& batch = out_batches.ExpandAndGetRef();
        batch.m_pVertexBuffer              = pMeshBuffer->GetVertexBuffer().Borrow();
        batch.m_uiVertexCount              = uiBatchCount;
        batch.m_bMonochrome                = formatProperties.m_uiComponentCount == 1U;
        batch.m_pTextureView               = pTextureView;

        QueueTransition(transitions, batch.m_pVertexBuffer, xiiGALResourceStateFlags::VertexBuffer);

        const xiiSharedPtr<xiiGALTexture> pTexture = pTextureView->GetTexture();
        QueueTransition(transitions, pTexture.Borrow(), xiiGALResourceStateFlags::ShaderResource);
      }
    }
  }

  static void UploadBoxInstances(const DebugUploadState& uploadState, DebugUploadAllocator& allocator, DebugTransitionCollector& transitions, xiiArrayPtr<const BoxData> boxes, xiiGALPrimitiveTopology::Enum topology, xiiDynamicArray<PreparedBoxBatch>& out_batches)
  {
    if (boxes.IsEmpty())
      return;

    const BufferType::Enum                 bufferType  = topology == xiiGALPrimitiveTopology::LineList ? BufferType::LineBoxes : BufferType::SolidBoxes;
    const xiiMeshBufferResourceHandle      hMeshBuffer = topology == xiiGALPrimitiveTopology::LineList ? s_hLineBoxMeshBuffer : s_hSolidBoxMeshBuffer;
    xiiResourceLock<xiiMeshBufferResource> pMeshBuffer(hMeshBuffer, xiiResourceAcquireMode::BlockTillLoaded);
    if (!pMeshBuffer.IsValid() || pMeshBuffer->GetVertexBuffer() == nullptr || pMeshBuffer->GetIndexBuffer() == nullptr)
      return;

    xiiGALBuffer* pVertexBuffer = pMeshBuffer->GetVertexBuffer().Borrow();
    xiiGALBuffer* pIndexBuffer  = pMeshBuffer->GetIndexBuffer().Borrow();

    for (xiiUInt32 uiOffset = 0; uiOffset < boxes.GetCount(); uiOffset += BOXES_PER_BATCH)
    {
      const xiiUInt32            uiBatchCount  = xiiMath::Min(boxes.GetCount() - uiOffset, xiiUInt32{BOXES_PER_BATCH});
      xiiSharedPtr<xiiGALBuffer> pInstanceData = AcquireDataBufferPage(allocator, bufferType, sizeof(BoxData));
      xiiGALDeviceUtilities::MapAndUpdateBuffer(&uploadState.m_CommandList, pInstanceData, 0U, xiiMakeArrayPtr(boxes.GetPtr() + uiOffset, uiBatchCount).ToByteArray()).AssertSuccess();

      PreparedBoxBatch& batch = out_batches.ExpandAndGetRef();
      batch.m_pVertexBuffer   = pVertexBuffer;
      batch.m_pIndexBuffer    = pIndexBuffer;
      batch.m_pInstanceData   = pInstanceData.Borrow();
      batch.m_uiIndexCount    = pMeshBuffer->GetIndexCount();
      batch.m_IndexType       = pMeshBuffer->GetIndexType();
      batch.m_uiInstanceCount = uiBatchCount;

      QueueTransition(transitions, pVertexBuffer, xiiGALResourceStateFlags::VertexBuffer);
      QueueTransition(transitions, pIndexBuffer, xiiGALResourceStateFlags::IndexBuffer);
      QueueTransition(transitions, batch.m_pInstanceData, xiiGALResourceStateFlags::ShaderResource);
    }
  }

  static void UploadGlyphs(const DebugUploadState& uploadState, DebugUploadAllocator& allocator, DebugTransitionCollector& transitions, xiiArrayPtr<const GlyphData> glyphs, xiiDynamicArray<PreparedGlyphBatch>& out_batches)
  {
    if (glyphs.IsEmpty())
      return;

    xiiResourceLock<xiiTexture2DResource> pFontTexture(s_hDebugFontTexture, xiiResourceAcquireMode::BlockTillLoaded);
    if (!pFontTexture.IsValid() || pFontTexture->GetGALTexture() == nullptr)
      return;

    const xiiSharedPtr<xiiGALTexture> pFontGALTexture = pFontTexture->GetGALTexture();
    QueueTransition(transitions, pFontGALTexture.Borrow(), xiiGALResourceStateFlags::ShaderResource);

    for (xiiUInt32 uiOffset = 0; uiOffset < glyphs.GetCount(); uiOffset += GLYPHS_PER_BATCH)
    {
      const xiiUInt32            uiBatchCount = xiiMath::Min(glyphs.GetCount() - uiOffset, xiiUInt32{GLYPHS_PER_BATCH});
      xiiSharedPtr<xiiGALBuffer> pGlyphData   = AcquireDataBufferPage(allocator, BufferType::Glyphs, sizeof(GlyphData));
      xiiGALDeviceUtilities::MapAndUpdateBuffer(&uploadState.m_CommandList, pGlyphData, 0U, xiiMakeArrayPtr(glyphs.GetPtr() + uiOffset, uiBatchCount).ToByteArray()).AssertSuccess();

      PreparedGlyphBatch& batch = out_batches.ExpandAndGetRef();
      batch.m_pGlyphDataBuffer  = pGlyphData.Borrow();
      batch.m_uiGlyphCount      = uiBatchCount;

      QueueTransition(transitions, batch.m_pGlyphDataBuffer, xiiGALResourceStateFlags::ShaderResource);
    }
  }

  static void Upload3DText(const DebugUploadState& uploadState, DebugUploadAllocator& allocator, DebugTransitionCollector& transitions, const PerContextData& data, xiiDynamicArray<PreparedGlyphBatch>& out_batches)
  {
    xiiDynamicArray<GlyphData, xiiAlignedAllocatorWrapper> glyphs;

    for (const TextLineData3D& textLine : data.m_sTextLines3D)
    {
      xiiVec3 screenPosition;
      if (uploadState.m_View.ComputeScreenSpacePos(textLine.m_vPosition, screenPosition).Failed() || screenPosition.z <= 0.0f)
        continue;

      TextLineData2D projectedLine;
      projectedLine.m_sText          = textLine.m_sText;
      projectedLine.m_vTopLeftCorner = textLine.m_vTopLeftCorner + xiiVec2(xiiMath::Round(screenPosition.x * uploadState.m_uiViewportWidth), xiiMath::Round(screenPosition.y * uploadState.m_uiViewportHeight));
      projectedLine.m_Color          = textLine.m_Color;
      projectedLine.m_uiSizeInPixel  = textLine.m_uiSizeInPixel;

      AppendGlyphs(glyphs, projectedLine);
    }

    UploadGlyphs(uploadState, allocator, transitions, glyphs, out_batches);
  }

  static void Upload2DText(const DebugUploadState& uploadState, DebugUploadAllocator& allocator, DebugTransitionCollector& transitions, const PerContextData& data, xiiDynamicArray<PreparedGlyphBatch>& out_batches)
  {
    xiiDynamicArray<GlyphData, xiiAlignedAllocatorWrapper> glyphs;

    for (const TextLineData2D& textLine : data.m_sTextLines2D)
    {
      AppendGlyphs(glyphs, textLine);
    }

    UploadGlyphs(uploadState, allocator, transitions, glyphs, out_batches);
  }

  static void UploadContextData(const xiiDebugRendererContext& context, const DebugUploadState& uploadState, DebugUploadAllocator& allocator, DebugTransitionCollector& transitions, PreparedContextData& out_preparedData)
  {
    if (const PerContextData* pWorldSpaceData = PrepareWorldSpaceData(context); pWorldSpaceData != nullptr)
    {
      UploadBoxInstances(uploadState, allocator, transitions, pWorldSpaceData->m_SolidBoxes, xiiGALPrimitiveTopology::TriangleList, out_preparedData.m_SolidBoxes);
      UploadVertexArray(uploadState, allocator, transitions, pWorldSpaceData->m_TriangleVertices, xiiGALPrimitiveTopology::TriangleList, out_preparedData.m_Triangles3D);
      UploadTexturedVertexBatches(uploadState, allocator, transitions, pWorldSpaceData->m_TexturedTriangle3DVertices, out_preparedData.m_TexturedTriangles3D);
      UploadVertexArray(uploadState, allocator, transitions, pWorldSpaceData->m_LineVertices, xiiGALPrimitiveTopology::LineList, out_preparedData.m_Lines3D);
      UploadBoxInstances(uploadState, allocator, transitions, pWorldSpaceData->m_LineBoxes, xiiGALPrimitiveTopology::LineList, out_preparedData.m_LineBoxes);
      Upload3DText(uploadState, allocator, transitions, *pWorldSpaceData, out_preparedData.m_Text3D);
    }

    if (const PerContextData* pScreenSpaceData = PrepareScreenSpaceData(context, uploadState.m_uiViewportWidth, uploadState.m_uiViewportHeight); pScreenSpaceData != nullptr)
    {
      UploadVertexArray(uploadState, allocator, transitions, pScreenSpaceData->m_Triangle2DVertices, xiiGALPrimitiveTopology::TriangleList, out_preparedData.m_Triangles2D);
      UploadTexturedVertexBatches(uploadState, allocator, transitions, pScreenSpaceData->m_TexturedTriangle2DVertices, out_preparedData.m_TexturedTriangles2D);
      UploadVertexArray(uploadState, allocator, transitions, pScreenSpaceData->m_Line2DVertices, xiiGALPrimitiveTopology::LineList, out_preparedData.m_Lines2D);
      Upload2DText(uploadState, allocator, transitions, *pScreenSpaceData, out_preparedData.m_Text2D);
    }
  }

  static void DrawVertexBatches(const DebugDrawState& drawState, xiiArrayPtr<const PreparedVertexBatch> batches, xiiGALPrimitiveTopology::Enum topology, bool bPreTransformed)
  {
    if (batches.IsEmpty())
      return;

    xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline = GetOrCreatePipeline(drawState, DebugPipelineKind::Primitive, topology, bPreTransformed, false);

    for (const PreparedVertexBatch& batch : batches)
    {
      if (batch.m_pVertexBuffer == nullptr || batch.m_uiVertexCount == 0U)
        continue;

      drawState.m_CommandList.SetPipelineState(pPipeline);
      BindGlobalConstants(drawState.m_CommandList);

      xiiGALBuffer* pVertexBuffers[] = {batch.m_pVertexBuffer};
      drawState.m_CommandList.SetVertexBuffers(0U, xiiMakeArrayPtr(pVertexBuffers), xiiArrayPtr<xiiUInt64>(), xiiGALSetVertexBufferFlags::Reset, xiiGALStateTransitionMode::None);
      drawState.m_CommandList.CommitShaderResources(xiiGALStateTransitionMode::None).IgnoreResult();
      drawState.m_CommandList.Draw({batch.m_uiVertexCount, drawState.m_uiEyeCount});
    }
  }

  static void DrawTexturedVertexBatches(const DebugDrawState& drawState, xiiArrayPtr<const PreparedTexturedVertexBatch> batches, bool bPreTransformed)
  {
    for (const PreparedTexturedVertexBatch& batch : batches)
    {
      if (batch.m_pVertexBuffer == nullptr || batch.m_uiVertexCount == 0U || batch.m_pTextureView == nullptr)
        continue;

      xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline = GetOrCreatePipeline(drawState, DebugPipelineKind::TexturedPrimitive, xiiGALPrimitiveTopology::TriangleList, bPreTransformed, batch.m_bMonochrome);

      drawState.m_CommandList.SetPipelineState(pPipeline);
      BindGlobalConstants(drawState.m_CommandList);
      drawState.m_CommandList.ResolveAndSetShaderResourceTextureView("BaseTexture", batch.m_pTextureView.Borrow(), xiiGALShaderType::Pixel);

      xiiGALBuffer* pVertexBuffers[] = {batch.m_pVertexBuffer};
      drawState.m_CommandList.SetVertexBuffers(0U, xiiMakeArrayPtr(pVertexBuffers), xiiArrayPtr<xiiUInt64>(), xiiGALSetVertexBufferFlags::Reset, xiiGALStateTransitionMode::None);
      drawState.m_CommandList.CommitShaderResources(xiiGALStateTransitionMode::None).IgnoreResult();
      drawState.m_CommandList.Draw({batch.m_uiVertexCount, drawState.m_uiEyeCount});
    }
  }

  static void DrawBoxBatches(const DebugDrawState& drawState, xiiArrayPtr<const PreparedBoxBatch> batches, xiiGALPrimitiveTopology::Enum topology)
  {
    if (batches.IsEmpty())
      return;

    xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline = GetOrCreatePipeline(drawState, DebugPipelineKind::Geometry, topology, false, false);

    for (const PreparedBoxBatch& batch : batches)
    {
      if (batch.m_pVertexBuffer == nullptr || batch.m_pIndexBuffer == nullptr || batch.m_pInstanceData == nullptr || batch.m_uiInstanceCount == 0U)
        continue;

      drawState.m_CommandList.SetPipelineState(pPipeline);
      BindGlobalConstants(drawState.m_CommandList);
      drawState.m_CommandList.ResolveAndSetShaderResourceBufferView("boxData", batch.m_pInstanceData->GetDefaultView(xiiGALBufferViewType::ShaderResource).Borrow(), xiiGALShaderType::Vertex);

      xiiGALBuffer* pVertexBuffers[] = {batch.m_pVertexBuffer};
      drawState.m_CommandList.SetVertexBuffers(0U, xiiMakeArrayPtr(pVertexBuffers), xiiArrayPtr<xiiUInt64>(), xiiGALSetVertexBufferFlags::Reset, xiiGALStateTransitionMode::None);
      drawState.m_CommandList.SetIndexBuffer(batch.m_pIndexBuffer, 0U, xiiGALStateTransitionMode::None);
      drawState.m_CommandList.CommitShaderResources(xiiGALStateTransitionMode::None).IgnoreResult();
      drawState.m_CommandList.DrawIndexed({batch.m_uiIndexCount, batch.m_IndexType, batch.m_uiInstanceCount * drawState.m_uiEyeCount});
    }
  }

  static void DrawGlyphBatches(const DebugDrawState& drawState, xiiArrayPtr<const PreparedGlyphBatch> batches)
  {
    if (batches.IsEmpty())
      return;

    xiiResourceLock<xiiTexture2DResource> pFontTexture(s_hDebugFontTexture, xiiResourceAcquireMode::BlockTillLoaded);
    if (!pFontTexture.IsValid() || pFontTexture->GetGALTexture() == nullptr)
      return;

    xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline        = GetOrCreatePipeline(drawState, DebugPipelineKind::Text, xiiGALPrimitiveTopology::TriangleList, true, false);
    xiiGALTextureView*                        pFontTextureView = pFontTexture->GetGALTexture()->GetDefaultView(xiiGALTextureViewType::ShaderResource).Borrow();

    for (const PreparedGlyphBatch& batch : batches)
    {
      if (batch.m_pGlyphDataBuffer == nullptr || batch.m_uiGlyphCount == 0U)
        continue;

      drawState.m_CommandList.SetPipelineState(pPipeline);
      BindGlobalConstants(drawState.m_CommandList);
      drawState.m_CommandList.ResolveAndSetShaderResourceBufferView("glyphData", batch.m_pGlyphDataBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource).Borrow(), xiiGALShaderType::Vertex);
      drawState.m_CommandList.ResolveAndSetShaderResourceTextureView("FontTexture", pFontTextureView, xiiGALShaderType::Pixel);
      drawState.m_CommandList.CommitShaderResources(xiiGALStateTransitionMode::None).IgnoreResult();
      drawState.m_CommandList.Draw({batch.m_uiGlyphCount * 6U, drawState.m_uiEyeCount});
    }
  }

  static void DrawWorldSpaceContext(const DebugDrawState& drawState, const PreparedContextData& preparedData)
  {
    DrawBoxBatches(drawState, preparedData.m_SolidBoxes, xiiGALPrimitiveTopology::TriangleList);
    DrawVertexBatches(drawState, preparedData.m_Triangles3D, xiiGALPrimitiveTopology::TriangleList, false);
    DrawTexturedVertexBatches(drawState, preparedData.m_TexturedTriangles3D, false);
    DrawVertexBatches(drawState, preparedData.m_Lines3D, xiiGALPrimitiveTopology::LineList, false);
    DrawBoxBatches(drawState, preparedData.m_LineBoxes, xiiGALPrimitiveTopology::LineList);
    DrawGlyphBatches(drawState, preparedData.m_Text3D);
  }

  static void DrawScreenSpaceContext(const DebugDrawState& drawState, const PreparedContextData& preparedData)
  {
    DrawVertexBatches(drawState, preparedData.m_Triangles2D, xiiGALPrimitiveTopology::TriangleList, true);
    DrawTexturedVertexBatches(drawState, preparedData.m_TexturedTriangles2D, true);
    DrawVertexBatches(drawState, preparedData.m_Lines2D, xiiGALPrimitiveTopology::LineList, true);
    DrawGlyphBatches(drawState, preparedData.m_Text2D);
  }
} // namespace

// static
void xiiDebugRenderer::SetupDebugUpload(xiiDebugUploadData& data, xiiRGBuilder& builder)
{
  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = sizeof(xiiUInt32);
  description.m_uiSize              = sizeof(xiiUInt32);
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  description.m_Usage               = xiiGALResourceUsage::Mutable;

  data.m_hSyncToken = builder.WriteBuffer("DebugUploadSyncToken", description, xiiGALResourceStateFlags::ShaderResource);
  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

// static
void xiiDebugRenderer::ExecuteDebugUpload(const xiiDebugUploadData& data, xiiRGPassContext& context)
{
  const xiiView* pView = context.GetView();
  if (pView == nullptr)
    return;

  const xiiExtractedRenderData* pExtractedData = pView->GetExtractedRenderData();
  const xiiUInt32               uiViewKey      = GetViewStorageKey(*pView);
  s_PreparedViewData.Remove(uiViewKey);

  if (pExtractedData == nullptr || !data.m_hSyncToken.IsValid())
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  const xiiUInt32 uiViewportWidth  = xiiMath::Max(1U, static_cast<xiiUInt32>(xiiMath::Round(pView->GetViewport().width)));
  const xiiUInt32 uiViewportHeight = xiiMath::Max(1U, static_cast<xiiUInt32>(xiiMath::Round(pView->GetViewport().height)));

  DebugUploadState         uploadState{*pView, cmd, uiViewportWidth, uiViewportHeight};
  DebugUploadAllocator     allocator;
  DebugTransitionCollector transitionCollector;
  PreparedDebugViewData    preparedViewData;

  UpdateGlobalConstants(*pView, cmd, uiViewportWidth, uiViewportHeight);
  QueueTransition(transitionCollector, s_pGlobalConstantsBuffer.Borrow(), xiiGALResourceStateFlags::ConstantBuffer);

  UploadContextData(pExtractedData->GetWorldDebugContext(), uploadState, allocator, transitionCollector, preparedViewData.m_WorldContext);
  UploadContextData(pExtractedData->GetViewDebugContext(), uploadState, allocator, transitionCollector, preparedViewData.m_ViewContext);

  FlushTransitions(cmd, transitionCollector);
  s_PreparedViewData.Insert(uiViewKey, std::move(preparedViewData));
}

// static
void xiiDebugRenderer::SetupDebugVisualization(xiiDebugVisualizationData& data, xiiRGBuilder& builder)
{
  data.m_hSyncToken  = builder.ReadBuffer("DebugUploadSyncToken", xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneColor = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_LDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);

  builder.SetPassAllowMerge(false);
}

// static
void xiiDebugRenderer::ExecuteDebugVisualization(const xiiDebugVisualizationData& data, xiiRGPassContext& context)
{
  const xiiView* pView = context.GetView();
  if (pView == nullptr)
    return;

  if (!data.m_hSceneColor.IsValid() || !data.m_hSceneDepth.IsValid() || !data.m_hSyncToken.IsValid())
    return;

  PreparedDebugViewData* pPreparedViewData = nullptr;
  if (!s_PreparedViewData.TryGetValue(GetViewStorageKey(*pView), pPreparedViewData) || pPreparedViewData == nullptr)
    return;

  xiiGALCommandList& cmd         = context.GetCommandList();
  xiiGALTexture*     pSceneColor = context.GetTexture(data.m_hSceneColor);
  xiiGALTexture*     pSceneDepth = context.GetTexture(data.m_hSceneDepth);
  if (pSceneColor == nullptr || pSceneDepth == nullptr)
    return;

  const xiiUInt32 uiViewportWidth  = xiiMath::Max(1U, static_cast<xiiUInt32>(xiiMath::Round(pView->GetViewport().width)));
  const xiiUInt32 uiViewportHeight = xiiMath::Max(1U, static_cast<xiiUInt32>(xiiMath::Round(pView->GetViewport().height)));

  DebugDrawState drawState{
    *pView,
    cmd,
    GetOrCreateRenderPass(*pSceneColor, *pSceneDepth),
    GetCameraMode(*pView),
    GetEyeCount(*pView),
    uiViewportWidth,
    uiViewportHeight,
  };

  cmd.BeginDebugGroup("DebugVisualization");
  {
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(uiViewportWidth), static_cast<float>(uiViewportHeight), 0.0f, 1.0f});
    DrawWorldSpaceContext(drawState, pPreparedViewData->m_WorldContext);
    DrawWorldSpaceContext(drawState, pPreparedViewData->m_ViewContext);
    DrawScreenSpaceContext(drawState, pPreparedViewData->m_WorldContext);
    DrawScreenSpaceContext(drawState, pPreparedViewData->m_ViewContext);
  }
  cmd.EndDebugGroup();

  s_PreparedViewData.Remove(GetViewStorageKey(*pView));
}

// static
void xiiDebugRenderer::AddRenderGraphPasses(xiiRenderGraph& graph)
{
  graph.AddPass<xiiDebugUploadData>("DebugUpload", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiDebugRenderer::SetupDebugUpload), xiiMakeDelegate(&xiiDebugRenderer::ExecuteDebugUpload));
  graph.AddPass<xiiDebugVisualizationData>("DebugVisualization", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiDebugRenderer::SetupDebugVisualization), xiiMakeDelegate(&xiiDebugRenderer::ExecuteDebugVisualization));
}

void xiiDebugRenderer::OnEngineStartup()
{
  {
    xiiGeometry geom;
    geom.AddLineBox(xiiVec3(2.0f));

    xiiMeshBufferResourceDescriptor desc;
    desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::LineList);

    s_hLineBoxMeshBuffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("DebugLineBox", std::move(desc), "Mesh for Rendering Debug Line Boxes");
  }

  {
    xiiGeometry geom;
    geom.AddBox(xiiVec3(2.0f), false);

    xiiMeshBufferResourceDescriptor desc;
    desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);

    s_hSolidBoxMeshBuffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("DebugSolidBox", std::move(desc), "Mesh for Rendering Debug Solid Boxes");
  }

  {
    EnsureDynamicMeshBufferPage(DynamicMeshBufferKind::Line, 0U);
    EnsureDynamicMeshBufferPage(DynamicMeshBufferKind::Triangle, 0U);
    EnsureDynamicMeshBufferPage(DynamicMeshBufferKind::TexturedTriangle, 0U);
  }

  {
    xiiImage debugFontImage;
    xiiGraphicsUtils::CreateSimpleASCIIFontTexture(debugFontImage);

    xiiGALTextureSubResourceData memoryDesc;
    memoryDesc.m_pData         = debugFontImage.GetByteBlobPtr();
    memoryDesc.m_uiStride      = static_cast<xiiUInt32>(debugFontImage.GetRowPitch());
    memoryDesc.m_uiDepthStride = static_cast<xiiUInt32>(debugFontImage.GetDepthPitch());

    xiiTexture2DResourceDescriptor desc;
    desc.m_TextureDescription               = xiiGALTextureUtilities::GetDefaultTexture2DDescription();
    desc.m_TextureDescription.m_Size.width  = debugFontImage.GetWidth();
    desc.m_TextureDescription.m_Size.height = debugFontImage.GetHeight();
    desc.m_TextureDescription.m_Format      = xiiGALResourceFormat::R8UNormalized;
    desc.m_InitialContent                   = xiiMakeArrayPtr(&memoryDesc, 1);

    s_hDebugFontTexture = xiiResourceManager::CreateResource<xiiTexture2DResource>("DebugFontTexture", std::move(desc));
  }

  s_hDebugGeometryShader          = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugGeometry.xiiShader");
  s_hDebugPrimitiveShader         = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugPrimitive.xiiShader");
  s_hDebugTexturedPrimitiveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugTexturedPrimitive.xiiShader");
  s_hDebugTextShader              = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugText.xiiShader");
  EnsureGlobalConstantsBuffer();
}

void xiiDebugRenderer::OnEngineShutdown()
{
  for (xiiUInt32 i = 0; i < BufferType::Count; ++i)
  {
    DestroyDataBuffers(static_cast<BufferType::Enum>(i));
  }

  for (xiiUInt32 i = 0; i < DynamicMeshBufferKind::Count; ++i)
  {
    s_DynamicMeshBufferPages[i].Clear();
  }

  s_hLineBoxMeshBuffer.Invalidate();
  s_hSolidBoxMeshBuffer.Invalidate();
  s_hDebugFontTexture.Invalidate();
  s_pGlobalConstantsBuffer.Clear();
  s_pPositionOnlyInputLayout.Clear();
  s_pVertexInputLayout.Clear();
  s_pTexVertexInputLayout.Clear();
  s_RenderPassCache.Clear();
  s_GraphicsPipelineCache.Clear();

  s_hDebugGeometryShader.Invalidate();
  s_hDebugPrimitiveShader.Invalidate();
  s_hDebugTexturedPrimitiveShader.Invalidate();
  s_hDebugTextShader.Invalidate();

  s_PerContextData.Clear();
  s_PreparedViewData.Clear();

  s_PersistentPerContextData.Clear();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptExtensionClass_Debug, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetResolution),

    XII_SCRIPT_FUNCTION_PROPERTY(DrawCross, In, "World", In, "Position", In, "Size", In, "Color", In, "Transform")->AddAttributes(new xiiFunctionArgumentAttributes(2, new xiiDefaultValueAttribute(0.1f)),new xiiFunctionArgumentAttributes(3, new xiiExposeColorAlphaAttribute())),
    XII_SCRIPT_FUNCTION_PROPERTY(DrawLineBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform")->AddAttributes(new xiiFunctionArgumentAttributes(2, new xiiDefaultValueAttribute(xiiVec3(1))),new xiiFunctionArgumentAttributes(3, new xiiExposeColorAlphaAttribute())),
    XII_SCRIPT_FUNCTION_PROPERTY(DrawLineSphere, In, "World", In, "Position", In, "Radius", In, "Color", In, "Transform")->AddAttributes(new xiiFunctionArgumentAttributes(2, new xiiDefaultValueAttribute(1.0f)),new xiiFunctionArgumentAttributes(3, new xiiExposeColorAlphaAttribute())),

    XII_SCRIPT_FUNCTION_PROPERTY(DrawSolidBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform")->AddAttributes(new xiiFunctionArgumentAttributes(2, new xiiDefaultValueAttribute(xiiVec3(1))),new xiiFunctionArgumentAttributes(3, new xiiExposeColorAlphaAttribute())),

    XII_SCRIPT_FUNCTION_PROPERTY(Draw2DText, In, "World", In, "Text", In, "Position", In, "Color", In, "SizeInPixel", In, "HAlign")->AddAttributes(new xiiFunctionArgumentAttributes(4, new xiiDefaultValueAttribute(16))),
    XII_SCRIPT_FUNCTION_PROPERTY(Draw3DText, In, "World", In, "Text", In, "Position", In, "Color", In, "SizeInPixel")->AddAttributes(new xiiFunctionArgumentAttributes(4, new xiiDefaultValueAttribute(16))),
    XII_SCRIPT_FUNCTION_PROPERTY(DrawInfoText, In, "World", In, "Text", In, "Placement", In, "Group", In, "Color"),

    XII_SCRIPT_FUNCTION_PROPERTY(AddPersistentCross, In, "World", In, "Position", In, "Size", In, "Color", In, "Transform", In, "Duration")->AddAttributes(new xiiFunctionArgumentAttributes(2, new xiiDefaultValueAttribute(0.1f)),new xiiFunctionArgumentAttributes(3, new xiiExposeColorAlphaAttribute()),new xiiFunctionArgumentAttributes(5, new xiiDefaultValueAttribute(xiiTime::MakeFromSeconds(1)))),
    XII_SCRIPT_FUNCTION_PROPERTY(AddPersistentLineBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform", In, "Duration")->AddAttributes(new xiiFunctionArgumentAttributes(2, new xiiDefaultValueAttribute(xiiVec3(1))),new xiiFunctionArgumentAttributes(3, new xiiExposeColorAlphaAttribute()),new xiiFunctionArgumentAttributes(5, new xiiDefaultValueAttribute(xiiTime::MakeFromSeconds(1)))),
    XII_SCRIPT_FUNCTION_PROPERTY(AddPersistentLineSphere, In, "World", In, "Position", In, "Radius", In, "Color", In, "Transform", In, "Duration")->AddAttributes(new xiiFunctionArgumentAttributes(2, new xiiDefaultValueAttribute(1.0f)),new xiiFunctionArgumentAttributes(3, new xiiExposeColorAlphaAttribute()),new xiiFunctionArgumentAttributes(5, new xiiDefaultValueAttribute(xiiTime::MakeFromSeconds(1)))),

    XII_SCRIPT_FUNCTION_PROPERTY(DrawLine, In, "World", In, "Start", In, "End", In, "StartColor", In, "EndColor")->AddAttributes(new xiiFunctionArgumentAttributes(3, new xiiExposeColorAlphaAttribute()),new xiiFunctionArgumentAttributes(4, new xiiExposeColorAlphaAttribute())),
    XII_SCRIPT_FUNCTION_PROPERTY(Draw2DLine, In, "World", In, "Start", In, "End", In, "StartColor", In, "EndColor")->AddAttributes(new xiiFunctionArgumentAttributes(3, new xiiExposeColorAlphaAttribute()),new xiiFunctionArgumentAttributes(4, new xiiExposeColorAlphaAttribute())),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiScriptExtensionAttribute("Debug"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiVec2 xiiScriptExtensionClass_Debug::GetResolution()
{
  for (xiiUInt32 uiWorldIndex = 0; uiWorldIndex < xiiWorld::GetWorldCount(); ++uiWorldIndex)
  {
    xiiWorld* pWorld = xiiWorld::GetWorld(uiWorldIndex);
    if (pWorld == nullptr)
      continue;

    xiiRenderWorldModule* pRenderWorldModule = pWorld->GetModule<xiiRenderWorldModule>();
    if (pRenderWorldModule == nullptr)
      continue;

    if (xiiView* pView = pRenderWorldModule->GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView); pView != nullptr)
    {
      return xiiVec2(pView->GetViewport().width, pView->GetViewport().height);
    }
  }

  return xiiVec2::MakeZero();
}

// static
void xiiScriptExtensionClass_Debug::DrawCross(const xiiWorld* pWorld, const xiiVec3& vPosition, float fSize, const xiiColor& color, const xiiTransform& transform)
{
  xiiDebugRenderer::DrawCross(pWorld, vPosition, fSize, color, transform);
}

// static
void xiiScriptExtensionClass_Debug::DrawLineBox(const xiiWorld* pWorld, const xiiVec3& vPosition, const xiiVec3& vHalfExtents, const xiiColor& color, const xiiTransform& transform)
{
  xiiDebugRenderer::DrawLineBox(pWorld, xiiBoundingBox::MakeFromCenterAndHalfExtents(vPosition, vHalfExtents), color, transform);
}

// static
void xiiScriptExtensionClass_Debug::DrawLineSphere(const xiiWorld* pWorld, const xiiVec3& vPosition, float fRadius, const xiiColor& color, const xiiTransform& transform)
{
  xiiDebugRenderer::DrawLineSphere(pWorld, xiiBoundingSphere::MakeFromCenterAndRadius(vPosition, fRadius), color, transform);
}

// static
void xiiScriptExtensionClass_Debug::DrawSolidBox(const xiiWorld* pWorld, const xiiVec3& vPosition, const xiiVec3& vHalfExtents, const xiiColor& color, const xiiTransform& transform)
{
  xiiDebugRenderer::DrawSolidBox(pWorld, xiiBoundingBox::MakeFromCenterAndHalfExtents(vPosition, vHalfExtents), color, transform);
}

// static
void xiiScriptExtensionClass_Debug::Draw2DText(const xiiWorld* pWorld, xiiStringView sText, const xiiVec3& vPosition, const xiiColor& color, xiiUInt32 uiSizeInPixel, xiiEnum<xiiDebugTextHAlign> horizontalAlignment)
{
  xiiVec2I32 vPositionInPixel = xiiVec2I32(static_cast<int>(xiiMath::Round(vPosition.x)), static_cast<int>(xiiMath::Round(vPosition.y)));
  xiiDebugRenderer::Draw2DText(pWorld, sText, vPositionInPixel, color, uiSizeInPixel, horizontalAlignment);
}

// static
void xiiScriptExtensionClass_Debug::Draw3DText(const xiiWorld* pWorld, xiiStringView sText, const xiiVec3& vPosition, const xiiColor& color, xiiUInt32 uiSizeInPixel)
{
  xiiDebugRenderer::Draw3DText(pWorld, sText, vPosition, color, uiSizeInPixel);
}

// static
void xiiScriptExtensionClass_Debug::DrawInfoText(const xiiWorld* pWorld, xiiStringView sText, xiiEnum<xiiDebugTextPlacement> placement, xiiStringView sGroupName, const xiiColor& color)
{
  xiiDebugRenderer::DrawInfoText(pWorld, placement, sGroupName, sText, color);
}

// static
void xiiScriptExtensionClass_Debug::AddPersistentCross(const xiiWorld* pWorld, const xiiVec3& vPosition, float fSize, const xiiColor& color, const xiiTransform& transform, xiiTime duration)
{
  xiiTransform t = transform;
  t.m_vPosition += vPosition;

  xiiDebugRenderer::AddPersistentCross(pWorld, fSize, color, t, duration);
}

// static
void xiiScriptExtensionClass_Debug::AddPersistentLineBox(const xiiWorld* pWorld, const xiiVec3& vPosition, const xiiVec3& vHalfExtents, const xiiColor& color, const xiiTransform& transform, xiiTime duration)
{
  xiiTransform t = transform;
  t.m_vPosition += vPosition;

  xiiDebugRenderer::AddPersistentLineBox(pWorld, vHalfExtents, color, t, duration);
}

// static
void xiiScriptExtensionClass_Debug::AddPersistentLineSphere(const xiiWorld* pWorld, const xiiVec3& vPosition, float fRadius, const xiiColor& color, const xiiTransform& transform, xiiTime duration)
{
  xiiTransform t = transform;
  t.m_vPosition += vPosition;

  xiiDebugRenderer::AddPersistentLineSphere(pWorld, fRadius, color, t, duration);
}

void xiiScriptExtensionClass_Debug::DrawLine(const xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& startColor, const xiiColor& endColor)
{
  xiiDebugRendererLine line[1];
  line[0].m_vStart     = vStart;
  line[0].m_vEnd       = vEnd;
  line[0].m_StartColor = startColor;
  line[0].m_EndColor   = endColor;

  xiiDebugRenderer::DrawLines(pWorld, line, xiiColor::White);
}

void xiiScriptExtensionClass_Debug::Draw2DLine(const xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& startColor, const xiiColor& endColor)
{
  xiiDebugRendererLine line[1];
  line[0].m_vStart     = vStart;
  line[0].m_vEnd       = vEnd;
  line[0].m_StartColor = startColor;
  line[0].m_EndColor   = endColor;

  xiiDebugRenderer::Draw2DLines(pWorld, line, xiiColor::White);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Debug_Implementation_DebugRenderer);
