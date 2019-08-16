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


#include <QtCore/QEvent>
#include <QtCore/QDebug>

#include <QtWidgets/QMenu>
#include <QtWidgets/QLayout>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTreeWidgetItem>
#include <QtGui/QFocusEvent>

#include "mapIndex.h"
#include "mainwindow.h"

QT_BEGIN_NAMESPACE

MapIndex::MapIndex(QWidget *parent)
    : QTreeWidget(parent)
{
    m_mainWindow = parent;

    setColumnCount(2);
    setHeaderLabels(columnNames);
    header()->setSectionResizeMode(Column::MAP, QHeaderView::Stretch);
    header()->setSectionResizeMode(Column::EDITABLE, QHeaderView::ResizeToContents);
    header()->resizeSection(Column::EDITABLE, 10);
    header()->setStretchLastSection(false);

    installEventFilter(this);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this,
        SLOT(onSelchangingTree(QTreeWidgetItem*, int)));
}

void MapIndex::removeAllItem(QTreeWidgetItem *start)
{
    int index;
    QTreeWidgetItem *currentItem = start;
    if(currentItem)
	{
        QTreeWidgetItem *parent = currentItem->parent();
        if (parent) {
            index = parent->indexOfChild(currentItem);
            delete parent->takeChild(index);
        } else {
            index = indexOfTopLevelItem(currentItem);
            delete takeTopLevelItem(index);
        }
    }
}

QTreeWidgetItem * MapIndex::addNewItem(const QString &title, QTreeWidgetItem* parent)
{
    QTreeWidgetItem *newItem = 0;

    QStringList columnStrings(title);
    if (parent != NULL) {
        newItem = new QTreeWidgetItem(parent, columnStrings);
    } else {
        newItem = new QTreeWidgetItem(this, columnStrings);
    }

    setCurrentItem(newItem);
    newItem->setFlags(newItem->flags() &~ (Qt::ItemIsEditable | Qt::ItemIsSelectable));
	return newItem;
}

void MapIndex::makeTree()
{
    m_treegraphmap.clear();
    m_treedrawingmap.clear();

    for (int i = 0; i < 5; i++) m_treeroots[i] = NULL;

    makeGraphTree();
    makeDrawingTree();
}

void MapIndex::onSelchangingTree(QTreeWidgetItem* hItem, int col)
{
    MainWindow *mainWindow = ((MainWindow *) m_mainWindow);
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    MetaGraph *graph = graphDoc->m_meta_graph;

    if(mainWindow->in_FocusGraph) return;

    bool update = false;

    // look it up in the table to see what to do:
    auto iter = m_treegraphmap.find(hItem);
    if (iter != m_treegraphmap.end()) {
        ItemTreeEntry entry = iter->second;
        bool remenu = false;
        if (entry.m_cat != -1) {
            if (entry.m_subcat == -1 && isMapColumn(col)) {
                switch (entry.m_type) {
                case 0:
                    if (graph->getViewClass() & MetaGraph::VIEWVGA) {
                        if (graph->getDisplayedPointMapRef() == entry.m_cat) {
                            graph->setViewClass(MetaGraph::SHOWHIDEVGA);
                        }
                        else {
                            graph->setDisplayedPointMapRef(entry.m_cat);
                        }
                    }
                    else {
                        graph->setDisplayedPointMapRef(entry.m_cat);
                        graph->setViewClass(MetaGraph::SHOWVGATOP);
                    }
                    remenu = true;
                    break;
                case 1:
                   if (graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                      if (graph->getDisplayedShapeGraphRef() == entry.m_cat) {
                         graph->setViewClass(MetaGraph::SHOWHIDEAXIAL);
                      }
                      else {
                         graph->setDisplayedShapeGraphRef(entry.m_cat);
                      }
                   }
                   else {
                      graph->setDisplayedShapeGraphRef(entry.m_cat);
                      graph->setViewClass(MetaGraph::SHOWAXIALTOP);
                   }
                    remenu = true;
                    break;
                case 2:
                   if (graph->getViewClass() & MetaGraph::VIEWDATA) {
                      if (graph->getDisplayedDataMapRef() == entry.m_cat) {
                         graph->setViewClass(MetaGraph::SHOWHIDESHAPE);
                      }
                      else {
                         graph->setDisplayedDataMapRef(entry.m_cat);
                      }
                   }
                   else {
                      graph->setDisplayedDataMapRef(entry.m_cat);
                      graph->setViewClass(MetaGraph::SHOWSHAPETOP);
                   }
                    remenu = true;
                    break;
                case 4:
                    // slightly different for this one
                    break;
                }
                if (remenu) {
                    setGraphTreeChecks();
                    graphDoc->SetRemenuFlag(QGraphDoc::VIEW_ALL, true);
                    mainWindow->OnFocusGraph(graphDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);
                }
                graphDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_TABLE );
            }
            else if (entry.m_subcat == -1 && isEditableColumn(col)) {
                // hit editable box
                if (entry.m_type == 1) {
                    int type = graph->getShapeGraphs()[entry.m_cat]->getMapType();
                    if (type != ShapeMap::SEGMENTMAP && type != ShapeMap::ALLLINEMAP) {
                        graph->getShapeGraphs()[entry.m_cat]->setEditable(isItemSetEditable(hItem));
                        update = true;
                    }
                }
                if (entry.m_type == 2) {
                    graph->getDataMaps()[entry.m_cat].setEditable(isItemSetEditable(hItem));
                    update = true;
                }
                if (update) {
                    // Depending on if the map is displayed you may have to redraw -- I'm just going to redraw *anyway*
                    // (it may be worth switching it to topmost when they do click here)
                    mainWindow->OnFocusGraph(graphDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);
                }
            }
            else {
                // They've clicked on the displayed layers
                if (entry.m_type == 1) {
                   update = true;
                   graph->getShapeGraphs()[entry.m_cat]->setLayerVisible(entry.m_subcat, isItemSetVisible(hItem));
                }
                else if (entry.m_type == 2) {
                   update = true;
                   graph->getDataMaps()[entry.m_cat].setLayerVisible(entry.m_subcat, isItemSetVisible(hItem));
                }
                if (update) {
                    graphDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_TABLE );
                    mainWindow->OnFocusGraph(graphDoc, QGraphDoc::CONTROLS_CHANGEATTRIBUTE);
                }
            }
        }
    }
    else {
        auto iter = m_treedrawingmap.find(hItem);
        if (iter != m_treedrawingmap.end()) {
            ItemTreeEntry entry = iter->second;
            if (entry.m_subcat != -1) {
                if (graph->getLineLayer(entry.m_cat,entry.m_subcat).isShown()) {
                    graph->getLineLayer(entry.m_cat,entry.m_subcat).setShow(false);
                    graph->redoPointMapBlockLines();
                    graph->resetBSPtree();
                }
                else {
                    graph->getLineLayer(entry.m_cat,entry.m_subcat).setShow(true);
                    graph->redoPointMapBlockLines();
                    graph->resetBSPtree();
                }
            }
            graphDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_LINESET );
        }
    }
}

void MapIndex::setGraphTreeChecks()
{
    MainWindow *mainWindow = ((MainWindow *) m_mainWindow);
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    MetaGraph *graph = graphDoc->m_meta_graph;

    mainWindow->in_FocusGraph = true;

    int viewclass = graph->getViewClass();
    for (auto item: m_treegraphmap) {
        QTreeWidgetItem* key = item.first;
        ItemTreeEntry entry = item.second;
        int checkstyle = 7;
        if (entry.m_cat != -1) {
            if (entry.m_subcat == -1) {
                // this is the main type box hit
                switch (entry.m_type) {
                    case 0:
                        if (viewclass & MetaGraph::VIEWVGA && graph->getDisplayedPointMapRef() == entry.m_cat) {
                            checkstyle = 5;
                            m_topgraph = key;
                        }
                        else if (viewclass & MetaGraph::VIEWBACKVGA && graph->getDisplayedPointMapRef() == entry.m_cat) {
                            checkstyle = 6;
                            m_backgraph = key;
                        }
                        break;
                    case 1:
                        if (viewclass & MetaGraph::VIEWAXIAL && graph->getDisplayedShapeGraphRef() == entry.m_cat) {
                            checkstyle = 5;
                            m_topgraph = key;
                        }
                        else if (viewclass & MetaGraph::VIEWBACKAXIAL && graph->getDisplayedShapeGraphRef() == entry.m_cat) {
                            checkstyle = 6;
                            m_backgraph = key;
                        }
                        break;
                    case 2:
                        if (viewclass & MetaGraph::VIEWDATA && graph->getDisplayedDataMapRef() == entry.m_cat) {
                            checkstyle = 5;
                            m_topgraph = key;
                        }
                        else if (viewclass & MetaGraph::VIEWBACKDATA && graph->getDisplayedDataMapRef() == entry.m_cat) {
                            checkstyle = 6;
                            m_backgraph = key;
                        }
                        break;
                }

                if(checkstyle == 5)
                    setItemVisibility(key, Qt::Checked);
                else if(checkstyle == 6)
                    setItemVisibility(key, Qt::PartiallyChecked);
                else if(checkstyle == 7)
                    setItemVisibility(key, Qt::Unchecked);

                // the editable box
                int editable = MetaGraph::NOT_EDITABLE;
                switch (entry.m_type) {
                    case 0:
                        if (graph->getPointMaps()[entry.m_cat].isProcessed()) {
                            editable = MetaGraph::NOT_EDITABLE;
                        }
                        else {
                            editable = MetaGraph::EDITABLE_ON;
                        }
                        break;
                    case 1:
                        {
                            int type = graph->getShapeGraphs()[entry.m_cat]->getMapType();
                            if (type == ShapeMap::SEGMENTMAP || type == ShapeMap::ALLLINEMAP) {
                                editable = MetaGraph::NOT_EDITABLE;
                            }
                            else {
                                editable = graph->getShapeGraphs()[entry.m_cat]->isEditable() ? MetaGraph::EDITABLE_ON : MetaGraph::EDITABLE_OFF;
                            }
                        }
                        break;
                    case 2:
                        editable = graph->getDataMaps()[entry.m_cat].isEditable() ? MetaGraph::EDITABLE_ON : MetaGraph::EDITABLE_OFF;
                        break;
                }
                switch (editable) {
                    case MetaGraph::NOT_EDITABLE:
                        setItemReadOnly(key);
                        break;
                    case MetaGraph::EDITABLE_OFF:
                        setItemEditability(key, Qt::Unchecked);
                        break;
                    case MetaGraph::EDITABLE_ON:
                        setItemEditability(key, Qt::Checked);
                    break;
                }
            }
            else {
                // the displayed layers (note that VGA graphs (type 0)
                // do not currently have layers supported
                bool show = false;
                if (entry.m_type == 1) {
                    show = graph->getShapeGraphs()[entry.m_cat]->isLayerVisible(entry.m_subcat);
                }
                else if (entry.m_type == 2) {
                    show = graph->getDataMaps()[entry.m_cat].isLayerVisible(entry.m_subcat);
                }
                if (show) {
                      setItemVisibility(key, Qt::Checked);
                }
                else {
                      setItemVisibility(key, Qt::Unchecked);
                }
            }
        }
    }
    mainWindow->MakeAttributeList();
    mainWindow->in_FocusGraph = false;
}

void MapIndex::setDrawingTreeChecks()
{
    MainWindow *mainWindow = ((MainWindow *) m_mainWindow);
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    MetaGraph *graph = graphDoc->m_meta_graph;

    for (auto iter: m_treedrawingmap) {
        ItemTreeEntry entry = iter.second;
        if (entry.m_subcat != -1) {
            if (graph->getLineLayer(entry.m_cat,entry.m_subcat).isShown()) {
                  iter.first->setIcon(0, mainWindow->m_tree_icon[12]);
            }
            else {
                  iter.first->setIcon(0, mainWindow->m_tree_icon[13]);
            }
        }
    }
}

// clear the graph tree (not the drawing tree) but also clear the attribute list

void MapIndex::clearGraphTree()
{
    for (int i = 2; i >= 0; i--) {
        if (m_treeroots[i]) {
            m_treeroots[i] = NULL;
        }
    }
    m_treegraphmap.clear();
}

void MapIndex::makeGraphTree()
{
    MainWindow *mainWindow = ((MainWindow *) m_mainWindow);
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    MetaGraph *graph = graphDoc->m_meta_graph;

    int state = graph->getState();

    if (state & MetaGraph::POINTMAPS) {
        if (!m_treeroots[0]) {
            QTreeWidgetItem* hItem = addNewItem(tr("Visibility Graphs"));
            hItem->setIcon(0, mainWindow->m_tree_icon[0]);
            ItemTreeEntry entry(0,-1,-1);
            m_treegraphmap[hItem] = entry;
            m_treeroots[0] = hItem;
        }
        int i = 0;
        for (auto& pointmap: graph->getPointMaps()) {
            QString name = QString(pointmap.getName().c_str());
            QTreeWidgetItem* hItem = addNewItem(name, m_treeroots[0]);
            setItemVisibility(hItem, Qt::Unchecked);
            setItemEditability(hItem, Qt::Unchecked);
            ItemTreeEntry entry(0,(short)i,-1);
            m_treegraphmap.insert(std::make_pair(hItem,entry));
            i++;
        }
    }
    else if (m_treeroots[0]) {
        m_treeroots[0]->removeChild(m_treeroots[0]);
        auto iter = m_treegraphmap.find(m_treeroots[0]);
        if(iter != m_treegraphmap.end()) {
            m_treegraphmap.erase(iter);
        }
        m_treeroots[0] = NULL;
    }

    if (state & MetaGraph::SHAPEGRAPHS) {
        if (!m_treeroots[1]) {
            QTreeWidgetItem* hItem = addNewItem(tr("Shape Graphs"));
            hItem->setIcon(0, mainWindow->m_tree_icon[1]);
            ItemTreeEntry entry(1,-1,-1);
            m_treegraphmap[hItem] = entry;
            m_treeroots[1] = hItem;
        }
        for (size_t i = 0; i < graph->getShapeGraphs().size(); i++) {
            QString name = QString(graph->getShapeGraphs()[i]->getName().c_str());
            QTreeWidgetItem* hItem = addNewItem(name, m_treeroots[1]);
            setItemVisibility(hItem, Qt::Unchecked);
            setItemEditability(hItem, Qt::Unchecked);
            ItemTreeEntry entry(1,(short)i,-1);
            m_treegraphmap.insert(std::make_pair(hItem,entry));
            LayerManagerImpl& layers = graph->getShapeGraphs()[i]->getLayers();
            if(layers.getNumLayers() > 1) {
                for (int j = 0; j < layers.getNumLayers(); j++) {
                    QString name = QString(layers.getLayerName(j).c_str());
                    QTreeWidgetItem* hNewItem = addNewItem(name, hItem);
                    ItemTreeEntry entry(1,(short)i,j);
                    m_treegraphmap[hNewItem] = entry;
                }
            }
        }
    }
    else if (m_treeroots[1]) {
        m_treeroots[1]->removeChild(m_treeroots[1]);
        auto iter = m_treegraphmap.find(m_treeroots[1]);
        if(iter != m_treegraphmap.end()) {
            m_treegraphmap.erase(iter);
        }
        m_treeroots[1] = NULL;
    }

    if (state & MetaGraph::DATAMAPS) {
        if (!m_treeroots[2]) {
            QTreeWidgetItem* hItem = addNewItem(tr("Data Maps"));
            hItem->setIcon(0, mainWindow->m_tree_icon[2]);
            ItemTreeEntry entry(2,-1,-1);
            m_treegraphmap[hItem] = entry;
            m_treeroots[2] = hItem;
        }
        for (size_t i = 0; i < graph->getDataMaps().size(); i++) {
            QString name = QString(graph->getDataMaps()[i].getName().c_str());
            QTreeWidgetItem* hItem = addNewItem(name, m_treeroots[2]);
            setItemVisibility(hItem, Qt::Unchecked);
            setItemEditability(hItem, Qt::Unchecked);
            ItemTreeEntry entry(2,(short)i,-1);
            m_treegraphmap[hItem] = entry;

            LayerManagerImpl layers = graph->getDataMaps()[i].getLayers();
            if(layers.getNumLayers() > 1) {
                for (int j = 0; j < layers.getNumLayers(); j++) {
                    QString name = QString(layers.getLayerName(j).c_str());
                    QTreeWidgetItem* hNewItem = addNewItem(name, hItem);
                    setItemVisibility(hNewItem, Qt::Unchecked);
                    ItemTreeEntry entry(2,(short)i,j);
                    m_treegraphmap.insert(std::make_pair(hNewItem,entry));
                }
            }
        }
    }
    else if (m_treeroots[2]) {
        m_treeroots[2]->removeChild(m_treeroots[2]);
        auto iter = m_treegraphmap.find(m_treeroots[2]);
        if(iter != m_treegraphmap.end()) {
            m_treegraphmap.erase(iter);
        }
        m_treeroots[2] = NULL;
    }

    setGraphTreeChecks();
}

void MapIndex::makeDrawingTree()
{
    MainWindow *mainWindow = ((MainWindow *) m_mainWindow);
    QGraphDoc *graphDoc = mainWindow->activeMapDoc();
    MetaGraph *graph = graphDoc->m_meta_graph;

    int state = graph->getState();

    if (state & MetaGraph::LINEDATA) {
        if (m_treeroots[4]) {
            m_treeroots[4] = NULL;
            m_treedrawingmap.clear();
        }
        // we'll do all of these if it works...
        QTreeWidgetItem* root = addNewItem(tr("Drawing Layers"));
        root->setIcon(0, mainWindow->m_tree_icon[4]);
        ItemTreeEntry entry(4,0,-1);
        m_treedrawingmap.insert(std::make_pair(root,entry));
        m_treeroots[4] = root;
        for (int i = 0; i < graph->getLineFileCount(); i++) {

            QTreeWidgetItem* subroot = addNewItem(QString(graph->getLineFileName(i).c_str()), m_treeroots[4]);
            subroot->setIcon(0, mainWindow->m_tree_icon[8]);
            ItemTreeEntry entry(4,i,-1);
            m_treedrawingmap.insert(std::make_pair(subroot,entry));

            for (int j = 0; j < graph->getLineLayerCount(i); j++) {
                QString name(graph->getLineLayer(i,j).getName().c_str());
                QTreeWidgetItem* hItem = addNewItem(name, subroot);
                if (graph->getLineLayer(i,j).isShown()) {
                    setItemVisibility(hItem, Qt::Checked);
                }
                else {
                    setItemVisibility(hItem, Qt::Unchecked);
                }
                ItemTreeEntry entry(4,i,j);
                m_treedrawingmap.insert(std::make_pair(hItem,entry));
            }
        }
    }
}


QT_END_NAMESPACE
