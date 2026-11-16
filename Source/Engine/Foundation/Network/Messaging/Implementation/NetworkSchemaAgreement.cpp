/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Messaging/Implementation/NetworkReflectionEndpointInternal.h>

namespace xiiNetworkMessagingInternal
{
  namespace
  {
    struct FieldView
    {
      xiiUInt64                   m_uiId = 0;
      xiiArrayPtr<const xiiUInt8> m_Shape, m_Nested;
      bool                        m_bOptional  = false;
      xiiUInt32                   m_uiElements = 0, m_uiString = 0;
    };
    xiiNetworkDataError Fields(xiiNetworkBinaryReader& reader, xiiHybridArray<FieldView, 16>& fields)
    {
      const auto count = reader.U32();
      if (count > 512)
        return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
      xiiUInt64 previous = 0;
      for (xiiUInt32 i = 0; i < count; ++i)
      {
        FieldView field;
        field.m_uiId         = reader.U64();
        const auto shapeSize = reader.U32();
        if (shapeSize > 16384)
          return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
        field.m_Shape         = reader.Bytes(shapeSize);
        const auto optional   = reader.U8();
        field.m_bOptional     = optional == 1;
        field.m_uiElements    = reader.U32();
        field.m_uiString      = reader.U32();
        const auto nestedSize = reader.U32();
        field.m_Nested        = reader.Bytes(nestedSize);
        if (reader.GetError().Failed())
          return reader.GetError();
        if (field.m_uiId <= previous || optional > 1 || field.m_Shape.IsEmpty())
          return xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
        previous = field.m_uiId;
        fields.PushBack(field);
      }
      return reader.IsFinished() ? xiiNetworkDataError() : xiiNetworkDataError(xiiNetworkDataErrorCode::InvalidEncoding);
    }
  } // namespace

  xiiNetworkDataError Compatible(xiiArrayPtr<const xiiUInt8> sender, xiiArrayPtr<const xiiUInt8> receiver, xiiUInt32 depth)
  {
    if (depth > 32)
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
    xiiNetworkBinaryReader left(sender), right(receiver);
    const auto             type = left.U64();
    if (type == 0 || type != right.U64())
      return xiiNetworkDataError(xiiNetworkDataErrorCode::SchemaMismatch);
    XII_IGNORE_UNUSED(left.U32());
    XII_IGNORE_UNUSED(right.U32());
    xiiHybridArray<FieldView, 16> from, to;
    auto                          error = Fields(left, from);
    if (error.Failed())
      return error;
    error = Fields(right, to);
    if (error.Failed())
      return error;
    xiiUInt32 cursor = 0;
    for (const auto& field : to)
    {
      while (cursor < from.GetCount() && from[cursor].m_uiId < field.m_uiId)
        ++cursor;
      if (cursor == from.GetCount() || from[cursor].m_uiId != field.m_uiId)
      {
        if (!field.m_bOptional)
          return xiiNetworkDataError(xiiNetworkDataErrorCode::MissingRequiredField, type, field.m_uiId);
        continue;
      }
      const auto& incoming = from[cursor];
      if (incoming.m_Shape != field.m_Shape || incoming.m_uiElements > field.m_uiElements || incoming.m_uiString > field.m_uiString ||
          incoming.m_Nested.IsEmpty() != field.m_Nested.IsEmpty())
        return xiiNetworkDataError(xiiNetworkDataErrorCode::SchemaMismatch, type, field.m_uiId);
      if (!field.m_Nested.IsEmpty())
      {
        error = Compatible(incoming.m_Nested, field.m_Nested, depth + 1);
        if (error.Failed())
          return error;
      }
    }
    return {};
  }

  xiiNetworkDataError Transport(xiiNetworkError error)
  {
    xiiNetworkDataError result;
    if (error.Failed())
    {
      result.m_Code      = xiiNetworkDataErrorCode::TransportFailure;
      result.m_Transport = error;
    }
    return result;
  }
  void Envelope(xiiNetworkBinaryWriter& writer, Kind kind)
  {
    writer.U32(Magic);
    writer.U8(static_cast<xiiUInt8>(kind));
  }
} // namespace xiiNetworkMessagingInternal
