// Copyright (C) 2020 Petros Koutsolampros

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

#include "vgaparallelmainwindow.h"

#include "modules/vgaparallel/core/vgaangularopenmp.h"
#include "modules/vgaparallel/core/vgametricopenmp.h"
#include "modules/vgaparallel/core/vgavisualglobalopenmp.h"
#include "modules/vgaparallel/core/vgavisuallocaladjmatrix.h"
#include "modules/vgaparallel/core/vgavisuallocalopenmp.h"
#include "vgaparalleloptionsdlg.h"

#include "depthmapX/mainwindowhelpers.h"

#include <QMenuBar>
#include <QMessageBox>

bool VGAParallelMainWindow::createMenus(MainWindow *mainWindow) {
    QMenu *toolsMenu = MainWindowHelpers::getOrAddRootMenu(mainWindow, tr("&Tools"));
    QMenu *visibilityMenu = MainWindowHelpers::getOrAddMenu(toolsMenu, tr("&Visibility"));

    QAction *vgaParallelAct = new QAction(tr("Parallel Visibility Graph Analysis..."), mainWindow);
    connect(vgaParallelAct, &QAction::triggered, this, [this, mainWindow] { OnVGAParallel(mainWindow); });
    visibilityMenu->addAction(vgaParallelAct);

    return true;
}

void VGAParallelMainWindow::OnVGAParallel(MainWindow *mainWindow) {
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    if (graphDoc == nullptr)
        return;

    if (graphDoc->m_communicator) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please wait, another process is running"), QMessageBox::Ok,
                             QMessageBox::Ok);
        return;
    }
    if (graphDoc->m_meta_graph->getDisplayedMapType() != ShapeMap::POINTMAP) {
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please make sure the displayed map is a vga map"),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    VGAParallelOptionsDlg dlg;

    dlg.m_layer_names.push_back("<None>");
    for (auto &dataMap : graphDoc->m_meta_graph->getDataMaps()) {
        dlg.m_layer_names.push_back(dataMap.getName());
    }

    if (QDialog::Accepted != dlg.exec()) {
        return;
    }

    graphDoc->m_communicator = new CMSCommunicator();

    switch (dlg.m_output_type) {
    case VGAParallelOptionsDlg::AnalysisType::ISOVIST_OPENMP:
        //        analysisCompleted = VGAIsovist().run(communicator, getDisplayedPointMap(), simple_version);
        break;
    case VGAParallelOptionsDlg::AnalysisType::VISUAL_GLOBAL_OPENMP:
        graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(new VGAVisualGlobalOpenMP(
            graphDoc->m_meta_graph->getDisplayedPointMap(), dlg.m_radius, dlg.m_gates_only)));
        break;
    case VGAParallelOptionsDlg::AnalysisType::VISUAL_LOCAL_OPENMP:
        graphDoc->m_communicator->setAnalysis(
            std::unique_ptr<IAnalysis>(new VGAVisualLocalOpenMP(graphDoc->m_meta_graph->getDisplayedPointMap())));
        break;
    case VGAParallelOptionsDlg::AnalysisType::VISUAL_LOCAL_ADJMATRIX:
        graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
            new VGAVisualLocalAdjMatrix(graphDoc->m_meta_graph->getDisplayedPointMap(), dlg.m_gates_only)));
        break;
    case VGAParallelOptionsDlg::AnalysisType::METRIC_OPENMP:
        graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
            new VGAMetricOpenMP(graphDoc->m_meta_graph->getDisplayedPointMap(), dlg.m_radius, dlg.m_gates_only)));
        break;
    case VGAParallelOptionsDlg::AnalysisType::ANGULAR_OPENMP:
        graphDoc->m_communicator->setAnalysis(std::unique_ptr<IAnalysis>(
            new VGAAngularOpenMP(graphDoc->m_meta_graph->getDisplayedPointMap(), dlg.m_radius, dlg.m_gates_only)));
        break;
    case VGAParallelOptionsDlg::AnalysisType::THROUGH_VISION_OPENMP:
        //        analysisCompleted = VGAThroughVision().run(communicator, getDisplayedPointMap(), simple_version);
        break;
    case VGAParallelOptionsDlg::AnalysisType::NONE:
        QMessageBox::warning(mainWindow, tr("Warning"), tr("Please select an analysis type"), QMessageBox::Ok,
                             QMessageBox::Ok);
        return;
    }

    graphDoc->m_communicator->SetFunction(CMSCommunicator::FROMCONNECTOR);
    graphDoc->m_communicator->setSuccessUpdateFlags(QGraphDoc::NEW_DATA);
    graphDoc->m_communicator->setSuccessRedrawFlags(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA);

    graphDoc->CreateWaitDialog(tr("Calculating shortest path..."));
    graphDoc->m_thread.render(graphDoc);
}
