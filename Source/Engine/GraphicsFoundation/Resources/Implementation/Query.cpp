#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Query.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALQuery, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

// clang-format on

xiiGALQuery::xiiGALQuery(const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALResource<xiiGALQueryCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALQuery::~xiiGALQuery() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Query);
