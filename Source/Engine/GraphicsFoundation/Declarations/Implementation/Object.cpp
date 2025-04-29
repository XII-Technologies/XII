#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Declarations/Object.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALObject, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

xiiGALObject::xiiGALObject() = default;

xiiGALObject::~xiiGALObject() = default;

void xiiGALObject::SetDebugName(xiiStringView sDebugName)
{
  m_sDebugName.Assign(sDebugName);

  SetDebugNamePlatform(m_sDebugName.GetView());
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Declarations_Implementation_Object);
