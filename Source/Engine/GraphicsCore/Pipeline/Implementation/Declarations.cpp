#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

void xiiRenderViewContext::SetShaderPermutationVariable(const char* szName, const xiiTempHashedString& sTempValue) const
{
  xiiTempHashedString sHashedName(szName);

  xiiHashedString sName, sValue;
  if (xiiGALShaderManager::IsPermutationValueAllowed(szName, sHashedName, sTempValue, sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void xiiRenderViewContext::SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sTempValue) const
{
  xiiTempHashedString sHashedName(sName);

  xiiHashedString sName0, sValue;
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sHashedName, sTempValue, sName0, sValue))
  {
    SetShaderPermutationVariableInternal(sName0, sValue);
  }
}

void xiiRenderViewContext::SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue) const
{
  if (xiiGALShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void xiiRenderViewContext::SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue) const
{
  xiiHashedString* pOldValue = nullptr;
  m_PermutationVariables.TryGetValue(sName, pOldValue);
 
  if (pOldValue == nullptr || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);
  }
}
