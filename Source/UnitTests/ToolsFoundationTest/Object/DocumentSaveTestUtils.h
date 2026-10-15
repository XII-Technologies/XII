/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <TestFramework/Framework/TestFramework.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

namespace xiiDocumentSaveTestUtils
{
  class SaveTestDocument : public xiiTestDocument
  {
  public:
    explicit SaveTestDocument(xiiStringView path) : xiiTestDocument(path) {}
    void FinishPendingSaves() { xiiDocument::BeforeClosing(); }

    void      SetWritable(bool writable) { SetReadOnly(!writable); }
    xiiUInt32 m_uiSerializationCount = 0;
    xiiUInt32 m_uiAfterSaveCount     = 0;

  protected:
    xiiTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override
    {
      ++m_uiSerializationCount;
      return xiiTestDocument::InternalSaveDocument(callback);
    }
    void InternalAfterSaveDocument() override { ++m_uiAfterSaveCount; }
  };

  class SaveTestManager : public xiiDocumentManager
  {
  public:
    SaveTestManager()
    {
      m_Type.m_sDocumentTypeName = "SaveValidation";
      m_Type.m_sFileExtension    = "xiiSaveTest";
      m_Type.m_pDocumentType     = xiiGetStaticRTTI<xiiTestDocument>();
      m_Type.m_pManager          = this;
    }
    ~SaveTestManager() override { CloseAllDocumentsOfManager(); }

  private:
    xiiStatus InternalCreateDocument(xiiStringView, xiiStringView path, bool, xiiDocument*& out_document, const xiiDocumentObject*) override
    {
      out_document = new SaveTestDocument(path);
      return XII_SUCCESS;
    }
    void                      InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& types) const override { types.PushBack(&m_Type); }
    xiiDocumentTypeDescriptor m_Type;
  };

  inline xiiResult ReadContents(xiiStringView path, xiiContiguousMemoryStreamStorage& storage)
  {
    xiiFileReader file;
    XII_SUCCEED_OR_RETURN(file.Open(path));
    storage.Clear();
    storage.ReadAll(file);
    return XII_SUCCESS;
  }

  inline xiiResult WriteContents(xiiStringView path, const xiiContiguousMemoryStreamStorage& storage)
  {
    xiiDeferredFileWriter file;
    file.SetOutput(path);
    XII_SUCCEED_OR_RETURN(storage.CopyToStream(file));
    return file.Close();
  }

  inline xiiStatus AddContent(xiiDocument& document, xiiUuid& known, xiiUuid& missing)
  {
    auto* accessor = document.GetObjectAccessor();
    accessor->StartTransaction("Initial content");
    bool finished = false;
    XII_SCOPE_EXIT(if (!finished) accessor->CancelTransaction());
    XII_SUCCEED_OR_RETURN(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiIntegerStruct>(), known));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(accessor->GetObject(known), "Int32", 4711));
    XII_SUCCEED_OR_RETURN(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiIntegerStruct>(), missing));
    XII_SUCCEED_OR_RETURN(accessor->SetValueByName(accessor->GetObject(missing), "Int32", 7829));
    accessor->FinishTransaction();
    finished = true;
    return XII_SUCCESS;
  }

  inline xiiResult EditContent(xiiDocument& document, const xiiUuid& known)
  {
    auto* accessor = document.GetObjectAccessor();
    accessor->StartTransaction("Edit known content");
    if (accessor->SetValueByName(accessor->GetObject(known), "Int32", 9001).Failed())
    {
      accessor->CancelTransaction();
      return XII_FAILURE;
    }
    accessor->FinishTransaction();
    return XII_SUCCESS;
  }

  inline bool MakeSeed(SaveTestManager& manager, xiiStringView path, xiiUuid& known, xiiUuid& missing)
  {
    xiiDocument* document = nullptr;
    if (!XII_TEST_BOOL(manager.CreateDocument("SaveValidation", path, document, xiiDocumentFlags::EmptyDocument).Succeeded()))
      return false;
    XII_SCOPE_EXIT(manager.CloseDocument(document));
    return XII_TEST_BOOL(AddContent(*document, known, missing).Succeeded()) && XII_TEST_BOOL(document->SaveDocument().Succeeded());
  }
} // namespace xiiDocumentSaveTestUtils
