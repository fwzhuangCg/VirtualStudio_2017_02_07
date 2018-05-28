#ifndef OBJECT_BROWSER_WIDGET_H
#define OBJECT_BROWSER_WIDGET_H

#include <QTabWidget>

class QTabWidget;
class QTreeView;
class QTableView;

class ObjectBrowserWidget;
class SceneGraphTreeWidget;
/************************************************************************/
/* ���泡���������                                                                     */
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
/* ��װ�����Ƴ����������                                               */
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
/* ���������                                                            */
/************************************************************************/
// ���ڲ鿴 ���÷��������и������������
class ObjectBrowserWidget : public QTabWidget
{
public:
	explicit ObjectBrowserWidget(QWidget *parent = 0);

private:
	SimulationWidget*	simulation_widget_;
	DesignWidget*		design_widget_;
};
#endif // OBJECT_BROWSER_WIDGET_H