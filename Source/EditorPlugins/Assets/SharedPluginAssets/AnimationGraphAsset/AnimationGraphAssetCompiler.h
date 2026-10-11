/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

class xiiStringBuilder;
class xiiDocumentNodeManager;
struct xiiAnimationGraphEditorTransition;

/// Validate native animation graph authoring before publishing a runtime resource.
/// The source is never modified. Failure clears the output and reports the first
/// structural error. Aliased input/output is rejected without modifying either. Pose dependencies must be acyclic, including disconnected nodes.
/// Evaluation depth is limited to 128 because the current runtime evaluates recursively.
/// Transitions retain their authored order, which determines priority at runtime.
class XII_SHAREDPLUGINASSETS_DLL xiiAnimationGraphAssetCompiler
{
public:
  /// Compile document pose pins in their displayed order, resolving named transitions.
  /// Requires exactly one Output node. Empty node names use the stable document UUID.
  /// Nodes are ordered by document UUID so remove/undo and serialization retain indices.
  static xiiResult CompileDocument(const xiiDocumentNodeManager& document, xiiArrayPtr<const xiiAnimationGraphEditorTransition> transitions,
                                   xiiAnimGraphResourceDescriptor& out_descriptor, xiiStringBuilder* out_pError = nullptr);

  static xiiResult Compile(const xiiAnimGraphResourceDescriptor& source, xiiAnimGraphResourceDescriptor& out_descriptor,
                           xiiStringBuilder* out_pError = nullptr);
};
