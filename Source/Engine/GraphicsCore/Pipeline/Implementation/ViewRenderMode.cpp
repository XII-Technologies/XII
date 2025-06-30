#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/ViewRenderMode.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiViewRenderMode, 1)
  XII_ENUM_CONSTANT(xiiViewRenderMode::None)->AddAttributes(new xiiGroupAttribute("Default")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::WireframeColor)->AddAttributes(new xiiGroupAttribute("Wireframe")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::WireframeMonochrome),
  XII_ENUM_CONSTANT(xiiViewRenderMode::DiffuseLitOnly)->AddAttributes(new xiiGroupAttribute("Lighting")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::SpecularLitOnly),
  XII_ENUM_CONSTANT(xiiViewRenderMode::LightCount)->AddAttributes(new xiiGroupAttribute("Performance")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::DecalCount),
  XII_ENUM_CONSTANT(xiiViewRenderMode::StaticVsDynamic),
  XII_ENUM_CONSTANT(xiiViewRenderMode::TexCoordsUV0)->AddAttributes(new xiiGroupAttribute("TexCoords")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::TexCoordsUV1),
  XII_ENUM_CONSTANT(xiiViewRenderMode::VertexColors0)->AddAttributes(new xiiGroupAttribute("VertexColors")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::VertexColors1),
  XII_ENUM_CONSTANT(xiiViewRenderMode::VertexNormals)->AddAttributes(new xiiGroupAttribute("Normals")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::VertexTangents),
  XII_ENUM_CONSTANT(xiiViewRenderMode::PixelNormals),
  XII_ENUM_CONSTANT(xiiViewRenderMode::DiffuseColor)->AddAttributes(new xiiGroupAttribute("PixelColors")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::DiffuseColorRange),
  XII_ENUM_CONSTANT(xiiViewRenderMode::SpecularColor),
  XII_ENUM_CONSTANT(xiiViewRenderMode::EmissiveColor),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Roughness)->AddAttributes(new xiiGroupAttribute("Surface")),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Occlusion),
  XII_ENUM_CONSTANT(xiiViewRenderMode::Depth),
  XII_ENUM_CONSTANT(xiiViewRenderMode::BoneWeights)->AddAttributes(new xiiGroupAttribute("Animation")),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
xiiTempHashedString xiiViewRenderMode::GetPermutationValue(Enum renderMode)
{
  if (renderMode >= WireframeColor && renderMode <= WireframeMonochrome)
  {
    return "RENDER_PASS_WIREFRAME";
  }
  else if (renderMode >= DiffuseLitOnly && renderMode < ENUM_COUNT)
  {
    return "RENDER_PASS_EDITOR";
  }

  return {};
}

// static
xiiInt32 xiiViewRenderMode::GetRenderPassForShader(Enum renderMode)
{
  switch (renderMode)
  {
    case xiiViewRenderMode::None:
      return -1;

    case xiiViewRenderMode::WireframeColor:
      return WIREFRAME_RENDER_PASS_COLOR;

    case xiiViewRenderMode::WireframeMonochrome:
      return WIREFRAME_RENDER_PASS_MONOCHROME;

    case xiiViewRenderMode::DiffuseLitOnly:
      return EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY;

    case xiiViewRenderMode::SpecularLitOnly:
      return EDITOR_RENDER_PASS_SPECULAR_LIT_ONLY;

    case xiiViewRenderMode::LightCount:
      return EDITOR_RENDER_PASS_LIGHT_COUNT;

    case xiiViewRenderMode::DecalCount:
      return EDITOR_RENDER_PASS_DECAL_COUNT;

    case xiiViewRenderMode::TexCoordsUV0:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV0;

    case xiiViewRenderMode::TexCoordsUV1:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV1;

    case xiiViewRenderMode::VertexColors0:
      return EDITOR_RENDER_PASS_VERTEX_COLORS0;

    case xiiViewRenderMode::VertexColors1:
      return EDITOR_RENDER_PASS_VERTEX_COLORS1;

    case xiiViewRenderMode::VertexNormals:
      return EDITOR_RENDER_PASS_VERTEX_NORMALS;

    case xiiViewRenderMode::VertexTangents:
      return EDITOR_RENDER_PASS_VERTEX_TANGENTS;

    case xiiViewRenderMode::PixelNormals:
      return EDITOR_RENDER_PASS_PIXEL_NORMALS;

    case xiiViewRenderMode::DiffuseColor:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR;

    case xiiViewRenderMode::DiffuseColorRange:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE;

    case xiiViewRenderMode::SpecularColor:
      return EDITOR_RENDER_PASS_SPECULAR_COLOR;

    case xiiViewRenderMode::EmissiveColor:
      return EDITOR_RENDER_PASS_EMISSIVE_COLOR;

    case xiiViewRenderMode::Roughness:
      return EDITOR_RENDER_PASS_ROUGHNESS;

    case xiiViewRenderMode::Occlusion:
      return EDITOR_RENDER_PASS_OCCLUSION;

    case xiiViewRenderMode::Depth:
      return EDITOR_RENDER_PASS_DEPTH;

    case xiiViewRenderMode::StaticVsDynamic:
      return EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC;

    case xiiViewRenderMode::BoneWeights:
      return EDITOR_RENDER_PASS_BONE_WEIGHTS;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return -1;
  }
}

// static
void xiiViewRenderMode::GetDebugText(Enum renderMode, xiiStringBuilder& out_sDebugText)
{
  if (renderMode == DiffuseColorRange)
  {
    out_sDebugText = "Pure magenta means the diffuse color is too dark, pure green means it is too bright.";
  }
  else if (renderMode == StaticVsDynamic)
  {
    out_sDebugText = "Static objects are shown in green, dynamic objects are shown in red.";
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_ViewRenderMode);
