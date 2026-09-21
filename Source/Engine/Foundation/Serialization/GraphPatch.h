/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Utilities/EnumerableClass.h>

class xiiRTTI;
class xiiAbstractObjectNode;
class xiiAbstractObjectGraph;
class xiiGraphVersioning;
class xiiGraphPatchContext;

/// Patch base class for xiiAbstractObjectGraph patches.
///
/// Create static instance of derived class to automatically patch graphs on load.
class XII_FOUNDATION_DLL xiiGraphPatch : public xiiEnumerable<xiiGraphPatch>
{
public:
  enum class PatchType : xiiUInt8
  {
    NodePatch,  ///< Patch applies to a node of a certain type and version
    GraphPatch, ///< Patch applies to an entire graph without any restrictions.
  };

  /// Constructor. pType is the type to patch. uiTypeVersion is the version to patch to.
  ///
  /// Patches are executed in order from version uiTypeVersion-1 to uiTypeVersion. If no patch exists for previous versions
  /// the input to the patch function can potentially be of a lower version than uiTypeVersion-1.
  /// If type is PatchType::NodePatch, the patch is executed for each instance of the given type.
  /// If type is PatchType::GraphPatch, the patch is executed once for the entire graph. In this case
  /// szType and uiTypeVersion are ignored and the patch function has to figure out what to do by itself.
  xiiGraphPatch(xiiStringView sType, xiiUInt32 uiTypeVersion, PatchType type = PatchType::NodePatch);

  /// Patch function. If type == PatchType::NodePatch, the implementation needs to patch pNode in pGraph to m_uiTypeVersion.
  ///  If type == PatchType::GraphPatch, pNode will be nullptr and the implementation has to figure out what to patch in pGraph on its own.
  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const = 0;

  /// Returns the type to patch.
  xiiStringView GetType() const;

  /// Returns the type version to patch to.
  xiiUInt32 GetTypeVersion() const;
  PatchType GetPatchType() const;

  XII_DECLARE_ENUMERABLE_CLASS(xiiGraphPatch);

private:
  xiiStringView m_sType;
  xiiUInt32     m_uiTypeVersion;
  PatchType     m_PatchType;
};
