/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>

/// A helper class to iterate over all possible permutations.
///
/// Just add all permutation variables and their possible values.
/// Then the number of possible permutations and each permutation can be queried.
class XII_GRAPHICSFOUNDATION_DLL xiiGALPermutationGenerator
{
public:
  /// Resets everything.
  void Clear();

  /// Removes all permutations for the given variable
  void RemovePermutations(const xiiHashedString& sPermutationVariableName);

  /// Adds the name and one of the possible values of a permutation variable.
  void AddPermutation(const xiiHashedString& sName, const xiiHashedString& sValue);

  /// Returns how many permutations are possible.
  xiiUInt32 GetPermutationCount() const;

  /// Returns the n-th permutation.
  void GetPermutation(xiiUInt32 uiPerm, xiiHybridArray<xiiGALPermutationVariable, 16>& out_permutationVariables) const;

private:
  xiiMap<xiiHashedString, xiiHashSet<xiiHashedString>> m_Permutations;
};
