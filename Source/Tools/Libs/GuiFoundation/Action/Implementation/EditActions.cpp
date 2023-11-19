#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// xiiEditActions
////////////////////////////////////////////////////////////////////////

xiiActionDescriptorHandle xiiEditActions::s_hEditCategory;
xiiActionDescriptorHandle xiiEditActions::s_hCopy;
xiiActionDescriptorHandle xiiEditActions::s_hPaste;
xiiActionDescriptorHandle xiiEditActions::s_hPasteAsChild;
xiiActionDescriptorHandle xiiEditActions::s_hPasteAtOriginalLocation;
xiiActionDescriptorHandle xiiEditActions::s_hDelete;

void xiiEditActions::RegisterActions()
{
  s_hEditCategory            = XII_REGISTER_CATEGORY("EditCategory");
  s_hCopy                    = XII_REGISTER_ACTION_1("Selection.Copy", xiiActionScope::Document, "Document", "Ctrl+C", xiiEditAction, xiiEditAction::ButtonType::Copy);
  s_hPaste                   = XII_REGISTER_ACTION_1("Selection.Paste", xiiActionScope::Document, "Document", "Ctrl+V", xiiEditAction, xiiEditAction::ButtonType::Paste);
  s_hPasteAsChild            = XII_REGISTER_ACTION_1("Selection.PasteAsChild", xiiActionScope::Document, "Document", "", xiiEditAction, xiiEditAction::ButtonType::PasteAsChild);
  s_hPasteAtOriginalLocation = XII_REGISTER_ACTION_1("Selection.PasteAtOriginalLocation", xiiActionScope::Document, "Document", "", xiiEditAction, xiiEditAction::ButtonType::PasteAtOriginalLocation);
  s_hDelete                  = XII_REGISTER_ACTION_1("Selection.Delete", xiiActionScope::Document, "Document", "", xiiEditAction, xiiEditAction::ButtonType::Delete);
}

void xiiEditActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hEditCategory);
  xiiActionManager::UnregisterAction(s_hCopy);
  xiiActionManager::UnregisterAction(s_hPaste);
  xiiActionManager::UnregisterAction(s_hPasteAsChild);
  xiiActionManager::UnregisterAction(s_hPasteAtOriginalLocation);
  xiiActionManager::UnregisterAction(s_hDelete);
}

void xiiEditActions::MapActions(xiiStringView sMapping, bool bDeleteAction, bool bAdvancedPasteActions)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", sMapping);

  pMap->MapAction(s_hEditCategory, "G.Edit", 3.5f);

  pMap->MapAction(s_hCopy, "G.Edit", "EditCategory", 1.0f);
  pMap->MapAction(s_hPaste, "G.Edit", "EditCategory", 2.0f);

  if (bAdvancedPasteActions)
  {
    pMap->MapAction(s_hPasteAsChild, "G.Edit", "EditCategory", 2.5f);
    pMap->MapAction(s_hPasteAtOriginalLocation, "G.Edit", "EditCategory", 2.7f);
  }

  if (bDeleteAction)
    pMap->MapAction(s_hDelete, "G.Edit", "EditCategory", 3.0f);
}


void xiiEditActions::MapContextMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", sMapping);

  pMap->MapAction(s_hEditCategory, "", 10.0f);

  pMap->MapAction(s_hCopy, "EditCategory", 1.0f);
  pMap->MapAction(s_hPasteAsChild, "EditCategory", 2.0f);
  pMap->MapAction(s_hDelete, "EditCategory", 3.0f);
}


void xiiEditActions::MapViewContextMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", sMapping);

  pMap->MapAction(s_hEditCategory, "", 10.0f);

  pMap->MapAction(s_hCopy, "EditCategory", 1.0f);
  pMap->MapAction(s_hPasteAsChild, "EditCategory", 2.0f);
  pMap->MapAction(s_hPasteAtOriginalLocation, "EditCategory", 2.5f);
  pMap->MapAction(s_hDelete, "EditCategory", 3.0f);
}

////////////////////////////////////////////////////////////////////////
// xiiEditAction
////////////////////////////////////////////////////////////////////////

xiiEditAction::xiiEditAction(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case xiiEditAction::ButtonType::Copy:
      SetIconPath(":/GuiFoundation/Icons/Copy.svg");
      break;
    case xiiEditAction::ButtonType::Paste:
      SetIconPath(":/GuiFoundation/Icons/Paste.svg");
      break;
    case xiiEditAction::ButtonType::PasteAsChild:
      SetIconPath(":/GuiFoundation/Icons/Paste.svg"); /// \todo Icon
      break;
    case xiiEditAction::ButtonType::PasteAtOriginalLocation:
      SetIconPath(":/GuiFoundation/Icons/Paste.svg");
      break;
    case xiiEditAction::ButtonType::Delete:
      SetIconPath(":/GuiFoundation/Icons/Delete.svg");
      break;
  }

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiEditAction::SelectionEventHandler, this));

  if (m_ButtonType == ButtonType::Copy || m_ButtonType == ButtonType::Delete)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}

xiiEditAction::~xiiEditAction()
{
  if (m_Context.m_pDocument)
  {
    m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiEditAction::SelectionEventHandler, this));
  }
}

void xiiEditAction::Execute(const xiiVariant& value)
{
  switch (m_ButtonType)
  {
    case xiiEditAction::ButtonType::Copy:
    {
      xiiStringBuilder sMimeType;

      xiiAbstractObjectGraph graph;
      if (!m_Context.m_pDocument->CopySelectedObjects(graph, sMimeType))
        break;

      // Serialize to string
      xiiContiguousMemoryStreamStorage streamStorage;
      xiiMemoryStreamWriter            memoryWriter(&streamStorage);
      xiiAbstractGraphDdlSerializer::Write(memoryWriter, &graph, nullptr, false);
      memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

      // Write to clipboard
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData*  mimeData  = new QMimeData();
      QByteArray  encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize32());

      mimeData->setData(sMimeType.GetData(), encodedData);
      mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
      clipboard->setMimeData(mimeData);
    }
    break;

    case xiiEditAction::ButtonType::Paste:
    case xiiEditAction::ButtonType::PasteAsChild:
    case xiiEditAction::ButtonType::PasteAtOriginalLocation:
    {
      // Check for clipboard data of the correct type.
      QClipboard* clipboard = QApplication::clipboard();
      auto        mimedata  = clipboard->mimeData();

      xiiHybridArray<xiiString, 4> MimeTypes;
      m_Context.m_pDocument->GetSupportedMimeTypesForPasting(MimeTypes);

      xiiInt32 iFormat = -1;
      {
        for (xiiUInt32 i = 0; i < MimeTypes.GetCount(); ++i)
        {
          if (mimedata->hasFormat(MimeTypes[i].GetData()))
          {
            iFormat = i;
            break;
          }
        }

        if (iFormat < 0)
          break;
      }

      // Paste at current selected object.
      xiiPasteObjectsCommand cmd;
      cmd.m_sMimeType = MimeTypes[iFormat];

      QByteArray ba          = mimedata->data(MimeTypes[iFormat].GetData());
      cmd.m_sGraphTextFormat = ba.data();

      if (m_ButtonType == ButtonType::PasteAsChild)
      {
        if (!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty())
          cmd.m_Parent = m_Context.m_pDocument->GetSelectionManager()->GetSelection().PeekBack()->GetGuid();
      }
      else if (m_ButtonType == ButtonType::PasteAtOriginalLocation)
      {
        cmd.m_bAllowPickedPosition = false;
      }

      auto history = m_Context.m_pDocument->GetCommandHistory();

      history->StartTransaction("Paste");

      if (history->AddCommand(cmd).Failed())
        history->CancelTransaction();
      else
        history->FinishTransaction();
    }
    break;

    case xiiEditAction::ButtonType::Delete:
    {
      m_Context.m_pDocument->DeleteSelectedObjects();
    }
    break;
  }
}

void xiiEditAction::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  if (m_ButtonType == ButtonType::Copy || m_ButtonType == ButtonType::Delete)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}
