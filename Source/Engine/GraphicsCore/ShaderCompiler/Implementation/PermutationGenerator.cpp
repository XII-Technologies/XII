#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/ShaderCompiler/PermutationGenerator.h>

void xiiPermutationGenerator::Clear()
{
  m_Permutations.Clear();
}

void xiiPermutationGenerator::RemovePermutations(const xiiHashedString& sPermVarName)
{
  m_Permutations.Remove(sPermVarName);
}

void xiiPermutationGenerator::AddPermutation(const xiiHashedString& sName, const xiiHashedString& sValue)
{
  XII_ASSERT_DEV(!sName.IsEmpty(), "");
  XII_ASSERT_DEV(!sValue.IsEmpty(), "");

  m_Permutations[sName].Insert(sValue);
}

xiiUInt32 xiiPermutationGenerator::GetPermutationCount() const
{
  xiiUInt32 uiPermutations = 1;

  for (auto it = m_Permutations.GetIterator(); it.IsValid(); ++it)
  {
    uiPermutations *= it.Value().GetCount();
  }

  return uiPermutations;
}

void xiiPermutationGenerator::GetPermutation(xiiUInt32 uiPerm, xiiHybridArray<xiiPermutationVar, 16>& out_permVars) const
{
  out_permVars.Clear();

  for (auto itVariable = m_Permutations.GetIterator(); itVariable.IsValid(); ++itVariable)
  {
    const xiiUInt32 uiValues   = itVariable.Value().GetCount();
    xiiUInt32       uiUseValue = uiPerm % uiValues;

    uiPerm /= uiValues;

    auto itValue = itVariable.Value().GetIterator();

    for (; uiUseValue > 0; --uiUseValue)
    {
      ++itValue;
    }

    xiiPermutationVar& pv = out_permVars.ExpandAndGetRef();
    pv.m_sName            = itVariable.Key();
    pv.m_sValue           = itValue.Key();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_ShaderCompiler_Implementation_PermutationGenerator);
