#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/VisualizeHandComponent.h>
#include <GameEngine/XR/XRHandTrackingInterface.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Debug/DebugRendererContext.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiVisualizeHandComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("XR"),
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Beta),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiVisualizeHandComponent::xiiVisualizeHandComponent()  = default;
xiiVisualizeHandComponent::~xiiVisualizeHandComponent() = default;

void xiiVisualizeHandComponent::Update()
{
  xiiXRHandTrackingInterface* pXRHand = xiiSingletonRegistry::GetSingletonInstance<xiiXRHandTrackingInterface>();

  if (!pXRHand)
    return;

  xiiHybridArray<xiiXRHandBone, 6> bones;
  for (xiiXRHand::Enum hand : {xiiXRHand::Left, xiiXRHand::Right})
  {
    for (xiiUInt32 uiPart = 0; uiPart < xiiXRHandPart::COUNT; ++uiPart)
    {
      xiiXRHandPart::Enum part = static_cast<xiiXRHandPart::Enum>(uiPart);
      if (pXRHand->TryGetBoneTransforms(hand, part, xiiXRTransformSpace::Global, bones) == xiiXRHandTrackingInterface::HandPartTrackingState::Tracked)
      {
        xiiHybridArray<xiiDebugRenderer::Line, 6> m_Lines;
        for (xiiUInt32 uiBone = 0; uiBone < bones.GetCount(); uiBone++)
        {
          const xiiXRHandBone& bone   = bones[uiBone];
          xiiBoundingSphere    sphere = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), bone.m_fRadius);
          xiiDebugRenderer::DrawLineSphere(GetWorld(), sphere, xiiColor::Aquamarine, bone.m_Transform);

          if (uiBone + 1 < bones.GetCount())
          {
            const xiiXRHandBone& nextBone = bones[uiBone + 1];
            m_Lines.PushBack(xiiDebugRenderer::Line(bone.m_Transform.m_vPosition, nextBone.m_Transform.m_vPosition));
          }
        }
        xiiDebugRenderer::DrawLines(GetWorld(), m_Lines, xiiColor::IndianRed);
      }
    }
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_VisualizeHandComponent);
