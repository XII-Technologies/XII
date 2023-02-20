#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>

class xiiVolumeCollection;

struct XII_PROCGENPLUGIN_DLL xiiProcGenExpressionFunctions
{
  static xiiExpressionFunction s_ApplyVolumesFunc;
  static xiiExpressionFunction s_GetInstanceSeedFunc;
};

namespace xiiProcGenInternal
{
  void ExtractVolumeCollections(const xiiWorld& world, const xiiBoundingBox& box, const Output& output, xiiDeque<xiiVolumeCollection>& volumeCollections, xiiExpression::GlobalData& globalData);

  void SetInstanceSeed(xiiUInt32 uiSeed, xiiExpression::GlobalData& globalData);
} // namespace xiiProcGenInternal
