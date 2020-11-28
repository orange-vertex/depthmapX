// Copyright (C) 2011-2012, Tasos Varoudis

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "vgaparalleloptionsdlg.h"
#include "depthmapX/mainwindow.h"
#include <QMessageBox>

VGAParallelOptionsDlg::VGAParallelOptionsDlg(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    m_global = false;
    m_local = false;
    m_radiusString = tr("");
    m_radius = -1;
    m_gates_only = false;
    m_output_type = AnalysisType::NONE;
    m_radiusString2 = tr("");
    m_gatelayer = -1;
}

void VGAParallelOptionsDlg::OnOutputType(bool value) {
    UpdateData(true);

    if (m_output_type == AnalysisType::VISUAL_GLOBAL_OPENMP) {
        c_local->setEnabled(true);
        c_global->setEnabled(true);
        c_radius->setEnabled(true);
    } else {
        c_local->setEnabled(false);
        c_global->setEnabled(false);
        c_radius->setEnabled(false);
        c_radius->setText(tr("n")); // <- essentially, undo changes
    }

    if (m_output_type == AnalysisType::METRIC_OPENMP) {
        c_radius2->setEnabled(true);
    } else {
        c_radius2->setText(tr("n")); // <- essentially, undo changes
        c_radius2->setEnabled(false);
    }
}

void VGAParallelOptionsDlg::OnUpdateRadius(QString text) {
    if (text.length()) {
        if (!text.toInt() && text != tr("n")) {
            QMessageBox::warning(this, tr("Warning"), tr("The radius must either be n or number in range 1-99"),
                                 QMessageBox::Ok, QMessageBox::Ok);
            c_radius->setText(tr("n"));
        }
    }
}

void VGAParallelOptionsDlg::OnUpdateRadius2(QString text) {
    if (text.length()) {
        if (text.toDouble() == 0.0 && text != tr("n")) {
            QMessageBox::warning(this, tr("Warning"), tr("The radius must either be n or a positive number"),
                                 QMessageBox::Ok, QMessageBox::Ok);
            c_radius2->setText(tr("n"));
        }
    }
}

void VGAParallelOptionsDlg::OnOK() {
    UpdateData(true);

    m_gatelayer = c_layer_selector->currentIndex() - 1;

    if (m_output_type == AnalysisType::VISUAL_GLOBAL_OPENMP) {
        if (m_radiusString.compare(tr("n")) == 0) { // 0 means identical
            m_radius = -1.0;
        } else {
            m_radius = (double)m_radiusString.toInt();
            if (m_radius <= 0.0) {
                QMessageBox::warning(this, tr("Warning"),
                                     tr("The radius must either be n or a number in the range 1-99"), QMessageBox::Ok,
                                     QMessageBox::Ok);
                return;
            }
        }
    } else {
        if (m_radiusString2.compare(tr("n")) == 0) { // 0 means identical
            m_radius = -1.0;
        } else {
            m_radius = m_radiusString2.toDouble();
            if (m_radius <= 0.0) {
                QMessageBox::warning(this, tr("Warning"), tr("The radius must either be n or a positive number"),
                                     QMessageBox::Ok, QMessageBox::Ok);
                return;
            }
        }
    }
    accept();
}

void VGAParallelOptionsDlg::OnCancel() { reject(); }

void VGAParallelOptionsDlg::UpdateData(bool value) {
    if (value) {
        if (c_global->checkState())
            m_global = true;
        else
            m_global = false;

        if (c_local->checkState())
            m_local = true;
        else
            m_local = false;

        m_radiusString = c_radius->text();

        if (c_output_type->isChecked())
            m_output_type = AnalysisType::ISOVIST_OPENMP;
        else if (c_radio1->isChecked())
            m_output_type = AnalysisType::VISUAL_GLOBAL_OPENMP;
        else if (c_radio2->isChecked())
            m_output_type = AnalysisType::METRIC_OPENMP;
        else if (c_radio3->isChecked())
            m_output_type = AnalysisType::ANGULAR_OPENMP;
        else if (c_radio4->isChecked())
            m_output_type = AnalysisType::THROUGH_VISION_OPENMP;
        else
            m_output_type = AnalysisType::NONE;
        m_radiusString2 = c_radius2->text();
    } else {
        if (m_global)
            c_global->setCheckState(Qt::Checked);
        else
            c_global->setCheckState(Qt::Unchecked);

        if (m_local)
            c_local->setCheckState(Qt::Checked);
        else
            c_local->setCheckState(Qt::Unchecked);

        c_radius->setText(m_radiusString);

        switch (m_output_type) {
        case AnalysisType::ISOVIST_OPENMP:
            c_output_type->setChecked(true);
            break;
        case AnalysisType::VISUAL_GLOBAL_OPENMP:
            c_radio1->setChecked(true);
            break;
        case AnalysisType::METRIC_OPENMP:
            c_radio2->setChecked(true);
            break;
        case AnalysisType::ANGULAR_OPENMP:
            c_radio3->setChecked(true);
            break;
        case AnalysisType::THROUGH_VISION_OPENMP:
            c_radio4->setChecked(true);
            break;
        default:
            break;
        }
        c_radius2->setText(m_radiusString2);
    }
}

void VGAParallelOptionsDlg::showEvent(QShowEvent *event) {
    for (size_t i = 0; i < m_layer_names.size(); i++) {
        c_layer_selector->addItem(QString(m_layer_names[i].c_str()));
    }
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin) {
            c_layer_selector->setCurrentIndex(m_gatelayer + 1);
            break;
        }
    }

    OnOutputType(false);

    UpdateData(false);
}
