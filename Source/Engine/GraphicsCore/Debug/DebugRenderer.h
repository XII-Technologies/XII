/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <GraphicsCore/Debug/DebugRendererContext.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/Declarations.h>

template <typename Type>
class xiiRectTemplate;
using xiiRectFloat = xiiRectTemplate<float>;

class xiiFormatString;
class xiiFrustum;
class xiiRenderGraph;
class xiiRenderGraphBuilder;
class xiiRenderGraphPassContext;

struct xiiDebugUploadData;
struct xiiDebugVisualizationData;

/// Horizontal alignment of debug text.
struct xiiDebugTextHAlign
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Left,
    Center,
    Right,

    Default = Left
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDebugTextHAlign);

/// Vertical alignment of debug text.
struct xiiDebugTextVAlign
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Top,
    Center,
    Bottom,

    Default = Top
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDebugTextVAlign);

/// Screen placement of debug text.
struct xiiDebugTextPlacement
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,

    ENUM_COUNT,

    Default = TopLeft
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDebugTextPlacement);

struct XII_GRAPHICSCORE_DLL xiiDebugRendererLine
{
  XII_DECLARE_POD_TYPE();

  xiiDebugRendererLine();
  xiiDebugRendererLine(const xiiVec3& vStart, const xiiVec3& vEnd);
  xiiDebugRendererLine(const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& color);

  xiiVec3 m_vStart;
  xiiVec3 m_vEnd;

  xiiColor m_StartColor = xiiColor::White;
  xiiColor m_EndColor   = xiiColor::White;
};

struct XII_GRAPHICSCORE_DLL xiiDebugRendererTriangle
{
  XII_DECLARE_POD_TYPE();

  xiiDebugRendererTriangle();
  xiiDebugRendererTriangle(const xiiVec3& v0, const xiiVec3& v1, const xiiVec3& v2);

  xiiVec3  m_vPosition[3];
  xiiColor m_Color = xiiColor::White;
};

struct XII_GRAPHICSCORE_DLL xiiDebugRendererTexturedTriangle
{
  XII_DECLARE_POD_TYPE();

  xiiVec3  m_vPosition[3];
  xiiVec2  m_vTexCoord[3];
  xiiColor m_Color = xiiColor::White;
};

/// Enables a function to take an xiiMat3, xiiMat4 or xiiTransfrom.
struct XII_GRAPHICSCORE_DLL xiiMatOrTransform
{
  xiiMatOrTransform(const xiiMat4& mMat4) :
    m_Mat4(mMat4)
  {
  }

  xiiMatOrTransform(const xiiMat3& mMat3)
  {
    m_Mat4.SetIdentity();
    m_Mat4.SetRotationalPart(mMat3);
  }

  xiiMatOrTransform(const xiiTransform& transform)
  {
    m_Mat4 = transform.GetAsMat4();
  }

  xiiMat4 m_Mat4;
};

/// Draws simple shapes into the scene or view.
///
/// Shapes can be rendered for a single frame, or 'persistent' for a certain duration.
/// The 'context' specifies whether shapes are generally visible in a scene, from all views,
/// or specific to a single view. See the xiiDebugRendererContext constructors for what can be implicitly used as a context.
class XII_GRAPHICSCORE_DLL xiiDebugRenderer
{
public:
  /// Renders the given set of lines for one frame.
  static void DrawLines(const xiiDebugRendererContext& context, xiiArrayPtr<const xiiDebugRendererLine> lines, const xiiColor& color, xiiMatOrTransform mTransform = xiiMat4::MakeIdentity());

  /// Renders the given set of lines in 2D (screen-space) for one frame.
  static void Draw2DLines(const xiiDebugRendererContext& context, xiiArrayPtr<const xiiDebugRendererLine> lines, const xiiColor& color);

  /// Renders a cross for one frame.
  static void DrawCross(const xiiDebugRendererContext& context, const xiiVec3& vGlobalPosition, float fLineLength, const xiiColor& color, xiiMatOrTransform mTransform = xiiMat4::MakeIdentity());

  /// Renders a wireframe box for one frame.
  static void DrawLineBox(const xiiDebugRendererContext& context, const xiiBoundingBox& box, const xiiColor& color, xiiMatOrTransform mTransform = xiiMat4::MakeIdentity());

  /// Renders the corners of a wireframe box for one frame.
  static void DrawLineBoxCorners(const xiiDebugRendererContext& context, const xiiBoundingBox& box, float fCornerFraction, const xiiColor& color, xiiMatOrTransform mTransform = xiiMat4::MakeIdentity());

  /// Renders a wireframe sphere for one frame.
  static void DrawLineSphere(const xiiDebugRendererContext& context, const xiiBoundingSphere& sphere, const xiiColor& color, xiiMatOrTransform mTransform = xiiMat4::MakeIdentity());

  /// Renders an upright wireframe capsule for one frame.
  static void DrawLineCapsuleZ(const xiiDebugRendererContext& context, float fLength, float fRadius, const xiiColor& color, xiiMatOrTransform mTransform = xiiMat4::MakeIdentity());

  /// Renders an upright wireframe cylinder for one frame.
  static void DrawLineCylinderZ(const xiiDebugRendererContext& context, float fLength, float fRadius, const xiiColor& color, xiiMatOrTransform mTransform = xiiMat4::MakeIdentity());

  /// Renders a wireframe frustum for one frame.
  static void DrawLineFrustum(const xiiDebugRendererContext& context, const xiiFrustum& frustum, const xiiColor& color, bool bDrawPlaneNormals = false);

  /// Renders a solid box for one frame.
  static void DrawSolidBox(const xiiDebugRendererContext& context, const xiiBoundingBox& box, const xiiColor& color, xiiMatOrTransform mTransform = xiiMat4::MakeIdentity());

  /// Renders the set of filled triangles for one frame.
  static void DrawSolidTriangles(const xiiDebugRendererContext& context, xiiArrayPtr<xiiDebugRendererTriangle> triangles, const xiiColor& color);

  /// Renders the set of textured triangles for one frame.
  static void DrawTexturedTriangles(const xiiDebugRendererContext& context, xiiArrayPtr<xiiDebugRendererTexturedTriangle> triangles, const xiiColor& color, const xiiTexture2DResourceHandle& hTexture);

  /// Renders a filled 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color);

  /// Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color, const xiiTexture2DResourceHandle& hTexture, xiiVec2 vScale = xiiVec2(1, 1));

  /// Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color, xiiSharedPtr<xiiGALTextureView> pTextureView, xiiVec2 vScale = xiiVec2(1, 1));

  /// Renders a wireframe 2D rectangle in screen-space for one frame.
  static void Draw2DLineRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color);

  /// Displays a string in screen-space for one frame.
  ///
  /// The string may contain newlines (\n) for multi-line output.
  /// If horizontal alignment is right, the entire text block is aligned according to the longest line.
  /// If vertical alignment is bottom, the entire text block is aligned there.
  ///
  /// Data can be output as a table, by separating columns with tabs (\t). For example:
  /// "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|"
  ///
  /// Returns the number of lines that the text was split up into.
  static xiiUInt32 Draw2DText(const xiiDebugRendererContext& context, const xiiFormatString& text, const xiiVec2I32& vPositionInPixel, const xiiColor& color, xiiUInt32 uiSizeInPixel = 16, xiiDebugTextHAlign::Enum horizontalAlignment = xiiDebugTextHAlign::Left, xiiDebugTextVAlign::Enum verticalAlignment = xiiDebugTextVAlign::Top);

  /// Draws a piece of text in one of the screen corners.
  ///
  /// Text positioning is automatic, all lines are placed in each corner such that they don't overlap.
  /// Text from different corners may overlap, though.
  ///
  /// For text formatting options, see Draw2DText().
  ///
  /// The \a sGroupName parameter is used to insert whitespace between unrelated pieces of text,
  /// it is not displayed anywhere, though.
  ///
  /// Text size cannot be changed.
  static void DrawInfoText(const xiiDebugRendererContext& context, xiiDebugTextPlacement::Enum placement, xiiStringView sGroupName, const xiiFormatString& text, const xiiColor& color = xiiColor::White);

  /// Same as DrawInfoText but displays the text for a certain duration.
  static void AddPersistentInfoText(const xiiDebugRendererContext& context, xiiDebugTextPlacement::Enum placement, const xiiFormatString& text, xiiTime duration, const xiiColor& color = xiiColor::White);

  /// Displays a string in 3D space for one frame.
  static xiiUInt32 Draw3DText(const xiiDebugRendererContext& context, const xiiFormatString& text, const xiiVec3& vGlobalPosition, const xiiColor& color, xiiUInt32 uiSizeInPixel = 16, xiiDebugTextHAlign::Enum horizontalAlignment = xiiDebugTextHAlign::Center, xiiDebugTextVAlign::Enum verticalAlignment = xiiDebugTextVAlign::Bottom);

  /// Renders a cross at the given location for as many frames until \a duration has passed.
  static void AddPersistentCross(const xiiDebugRendererContext& context, float fSize, const xiiColor& color, xiiMatOrTransform mTransform, xiiTime duration);

  /// Renders a wireframe sphere at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineSphere(const xiiDebugRendererContext& context, float fRadius, const xiiColor& color, xiiMatOrTransform mTransform, xiiTime duration);

  /// Renders a wireframe box at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineBox(const xiiDebugRendererContext& context, const xiiVec3& vHalfSize, const xiiColor& color, xiiMatOrTransform mTransform, xiiTime duration);

  /// Renders lines at the given location for as many frames until \a duration has passed.
  static void AddPersistentLines(const xiiDebugRendererContext& context, xiiArrayPtr<const xiiDebugRendererLine> lines, const xiiColor& color, xiiMatOrTransform mTransform, xiiTime duration);

  /// Renders a solid 2D cone in a plane with a given angle.
  ///
  /// The rotation goes around the given \a rotationAxis.
  /// An angle of zero is pointing into forwardAxis direction.
  /// Both angles may be negative.
  static void DrawAngle(const xiiDebugRendererContext& context, xiiAngle startAngle, xiiAngle endAngle, const xiiColor& solidColor, const xiiColor& lineColor, xiiMatOrTransform mTransform, xiiVec3 vForwardAxis = xiiVec3::MakeAxisX(), xiiVec3 vRotationAxis = xiiVec3::MakeAxisZ());

  /// Renders a cone with the tip at the center position, opening up with the given angle.
  static void DrawOpeningCone(const xiiDebugRendererContext& context, xiiAngle halfAngle, const xiiColor& colorInside, const xiiColor& colorOutside, xiiMatOrTransform mTransform, xiiVec3 vForwardAxis = xiiVec3::MakeAxisX());

  /// Renders a bent cone with the tip at the center position, pointing into the +X direction opening up with halfAngle1 and halfAngle2 along the Y and Z axis.
  ///
  /// If solidColor.a > 0, the cone is rendered with as solid triangles.
  /// If lineColor.a > 0, the cone is rendered as lines.
  /// Both can be combined.
  static void DrawLimitCone(const xiiDebugRendererContext& context, xiiAngle halfAngle1, xiiAngle halfAngle2, const xiiColor& solidColor, const xiiColor& lineColor, xiiMatOrTransform mTransform);

  /// Renders a cylinder starting at the center position, along the +X axis.
  ///
  /// If the start and end radius are different, a cone or arrow can be created.
  static void DrawCylinder(const xiiDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const xiiColor& solidColor, const xiiColor& lineColor, xiiMatOrTransform mTransform, bool bCapStart = false, bool bCapEnd = false, xiiBasisAxis::Enum cylinderAxis = xiiBasisAxis::PositiveX);

  /// Renders a line arrow.
  static void DrawArrow(const xiiDebugRendererContext& context, float fSize, const xiiColor& color, xiiMatOrTransform mTransform, xiiVec3 vForwardAxis = xiiVec3::MakeAxisX());

  /// Returns the width of single glyph in pixels for the given text size
  static float GetTextGlyphWidth(xiiUInt32 uiSizeInPixel = 16U);

  /// Returns the line height in pixels for the given text size
  static float GetTextLineHeight(xiiUInt32 uiSizeInPixel = 16U);

  /// Returns the global debug text scale
  static float GetTextScale();

  /// Sets the global debug text scale
  static void SetTextScale(float fScale);

public:
  static void AddRenderGraphPasses(xiiRenderGraph& graph);

private:
  static void SetupDebugUpload(xiiDebugUploadData& data, xiiRenderGraphBuilder& builder);
  static void ExecuteDebugUpload(const xiiDebugUploadData& data, xiiRenderGraphPassContext& context);

  static void SetupDebugVisualization(xiiDebugVisualizationData& data, xiiRenderGraphBuilder& builder);
  static void ExecuteDebugVisualization(const xiiDebugVisualizationData& data, xiiRenderGraphPassContext& context);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, DebugRenderer);
};

/// Helper class to expose debug rendering to scripting
class XII_GRAPHICSCORE_DLL xiiScriptExtensionClass_Debug
{
public:
  /// Returns the resolution of the first main view that it can find.
  static xiiVec2 GetResolution();

  static void DrawCross(const xiiWorld* pWorld, const xiiVec3& vPosition, float fSize, const xiiColor& color, const xiiTransform& transform);
  static void DrawLineBox(const xiiWorld* pWorld, const xiiVec3& vPosition, const xiiVec3& vHalfExtents, const xiiColor& color, const xiiTransform& transform);
  static void DrawLineSphere(const xiiWorld* pWorld, const xiiVec3& vPosition, float fRadius, const xiiColor& color, const xiiTransform& transform);

  static void DrawSolidBox(const xiiWorld* pWorld, const xiiVec3& vPosition, const xiiVec3& vHalfExtents, const xiiColor& color, const xiiTransform& transform);

  static void Draw2DText(const xiiWorld* pWorld, xiiStringView sText, const xiiVec3& vPositionInPixel, const xiiColor& color, xiiUInt32 uiSizeInPixel, xiiEnum<xiiDebugTextHAlign> horizontalAlignment);
  static void Draw3DText(const xiiWorld* pWorld, xiiStringView sText, const xiiVec3& vPosition, const xiiColor& color, xiiUInt32 uiSizeInPixel);
  static void DrawInfoText(const xiiWorld* pWorld, xiiStringView sText, xiiEnum<xiiDebugTextPlacement> placement, xiiStringView sGroupName, const xiiColor& color);

  static void AddPersistentCross(const xiiWorld* pWorld, const xiiVec3& vPosition, float fSize, const xiiColor& color, const xiiTransform& transform, xiiTime duration);
  static void AddPersistentLineBox(const xiiWorld* pWorld, const xiiVec3& vPosition, const xiiVec3& vHalfExtents, const xiiColor& color, const xiiTransform& transform, xiiTime duration);
  static void AddPersistentLineSphere(const xiiWorld* pWorld, const xiiVec3& vPosition, float fRadius, const xiiColor& color, const xiiTransform& transform, xiiTime duration);

  static void DrawLine(const xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& startColor, const xiiColor& endColor);

  static void Draw2DLine(const xiiWorld* pWorld, const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& startColor, const xiiColor& endColor);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiScriptExtensionClass_Debug);

#include <GraphicsCore/Debug/Implementation/DebugRenderer_inl.h>
