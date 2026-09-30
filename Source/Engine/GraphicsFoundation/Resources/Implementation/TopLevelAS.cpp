/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/TopLevelAS.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALHitGroupBindingMode, 1)
  XII_ENUM_CONSTANT(xiiGALHitGroupBindingMode::PerGeometry),
  XII_ENUM_CONSTANT(xiiGALHitGroupBindingMode::PerInstance),
  XII_ENUM_CONSTANT(xiiGALHitGroupBindingMode::PerTopLevelAccelerationStructure),
  XII_ENUM_CONSTANT(xiiGALHitGroupBindingMode::UserDefined),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALRayTracingInstanceFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiGALRayTracingInstanceFlags::TriangleCullDisable, xiiGALRayTracingInstanceFlags::TriangleFrontCounterClockwise)
  XII_BITFLAGS_CONSTANTS(xiiGALRayTracingInstanceFlags::ForceOpaque, xiiGALRayTracingInstanceFlags::ForceNonOpaque)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALTLASInstanceData, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGALTLASInstanceData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Transform", GetTransform, SetTransform),
    XII_ACCESSOR_PROPERTY("InstanceID", GetInstanceID, SetInstanceID),
    XII_ACCESSOR_PROPERTY("Mask", GetMask, SetMask),
    XII_ACCESSOR_PROPERTY("HitGroupContribution", GetHitGroupContribution, SetHitGroupContribution),
    XII_ACCESSOR_PROPERTY("Flags", GetFlagsValue, SetFlagsValue),
    XII_MEMBER_PROPERTY("BottomLevelASDeviceAddress", m_uiBottomLevelASDeviceAddress),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTopLevelAS, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMat4 xiiGALTLASInstanceData::GetTransform() const
{
  xiiMat4 transform = xiiMat4::MakeIdentity();
  transform.SetRow(0U, m_TransformRow0);
  transform.SetRow(1U, m_TransformRow1);
  transform.SetRow(2U, m_TransformRow2);
  return transform;
}

void xiiGALTLASInstanceData::SetTransform(xiiMat4 transform)
{
  m_TransformRow0 = transform.GetRow(0U);
  m_TransformRow1 = transform.GetRow(1U);
  m_TransformRow2 = transform.GetRow(2U);
}

void xiiGALTLASInstanceData::SetInstanceID(xiiUInt32 uiInstanceID)
{
  XII_ASSERT_DEV(uiInstanceID <= 0x00FFFFFFU, "Ray tracing instance IDs are limited to 24 bits.");
  m_uiInstanceIDAndMask = (m_uiInstanceIDAndMask & 0xFF000000U) | (uiInstanceID & 0x00FFFFFFU);
}

void xiiGALTLASInstanceData::SetMask(xiiUInt8 uiMask)
{
  m_uiInstanceIDAndMask = (m_uiInstanceIDAndMask & 0x00FFFFFFU) | (static_cast<xiiUInt32>(uiMask) << 24U);
}

void xiiGALTLASInstanceData::SetHitGroupContribution(xiiUInt32 uiContribution)
{
  XII_ASSERT_DEV(uiContribution <= 0x00FFFFFFU, "Ray tracing hit-group contributions are limited to 24 bits.");
  m_uiHitGroupContributionAndFlags = (m_uiHitGroupContributionAndFlags & 0xFF000000U) | (uiContribution & 0x00FFFFFFU);
}

void xiiGALTLASInstanceData::SetFlags(xiiBitflags<xiiGALRayTracingInstanceFlags> flags)
{
  m_uiHitGroupContributionAndFlags = (m_uiHitGroupContributionAndFlags & 0x00FFFFFFU) | (static_cast<xiiUInt32>(flags.GetValue()) << 24U);
}

xiiGALTopLevelAS::xiiGALTopLevelAS(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALResource(std::move(pDevice)), m_Description(creationDescription)
{
}

xiiGALTopLevelAS::~xiiGALTopLevelAS() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_TopLevelAS);
