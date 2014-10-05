#include <QtGui>
#include <QtWidgets/QtWidgets>

#include "dl_dxf.h"

#include "pattern.h"

#include <iostream>
#include <cstdio>

#include <QDebug>
/************************************************************************/
/* DXF文件导入器                                                         */
/************************************************************************/
// 注意 由于时间关系 本实现只能处理简单的DXF格式
void DXFImpoter::addLayer( const DL_LayerData& data )
{
#ifdef _DEBUG
	char str[100];
	sprintf(str, "LAYER: %s flags: %d\n", data.name.c_str(), data.flags);
	OutputDebugString(str);
	printAttributes();
#endif
}

void DXFImpoter::addPoint( const DL_PointData& data )
{
#ifdef _DEBUG
	char str[100];
	sprintf(str, "POINT    (%6.3f, %6.3f, %6.3f)\n", data.x, data.y, data.z);
	OutputDebugString(str);
	printAttributes();
#endif
}

void DXFImpoter::addLine( const DL_LineData& data )
{
#ifdef _DEBUG
	char str[100];
	sprintf(str, "LINE     (%6.3f, %6.3f, %6.3f) (%6.3f, %6.3f, %6.3f)\n", data.x1, data.y1, data.z1, data.x2, data.y2, data.z2);
	OutputDebugString(str);
	printAttributes();
#endif
	current_panel_ = attributes.getLayer();
	new_panel_flag_ = (current_panel_ != previous_panel_);
	if (new_panel_flag_)
	{
		temp_contour_.closeSubpath();
		panel_contours_.append(temp_contour_);
		temp_contour_ = QPainterPath();
		previous_panel_ = current_panel_;
	}
	else
	{
		if (temp_contour_.isEmpty())
		{
			temp_contour_.moveTo(data.x1 * scale_factor_, data.y1 * scale_factor_);
			temp_contour_.lineTo(data.x2 * scale_factor_, data.y2 * scale_factor_);
		}
		else
		{
			temp_contour_.lineTo(data.x2 * scale_factor_, data.y2 * scale_factor_);
		}
	}
}

void DXFImpoter::addArc( const DL_ArcData& data )
{
#ifdef _DEBUG
	char str[100];
	sprintf(str, "ARC      (%6.3f, %6.3f, %6.3f) %6.3f, %6.3f, %6.3f\n",
		data.cx, data.cy, data.cz,
		data.radius, data.angle1, data.angle2);
	OutputDebugString(str);
	printAttributes();
#endif
}

void DXFImpoter::addCircle( const DL_CircleData& data )
{
#ifdef _DEBUG
	char str[100];
	sprintf(str, "CIRCLE   (%6.3f, %6.3f, %6.3f) %6.3f\n",
		data.cx, data.cy, data.cz,
		data.radius);
	OutputDebugString(str);
	printAttributes();
#endif
}

void DXFImpoter::addPolyline( const DL_PolylineData& data )
{
#ifdef _DEBUG
	char str[100];
	sprintf(str, "POLYLINE \n");
	OutputDebugString(str);
	sprintf(str, "flags: %d\n", (int)data.flags);
	OutputDebugString(str);
	printAttributes();
#endif
	current_panel_ = attributes.getLayer();
	new_panel_flag_ = (current_panel_ != previous_panel_);
	if (new_panel_flag_)
	{
		temp_contour_.closeSubpath();
		panel_contours_.append(temp_contour_);
		temp_contour_ = QPainterPath();
		previous_panel_ = current_panel_;
	}
}

void DXFImpoter::addVertex( const DL_VertexData& data )
{
#ifdef _DEBUG
	char str[100];
	sprintf(str, "VERTEX   (%6.3f, %6.3f, %6.3f) %6.3f\n", data.x, data.y, data.z, data.bulge);
	OutputDebugString(str);
	printAttributes();
#endif
	if (temp_contour_.isEmpty())
	{
		temp_contour_.moveTo(data.x * scale_factor_, data.y * scale_factor_);
	}
	else
	{
		temp_contour_.lineTo(data.x * scale_factor_, data.y * scale_factor_);
	}
}

void DXFImpoter::add3dFace( const DL_3dFaceData& data )
{
#ifdef _DEBUG
	char str[100];
	sprintf(str, "3DFACE\n");
	OutputDebugString(str);
	for (int i=0; i<4; i++) {
		sprintf(str, "   corner %d: %6.3f %6.3f %6.3f\n", 
			i, data.x[i], data.y[i], data.z[i]);
		OutputDebugString(str);
	}
	printAttributes();
#endif
}

// 仅供输出日志 调试之用
void DXFImpoter::printAttributes()
{
	char str[100];
	sprintf(str, "  Attributes: Layer: %s, ", attributes.getLayer().c_str());
	OutputDebugString(str);
	sprintf(str, " Color: ");
	OutputDebugString(str);
	if (attributes.getColor()==256)	
	{
		sprintf(str, "BYLAYER");
		OutputDebugString(str);
	} 
	else if (attributes.getColor()==0) 
	{
		sprintf(str, "BYBLOCK");
		OutputDebugString(str);
	} 
	else 
	{
		sprintf(str, "%d", attributes.getColor());
		OutputDebugString(str);
	}
	sprintf(str, " Width: ");
	OutputDebugString(str);
	if (attributes.getWidth()==-1) 
	{
		sprintf(str, "BYLAYER");
		OutputDebugString(str);
	} 
	else if (attributes.getWidth()==-2) 
	{
		sprintf(str, "BYBLOCK");
		OutputDebugString(str);
	} 
	else if (attributes.getWidth()==-3) 
	{
		sprintf(str, "DEFAULT");
		OutputDebugString(str);
	} 
	else 
	{
		sprintf(str, "%d", attributes.getWidth());
		OutputDebugString(str);
	}
	sprintf(str, " Type: %s\n", attributes.getLineType().c_str());
	OutputDebugString(str);
}

void DXFImpoter::addLastContour()
{
	temp_contour_.closeSubpath();
	panel_contours_.append(temp_contour_);
}

void DXFImpoter::clear()
{
	previous_panel_ = current_panel_ = "1";
	new_panel_flag_ = false;
	temp_contour_ = QPainterPath();
	panel_contours_.clear();
}

/************************************************************************/
/* 缝合线                                                               */
/************************************************************************/
SeamLine::SeamLine( Line *startItem, Line *endItem, QMenu *contextMenu) : 
	QGraphicsLineItem(),
	start_item_(startItem), 
	end_item_(endItem), 
	context_menu_(contextMenu), 
	color_(Qt::green)
{
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setPen(QPen(color_, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void SeamLine::updatePosition()
{
	QLineF line(mapFromItem(start_item_, 0, 0), mapFromItem(end_item_, 0, 0));
	setLine(line);
}

void SeamLine::contextMenuEvent( QGraphicsSceneContextMenuEvent *event )
{
	scene()->clearSelection();
	setSelected(true);
	context_menu_->exec(event->screenPos());
}

void SeamLine::setColor( const QColor &color )
{
	color_ = color;
	setPen(QPen(color_, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

/************************************************************************/
/* 衣片                                                                 */
/************************************************************************/
Line::Line( const QPainterPath &path, QGraphicsScene *scene /*= 0*/ ) : scene_(scene)
{
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
	setAcceptHoverEvents(true);
	QGraphicsPathItem::setPath(path);
	line_ = path;
}

void Line::removeSeamLine( SeamLine *seamline )
{
	int index = seam_lines_.indexOf(seamline);

	if (index != -1)
		seam_lines_.removeAt(index);
}

void Line::removeSeamLines()
{
	foreach (SeamLine *seamline, seam_lines_)
	{
		seamline->startItem()->removeSeamLine(seamline);
		seamline->endItem()->removeSeamLine(seamline);
		scene()->removeItem(seamline);
		delete seamline;
	}
}

void Line::addSeamLine( SeamLine *seamline )
{
	seam_lines_.append(seamline);
}

void Line::contextMenuEvent( QGraphicsSceneContextMenuEvent *event )
{
	scene()->clearSelection();
	setSelected(true);
	context_menu_->exec(event->screenPos());
}

QVariant Line::itemChange( GraphicsItemChange change, const QVariant &value )
{
	if (change == QGraphicsItem::ItemPositionChange)
	{
		foreach (SeamLine *seamline, seam_lines_)
		{
			seamline->updatePosition();
		}
	}

	return value;
}

void Line::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
	setColor(QColor(255, 0, 0));
}

void Line::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
	setColor(QColor(0, 0, 0));
}

/************************************************************************/
/* 服装打板场景                                                          */
/************************************************************************/
PatternScene::PatternScene(/* QMenu *panelMenu, QMenu *seamlineMenu, */QObject *parent /*= 0*/ )
	: //panel_menu_(panelMenu), 
	//seamline_menu_(seamlineMenu), 
	QGraphicsScene(parent),
	mode_(MOVE_PATTERN),
	panel_color_(Qt::darkRed),
	seamline_color_(Qt::green),
	grid_visible_(true),
	dxf_importer_(new DXFImpoter(this)),
	dxf_file_(nullptr)
{
}

PatternScene::~PatternScene()
{
	 delete dxf_importer_;
	 delete dxf_file_;
}

void PatternScene::setPanelColor( const QColor &color )
{
	panel_color_ = color;;
	if (isItemChange(Line::Type))
	{
		Line *item = qgraphicsitem_cast<Line *>(selectedItems().first());
		item->setBrush(panel_color_);
	}
}

void PatternScene::setSeamlineColor( const QColor &color )
{
	seamline_color_ = color;;
	if (isItemChange(SeamLine::Type))
	{
		SeamLine *item = qgraphicsitem_cast<SeamLine *>(selectedItems().first());
		item->setColor(seamline_color_);
		update();
	}
}

void PatternScene::setMode( Mode mode )
{
	mode_ = mode;
}

void PatternScene::mousePressEvent( QGraphicsSceneMouseEvent *mouseEvent )
{
	QGraphicsScene::mousePressEvent(mouseEvent);
}

void PatternScene::mouseMoveEvent( QGraphicsSceneMouseEvent *mouseEvent )
{
	QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void PatternScene::mouseReleaseEvent( QGraphicsSceneMouseEvent *mouseEvent )
{
	QGraphicsScene::mouseReleaseEvent(mouseEvent);
}

void PatternScene::drawBackground( QPainter *painter, const QRectF &rect )
{
	// 绘制网格
	if (grid_visible_) 
	{
		const int grid_size = 10;
		const int MaxX = static_cast<int>(qCeil(width()) / grid_size) * grid_size;
		const int MaxY = static_cast<int>(qCeil(height()) / grid_size) * grid_size;
		QPen pen(QColor(175, 175, 175, 127));
		painter->setPen(pen);
		for (int x = 0; x <= MaxX; x += grid_size) 
		{
			painter->drawLine(x, 0, x, height());
		}
		for (int y = 0; y <= MaxY; y += grid_size) 
		{
			painter->drawLine(0, y, width(), y);
		}
	}
}

bool PatternScene::isItemChange( int type )
{
	foreach (QGraphicsItem *item, selectedItems()) 
	{
		if (item->type() == type)
			return true;
	}
	return false;
}

bool PatternScene::importPattern( const QString& filename )
{
	 delete dxf_file_;
	 dxf_file_ = new DL_Dxf();
	 dxf_importer_->clear();
	 if (!dxf_file_->in(filename.toStdString(), dxf_importer_)) 
	 {
		 qDebug() << "Failed to open" << filename;
		 return false;
	 }
	 dxf_importer_->addLastContour();
	 for (int i = 0; i < dxf_importer_->panel_contours_.size(); ++i)
	 {
		 Line* new_panel = new Line(dxf_importer_->panel_contours_[i], this);
		 addItem(new_panel);
		 panels_.push_back(new_panel);
	 }

	 return true;
}


/************************************************************************/
/* 服装打板视图                                                          */
/************************************************************************/

PatternView::PatternView( QWidget *parent /*= 0*/ ) : QGraphicsView(parent)
{
	setDragMode(RubberBandDrag);
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
	scale(1, -1); // make the y axis upward
}

void PatternView::wheelEvent( QWheelEvent *event )
{
	scaleBy(qPow(4.0 / 3.0, -event->delta() / 240.0));
}

void PatternView::scaleBy( double factor )
{
	scale(factor, factor);
}

