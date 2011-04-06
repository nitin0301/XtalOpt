/**********************************************************************
  PbsConfigDialog -- Setup for remote PBS queues

  Copyright (C) 2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

// Doxygen skip:
/// @cond

#include <globalsearch/queueinterfaces/pbsdialog.h>

#include <globalsearch/queueinterfaces/pbs.h>

#include <globalsearch/ui/abstractdialog.h>
#include <globalsearch/optbase.h>

#include "ui_pbsdialog.h"

namespace GlobalSearch {

  PbsConfigDialog::PbsConfigDialog(AbstractDialog *parent,
                                   OptBase *o,
                                   PbsQueueInterface *p)
    : QDialog(parent),
      ui(new Ui::PbsConfigDialog),
      m_opt(o),
      m_pbs(p)
  {
    ui->setupUi(this);
  }

  PbsConfigDialog::~PbsConfigDialog()
  {
    delete ui;
  }

  void PbsConfigDialog::updateGUI()
  {
    ui->edit_description->blockSignals(true);
    ui->edit_host->blockSignals(true);
    ui->edit_qdel->blockSignals(true);
    ui->edit_qstat->blockSignals(true);
    ui->edit_qsub->blockSignals(true);
    ui->edit_rempath->blockSignals(true);
    ui->edit_locpath->blockSignals(true);
    ui->edit_username->blockSignals(true);
    ui->spin_port->blockSignals(true);

    ui->edit_description->setText(m_opt->description);
    ui->edit_host->setText(m_opt->host);
    ui->edit_qdel->setText(m_pbs->m_qdel);
    ui->edit_qstat->setText(m_pbs->m_qstat);
    ui->edit_qsub->setText(m_pbs->m_qsub);
    ui->edit_rempath->setText(m_opt->rempath);
    ui->edit_locpath->setText(m_opt->filePath);
    ui->edit_username->setText(m_opt->username);
    ui->spin_port->setValue(m_opt->port);

    ui->edit_description->blockSignals(false);
    ui->edit_host->blockSignals(false);
    ui->edit_qdel->blockSignals(false);
    ui->edit_qstat->blockSignals(false);
    ui->edit_qsub->blockSignals(false);
    ui->edit_rempath->blockSignals(false);
    ui->edit_locpath->blockSignals(false);
    ui->edit_username->blockSignals(false);
    ui->spin_port->blockSignals(false);
  }

  void PbsConfigDialog::accept()
  {
    m_opt->description = ui->edit_description->text();
    m_opt->host = ui->edit_host->text();
    m_pbs->m_qdel = ui->edit_qdel->text();
    m_pbs->m_qstat = ui->edit_qstat->text();
    m_pbs->m_qsub = ui->edit_qsub->text();
    m_opt->rempath = ui->edit_rempath->text();
    m_opt->filePath = ui->edit_locpath->text();
    m_opt->username = ui->edit_username->text();
    m_opt->port = ui->spin_port->value();
    QDialog::accepted();
    close();
  }

  void PbsConfigDialog::reject()
  {
    updateGUI();
    QDialog::reject();
    close();
  }

}

/// @endcond