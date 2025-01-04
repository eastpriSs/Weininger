#include "CodeEditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QToolTip>
#include <QScrollBar>
#include <QStringListModel>

#include "LanguageList.h"
#include "AnalyzerC.h"
#include "AnalyzerApraam.h"

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);
    setUpCompleter(new QCompleter(this));

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::textChanged, this, &CodeEditor::updateCompleterPrefix);

    updateLineNumberAreaWidth();
    changeToLightTheme();
    charFont.setPointSize(charsSize);
    setFont(charFont);
    setTabsSize(4);
}

void CodeEditor::setUpCompleter(QCompleter *completer)
{
    if (compltr)
        compltr->disconnect(this);

    compltr = completer;

    if (!compltr)
        return;

    compltr->setModel(new QStringListModel({"someone"}));
    compltr->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    compltr->setCaseSensitivity(Qt::CaseSensitive);
    compltr->setWrapAround(false);
    compltr->setWidget(this);
    compltr->setCompletionMode(QCompleter::PopupCompletion);
    connect(compltr, QOverload<const QString &>::of(&QCompleter::activated),
                     this, &CodeEditor::insertCompletion);
}

void CodeEditor::insertCompletion(const QString &completion)
{
    if (compltr->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - compltr->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

QString CodeEditor::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void CodeEditor::focusInEvent(QFocusEvent *e)
{
    if (compltr)
        compltr->setWidget(this);
    QPlainTextEdit::focusInEvent(e);
}

void CodeEditor::setTabsSize(const int& size) noexcept
{
    tabsSize = size;
    setTabStopDistance(QFontMetrics(QFont()).horizontalAdvance(' ') * size * 2);
}

void CodeEditor::increaseCharsSize() noexcept
{
    charFont.setPointSize(++charsSize);
    setFont(charFont);
}

void CodeEditor::reduceCharsSize() noexcept
{
    charFont.setPointSize(--charsSize);
    setFont(charFont);
}

int CodeEditor::lineNumberAreaWidth()
{
    return 100;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();

    updateFoldMarkers(); // Обновление маркеров свертывания
}

void CodeEditor::changeModeToCommandInput()
{
    mode = codeEditorMode::commandMode;
    setFixedHeight(height() - 100);
    setReadOnly(true);
}

void CodeEditor::changeModeToFullEdit()
{
    mode = codeEditorMode::fullEditMode;
    setFixedHeight(height() + 100);
    setReadOnly(false);
}

void CodeEditor::languageChanged(QListWidget* list)
{
    logger.hide();
    if (list->currentItem()->text() == "C")  {
        AnalyzerC* a = new AnalyzerC();
        a->holdCompleterModel(compltr->model());
        highlighter.switchAnalyzer(a);

    } else if (list->currentItem()->text() == "Apraam Simcode") {
        logger.show();
        AnalyzerApraam* a = new AnalyzerApraam(&logger);
        a->holdCompleterModel(compltr->model());
        highlighter.switchAnalyzer(a);

    } else {
        highlighter.switchAnalyzer(new Analyzer());
    }
    highlighter.setDocument(document());
}

void CodeEditor::openSwitchingLanguageMenu()
{
    if (langList != nullptr)
        delete langList;

    langList = new LanguageList(supportedLanguages);
    langList->setWindowModality(Qt::WindowModal);
    langList->setFocus();
    connect(langList, &QListWidget::itemSelectionChanged, this,
            [this]()
            {
                this->languageChanged(langList);
            }
    );
    langList->show();
    QEventLoop loop;
    connect(this, SIGNAL(destroyed()), & loop, SLOT(quit()));
    loop.exec();
}

void CodeEditor::keyPressEventInCommandMode(QKeyEvent *event)
{
    switch(event->key())
    {
    case Qt::Key_Escape:
        changeModeToFullEdit();
        break;
    case Qt::Key_L:
        openSwitchingLanguageMenu();
        break;
    case Qt::Key_I:
        increaseCharsSize();
        break;
    case Qt::Key_R:
        reduceCharsSize();
        break;
    }
}


void CodeEditor::updateCompleterPrefix() {
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    QString prefix = cursor.selectedText();
    QPoint cursorPosRect = cursorRect().bottomRight();
    QSize rectSz = QSize(100,5);
    QPoint rectPos = QPoint(cursorPosRect.x() + rectSz.width(), cursorPosRect.y());
    QRect rect = QRect(rectPos,rectSz);
    if (!prefix.isEmpty()) {
        compltr->setCompletionPrefix(prefix);
        compltr->complete(rect);
    }
    else {
        compltr->popup()->hide();
    }
}

void CodeEditor::keyPressEventInEditMode(QKeyEvent *event)
{
    if (compltr && compltr->popup()->isVisible()) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            event->ignore();
            return;
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        if (mode == commandMode)
            changeModeToFullEdit();
        else
            changeModeToCommandInput();
    }

    if (event->key() == Qt::Key_Enter
        || event->key() == Qt::Key_Return
        || event->key() == Qt::Key_Tab) {
        if (compltr) {
            if (compltr->popup()->isVisible()) {
                compltr->popup()->hide();
                event->accept();
            } else {
                QPlainTextEdit::keyPressEvent(event);
            }
        }
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    if (mode == codeEditorMode::commandMode)
        keyPressEventInCommandMode(event);
    else if (mode == codeEditorMode::fullEditMode)
        keyPressEventInEditMode(event);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (highlightEnabled && !isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        selection.format.setBackground(currentLineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.format.setForeground(currentLineSymbolColor);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), 20, // magic const height 20 BIG TODO
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::changeToDarkTheme()
{
    setStyleSheet("color: white;");
    currentLineColor = QColor::fromRgb(53, 57, 66);         // DarkBlue
    currentLineSymbolColor = QColor::fromRgb(173, 216, 230);// LightBlue
    highlightCurrentLine();
    highlighter.turnOnDarkTheme();
    highlighter.setDocument(document());
}

void CodeEditor::changeToLightTheme()
{
    setStyleSheet("color: black;");
    currentLineColor = QColor::fromRgb(173, 205, 255);      // LightBlue 2
    currentLineSymbolColor = QColor::fromRgb(32, 47, 92);   // DarkBlue 2
    highlightCurrentLine();
    highlighter.turnOnLightTheme();
    highlighter.setDocument(document());
}

void CodeEditor::turnOnCurrentLineHighlighter()
{
    highlightEnabled = true;
    highlightCurrentLine();
}

void CodeEditor::turnOffCurrentLineHighlighter()
{
    highlightEnabled = false;
    highlightCurrentLine();
}

void CodeEditor::updateFoldMarkers() {
    qDeleteAll(foldMarkers);
    foldMarkers.clear();

    QTextBlock block = document()->begin();
    while (block.isValid()) {
        QString text = block.text().trimmed();
        if (text.endsWith("{") && block.next().isValid() && block.next() != findClosingBraceBlock(block)) {
            FoldMarker *marker = new FoldMarker(this);
            marker->setBlockIndex(block.blockNumber());
            marker->setFoldState(foldedBlocks.value(block.blockNumber(), false));
            connect(marker, &FoldMarker::toggled, this, &CodeEditor::toggleFold);

            QRect blockRect = blockBoundingGeometry(block).translated(contentOffset()).toRect();
            int tabWidth = fontMetrics().horizontalAdvance(' ') * 4;
            marker->move(blockRect.left() + tabWidth, blockRect.top());
            marker->show();
            foldMarkers.append(marker);
        }
        block = block.next();
    }
}

void CodeEditor::toggleFold(int blockIndex) {
    QTextBlock startBlock = document()->findBlockByNumber(blockIndex);
    if (!startBlock.isValid()) return;

    QTextBlock closingBlock = findClosingBraceBlock(startBlock);
    if (!closingBlock.isValid()) return;

    bool isCurrentlyFolded = foldedBlocks.value(blockIndex, false);
    foldedBlocks[blockIndex] = !isCurrentlyFolded;

    QTextBlock currentBlock = startBlock.next();

    // Обновляем видимость строк
    while (currentBlock.isValid() && currentBlock != closingBlock.next()) {
        currentBlock.setVisible(!isCurrentlyFolded);
        currentBlock = currentBlock.next();
    }

    // Обновляем состояние маркера
    for (FoldMarker *marker : foldMarkers) {
        if (marker->blockIndex() == blockIndex) {
            marker->setFoldState(!isCurrentlyFolded);
            break;
        }
    }

    // Обновляем представление редактора
    viewport()->update();
}

QTextBlock CodeEditor::findClosingBraceBlock(QTextBlock startBlock) {
    int openBraces = 1;
    QTextBlock block = startBlock.next();

    while (block.isValid()) {
        QString text = block.text().trimmed();
        openBraces += text.count("{");
        openBraces -= text.count("}");

        if (openBraces == 0) {
            return block;
        }
        block = block.next();
    }

    return QTextBlock(); // Возвращаем недействительный блок, если закрывающая скобка не найдена
}
