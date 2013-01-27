/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "extractingprogressdialog.h"

ExtractingProgressDialog::ExtractingProgressDialog(ProfileModel *profile_model, CDDAModel *cdda_model, QWidget *parent) : KDialog(parent) {

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(i18n("Rip And Encode"));

  setButtons(Cancel);

  this->profile_model = profile_model;
  this->cdda_model = cdda_model;

  QString title = QString("%1 - %2").arg(cdda_model->artist()).arg(cdda_model->title());
  ui.label_header->setText(title);
  
  p_single_file = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SF_INDEX)).toBool();

  if (p_single_file) {
   
    ui.label_extracting->setText(i18n("Ripping whole CD as single track"));
    ui.label_encoding->setText(i18n("Encoding"));
    
  } else {
  
    ui.label_extracting->setText(i18n("Ripping Track 0 of %1", cdda_model->numOfAudioTracks()));
    ui.label_encoding->setText(i18n("Encoding Track 0 of %1", cdda_model->numOfAudioTracks()));
    
  }

  audex = new Audex(this, profile_model, cdda_model);

  connect(audex, SIGNAL(error(const QString&, const QString&)), this, SLOT(show_error(const QString&, const QString&)));
  connect(audex, SIGNAL(warning(const QString&)), this, SLOT(show_warning(const QString&)));
  connect(audex, SIGNAL(info(const QString&)), this, SLOT(show_info(const QString&)));
  connect(audex, SIGNAL(finished(bool)), this, SLOT(conclusion(bool)));
  connect(audex, SIGNAL(speedEncode(double)), this, SLOT(show_speed_encode(double)));
  connect(audex, SIGNAL(speedExtract(double)), this, SLOT(show_speed_extract(double)));
  connect(audex, SIGNAL(progressExtractTrack(int)), this, SLOT(show_progress_extract_track(int)));
  connect(audex, SIGNAL(progressExtractOverall(int)), this, SLOT(show_progress_extract_overall(int)));
  connect(audex, SIGNAL(progressEncodeTrack(int)), this, SLOT(show_progress_encode_track(int)));
  connect(audex, SIGNAL(progressEncodeOverall(int)), this, SLOT(show_progress_encode_overall(int)));
  connect(audex, SIGNAL(changedExtractTrack(int, int, const QString&, const QString&)), this, SLOT(show_changed_extract_track(int, int, const QString&, const QString&)));
  connect(audex, SIGNAL(changedEncodeTrack(int, int, const QString&)), this, SLOT(show_changed_encode_track(int, int, const QString&)));
  connect(audex, SIGNAL(timeout()), this, SLOT(ask_timeout()));
  connect(ui.details_button, SIGNAL(pressed()), this, SLOT(toggle_details()));

  showButtonSeparator(TRUE);
  finished = FALSE;

  progressbar_np_flag = FALSE;
  unity_message=QDBusMessage::createSignal("/Audex", "com.canonical.Unity.LauncherEntry", "Update");
}

ExtractingProgressDialog::~ExtractingProgressDialog() {

  delete audex;

}

int ExtractingProgressDialog::exec() {
  
  KConfigGroup grp(KGlobal::config(), "ExtractingProgressDialog");

  resize(600, 400);
  current_extract_overall = 0;
  current_encode_overall = 0;
  ui.details_button->setArrowType(grp.readEntry("Simple", TRUE) ? Qt::UpArrow : Qt::DownArrow);
  toggle_details();
  show();
  setModal(TRUE);
  if (audex->prepare()) {
    audex->start();
  }
  int rv = KDialog::exec();

  grp.writeEntry("Simple", (Qt::DownArrow==ui.details_button->arrowType()));
  
  return rv;
  
}

void ExtractingProgressDialog::calc_overall_progress() {
  ui.progressBar_overall->setValue((int)(((float)(current_extract_overall+current_encode_overall)/2.0f)+.5f));
  update_unity();
}

void ExtractingProgressDialog::toggle_details() {
  
  if (Qt::UpArrow == ui.details_button->arrowType()) {
    
    ui.details_button->setArrowType(Qt::DownArrow);
    ui.details->setVisible(FALSE);
    ui.label_overall->setVisible(FALSE);
    ui.label_overall_track->setVisible(TRUE);
    ui.progressBar_overall->setVisible(TRUE);
    resize(width(), 32);
    
  } else {
    
    ui.details_button->setArrowType(Qt::UpArrow);
    ui.details->setVisible(TRUE);
    ui.label_overall_track->setVisible(FALSE);
    
    if (cdda_model->numOfAudioTracksInSelection() < 2) {
      ui.label_overall->setVisible(FALSE);
      ui.progressBar_overall->setVisible(FALSE);
    } else {
      ui.label_overall->setVisible(TRUE);
    }
    resize(width(), 400);
    
  }
  
}

void ExtractingProgressDialog::slotButtonClicked(int button) {
  
  switch(button) {
    case Cancel:
      cancel();
      break;
    case Close:
      close();
      break;
    case User1:
      open_encoder_protocol_dialog();
      break;
    case User2:
      open_extract_protocol_dialog();
      break;
    default:
      KDialog::slotButtonClicked(button);
  }
  
}

void ExtractingProgressDialog::cancel() {

  if (finished) {

    close();

  } else {

    if (KMessageBox::warningYesNo(this, i18n("Do you really want to cancel?"),
				i18n("Cancel"),
				KStandardGuiItem::yes(),
				KStandardGuiItem::no()) == KMessageBox::Yes) {
      enableButton(Cancel, FALSE);
      audex->cancel();
    }

  }

}

void ExtractingProgressDialog::show_changed_extract_track(int no, int total, const QString& artist, const QString& title) {
  
  Q_UNUSED(artist);
  Q_UNUSED(title);
  
  if (!p_single_file) {
    
    ui.label_extracting->setText((1==total) ? i18n("Ripping Track") : i18n("Ripping Track %1 of %2", no, total));
    ui.label_overall_track->setText((1==total) ? i18n("Overall Progress") : i18n("Overall Progress (Ripping Track %1 of %2)", no, total));
    current_track = no;
    update_unity();
    
  } else {
    
    ui.label_extracting->setText(i18n("Ripping whole CD as single track"));
    ui.label_overall_track->setText(i18n("Overall Progress"));
    
  }
  
}

void ExtractingProgressDialog::show_changed_encode_track(int no, int total, const QString& filename) {

  Q_UNUSED(filename);
  
  if (no == 0) {
    ui.label_encoding->setText("<i>"+i18n("Waiting for an encoding job...")+"</i>");
    ui.label_speed_encoding->clear();
  } else {
    if (!p_single_file) ui.label_encoding->setText((1==total) ? i18n("Encoding Track") : i18n("Encoding Track %1 of %2", no, total));
  }

}

void ExtractingProgressDialog::show_progress_extract_track(int percent) {

  ui.progressBar_extracting->setValue(percent);

}

void ExtractingProgressDialog::show_progress_extract_overall(int percent) {

  current_extract_overall = percent;
  calc_overall_progress();
  
}

void ExtractingProgressDialog::show_progress_encode_track(int percent) {

  if (percent >= 0) {
    ui.progressBar_encoding->setValue(percent);
    if (progressbar_np_flag) {
      ui.progressBar_encoding->setRange(0, 100);
      ui.progressBar_encoding->setTextVisible(TRUE);
      progressbar_np_flag = FALSE;
    }
  } else {
    if (!progressbar_np_flag) {
      progressbar_np_flag = TRUE;
      ui.progressBar_encoding->setRange(0, 0);
      ui.progressBar_encoding->setTextVisible(FALSE);
    }
  }

}

void ExtractingProgressDialog::show_progress_encode_overall(int percent) {

  current_encode_overall = percent;
  calc_overall_progress();
  
}

void ExtractingProgressDialog::show_speed_encode(double speed) {

  QString s = QString("%1").arg((double)speed, 0, 'f', 2);
  ui.label_speed_encoding->setText("<i>"+i18n("Speed: %1x", s)+"</i>");

}

void ExtractingProgressDialog::show_speed_extract(double speed) {

  QString s = QString("%1").arg((double)speed, 0, 'f', 2);
  ui.label_speed_extracting->setText("<i>"+i18n("Speed: %1x", s)+"</i>");

}

void ExtractingProgressDialog::conclusion(bool successful) {

  QFlags<KDialog::ButtonCode> buttons = Close;

  finished = TRUE;
  
  update_unity();
  
  QPalette pal(ui.label_extracting->palette());
  KColorScheme kcs(QPalette::Active);
  if (successful) {
    QListWidgetItem *item = new QListWidgetItem(KIcon("dialog-ok-apply"), i18n("All jobs successfully done."));
    ui.klistwidget->addItem(item);
    ui.klistwidget->scrollToItem(item);
    pal.setBrush(QPalette::Text, kcs.foreground(KColorScheme::PositiveText));
    ui.label_extracting->setText("<font style=\"font-weight:bold;\">"+i18n("Finished!")+"</font>");
    ui.label_encoding->setText("<font style=\"font-weight:bold;\">"+i18n("Finished!")+"</font>");
    ui.label_overall_track->setText("<font style=\"font-weight:bold;\">"+i18n("Finished!")+"</font>");
    ui.progressBar_extracting->setValue(100);
    ui.progressBar_encoding->setValue(100);
    ui.progressBar_overall->setValue(100);
  } else {
    QListWidgetItem *item = new QListWidgetItem(KIcon("dialog-cancel"), i18n("At least one job failed."));
    pal.setBrush(QPalette::Text, kcs.foreground(KColorScheme::NegativeText));
    ui.klistwidget->addItem(item);
    ui.klistwidget->scrollToItem(item);
    ui.label_extracting->setText("<font style=\"color:red;font-weight:bold;\">"+i18n("Failed!")+"</font>");
    ui.label_encoding->setText("<font style=\"color:red;font-weight:bold;\">"+i18n("Failed!")+"</font>");
    ui.label_overall_track->setText("<font style=\"color:red;font-weight:bold;\">"+i18n("Failed!")+"</font>");
    if (audex->encoderProtocol().count() > 0) { buttons |= User1; }
    if (audex->extractProtocol().count() > 0) { buttons |= User2; }
  }
  
  ui.progressBar_extracting->setEnabled(FALSE);
  ui.progressBar_encoding->setEnabled(FALSE);
  ui.progressBar_overall->setEnabled(FALSE);
  ui.label_speed_extracting->setEnabled(FALSE);
  ui.label_speed_encoding->setEnabled(FALSE);
  ui.label_overall->setEnabled(FALSE);
  
  ui.label_extracting->setPalette(pal);
  ui.label_encoding->setPalette(pal);
  setButtons(buttons);
  if (audex->encoderProtocol().count() > 0) {
    setButtonText(User1, i18n("Show encoding log..."));
    setButtonIcon(User1, KIcon("media-optical-audio"));
  }
  if (audex->extractProtocol().count() > 0) {
    setButtonText(User2, i18n("Show rip log..."));
    setButtonIcon(User2, KIcon("media-optical"));
  }

}

void ExtractingProgressDialog::show_info(const QString& message) {

  QListWidgetItem *item = new QListWidgetItem(KIcon("dialog-information"), message);
  ui.klistwidget->addItem(item);
  ui.klistwidget->scrollToItem(item);

}

void ExtractingProgressDialog::show_warning(const QString& message) {
  QListWidgetItem *item = new QListWidgetItem(KIcon("dialog-warning"), message);
  ui.klistwidget->addItem(item);
  ui.klistwidget->scrollToItem(item);

}

void ExtractingProgressDialog::show_error(const QString& message, const QString& details) {

  QListWidgetItem *item;
  if (details.isEmpty()) {
    item = new QListWidgetItem(KIcon("dialog-error"), QString("%1").arg(message));
  } else {
    item = new QListWidgetItem(KIcon("dialog-error"), QString("%1 (%2)").arg(message).arg(details));
  }
  ui.klistwidget->addItem(item);
  ui.klistwidget->scrollToItem(item);

}

void ExtractingProgressDialog::ask_timeout() {

  if (KMessageBox::questionYesNo(this, i18n("Ripping speed was extremly slow for the last 5 minutes.\nDue to extraction quality, audex is configured to never skip any detected error. "
				"If your disc is really broken extraction may never end!\nIn some cases, it might be that only this drive has difficulty ripping audio data from this disc. Maybe try another one.\n\n"
				"However, do you want to continue extraction?"),
				i18n("Cancel extraction"),
				KStandardGuiItem::yes(),
				KStandardGuiItem::no()) == KMessageBox::No) {
    audex->cancel();
  }

}

void ExtractingProgressDialog::open_encoder_protocol_dialog() {
  ProtocolDialog *protocolDialog = new ProtocolDialog(audex->encoderProtocol(), i18n("Encoding protocol"), this);
  protocolDialog->exec();
  delete protocolDialog;
}

void ExtractingProgressDialog::open_extract_protocol_dialog() {
  ProtocolDialog *protocolDialog = new ProtocolDialog(audex->extractProtocol(), i18n("Ripping protocol"), this);
  protocolDialog->exec();
  delete protocolDialog;
}

void ExtractingProgressDialog::update_unity() {
    QList<QVariant> args;
    int progress = ui.progressBar_overall->value();
    bool show_progress = progress>-1 && progress < 100 && !finished;
    QMap<QString, QVariant> props;
    props["count-visible"]=current_track>0 && !finished;
    props["count"]=current_track;
    props["progress-visible"]=show_progress;
    props["progress"]=show_progress ? (double)(progress/100.0) : 0.0;
    args.append("application://audex.desktop");
    args.append(props);
    unity_message.setArguments(args);
    QDBusConnection::sessionBus().send(unity_message);
}
