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
    m_visualRadiusString = QString("n");
    m_metricRadiusString = QString("n");
    m_radius = -1;
    m_output_type = AnalysisType::NONE;
    m_gatelayer = -1;
}

void VGAParallelOptionsDlg::OnUpdateVisualRadius(QString text) {
    if (text.length()) {
        if (!text.toInt() && text != tr("n")) {
            QMessageBox::warning(this, tr("Warning"), tr("The radius must either be n or number in range 1-99"),
                                 QMessageBox::Ok, QMessageBox::Ok);
            c_visualRadius->setText(tr("n"));
        }
    }
}

void VGAParallelOptionsDlg::OnUpdateMetricRadius(QString text) {
    if (text.length()) {
        if (text.toDouble() == 0.0 && text != tr("n")) {
            QMessageBox::warning(this, tr("Warning"), tr("The metric radius must either be n or a positive number"),
                                 QMessageBox::Ok, QMessageBox::Ok);
            c_metricRadius->setText(tr("n"));
        }
    }
}

void VGAParallelOptionsDlg::OnOK() {
    UpdateData(true);

    if (m_output_type == AnalysisType::VISUAL_GLOBAL_OPENMP) {
        if (m_visualRadiusString.compare(tr("n")) == 0) { // 0 means identical
            m_radius = -1.0;
        } else {
            m_radius = (double)m_visualRadiusString.toInt();
            if (m_radius <= 0.0) {
                QMessageBox::warning(this, tr("Warning"),
                                     tr("The radius must either be n or a number in the range 1-99"), QMessageBox::Ok,
                                     QMessageBox::Ok);
                return;
            }
        }
    } else {
        if (m_metricRadiusString.compare(tr("n")) == 0) { // 0 means identical
            m_radius = -1.0;
        } else {
            m_radius = m_metricRadiusString.toDouble();
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
        m_visualRadiusString = c_visualRadius->text();

        if (c_calcIsovistProperties->isChecked())
            m_output_type = AnalysisType::ISOVIST_OPENMP;
        else if (c_calcVisualGlobal->isChecked())
            m_output_type = AnalysisType::VISUAL_GLOBAL_OPENMP;
        else if (c_calcVisualLocalOpenMP->isChecked())
            m_output_type = AnalysisType::VISUAL_LOCAL_OPENMP;
        else if (c_calcVisualLocalAdjMatrix->isChecked())
            m_output_type = AnalysisType::VISUAL_LOCAL_ADJMATRIX;
        else if (c_calcMetricGlobal->isChecked())
            m_output_type = AnalysisType::METRIC_OPENMP;
        else if (c_calcAngularGlobal->isChecked())
            m_output_type = AnalysisType::ANGULAR_OPENMP;
        else if (c_calcThroughVision->isChecked())
            m_output_type = AnalysisType::THROUGH_VISION_OPENMP;
        else
            m_output_type = AnalysisType::NONE;
        m_metricRadiusString = c_metricRadius->text();
    } else {
        c_visualRadius->setText(m_visualRadiusString);

        switch (m_output_type) {
        case AnalysisType::ISOVIST_OPENMP:
            c_calcIsovistProperties->setChecked(true);
            break;
        case AnalysisType::VISUAL_GLOBAL_OPENMP:
            c_calcVisualGlobal->setChecked(true);
            break;
        case AnalysisType::VISUAL_LOCAL_OPENMP:
            c_calcVisualLocalOpenMP->setChecked(true);
            break;
        case AnalysisType::VISUAL_LOCAL_ADJMATRIX:
            c_calcVisualLocalAdjMatrix->setChecked(true);
            break;
        case AnalysisType::METRIC_OPENMP:
            c_calcMetricGlobal->setChecked(true);
            break;
        case AnalysisType::ANGULAR_OPENMP:
            c_calcAngularGlobal->setChecked(true);
            break;
        case AnalysisType::THROUGH_VISION_OPENMP:
            c_calcThroughVision->setChecked(true);
            break;
        default:
            break;
        }
        c_metricRadius->setText(m_metricRadiusString);
    }
}

void VGAParallelOptionsDlg::showEvent(QShowEvent *event) {
    UpdateData(false);
}
