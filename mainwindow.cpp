#include "mainwindow.h"

#include <QtGui>
#include <QWidget>
#include <QPrinter>
#include <QSplitter>
#include <QWindow>

// simulation
#include "scene.h"

// GUI
#include "simulation_window.h"
#include "pattern.h"
#include "object_browser_widget.h"
#include "animation_editor_widget.h"

MainWindow::MainWindow( QWidget *parent )
	: QMainWindow(parent), 
    scene_(new Scene())
{
	printer_ = new QPrinter(QPrinter::HighResolution);

	createViews();
	createActions();
	createMenusAndToolBars();
	createDockWidgets();
	createConnections();

	setWindowIcon(QIcon(":images/clothes.png"));
} 

void MainWindow::createViews() 
{
	simulation_view_ = new SimulationWindow(scene_);
	
	pattern_scene_ = new PatternScene(this);
	QSize page_size = printer_->paperSize(QPrinter::Point).toSize();
	pattern_scene_->setSceneRect(0, 0, page_size.width(), page_size.height());
	design_view_ = new PatternView(this);
	design_view_->setScene(pattern_scene_);
    design_view_->setSceneRect(pattern_scene_->sceneRect());

	splitter_ = new QSplitter(Qt::Horizontal);
	QWidget* container = createWindowContainer(simulation_view_, this);
	container->setMinimumSize(simulation_view_->size());
	container->resize(500, 300);
	
	splitter_->addWidget(container);
	splitter_->addWidget(design_view_);
	splitter_->setStretchFactor(1, 1);
	setCentralWidget(splitter_);
}

void MainWindow::createActions()
{
	file_open_action_ = new QAction(QIcon(":images/open.png"), tr("Open..."), this);
	file_open_action_->setStatusTip(tr("Open project"));
	file_open_action_->setToolTip(tr("Open VirtualStudio project file"));
	file_open_action_->setShortcut(QKeySequence::Open);

	file_import_avatar_action_ = new QAction(QIcon(":images/import_avatar.png"), tr("Import avatar"), this);
	file_import_avatar_action_->setStatusTip(tr("Import avatar"));
	file_import_avatar_action_->setToolTip(tr("Import avatar into the simulation"));

	file_import_pattern_action_ = new QAction(QIcon(":images/import_pattern.png"), tr("Import pattern"), this);
	file_import_pattern_action_->setStatusTip(tr("Import pattern"));
	file_import_pattern_action_->setToolTip(tr("Import pattern into the canvas"));

	file_import_cloth_action_ = new QAction(QIcon(":images/clothes.png"), tr("Import cloth"), this);
	file_import_cloth_action_->setStatusTip(tr("Import cloth"));
	file_import_cloth_action_->setToolTip(tr("Import cloth into the simulation"));

    file_export_as_video_action_ = new QAction(QIcon(":images/export_as_video.png"), tr("Export as video"), this);
    file_export_as_video_action_->setStatusTip(tr("Export as video"));
    file_export_as_video_action_->setToolTip(tr("Export as video"));

	file_exit_action_ = new QAction(QIcon(":images/exit.png"), tr("Exit"), this);
	file_exit_action_->setStatusTip(tr("Quit"));
	file_exit_action_->setToolTip(tr("Quit the application"));
	file_exit_action_->setShortcut(QKeySequence::Quit);

    simulation_select_action_ = new QAction(QIcon(":images/select.png"), tr("Select"), this);
    simulation_select_action_->setStatusTip(tr("Click to select"));
    simulation_select_action_->setToolTip(tr("Select object"));
    simulation_select_action_->setCheckable(true);

	simulation_shading_action_ = new QAction(QIcon(":images/shading.png"), tr("Shading"), this);
	simulation_shading_action_->setStatusTip(tr("View the avatar"));
	simulation_shading_action_->setToolTip(tr("Show avatar in shading"));

	simulation_skeleton_action_ = new QAction(QIcon(":images/skeleton.png"), tr("Skeleton"), this);
	simulation_skeleton_action_->setStatusTip(tr("View the skeleton"));
	simulation_skeleton_action_->setToolTip(tr("Show avatar's skeleton only"));

	simulation_xray_action_ = new QAction(QIcon(":images/x_ray.png"), tr("X-Ray"), this);
	simulation_xray_action_->setStatusTip(tr("View the avatar with skeleton"));
	simulation_xray_action_->setToolTip(tr("Show avatar in x-ray style (see-through skeleton)"));

	shading_group_ = new QActionGroup(this);
	shading_group_->addAction(simulation_shading_action_);
	shading_group_->addAction(simulation_xray_action_);
	shading_group_->addAction(simulation_skeleton_action_);

	design_showgrid_action_ = new QAction(QIcon(":images/showgrid.png"), tr("Show/Hide grid"), this);
	design_showgrid_action_->setCheckable(true);
	design_showgrid_action_->setChecked(pattern_scene_->gridVisible());
	design_showgrid_action_->setStatusTip(tr("Show/Hide the background grid"));
	design_showgrid_action_->setToolTip(tr("Show/Hide the background grid"));

    design_add_seamline_action_ = new QAction(QIcon(":images/add_seamline.png"), tr("Add seamline"), this);
    design_showgrid_action_->setStatusTip(tr("Add seamline"));
    design_showgrid_action_->setToolTip(tr("Add seamline"));

    design_generate_cloth_action_ = new QAction(QIcon(":images/generate_cloth.png"), tr("Generate cloth"), this);
    design_showgrid_action_->setStatusTip(tr("Generate cloth"));
    design_showgrid_action_->setToolTip(tr("Generate cloth"));
}

void MainWindow::createMenusAndToolBars()
{
	file_menu_ = menuBar()->addMenu(tr("&File"));
	file_menu_->addAction(file_open_action_);
	file_menu_->addAction(file_import_avatar_action_);
	file_menu_->addAction(file_import_pattern_action_);
	file_menu_->addAction(file_import_cloth_action_);
    file_menu_->addAction(file_export_as_video_action_);
	file_menu_->addSeparator();
	file_menu_->addAction(file_exit_action_);

	window_menu_ = menuBar()->addMenu(tr("&Window"));

	simulation_menu_ = menuBar()->addMenu(tr("&Simulation"));
    simulation_menu_->addAction(simulation_select_action_);
	simulation_menu_->addSeparator();
	QMenu* display_mode_menu = new QMenu(tr("Display mode"));
	display_mode_menu->addAction(simulation_shading_action_);
	display_mode_menu->addAction(simulation_skeleton_action_);
	display_mode_menu->addAction(simulation_xray_action_);
	simulation_menu_->addMenu(display_mode_menu);

	design_menu_ = menuBar()->addMenu(tr("&Design"));
	design_menu_->addAction(design_showgrid_action_);
    design_menu_->addAction(design_add_seamline_action_);
    design_menu_->addAction(design_generate_cloth_action_);

	file_tool_bar_ = addToolBar(tr("&File"));
	file_tool_bar_->addAction(file_open_action_);
	file_tool_bar_->addAction(file_import_avatar_action_);
	file_tool_bar_->addAction(file_import_pattern_action_);
	file_tool_bar_->addAction(file_import_cloth_action_);
    file_tool_bar_->addAction(file_export_as_video_action_);

	simulation_tool_bar_ = addToolBar(tr("&Simulation"));
    simulation_tool_bar_->addAction(simulation_select_action_);

	rendering_mode_combo_ = new QComboBox;
	rendering_mode_combo_->addItem(simulation_shading_action_->icon(), simulation_shading_action_->text());
	rendering_mode_combo_->addItem(simulation_skeleton_action_->icon(), simulation_skeleton_action_->text());
	rendering_mode_combo_->addItem(simulation_xray_action_->icon(), simulation_xray_action_->text());
	simulation_tool_bar_->addWidget(rendering_mode_combo_);

	design_tool_bar_ = addToolBar(tr("&Design"));
	design_tool_bar_->addAction(design_showgrid_action_);
    design_tool_bar_->addAction(design_add_seamline_action_);
    design_tool_bar_->addAction(design_generate_cloth_action_);
}		

void MainWindow::createDockWidgets()
{
	setDockOptions(QMainWindow::AnimatedDocks);
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable;

	object_browser_ = new ObjectBrowserWidget(this);
	object_browser_dock_widget_ = new QDockWidget(tr("Object Browser"), this);
	object_browser_dock_widget_->setFeatures(features);
	object_browser_dock_widget_->setWidget(object_browser_);
	addDockWidget(Qt::RightDockWidgetArea, object_browser_dock_widget_);

	animation_editor_ = new AnimationEditorWidget(this);
	animation_editor_dock_widget_ = new QDockWidget(tr("Animation Editor"), this);
	animation_editor_dock_widget_->setFeatures(features);
	animation_editor_dock_widget_->setWidget(animation_editor_);
	addDockWidget(Qt::BottomDockWidgetArea, animation_editor_dock_widget_);

	window_menu_->addAction(object_browser_dock_widget_->toggleViewAction());
	window_menu_->addAction(animation_editor_dock_widget_->toggleViewAction());
}

void MainWindow::createConnections()
{
	connect(file_import_avatar_action_, SIGNAL(triggered()), this, SLOT(fileImportAvatar()));
	connect(file_import_pattern_action_, SIGNAL(triggered()), this, SLOT(fileImportPattern()));
	connect(design_showgrid_action_, SIGNAL(triggered()), this, SLOT(toggleGridVisible()));

	// interaction mode
	connect(rendering_mode_combo_, SIGNAL(currentIndexChanged(int)), this, SLOT(renderingModeChanged(int)));

	connect(simulation_select_action_, SIGNAL(triggered()), this, SLOT(switchInteractionMode()));

    connect(animation_editor_, SIGNAL(frameChanged(int)), this, SLOT(updateAnimation(int)));
	connect(animation_editor_, SIGNAL(bindposeRestored()), simulation_view_, SLOT(restoreToBindpose()));
    connect(animation_editor_, SIGNAL(mocapSelected(QString& , QString&)), this, SLOT(importMocap(QString& , QString&)));
}

bool MainWindow::okToContinue()
{
	if (isWindowModified()) 
    {
		int r = QMessageBox::warning(this, tr("VirtualStudio"),
			tr("The document has been modified.\nDo you want to save your changes?"),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if (r == QMessageBox::Yes) 
        {
			return save();
		} 
        else if (r == QMessageBox::Cancel)
        {
			return false;
		}
	}
	return true;
}

void MainWindow::fileImportAvatar()
{
	if (okToContinue()) 
    {
		QString file_name = QFileDialog::getOpenFileName(this, tr("Import Avatar"),  ".", tr("Avatar files (*.dae; *.x)"));

		if (!file_name.isEmpty()) 
        {
			scene_->importAvatar(file_name);
			simulation_view_->paintGL();
            // glue the avatar and editor together
            //scene_->setSyntheticAnimation(animation_editor_->syntheticAnimation());
            //animation_editor_->setAvatar(scene_->avatar());
            animation_editor_->reset();
			animation_editor_->setAnimationTableModel(scene_->avatarAnimationTableModel());
            animation_editor_->setNameAnimationMap(scene_->avatarNameAnimationMap());
            animation_editor_->setNameChannelIndexMap(scene_->avatarNameChannelIndexMap());
			animation_editor_->setSkeletonTreeModel(scene_->avatarSkeletonTreeModel());	
		}
	}
}

void MainWindow::fileImportPattern()
{
	if (okToContinue()) {
		QString file_name = QFileDialog::getOpenFileName(this, tr("Import Pattern"),  ".", tr("Pattern files (*.dxf)"));

		if (!file_name.isEmpty()) {
			pattern_scene_->importPattern(file_name);
			pattern_scene_->update();
		}
	}
}

bool MainWindow::save()
{
	return true;
}

void MainWindow::updateAnimation(int frame)
{
    Q_ASSERT(simulation_view_ && animation_editor_);
    simulation_view_->updateAnimation(animation_editor_->syntheticAnimation(), frame);
}

void MainWindow::importMocap(QString& asf_file, QString& amc_file)
{
    if (scene_->avatar())
    {
        scene_->avatar()->importMocap(asf_file, amc_file);
        QMessageBox::information(this, "Mocap import", 
            QString("Mocap data from file %1 and %2!").arg(asf_file).arg(amc_file), QMessageBox::Ok);
    }
    else
    {
        QMessageBox::critical(this, "Failed to import %1 and %2", QString("").arg(asf_file).arg(amc_file), QMessageBox::Ok);
    }
}

void MainWindow::toggleGridVisible()
{
	pattern_scene_->setGridVisible(design_showgrid_action_->isChecked());
	pattern_scene_->update();
}

void MainWindow::switchInteractionMode()
{
    if (simulation_select_action_->isChecked())
    {
        scene_->setInteractionMode(Scene::SELECT);
    }
    else
    {
        scene_->setInteractionMode(Scene::ROTATE);
    }
}

void MainWindow::renderingModeChanged( int index)
{
	switch (index)
	{
	case 0:
		scene_->setDisplayMode(Scene::SHADING);
		break;
	case 1:
		scene_->setDisplayMode(Scene::SKELETON);
		break;
	case 2:
		scene_->setDisplayMode(Scene::XRAY);
		break;
	}
    simulation_view_->paintGL();
}

void MainWindow::exportAsVideo()
{

}

void MainWindow::addSeamline()
{

}

void MainWindow::generateCloth()
{

}

void MainWindow::fileImportCloth()
{
	if (okToContinue()) {
		QString file_name = QFileDialog::getOpenFileName(this, tr("Import Cloth"),  ".", tr("OBJ files (*.obj)"));

		if (!file_name.isEmpty()) {
			scene_->importCloth(file_name);
			simulation_view_->paintGL();
		}
	}
}