/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

/// Provides functionality for importing files as asset documents.
///
/// Derived from this class to add a custom importer (see existing derived classes for examples).
/// Each importer typically handles one target asset type.
class XII_EDITORFRAMEWORK_DLL xiiAssetDocumentGenerator : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetDocumentGenerator, xiiReflectedClass);

public:
  xiiAssetDocumentGenerator();
  ~xiiAssetDocumentGenerator();

  /// Describes one option to import an asset.
  ///
  /// The name is used to identify which option the user chose.
  /// The priority should be set to pick a 'likely' option for the UI to prefer.
  struct ImportMode
  {
    xiiAssetDocumentGenerator*   m_pGenerator = nullptr; ///< automatically set by xiiAssetDocumentGenerator
    xiiAssetDocGeneratorPriority m_Priority   = xiiAssetDocGeneratorPriority::Undecided;
    xiiString                    m_sName;
    xiiString                    m_sIcon;
  };

  /// Creates a list of all importable file extensions. Note that this is an expensive function so the the result should be cached.
  /// \param out_Extensions List of all file extensions that can be imported.
  static void GetSupportsFileTypes(xiiSet<xiiString>& out_extensions);

  /// Opens a file browse dialog to let the user choose which files to import.
  ///
  /// After the user chose one or multiple files, opens the "Asset Import" dialog to let them choose details.
  static void ImportAssets();

  /// Opens the "Asset Import" dialog to let the user choose how to import the given files.
  static void ImportAssets(const xiiDynamicArray<xiiString>& filesToImport);

  /// Imports the given file with the mode. Must be a mode that the generator supports.
  xiiStatus Import(xiiStringView sInputFileAbs, xiiStringView sMode, bool bOpenDocument);

  /// Used to fill out which import modes may be available for the given asset.
  ///
  /// Note: sAbsInputFile may be empty, in this case it should fill out the array for "general purpose" import (any file of the supported types).
  virtual void GetImportModes(xiiStringView sAbsInputFile, xiiDynamicArray<ImportMode>& out_modes) const = 0;

  /// Returns the target asset document file extension.
  virtual xiiStringView GetDocumentExtension() const = 0;

  /// Allows to merge the import modes of multiple generators in the UI in one group.
  virtual xiiStringView GetGeneratorGroup() const = 0;

  /// Tells the generator to create a new asset document with the chosen mode.
  virtual xiiStatus Generate(xiiStringView sInputFileAbs, xiiStringView sMode, xiiDynamicArray<xiiDocument*>& out_generatedDocuments) = 0;

  /// Returns whether this generator supports the given file type for import.
  bool SupportsFileType(xiiStringView sFile) const;

  /// Instantiates all currently available generators.
  static void CreateGenerators(xiiHybridArray<xiiAssetDocumentGenerator*, 16>& out_generators);

  /// Destroys the previously instantiated generators.
  static void DestroyGenerators(const xiiHybridArray<xiiAssetDocumentGenerator*, 16>& generators);

protected:
  void AddSupportedFileType(xiiStringView sExtension);

private:
  void BuildFileDialogFilterString(xiiStringBuilder& out_sFilter) const;
  void AppendFileFilterStrings(xiiStringBuilder& out_sFilter, bool& ref_bSemicolon) const;

  friend class xiiQtAssetImportDlg;

  struct ImportGroupOptions
  {
    xiiString m_sGroup;
    xiiString m_sInputFileRelative;
    xiiString m_sInputFileAbsolute;
    xiiInt32  m_iSelectedOption = -1;

    xiiHybridArray<xiiAssetDocumentGenerator::ImportMode, 4> m_ImportOptions;
  };

  static void SortAndSelectBestImportOption(xiiDynamicArray<xiiAssetDocumentGenerator::ImportGroupOptions>& allImports);
  static void CreateImportOptionList(const xiiDynamicArray<xiiString>& filesToImport, xiiDynamicArray<xiiAssetDocumentGenerator::ImportGroupOptions>& allImports, const xiiHybridArray<xiiAssetDocumentGenerator*, 16>& generators);

  xiiHybridArray<xiiString, 16> m_SupportedFileTypes;
};
