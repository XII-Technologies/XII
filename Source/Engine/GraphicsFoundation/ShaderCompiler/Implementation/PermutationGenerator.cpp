/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/ShaderCompiler/PermutationGenerator.h>

void xiiGALPermutationGenerator::Clear()
{
  m_Permutations.Clear();
}

void xiiGALPermutationGenerator::RemovePermutations(const xiiHashedString& sPermutationVariableName)
{
  m_Permutations.Remove(sPermutationVariableName);
}

void xiiGALPermutationGenerator::AddPermutation(const xiiHashedString& sName, const xiiHashedString& sValue)
{
  XII_ASSERT_DEV(!sName.IsEmpty(), "");
  XII_ASSERT_DEV(!sValue.IsEmpty(), "");

  m_Permutations[sName].Insert(sValue);
}

xiiUInt32 xiiGALPermutationGenerator::GetPermutationCount() const
{
  xiiUInt32 uiPermutations = 1;

  for (auto it = m_Permutations.GetIterator(); it.IsValid(); ++it)
  {
    uiPermutations *= it.Value().GetCount();
  }

  return uiPermutations;
}

void xiiGALPermutationGenerator::GetPermutation(xiiUInt32 uiPerm, xiiHybridArray<xiiGALPermutationVariable, 16>& out_permutationVariables) const
{
  out_permutationVariables.Clear();

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

    xiiGALPermutationVariable& pv = out_permutationVariables.ExpandAndGetRef();
    pv.m_sName                    = itVariable.Key();
    pv.m_sValue                   = itValue.Key();
  }
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_ShaderCompiler_Implementation_PermutationGenerator);
