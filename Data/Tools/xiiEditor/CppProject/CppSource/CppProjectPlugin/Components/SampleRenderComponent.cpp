#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <CppProjectPlugin/Components/SampleRenderComponent.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(SampleRenderComponentMask, 1)
  XII_BITFLAGS_CONSTANT(SampleRenderComponentMask::Box),
  XII_BITFLAGS_CONSTANT(SampleRenderComponentMask::Sphere),
  XII_BITFLAGS_CONSTANT(SampleRenderComponentMask::Cross),
  XII_BITFLAGS_CONSTANT(SampleRenderComponentMask::Quad)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_COMPONENT_TYPE(SampleRenderComponent, 1 /* version */, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new xiiDefaultValueAttribute(1), new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::White)),
    XII_ACCESSOR_PROPERTY("Texture", GetTextureFile, SetTextureFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    XII_BITFLAGS_MEMBER_PROPERTY("Render", SampleRenderComponentMask, m_RenderTypes)->AddAttributes(new xiiDefaultValueAttribute(SampleRenderComponentMask::Box)),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("CppProject"), // Component menu group
  }
  XII_END_ATTRIBUTES;

  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgSetColor, OnSetColor)
  }
  XII_END_MESSAGEHANDLERS;

  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetRandomColor)
  }
  XII_END_FUNCTIONS;
}
XII_END_COMPONENT_TYPE
// clang-format on

SampleRenderComponent::SampleRenderComponent() = default;
SampleRenderComponent::~SampleRenderComponent() = default;

void SampleRenderComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fSize;
  s << m_Color;
  s << m_hTexture;
  s << m_RenderTypes;
}

void SampleRenderComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fSize;
  s >> m_Color;
  s >> m_hTexture;
  s >> m_RenderTypes;
}

void SampleRenderComponent::SetTexture(const xiiTexture2DResourceHandle& hTexture)
{
  m_hTexture = hTexture;
}

const xiiTexture2DResourceHandle& SampleRenderComponent::GetTexture() const
{
  return m_hTexture;
}

void SampleRenderComponent::SetTextureFile(const char* szFile)
{
  xiiTexture2DResourceHandle hTexture;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(szFile);
  }

  SetTexture(hTexture);
}

const char* SampleRenderComponent::GetTextureFile(void) const
{
  if (m_hTexture.IsValid())
    return m_hTexture.GetResourceID();

  return nullptr;
}

void SampleRenderComponent::OnSetColor(xiiMsgSetColor& msg)
{
  m_Color = msg.m_Color;
}

void SampleRenderComponent::SetRandomColor()
{
  xiiRandom& rng = GetWorld()->GetRandomNumberGenerator();

  m_Color.r = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.g = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.b = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
}

void SampleRenderComponent::Update()
{
  const xiiTransform ownerTransform = GetOwner()->GetGlobalTransform();

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Box))
  {
    xiiBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(xiiVec3::ZeroVector(), xiiVec3(m_fSize));

    xiiDebugRenderer::DrawLineBox(GetWorld(), bbox, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Cross))
  {
    xiiDebugRenderer::DrawCross(GetWorld(), xiiVec3::ZeroVector(), m_fSize, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Sphere))
  {
    xiiBoundingSphere sphere;
    sphere.SetElements(xiiVec3::ZeroVector(), m_fSize);
    xiiDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Quad) && m_hTexture.IsValid())
  {
    xiiHybridArray<xiiDebugRenderer::TexturedTriangle, 16> triangles;

    {
      auto& t0 = triangles.ExpandAndGetRef();

      t0.m_position[0].Set(0, -m_fSize, +m_fSize);
      t0.m_position[1].Set(0, +m_fSize, -m_fSize);
      t0.m_position[2].Set(0, -m_fSize, -m_fSize);

      t0.m_texcoord[0].Set(0.0f, 0.0f);
      t0.m_texcoord[1].Set(1.0f, 1.0f);
      t0.m_texcoord[2].Set(0.0f, 1.0f);
    }

    {
      auto& t1 = triangles.ExpandAndGetRef();

      t1.m_position[0].Set(0, -m_fSize, +m_fSize);
      t1.m_position[1].Set(0, +m_fSize, +m_fSize);
      t1.m_position[2].Set(0, +m_fSize, -m_fSize);

      t1.m_texcoord[0].Set(0.0f, 0.0f);
      t1.m_texcoord[1].Set(1.0f, 0.0f);
      t1.m_texcoord[2].Set(1.0f, 1.0f);
    }

    // move the triangles into our object space
    for (auto& tri : triangles)
    {
      tri.m_position[0] = ownerTransform.TransformPosition(tri.m_position[0]);
      tri.m_position[1] = ownerTransform.TransformPosition(tri.m_position[1]);
      tri.m_position[2] = ownerTransform.TransformPosition(tri.m_position[2]);
    }

    xiiDebugRenderer::DrawTexturedTriangles(GetWorld(), triangles, m_Color, m_hTexture);
  }
}
