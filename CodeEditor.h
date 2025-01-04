#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QObject>
#include <QListWidget>
#include <QCompleter>
#include <QMap>
#include <QList>

#include "SyntaxHighlighter.h"
#include "LanguageList.h"
#include "ListLogger.h"
#include "FoldMarker.h"


class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    QString nameEditingFile = "untitled";

public:
    CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent*);
    int  lineNumberAreaWidth();
    void changeToDarkTheme();
    void changeToLightTheme();
    void turnOnCurrentLineHighlighter();
    void turnOffCurrentLineHighlighter();
    void setUpCompleter(QCompleter*);

protected:
    void resizeEvent(QResizeEvent*) override;
    void focusInEvent(QFocusEvent*) override;

private slots:
    void keyPressEvent(QKeyEvent*) override;
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect&, int);
    void languageChanged(QListWidget*);
    void insertCompletion(const QString&);

private :
    void changeModeToFullEdit();
    void changeModeToCommandInput();
    void keyPressEventInCommandMode(QKeyEvent*);
    void keyPressEventInEditMode(QKeyEvent*);
    void openSwitchingLanguageMenu();
    void updateCompleterPrefix();
    void setTabsSize(const int&) noexcept;
    void increaseCharsSize() noexcept;
    void reduceCharsSize() noexcept;
    QString textUnderCursor() const;

private:
    QList<FoldMarker*> foldMarkers; // Список маркеров
    QMap<int, bool> foldedBlocks;  // Хранение состояния свернутых блоков

    void updateFoldMarkers();                     // Метод для обновления маркеров
    void toggleFold(int blockIndex);              // Метод для сворачивания/разворачивания
    QTextBlock findClosingBraceBlock(QTextBlock); // Поиск закрывающей скобки

private:
    enum codeEditorMode {commandMode, fullEditMode};

    SyntaxHiglighter  highlighter = SyntaxHiglighter(document());
    ListLogger        logger = ListLogger(this);
    QColor            currentLineColor;
    QColor            currentLineSymbolColor;
    QFont             charFont = QFont();
    QWidget*          lineNumberArea = nullptr;
    LanguageList*     langList = nullptr;
    QCompleter*       compltr = nullptr;
    codeEditorMode    mode = codeEditorMode::fullEditMode;;
    int               tabsSize = 4;
    int               charsSize = 10;
    bool              highlightEnabled;
    const QStringList supportedLanguages = {"Apraam Simcode", "C", "Native"};
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor){}

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

#endif // CODEEDITOR_H
