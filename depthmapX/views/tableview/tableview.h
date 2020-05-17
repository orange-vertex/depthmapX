// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2019, Petros Koutsolampros

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

#include "GraphDoc.h"
#include "settings.h"
#include <QTableWidget>

class QEvent;
class QTableWidgetItem;

class TableView : public QTableWidget {
    Q_OBJECT

  public:
    TableView(Settings &settings, QWidget *parent = 0, QGraphDoc *p = 0);
    QSize sizeHint() const;

    int m_column_count;
    int m_row_count;
    int m_from;
    int m_curr_row;
    QWidget *m_mainWindow;
    QGraphDoc *pDoc;
    bool m_protect_edit;
    void RedoTable();

  private slots:
    void itemChanged(QTableWidgetItem *item);
    void itemEditChanged(QTableWidgetItem *);
    void colum_Sort(int sort);

  protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void scrollContentsBy(int dx, int dy);
    void focusInEvent(QFocusEvent* e);

  private:
    void PrepareCache(int to);

    bool m_custom;
    QSize m_initialSize;
    bool m_updating = false;
};
