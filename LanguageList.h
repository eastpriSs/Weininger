#ifndef LANGUAGELIST_H
#define LANGUAGELIST_H

#include <QListWidget>
#include <QObject>
#include <QWidget>

class LanguageList final : public QListWidget
{

private:
    const QList<QString> languages = {"C", "Java", "Apraam", "Non"};

private slots:
    void keyPressEvent(QKeyEvent*) override;

public:
    LanguageList();
};

#endif // LANGUAGELIST_H
