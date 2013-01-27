#ifndef _AUDEX_ERROR_DIALOG_
#define _AUDEX_ERROR_DIALOG_

#include <QtCore/QString>
class QWidget;

namespace ErrorDialog {

extern void show(QWidget *parent, const QString &message, const QString &details, const QString &caption=QString());

}
#endif
