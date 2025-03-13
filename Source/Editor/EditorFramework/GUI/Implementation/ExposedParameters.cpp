#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiExposedParameter, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiExposedParameter>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Type", m_sType),
    XII_MEMBER_PROPERTY("DefaultValue", m_DefaultValue),
    XII_ARRAY_MEMBER_PROPERTY("Attributes", m_Attributes)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

xiiExposedParameter::xiiExposedParameter() = default;

xiiExposedParameter::~xiiExposedParameter()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
  }
}

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExposedParameters, 3, xiiRTTIDefaultAllocator<xiiExposedParameters>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Parameters", m_Parameters)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExposedParameters::xiiExposedParameters() = default;

xiiExposedParameters::~xiiExposedParameters()
{
  for (auto pAttr : m_Parameters)
  {
    xiiGetStaticRTTI<xiiExposedParameter>()->GetAllocator()->Deallocate(pAttr);
  }
}

const xiiExposedParameter* xiiExposedParameters::Find(xiiStringView sParamName) const
{
  const xiiExposedParameter* const* pParam = std::find_if(cbegin(m_Parameters), cend(m_Parameters), [sParamName](const xiiExposedParameter* pParam) { return pParam->m_sName == sParamName; });
  return pParam != cend(m_Parameters) ? *pParam : nullptr;
}
