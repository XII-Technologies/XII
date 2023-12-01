#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Fence.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALFence, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALFenceType, 1)
  XII_ENUM_CONSTANT(xiiGALFenceType::CpuWaitOnly),
  XII_ENUM_CONSTANT(xiiGALFenceType::General),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on

xiiGALFence::xiiGALFence(const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALResource<xiiGALFenceCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALFence::~xiiGALFence() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Fence);
