/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Application/Config/ProjectManifest.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(ProjectManifest);

XII_CREATE_SIMPLE_TEST(ProjectManifest, Format)
{
  const xiiStringBuilder project(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/Manifest/xiiProject");
  xiiProjectManifest     manifest;
  manifest.Initialize(project);
  const auto id = manifest.m_ProjectID;
  XII_TEST_STATUS(manifest.Save(project));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Round trip and portable persistent identity")
  {
    xiiProjectManifest loaded;
    bool               legacy = true;
    XII_TEST_STATUS(loaded.Load(project, &legacy));
    XII_TEST_BOOL(!legacy);
    XII_TEST_BOOL(loaded.m_ProjectID == id);
    XII_TEST_STRING(loaded.m_sName, "Manifest");
    loaded.m_sDescription = "A project with Unicode and escaped text: \"hello\"\nworld";
    XII_TEST_STATUS(loaded.Save(project));
    XII_TEST_STATUS(manifest.Load(project));
    XII_TEST_STRING(manifest.m_sDescription, loaded.m_sDescription);
    const xiiStringBuilder relocated(project.GetFileDirectory(), "Moved/xiiProject");
    XII_TEST_STATUS(manifest.Save(relocated));
    XII_TEST_STATUS(loaded.Load(relocated));
    XII_TEST_BOOL(loaded.m_ProjectID == id);
  }

  xiiContiguousMemoryStreamStorage serialized;
  xiiMemoryStreamWriter            writer(&serialized);
  XII_TEST_STATUS(manifest.Write(writer));
  const xiiStringBuilder valid(xiiStringView(reinterpret_cast<const char*>(serialized.GetData()), serialized.GetStorageSize32()));
  auto                   read = [&](xiiStringView text) {
    xiiRawMemoryStreamReader stream(text.GetStartPointer(), text.GetElementCount());
    return manifest.Read(stream);
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Strict schema, version, compatibility and transactional validation")
  {
    const char* invalid[] = {
      "Project {}",
      "Other {}",
      "Project {} Project {}",
      "Project { unsigned_int32 %FormatVersion {2} }",
      "Project { string %FormatVersion {\"1\"} }",
      "Project { unsigned_int32 %FormatVersion {1, 1} }",
      "Project { unsigned_int32 %FormatVersion {1} unsigned_int32 %FormatVersion {1} }",
    };
    for (const auto* text : invalid)
    {
      XII_TEST_BOOL(read(text).Failed());
      XII_TEST_BOOL(manifest.m_ProjectID == id);
    }
    for (const auto* path : {"../Outside.ddl", "D:/Machine/Config.ddl", ":project/Config.ddl", ">sdk/Config.ddl", "Folder/../Config.ddl", ""})
    {
      auto changed               = manifest;
      changed.m_sDataDirectories = path;
      XII_TEST_BOOL(changed.Validate().Failed());
    }
    auto incompatible = manifest;
    incompatible.m_uiEngineMajor += 1;
    XII_TEST_BOOL(incompatible.CheckEngineCompatibility().Failed());
    auto malformed = valid;
    malformed.ReplaceAll("%Name", "%ID");
    XII_TEST_BOOL(read(malformed).Failed());
    malformed = valid;
    malformed.ReplaceAll("%Name", "%UnexpectedName");
    XII_TEST_BOOL(read(malformed).Failed());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Optional extension fields survive read and save")
  {
    auto       extended = valid;
    const auto last     = extended.FindLastSubString("}");
    XII_TEST_BOOL(last != nullptr);
    extended.Insert(last, "Extension %Vendor { string %Note {\"preserved\"} unsigned_int64 %Flags {42} }\n");
    XII_TEST_STATUS(read(extended));
    XII_TEST_STATUS(manifest.Save(project));
    xiiProjectManifest loaded;
    XII_TEST_STATUS(loaded.Load(project));
    xiiContiguousMemoryStreamStorage output;
    xiiMemoryStreamWriter            stream(&output);
    XII_TEST_STATUS(loaded.Write(stream));
    const xiiStringView text(reinterpret_cast<const char*>(output.GetData()), output.GetStorageSize32());
    XII_TEST_BOOL(text.FindSubString("preserved") != nullptr);
    XII_TEST_BOOL(text.FindSubString("Vendor") != nullptr);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Legacy migration preserves marker backup and new identity")
  {
    const xiiStringBuilder legacyPath(project.GetFileDirectory(), "Legacy/xiiProject");
    constexpr char         marker[] = "xiiEditor Project File";
    {
      xiiOSFile file;
      XII_TEST_RESULT(file.Open(legacyPath, xiiFileOpenMode::Write));
      XII_TEST_RESULT(file.Write(marker, sizeof(marker)));
    }
    bool legacy = false;
    XII_TEST_STATUS(manifest.Load(legacyPath, &legacy));
    XII_TEST_BOOL(legacy);
    const auto migratedID = manifest.m_ProjectID;
    XII_TEST_STATUS(manifest.Migrate(legacyPath));
    XII_TEST_STATUS(manifest.Load(legacyPath, &legacy));
    XII_TEST_BOOL(!legacy);
    XII_TEST_BOOL(manifest.m_ProjectID == migratedID);
    xiiProjectManifest competing;
    competing.Initialize(legacyPath);
    XII_TEST_STATUS(competing.Migrate(legacyPath));
    XII_TEST_STATUS(competing.Load(legacyPath));
    XII_TEST_BOOL(competing.m_ProjectID == migratedID);
    const xiiStringBuilder backup(legacyPath, ".legacy");
    xiiOSFile              file;
    XII_TEST_RESULT(file.Open(backup, xiiFileOpenMode::Read));
    char bytes[sizeof(marker)] = {};
    XII_TEST_INT(file.Read(bytes, sizeof(bytes)), sizeof(bytes));
    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(bytes, marker, sizeof(marker)));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Failed replacement retains the previous complete manifest")
  {
    XII_TEST_STATUS(manifest.Load(project));
    const auto previousName = manifest.m_sName;
    auto       changed      = manifest;
    changed.m_sName         = "Must not be committed";
    const xiiStringBuilder missing(project.GetFileDirectory(), "DoesNotExist.tmp");
    XII_TEST_BOOL(xiiOSFile::AtomicReplaceFile(missing, project).Failed());
    changed.m_ProjectID = xiiUuid::MakeInvalid();
    XII_TEST_BOOL(changed.Save(project).Failed());
    XII_TEST_STATUS(changed.Load(project));
    XII_TEST_BOOL(changed.m_ProjectID == manifest.m_ProjectID);
    XII_TEST_STRING(changed.m_sName, previousName);
  }
}

XII_CREATE_SIMPLE_TEST(ProjectManifest, BundledProjects)
{
  if (!XII_TEST_RESULT(xiiFileSystem::DetectSdkRootDirectory()))
    return;
  for (const xiiStringView relative : {"Data/Base/xiiProject", "Data/Samples/FeatureLab/xiiProject"})
  {
    const xiiStringBuilder path(xiiFileSystem::GetSdkRootDirectory(), "/", relative);
    xiiProjectManifest     manifest, reloaded;
    XII_TEST_STATUS(manifest.Load(path));
    XII_TEST_STATUS(reloaded.Load(path));
    XII_TEST_BOOL(manifest.m_ProjectID.IsValid());
    XII_TEST_BOOL(manifest.m_ProjectID == reloaded.m_ProjectID);
  }
}
