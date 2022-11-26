#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiGraphPatch);

xiiGraphPatch::xiiGraphPatch(const char* szType, xiiUInt32 uiTypeVersion, PatchType type) :
  m_szType(szType), m_uiTypeVersion(uiTypeVersion), m_PatchType(type)
{
}

const char* xiiGraphPatch::GetType() const
{
  return m_szType;
}

xiiUInt32 xiiGraphPatch::GetTypeVersion() const
{
  return m_uiTypeVersion;
}


xiiGraphPatch::PatchType xiiGraphPatch::GetPatchType() const
{
  return m_PatchType;
}

XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_GraphPatch);
