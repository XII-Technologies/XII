/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Config/ProjectManifest.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace
{
  constexpr xiiStringView s_sLegacyMarker         = "xiiEditor Project File";
  constexpr xiiUInt64     s_uiMaximumManifestSize = 1024 * 1024;

  struct StringField
  {
    xiiStringView m_sName;
    xiiString xiiProjectManifest::* m_pMember;
  };

  const StringField s_StringFields[] = {
    {"Name", &xiiProjectManifest::m_sName},
    {"Description", &xiiProjectManifest::m_sDescription},
    {"DataDirectories", &xiiProjectManifest::m_sDataDirectories},
    {"PluginSelection", &xiiProjectManifest::m_sPluginSelection},
    {"RuntimePlugins", &xiiProjectManifest::m_sRuntimePlugins},
    {"AssetProfiles", &xiiProjectManifest::m_sAssetProfiles},
    {"CppSettings", &xiiProjectManifest::m_sCppSettings},
    {"ActiveAssetProfile", &xiiProjectManifest::m_sActiveAssetProfile},
    {"DefaultScene", &xiiProjectManifest::m_sDefaultScene},
  };

  bool IsPortablePath(xiiStringView path, bool bAllowEmpty = false)
  {
    if (path.IsEmpty())
      return bAllowEmpty;
    xiiStringBuilder clean(path);
    clean.MakeCleanPath();
    return !clean.IsEmpty() && !clean.IsAbsolutePath() && !clean.IsRootedPath() && !clean.StartsWith("..") &&
      clean.FindSubString(":") == nullptr && clean.FindSubString(">") == nullptr && clean == path;
  }

  // Preserve optional extension objects, including primitive types unknown to this schema.
  void CopyElement(xiiOpenDdlWriter& writer, const xiiOpenDdlReaderElement& element)
  {
    if (element.IsCustomType())
    {
      writer.BeginObject(element.GetCustomType(), element.GetName(), element.IsNameGlobal());
      for (auto* child = element.GetFirstChild(); child; child = child->GetSibling())
        CopyElement(writer, *child);
      writer.EndObject();
      return;
    }
    writer.BeginPrimitiveList(element.GetPrimitivesType(), element.GetName(), element.IsNameGlobal());
    const auto count = element.GetNumPrimitives();
    switch (element.GetPrimitivesType())
    {
      case xiiOpenDdlPrimitiveType::Bool:
        writer.WriteBool(element.GetPrimitivesBool(), count);
        break;
      case xiiOpenDdlPrimitiveType::Int8:
        writer.WriteInt8(element.GetPrimitivesInt8(), count);
        break;
      case xiiOpenDdlPrimitiveType::Int16:
        writer.WriteInt16(element.GetPrimitivesInt16(), count);
        break;
      case xiiOpenDdlPrimitiveType::Int32:
        writer.WriteInt32(element.GetPrimitivesInt32(), count);
        break;
      case xiiOpenDdlPrimitiveType::Int64:
        writer.WriteInt64(element.GetPrimitivesInt64(), count);
        break;
      case xiiOpenDdlPrimitiveType::UInt8:
        writer.WriteUInt8(element.GetPrimitivesUInt8(), count);
        break;
      case xiiOpenDdlPrimitiveType::UInt16:
        writer.WriteUInt16(element.GetPrimitivesUInt16(), count);
        break;
      case xiiOpenDdlPrimitiveType::UInt32:
        writer.WriteUInt32(element.GetPrimitivesUInt32(), count);
        break;
      case xiiOpenDdlPrimitiveType::UInt64:
        writer.WriteUInt64(element.GetPrimitivesUInt64(), count);
        break;
      case xiiOpenDdlPrimitiveType::Float:
        writer.WriteFloat(element.GetPrimitivesFloat(), count);
        break;
      case xiiOpenDdlPrimitiveType::Double:
        writer.WriteDouble(element.GetPrimitivesDouble(), count);
        break;
      case xiiOpenDdlPrimitiveType::String:
        for (xiiUInt32 i = 0; i < count; ++i)
          writer.WriteString(element.GetPrimitivesString()[i]);
        break;
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        break;
    }
    writer.EndPrimitiveList();
  }
} // namespace

void xiiProjectManifest::Initialize(xiiStringView sProjectFile)
{
  *this       = xiiProjectManifest();
  m_ProjectID = xiiUuid::MakeUuid();
  xiiStringBuilder directory(sProjectFile);
  directory.PathParentDirectory();
  directory.Trim("", "/");
  m_sName = directory.GetFileNameAndExtension();
  if (m_sName.IsEmpty())
    m_sName = "Project";
  m_uiEngineMajor = BUILDSYSTEM_SDKVERSION_MAJOR;
  m_uiEngineMinor = BUILDSYSTEM_SDKVERSION_MINOR;
  m_uiEnginePatch = BUILDSYSTEM_SDKVERSION_PATCH;
}

xiiStatus xiiProjectManifest::Validate() const
{
  if (!m_ProjectID.IsValid() || m_sName.IsEmpty())
    return xiiStatus("Project requires a nonzero UUID and a display name.");
  for (const auto& field : s_StringFields)
  {
    if (field.m_pMember == &xiiProjectManifest::m_sName || field.m_pMember == &xiiProjectManifest::m_sDescription ||
        field.m_pMember == &xiiProjectManifest::m_sActiveAssetProfile)
      continue;
    if (!IsPortablePath(this->*field.m_pMember, field.m_pMember == &xiiProjectManifest::m_sDefaultScene))
      return xiiStatus(xiiFmt("Project field '{}' must be a normalized project-relative path without parent traversal.", field.m_sName));
  }
  if (!xiiStringUtils::IsValidIdentifierName(m_sActiveAssetProfile.GetData(), m_sActiveAssetProfile.GetData() + m_sActiveAssetProfile.GetElementCount()))
    return xiiStatus("ActiveAssetProfile must be a nonempty profile identifier.");
  return XII_SUCCESS;
}

xiiStatus xiiProjectManifest::CheckEngineCompatibility() const
{
  if (m_uiEngineMajor != BUILDSYSTEM_SDKVERSION_MAJOR || m_uiEngineMinor > BUILDSYSTEM_SDKVERSION_MINOR ||
      (m_uiEngineMinor == BUILDSYSTEM_SDKVERSION_MINOR && m_uiEnginePatch > BUILDSYSTEM_SDKVERSION_PATCH))
    return xiiStatus(xiiFmt("Project requires XII {}.{}.{}; this engine is {}.{}.{}.", m_uiEngineMajor, m_uiEngineMinor, m_uiEnginePatch,
                            BUILDSYSTEM_SDKVERSION_MAJOR, BUILDSYSTEM_SDKVERSION_MINOR, BUILDSYSTEM_SDKVERSION_PATCH));
  return XII_SUCCESS;
}

xiiStatus xiiProjectManifest::Read(xiiStreamReader& stream)
{
  xiiOpenDdlReader reader;
  if (reader.ParseDocument(stream, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
    return xiiStatus("Invalid OpenDDL project manifest.");
  const auto* root = reader.GetRootElement()->GetFirstChild();
  if (!root || !root->IsCustomType("Project") || root->GetSibling())
    return xiiStatus("Project manifest requires exactly one Project object.");
  xiiProjectManifest               result;
  xiiSet<xiiString>                names;
  xiiContiguousMemoryStreamStorage optional;
  xiiMemoryStreamWriter            optionalStream(&optional);
  xiiOpenDdlWriter                 optionalWriter;
  optionalWriter.SetOutputStream(&optionalStream);
  bool version = false, identity = false, engine = false;
  for (auto* child = root->GetFirstChild(); child; child = child->GetSibling())
  {
    if (!child->HasName() || names.Contains(child->GetName()))
      return xiiStatus("Project fields must have unique names.");
    names.Insert(child->GetName());
    const auto name = child->GetName();
    if (name == "FormatVersion")
    {
      if (child->GetPrimitivesType() != xiiOpenDdlPrimitiveType::UInt32 || child->GetNumPrimitives() != 1 || child->GetPrimitivesUInt32()[0] != s_uiFormatVersion)
        return xiiStatus("Unsupported or malformed project FormatVersion. Upgrade the engine to open newer project formats.");
      version = true;
      continue;
    }
    if (name == "EngineVersion")
    {
      if (child->GetPrimitivesType() != xiiOpenDdlPrimitiveType::UInt32 || child->GetNumPrimitives() != 3)
        return xiiStatus("EngineVersion requires three unsigned_int32 values.");
      result.m_uiEngineMajor = child->GetPrimitivesUInt32()[0];
      result.m_uiEngineMinor = child->GetPrimitivesUInt32()[1];
      result.m_uiEnginePatch = child->GetPrimitivesUInt32()[2];
      engine                 = true;
      continue;
    }
    if (name == "ID")
    {
      if (child->GetPrimitivesType() != xiiOpenDdlPrimitiveType::String || child->GetNumPrimitives() != 1 || !xiiConversionUtils::IsStringUuid(child->GetPrimitivesString()[0]))
        return xiiStatus("ID must contain one UUID string.");
      result.m_ProjectID = xiiConversionUtils::ConvertStringToUuid(child->GetPrimitivesString()[0]);
      identity           = true;
      continue;
    }
    bool known = false;
    for (const auto& field : s_StringFields)
    {
      if (field.m_sName != name)
        continue;
      if (child->GetPrimitivesType() != xiiOpenDdlPrimitiveType::String || child->GetNumPrimitives() != 1)
        return xiiStatus(xiiFmt("Project field '{}' requires one string.", name));
      result.*field.m_pMember = child->GetPrimitivesString()[0];
      known                   = true;
      break;
    }
    if (!known)
      CopyElement(optionalWriter, *child);
  }
  if (!version || !identity || !engine)
    return xiiStatus("Project manifest requires FormatVersion, ID and EngineVersion.");
  XII_SUCCEED_OR_RETURN(result.Validate());
  XII_SUCCEED_OR_RETURN(result.CheckEngineCompatibility());
  result.m_sOptionalFields = xiiStringView(reinterpret_cast<const char*>(optional.GetData()), optional.GetStorageSize32());
  *this                    = std::move(result);
  return XII_SUCCESS;
}

xiiStatus xiiProjectManifest::Write(xiiStreamWriter& stream) const
{
  XII_SUCCEED_OR_RETURN(Validate());
  // OpenDDL's writer assumes infallible output; serialize in memory before the checked file write.
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            memory(&storage);
  xiiOpenDdlWriter                 writer;
  writer.SetOutputStream(&memory);
  writer.SetPrimitiveTypeStringMode(xiiOpenDdlWriter::TypeStringMode::Compliant);
  writer.BeginObject("Project");
  xiiOpenDdlUtils::StoreUInt32(writer, s_uiFormatVersion, "FormatVersion");
  xiiStringBuilder id;
  xiiConversionUtils::ToString(m_ProjectID, id);
  xiiOpenDdlUtils::StoreString(writer, id, "ID");
  const xiiUInt32 engine[] = {m_uiEngineMajor, m_uiEngineMinor, m_uiEnginePatch};
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt32, "EngineVersion");
  writer.WriteUInt32(engine, XII_ARRAY_SIZE(engine));
  writer.EndPrimitiveList();
  for (const auto& field : s_StringFields)
    xiiOpenDdlUtils::StoreString(writer, this->*field.m_pMember, field.m_sName);
  if (!m_sOptionalFields.IsEmpty())
  {
    xiiRawMemoryStreamReader optional(m_sOptionalFields.GetData(), m_sOptionalFields.GetElementCount());
    xiiOpenDdlReader         reader;
    XII_SUCCEED_OR_RETURN(reader.ParseDocument(optional));
    for (auto* child = reader.GetRootElement()->GetFirstChild(); child; child = child->GetSibling())
      CopyElement(writer, *child);
  }
  writer.EndObject();
  return storage.CopyToStream(stream);
}

xiiStatus xiiProjectManifest::Load(xiiStringView sProjectFile, bool* out_pLegacy)
{
  if (out_pLegacy)
    *out_pLegacy = false;
  xiiOSFile file;
  if (file.Open(sProjectFile, xiiFileOpenMode::Read).Failed())
    return xiiStatus(xiiFmt("Could not read project '{}'.", sProjectFile));
  const auto size = file.GetFileSize();
  if (size == 0 || size > s_uiMaximumManifestSize)
    return xiiStatus("Project manifest is empty or exceeds the 1 MiB size limit.");
  xiiDynamicArray<xiiUInt8> bytes;
  bytes.SetCountUninitialized(static_cast<xiiUInt32>(size));
  if (file.Read(bytes.GetData(), size) != size)
    return xiiStatus("Could not read the complete project manifest.");
  if (size == s_sLegacyMarker.GetElementCount() + 1 && bytes.PeekBack() == 0 &&
      xiiMemoryUtils::IsEqual(bytes.GetData(), reinterpret_cast<const xiiUInt8*>(s_sLegacyMarker.GetStartPointer()), s_sLegacyMarker.GetElementCount()))
  {
    Initialize(sProjectFile);
    if (out_pLegacy)
      *out_pLegacy = true;
    return XII_SUCCESS;
  }
  xiiRawMemoryStreamReader stream(bytes.GetData(), size);
  return Read(stream);
}

xiiStatus xiiProjectManifest::Save(xiiStringView sProjectFile) const
{
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            stream(&storage);
  XII_SUCCEED_OR_RETURN(Write(stream));
  xiiStringBuilder suffix, staged(sProjectFile);
  xiiConversionUtils::ToString(xiiUuid::MakeUuid(), suffix);
  staged.Append(".", suffix, ".tmp");
  XII_SCOPE_EXIT(if (xiiOSFile::ExistsFile(staged) && xiiOSFile::DeleteFile(staged).Failed()) xiiLog::Warning("Could not remove staged manifest '{}'.", staged));
  {
    xiiOSFile file;
    if (file.Open(staged, xiiFileOpenMode::Write).Failed() || file.Write(storage.GetData(), storage.GetStorageSize64()).Failed())
      return xiiStatus(xiiFmt("Could not stage project manifest '{}'.", staged));
  }
  if (xiiOSFile::AtomicReplaceFile(staged, sProjectFile).Failed())
    return xiiStatus(xiiFmt("Could not atomically replace project manifest '{}'. Original retained.", sProjectFile));
  return XII_SUCCESS;
}

xiiStatus xiiProjectManifest::Migrate(xiiStringView sProjectFile) const
{
  xiiStringBuilder backup(sProjectFile, ".legacy");
  if (!xiiOSFile::ExistsFile(backup) && xiiOSFile::CopyFile(sProjectFile, backup).Failed())
    return xiiStatus(xiiFmt("Could not preserve legacy project at '{}'.", backup));
  return Save(sProjectFile);
}

xiiStatus xiiProjectManifest::ResolveConfigPath(xiiStringView sDefaultPath, xiiStringBuilder& out_sPath)
{
  out_sPath = sDefaultPath;
  xiiStringBuilder projectFile;
  if (xiiFileSystem::ResolvePath(":project/xiiProject", &projectFile, nullptr).Failed() || !xiiOSFile::ExistsFile(projectFile))
    return XII_SUCCESS;
  xiiProjectManifest manifest;
  XII_SUCCEED_OR_RETURN(manifest.Load(projectFile));
  const xiiProjectManifest defaults;
  for (const auto& field : s_StringFields)
  {
    if (field.m_pMember == &xiiProjectManifest::m_sName || field.m_pMember == &xiiProjectManifest::m_sDescription ||
        field.m_pMember == &xiiProjectManifest::m_sActiveAssetProfile || field.m_pMember == &xiiProjectManifest::m_sDefaultScene)
      continue;
    const xiiStringBuilder canonical(":project/", defaults.*field.m_pMember);
    if (canonical == sDefaultPath)
    {
      out_sPath.Set(":project/", manifest.*field.m_pMember);
      break;
    }
  }
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_Application_Config_Implementation_ProjectManifest);
