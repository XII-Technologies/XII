#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionFunctions.h>

class xiiVolumeCollection;

struct XII_PROCGENPLUGIN_DLL xiiProcGenExpressionFunctions
{
  static void      ApplyVolumes(xiiExpression::Inputs inputs, xiiExpression::Output output, const xiiExpression::GlobalData& globalData);
  static xiiResult ApplyVolumesValidate(const xiiExpression::GlobalData& globalData);
};

namespace xiiProcGenInternal
{
  void ExtractVolumeCollections(const xiiWorld& world, const xiiBoundingBox& box, const Output& output, xiiDeque<xiiVolumeCollection>& volumeCollections, xiiExpression::GlobalData& globalData);
}
