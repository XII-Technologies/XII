#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Status.h>

class xiiDocument;

enum class xiiAssetDocGeneratorPriority
{
  Undecided,
  LowPriority,
  DefaultPriority,
  HighPriority,
  ENUM_COUNT
};

class XII_EDITORFRAMEWORK_DLL xiiAssetDocumentGenerator : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetDocumentGenerator, xiiReflectedClass);

public:
  xiiAssetDocumentGenerator();
  ~xiiAssetDocumentGenerator();

  struct Info
  {
    xiiAssetDocumentGenerator*   m_pGenerator = nullptr;      ///< automatically set by xiiAssetDocumentGenerator
    xiiAssetDocGeneratorPriority m_Priority;                  ///< has to be specified by generator
    xiiString                    m_sOutputFileParentRelative; ///< has to be specified by generator
    xiiString                    m_sOutputFileAbsolute;       ///< automatically generated from m_sOutputFileParentRelative
    xiiString                    m_sName;                     ///< has to be specified by generator, used to know which action to take by Generate()
    xiiString                    m_sIcon;                     ///< has to be specified by generator
  };

  struct ImportData
  {
    xiiString m_sGroup;
    xiiString m_sInputFileRelative;
    xiiString m_sInputFileParentRelative;
    xiiString m_sInputFileAbsolute;
    xiiInt32  m_iSelectedOption = -1;
    xiiString m_sImportMessage; // error text or "already exists"
    bool      m_bDoNotImport = false;

    xiiHybridArray<xiiAssetDocumentGenerator::Info, 4> m_ImportOptions;
  };

  static void ImportAssets();
  static void ImportAssets(const xiiHybridArray<xiiString, 16>& filesToImport);
  static void ExecuteImport(xiiDynamicArray<xiiAssetDocumentGenerator::ImportData>& allImports);

  virtual void        GetImportModes(const char* szParentDirRelativePath, xiiHybridArray<xiiAssetDocumentGenerator::Info, 4>& out_Modes) const       = 0;
  virtual xiiStatus   Generate(const char* szDataDirRelativePath, const xiiAssetDocumentGenerator::Info& mode, xiiDocument*& out_pGeneratedDocument) = 0;
  virtual const char* GetDocumentExtension() const                                                                                                   = 0;
  virtual const char* GetGeneratorGroup() const                                                                                                      = 0;

  bool SupportsFileType(const char* szFile) const;
  void BuildFileDialogFilterString(xiiStringBuilder& out_Filter) const;
  void AppendFileFilterStrings(xiiStringBuilder& out_Filter, bool& semicolon) const;

protected:
  void AddSupportedFileType(const char* szExtension);

private:
  static void      CreateGenerators(xiiHybridArray<xiiAssetDocumentGenerator*, 16>& out_Generators);
  static void      DestroyGenerators(xiiHybridArray<xiiAssetDocumentGenerator*, 16>& generators);
  static xiiResult DetermineInputAndOutputFiles(ImportData& data, Info& option);
  static void      SortAndSelectBestImportOption(xiiDynamicArray<xiiAssetDocumentGenerator::ImportData>& allImports);
  static void      CreateImportOptionList(const xiiHybridArray<xiiString, 16>&                    filesToImport,
                                          xiiDynamicArray<xiiAssetDocumentGenerator::ImportData>& allImports,
                                          const xiiHybridArray<xiiAssetDocumentGenerator*, 16>&   generators);

  xiiHybridArray<xiiString, 16> m_SupportedFileTypes;
};
