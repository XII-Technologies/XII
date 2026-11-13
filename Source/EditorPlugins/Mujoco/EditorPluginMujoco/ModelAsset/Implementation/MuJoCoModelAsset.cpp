/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMujoco/EditorPluginMujocoPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginMujoco/ModelAsset/MuJoCoModelAsset.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <MujocoPlugin/Resources/MuJoCoModelResource.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <mujoco/mujoco.h>

#include <memory>

namespace
{
  using SpecPtr  = std::unique_ptr<mjSpec, decltype(&mj_deleteSpec)>;
  using ModelPtr = std::unique_ptr<mjModel, decltype(&mj_deleteModel)>;

  /// A single import owns its provider, open buffers, and VFS. No process-global callbacks or mounts.
  class ModelInputs
  {
  public:
    ModelInputs()
    {
      mj_defaultVFS(&m_Vfs);
      mjp_defaultProvider(&m_Provider);
      m_Provider.data     = this;
      m_Provider.open     = Open;
      m_Provider.read     = Read;
      m_Provider.close    = Close;
      m_Provider.modified = Modified;
      m_bMounted          = mj_mountVFS(&m_Vfs, "", &m_Provider) == 0;
    }

    ~ModelInputs() { mj_deleteVFS(&m_Vfs); }

    xiiStatus Parse(xiiStringView sSource, SpecPtr& out_spec)
    {
      xiiStringBuilder path;
      if (!m_bMounted || xiiFileSystem::ResolveSpecialDirectory(sSource, path).Failed())
        return xiiStatus(xiiFmt("Could not resolve MuJoCo source '{}' through XII's filesystem.", sSource));
      if (xiiStringBuilder sEditorPath(path); xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sEditorPath))
        path = sEditorPath;
      xiiFileReader reader;
      if (reader.Open(path, 0, xiiFileShareMode::SharedReads).Failed())
        return xiiStatus(xiiFmt("Could not open MuJoCo source '{}' through XII's filesystem.", sSource));
      path = reader.GetFilePathAbsolute().GetView();
      reader.Close();
      if (!path.HasExtension("xml") && !path.HasExtension("mjcf") && !path.HasExtension("urdf"))
        return xiiStatus("MuJoCo sources must be MJCF (.xml/.mjcf) or MuJoCo-compatible URDF (.urdf).");
      path.MakeCleanPath();
      m_sRootPath      = path;
      char error[4096] = {};
      out_spec.reset(mj_parseXML(path.GetData(), &m_Vfs, error, sizeof(error)));
      if (!out_spec)
        return xiiStatus(xiiFmt("MuJoCo could not parse '{}': {}", sSource, error));
      return XII_SUCCESS;
    }

    xiiStatus Discover(mjSpec* pSpec)
    {
      // Read native specification metadata, not source XML. MuJoCo has already
      // expanded includes and URDF and retains each attached asset's origin spec.
      pSpec->compiler.usethread   = 0;
      pSpec->compiler.passthrough = mjPASSTHROUGH_NONE;
      m_bStripPath                = pSpec->strippath != 0;
      for (const auto type : {mjOBJ_MESH, mjOBJ_HFIELD, mjOBJ_SKIN, mjOBJ_TEXTURE})
        for (auto* pElement = mjs_firstElement(pSpec, type); pElement; pElement = mjs_nextElement(pSpec, pElement))
        {
          auto* pOrigin   = mjs_getOriginSpec(pElement);
          auto* pCompiler = mjs_getCompiler(pElement);
          if (!pOrigin)
            pOrigin = pSpec;
          if (!pCompiler)
            pCompiler = &pOrigin->compiler;
          pCompiler->usethread      = 0;
          pCompiler->passthrough    = mjPASSTHROUGH_NONE;
          const bool      texture   = type == mjOBJ_TEXTURE;
          const char*     directory = mjs_getString(texture ? pCompiler->texturedir : pCompiler->meshdir);
          const mjString* pFile     = nullptr;
          if (type == mjOBJ_MESH)
            pFile = mjs_asMesh(pElement)->file;
          if (type == mjOBJ_HFIELD)
            pFile = mjs_asHField(pElement)->file;
          if (type == mjOBJ_SKIN)
            pFile = mjs_asSkin(pElement)->file;
          if (texture)
          {
            auto* pTexture = mjs_asTexture(pElement);
            pFile          = pTexture->file;
            // XII resources must be hermetic, including sources explicitly requesting by-reference pixels.
            pTexture->embed = mjEMBED_TRUE;
            for (int i = 0; i < 6; ++i)
              if (const char* face = mjs_getInStringVec(pTexture->cubefiles, i); face && *face)
                XII_SUCCEED_OR_RETURN(Probe(pOrigin, directory, face));
          }
          if (pFile)
            XII_SUCCEED_OR_RETURN(Probe(pOrigin, directory, mjs_getString(pFile)));
        }
      return XII_SUCCESS;
    }

    xiiDynamicArray<xiiString> GetDependencies() const
    {
      XII_LOCK(m_Mutex);
      xiiSet<xiiString> unique;
      for (const auto& entry : m_Files)
        unique.Insert(entry.Value()->m_sDependency);
      xiiDynamicArray<xiiString> dependencies;
      for (const auto& path : unique)
        dependencies.PushBack(path);
      return dependencies;
    }

    xiiResult StoreSources(xiiMuJoCoModelResourceDescriptor& out_descriptor)
    {
      XII_LOCK(m_Mutex);
      const auto root = m_Files.Find(m_sRootPath);
      if (!root.IsValid())
        return XII_FAILURE;
      auto& files = out_descriptor.m_SourceFiles;
      files.Reserve(m_Files.GetCount());
      const auto store = [&files](const xiiString& path, Input& input) {
        auto& file   = files.ExpandAndGetRef();
        file.m_sPath = path;
        file.m_Data  = std::move(input.m_Bytes);
      };
      store(m_sRootPath, *root.Value());
      for (auto& entry : m_Files)
        if (entry.Key() != m_sRootPath)
          store(entry.Key(), *entry.Value());
      return XII_SUCCESS;
    }

    mjVFS m_Vfs{};

  private:
    struct Input
    {
      xiiDynamicArray<xiiUInt8> m_Bytes;
      xiiString                 m_sDependency;
      xiiUInt64                 m_uiHash = 0;
    };

    static int Open(mjResource* pResource)
    {
      auto& inputs = *static_cast<ModelInputs*>(pResource->provider->data);
      XII_LOCK(inputs.m_Mutex);
      xiiStringBuilder path(pResource->name);
      path.MakeCleanPath();
      if (auto it = inputs.m_Files.Find(path); it.IsValid())
        pResource->data = it.Value().Borrow();
      else
      {
        xiiFileReader reader;
        if (reader.Open(path, 64 * 1024, xiiFileShareMode::SharedReads).Failed() || reader.GetFileSize() > 512 * 1024 * 1024)
          return 0;
        xiiUniquePtr<Input> input = XII_DEFAULT_NEW(Input);
        input->m_Bytes.SetCountUninitialized(static_cast<xiiUInt32>(reader.GetFileSize()));
        if (reader.ReadBytes(input->m_Bytes.GetData(), input->m_Bytes.GetCount()) != input->m_Bytes.GetCount())
          return 0;
        input->m_sDependency = reader.GetFilePathAbsolute().GetView();
        // Retain the exact file if a data-directory-relative path would resolve to a different mount.
        xiiStringBuilder relative(input->m_sDependency);
        auto*            pApp = xiiQtEditorApp::GetSingleton();
        if (pApp->MakePathDataDirectoryParentRelative(relative))
        {
          xiiStringBuilder resolved(relative);
          if (pApp->MakeDataDirectoryRelativePathAbsolute(resolved) && resolved == input->m_sDependency)
            input->m_sDependency = relative;
        }
        input->m_uiHash = xiiHashingUtils::xxHash64(input->m_Bytes.GetData(), input->m_Bytes.GetCount());
        pResource->data = input.Borrow();
        inputs.m_Files.Insert(path, std::move(input));
      }
      const auto& input = *static_cast<const Input*>(pResource->data);
      xiiStringUtils::snprintf(pResource->timestamp, XII_ARRAY_SIZE(pResource->timestamp), "%llu", static_cast<unsigned long long>(input.m_uiHash));
      return 1;
    }

    static int Read(mjResource* pResource, const void** out_buffer)
    {
      const auto& input = *static_cast<const Input*>(pResource->data);
      *out_buffer       = input.m_Bytes.GetData();
      return static_cast<int>(input.m_Bytes.GetCount());
    }

    static void Close(mjResource* pResource) { pResource->data = nullptr; }
    static int  Modified(const mjResource* pResource, const char* szTimestamp)
    {
      return xiiStringUtils::IsEqual(pResource->timestamp, szTimestamp) ? 0 : 1;
    }

    xiiStatus Probe(const mjSpec* pOrigin, const char* szDirectory, const char* szFile)
    {
      if (!szFile || !*szFile)
        return XII_SUCCESS;
      xiiStringBuilder path(m_bStripPath ? xiiPathUtils::GetFileNameAndExtension(szFile) : xiiStringView(szFile));
      if (!xiiPathUtils::IsAbsolutePath(path))
      {
        xiiStringBuilder combined(szDirectory);
        combined.AppendPath(path);
        path = combined;
      }
      char  error[2048] = {};
      auto* pResource   = mju_openResource(mjs_getString(pOrigin->modelfiledir), path.GetData(), &m_Vfs, error, sizeof(error));
      if (!pResource)
        return xiiStatus(xiiFmt("MuJoCo dependency '{}': {}", path, error));
      mju_closeResource(pResource);
      return XII_SUCCESS;
    }

    mjpResourceProvider                    m_Provider{};
    mutable xiiMutex                       m_Mutex;
    xiiMap<xiiString, xiiUniquePtr<Input>> m_Files;
    xiiString                              m_sRootPath;
    bool                                   m_bMounted   = false;
    bool                                   m_bStripPath = false;
  };
} // namespace

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoModelAssetProperties, 1, xiiRTTIDefaultAllocator<xiiMuJoCoModelAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SourceFile", m_sSourceFile)->AddAttributes(new xiiFileBrowserAttribute("MuJoCo Source", "*.xml;*.mjcf;*.urdf", {}, {}, xiiDependencyFlags::Transform)),
    XII_MEMBER_PROPERTY("WarningsAsErrors", m_bWarningsAsErrors)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ARRAY_MEMBER_PROPERTY("Dependencies", m_Dependencies)->AddAttributes(new xiiReadOnlyAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoModelAssetDocument, 2, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoModelAssetDocumentManager, 1, xiiRTTIDefaultAllocator<xiiMuJoCoModelAssetDocumentManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoModelAssetDocumentGenerator, 1, xiiRTTIDefaultAllocator<xiiMuJoCoModelAssetDocumentGenerator>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMuJoCoModelAssetDocument::xiiMuJoCoModelAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiMuJoCoModelAssetProperties>(sDocumentPath, xiiAssetDocEngineConnection::None)
{
}

void xiiMuJoCoModelAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);
  for (const auto& path : GetProperties()->m_Dependencies)
    pInfo->m_TransformDependencies.Insert(path);
}

xiiTransformStatus xiiMuJoCoModelAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView,
                                                                       const xiiPlatformProfile*, const xiiAssetFileHeader&, xiiBitflags<xiiTransformFlags> flags)
{
  XII_PROFILE_SCOPE("MuJoCoModelTransform");
  auto*       pProperties = GetProperties();
  ModelInputs inputs;
  SpecPtr     spec(nullptr, mj_deleteSpec);
  XII_SUCCEED_OR_RETURN(inputs.Parse(pProperties->m_sSourceFile, spec));
  XII_SUCCEED_OR_RETURN(inputs.Discover(spec.get()));
  auto dependencies = inputs.GetDependencies();
  if (dependencies != pProperties->m_Dependencies)
  {
    if (flags.IsSet(xiiTransformFlags::BackgroundProcessing))
      return xiiTransformStatus(xiiTransformResult::NeedsImport);
    GetObjectAccessor()->StartTransaction("Update MuJoCo Model Dependencies");
    pProperties->m_Dependencies = dependencies;
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();
    // Resource publication requires a hash incorporating the newly imported dependency graph.
    if (!flags.IsSet(xiiTransformFlags::ImportSource))
      return xiiTransformStatus(xiiTransformResult::NeedsImport);
  }
  if (flags.IsSet(xiiTransformFlags::ImportSource))
    return XII_SUCCESS;

  ModelPtr model(mj_compile(spec.get(), &inputs.m_Vfs), mj_deleteModel);
  if (!model)
    return xiiStatus(xiiFmt("MuJoCo compilation failed: {}", mjs_getError(spec.get())));
  const int warnings = mjs_numWarnings(spec.get());
  for (int i = 0; i < warnings; ++i)
    xiiLog::Warning("MuJoCo '{}': {}", pProperties->m_sSourceFile, mjs_getWarning(spec.get(), i));
  if (warnings > 0 && pProperties->m_bWarningsAsErrors)
    return xiiStatus("MuJoCo compilation produced warnings and WarningsAsErrors is enabled.");
  if (!xiiMath::IsFinite(model->opt.timestep) || model->opt.timestep <= 0)
    return xiiStatus("MuJoCo compiled an invalid simulation timestep.");
  if (model->nplugin != 0)
    return xiiStatus("MuJoCo models that require native extension plugins cannot be exported as XII assets. MJB stores process-specific plugin slots.");
  // Never publish an asset with compiler inputs omitted from its dependency hash.
  if (inputs.GetDependencies() != dependencies)
    return xiiStatus("MuJoCo opened additional compiler inputs not exposed by its model specification. This source decoder cannot be dependency-tracked safely.");
  const auto size = mj_sizeModel(model.get());
  if (size <= 0 || size > 512 * 1024 * 1024)
    return xiiStatus("MuJoCo compiled model exceeds the supported resource size.");
  xiiMuJoCoModelResourceDescriptor descriptor;
  descriptor.m_ModelData.SetCountUninitialized(static_cast<xiiUInt32>(size));
  mj_saveModel(model.get(), nullptr, descriptor.m_ModelData.GetData(), size);
  if (inputs.StoreSources(descriptor).Failed())
    return xiiStatus("Could not capture the MuJoCo model's source snapshot.");
  if (descriptor.Save(stream).Failed())
    return xiiStatus("Could not serialize the compiled MuJoCo model resource.");
  return XII_SUCCESS;
}

xiiMuJoCoModelAssetDocumentManager::xiiMuJoCoModelAssetDocumentManager()
{
  m_TypeDescriptor.m_sDocumentTypeName = "MuJoCoModel";
  m_TypeDescriptor.m_sFileExtension    = "xiiMuJoCoModelAsset";
  m_TypeDescriptor.m_sAssetCategory    = "Physics";
  m_TypeDescriptor.m_pDocumentType     = xiiGetStaticRTTI<xiiMuJoCoModelAssetDocument>();
  m_TypeDescriptor.m_pManager          = this;
  m_TypeDescriptor.m_CompatibleTypes.PushBack("CompatibleAsset_MuJoCoModel");
  m_TypeDescriptor.m_sResourceFileExtension = "xiiBinMuJoCoModel";
  m_TypeDescriptor.m_AssetDocumentFlags     = xiiAssetDocumentFlags::AutoTransformOnSave | xiiAssetDocumentFlags::RefreshSourceDependencies;
}

xiiStatus xiiMuJoCoModelAssetDocumentManager::InternalCreateDocument(xiiStringView, xiiStringView sPath, bool,
                                                                     xiiDocument*& out_pDocument, const xiiDocumentObject*)
{
  out_pDocument = new xiiMuJoCoModelAssetDocument(sPath);
  return XII_SUCCESS;
}

void xiiMuJoCoModelAssetDocumentManager::InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_types) const
{
  inout_types.PushBack(&m_TypeDescriptor);
}

xiiUInt64 xiiMuJoCoModelAssetDocumentManager::ComputeAssetProfileHashImpl(const xiiPlatformProfile*) const
{
  return xiiMuJoCoModelResourceDescriptor::GetCompilerVersion();
}

xiiMuJoCoModelAssetDocumentGenerator::xiiMuJoCoModelAssetDocumentGenerator()
{
  AddSupportedFileType("xml");
  AddSupportedFileType("mjcf");
  AddSupportedFileType("urdf");
}

void xiiMuJoCoModelAssetDocumentGenerator::GetImportModes(xiiStringView, xiiDynamicArray<ImportMode>& out_modes) const
{
  auto& mode      = out_modes.ExpandAndGetRef();
  mode.m_sName    = "MuJoCoModelImport";
  mode.m_Priority = xiiAssetDocGeneratorPriority::LowPriority;
}

xiiStatus xiiMuJoCoModelAssetDocumentGenerator::Generate(xiiStringView                  sInputFileAbs, xiiStringView,
                                                         xiiDynamicArray<xiiDocument*>& out_generatedDocuments)
{
  xiiStringBuilder output(sInputFileAbs);
  output.ChangeFileExtension(GetDocumentExtension());
  xiiOSFile::FindFreeFilename(output);
  auto*            pApp = xiiQtEditorApp::GetSingleton();
  xiiStringBuilder source(sInputFileAbs);
  if (!pApp->MakePathDataDirectoryRelative(source))
    return xiiStatus("MuJoCo source must belong to a project data directory.");
  auto* pDocument = pApp->CreateDocument(output, xiiDocumentFlags::None);
  if (!pDocument)
    return xiiStatus("Could not create the MuJoCo model asset document.");
  out_generatedDocuments.PushBack(pDocument);
  return pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]->GetTypeAccessor().SetValue("SourceFile", source.GetView()) ? xiiStatus(XII_SUCCESS) : xiiStatus("Could not assign the MuJoCo source file.");
}

XII_STATICLINK_FILE(EditorPluginMujoco, EditorPluginMujoco_ModelAsset_Implementation_MuJoCoModelAsset);
