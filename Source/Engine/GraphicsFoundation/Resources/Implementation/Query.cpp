#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Query.h>

xiiGALQuery::xiiGALQuery(const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALResource<xiiGALQueryCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALQuery::~xiiGALQuery() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Query);
