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

#pragma once

#include "GraphDoc.h"
#include "settings.h"
#include <QOpenGLWidget>

class MapView : public QOpenGLWidget {
    Q_OBJECT

  protected:
    QGraphDoc &m_pDoc;
    Settings &m_settings;
    QString m_currentFile;

  public:
    MapView(QGraphDoc &pDoc, Settings &settings, QWidget *parent = Q_NULLPTR);

    virtual void OnModeJoin() = 0;
    virtual void OnModeUnjoin() = 0;
    virtual void OnViewPan() = 0;
    virtual void OnViewZoomIn() = 0;
    virtual void OnViewZoomOut() = 0;
    virtual void OnEditFill() = 0;
    virtual void OnEditSemiFill() = 0;
    virtual void OnEditAugmentFill() = 0;
    virtual void OnEditPencil() = 0;
    virtual void OnModeIsovist() = 0;
    virtual void OnModeTargetedIsovist() = 0;
    virtual void OnEditLineTool() = 0;
    virtual void OnEditPolygonTool() = 0;
    virtual void OnModeSeedAxial() = 0;
    virtual void OnEditSelect() = 0;
    virtual void postLoadFile() = 0;
    virtual void OnViewZoomsel() = 0;
    virtual void OnEditCopy() = 0;
    virtual void OnEditSave() = 0;
    virtual void OnViewZoomToRegion(QtRegion region) = 0;

    QGraphDoc *getGraphDoc() { return &m_pDoc; }
    void setCurrentFile(const QString &fileName) { m_currentFile = fileName; }
    QString getCurrentFile() { return m_currentFile; }
};
