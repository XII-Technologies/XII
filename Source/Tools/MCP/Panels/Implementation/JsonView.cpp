/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCP/MCPPCH.h>

#ifdef XII_USE_QT

#  include <Foundation/Math/Math.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Strings/StringView.h>
#  include <Foundation/Types/Types.h>
#  include <Foundation/Types/Variant.h>
#  include <MCP/Panels/JsonView.h>
#  include <MCPPlugin/Json.h>

#  include <QApplication>
#  include <QClipboard>
#  include <QColor>
#  include <QFontDatabase>
#  include <QHeaderView>
#  include <QImage>
#  include <QLabel>
#  include <QPixmap>
#  include <QPlainTextEdit>
#  include <QPushButton>
#  include <QRegularExpression>
#  include <QScrollArea>
#  include <QSyntaxHighlighter>
#  include <QTabWidget>
#  include <QTextDocument>
#  include <QTreeWidget>
#  include <QVBoxLayout>

namespace
{
  constexpr xiiUInt32 JsonChildrenPerPage = 128;
  constexpr xiiUInt32 JsonPreviewBytes    = 4 * xiiMCPJson::DefaultMaximumBytes;

  QString JsonText(xiiStringView sText)
  {
    return QString::fromUtf8(sText.GetStartPointer(), static_cast<qsizetype>(sText.GetElementCount()));
  }

  QString JsonToken(const xiiVariant& value)
  {
    xiiStringBuilder sToken;
    if (xiiMCPJson::Write(value, sToken, JsonPreviewBytes).Failed())
    {
      return {};
    }
    const auto token = JsonText(sToken);
    return token.size() > 160 ? token.left(160) + QStringLiteral("…") : token;
  }

  QString FormatJsonSource(const xiiVariant& value)
  {
    xiiStringBuilder sJson;
    if (xiiMCPJson::WriteIndented(value, sJson, JsonPreviewBytes).Succeeded() || xiiMCPJson::Write(value, sJson, JsonPreviewBytes).Succeeded())
    {
      return JsonText(sJson);
    }
    return {};
  }

  xiiUInt32 JsonChildCount(const xiiVariant& value)
  {
    if (value.IsA<xiiVariantDictionary>())
    {
      return value.Get<xiiVariantDictionary>().GetCount();
    }
    if (value.IsA<xiiVariantArray>())
    {
      return value.Get<xiiVariantArray>().GetCount();
    }
    return 0;
  }

  class xiiMCPJsonTreeItem final : public QTreeWidgetItem
  {
  public:
    xiiMCPJsonTreeItem(const QString& key, const xiiVariant& value) :
      m_Value(value), m_uiCount(JsonChildCount(value))
    {
      setText(0, key);
      if (value.IsA<xiiVariantDictionary>() || value.IsA<xiiVariantArray>())
      {
        setText(1, value.IsA<xiiVariantDictionary>() ? QString("Object (%1)").arg(m_uiCount) : QString("Array (%1)").arg(m_uiCount));
        setChildIndicatorPolicy(m_uiCount > 0 ? ShowIndicator : DontShowIndicator);
        return;
      }

      setText(1, JsonToken(value));
      const QColor scalarColor = value.IsNumber() && !value.IsA<bool>() ? QColor("#f4ce94") : QColor("#c1afff");
      setForeground(1, value.IsA<xiiString>() ? QColor("#a8d19b") : scalarColor);
    }

    void Populate()
    {
      if (m_uiCount == 0 || childCount() != 0)
      {
        return;
      }

      const xiiUInt32 uiEnd = m_uiStart + m_uiCount;
      if (m_uiCount > JsonChildrenPerPage)
      {
        for (xiiUInt32 uiStart = m_uiStart; uiStart < uiEnd; uiStart += JsonChildrenPerPage)
        {
          addChild(new xiiMCPJsonTreeItem(m_Value, uiStart, xiiMath::Min(JsonChildrenPerPage, uiEnd - uiStart)));
        }
      }
      else if (m_Value.IsA<xiiVariantArray>())
      {
        const auto& array = m_Value.Get<xiiVariantArray>();
        for (xiiUInt32 uiIndex = m_uiStart; uiIndex < uiEnd; ++uiIndex)
        {
          addChild(new xiiMCPJsonTreeItem(QString("[%1]").arg(uiIndex), array[uiIndex]));
        }
      }
      else
      {
        auto it = m_Value.Get<xiiVariantDictionary>().GetIterator();
        for (xiiUInt32 uiIndex = 0; uiIndex < uiEnd; ++uiIndex, ++it)
        {
          if (uiIndex >= m_uiStart)
          {
            addChild(new xiiMCPJsonTreeItem(JsonToken(it.Key()), it.Value()));
          }
        }
      }
    }

  private:
    xiiMCPJsonTreeItem(const xiiVariant& value, xiiUInt32 uiStart, xiiUInt32 uiCount) :
      m_Value(value), m_uiStart(uiStart), m_uiCount(uiCount)
    {
      setText(0, QString("%1–%2").arg(uiStart).arg(uiStart + uiCount - 1));
      setChildIndicatorPolicy(ShowIndicator);
    }

    xiiVariant m_Value;
    xiiUInt32  m_uiStart = 0;
    xiiUInt32  m_uiCount = 0;
  };

  class xiiMCPJsonHighlighter final : public QSyntaxHighlighter
  {
  public:
    explicit xiiMCPJsonHighlighter(QTextDocument* pDocument) :
      QSyntaxHighlighter(pDocument)
    {
    }

  protected:
    void highlightBlock(const QString& line) override
    {
      static const QRegularExpression tokens(QStringLiteral(R"json("(?:[^"\\]|\\.)*"(?:(\s*:))?|\b(?:true|false|null)\b|-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)json"));
      auto                            matches = tokens.globalMatch(line);
      while (matches.hasNext())
      {
        const auto match = matches.next();
        const auto token = match.capturedView();
        if (token.startsWith('"'))
        {
          const auto iLength = match.hasCaptured(1) ? match.capturedStart(1) - match.capturedStart() : token.size();
          setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(iLength), match.hasCaptured(1) ? QColor("#9dcaff") : QColor("#a8d19b"));
        }
        else
        {
          setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(token.size()), token.front().isLetter() ? QColor("#c1afff") : QColor("#f4ce94"));
        }
      }
    }
  };

  QPlainTextEdit* MakeJsonSource(const QString& name)
  {
    auto* pSource = new QPlainTextEdit();
    pSource->setAccessibleName(name);
    pSource->setReadOnly(true);
    pSource->setLineWrapMode(QPlainTextEdit::NoWrap);
    pSource->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    new xiiMCPJsonHighlighter(pSource->document());
    return pSource;
  }
} // namespace

xiiMCPJsonView::xiiMCPJsonView(const QString& title) :
  m_pTabs(new QTabWidget(this)),
  m_pTree(new QTreeWidget(m_pTabs)),
  m_pJson(MakeJsonSource(title + " JSON")),
  m_pResponse(MakeJsonSource("Original MCP response")),
  m_pImageScroll(new QScrollArea(m_pTabs)),
  m_pImage(new QLabel),
  m_pCopy(new QPushButton("Copy JSON", this))
{
  auto* pLayout = new QVBoxLayout(this);
  pLayout->setContentsMargins(0, 0, 0, 0);
  pLayout->addWidget(new QLabel(title));

  m_pTree->setAccessibleName(title + " tree");
  m_pTree->setHeaderLabels({"Key / index", "Value"});
  m_pTree->setUniformRowHeights(true);
  m_pTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
  m_pTree->setColumnWidth(0, 160);
  m_pTabs->addTab(m_pTree, "Tree");
  m_pTabs->addTab(m_pJson, "JSON");
  m_pTabs->addTab(m_pResponse, "Response");
  m_pTabs->setTabVisible(2, false);
  m_pImageScroll->setAlignment(Qt::AlignCenter);
  m_pImageScroll->setWidget(m_pImage);
  m_pImage->setAccessibleName("MCP image preview");
  m_pTabs->setTabVisible(m_pTabs->addTab(m_pImageScroll, "Image"), false);
  pLayout->addWidget(m_pTabs, 1);

  m_pCopy->setEnabled(false);
  pLayout->addWidget(m_pCopy, 0, Qt::AlignLeft);
  connect(m_pTree, &QTreeWidget::itemExpanded, this, [](QTreeWidgetItem* pItem) { static_cast<xiiMCPJsonTreeItem*>(pItem)->Populate(); });
  connect(m_pTabs, &QTabWidget::currentChanged, this, [this] { RefreshSource(); });
  connect(m_pCopy, &QPushButton::clicked, this, [this] {
    const auto& source = m_pTabs->currentWidget() == m_pResponse ? m_sResponse : GetJsonSource();
    if (!source.isEmpty())
    {
      QApplication::clipboard()->setText(source);
    }
  });
}

void xiiMCPJsonView::Clear(const QString& message)
{
  m_pTree->clear();
  m_Value = {};
  m_sJson.clear();
  m_sResponse.clear();
  m_bHasDocument = false;
  m_pJson->clear();
  m_pResponse->clear();
  SetImagePreview({});
  m_pTabs->setTabVisible(2, false);
  m_pTabs->setTabEnabled(0, false);
  m_pJson->setPlaceholderText(message);
  m_pTabs->setCurrentWidget(m_pJson);
  m_pCopy->setEnabled(false);
}

void xiiMCPJsonView::SetMessage(const QString& message)
{
  Clear();
  m_pJson->setPlainText(message);
}

void xiiMCPJsonView::SetDocument(const xiiVariant& value, xiiStringView sResponse)
{
  const bool bHadDocument = m_bHasDocument;
  m_pTree->clear();
  m_Value = value;
  m_sJson.clear();
  m_sResponse = JsonText(sResponse);
  m_pJson->clear();
  m_pResponse->clear();
  SetImagePreview({});
  m_pJson->setPlaceholderText({});
  m_bHasDocument = true;
  m_pTabs->setTabEnabled(0, true);
  m_pTabs->setTabVisible(2, !m_sResponse.isEmpty());

  auto* pRoot = new xiiMCPJsonTreeItem("root", m_Value);
  m_pTree->addTopLevelItem(pRoot);
  pRoot->setExpanded(true);
  if (!bHadDocument)
  {
    m_pTabs->setCurrentWidget(m_pTree);
  }
  RefreshSource();
}

void xiiMCPJsonView::SetImagePreview(const QImage& image)
{
  m_pTabs->setTabVisible(m_pTabs->indexOf(m_pImageScroll), !image.isNull());
  if (image.isNull())
  {
    m_pImage->clear();
    return;
  }
  m_pImage->setPixmap(QPixmap::fromImage(image));
  m_pImage->resize(image.size());
  m_pTabs->setCurrentWidget(m_pImageScroll);
}

const QString& xiiMCPJsonView::GetJsonSource()
{
  if (m_sJson.isEmpty())
  {
    m_sJson = FormatJsonSource(m_Value);
  }
  return m_sJson;
}

void xiiMCPJsonView::RefreshSource()
{
  m_pCopy->setEnabled(m_bHasDocument);
  const bool bResponse = m_pTabs->currentWidget() == m_pResponse;
  m_pCopy->setText(bResponse ? "Copy response" : "Copy JSON");
  if (!m_bHasDocument)
  {
    return;
  }
  if (bResponse && m_pResponse->document()->isEmpty())
  {
    const auto response = m_sResponse.toUtf8();
    xiiVariant envelope;
    const auto formatted = xiiMCPJson::Parse(xiiStringView(response.constData(), static_cast<xiiUInt32>(response.size())), envelope).Succeeded() ?
      FormatJsonSource(envelope) :
      QString();
    m_pResponse->setPlainText(formatted.isEmpty() ? m_sResponse : formatted);
  }
  else if (m_pTabs->currentWidget() == m_pJson && m_pJson->document()->isEmpty())
  {
    const auto& json = GetJsonSource();
    m_pJson->setPlaceholderText(json.isEmpty() ? "JSON exceeds the source preview limit." : QString());
    m_pJson->setPlainText(json);
  }
}

#endif
