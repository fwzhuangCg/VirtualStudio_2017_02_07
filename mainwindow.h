#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// Simulation classes
class Scene;
class AnimationClip;
class AnimationTrack;
// UI classes
class SimulationWindow;
class PatternView;
class PatternScene;
class ObjectBrowserWidget;
class AnimationEditorWidget;
class QDockWidget;
class QAction;
class QActionGroup;
class QMenu;
class QToolBar;
class QSplitter;
class QPrinter;
class QComboBox;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow() {}

private:
	void createViews();
	void createActions();
	void createMenusAndToolBars();
	void createDockWidgets();
	void createConnections();
	bool okToContinue();

private slots:
	void fileImportAvatar();	// 导入虚拟人物
	void fileImportPattern();	// 导入服装打板
	void toggleGridVisible();	// 显示/隐藏参考网格

	void switchInteractionMode();

	void renderingModeChanged(int);

	void exportAsVideo();
	void addSeamline();
	void generateCloth();

	bool save();
	void updateAnimation(int);
	void importMocap(QString& , QString&);

	// wnf添加，导入OBJ服装
	void fileImportCloth();
	void startSimulate();

private:
	// 仿真相关成员
	Scene* scene_;

	// UI相关成员
	QAction* file_open_action_;
	QAction* file_import_avatar_action_;
	QAction* file_import_pattern_action_;
	QAction* file_exit_action_;
	QAction* file_export_as_video_action_;
	QAction* simulation_select_action_;
	QActionGroup* shading_group_;
	QAction* simulation_shading_action_;
	QAction* simulation_xray_action_;
	QAction* simulation_skeleton_action_;
	QAction* design_showgrid_action_;
	QAction* design_add_seamline_action_;
	QAction* design_generate_cloth_action_;
	QAction* start_simulate_;
	
	// wnf添加，导入OBJ服装
	QAction* file_import_cloth_action_;

	QMenu* file_menu_;
	QMenu* window_menu_;
	QMenu* simulation_menu_;
	QMenu* design_menu_;

	QComboBox* rendering_mode_combo_;

	QToolBar* file_tool_bar_;
	QToolBar* simulation_tool_bar_;
	QToolBar* design_tool_bar_;

	SimulationWindow*		simulation_view_;
	PatternView*			design_view_;
	PatternScene*			pattern_scene_;
	QSplitter*				splitter_;
	ObjectBrowserWidget*	object_browser_;
	AnimationEditorWidget*	animation_editor_;
	QDockWidget*			object_browser_dock_widget_;
	QDockWidget*			animation_editor_dock_widget_;
	QPrinter*				printer_;
};

#endif // MAINWINDOW_H