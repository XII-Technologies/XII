#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/PostProcessVolumeComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiPostProcessVolumeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ColourTint", m_ColourTint),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/PostProcess"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiPostProcessVolumeComponent::xiiPostProcessVolumeComponent()  = default;
xiiPostProcessVolumeComponent::~xiiPostProcessVolumeComponent() = default;

void xiiPostProcessVolumeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_ColourTint;
}

void xiiPostProcessVolumeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_ColourTint;
}
