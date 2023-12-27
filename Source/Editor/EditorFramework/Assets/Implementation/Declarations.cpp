#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/Declarations.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTransformResult, 1)
  XII_ENUM_CONSTANT(xiiTransformResult::Success),
  XII_ENUM_CONSTANT(xiiTransformResult::Failure),
  XII_ENUM_CONSTANT(xiiTransformResult::NeedsImport),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTransformStatus, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTransformStatus>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Result", xiiTransformResult, m_Result),
    XII_MEMBER_PROPERTY("Message", m_sMessage),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on
