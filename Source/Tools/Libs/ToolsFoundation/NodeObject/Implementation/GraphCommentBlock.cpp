/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/NodeObject/GraphCommentBlock.h>

xiiVariant xiiGraphCommentBlock::ToVariant() const
{
  xiiVariantDictionary data;
  data.Insert("Id", m_Id);
  data.Insert("Position", m_vPosition);
  data.Insert("Size", m_vSize);
  data.Insert("Text", m_sText);
  data.Insert("Color", m_Color);
  return data;
}

xiiStatus xiiGraphCommentBlock::FromVariant(const xiiVariant& value, xiiGraphCommentBlock& out_block)
{
  if (!value.IsA<xiiVariantDictionary>())
    return xiiStatus("A graph comment block must be a dictionary.");
  const auto& data     = value.Get<xiiVariantDictionary>();
  const auto* id       = data.GetValue("Id");
  const auto* position = data.GetValue("Position");
  const auto* size     = data.GetValue("Size");
  const auto* text     = data.GetValue("Text");
  const auto* color    = data.GetValue("Color");
  if (!id || !id->IsA<xiiUuid>() || !id->Get<xiiUuid>().IsValid() || !position || !position->IsA<xiiVec2>() || !position->Get<xiiVec2>().IsValid() ||
      !size || !size->IsA<xiiVec2>() || !size->Get<xiiVec2>().IsValid() || size->Get<xiiVec2>().x < 160 || size->Get<xiiVec2>().y < 80 ||
      !text || !text->IsA<xiiString>() || !color || !color->IsA<xiiColor>() || !color->Get<xiiColor>().IsValid())
    return xiiStatus("Graph comment block identity, bounds, text or color are invalid.");
  xiiGraphCommentBlock block;
  block.m_Id        = id->Get<xiiUuid>();
  block.m_vPosition = position->Get<xiiVec2>();
  block.m_vSize     = size->Get<xiiVec2>();
  block.m_sText     = text->Get<xiiString>();
  block.m_Color     = color->Get<xiiColor>();
  out_block         = std::move(block);
  return XII_SUCCESS;
}
