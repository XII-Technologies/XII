#pragma once

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Declarations/Object.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALObject, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE

void xiiGALObject::SetDebugName(xiiStringView sDebugName)
{
  m_sDebugName.Assign(sDebugName);

  SetDebugNamePlatform(m_sDebugName.GetView());
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Declarations_Implementation_Object);
