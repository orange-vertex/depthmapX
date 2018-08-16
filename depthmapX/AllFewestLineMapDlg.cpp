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

#include "AllFewestLineMapDlg.h"
#include "mainwindow.h"
#include <QMessageBox>

CAllFewestLineMapDlg::CAllFewestLineMapDlg(QWidget *parent)
: QDialog(parent)
{
    setupUi(this);
    m_all_line = true;
    c_all_line->setChecked(m_all_line);
    m_fewest_line_subsets = false;
    c_fewest_line_subsets->setChecked(m_fewest_line_subsets);
    m_fewest_line_minimal = false;
    c_fewest_line_minimal->setChecked(m_fewest_line_minimal);
}

void CAllFewestLineMapDlg::OnOK()
{
    m_all_line = c_all_line->isChecked();
    m_fewest_line_subsets = c_fewest_line_subsets->isChecked();
    m_fewest_line_minimal = c_fewest_line_minimal->isChecked();

	accept();
}

void CAllFewestLineMapDlg::OnCancel()
{
	reject();
}
