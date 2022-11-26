#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

/// \brief A helper class to iterate over all possible permutations.
///
/// Just add all permutation variables and their possible values.
/// Then the number of possible permutations and each permutation
/// can be queried.
class XII_RENDERERCORE_DLL xiiPermutationGenerator
{
public:
  /// \brief Resets everything.
  void Clear();

  /// \brief Removes all permutations for the given variable
  void RemovePermutations(const xiiHashedString& sPermVarName);

  /// \brief Adds the name and one of the possible values of a permutation variable.
  void AddPermutation(const xiiHashedString& sName, const xiiHashedString& sValue);

  /// \brief Returns how many permutations are possible.
  xiiUInt32 GetPermutationCount() const;

  /// \brief Returns the n-th permutation.
  void GetPermutation(xiiUInt32 uiPerm, xiiHybridArray<xiiPermutationVar, 16>& out_PermVars) const;

private:
  xiiMap<xiiHashedString, xiiHashSet<xiiHashedString>> m_Permutations;
};
