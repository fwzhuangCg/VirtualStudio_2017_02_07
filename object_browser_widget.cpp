#include "object_browser_widget.h"

#include <QtGui>
#include <QtWidgets/QtWidgets>

#include "scene.h"

SimulationWidget::SimulationWidget( QWidget *parent /*= 0*/ )
    : QWidget(parent)
{
// 	scene_tree_view_ = new QTreeView(this);
// 	scene_tree_view_->setMinimumHeight(400);
    scene_tree_widget = new SceneGraphTreeWidget(this);
    scene_tree_widget->setMinimumHeight(400);

	property_table_view_ = new QTableView(this);

	QSplitter* splitter = new QSplitter(Qt::Vertical, this);
	splitter->addWidget(scene_tree_widget); //scene_tree_view_
	splitter->addWidget(property_table_view_);
	splitter->setStretchFactor(1, 1);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(splitter);
	setLayout(layout);	
}

DesignWidget::DesignWidget( QWidget *parent /*= 0*/ )
    : QWidget(parent)
{
	pattern_tree_view_ = new QTreeView(this);
	pattern_tree_view_->setMinimumHeight(400);

	property_table_view_ = new QTableView(this);

	QSplitter* splitter = new QSplitter(Qt::Vertical, this);
	splitter->addWidget(pattern_tree_view_);
	splitter->addWidget(property_table_view_);
	splitter->setStretchFactor(1, 1);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(splitter);
	setLayout(layout);	
}

ObjectBrowserWidget::ObjectBrowserWidget( QWidget *parent /*= 0*/ )
	: QTabWidget(parent)
{
	simulation_widget_ = new SimulationWidget(this);
	design_widget_ = new DesignWidget(this);

	addTab(simulation_widget_, tr("Simulation"));
	addTab(design_widget_, tr("Design"));

	setMinimumWidth(200);
	setMaximumWidth(300);
}
