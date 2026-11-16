/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Serialization/NetworkDataError.h>

const char* xiiNetworkDataError::GetCodeName() const
{
  switch (m_Code)
  {
    case xiiNetworkDataErrorCode::Success:
      return "Success";
    case xiiNetworkDataErrorCode::InvalidArgument:
      return "InvalidArgument";
    case xiiNetworkDataErrorCode::UnknownType:
      return "UnknownType";
    case xiiNetworkDataErrorCode::UnsupportedProperty:
      return "UnsupportedProperty";
    case xiiNetworkDataErrorCode::InvalidAttribute:
      return "InvalidAttribute";
    case xiiNetworkDataErrorCode::IdentifierCollision:
      return "IdentifierCollision";
    case xiiNetworkDataErrorCode::RegistryUnavailable:
      return "RegistryUnavailable";
    case xiiNetworkDataErrorCode::SchemaMismatch:
      return "SchemaMismatch";
    case xiiNetworkDataErrorCode::MissingRequiredField:
      return "MissingRequiredField";
    case xiiNetworkDataErrorCode::InvalidFieldType:
      return "InvalidFieldType";
    case xiiNetworkDataErrorCode::InvalidEncoding:
      return "InvalidEncoding";
    case xiiNetworkDataErrorCode::Truncated:
      return "Truncated";
    case xiiNetworkDataErrorCode::SizeLimit:
      return "SizeLimit";
    case xiiNetworkDataErrorCode::AllocationLimit:
      return "AllocationLimit";
    case xiiNetworkDataErrorCode::UnsupportedNestedType:
      return "UnsupportedNestedType";
    case xiiNetworkDataErrorCode::CustomCodecFailure:
      return "CustomCodecFailure";
    case xiiNetworkDataErrorCode::Unauthorized:
      return "Unauthorized";
    case xiiNetworkDataErrorCode::UnknownMessage:
      return "UnknownMessage";
    case xiiNetworkDataErrorCode::UnknownRpc:
      return "UnknownRpc";
    case xiiNetworkDataErrorCode::UnsupportedSignature:
      return "UnsupportedSignature";
    case xiiNetworkDataErrorCode::TimedOut:
      return "TimedOut";
    case xiiNetworkDataErrorCode::Cancelled:
      return "Cancelled";
    case xiiNetworkDataErrorCode::Disconnected:
      return "Disconnected";
    case xiiNetworkDataErrorCode::Backpressure:
      return "Backpressure";
    case xiiNetworkDataErrorCode::HandlerRemoved:
      return "HandlerRemoved";
    case xiiNetworkDataErrorCode::ExecutionFailed:
      return "ExecutionFailed";
    case xiiNetworkDataErrorCode::TransportFailure:
      return "TransportFailure";
    case xiiNetworkDataErrorCode::NotNegotiated:
      return "NotNegotiated";
    case xiiNetworkDataErrorCode::DuplicateRequest:
      return "DuplicateRequest";
  }
  return "Unknown";
}
