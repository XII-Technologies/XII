#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiJoltShapeComponent, 1)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Shapes"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiJoltShapeComponent::xiiJoltShapeComponent()  = default;
xiiJoltShapeComponent::~xiiJoltShapeComponent() = default;

void xiiJoltShapeComponent::Initialize()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiJoltShapeComponent::OnDeactivated()
{
  if (m_uiUserDataIndex != xiiInvalidIndex)
  {
    xiiJoltWorldModule* pModule = GetWorld()->GetModule<xiiJoltWorldModule>();
    pModule->DeallocateUserData(m_uiUserDataIndex);
  }

  SUPER::OnDeactivated();
}

const xiiJoltUserData* xiiJoltShapeComponent::GetUserData()
{
  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  if (m_uiUserDataIndex != xiiInvalidIndex)
  {
    return &pModule->GetUserData(m_uiUserDataIndex);
  }
  else
  {
    xiiJoltUserData* pUserData = nullptr;
    m_uiUserDataIndex          = pModule->AllocateUserData(pUserData);
    pUserData->Init(this);

    return pUserData;
  }
}

xiiUInt32 xiiJoltShapeComponent::GetUserDataIndex()
{
  GetUserData();
  return m_uiUserDataIndex;
}
