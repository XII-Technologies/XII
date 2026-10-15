/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace
{
  void WriteDefinitions(xiiStreamWriter& stream, xiiArrayPtr<xiiPlatformProfile* const> profiles)
  {
    xiiOpenDdlWriter ddl;
    ddl.SetOutputStream(&stream);
    ddl.BeginObject("AssetProfiles");
    for (const auto* profile : profiles)
    {
      ddl.BeginObject("Config", profile->GetConfigName());
      xiiReflectionSerializer::WriteObjectToDDL(ddl, profile->GetDynamicRTTI(), profile, xiiUuid::MakeStableUuidFromString(profile->GetConfigName()));
      ddl.EndObject();
    }
    ddl.EndObject();
  }

  struct ProfileFile
  {
    xiiString m_Target;
    xiiString m_Staged;
    xiiString m_Backup;
    bool      m_bBackedUp  = false;
    bool      m_bInstalled = false;
  };

  // Originals are renamed rather than truncated. Keep any backup that could not be restored,
  // and return its exact path instead of silently accepting a partial rollback.
  xiiStatus RollBack(xiiDynamicArray<ProfileFile>& files, xiiStringView failedPath)
  {
    xiiStringBuilder recovery;
    for (xiiUInt32 i = files.GetCount(); i > 0; --i)
    {
      auto& file = files[i - 1];
      if (file.m_bInstalled)
      {
        if (xiiOSFile::MoveFileOrDirectory(file.m_Target, file.m_Staged).Failed())
        {
          recovery.AppendFormat(" Unable to remove '{}'; original retained at '{}'.", file.m_Target, file.m_Backup);
          continue;
        }
        file.m_bInstalled = false;
      }
      if (file.m_bBackedUp)
      {
        if (xiiOSFile::MoveFileOrDirectory(file.m_Backup, file.m_Target).Failed())
          recovery.AppendFormat(" Restore '{}' from '{}'.", file.m_Target, file.m_Backup);
        else
          file.m_bBackedUp = false;
      }
    }
    return xiiStatus(xiiFmt("Could not install profile file '{}'.{}", failedPath, recovery));
  }
} // namespace

xiiResult xiiAssetCurator::SaveAssetProfiles()
{
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         stream(&storage);
  WriteDefinitions(stream, m_AssetProfiles.GetArrayPtr());
  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(":project/Editor/AssetProfiles.ddl"));
  XII_SUCCEED_OR_RETURN(storage.CopyToStream(file));
  return file.Flush();
}

void xiiAssetCurator::PublishAssetProfiles(xiiHybridArray<xiiPlatformProfile*, 8>& profiles, xiiUInt64 uiHash, xiiUInt32 uiActiveProfile)
{
  const bool initialized = m_pAssetTableWriter != nullptr;
  if (initialized)
    ShutdownUpdateTask();
  {
    XII_LOCK(m_CuratorMutex);
    ClearAssetProfiles();
    m_AssetProfiles             = std::move(profiles);
    m_uiActiveAssetProfile      = uiActiveProfile;
    m_uiLoadedAssetProfilesHash = uiHash;
    m_bAssetProfilesLoaded      = true;
  }
  if (initialized)
    SetActiveAssetProfileByIndex(uiActiveProfile, true);
}

xiiStatus xiiAssetCurator::ApplyAssetProfiles(xiiArrayPtr<const xiiPlatformProfile* const> input, xiiUInt32 uiActiveProfile)
{
  if (input.IsEmpty() || uiActiveProfile >= input.GetCount() || input[0] == nullptr || input[0]->GetConfigName() != "Default")
    return xiiStatus("Profiles require Default at index zero and a valid active selection.");
  xiiHybridArray<xiiPlatformProfile*, 8> profiles;
  XII_SCOPE_EXIT(for (auto* profile : profiles) XII_DEFAULT_DELETE(profile));
  for (const auto* source : input)
  {
    if (source == nullptr || !xiiStringUtils::IsValidIdentifierName(source->GetConfigName().GetStartPointer(), source->GetConfigName().GetEndPointer()))
      return xiiStatus("Profile names must be nonempty identifiers.");
    for (const auto* previous : profiles)
      if (previous->GetConfigName().IsEqual_NoCase(source->GetConfigName()))
        return xiiStatus(xiiFmt("Duplicate profile name '{}'.", source->GetConfigName()));
    auto* copy = xiiReflectionSerializer::Clone(source);
    copy->AddMissingConfigs();
    profiles.PushBack(copy);
  }

  xiiContiguousMemoryStreamStorage definitions;
  xiiMemoryStreamWriter            stream(&definitions);
  WriteDefinitions(stream, profiles.GetArrayPtr());
  const auto       hash = xiiHashingUtils::xxHash64(definitions.GetData(), definitions.GetStorageSize64());
  xiiStringBuilder root;
  if (xiiFileSystem::ResolvePath(":project/", &root, nullptr).Failed())
    return xiiStatus("The project directory is not mounted.");
  xiiStringBuilder suffix;
  xiiConversionUtils::ToString(xiiUuid::MakeUuid(), suffix);
  xiiDynamicArray<ProfileFile> files;
  auto                         addFile = [&](xiiStringView relative, bool stage) -> ProfileFile& {
    auto&            file = files.ExpandAndGetRef();
    xiiStringBuilder path(root);
    path.AppendPath(relative);
    file.m_Target = path;
    path.Append(".", suffix, ".profile-backup");
    file.m_Backup = path;
    if (stage)
    {
      path = file.m_Target;
      path.Append(".", suffix, ".profile-staged");
      file.m_Staged = path;
    }
    return file;
  };
  XII_SCOPE_EXIT(for (auto& file : files) if (!file.m_Staged.IsEmpty() && xiiOSFile::DeleteFile(file.m_Staged).Failed())
                   xiiLog::Warning("Could not remove staged profile '{}'.", file.m_Staged));
  for (const auto* profile : profiles)
  {
    xiiStringBuilder relative("RuntimeConfigs/", profile->GetConfigName(), ".xiiProfile");
    auto&            file = addFile(relative, true);
    if (profile->SaveForRuntime(file.m_Staged).Failed())
      return xiiStatus(xiiFmt("Could not write runtime profile '{}'.", file.m_Target));
  }
  for (const auto* old : m_AssetProfiles)
  {
    // Native callers can edit profiles before validation. Never use an invalid old name
    // to construct a removal path outside RuntimeConfigs when replacing that collection.
    if (!xiiStringUtils::IsValidIdentifierName(old->GetConfigName().GetStartPointer(), old->GetConfigName().GetEndPointer()))
      continue;
    xiiStringBuilder relative("RuntimeConfigs/", old->GetConfigName(), ".xiiProfile"), target(root);
    target.AppendPath(relative);
    bool retained = false;
    for (const auto& file : files)
    {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      retained |= file.m_Target.IsEqual_NoCase(target);
#else
      retained |= file.m_Target == target;
#endif
    }
    if (!retained)
      addFile(relative, false);
  }
  // Install the authoritative definitions last, after all runtime files are ready.
  auto& definitionFile = addFile("Editor/AssetProfiles.ddl", true);
  {
    xiiFileWriter file;
    if (file.Open(definitionFile.m_Staged).Failed() || definitions.CopyToStream(file).Failed() || file.Flush().Failed())
      return xiiStatus(xiiFmt("Could not write profile definitions '{}'.", definitionFile.m_Target));
  }

  auto*      processor        = xiiAssetProcessor::GetSingleton();
  const bool restartProcessor = processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Running;
  processor->StopProcessTask(false, true);
  XII_SCOPE_EXIT(if (restartProcessor) processor->StartProcessTask());
  ShutdownUpdateTask();
  XII_SCOPE_EXIT(RestartUpdateTask());
  for (auto& file : files)
  {
    if (xiiOSFile::ExistsFile(file.m_Target))
    {
      if (xiiOSFile::MoveFileOrDirectory(file.m_Target, file.m_Backup).Failed())
        return RollBack(files, file.m_Target);
      file.m_bBackedUp = true;
    }
    if (!file.m_Staged.IsEmpty())
    {
      if (xiiOSFile::MoveFileOrDirectory(file.m_Staged, file.m_Target).Failed())
        return RollBack(files, file.m_Target);
      file.m_bInstalled = true;
    }
  }
  PublishAssetProfiles(profiles, hash, uiActiveProfile);
  for (const auto& file : files)
    if (file.m_bBackedUp && xiiOSFile::DeleteFile(file.m_Backup).Failed())
      xiiLog::Warning("Could not remove profile backup '{}'.", file.m_Backup);
  return XII_SUCCESS;
}
