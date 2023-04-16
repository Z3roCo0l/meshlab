/****************************************************************************
* MeshLab                                                           o o     *
* A versatile mesh processing toolbox                             o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005-2008                                           \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *   
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/

#include "edit_select_factory.h"
#include "edit_select.h"
#include "common/parameters/rich_parameter_list.h"

EditSelectFactory::EditSelectFactory()
{
	editSelect = new QAction(QIcon(":/images/select_face.png"),"Select Faces in a rectangular region", this);
	editSelectConnected = new QAction(QIcon(":/images/select_face_connected.png"),"Select Connected Components in a region", this);
	editSelectVert = new QAction(QIcon(":/images/select_vertex.png"),"Select Vertices", this);
	editSelectArea = new QAction(QIcon(":/images/select_area.png"), "Select Faces/Vertices inside polyline area", this);

	actionList.push_back(editSelectVert);
	actionList.push_back(editSelect);
	actionList.push_back(editSelectConnected);
	actionList.push_back(editSelectArea);
	
	foreach(QAction *editAction, actionList)
		editAction->setCheckable(true); 	
}
void EditSelectFactory::initGlobalParameterList(RichParameterList& defaultGlobalParamSet) {
    defaultGlobalParamSet.addParam(RichBool(InvertCtrlBehavior(), true,"Inverting the behavior of the CTRL modifier on edit selec rectangle tools",""));
}

QString EditSelectFactory::pluginName() const
{
	return "EditSelect";
}

//TODO - same as under this lines but better
//EditTool* EditSelectFactory::getEditTool(const QAction *action, MainWindow *mainWindow)
//{
//    static EditSelectPlugin selectFaceMode(EditSelectPlugin::SELECT_FACE_MODE);
//    static EditSelectPlugin selectConnMode(EditSelectPlugin::SELECT_CONN_MODE);
//    static EditSelectPlugin selectVertMode(EditSelectPlugin::SELECT_VERT_MODE);
//    static EditSelectPlugin selectAreaMode(EditSelectPlugin::SELECT_AREA_MODE);

//    EditSelectPlugin* result = nullptr;
//    if(action == editSelect)
//        result = &selectFaceMode;
//    else if(action == editSelectConnected)
//        result = &selectConnMode;
//    else if(action == editSelectVert)
//        result = &selectVertMode;
//    else if (action == editSelectArea)
//        result = &selectAreaMode;

//    QObject::connect(mainWindow, SIGNAL(dispatchCustomSettings()), result, SLOT(updateCustomSettingValues()));

//    if (result == nullptr) {
//        assert(0);
//    }

//    return static_cast<EditTool*>(result);
//}

//get the edit tool for the given action
EditTool* EditSelectFactory::getEditTool(const QAction *action)
{
    EditSelectPlugin* result = nullptr;
    if(action == editSelect)
        result = new EditSelectPlugin(currentGlobalParamSet,EditSelectPlugin::SELECT_FACE_MODE);
    else if(action == editSelectConnected)
        result = new EditSelectPlugin(currentGlobalParamSet,EditSelectPlugin::SELECT_CONN_MODE);
    else if(action == editSelectVert)
        result = new EditSelectPlugin(currentGlobalParamSet,EditSelectPlugin::SELECT_VERT_MODE);
    else if (action == editSelectArea)
        result = new EditSelectPlugin(currentGlobalParamSet,EditSelectPlugin::SELECT_AREA_MODE);

    if (result == nullptr) {
        assert(0);
    }

    return (EditTool*)result;
}

QString EditSelectFactory::getEditToolDescription(const QAction * /*a*/)
{
	return EditSelectPlugin::info();
}

MESHLAB_PLUGIN_NAME_EXPORTER(EditSelectFactory)
