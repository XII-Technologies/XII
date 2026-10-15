/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graph/GraphComposition.h>
#include <Foundation/Types/SharedPtr.h>

/// Immutable expanded composition. Holding a snapshot preserves its topology and provenance
/// while the authoring definitions or cache are replaced. Runtime consumer state is not stored here.
struct xiiGraphCompiledComposition : xiiRefCounted
{
  xiiGraphExpansion m_Expansion;
};

/// Optional CPU composition cache, separate from graph storage and mutation. Synchronize access
/// externally. Publishers must advance the appropriate revision when changing a definition;
/// advance DefinitionRevision when replacing a document without finer change classification.
/// Presentation changes are ignored. Instance parameter/override edits belong to their containing
/// definition's Parameters/Resources/Topology revision, according to consumer semantics.
class xiiGraphCompositionCache
{
public:
  xiiResult Compile(const xiiGraphDefinition& root, xiiGraphComposition::Resolver resolver, xiiSharedPtr<const xiiGraphCompiledComposition>& out_composition, bool* out_reused = nullptr, xiiStringBuilder* out_error = nullptr)
  {
    if (out_reused)
      *out_reused = false;
    const auto resolveRoot = [&](const xiiUuid& id) -> const xiiGraphDefinition* {
      if (id == root.m_Id)
        return &root;
      return resolver.IsValid() ? resolver(id) : nullptr;
    };
    if (m_pCompiled && m_Root == root.m_Id && xiiGraphComposition::IsCurrent(m_pCompiled->m_Expansion.m_Dependencies, resolveRoot))
    {
      out_composition = m_pCompiled;
      if (out_reused)
        *out_reused = true;
      if (out_error)
        out_error->Clear();
      return XII_SUCCESS;
    }
    xiiSharedPtr<xiiGraphCompiledComposition> compiled = XII_DEFAULT_NEW(xiiGraphCompiledComposition);
    XII_SUCCEED_OR_RETURN(xiiGraphComposition::Expand(root, resolveRoot, compiled->m_Expansion, out_error));
    m_Root          = root.m_Id;
    m_pCompiled     = compiled;
    out_composition = std::move(compiled);
    return XII_SUCCESS;
  }
  void Clear()
  {
    m_pCompiled = nullptr;
    m_Root      = {};
  }

private:
  xiiUuid                                         m_Root;
  xiiSharedPtr<const xiiGraphCompiledComposition> m_pCompiled;
};
