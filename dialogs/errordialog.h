#ifndef ERROR_DIALOG_H
#define ERROR_DIALOG_H

#include <QString>

#include <KMessageBox>

class QWidget;

namespace ErrorDialog
{
extern void show(QWidget *parent, const QString &message, const QString &details, const QString &caption = QString());

}
#endif
