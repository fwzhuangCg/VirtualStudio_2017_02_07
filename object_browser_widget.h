#ifndef OBJECT_BROWSER_WIDGET_H
#define OBJECT_BROWSER_WIDGET_H

#include <QTabWidget>

class QTabWidget;
class QTreeView;
class QTableView;

class ObjectBrowserWidget;
class SceneGraphTreeWidget;
/************************************************************************/
/* 仿真场景浏览部件                                                                     */
/************************************************************************/
class SimulationWidget : QWidget
{
	friend ObjectBrowserWidget;

public:
	SimulationWidget(QWidget *parent = 0);

private:
	//QTreeView*	scene_tree_view_;
    SceneGraphTreeWidget* scene_tree_widget;
	QTableView*	property_table_view_;
};

/************************************************************************/
/* 服装打板设计场景浏览部件                                               */
/************************************************************************/
class DesignWidget : public QWidget
{
	friend ObjectBrowserWidget;

public:
	DesignWidget(QWidget *parent = 0);

private:
	QTreeView*	pattern_tree_view_;
	QTableView*	property_table_view_;
};

/************************************************************************/
/* 对象浏览器                                                            */
/************************************************************************/
// 用于查看 设置仿真和设计中各个对象的属性
class ObjectBrowserWidget : public QTabWidget
{
public:
	explicit ObjectBrowserWidget(QWidget *parent = 0);

private:
	SimulationWidget*	simulation_widget_;
	DesignWidget*		design_widget_;
};
#endif // OBJECT_BROWSER_WIDGET_H