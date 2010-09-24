/**********************************************************************
  OptBase - Base class for global search extensions

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/optbase.h>

#include <globalsearch/structure.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/sshmanager.h>
#include <globalsearch/macros.h>
#include <globalsearch/ui/abstractdialog.h>
#include <globalsearch/bt.h>

#include <QtCore/QFile>
#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>

using namespace OpenBabel;

namespace GlobalSearch {

  OptBase::OptBase(AbstractDialog *parent) :
    QObject(parent),
    m_dialog(parent),
    m_tracker(new Tracker (this)),
    m_queue(new QueueManager(this, m_tracker)),
    m_optimizer(0), // This will be set when the GUI is initialized
    m_ssh(new SSHManager (5, this)),
    m_idString("Generic"),
    sOBMutex(new QMutex),
    stateFileMutex(new QMutex),
    backTraceMutex(new QMutex),
    savePending(false),
    saveOnExit(true),
    readOnly(false),
    testingMode(false),
    test_nRunsStart(1),
    test_nRunsEnd(100),
    test_nStructs(600),
    cutoff(-1)
  {
    // Connections
    connect(this, SIGNAL(startingSession()),
            this, SLOT(setIsStartingTrue()));
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(setIsStartingFalse()));
    connect(this, SIGNAL(needBoolean(const QString&, bool*)),
            this, SLOT(promptForBoolean(const QString&, bool*)),
            Qt::BlockingQueuedConnection); // Wait until slot returns
    connect(this, SIGNAL(needPassword(const QString&, QString*, bool*)),
            this, SLOT(promptForPassword(const QString&, QString*, bool*)),
            Qt::BlockingQueuedConnection); // Wait until slot returns

    INIT_RANDOM_GENERATOR();
  }

  OptBase::~OptBase() {
    // Wait for save to finish
    if (saveOnExit) {
      while (savePending) {
        qDebug() << "Spinning on save before destroying OptBase...";
#ifdef WIN32
        _sleep(1000);
#else
        sleep(1);
#endif // _WIN32
      };
      savePending = true;
    }
    delete m_queue;
    delete m_tracker;
  }

  void OptBase::reset() {
    m_tracker->deleteAllStructures();
    m_tracker->reset();
    m_queue->reset();
  }

  void OptBase::printBackTrace() {
    backTraceMutex->lock();
    QStringList l = getBackTrace();
    backTraceMutex->unlock();
    for (int i = 0; i < l.size();i++)
      qDebug() << l.at(i);
  }

  bool OptBase::save(const QString &stateFilename, bool notify)
  {
    if (isStarting ||
        readOnly) {
      savePending = false;
      return false;
    }
    QReadLocker trackerLocker (m_tracker->rwLock());
    QMutexLocker locker (stateFileMutex);
    QString filename;
    if (stateFilename.isEmpty()) {
      filename = filePath + "/" + m_idString.toLower() + ".state";
    }
    else {
      filename = stateFilename;
    }
    QString oldfilename = filename + ".old";

    if (notify) {
      m_dialog->startProgressUpdate(tr("Saving: Writing %1...")
                                    .arg(filename),
                                    0, 0);
    }

    // Copy .state -> .state.old
    if (QFile::exists(filename) ) {
      if (QFile::exists(oldfilename)) {
        QFile::remove(oldfilename);
      }
      QFile::copy(filename, oldfilename);
    }

    SETTINGS(filename);
    const int VERSION = 1;
    settings->beginGroup(m_idString.toLower());
    settings->setValue("version",          VERSION);
    settings->setValue("saveSuccessful", false);
    settings->endGroup();

    // Write/update .state
    m_dialog->writeSettings(filename);

    // Loop over structures and save them
    QList<Structure*> *structures = m_tracker->list();

    QString structureStateFileName;

    Structure* structure;
    for (int i = 0; i < structures->size(); i++) {
      structure = structures->at(i);
      structure->lock()->lockForRead();
      // Set index here -- this is the only time these are written, so
      // this is "ok" under a read lock because of the savePending logic
      structure->setIndex(i);
      structureStateFileName = structure->fileName() + "/structure.state";
      if (notify) {
        m_dialog->updateProgressLabel(tr("Saving: Writing %1...")
                                      .arg(structureStateFileName));
      }
      structure->writeSettings(structureStateFileName);
      structure->lock()->unlock();
    }

    /////////////////////////
    // Print results files //
    /////////////////////////

    QFile file (filePath + "/results.txt");
    QFile oldfile (filePath + "/results_old.txt");
    if (notify) {
      m_dialog->updateProgressLabel(tr("Saving: Writing %1...")
                                    .arg(file.fileName()));
    }
    if (oldfile.open(QIODevice::ReadOnly))
      oldfile.remove();
    if (file.open(QIODevice::ReadOnly))
      file.copy(oldfile.fileName());
    file.close();
    if (!file.open(QIODevice::WriteOnly)) {
      error("OptBase::save(): Error opening file "+file.fileName()+" for writing...");
      savePending = false;
      return false;
    }
    QTextStream out (&file);

    QList<Structure*> sortedStructures;

    for (int i = 0; i < structures->size(); i++)
      sortedStructures.append(structures->at(i));
    if (sortedStructures.size() != 0) {
      Structure::sortAndRankByEnthalpy(&sortedStructures);
      out << sortedStructures.first()->getResultsHeader() << endl;
    }

    for (int i = 0; i < sortedStructures.size(); i++) {
      structure = sortedStructures.at(i);
      if (!structure) continue; // In case there was a problem copying.
      structure->lock()->lockForRead();
      out << structure->getResultsEntry() << endl;
      structure->lock()->unlock();
      if (notify) {
        m_dialog->stopProgressUpdate();
      }
    }

    // Mark operation successful
    settings->setValue(m_idString.toLower() + "/saveSuccessful", true);
    DESTROY_SETTINGS(filename);

    savePending = false;
    return true;
  }

  QString OptBase::interpretTemplate(const QString & str, Structure* structure)
  {
    QStringList list = str.split("%");
    QString line;
    QString origLine;
    for (int line_ind = 0; line_ind < list.size(); line_ind++) {
      origLine = line = list.at(line_ind);
      interpretKeyword_base(line, structure);
      // Add other interpret keyword sections here if needed
      if (line != origLine) { // Line was a keyword
        list.replace(line_ind, line);
      }
    }
    // Rejoin string
    QString ret = list.join("");
    ret = ret.remove("%") + "\n";
    return ret;
  }

  void OptBase::interpretKeyword_base(QString &line, Structure* structure)
  {
    QString rep = "";
    // User data
    if (line == "user1")    		rep += optimizer()->getUser1();
    else if (line == "user2")    	rep += optimizer()->getUser2();
    else if (line == "user3")    	rep += optimizer()->getUser3();
    else if (line == "user4")    	rep += optimizer()->getUser4();
    else if (line == "description")	rep += description;

    // Structure specific data
    if (line == "coords") {
      OpenBabel::OBMol obmol = structure->OBMol();
      FOR_ATOMS_OF_MOL(atom, obmol) {
        rep += static_cast<QString>(OpenBabel::etab.GetSymbol(atom->GetAtomicNum())) + " ";
        rep += QString::number(atom->GetX()) + " ";
        rep += QString::number(atom->GetY()) + " ";
        rep += QString::number(atom->GetZ()) + "\n";
      }
    }
    else if (line == "coordsId") {
      OpenBabel::OBMol obmol = structure->OBMol();
      FOR_ATOMS_OF_MOL(atom, obmol) {
        rep += static_cast<QString>(OpenBabel::etab.GetSymbol(atom->GetAtomicNum())) + " ";
        rep += QString::number(atom->GetAtomicNum()) + " ";
        rep += QString::number(atom->GetX()) + " ";
        rep += QString::number(atom->GetY()) + " ";
        rep += QString::number(atom->GetZ()) + "\n";
      }
    }
    else if (line == "numAtoms")	rep += QString::number(structure->numAtoms());
    else if (line == "numSpecies")	rep += QString::number(structure->getSymbols().size());
    else if (line == "filename")	rep += structure->fileName();
    else if (line == "rempath")       	rep += structure->getRempath();
    else if (line == "gen")           	rep += QString::number(structure->getGeneration());
    else if (line == "id")            	rep += QString::number(structure->getIDNumber());
    else if (line == "incar")         	rep += QString::number(structure->getCurrentOptStep());
    else if (line == "optStep")       	rep += QString::number(structure->getCurrentOptStep());

    if (!rep.isEmpty()) {
      // Remove any trailing newlines
      rep = rep.replace(QRegExp("\n$"), "");
      line = rep;
    }
  }

  QString OptBase::getTemplateKeywordHelp_base()
  {
    QString str;
    QTextStream out (&str);
    out
      << "The following keywords should be used instead of the indicated variable data:\n"
      << "\n"
      << "User data:\n"
      << "%userX% -- User specified value, where X = 1, 2, 3, or 4\n"
      << "%description% -- Optimization description\n"
      << "\n"
      << "Generic structure data:\n"
      << "%coords% -- cartesian coordinate data\n\t[symbol] [x] [y] [z]\n"
      << "%coordsId% -- cartesian coordinate data with atomic number\n\t[symbol] [atomic number] [x] [y] [z]\n"
      << "%numAtoms% -- Number of atoms in unit cell\n"
      << "%numSpecies% -- Number of unique atomic species in unit cell\n"
      << "%filename% -- local output filename\n"
      << "%rempath% -- path to structure's remote directory\n"
      << "%gen% -- structure generation number (if relevant)\n"
      << "%id% -- structure id number\n"
      << "%optStep% -- current optimization step\n";
    return str;
  }

  void OptBase::setOptimizer_opt(Optimizer *o) {
    Optimizer *old = m_optimizer;
    if (m_optimizer) {
      old->deleteLater();
    }
    m_optimizer = o;
    emit optimizerChanged(o);
  }

  void OptBase::promptForPassword(const QString &message,
                                  QString *newPassword,
                                  bool *ok)
  {
    (*newPassword) = QInputDialog::getText(dialog(), "Need password:", message,
                                           QLineEdit::Password, QString(), ok);
  };

  void OptBase::promptForBoolean(const QString &message,
                                 bool *ok)
  {
    if (QMessageBox::question(dialog(), m_idString, message,
                              QMessageBox::Yes | QMessageBox::No)
        == QMessageBox::Yes) {
      *ok = true;
    } else {
      *ok = false;
    }
  }

  void OptBase::warning(const QString & s) {
    qWarning() << "Warning: " << s;
    emit warningStatement(s);
  }

  void OptBase::debug(const QString & s) {
    qDebug() << "Debug: " << s;
    emit debugStatement(s);
  }

  void OptBase::error(const QString & s) {
    qWarning() << "Error: " << s;
    emit errorStatement(s);
  }

} // end namespace Avogadro

//#include "optbase.moc"