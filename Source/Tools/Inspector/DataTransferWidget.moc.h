#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_DataTransferWidget.h>
#include <ads/DockWidget.h>

class xiiQtDataWidget : public ads::CDockWidget, public Ui_DataTransferWidget
{
public:
  Q_OBJECT

public:
  xiiQtDataWidget(QWidget* pParent = nullptr);

  static xiiQtDataWidget* s_pWidget;

private Q_SLOTS:
  virtual void on_ButtonRefresh_clicked();
  virtual void on_ComboTransfers_currentIndexChanged(int index);
  virtual void on_ComboItems_currentIndexChanged(int index);
  virtual void on_ButtonSave_clicked();
  virtual void on_ButtonOpen_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct TransferDataObject
  {
    xiiString                        m_sMimeType;
    xiiString                        m_sExtension;
    xiiContiguousMemoryStreamStorage m_Storage;
    xiiString                        m_sFileName;
  };

  struct TransferData
  {
    xiiMap<xiiString, TransferDataObject> m_Items;
  };

  bool SaveToFile(TransferDataObject& item, xiiStringView sFile);

  TransferDataObject* GetCurrentItem();
  TransferData*       GetCurrentTransfer();

  xiiMap<xiiString, TransferData> m_Transfers;
};
