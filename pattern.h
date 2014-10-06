#ifndef PATTERNSCENE_H
#define PATTERNSCENE_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWheelEvent>
#include <QGraphicsPathItem>
#include <QList>
#include <QPainterPath>
#include "cad2d.h"
// dxflib headers
#include <dl_creationadapter.h>
#include <memory>

// forward declaration
class QGraphicsSceneMouseEvent;
class QMenu;
class QGraphicsLineItem;
class QColor;
class QGraphicsItem;
class QGraphicsScene;
class QGraphicsSceneContextMenuEvent;
class QStyleOptionGraphicsItem;

class DL_Dxf;
class Line;
class PatternScene;
struct Panel;

typedef std::tr1::shared_ptr<Panel> SmtPtrPanel;
typedef std::tr1::shared_ptr<Line> SmtPtrLine;
/************************************************************************/
/* DXF文件导入器                                                         */
/************************************************************************/
class DXFImpoter : public DL_CreationAdapter
{
    friend class PatternScene;
public:
    DXFImpoter(PatternScene* patternScene);

    virtual void addLayer(const DL_LayerData& data);
    virtual void addPoint(const DL_PointData& data);
    virtual void addLine(const DL_LineData& data);
    virtual void addArc(const DL_ArcData& data);
    virtual void addCircle(const DL_CircleData& data);
    virtual void addPolyline(const DL_PolylineData& data);
    virtual void addVertex(const DL_VertexData& data);
    virtual void add3dFace(const DL_3dFaceData& data);

    void clear();
    void printAttributes();
    void addLastContour();

	QList<SmtPtrPanel> panels() {return panels_;}

private:
    PatternScene* pattern_scene_;
    std::string previous_panel_;
    std::string current_panel_;
    bool new_panel_flag_;

	SmtPtrPanel temp_panel_;
    QPainterPath temp_contour_;
	QPainterPath temp_curve_;
    QPointF temp_point_;
    QList<QPainterPath> panel_contours_;
	QList<SmtPtrPanel> panels_;
    const float scale_factor_;
};
/************************************************************************/
/* 缝合线                                                               */
/************************************************************************/
class SeamLine : public QGraphicsLineItem
{
    friend class Line;
public:
    enum { Type = UserType + 2 };
    int type() const { return Type; }

    SeamLine(Line *startItem, Line *endItem, QMenu *contextMenu = nullptr);

    QColor color() const { return color_; }
    void setColor(const QColor &color);
    Line * startItem() const { return start_item_; }	
    Line * endItem() const { return end_item_; }

    void updatePosition();

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

private:
    Line*  start_item_;
    Line*  end_item_;
    QMenu*  context_menu_;
    QColor  color_;
};

class PatternScene;
/************************************************************************/
/* 衣片                                                                 */
/************************************************************************/
struct Panel
{
    QGraphicsPathItem contour_;
	QList<SmtPtrLine> lines_;
};

class Line : public QGraphicsPathItem
{
    friend class PatternScene;
public:
    enum { Type = UserType + 3 };
    int type() const { return Type; }

	Line(double x1, double y1, double x2, double y2);
    Line(const QPainterPath &path, QGraphicsScene *scene = 0);

    void addSeamLine(SeamLine *seamline);
    void removeSeamLine(SeamLine *seamline);
    void removeSeamLines();

    void setToolTip(const QString &toolTip) { tool_tip_ = toolTip; }
	void setColor(const QColor &color) { color_ = color;setPen(color); }

    QColor color() const { return color_; }
    QString toolTip() const { return tool_tip_; }


protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	void hoverEnterEvent(QGraphicsSceneHoverEvent * event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent * event);

private:
	Line(const Line & line){}

    QColor  color_;
    QString tool_tip_;
    QMenu*  context_menu_;
	QPainterPath line_;
	QGraphicsScene * scene_;
    QList<SeamLine*>        seam_lines_;
};

/************************************************************************/
/* 服装打板场景                                                          */
/************************************************************************/
class PatternScene : public QGraphicsScene
{
	Q_OBJECT

public:
	enum Mode { ADD_PATTERN, ADD_SEAMLINE, MOVE_PATTERN };

	PatternScene(/*QMenu *panelMenu, QMenu *seamlineMenu, */QObject *parent = 0);
    ~PatternScene();

	bool importPattern(const QString& filename);	// 导入服装打板
		
	bool gridVisible() const { return grid_visible_; }
	QColor panelColor() const { return panel_color_; }
	QColor seamlineColor() const { return seamline_color_; }
	void setPanelColor(const QColor &color);
	void setSeamlineColor(const QColor &color);
	void setGridVisible(bool val) { grid_visible_ = val; }

	void setMode(Mode mode);

	QList<SmtPtrPanel> getPanels(){return panels_;}

signals:
	void panelAdded(Line *item);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
	void drawBackground( QPainter *painter, const QRectF &rect );

private:
	bool isItemChange(int type);

	QMenu*	panel_menu_;
	QMenu*	seamline_menu_;
	Mode	mode_;
	QColor	panel_color_;
	QColor	seamline_color_;
	bool	grid_visible_;

	QList<SmtPtrPanel>	panels_;	    // 衣片
    DXFImpoter*     dxf_importer_;  // DXF导入器
    DL_Dxf*         dxf_file_;      // DXF文件
};

/************************************************************************/
/* 服装打板视图                                                          */
/************************************************************************/
class PatternView : public QGraphicsView
{
public:
	explicit PatternView(QWidget *parent = 0);

	void zoomIn() { scaleBy(1.1); }
	void zoomOut() { scaleBy(1.0/ 1.1); }

protected:
	void wheelEvent(QWheelEvent *event);

private:
	void scaleBy(double factor);
};
#endif // PATTERNSCENE_H