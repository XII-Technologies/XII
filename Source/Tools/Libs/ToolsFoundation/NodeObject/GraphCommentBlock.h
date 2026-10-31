/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

/// Editor-only organization. Bounds are in graph coordinates. Blocks do not
/// own domain nodes or participate in graph compilation or transform hashes.
struct XII_TOOLSFOUNDATION_DLL xiiGraphCommentBlock
{
  xiiUuid   m_Id;
  xiiVec2   m_vPosition = xiiVec2::MakeZero();
  xiiVec2   m_vSize     = xiiVec2(400, 240);
  xiiString m_sText     = "Comment";
  xiiColor  m_Color     = xiiColor(0.18f, 0.28f, 0.4f);

  xiiVariant       ToVariant() const;
  static xiiStatus FromVariant(const xiiVariant& value, xiiGraphCommentBlock& out_block);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_TOOLSFOUNDATION_DLL, xiiGraphCommentBlock);
