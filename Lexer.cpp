#include "Lexer.h"
#include <QDebug>

Lexer::Lexer(const QString& block)
{
    begin = block.begin();
    it = begin;
    forward = it + 1;
}


void Lexer::setScanningBlock(const QString& b)
{
    begin = b.constBegin();
    it = begin;
    forward = it + 1;
}

void Lexer::addKeyword(QString&& kw)
{
    keywords.append(kw);
}

Token Lexer::scan()
{
    // todo highlighting nums
    return Token();
}
