/****************************************************************************
 * MeshLab                                                           o o     *
 * A versatile mesh processing toolbox                             o     o   *
 *                                                                _   O  _   *
 * Copyright(C) 2005                                                \/)\/    *
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

#include "edit_select.h"
#include <wrap/gl/addons.h>

#include <meshlab/glarea.h>
#include <meshlab/mainwindow.h>
#include <common/GLExtensionsManager.h>
#include <common/plugins/plugin_manager.h>
#include <wrap/gl/pick.h>
#include <wrap/qt/device_to_logical.h>
#include <meshlab/glarea.h>
#include <vcg/space/intersection2.h>
#include <QApplication>


using namespace std;
using namespace vcg;

EditSelectPlugin::EditSelectPlugin(RichParameterList* cgp, int ConnectedMode) :selectionMode(ConnectedMode) {
	isDragging = false;
    currentGlobalParamSet = cgp;
    qApp->installEventFilter(this);
}

QString EditSelectPlugin::info()
{
	return tr("Interactive selection inside a dragged rectangle in screen space");
}

void EditSelectPlugin::suggestedRenderingData(MeshModel & /*m*/, MLRenderingData & dt)
{
	MLPerViewGLOptions opts;
	dt.get(opts);
	opts._sel_enabled = true;

	if ((selectionMode == SELECT_FACE_MODE) || (selectionMode == SELECT_CONN_MODE))
		opts._face_sel = true;

	if (selectionMode == SELECT_VERT_MODE)
		opts._vertex_sel = true;

	if (selectionMode == SELECT_AREA_MODE)
	{
		opts._face_sel = true;
		opts._vertex_sel = true;
	}

	dt.set(opts);
}
bool EditSelectPlugin::keyReleaseEventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyRelease && QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (m_ref && gla_ref) {
            // Check if the released key is the Alt key
            if (keyEvent->key() == Qt::Key_Alt) {
                keyReleaseEvent(keyEvent, *m_ref, gla_ref);
            }
        }
    }
    // Pass the event to the base class event filter
    return QObject::eventFilter(obj, event);
}

void EditSelectPlugin::keyReleaseEvent(QKeyEvent *e, MeshModel &m, GLArea *gla)
{
    bool ctrlState = currentGlobalParamSet->getBool("MeshLab::Editors::InvertCTRLBehavior");

    // global "all" commands
    switch (e->key()) {
        case Qt::Key_A: // select all
            if (areaMode == 0) { // vertices
                tri::UpdateSelection<CMeshO>::VertexAll(m.cm);
                gla->updateSelection(m.id(), true, false);
            } else if (areaMode == 1) { // faces
                tri::UpdateSelection<CMeshO>::FaceAll(m.cm);
                gla->updateSelection(m.id(), false, true);
            }
            gla->update();
            e->accept();
            break;

        case Qt::Key_D: // deselect all
            if (areaMode == 0) { // vertices
                tri::UpdateSelection<CMeshO>::VertexClear(m.cm);
                gla->updateSelection(m.id(), true, false);
            } else if (areaMode == 1) { // faces
                tri::UpdateSelection<CMeshO>::FaceClear(m.cm);
                gla->updateSelection(m.id(), false, true);
            }
            gla->update();
            e->accept();
            break;

        case Qt::Key_I: // invert all
            if (areaMode == 0) { // vertices
                tri::UpdateSelection<CMeshO>::VertexInvert(m.cm);
                gla->updateSelection(m.id(), true, false);
            } else if (areaMode == 1) { // faces
                tri::UpdateSelection<CMeshO>::FaceInvert(m.cm);
                gla->updateSelection(m.id(), false, true);
            }
            gla->update();
            e->accept();
            break;

        default:
            break;
    }

    if (selectionMode == SELECT_AREA_MODE) {
        switch (e->key()) {
            case Qt::Key_T: // toggle pick mode
                areaMode = (areaMode + 1) % 2;
                gla->update();
                e->accept();
                break;

            case Qt::Key_C: // clear Polyline
                selPolyLine.clear();
                gla->update();
                e->accept();
                break;

            case Qt::Key_Backspace: // remove last point Polyline
                if (selPolyLine.size() > 0)
                    selPolyLine.pop_back();
                gla->update();
                e->accept();
                break;

            case Qt::Key_Q: // add to selection
                doSelection(m, gla, 0);
                gla->update();
                e->accept();
                break;

            case Qt::Key_W: // sub from selection
                doSelection(m, gla, 1);
                gla->update();
                e->accept();
                break;

            case Qt::Key_E: // invert selection
                doSelection(m, gla, 2);
                gla->update();
                e->accept();
                break;

            default:
                break;
        }
        gla->setCursor(QCursor(QPixmap(":/images/sel_area.png"), 1, 1));
    } else {
        if (ctrlState){
            gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
        } else {
            gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
        }
        Qt::KeyboardModifiers mod = e->modifiers();
        if (e->key() == Qt::Key_Alt)
        {
            if (ctrlState){
                if (mod & Qt::ControlModifier){
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
                } else if (mod & Qt::ShiftModifier){
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                } else{
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
                }
            } else{
                if (mod & Qt::ControlModifier){
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
                } else if (mod & Qt::ShiftModifier){
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                } else{
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
                }
            }
            e->accept();
        }

        switch (selectionMode) {
            case SELECT_VERT_MODE:
                if (ctrlState){
                    if (mod & Qt::ControlModifier)
                        gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
                    else if (mod & Qt::ShiftModifier)
                        gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                } else {
                    if (mod & Qt::ControlModifier)
                        gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
                    else if (mod & Qt::ShiftModifier)
                        gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                }
                break;

            default:
                if (mod & Qt::AltModifier) {
                    if (ctrlState){
                        if (mod & Qt::ControlModifier)
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
                        else if (mod & Qt::ShiftModifier)
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                        else
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
                    } else {
                        if (mod & Qt::ControlModifier)
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
                        else if (mod & Qt::ShiftModifier)
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                        else
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
                    }
                } else {
                    if (ctrlState){
                        if (mod & Qt::ControlModifier)
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
                        else if (mod & Qt::ShiftModifier)
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                    } else {
                        if (mod & Qt::ControlModifier)
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
                        else if (mod & Qt::ShiftModifier)
                            gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                    }
                }
                break;
        }
    }
    if(ctrlState){
            gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
        } else {
            gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
        }
}

void EditSelectPlugin::doSelection(MeshModel &m, GLArea *gla, int mode)
{
  QImage bufQImg(this->viewpSize[2],this->viewpSize[3],QImage::Format_RGB32);
  bufQImg.fill(Qt::white);
  QPainter bufQPainter(&bufQImg);
  vector<QPointF> qpoints;
  for(size_t i=0;i<selPolyLine.size();++i)
    qpoints.push_back(QPointF(selPolyLine[i][0],selPolyLine[i][1]));    
  bufQPainter.setBrush(QBrush(Qt::black));
  bufQPainter.drawPolygon(&qpoints[0],qpoints.size(), Qt::WindingFill); 
  QRgb blk=QColor(Qt::black).rgb();
  
    
  static Eigen::Matrix<Scalarm,4,4> LastSelMatrix;
  static vector<Point3m> projVec;
  static MeshModel *lastMeshModel=0;
  if((LastSelMatrix != SelMatrix) || lastMeshModel != &m)
  {
    GLPickTri<CMeshO>::FillProjectedVector(m.cm,projVec,this->SelMatrix,this->SelViewport);
    LastSelMatrix=this->SelMatrix;
    lastMeshModel=&m;
  }    
  
    if (areaMode == 0) // vertices
    {   
      for (size_t vi = 0; vi<m.cm.vert.size(); ++vi) if (!m.cm.vert[vi].IsD())
      {
        bool res=false;
        if ((projVec[vi][2] <= -1.0) || (projVec[vi][2] >= 1.0) ||
            (projVec[vi][0] <= 0) || (projVec[vi][0] >= this->viewpSize[2]) ||
            (projVec[vi][1] <= 0) || (projVec[vi][1] >= this->viewpSize[3]))
          res = false;
        else
        {
          res = (bufQImg.pixel( projVec[vi][0],projVec[vi][1]) == blk);
        }
        if (res)
          switch(mode){
          case 0: m.cm.vert[vi].SetS(); break;
          case 1: m.cm.vert[vi].ClearS(); break;
          case 2: m.cm.vert[vi].IsS() ? m.cm.vert[vi].ClearS() : m.cm.vert[vi].SetS();
          }
      }
      gla->updateSelection(m.id(), true, false);
    }
    else if (areaMode == 1) //faces
	{
      for (size_t fi = 0; fi < m.cm.face.size(); ++fi) if (!m.cm.face[fi].IsD())
      {
        bool res=false;
        for (int vi = 0; vi < 3 && !res ; vi++)
        {
          int vInd=tri::Index(m.cm,m.cm.face[fi].V(vi));
          if ((projVec[vInd][2] <= -1.0) || (projVec[vInd][2] >= 1.0) ||
              (projVec[vInd][0] <= 0) || (projVec[vInd][0] >= this->viewpSize[2]) ||
              (projVec[vInd][1] <= 0) || (projVec[vInd][1] >= this->viewpSize[3]))
            res = false; 
          else
            res = (bufQImg.pixel( projVec[vInd][0],projVec[vInd][1]) == blk);
        }

        if (res) // do the actual selection
        {
          switch(mode){
          case 0: m.cm.face[fi].SetS(); break;
          case 1: m.cm.face[fi].ClearS(); break;
          case 2: m.cm.face[fi].IsS() ? m.cm.face[fi].ClearS() : m.cm.face[fi].SetS();
          }
        }
      }
      gla->updateSelection(m.id(), false, true);
    }
    
}

void EditSelectPlugin::keyPressEvent(QKeyEvent *event, MeshModel &m, GLArea *gla)
{
    bool ctrlState = currentGlobalParamSet->getBool("MeshLab::Editors::InvertCTRLBehavior");

    switch (event->key())
    {
        case Qt::Key_Control:
        {
            if (ctrlState) {
                gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
            } else {
                gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
            }
            break;
        }
        case Qt::Key_Shift:
        {
            if (ctrlState) {
                gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
            } else {
                gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
            }
            break;
        }
        case Qt::Key_Alt:
        {
            if (ctrlState) {
                if (event->modifiers() & Qt::ControlModifier) {
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
                } else if (event->modifiers() & Qt::ShiftModifier) {
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                } else {
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus_eye.png"), 1, 1));
                }
            } else {
                if (event->modifiers() & Qt::ControlModifier) {
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
                } else if (event->modifiers() & Qt::ShiftModifier) {
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_minus.png"), 1, 1));
                } else {
                    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_eye.png"), 1, 1));
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void EditSelectPlugin::mousePressEvent(QMouseEvent * event, MeshModel &m, GLArea *gla)
{
    bool ctrlState = currentGlobalParamSet->getBool("MeshLab::Editors::InvertCTRLBehavior");
    if (selectionMode == SELECT_AREA_MODE)
    {
        selPolyLine.push_back(QTLogicalToOpenGL(gla, event->pos()));
        return;
    }

    LastSelVert.clear();
    LastSelFace.clear();

    int ctrl = (event->modifiers() & Qt::ControlModifier) ? 1 : 0;
    int shift = (event->modifiers() & Qt::ShiftModifier) ? 1 : 0;
    int alt = (event->modifiers() & Qt::AltModifier) ? 1 : 0;

    switch (ctrlState * 4 + ctrl * 2 + shift) {
        case 0:  // !ctrlState && !ctrl && !shift
            composingSelMode = SMClear;
            selectFrontFlag = false;
            break;
        case 1:  // !ctrlState && !ctrl && shift
            composingSelMode = SMSub;
            selectFrontFlag = false;
            break;
        case 2:  // !ctrlState && ctrl && !shift
            composingSelMode = SMAdd;
            selectFrontFlag = false;
            break;
        case 3:  // !ctrlState && ctrl && shift
            composingSelMode = SMSub;
            selectFrontFlag = false;
            break;
        case 4:  // ctrlState && !ctrl && !shift
            composingSelMode = SMAdd;
            selectFrontFlag = alt;
            break;
        case 5:  // ctrlState && !ctrl && shift
            composingSelMode = SMSub;
            selectFrontFlag = alt;
            break;
        case 6:  // ctrlState && ctrl && !shift
            composingSelMode = SMClear;
            selectFrontFlag = alt;
            break;
        case 7:  // ctrlState && ctrl && shift
            composingSelMode = SMSub;
            selectFrontFlag = alt;
            break;
    }

    start = QTLogicalToOpenGL(gla, event->pos());
    cur = start;

    if (ctrlState && (!(event->modifiers() & Qt::ControlModifier) || (event->modifiers() & Qt::ShiftModifier))) {
        for (CMeshO::FaceIterator fi = m.cm.face.begin(); fi != m.cm.face.end(); ++fi) {
            if (!(*fi).IsD() && (*fi).IsS()) {
                LastSelFace.push_back(&*fi);
            }
        }

        for (CMeshO::VertexIterator vi = m.cm.vert.begin(); vi != m.cm.vert.end(); ++vi) {
            if (!(*vi).IsD() && (*vi).IsS()) {
                LastSelVert.push_back(&*vi);
            }
        }
    }
}



void EditSelectPlugin::mouseMoveEvent(QMouseEvent * event, MeshModel &m, GLArea * gla)
{
    if (selectionMode == SELECT_AREA_MODE)
    {
        selPolyLine.back() = QTLogicalToOpenGL(gla, event->pos());
    }
    else
    {
        prev = cur;
        cur = QTLogicalToOpenGL(gla, event->pos());
        isDragging = true;
    }
    gla->update();

    //    // to avoid too frequent rendering
    //    if(gla->lastRenderingTime() < 200 )
    //    {
    //    }
    //    else{
    //      gla->makeCurrent();
    //      glDrawBuffer(GL_FRONT);
    //      DrawXORRect(gla,true);
    //      glDrawBuffer(GL_BACK);
    //      glFlush();
    //    }
}

void EditSelectPlugin::mouseReleaseEvent(QMouseEvent * event, MeshModel &m, GLArea * gla)
{
	//gla->update();
	if (gla == NULL)
		return;

	gla->updateAllSiblingsGLAreas();

	if (selectionMode == SELECT_AREA_MODE)
	{
		selPolyLine.back() = QTLogicalToOpenGL(gla, event->pos());
		return;
	}

	prev = cur;
	cur = QTLogicalToOpenGL(gla, event->pos());
	isDragging = false;
}

void EditSelectPlugin::DrawXORPolyLine(GLArea * gla)
{
	if (selPolyLine.size() == 0)
		return;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, QTDeviceWidth(gla), 0, QTDeviceHeight(gla), -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);
	glColor3f(1, 1, 1);
	glLineStipple(1, 0xAAAA);
	glEnable(GL_LINE_STIPPLE);
    glLineWidth(QTLogicalToDevice(gla,1));
	//draw PolyLine
	if (selPolyLine.size() == 1)
	{
		glBegin(GL_POINTS);
		glVertex(selPolyLine[0]);
	}
	else if (selPolyLine.size() == 2)
	{
		glBegin(GL_LINES);
		glVertex(selPolyLine[0]);
		glVertex(selPolyLine[1]);
	}
	else
	{
		glBegin(GL_LINE_LOOP);
		for (size_t ii = 0; ii < selPolyLine.size(); ii++)
			glVertex(selPolyLine[ii]);
	}
	glEnd();

	glDisable(GL_LOGIC_OP);
	// Closing 2D
	glPopAttrib();
	glPopMatrix(); // restore modelview
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void EditSelectPlugin::DrawXORRect(GLArea * gla, bool doubleDraw)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, QTDeviceWidth(gla), 0, QTDeviceHeight(gla), -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);
	glColor3f(1, 1, 1);
	if (doubleDraw)
	{
		glBegin(GL_LINE_LOOP);
		glVertex(start);
		glVertex2f(prev.X(), start.Y());
		glVertex(prev);
		glVertex2f(start.X(), prev.Y());
		glEnd();
	}
	glBegin(GL_LINE_LOOP);
	glVertex(start);
	glVertex2f(cur.X(), start.Y());
	glVertex(cur);
	glVertex2f(start.X(), cur.Y());
	glEnd();
	glDisable(GL_LOGIC_OP);

	// Closing 2D
	glPopAttrib();
	glPopMatrix(); // restore modelview
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

}

void EditSelectPlugin::decorate(MeshModel &m, GLArea * gla)
{
    bool ctrlState = currentGlobalParamSet->getBool("MeshLab::Editors::InvertCTRLBehavior");
	if (selectionMode == SELECT_AREA_MODE)
	{
		// get proj data of last rendering
		glPushMatrix();
		glMultMatrix(m.cm.Tr);
        GLPickTri<CMeshO>::glGetMatrixAndViewport(this->SelMatrix, this->SelViewport);                
		glGetDoublev(GL_MODELVIEW_MATRIX, mvMatrix_f);
		glGetDoublev(GL_PROJECTION_MATRIX, prMatrix_f);
		glGetIntegerv(GL_VIEWPORT, viewpSize);
		glPopMatrix();

		// draw current poly
		DrawXORPolyLine(gla);

		// then, the status
		QString line1 = "";
		QString line2 = "";
		QString line3 = "";
		QString line4 = "";

		if (areaMode == 0)
			line1 = "Vertices Selection - T for faces";
		else if (areaMode == 1)
			line1 = "Faces Selection - T for vertices";

		line2 = "C to clear polyline, BACKSPACE to remove last point";

		if (selPolyLine.size() < 3)
			line3 = "cannot select - more points needed";
		else
			line3 = "Q to add, W to subtract, E to invert";

		line4 = "<br>A select all, D de-select all, I invert all";

		this->realTimeLog("Selection from Area", m.shortName(),
			"%s<br>%s<br>%s<br>%s", line1.toStdString().c_str(), line2.toStdString().c_str(), line3.toStdString().c_str(), line4.toStdString().c_str());

		return;
	}
	else
	{
        if (ctrlState){
            QString line1, line2, line3;

            line1 = "Drag to select";
            if ((selectionMode == SELECT_FACE_MODE) || (selectionMode == SELECT_CONN_MODE))
                line2 = "you may hold:<br>- CTRL to NEW selection<br>- SHIFT to subtract<br>- ALT to select only visible";
            else
            line2 = "you may hold:<br>- CTRL to NEW selection<br>- SHIFT to subtract";
            line3 = "<br>A select all, D de-select all, I invert all";

            this->realTimeLog("Interactive Selection", m.shortName(), "%s<br>%s<br>%s", line1.toStdString().c_str(), line2.toStdString().c_str(), line3.toStdString().c_str());
        }
        else{
            QString line1, line2, line3;

            line1 = "Drag to select";
            if ((selectionMode == SELECT_FACE_MODE) || (selectionMode == SELECT_CONN_MODE))
               line2 = "you may hold:<br>- CTRL to add<br>- SHIFT to subtract<br>- ALT to select only visible";
            else
            line2 = "you may hold:<br>- CTRL to add<br>- SHIFT to subtract";
            line3 = "<br>A select all, D de-select all, I invert all";

            this->realTimeLog("Interactive Selection", m.shortName(), "%s<br>%s<br>%s", line1.toStdString().c_str(), line2.toStdString().c_str(), line3.toStdString().c_str());
        }
    }

	if (isDragging)
	{
		DrawXORRect(gla, false);
		vector<CMeshO::FacePointer>::iterator fpi;
		// Starting Sel
		vector<CMeshO::FacePointer> NewSelFace;
		Point2f mid = (start + cur) / 2;
		Point2f wid = vcg::Abs(start - cur);

		glPushMatrix();
		glMultMatrix(m.cm.Tr);
		if (selectionMode == SELECT_VERT_MODE)
		{
			//m.cm.selvert.clear();
			vector<CMeshO::VertexPointer> NewSelVert;
			vector<CMeshO::VertexPointer>::iterator vpi;

			GLPickTri<CMeshO>::PickVert(mid[0], mid[1], m.cm, NewSelVert, wid[0], wid[1]);
			glPopMatrix();
			tri::UpdateSelection<CMeshO>::VertexClear(m.cm);

			switch (composingSelMode)
			{
			case SMSub:  // Subtract mode : The faces in the rect must be de-selected
				for (vpi = LastSelVert.begin(); vpi != LastSelVert.end(); ++vpi)
					(*vpi)->SetS();
				for (vpi = NewSelVert.begin(); vpi != NewSelVert.end(); ++vpi)
					(*vpi)->ClearS();
				break;
			case SMAdd:  // Subtract mode : The faces in the rect must be de-selected
				for (vpi = LastSelVert.begin(); vpi != LastSelVert.end(); ++vpi)
					(*vpi)->SetS();
			case SMClear:  // Subtract mode : The faces in the rect must be de-selected
				for (vpi = NewSelVert.begin(); vpi != NewSelVert.end(); ++vpi)
					(*vpi)->SetS();
				break;
			}
			//for (unsigned int ii = 0; ii < m.cm.VN(); ++ii)
			//{
			//	CVertexO& vv = m.cm.vert[ii];
			//	if (!vv.IsD() && vv.IsS())
			//	{
			//		m.cm.selvert.push_back(Point3m::Construct(vv.cP()));
			//		++m.cm.svn;
			//	}
			//}
			gla->updateSelection(m.id(), true,false);
		}
		else
		{
			//m.cm.selface.clear();
			if (selectFrontFlag)	GLPickTri<CMeshO>::PickVisibleFace(mid[0], mid[1], m.cm, NewSelFace, wid[0], wid[1]);
			else                GLPickTri<CMeshO>::PickFace(mid[0], mid[1], m.cm, NewSelFace, wid[0], wid[1]);

			//    qDebug("Pickface: rect %i %i - %i %i",mid.x(),mid.y(),wid.x(),wid.y());
			//    qDebug("Pickface: Got  %i on %i",int(NewSelFace.size()),int(m.cm.face.size()));
			glPopMatrix();
			tri::UpdateSelection<CMeshO>::FaceClear(m.cm);
			switch (composingSelMode)
			{
			case SMSub:  // Subtract mode : The faces in the rect must be de-selected
				if (selectionMode == SELECT_CONN_MODE)
				{
					for (fpi = NewSelFace.begin(); fpi != NewSelFace.end(); ++fpi)
						(*fpi)->SetS();
					tri::UpdateSelection<CMeshO>::FaceConnectedFF(m.cm);
					NewSelFace.clear();
					for (CMeshO::FaceIterator fi = m.cm.face.begin(); fi != m.cm.face.end(); ++fi)
						if (!(*fi).IsD() && (*fi).IsS()) NewSelFace.push_back(&*fi);
				}
				// Normal case: simply deselect what has been selected.
				for (fpi = LastSelFace.begin(); fpi != LastSelFace.end(); ++fpi)
					(*fpi)->SetS();
				for (fpi = NewSelFace.begin(); fpi != NewSelFace.end(); ++fpi)
					(*fpi)->ClearS();
				break;
			case SMAdd:
				for (fpi = LastSelFace.begin(); fpi != LastSelFace.end(); ++fpi)
					(*fpi)->SetS();
			case SMClear:
				for (fpi = NewSelFace.begin(); fpi != NewSelFace.end(); ++fpi)
					(*fpi)->SetS();
				if (selectionMode == SELECT_CONN_MODE)
					tri::UpdateSelection<CMeshO>::FaceConnectedFF(m.cm);
				break;
			}
			gla->updateSelection(m.id(), false, true);
			isDragging = false;
		}

	}
}

bool EditSelectPlugin::startEdit(MeshModel & m, GLArea * gla, MLSceneGLSharedDataContext* /*cont*/)
{
    bool ctrlState = currentGlobalParamSet->getBool("MeshLab::Editors::InvertCTRLBehavior");
	if (gla == NULL)
		return false;
	if (!GLExtensionsManager::initializeGLextensions_notThrowing())
		return false;
    if (ctrlState){
    gla->setCursor(QCursor(QPixmap(":/images/sel_rect_plus.png"), 1, 1));
    }
    else{
    gla->setCursor(QCursor(QPixmap(":/images/sel_rect.png"), 1, 1));
    }


	if (selectionMode == SELECT_AREA_MODE)
	{
		if (m.cm.fn > 0)
			areaMode = 1;
		else
			areaMode = 0;

		selPolyLine.clear();
        gla->setCursor(QCursor(QPixmap(":/images/sel_area.png"), 1, 1));
	}

	if (selectionMode == SELECT_VERT_MODE)
		areaMode = 0;

	if ((selectionMode == SELECT_FACE_MODE) || (selectionMode == SELECT_CONN_MODE))
		areaMode = 1;

	if (selectionMode == SELECT_CONN_MODE)
		m.updateDataMask(MeshModel::MM_FACEFACETOPO);
	return true;
}
