/**********************************************************************
  AbstractDialog -- A base dialog class for use with libglobalsearch
  projects. See the accompanying .ui file for a Qt Designer template.

  Copyright (C) 2010 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/ui/abstractdialog.h>

#include <globalsearch/tracker.h>
#include <globalsearch/queuemanager.h>

#include <openbabel/oberror.h>

#include <QtGui/QLabel>
#include <QtGui/QFileDialog>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <QtCore/QSettings>
#include <QtCore/QtConcurrentRun>

using namespace std;

namespace GlobalSearch {

  AbstractDialog::AbstractDialog( GLWidget *glWidget,
                                  QWidget *parent,
                                  Qt::WindowFlags f ) :
    QDialog( parent, f ),
    m_opt(0),
    m_glWidget(glWidget)
  {
    // Turn off OB error logging. This helps prevent threading
    // problems, as the error log is implemented as a singleton.
    OpenBabel::obErrorLog.StopLogging();

    // Initialize vars, connections, etc
    progMutex = new QMutex;
    progTimer = new QTimer;
  }

  void AbstractDialog::initialize()
  {
    // Connections
    connect(ui_push_begin, SIGNAL(clicked()),
            this, SLOT(startSearch()));
    connect(ui_push_save, SIGNAL(clicked()),
            this, SLOT(saveSession()));
    connect(ui_push_resume, SIGNAL(clicked()),
            this, SLOT(resumeSession()));

    connect(m_opt->tracker(), SIGNAL(newStructureAdded(Structure*)),
            this, SLOT(saveSession()));
    connect(m_opt, SIGNAL(newInfoUpdate()),
            this, SLOT(saveSession()));
    connect(m_opt, SIGNAL(sessionStarted()),
            this, SLOT(updateGUI()));
    connect(m_opt, SIGNAL(sessionStarted()),
            this, SLOT(lockGUI()));
    connect(m_opt->queue(), SIGNAL(newStatusOverview(int,int,int)),
            this, SLOT(updateStatus(int,int,int)));
    connect(this, SIGNAL(sig_updateStatus(int,int,int)),
            this, SLOT(updateStatus_(int,int,int)));

    connect(progTimer, SIGNAL(timeout()),
            this, SLOT(repaintProgressBar_()));
    connect(this, SIGNAL(sig_startProgressUpdate(const QString &, int, int)),
            this, SLOT(startProgressUpdate_(const QString &, int, int)));
    connect(this, SIGNAL(sig_stopProgressUpdate()),
            this, SLOT(stopProgressUpdate_()));
    connect(this, SIGNAL(sig_updateProgressMinimum(int)),
            this, SLOT(updateProgressMinimum_(int)));
    connect(this, SIGNAL(sig_updateProgressMaximum(int)),
            this, SLOT(updateProgressMaximum_(int)));
    connect(this, SIGNAL(sig_updateProgressValue(int)),
            this, SLOT(updateProgressValue_(int)));
    connect(this, SIGNAL(sig_updateProgressLabel(const QString &)),
            this, SLOT(updateProgressLabel_(const QString &)));
    connect(this, SIGNAL(sig_repaintProgressBar()),
            this, SLOT(repaintProgressBar_()));

    connect(m_opt, SIGNAL(warningStatement(const QString &)),
            this, SLOT(newWarning(const QString &)));
    connect(m_opt, SIGNAL(debugStatement(const QString &)),
            this, SLOT(newDebug(const QString &)));
    connect(m_opt, SIGNAL(errorStatement(const QString &)),
            this, SLOT(newError(const QString &)));
    connect(this, SIGNAL(sig_errorBox(const QString &)),
            this, SLOT(errorBox_(const QString &)));

    // Select the first tab by default
    ui_tabs->setCurrentIndex(0);

    // Hide the progress bar/label
    ui_label_prog->setVisible(false);
    ui_progbar->setVisible(false);
    readSettings();

  }

  AbstractDialog::~AbstractDialog()
  {
    delete m_opt;
  }

  void AbstractDialog::disconnectGUI() {
    emit tabsDisconnectGUI();
    disconnect(m_opt, SIGNAL(sessionStarted()),
               this, SLOT(updateGUI()));
    disconnect(this, SIGNAL(sig_updateStatus(int,int,int)),
               this, SLOT(updateStatus_(int,int,int)));
  }

  void AbstractDialog::lockGUI() {
    ui_push_resume->setDisabled(true);
    ui_push_begin->setDisabled(true);
    emit tabsLockGUI();
  }

  void AbstractDialog::updateGUI() {
    setWindowTitle(QString("%1 - %2 @ %3%4")
                   .arg(m_opt->getIDString())
                   .arg(m_opt->description)
                   .arg(m_opt->host)
                   .arg( m_opt->readOnly ?
                         " (Read-Only mode)" :
                         "" )
                   );
    emit tabsUpdateGUI();
  }

  void AbstractDialog::resumeSession() {
    QMutexLocker locker (m_opt->stateFileMutex);
    QString filename;
    QFileDialog dialog (NULL,
                        QString("Select .state file to resume"),
                        m_opt->filePath,
                        "*.state;;*.*");
    dialog.selectFile(tr("%1.state")
                      .arg(m_opt->getIDString().toLower()));
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec())
      filename = dialog.selectedFiles().first();
    else { // User cancel file selection.
      return;
    }
    m_opt->emitStartingSession();
    startProgressUpdate(tr("Resuming session..."), 0, 0);
    m_opt->tracker()->deleteAllStructures();
    if (!m_opt->load(filename)) {
      stopProgressUpdate();
      m_opt->isStarting = false;
      return;
    }
    // Refresh dialog and settings
    writeSettings();
    stopProgressUpdate();
    m_opt->emitSessionStarted();
  }

  void AbstractDialog::updateStatus_(int opt, int run, int fail) {
    ui_label_opt->setText(QString::number(opt));
    ui_label_run->setText(QString::number(run));
    ui_label_fail->setText(QString::number(fail));
  }

  void AbstractDialog::startProgressUpdate_(const QString & text, int min, int max) {
    progMutex->lock();
    ui_progbar->reset();
    ui_progbar->setRange(min, max);
    ui_progbar->setValue(min);
    ui_label_prog->setText(text);
    ui_progbar->setVisible(true);
    ui_label_prog->setVisible(true);
    repaintProgressBar();
    progTimer->start(1000);
  }

  void AbstractDialog::stopProgressUpdate_() {
    ui_progbar->reset();
    ui_label_prog->setText("");
    ui_progbar->setVisible(false);
    ui_label_prog->setVisible(false);
    progTimer->stop();
    progMutex->unlock();
    repaintProgressBar();
  }

  void AbstractDialog::updateProgressMinimum_(int min) {
    ui_progbar->setMinimum(min);
    repaintProgressBar();
  }

  void AbstractDialog::updateProgressMaximum_(int max) {
    ui_progbar->setMaximum(max);
    repaintProgressBar();
  }

  void AbstractDialog::updateProgressValue_(int val) {
    ui_progbar->setValue(val);
    repaintProgressBar();
  }

  void AbstractDialog::updateProgressLabel_(const QString & text) {
    ui_label_prog->setText(text);
    repaintProgressBar();
  }

  void AbstractDialog::repaintProgressBar_() {
    ui_label_prog->repaint();
    ui_progbar->repaint();
  }

}

//#include "dialog.moc"