/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef TREEMANAGER_H
#define TREEMANAGER_H

class TreeData;
#include <DFTypes.h>
class Model;

class TreeManager
{
    public:
        TreeManager(vector<t_matgloss> & wood);
        ~TreeManager();
        TreeData *getTreeDesc( int32_t matgloss_idx );

    private:
        // map between matgloss idx and treedata
        map <int32_t, TreeData *> treemap;
};

#endif // TREEMANAGER_H