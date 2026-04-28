/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiGraphPatch);

xiiGraphPatch::xiiGraphPatch(xiiStringView sType, xiiUInt32 uiTypeVersion, PatchType type) :
  m_sType(sType), m_uiTypeVersion(uiTypeVersion), m_PatchType(type)
{
}

xiiStringView xiiGraphPatch::GetType() const
{
  return m_sType;
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
