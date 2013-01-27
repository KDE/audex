#include "errordialog.h"
#include <KDE/KMessageBox>

namespace ErrorDialog {

void show(QWidget *parent, const QString &message, const QString &details, const QString &caption) {
  if(details.isEmpty()) {
    KMessageBox::error(parent, message, caption);
  }
  else {
    KMessageBox::detailedError(parent, message, details, caption);      
  }
}

}
