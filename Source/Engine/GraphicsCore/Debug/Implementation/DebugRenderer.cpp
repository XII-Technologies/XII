#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Debug/SimpleASCIIFont.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Utils/CommandListUtilities.h>

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
    DoubleBufferedPerContextData() :
      m_uiLastRenderedFrame(0), m_pData{nullptr, nullptr}
    {
    }

    xiiUInt64                    m_uiLastRenderedFrame;
    xiiUniquePtr<PerContextData> m_pData[2];
  };

  static xiiHashTable<xiiDebugRendererContext, DoubleBufferedPerContextData> s_PerContextData;
  static xiiMutex                                                            s_Mutex;

  static PerContextData& GetDataForExtraction(const xiiDebugRendererContext& context)
  {
    DoubleBufferedPerContextData& doubleBufferedData = s_PerContextData[context];

    const xiiUInt32 uiDataIndex = xiiRenderWorld::IsRenderingThread() && (doubleBufferedData.m_uiLastRenderedFrame != xiiRenderWorld::GetFrameCounter()) ? xiiRenderWorld::GetDataIndexForRendering() : xiiRenderWorld::GetDataIndexForExtraction();

    xiiUniquePtr<PerContextData>& pData = doubleBufferedData.m_pData[uiDataIndex];
    if (pData == nullptr)
    {
      doubleBufferedData.m_pData[uiDataIndex] = XII_DEFAULT_NEW(PerContextData);
    }

    return *pData;
  }

  static void ClearRenderData()
  {
    XII_LOCK(s_Mutex);

    for (auto it = s_PerContextData.GetIterator(); it.IsValid(); ++it)
    {
      PerContextData* pData = it.Value().m_pData[xiiRenderWorld::GetDataIndexForRendering()].Borrow();
      if (pData)
      {
        pData->m_LineVertices.Clear();
        pData->m_Line2DVertices.Clear();
        pData->m_LineBoxes.Clear();
        pData->m_SolidBoxes.Clear();
        pData->m_TriangleVertices.Clear();
        pData->m_Triangle2DVertices.Clear();
        pData->m_TexturedTriangle2DVertices.Clear();
        pData->m_TexturedTriangle3DVertices.Clear();
        pData->m_sTextLines2D.Clear();
        pData->m_sTextLines3D.Clear();

        for (xiiUInt32 i = 0; i < (xiiUInt32)xiiDebugTextPlacement::ENUM_COUNT; ++i)
        {
          pData->m_InfoTextData[i].Clear();
        }
      }
    }
  }

  static void OnRenderEvent(const xiiRenderWorldRenderEvent& e)
  {
    if (e.m_Type == xiiRenderWorldRenderEvent::Type::EndRender)
    {
      ClearRenderData();
    }
  }

  struct BufferType
  {
    enum Enum
    {
      Lines,
      LineBoxes,
      SolidBoxes,
      Triangles3D,
      Triangles2D,
      TexTriangles2D,
      TexTriangles3D,
      Glyphs,
      Lines2D,

      Count
    };
  };

  static xiiSharedPtr<xiiGALBuffer> s_pDataBuffer[BufferType::Count];

  static xiiMeshBufferResourceHandle s_hLineBoxMeshBuffer;
  static xiiMeshBufferResourceHandle s_hSolidBoxMeshBuffer;
  static xiiInputLayoutInfo          s_InputLayoutInfo;
  static xiiInputLayoutInfo          s_TexInputLayoutInfo;
  static xiiTexture2DResourceHandle  s_hDebugFontTexture;

  static xiiShaderResourceHandle s_hDebugGeometryShader;
  static xiiShaderResourceHandle s_hDebugPrimitiveShader;
  static xiiShaderResourceHandle s_hDebugTexturedPrimitiveShader;
  static xiiShaderResourceHandle s_hDebugTextShader;

  enum
  {
    DEBUG_BUFFER_SIZE               = 1024 * 256,
    BOXES_PER_BATCH                 = DEBUG_BUFFER_SIZE / sizeof(BoxData),
    LINE_VERTICES_PER_BATCH         = DEBUG_BUFFER_SIZE / sizeof(Vertex),
    TRIANGLE_VERTICES_PER_BATCH     = (DEBUG_BUFFER_SIZE / sizeof(Vertex) / 3) * 3,
    TEX_TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(TexVertex) / 3) * 3,
    GLYPHS_PER_BATCH                = DEBUG_BUFFER_SIZE / sizeof(GlyphData),
  };

  static void CreateDataBuffer(BufferType::Enum bufferType, xiiUInt32 uiStructSize)
  {
    if (s_pDataBuffer[bufferType] == nullptr)
    {
      xiiGALBufferCreationDescription bufferDescription;
      bufferDescription.m_uiElementByteStride = uiStructSize;
      bufferDescription.m_uiSize              = DEBUG_BUFFER_SIZE;
      bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
      bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
      bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
      bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

      s_pDataBuffer[bufferType] = xiiGALDevice::GetDefaultDevice()->CreateBuffer(bufferDescription);
    }
  }

  static void CreateVertexBuffer(BufferType::Enum bufferType, xiiUInt32 uiVertexSize)
  {
    if (s_pDataBuffer[bufferType] == nullptr)
    {
      xiiGALBufferCreationDescription bufferDescription;
      bufferDescription.m_uiElementByteStride = uiVertexSize;
      bufferDescription.m_uiSize              = DEBUG_BUFFER_SIZE;
      bufferDescription.m_BindFlags           = xiiGALBindFlags::VertexBuffer;
      bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
      bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

      s_pDataBuffer[bufferType] = xiiGALDevice::GetDefaultDevice()->CreateBuffer(bufferDescription);
    }
  }

  static void DestroyBuffer(BufferType::Enum bufferType)
  {
    s_pDataBuffer[bufferType].Clear();
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

// static
void xiiDebugRenderer::RenderWorldSpace(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternalWorldSpace(*renderViewContext.m_pWorldDebugContext, renderViewContext, pCommandList);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternalWorldSpace(*renderViewContext.m_pViewDebugContext, renderViewContext, pCommandList);
  }
}

// static
void xiiDebugRenderer::RenderInternalWorldSpace(const xiiDebugRendererContext& context, const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  {
    XII_LOCK(s_Mutex);

    auto& data = s_PersistentPerContextData[context];
    data.m_Now = xiiClock::GetGlobalClock()->GetLastUpdateTime();

    // persistent crosses
    {
      xiiUInt32 uiNumItems = data.m_Crosses.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Crosses[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Crosses.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          xiiDebugRenderer::DrawCross(context, xiiVec3::MakeZero(), item.m_fSize, item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent spheres
    {
      xiiUInt32 uiNumItems = data.m_Spheres.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Spheres[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Spheres.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          xiiDebugRenderer::DrawLineSphere(context, xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), item.m_fRadius), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent boxes
    {
      xiiUInt32 uiNumItems = data.m_Boxes.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Boxes[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Boxes.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          xiiDebugRenderer::DrawLineBox(context, xiiBoundingBox::MakeFromMinMax(-item.m_vHalfSize, item.m_vHalfSize), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent lines
    {
      xiiUInt32 uiNumItems = data.m_Lines.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Lines[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Lines.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          xiiDebugRenderer::DrawLines(context, item.m_Lines.GetArrayPtr(), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }
  }

  DoubleBufferedPerContextData* pDoubleBufferedContextData = nullptr;
  if (!s_PerContextData.TryGetValue(context, pDoubleBufferedContextData))
    return;

  PerContextData* pData = pDoubleBufferedContextData->m_pData[xiiRenderWorld::GetDataIndexForRendering()].Borrow();
  if (pData == nullptr)
    return;

  // SolidBoxes
  {
    xiiUInt32 uiNumSolidBoxes = pData->m_SolidBoxes.GetCount();
    if (uiNumSolidBoxes != 0)
    {
      CreateDataBuffer(BufferType::SolidBoxes, sizeof(BoxData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);

      xiiGALCommandListUtilities::BindBuffer(pCommandList, "boxData", s_pDataBuffer[BufferType::SolidBoxes]);

      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hSolidBoxMeshBuffer);

      const BoxData* pSolidBoxData = pData->m_SolidBoxes.GetData();
      while (uiNumSolidBoxes > 0)
      {
        const xiiUInt32 uiNumSolidBoxesInBatch = xiiMath::Min<xiiUInt32>(uiNumSolidBoxes, BOXES_PER_BATCH);

        xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, s_pDataBuffer[BufferType::SolidBoxes], 0, xiiMakeArrayPtr(pSolidBoxData, uiNumSolidBoxesInBatch).ToByteArray()).AssertSuccess();

        unsigned int uiRenderedInstances = uiNumSolidBoxesInBatch;
        if (renderViewContext.m_pCamera->IsStereoscopic())
          uiRenderedInstances *= 2;

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiRenderedInstances).IgnoreResult();

        uiNumSolidBoxes -= uiNumSolidBoxesInBatch;
        pSolidBoxData += BOXES_PER_BATCH;
      }
    }
  }

  // Triangles
  {
    xiiUInt32 uiNumTriangleVertices = pData->m_TriangleVertices.GetCount();
    if (uiNumTriangleVertices != 0)
    {
      CreateVertexBuffer(BufferType::Triangles3D, sizeof(Vertex));

      renderViewContext.SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pTriangleData = pData->m_TriangleVertices.GetData();
      while (uiNumTriangleVertices > 0)
      {
        const xiiUInt32 uiNumTriangleVerticesInBatch = xiiMath::Min<xiiUInt32>(uiNumTriangleVertices, TRIANGLE_VERTICES_PER_BATCH);
        XII_ASSERT_DEV(uiNumTriangleVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");

        xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, s_pDataBuffer[BufferType::Triangles3D], 0, xiiMakeArrayPtr(pTriangleData, uiNumTriangleVerticesInBatch).ToByteArray()).AssertSuccess();

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_pDataBuffer[BufferType::Triangles3D], {}, &s_InputLayoutInfo, xiiGALPrimitiveTopology::TriangleList, uiNumTriangleVerticesInBatch / 3);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumTriangleVertices -= uiNumTriangleVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 3D triangles
  {
    for (auto itTex = pData->m_TexturedTriangle3DVertices.GetIterator(); itTex.IsValid(); ++itTex)
    {
      renderViewContext.m_pRenderContext->BindTexture2D("BaseTexture", itTex.Key());

      const auto& verts = itTex.Value();

      xiiUInt32 uiNumVertices = verts.GetCount();
      if (uiNumVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles3D, sizeof(TexVertex));

        renderViewContext.SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNumVertices > 0)
        {
          const xiiUInt32 uiNumVerticesInBatch = xiiMath::Min<xiiUInt32>(uiNumVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          XII_ASSERT_DEV(uiNumVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");

          xiiGALDeviceUtilities::MapAndUpdateBuffer(pGALCommandList, s_pDataBuffer[BufferType::TexTriangles3D], 0, xiiMakeArrayPtr(pTriangleData, uiNumVerticesInBatch).ToByteArray()).AssertSuccess();

          renderViewContext.m_pRenderContext->BindMeshBuffer(s_pDataBuffer[BufferType::TexTriangles3D], {}, &s_TexInputLayoutInfo, xiiGALPrimitiveTopology::TriangleList, uiNumVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          uiNumVertices -= uiNumVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
      }
    }
  }

  // 3D Lines
  {
    xiiUInt32 uiNumLineVertices = pData->m_LineVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines, sizeof(Vertex));

      renderViewContext.SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_LineVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        const xiiUInt32 uiNumLineVerticesInBatch = xiiMath::Min<xiiUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        XII_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");

        xiiGALDeviceUtilities::MapAndUpdateBuffer(pGALCommandList, s_pDataBuffer[BufferType::Lines], 0, xiiMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray()).AssertSuccess();

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_pDataBuffer[BufferType::Lines], {}, &s_InputLayoutInfo, xiiGALPrimitiveTopology::LineList, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // LineBoxes
  {
    xiiUInt32 uiNumLineBoxes = pData->m_LineBoxes.GetCount();
    if (uiNumLineBoxes != 0)
    {
      CreateDataBuffer(BufferType::LineBoxes, sizeof(BoxData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);

      xiiGALCommandListUtilities::BindBuffer(pCommandList, "boxData", s_pDataBuffer[BufferType::LineBoxes]);

      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hLineBoxMeshBuffer);

      const BoxData* pLineBoxData = pData->m_LineBoxes.GetData();
      while (uiNumLineBoxes > 0)
      {
        const xiiUInt32 uiNumLineBoxesInBatch = xiiMath::Min<xiiUInt32>(uiNumLineBoxes, BOXES_PER_BATCH);

        xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, s_pDataBuffer[BufferType::LineBoxes], 0, xiiMakeArrayPtr(pLineBoxData, uiNumLineBoxesInBatch).ToByteArray()).AssertSuccess();

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumLineBoxesInBatch).IgnoreResult();

        uiNumLineBoxes -= uiNumLineBoxesInBatch;
        pLineBoxData += BOXES_PER_BATCH;
      }
    }
  }

  // Text
  {
    pData->m_Glyphs.Clear();

    for (auto& textLine : pData->m_sTextLines3D)
    {
      xiiVec3 screenPos;
      if (renderViewContext.m_pViewData->ComputeScreenSpacePos(textLine.m_vPosition, screenPos).Succeeded() && screenPos.z > 0.0f)
      {
        renderViewContext.m_pViewData->ConvertScreenNormalizedPosToPixelPos(screenPos);

        textLine.m_vTopLeftCorner.x += xiiMath::Round(screenPos.x);
        textLine.m_vTopLeftCorner.y += xiiMath::Round(screenPos.y);

        AppendGlyphs(pData->m_Glyphs, textLine);
      }
    }

    xiiUInt32 uiNumGlyphs = pData->m_Glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);
      renderViewContext.m_pRenderContext->BindBuffer("glyphData", s_pDataBuffer[BufferType::Glyphs]->GetDefaultView(xiiGALBufferViewType::ShaderResource));

      xiiGALCommandListUtilities::BindTexture2D(pCommandList, "FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_Glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        const xiiUInt32 uiNumGlyphsInBatch = xiiMath::Min<xiiUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);

        xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, s_pDataBuffer[BufferType::Glyphs], 0, xiiMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray()).AssertSuccess();

        renderViewContext.m_pRenderContext->BindMeshBuffer({}, {}, nullptr, xiiGALPrimitiveTopology::TriangleList, uiNumGlyphsInBatch * 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumGlyphs -= uiNumGlyphsInBatch;
        pGlyphData += GLYPHS_PER_BATCH;
      }
    }
  }
}

// static
void xiiDebugRenderer::RenderScreenSpace(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternalScreenSpace(*renderViewContext.m_pWorldDebugContext, renderViewContext, pCommandList);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternalScreenSpace(*renderViewContext.m_pViewDebugContext, renderViewContext, pCommandList);
  }
}

// static
void xiiDebugRenderer::RenderInternalScreenSpace(const xiiDebugRendererContext& context, const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  {
    XII_LOCK(s_Mutex);

    auto& data = s_PersistentPerContextData[context];
    data.m_Now = xiiClock::GetGlobalClock()->GetLastUpdateTime();

    // persistent info text
    {
      xiiUInt32 uiNumItems = data.m_InfoText.GetCount();
      for (xiiUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_InfoText[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_InfoText.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          xiiDebugRenderer::DrawInfoText(context, item.m_Placement, "__Persistent", item.m_sText.GetView(), item.m_Color);

          ++i;
        }
      }
    }
  }

  DoubleBufferedPerContextData* pDoubleBufferedContextData = nullptr;
  if (!s_PerContextData.TryGetValue(context, pDoubleBufferedContextData))
    return;

  PerContextData* pData = pDoubleBufferedContextData->m_pData[xiiRenderWorld::GetDataIndexForRendering()].Borrow();
  if (pData == nullptr)
    return;

  // draw info text
  {
    static_assert((xiiUInt8)xiiDebugTextPlacement::ENUM_COUNT == 6);

    xiiDebugTextHAlign::Enum ha[(xiiUInt8)xiiDebugTextPlacement::ENUM_COUNT] = {
      xiiDebugTextHAlign::Left,
      xiiDebugTextHAlign::Center,
      xiiDebugTextHAlign::Right,
      xiiDebugTextHAlign::Left,
      xiiDebugTextHAlign::Center,
      xiiDebugTextHAlign::Right};

    xiiDebugTextVAlign::Enum va[(xiiUInt8)xiiDebugTextPlacement::ENUM_COUNT] = {
      xiiDebugTextVAlign::Top,
      xiiDebugTextVAlign::Top,
      xiiDebugTextVAlign::Top,
      xiiDebugTextVAlign::Bottom,
      xiiDebugTextVAlign::Bottom,
      xiiDebugTextVAlign::Bottom};

    xiiInt32 lineHeight = (xiiInt32)GetTextLineHeight();

    xiiInt32 resX = (xiiInt32)renderViewContext.m_pViewData->m_ViewPortRect.width;
    xiiInt32 resY = (xiiInt32)renderViewContext.m_pViewData->m_ViewPortRect.height;

    xiiVec2I32 anchor[(xiiUInt8)xiiDebugTextPlacement::ENUM_COUNT] = {
      xiiVec2I32(10, 10),
      xiiVec2I32(resX / 2, 10),
      xiiVec2I32(resX - 10, 10),
      xiiVec2I32(10, resY - 10),
      xiiVec2I32(resX / 2, resY - 10),
      xiiVec2I32(resX - 10, resY - 10)};

    for (xiiUInt32 corner = 0; corner < (xiiUInt32)xiiDebugTextPlacement::ENUM_COUNT; ++corner)
    {
      auto& cd = pData->m_InfoTextData[corner];

      // InsertionSort is stable
      xiiSorting::InsertionSort(cd, [](const InfoTextData& lhs, const InfoTextData& rhs) -> bool { return lhs.m_sGroup < rhs.m_sGroup; });

      xiiVec2I32 pos    = anchor[corner];
      xiiInt32   offset = offset = va[corner] == xiiDebugTextVAlign::Top ? lineHeight : -lineHeight;

      for (xiiUInt32 i = 0; i < cd.GetCount(); ++i)
      {
        // add some space between groups
        if (i > 0 && cd[i - 1].m_sGroup != cd[i].m_sGroup)
          pos.y += offset;

        pos.y += offset * Draw2DText(context, cd[i].m_sText.GetData(), pos, cd[i].m_Color, 16, ha[corner], va[corner]);
      }
    }
  }

  // update the frame counter
  pDoubleBufferedContextData->m_uiLastRenderedFrame = xiiRenderWorld::GetFrameCounter();

  // 2D Lines
  {
    xiiUInt32 uiNumLineVertices = pData->m_Line2DVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines2D, sizeof(Vertex));

      renderViewContext.SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_Line2DVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        const xiiUInt32 uiNumLineVerticesInBatch = xiiMath::Min<xiiUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        XII_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");

        xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, s_pDataBuffer[BufferType::Lines2D], 0, xiiMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray()).AssertSuccess();

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_pDataBuffer[BufferType::Lines2D], {}, &s_InputLayoutInfo, xiiGALPrimitiveTopology::LineList, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // 2D Rectangles
  {
    xiiUInt32 uiNum2DVertices = pData->m_Triangle2DVertices.GetCount();
    if (uiNum2DVertices != 0)
    {
      CreateVertexBuffer(BufferType::Triangles2D, sizeof(Vertex));

      renderViewContext.SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pTriangleData = pData->m_Triangle2DVertices.GetData();
      while (uiNum2DVertices > 0)
      {
        const xiiUInt32 uiNum2DVerticesInBatch = xiiMath::Min<xiiUInt32>(uiNum2DVertices, TRIANGLE_VERTICES_PER_BATCH);
        XII_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");

        xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, s_pDataBuffer[BufferType::Triangles2D], 0, xiiMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray()).AssertSuccess();

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_pDataBuffer[BufferType::Triangles2D], {}, &s_InputLayoutInfo, xiiGALPrimitiveTopology::TriangleList, uiNum2DVerticesInBatch / 3);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNum2DVertices -= uiNum2DVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 2D triangles
  {
    for (auto itTex = pData->m_TexturedTriangle2DVertices.GetIterator(); itTex.IsValid(); ++itTex)
    {
      xiiGALCommandListUtilities::BindTextureView(pCommandList, "BaseTexture", itTex.Key());

      const auto& verts = itTex.Value();

      xiiUInt32 uiNum2DVertices = verts.GetCount();
      if (uiNum2DVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles2D, sizeof(TexVertex));

        renderViewContext.SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNum2DVertices > 0)
        {
          const xiiUInt32 uiNum2DVerticesInBatch = xiiMath::Min<xiiUInt32>(uiNum2DVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          XII_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");

          xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, s_pDataBuffer[BufferType::TexTriangles2D], 0, xiiMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray()).AssertSuccess();

          renderViewContext.m_pRenderContext->BindMeshBuffer(s_pDataBuffer[BufferType::TexTriangles2D], {}, &s_TexInputLayoutInfo, xiiGALPrimitiveTopology::TriangleList, uiNum2DVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          uiNum2DVertices -= uiNum2DVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
      }
    }
  }

  // Text
  {
    pData->m_Glyphs.Clear();

    for (auto& textLine : pData->m_sTextLines2D)
    {
      AppendGlyphs(pData->m_Glyphs, textLine);
    }

    xiiUInt32 uiNumGlyphs = pData->m_Glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);

      xiiGALCommandListUtilities::BindBuffer(pCommandList, "glyphData", s_pDataBuffer[BufferType::Glyphs]);
      xiiGALCommandListUtilities::BindTexture2D(pCommandList, "FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_Glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        const xiiUInt32 uiNumGlyphsInBatch = xiiMath::Min<xiiUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);

        xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, s_pDataBuffer[BufferType::Glyphs], 0, xiiMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray()).AssertSuccess();

        renderViewContext.m_pRenderContext->BindMeshBuffer({}, {}, nullptr, xiiGALPrimitiveTopology::TriangleList, uiNumGlyphsInBatch * 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumGlyphs -= uiNumGlyphsInBatch;
        pGlyphData += GLYPHS_PER_BATCH;
      }
    }
  }
}

void xiiDebugRenderer::OnEngineStartup()
{
  {
    xiiGeometry geom;
    geom.AddLineBox(xiiVec3(2.0f));

    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALResourceFormat::RGB32Float);
    desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::LineList);

    s_hLineBoxMeshBuffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("DebugLineBox", std::move(desc), "Mesh for Rendering Debug Line Boxes");
  }

  {
    xiiGeometry geom;
    geom.AddBox(xiiVec3(2.0f), false);

    xiiMeshBufferResourceDescriptor desc;
    desc.AddStream(xiiGALInputLayoutSemantic::Position, xiiGALResourceFormat::RGB32Float);
    desc.AllocateStreamsFromGeometry(geom, xiiGALPrimitiveTopology::TriangleList);

    s_hSolidBoxMeshBuffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("DebugSolidBox", std::move(desc), "Mesh for Rendering Debug Solid Boxes");
  }

  {
    // reset, if already used before
    s_InputLayoutInfo.m_VertexStreams.Clear();

    {
      xiiVertexStreamInfo& si = s_InputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::Position;
      si.m_Format             = xiiGALResourceFormat::RGB32Float;
      si.m_uiOffset           = 0;
      si.m_uiElementSize      = 12;
    }

    {
      xiiVertexStreamInfo& si = s_InputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::Color0;
      si.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
      si.m_uiOffset           = 12;
      si.m_uiElementSize      = 4;
    }
  }

  {
    // reset, if already used before
    s_TexInputLayoutInfo.m_VertexStreams.Clear();

    {
      xiiVertexStreamInfo& si = s_TexInputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::Position;
      si.m_Format             = xiiGALResourceFormat::RGB32Float;
      si.m_uiOffset           = 0;
      si.m_uiElementSize      = 12;
    }

    {
      xiiVertexStreamInfo& si = s_TexInputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::Color0;
      si.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
      si.m_uiOffset           = 12;
      si.m_uiElementSize      = 4;
    }

    {
      xiiVertexStreamInfo& si = s_TexInputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::TexCoord0;
      si.m_Format             = xiiGALResourceFormat::RG32Float;
      si.m_uiOffset           = 16;
      si.m_uiElementSize      = 8;
    }

    {
      xiiVertexStreamInfo& si = s_TexInputLayoutInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic           = xiiGALInputLayoutSemantic::TexCoord1; // padding
      si.m_Format             = xiiGALResourceFormat::RG32Float;
      si.m_uiOffset           = 24;
      si.m_uiElementSize      = 8;
    }
  }

  {
    xiiImage debugFontImage;
    xiiGraphicsUtils::CreateSimpleASCIIFontTexture(debugFontImage);

    xiiGALTextureSubResourceData memoryDesc;
    memoryDesc.m_pData         = debugFontImage.GetByteBlobPtr();
    memoryDesc.m_uiStride      = static_cast<xiiUInt32>(debugFontImage.GetRowPitch());
    memoryDesc.m_uiDepthStride = static_cast<xiiUInt32>(debugFontImage.GetDepthPitch());

    xiiTexture2DResourceDescriptor desc;
    desc.m_DescGAL               = xiiGALTextureUtilities::GetDefaultTexture2DDescription();
    desc.m_DescGAL.m_Size.width  = debugFontImage.GetWidth();
    desc.m_DescGAL.m_Size.height = debugFontImage.GetHeight();
    desc.m_DescGAL.m_Format      = xiiGALResourceFormat::R8UNormalized;
    desc.m_InitialContent        = xiiMakeArrayPtr(&memoryDesc, 1);

    s_hDebugFontTexture = xiiResourceManager::CreateResource<xiiTexture2DResource>("DebugFontTexture", std::move(desc));
  }

  s_hDebugGeometryShader          = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugGeometry.xiiShader");
  s_hDebugPrimitiveShader         = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugPrimitive.xiiShader");
  s_hDebugTexturedPrimitiveShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugTexturedPrimitive.xiiShader");
  s_hDebugTextShader              = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Debug/DebugText.xiiShader");

  xiiRenderWorld::GetRenderEvent().AddEventHandler(&OnRenderEvent);
}

void xiiDebugRenderer::OnEngineShutdown()
{
  xiiRenderWorld::GetRenderEvent().RemoveEventHandler(&OnRenderEvent);

  for (xiiUInt32 i = 0; i < BufferType::Count; ++i)
  {
    DestroyBuffer(static_cast<BufferType::Enum>(i));
  }

  s_hLineBoxMeshBuffer.Invalidate();
  s_hSolidBoxMeshBuffer.Invalidate();
  s_hDebugFontTexture.Invalidate();

  s_hDebugGeometryShader.Invalidate();
  s_hDebugPrimitiveShader.Invalidate();
  s_hDebugTexturedPrimitiveShader.Invalidate();
  s_hDebugTextShader.Invalidate();

  s_PerContextData.Clear();

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
  for (const xiiViewHandle& hView : xiiRenderWorld::GetMainViews())
  {
    xiiView* pView;
    if (xiiRenderWorld::TryGetView(hView, pView))
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
