#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

template <typename Type>
class xiiRectTemplate;
using xiiRectFloat = xiiRectTemplate<float>;

class xiiFormatString;
class xiiFrustum;
struct xiiRenderViewContext;

/// \brief Draws simple shapes into the scene or view.
///
/// Shapes can be rendered for a single frame, or 'persistent' for a certain duration.
/// The 'context' specifies whether shapes are generally visible in a scene, from all views,
/// or specific to a single view. See the xiiDebugRendererContext constructors for what can be implicitly
/// used as a context.
class XII_RENDERERCORE_DLL xiiDebugRenderer
{
public:
  struct Line
  {
    XII_DECLARE_POD_TYPE();

    Line();
    Line(const xiiVec3& vStart, const xiiVec3& vEnd);
    Line(const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& color);

    xiiVec3 m_start;
    xiiVec3 m_end;

    xiiColor m_startColor = xiiColor::White;
    xiiColor m_endColor   = xiiColor::White;
  };

  struct Triangle
  {
    XII_DECLARE_POD_TYPE();

    Triangle();
    Triangle(const xiiVec3& v0, const xiiVec3& v1, const xiiVec3& v2);

    xiiVec3  m_position[3];
    xiiColor m_color = xiiColor::White;
  };

  struct TexturedTriangle
  {
    XII_DECLARE_POD_TYPE();

    xiiVec3  m_position[3];
    xiiVec2  m_texcoord[3];
    xiiColor m_color = xiiColor::White;
  };

  enum class HorizontalAlignment : xiiUInt8
  {
    Left,
    Center,
    Right
  };

  enum class VerticalAlignment : xiiUInt8
  {
    Top,
    Center,
    Bottom
  };

  enum class ScreenPlacement : xiiUInt8
  {
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,

    ENUM_COUNT
  };

  /// \brief Renders the given set of lines for one frame.
  static void DrawLines(const xiiDebugRendererContext& context, xiiArrayPtr<const Line> lines, const xiiColor& color, const xiiTransform& transform = xiiTransform::IdentityTransform());

  /// \brief Renders the given set of lines in 2D (screen-space) for one frame.
  static void Draw2DLines(const xiiDebugRendererContext& context, xiiArrayPtr<const Line> lines, const xiiColor& color);

  /// \brief Renders a cross for one frame.
  static void DrawCross(const xiiDebugRendererContext& context, const xiiVec3& vGlobalPosition, float fLineLength, const xiiColor& color, const xiiTransform& transform = xiiTransform::IdentityTransform());

  /// \brief Renders a wireframe box for one frame.
  static void DrawLineBox(const xiiDebugRendererContext& context, const xiiBoundingBox& box, const xiiColor& color, const xiiTransform& transform = xiiTransform::IdentityTransform());

  /// \brief Renders the corners of a wireframe box for one frame.
  static void DrawLineBoxCorners(const xiiDebugRendererContext& context, const xiiBoundingBox& box, float fCornerFraction, const xiiColor& color, const xiiTransform& transform = xiiTransform::IdentityTransform());

  /// \brief Renders a wireframe sphere for one frame.
  static void DrawLineSphere(const xiiDebugRendererContext& context, const xiiBoundingSphere& sphere, const xiiColor& color, const xiiTransform& transform = xiiTransform::IdentityTransform());

  /// \brief Renders an upright wireframe capsule for one frame.
  static void DrawLineCapsuleZ(const xiiDebugRendererContext& context, float fLength, float fRadius, const xiiColor& color, const xiiTransform& transform = xiiTransform::IdentityTransform());

  /// \brief Renders a wireframe frustum for one frame.
  static void DrawLineFrustum(const xiiDebugRendererContext& context, const xiiFrustum& frustum, const xiiColor& color, bool bDrawPlaneNormals = false);

  /// \brief Renders a solid box for one frame.
  static void DrawSolidBox(const xiiDebugRendererContext& context, const xiiBoundingBox& box, const xiiColor& color, const xiiTransform& transform = xiiTransform::IdentityTransform());

  /// \brief Renders the set of filled triangles for one frame.
  static void DrawSolidTriangles(const xiiDebugRendererContext& context, xiiArrayPtr<Triangle> triangles, const xiiColor& color);

  /// \brief Renders the set of textured triangles for one frame.
  static void DrawTexturedTriangles(const xiiDebugRendererContext& context, xiiArrayPtr<TexturedTriangle> triangles, const xiiColor& color, const xiiTexture2DResourceHandle& hTexture);

  /// \brief Renders a filled 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color);

  /// \brief Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color, const xiiTexture2DResourceHandle& hTexture, xiiVec2 vUvScale = xiiVec2(1, 1));

  /// \brief Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const xiiDebugRendererContext& context, const xiiRectFloat& rectInPixel, float fDepth, const xiiColor& color, xiiGALResourceViewHandle hResourceView, xiiVec2 vUvScale = xiiVec2(1, 1));

  /// \brief Displays a string in screen-space for one frame.
  ///
  /// The string may contain newlines (\n) for multi-line output.
  /// If horizontal alignment is right, the entire text block is aligned according to the longest line.
  /// If vertical alignment is bottom, the entire text block is aligned there.
  ///
  /// Data can be output as a table, by separating columns with tabs (\t). For example:
  /// "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|"
  ///
  /// Returns the number of lines that the text was split up into.
  static xiiUInt32 Draw2DText(const xiiDebugRendererContext& context, const xiiFormatString& text, const xiiVec2I32& vPositionInPixel, const xiiColor& color, xiiUInt32 uiSizeInPixel = 16, HorizontalAlignment horizontalAlignment = HorizontalAlignment::Left, VerticalAlignment verticalAlignment = VerticalAlignment::Top);

  /// \brief Draws a piece of text in one of the screen corners.
  ///
  /// Text positioning is automatic, all lines are placed in each corner such that they don't overlap.
  /// Text from different corners may overlap, though.
  ///
  /// For text formatting options, see Draw2DText().
  ///
  /// The \a groupName parameter is used to insert whitespace between unrelated pieces of text,
  /// it is not displayed anywhere, though.
  ///
  /// Text size cannot be changed.
  static void DrawInfoText(const xiiDebugRendererContext& context, ScreenPlacement placement, const char* szGroupName, const xiiFormatString& text, const xiiColor& color = xiiColor::White);

  /// \brief Displays a string in 3D space for one frame.
  static xiiUInt32 Draw3DText(const xiiDebugRendererContext& context, const xiiFormatString& text, const xiiVec3& vGlobalPosition, const xiiColor& color, xiiUInt32 uiSizeInPixel = 16, HorizontalAlignment horizontalAlignment = HorizontalAlignment::Center, VerticalAlignment verticalAlignment = VerticalAlignment::Bottom);

  /// \brief Renders a cross at the given location for as many frames until \a duration has passed.
  static void AddPersistentCross(const xiiDebugRendererContext& context, float fSize, const xiiColor& color, const xiiTransform& transform, xiiTime duration);

  /// \brief Renders a wireframe sphere at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineSphere(const xiiDebugRendererContext& context, float fRadius, const xiiColor& color, const xiiTransform& transform, xiiTime duration);

  /// \brief Renders a wireframe box at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineBox(const xiiDebugRendererContext& context, const xiiVec3& vHalfSize, const xiiColor& color, const xiiTransform& transform, xiiTime duration);

  /// \brief Renders a solid 2D cone in a plane with a given angle.
  ///
  /// The rotation goes around the given \a rotationAxis.
  /// An angle of zero is pointing into forwardAxis direction.
  /// Both angles may be negative.
  static void DrawAngle(const xiiDebugRendererContext& context, xiiAngle startAngle, xiiAngle endAngle, const xiiColor& solidColor, const xiiColor& lineColor, const xiiTransform& transform, xiiVec3 vForwardAxis = xiiVec3::UnitXAxis(), xiiVec3 vRotationAxis = xiiVec3::UnitZAxis());

  /// \brief Renders a cone with the tip at the center position, opening up with the given angle.
  static void DrawOpeningCone(const xiiDebugRendererContext& context, xiiAngle halfAngle, const xiiColor& colorInside, const xiiColor& colorOutside, const xiiTransform& transform, xiiVec3 vForwardAxis = xiiVec3::UnitXAxis());

  /// \brief Renders a bent cone with the tip at the center position, pointing into the +X direction opening up with halfAngle1 and halfAngle2 along the Y and Z axis.
  ///
  /// If solidColor.a > 0, the cone is rendered with as solid triangles.
  /// If lineColor.a > 0, the cone is rendered as lines.
  /// Both can be combined.
  static void DrawLimitCone(const xiiDebugRendererContext& context, xiiAngle halfAngle1, xiiAngle halfAngle2, const xiiColor& solidColor, const xiiColor& lineColor, const xiiTransform& transform);

  /// \brief Renders a cylinder starting at the center position, along the +X axis.
  ///
  /// If the start and end radius are different, a cone or arrow can be created.
  static void DrawCylinder(const xiiDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const xiiColor& solidColor, const xiiColor& lineColor, const xiiTransform& transform, bool bCapStart = false, bool bCapEnd = false);

private:
  friend class xiiSimpleRenderPass;

  static void Render(const xiiRenderViewContext& renderViewContext);
  static void RenderInternal(const xiiDebugRendererContext& context, const xiiRenderViewContext& renderViewContext);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, DebugRenderer);
};

#include <RendererCore/Debug/Implementation/DebugRenderer_inl.h>
