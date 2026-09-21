/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>
#include <Foundation/Types/Uuid.h>
#include <GraphicsFoundation/Device/SwapChain.h>

class xiiEditorEngineSyncObjectMsg;
class xiiEditorEngineSyncObject;
class xiiEditorEngineDocumentMsg;
class xiiEngineProcessViewContext;
class xiiEngineProcessCommunicationChannel;
class xiiProcessMessage;
class xiiExportDocumentMsgToEngine;
class xiiCreateThumbnailMsgToEngine;
struct xiiResourceEvent;

struct XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineProcessDocumentContextFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None        = 0U,
    CreateWorld = XII_BIT(0),
    Default     = None
  };

  struct Bits
  {
    StorageType CreateWorld : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiEngineProcessDocumentContextFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORENGINEPROCESSFRAMEWORK_DLL, xiiEngineProcessDocumentContextFlags);

/// A document context is the counter part to an editor document on the engine side.
///
/// For every document in the editor that requires engine output (rendering, picking, etc.), there is a xiiEngineProcessDocumentContext
/// created in the engine process.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineProcessDocumentContext : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEngineProcessDocumentContext, xiiReflectedClass);

public:
  xiiEngineProcessDocumentContext(xiiBitflags<xiiEngineProcessDocumentContextFlags> flags);
  virtual ~xiiEngineProcessDocumentContext();

  virtual void Initialize(const xiiUuid& documentGuid, const xiiVariant& metaData, xiiEngineProcessCommunicationChannel* pIPC, xiiStringView sDocumentType);
  void         Deinitialize();

  /// Returns the document type for which this context was created. Useful in case a context may be used for multiple document types.
  xiiStringView GetDocumentType() const { return m_sDocumentType; }

  void         SendProcessMessage(xiiProcessMessage* pMsg = nullptr);
  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg);

  static xiiEngineProcessDocumentContext* GetDocumentContext(xiiUuid guid);
  static void                             AddDocumentContext(xiiUuid guid, const xiiVariant& metaData, xiiEngineProcessDocumentContext* pView, xiiEngineProcessCommunicationChannel* pIPC, xiiStringView sDocumentType);
  static bool                             PendingOperationsInProgress();
  static void                             UpdateDocumentContexts();
  static void                             DestroyDocumentContext(xiiUuid guid);

  // Returns the bounding box of the objects in the world.
  xiiBoundingBoxSphere GetWorldBounds(xiiWorld* pWorld);

  void ProcessEditorEngineSyncObjectMsg(const xiiEditorEngineSyncObjectMsg& msg);

  const xiiUuid& GetDocumentGuid() const { return m_DocumentGuid; }

  virtual void Reset();
  void         ClearExistingObjects();

  xiiIPCObjectMirrorEngine                    m_Mirror;
  xiiWorldRttiConverterContext                m_Context; // TODO: Move actual context into the EngineProcessDocumentContext
  virtual xiiWorldRttiConverterContext&       GetContext() { return m_Context; }
  virtual const xiiWorldRttiConverterContext& GetContext() const { return m_Context; }

  xiiWorld* GetWorld() const { return m_pWorld; }

  /// Tries to resolve a 'reference' (given in pData) to a xiiGameObject.
  virtual xiiGameObjectHandle ResolveStringToGameObjectHandle(const void* pString, xiiComponentHandle hThis, xiiStringView sProperty) const;

protected:
  virtual void OnInitialize();
  virtual void OnDeinitialize();

  /// Needs to be implemented to create a view context used for windows and thumbnails rendering.
  virtual xiiEngineProcessViewContext* CreateViewContext() = 0;
  /// Needs to be implemented to destroy the view context created in CreateViewContext.
  virtual void DestroyViewContext(xiiEngineProcessViewContext* pContext) = 0;

  /// Should return true if this context has any operation in progress like thumbnail rendering
  /// and thus needs to continue rendering even if no new messages from the editor come in.
  virtual bool PendingOperationInProgress() const;

  /// A tick functions that allows each document context to do processing that continues
  /// over multiple frames and can't be handled in HandleMessage directly.
  ///
  /// Make sure to call the base implementation when overwriting as this handles the thumbnail
  /// rendering that takes multiple frames to complete.
  virtual void UpdateDocumentContext();

  /// Exports to current document resource to file. Make sure to write xiiAssetFileHeader at the start of it.
  virtual xiiStatus ExportDocument(const xiiExportDocumentMsgToEngine* pMsg);
  void              UpdateSyncObjects();

  /// Creates the thumbnail view context. It uses 'CreateViewContext' in combination with an off-screen render target.
  void CreateThumbnailViewContext(const xiiCreateThumbnailMsgToEngine* pMsg);

  /// Once a thumbnail is successfully rendered, the thumbnail view context is destroyed again.
  void DestroyThumbnailViewContext();

  /// Overwrite this function to apply the thumbnail render settings to the given context.
  ///
  /// Return false if you need more frames to be rendered to setup everything correctly.
  /// If true is returned for 'ThumbnailConvergenceFramesTarget' frames in a row the thumbnail image is taken.
  /// This is to allow e.g. camera updates after more resources have been streamed in. The frame counter
  /// will start over to count to 'ThumbnailConvergenceFramesTarget' when a new resource is being loaded
  /// to make sure we do not make an image of half-streamed in data.
  virtual bool UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext);

  /// Called before a thumbnail context is created.
  virtual void OnThumbnailViewContextRequested() {}
  /// Called after a thumbnail context was created. Allows to insert code before the thumbnail is generated.
  virtual void OnThumbnailViewContextCreated();
  /// Called before a thumbnail context is destroyed. Used for cleanup of what was done in OnThumbnailViewContextCreated()
  virtual void OnDestroyThumbnailViewContext();

  xiiWorld* m_pWorld = nullptr;

  /// Sets or removes the given tag on the object and optionally all children
  void SetTagOnObject(const xiiUuid& object, const char* szTag, bool bSet, bool recursive);

  /// Sets the given tag on the object and all children.
  void SetTagRecursive(xiiGameObject* pObject, const xiiTag& tag);
  /// Clears the given tag on the object and all children.
  void ClearTagRecursive(xiiGameObject* pObject, const xiiTag& tag);

protected:
  const xiiEngineProcessViewContext* GetViewContext(xiiUInt32 uiView) const
  {
    return uiView >= m_ViewContexts.GetCount() ? nullptr : m_ViewContexts[uiView];
  }

private:
  friend class xiiEditorEngineSyncObject;

  void                       AddSyncObject(xiiEditorEngineSyncObject* pSync);
  void                       RemoveSyncObject(xiiEditorEngineSyncObject* pSync);
  xiiEditorEngineSyncObject* FindSyncObject(const xiiUuid& guid);


private:
  void ClearViewContexts();

  // Maps a document guid to the corresponding context that handles that document on the engine side
  static xiiHashTable<xiiUuid, xiiEngineProcessDocumentContext*> s_DocumentContexts;

  /// Removes all sync objects that are tied to this context
  void CleanUpContextSyncObjects();

protected:
  xiiBitflags<xiiEngineProcessDocumentContextFlags> m_Flags;
  xiiUuid                                           m_DocumentGuid;
  xiiVariant                                        m_MetaData;

  xiiEngineProcessCommunicationChannel*           m_pIPC = nullptr;
  xiiHybridArray<xiiEngineProcessViewContext*, 4> m_ViewContexts;

  xiiMap<xiiUuid, xiiEditorEngineSyncObject*> m_SyncObjects;

private:
  enum Constants
  {
    ThumbnailSuperscaleFactor        = 2, ///< Thumbnail render target size is multiplied by this and then the final image is downscaled again. Needs to be power-of-two.
    ThumbnailConvergenceFramesTarget = 4  ///< Due to multi-threaded rendering, this must be at least 4
  };

  xiiUInt8                     m_uiThumbnailConvergenceFrames = 0;
  xiiUInt16                    m_uiThumbnailWidth             = 0;
  xiiUInt16                    m_uiThumbnailHeight            = 0;
  xiiEngineProcessViewContext* m_pThumbnailViewContext        = nullptr;
  xiiRenderTargets             m_ThumbnailRenderTargets;
  xiiSharedPtr<xiiGALTexture>  m_pThumbnailColorRT;
  xiiSharedPtr<xiiGALTexture>  m_pThumbnailColorRTStaging;
  xiiSharedPtr<xiiGALTexture>  m_pThumbnailDepthRT;
  bool                         m_bWorldSimStateBeforeThumbnail = false;
  xiiString                    m_sDocumentType;

  //////////////////////////////////////////////////////////////////////////
  // GameObject reference resolution
private:
  struct GoReferenceTo
  {
    xiiStringView m_sComponentProperty;
    xiiUuid       m_ReferenceToGameObject;
  };

  struct GoReferencedBy
  {
    xiiStringView m_sComponentProperty;
    xiiUuid       m_ReferencedByComponent;
  };

  // Components reference GameObjects
  mutable xiiMap<xiiUuid, xiiHybridArray<GoReferenceTo, 4>> m_GoRef_ReferencesTo;

  // GameObjects referenced by Components
  mutable xiiMap<xiiUuid, xiiHybridArray<GoReferencedBy, 4>> m_GoRef_ReferencedBy;

  void WorldRttiConverterContextEventHandler(const xiiWorldRttiConverterContext::Event& e);
};
