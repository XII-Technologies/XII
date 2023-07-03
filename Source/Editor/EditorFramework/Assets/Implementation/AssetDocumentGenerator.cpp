#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetDocumentGenerator, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAssetDocumentGenerator::xiiAssetDocumentGenerator() {}

xiiAssetDocumentGenerator::~xiiAssetDocumentGenerator() {}

void xiiAssetDocumentGenerator::AddSupportedFileType(xiiStringView sExtension)
{
  xiiStringBuilder tmp = sExtension;
  tmp.ToLower();

  m_SupportedFileTypes.PushBack(tmp);
}

bool xiiAssetDocumentGenerator::SupportsFileType(xiiStringView sFile) const
{
  xiiStringBuilder tmp = xiiPathUtils::GetFileExtension(sFile);
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
  for (const xiiString ext : m_SupportedFileTypes)
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

void xiiAssetDocumentGenerator::CreateGenerators(xiiHybridArray<xiiAssetDocumentGenerator*, 16>& out_Generators)
{
  for (xiiRTTI* pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<xiiAssetDocumentGenerator>() || !pRtti->GetAllocator()->CanAllocate())
      continue;

    out_Generators.PushBack(pRtti->GetAllocator()->Allocate<xiiAssetDocumentGenerator>());
  }

  // sort by name
  out_Generators.Sort([](xiiAssetDocumentGenerator* lhs, xiiAssetDocumentGenerator* rhs) -> bool { return lhs->GetDocumentExtension().Compare_NoCase(rhs->GetDocumentExtension()) < 0; });
}

void xiiAssetDocumentGenerator::DestroyGenerators(xiiHybridArray<xiiAssetDocumentGenerator*, 16>& generators)
{
  for (xiiAssetDocumentGenerator* pGen : generators)
  {
    pGen->GetDynamicRTTI()->GetAllocator()->Deallocate(pGen);
  }

  generators.Clear();
}


void xiiAssetDocumentGenerator::ExecuteImport(xiiDynamicArray<ImportData>& ref_allImports)
{
  for (auto& data : ref_allImports)
  {
    if (data.m_iSelectedOption < 0)
      continue;

    XII_LOG_BLOCK("Asset Import", data.m_sInputFileParentRelative);

    auto& option = data.m_ImportOptions[data.m_iSelectedOption];

    if (DetermineInputAndOutputFiles(data, option).Failed())
      continue;

    xiiDocument*    pGeneratedDoc = nullptr;
    const xiiStatus status        = option.m_pGenerator->Generate(data.m_sInputFileRelative, option, pGeneratedDoc);

    if (pGeneratedDoc)
    {
      pGeneratedDoc->SaveDocument(true).LogFailure();
      pGeneratedDoc->GetDocumentManager()->CloseDocument(pGeneratedDoc);

      xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(option.m_sOutputFileAbsolute);
    }

    if (status.Failed())
    {
      data.m_sImportMessage = status.m_sMessage;
      xiiLog::Error("Asset import failed: '{0}'", status.m_sMessage);
    }
    else
    {
      data.m_sImportMessage.Clear();
      data.m_bDoNotImport = true;
      xiiLog::Success("Generated asset document '{0}'", option.m_sOutputFileAbsolute);
    }
  }
}


xiiResult xiiAssetDocumentGenerator::DetermineInputAndOutputFiles(ImportData& data, Info& option)
{
  auto pApp = xiiQtEditorApp::GetSingleton();

  xiiStringBuilder inputFile = data.m_sInputFileParentRelative;
  if (!pApp->MakeParentDataDirectoryRelativePathAbsolute(inputFile, true))
  {
    data.m_sImportMessage = "Input file could not be located";
    return XII_FAILURE;
  }

  data.m_sInputFileAbsolute = inputFile;

  if (!pApp->MakePathDataDirectoryRelative(inputFile))
  {
    data.m_sImportMessage = "Input file is not in any known data directory";
    return XII_FAILURE;
  }

  data.m_sInputFileRelative = inputFile;

  xiiStringBuilder outputFile = option.m_sOutputFileParentRelative;
  if (!pApp->MakeParentDataDirectoryRelativePathAbsolute(outputFile, false))
  {
    data.m_sImportMessage = "Target file location could not be found";
    return XII_FAILURE;
  }

  option.m_sOutputFileAbsolute = outputFile;

  // don't create it when it already exists
  if (xiiOSFile::ExistsFile(outputFile))
  {
    data.m_bDoNotImport   = true;
    data.m_sImportMessage = "Target file already exists";
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiAssetDocumentGenerator::ImportAssets(const xiiHybridArray<xiiString, 16>& filesToImport)
{
  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  xiiDynamicArray<xiiAssetDocumentGenerator::ImportData> allImports;
  allImports.Reserve(filesToImport.GetCount());

  CreateImportOptionList(filesToImport, allImports, generators);

  SortAndSelectBestImportOption(allImports);

  xiiQtAssetImportDlg dlg(QApplication::activeWindow(), allImports);
  dlg.exec();

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

void xiiAssetDocumentGenerator::CreateImportOptionList(const xiiHybridArray<xiiString, 16>&                    filesToImport,
                                                       xiiDynamicArray<xiiAssetDocumentGenerator::ImportData>& allImports,
                                                       const xiiHybridArray<xiiAssetDocumentGenerator*, 16>&   generators)
{
  xiiQtEditorApp*  pApp = xiiQtEditorApp::GetSingleton();
  xiiStringBuilder sInputParentRelative, sInputRelative, sGroup;

  for (const xiiString& sInputAbsolute : filesToImport)
  {
    sInputParentRelative = sInputAbsolute;
    sInputRelative       = sInputAbsolute;

    if (!pApp->MakePathDataDirectoryParentRelative(sInputParentRelative) || !pApp->MakePathDataDirectoryRelative(sInputRelative))
    {
      auto& data                      = allImports.ExpandAndGetRef();
      data.m_sInputFileAbsolute       = sInputAbsolute;
      data.m_sInputFileParentRelative = sInputParentRelative;
      data.m_sInputFileRelative       = sInputRelative;
      data.m_sImportMessage           = "File is not located in any data directory.";
      data.m_bDoNotImport             = true;
      continue;
    }

    for (xiiAssetDocumentGenerator* pGen : generators)
    {
      if (pGen->SupportsFileType(sInputParentRelative))
      {
        sGroup = pGen->GetGeneratorGroup();

        ImportData* pData = nullptr;
        for (auto& importer : allImports)
        {
          if (importer.m_sGroup == sGroup && importer.m_sInputFileAbsolute == sInputAbsolute)
          {
            pData = &importer;
          }
        }

        if (pData == nullptr)
        {
          pData                             = &allImports.ExpandAndGetRef();
          pData->m_sGroup                   = sGroup;
          pData->m_sInputFileAbsolute       = sInputAbsolute;
          pData->m_sInputFileParentRelative = sInputParentRelative;
          pData->m_sInputFileRelative       = sInputRelative;
        }

        xiiHybridArray<xiiAssetDocumentGenerator::Info, 4> options;
        pGen->GetImportModes(sInputParentRelative, options);

        for (auto& option : options)
        {
          option.m_pGenerator = pGen;
        }

        pData->m_ImportOptions.PushBackRange(options);
      }
    }
  }
}

void xiiAssetDocumentGenerator::SortAndSelectBestImportOption(xiiDynamicArray<xiiAssetDocumentGenerator::ImportData>& allImports)
{
  allImports.Sort([](const xiiAssetDocumentGenerator::ImportData& lhs, const xiiAssetDocumentGenerator::ImportData& rhs) -> bool { return lhs.m_sInputFileParentRelative < rhs.m_sInputFileParentRelative; });

  for (auto& singleImport : allImports)
  {
    singleImport.m_ImportOptions.Sort([](const xiiAssetDocumentGenerator::Info& lhs, const xiiAssetDocumentGenerator::Info& rhs) -> bool { return xiiStringUtils::Compare_NoCase(xiiTranslate(lhs.m_sName), xiiTranslate(rhs.m_sName)) < 0; });

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
