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

#include "ui_vgaparalleloptionsdlg.h"
#include <salalib/axialmap.h>
#include <salalib/mgraph.h>
#include <salalib/shapemap.h>

class VGAParallelOptionsDlg : public QDialog, public Ui::VGAParallelOptionsDlg {
    Q_OBJECT
  public:
    enum class AnalysisType {
        NONE,
        ISOVIST_OPENMP,
        VISUAL_LOCAL_OPENMP,
        VISUAL_LOCAL_ADJMATRIX,
        VISUAL_GLOBAL_OPENMP,
        METRIC_OPENMP,
        ANGULAR_OPENMP,
        THROUGH_VISION_OPENMP
    };
    VGAParallelOptionsDlg(QWidget *parent = 0);
    QString m_visualRadiusString;
    QString m_metricRadiusString;
    double m_radius;
    int m_gatelayer;
    AnalysisType m_output_type;
    void UpdateData(bool value);
    void showEvent(QShowEvent *event);

    std::vector<std::string> m_layer_names;

  private slots:
    void OnUpdateVisualRadius(QString);
    void OnUpdateMetricRadius(QString);
    void OnOK();
    void OnCancel();
};