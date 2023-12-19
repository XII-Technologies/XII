#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetDocumentGenerator, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAssetDocumentGenerator::xiiAssetDocumentGenerator() = default;

xiiAssetDocumentGenerator::~xiiAssetDocumentGenerator() = default;

void xiiAssetDocumentGenerator::AddSupportedFileType(xiiStringView sExtension)
{
  xiiStringBuilder tmp = sExtension;
  tmp.ToLower();

  m_SupportedFileTypes.PushBack(tmp);
}

bool xiiAssetDocumentGenerator::SupportsFileType(xiiStringView sFile) const
{
  xiiStringBuilder tmp = xiiPathUtils::GetFileExtension(sFile);

  if (tmp.IsEmpty())
    tmp = sFile;

  tmp.ToLower();

  return m_SupportedFileTypes.Contains(tmp);
}

void xiiAssetDocumentGenerator::BuildFileDialogFilterString(xiiStringBuilder& out_sFilter) const
{
  bool semicolon = false;
  out_sFilter.Format("{0} (", GetDocumentExtension());
  AppendFileFilterStrings(out_sFilter, semicolon);
  out_sFilter.Append(")");
}

void xiiAssetDocumentGenerator::AppendFileFilterStrings(xiiStringBuilder& out_sFilter, bool& ref_bSemicolon) const
{
  for (const xiiString& ext : m_SupportedFileTypes)
  {
    xiiStringBuilder extWithStarDot;
    extWithStarDot.AppendFormat("*.{0}", ext);

    if (const char* pos = out_sFilter.FindSubString(extWithStarDot.GetData()))
    {
      const char afterExt = *(pos + extWithStarDot.GetElementCount());

      if (afterExt == '\0' || afterExt == ';')
        continue;
    }

    if (ref_bSemicolon)
    {
      out_sFilter.AppendFormat("; {0}", extWithStarDot.GetView());
    }
    else
    {
      out_sFilter.Append(extWithStarDot.GetView());
      ref_bSemicolon = true;
    }
  }
}

void xiiAssetDocumentGenerator::CreateGenerators(xiiHybridArray<xiiAssetDocumentGenerator*, 16>& out_generators)
{
  xiiRTTI::ForEachDerivedType<xiiAssetDocumentGenerator>(
    [&](const xiiRTTI* pRtti) {
      out_generators.PushBack(pRtti->GetAllocator()->Allocate<xiiAssetDocumentGenerator>());
    },
    xiiRTTI::ForEachOptions::ExcludeNonAllocatable);

  // sort by name
  out_generators.Sort([](xiiAssetDocumentGenerator* lhs, xiiAssetDocumentGenerator* rhs) -> bool { return lhs->GetDocumentExtension().Compare_NoCase(rhs->GetDocumentExtension()) < 0; });
}

void xiiAssetDocumentGenerator::DestroyGenerators(const xiiHybridArray<xiiAssetDocumentGenerator*, 16>& generators)
{
  for (xiiAssetDocumentGenerator* pGen : generators)
  {
    pGen->GetDynamicRTTI()->GetAllocator()->Deallocate(pGen);
  }
}

void xiiAssetDocumentGenerator::ImportAssets(const xiiDynamicArray<xiiString>& filesToImport)
{
  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  xiiDynamicArray<xiiAssetDocumentGenerator::ImportGroupOptions> allImports;
  allImports.Reserve(filesToImport.GetCount());

  CreateImportOptionList(filesToImport, allImports, generators);

  SortAndSelectBestImportOption(allImports);

  xiiQtAssetImportDlg dlg(QApplication::activeWindow(), allImports);
  dlg.exec();

  DestroyGenerators(generators);
}

void xiiAssetDocumentGenerator::GetSupportsFileTypes(xiiSet<xiiString>& out_extensions)
{
  out_extensions.Clear();

  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);
  for (auto pGen : generators)
  {
    for (const xiiString& ext : pGen->m_SupportedFileTypes)
    {
      out_extensions.Insert(ext);
    }
  }
  DestroyGenerators(generators);
}

void xiiAssetDocumentGenerator::ImportAssets()
{
  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  xiiStringBuilder singleFilter, fullFilter, allExtensions;
  bool             semicolon = false;

  for (auto pGen : generators)
  {
    pGen->AppendFileFilterStrings(allExtensions, semicolon);
    pGen->BuildFileDialogFilterString(singleFilter);
    fullFilter.Append(singleFilter, "\n");
  }

  fullFilter.Append("All files (*.*)");
  fullFilter.Prepend("All asset files (", allExtensions, ")\n");

  static xiiStringBuilder s_StartDir;
  if (s_StartDir.IsEmpty())
  {
    s_StartDir = xiiToolsProject::GetSingleton()->GetProjectDirectory();
  }

  QStringList filenames = QFileDialog::getOpenFileNames(QApplication::activeWindow(), "Import Assets", s_StartDir.GetData(),
                                                        QString::fromUtf8(fullFilter.GetData()), nullptr, QFileDialog::Option::DontResolveSymlinks);

  DestroyGenerators(generators);

  if (filenames.empty())
    return;

  s_StartDir = filenames[0].toUtf8().data();
  s_StartDir.PathParentDirectory();

  xiiHybridArray<xiiString, 16> filesToImport;
  for (QString s : filenames)
  {
    filesToImport.PushBack(s.toUtf8().data());
  }

  ImportAssets(filesToImport);
}

void xiiAssetDocumentGenerator::CreateImportOptionList(const xiiDynamicArray<xiiString>& filesToImport, xiiDynamicArray<xiiAssetDocumentGenerator::ImportGroupOptions>& allImports, const xiiHybridArray<xiiAssetDocumentGenerator*, 16>& generators)
{
  xiiQtEditorApp*  pApp = xiiQtEditorApp::GetSingleton();
  xiiStringBuilder sInputRelative, sGroup;

  for (const xiiString& sInputAbsolute : filesToImport)
  {
    sInputRelative = sInputAbsolute;

    if (!pApp->MakePathDataDirectoryRelative(sInputRelative))
    {
      // error, file is not in data directory -> skip
      continue;
    }

    for (xiiAssetDocumentGenerator* pGen : generators)
    {
      if (pGen->SupportsFileType(sInputRelative))
      {
        sGroup = pGen->GetGeneratorGroup();

        ImportGroupOptions* pData = nullptr;
        for (auto& importer : allImports)
        {
          if (importer.m_sGroup == sGroup && importer.m_sInputFileAbsolute == sInputAbsolute)
          {
            pData = &importer;
          }
        }

        if (pData == nullptr)
        {
          pData                       = &allImports.ExpandAndGetRef();
          pData->m_sGroup             = sGroup;
          pData->m_sInputFileAbsolute = sInputAbsolute;
          pData->m_sInputFileRelative = sInputRelative;
        }

        xiiHybridArray<xiiAssetDocumentGenerator::ImportMode, 4> options;
        pGen->GetImportModes(sInputAbsolute, options);

        for (auto& option : options)
        {
          option.m_pGenerator = pGen;
        }

        pData->m_ImportOptions.PushBackRange(options);
      }
    }
  }
}

void xiiAssetDocumentGenerator::SortAndSelectBestImportOption(xiiDynamicArray<xiiAssetDocumentGenerator::ImportGroupOptions>& allImports)
{
  allImports.Sort([](const xiiAssetDocumentGenerator::ImportGroupOptions& lhs, const xiiAssetDocumentGenerator::ImportGroupOptions& rhs) -> bool { return lhs.m_sInputFileRelative < rhs.m_sInputFileRelative; });

  for (auto& singleImport : allImports)
  {
    singleImport.m_ImportOptions.Sort([](const xiiAssetDocumentGenerator::ImportMode& lhs, const xiiAssetDocumentGenerator::ImportMode& rhs) -> bool { return xiiTranslate(lhs.m_sName).Compare_NoCase(xiiTranslate(rhs.m_sName)) < 0; });

    xiiUInt32 uiNumPrios[(xiiUInt32)xiiAssetDocGeneratorPriority::ENUM_COUNT] = {0};
    xiiUInt32 uiBestPrio[(xiiUInt32)xiiAssetDocGeneratorPriority::ENUM_COUNT] = {0};

    for (xiiUInt32 i = 0; i < singleImport.m_ImportOptions.GetCount(); ++i)
    {
      uiNumPrios[(xiiUInt32)singleImport.m_ImportOptions[i].m_Priority]++;
      uiBestPrio[(xiiUInt32)singleImport.m_ImportOptions[i].m_Priority] = i;
    }

    singleImport.m_iSelectedOption = -1;
    for (xiiUInt32 prio = (xiiUInt32)xiiAssetDocGeneratorPriority::HighPriority; prio > (xiiUInt32)xiiAssetDocGeneratorPriority::Undecided; --prio)
    {
      if (uiNumPrios[prio] == 1)
      {
        singleImport.m_iSelectedOption = uiBestPrio[prio];
        break;
      }

      if (uiNumPrios[prio] > 1)
        break;
    }
  }
}

xiiStatus xiiAssetDocumentGenerator::Import(xiiStringView sInputFileAbs, xiiStringView sMode, bool bOpenDocument)
{
  xiiStringBuilder ext = sInputFileAbs.GetFileExtension();
  ext.ToLower();

  if (!m_SupportedFileTypes.Contains(ext))
    return xiiStatus(xiiFmt("Files of type '{}' cannot be imported as '{}' documents.", ext, GetDocumentExtension()));

  xiiDocument* pGeneratedDoc = nullptr;
  XII_SUCCEED_OR_RETURN(Generate(sInputFileAbs, sMode, pGeneratedDoc));

  XII_ASSERT_DEV(pGeneratedDoc != nullptr, "");

  const xiiString sDocPath = pGeneratedDoc->GetDocumentPath();

  pGeneratedDoc->SaveDocument(true).LogFailure();
  pGeneratedDoc->GetDocumentManager()->CloseDocument(pGeneratedDoc);

  if (bOpenDocument)
  {
    xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(sDocPath);
  }

  return xiiStatus(XII_SUCCESS);
}
