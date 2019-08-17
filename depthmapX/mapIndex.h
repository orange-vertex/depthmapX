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

#pragma once

#include <QTreeWidget>

QT_BEGIN_NAMESPACE

class QEvent;
class QTreeWidgetItem;

class MapIndex : public QTreeWidget {
    Q_OBJECT
  private:
    QWidget *m_mainWindow;

    class ItemTreeEntry {
      public:
        ItemTreeEntry() {
            m_type = -1;
            m_cat = -1;
            m_subcat = -1;
        }
        ItemTreeEntry(char t, short c, short sc) {
            m_type = t;
            m_cat = c;
            m_subcat = sc;
        }
        char m_type;
        short m_cat;
        short m_subcat;
    };

    enum Column { MAP = 0, EDITABLE = 1 };

    std::map<QTreeWidgetItem *, ItemTreeEntry> m_treegraphmap;
    std::map<QTreeWidgetItem *, ItemTreeEntry> m_treedrawingmap;
    QTreeWidgetItem *m_topgraph;
    QTreeWidgetItem *m_backgraph;
    QTreeWidgetItem *m_treeroots[5];

    QAction *renameMapAct;

  public:
    MapIndex(QWidget *parent = 0);

    QString m_mapColumn = "Map";
    QString m_editableColumn = "Editable";

    void setItemVisibility(QTreeWidgetItem *item, Qt::CheckState checkState) {
        item->setCheckState(Column::MAP, checkState);
    }
    void setItemEditability(QTreeWidgetItem *item, Qt::CheckState checkState) {
        item->setCheckState(Column::EDITABLE, checkState);
    }
    void setItemReadOnly(QTreeWidgetItem *item) { item->setData(Column::EDITABLE, Qt::CheckStateRole, QVariant()); }
    bool isItemSetVisible(QTreeWidgetItem *item) { return item->checkState(Column::MAP); }
    bool isItemSetEditable(QTreeWidgetItem *item) { return item->checkState(Column::EDITABLE); }
    bool isMapColumn(int col) { return col == Column::MAP; }
    bool isEditableColumn(int col) { return col == Column::EDITABLE; }

    void makeTree();
    void makeGraphTree();
    void makeDrawingTree();
    void clearGraphTree();
    void setDrawingTreeChecks();
    void setGraphTreeChecks();
    void clearTopGraph() { m_topgraph = NULL; }
    void clearBackGraph() { m_backgraph = NULL; }

  signals:
    void requestShowLink(const QUrl &url);

  private:
    QStringList columnNames = (QStringList() << m_mapColumn << m_editableColumn);
  private slots:
    void removeAllItem(QTreeWidgetItem *start);
    QTreeWidgetItem *addNewItem(const QString &title, QTreeWidgetItem *parent = NULL);
    void onSelchangingTree(QTreeWidgetItem *item, int col);
    void onRenameMap();
    void showContextMenu(const QPoint &point);
};

QT_END_NAMESPACE
