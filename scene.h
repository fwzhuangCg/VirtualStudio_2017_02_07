#ifndef SCENE_H
#define SCENE_H

#include <memory>

#include <QOpenGLBuffer>
#include <QOpenGLDebugLogger>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QColor>
#include <QStringList>
#include <QTreeWidget>

#include "abstractscene.h"
#include "material.h"
#include "animation.h"
#include "cloth.h"

class Camera;
class Light;
struct DecorativeObject;
class QOpenGLFunctions_4_0_Core;
class ClothHandler;
/************************************************************************/
/* 仿真场景                                                              */
/************************************************************************/
class SceneModel;
class Scene : public AbstractScene
{
	friend class SceneModel;
public:
	Scene( QObject* parent = 0 );
	~Scene();

	virtual void initialize();
	virtual void render();
	virtual void update(float t);
	virtual void resize( int w, int h );

	int						numClothes() const;
	Avatar*                 avatar();
	QString					avatarDiffuseTexPath();
	AnimationTableModel*    avatarAnimationTableModel();
	SkeletonTreeModel*		avatarSkeletonTreeModel();
	NameToAnimMap*          avatarNameAnimationMap();
	NameToChIdMap*          avatarNameChannelIndexMap();
	
	void importAvatar(const QString& filename);

	void updateAvatarAnimation(const Animation* anim, int frame);	// 更新avatar动画
	void restoreToBindpose();						                // 切换到绑定姿态

	void renderFloor() const;
	void renderAvatar() const;
	void renderClothes(QOpenGLShaderProgramPtr & shader) const;
	void renderSkeleton() const;

	bool pick(const QPoint& pt);    // 拾取场景中的物体

	enum InteractionMode 
	{ 
		SELECT,
		ROTATE, 
		TRANSLATE, 
		SCALE, 
		INTERACTION_MODE_COUNT 
	};

	enum DisplayMode 
	{ 
		SHADING, 
		SKELETON, 
		XRAY, 
		DISPLAY_MODE_COUNT
	};

	enum { JOINT_SPHERE_RADIUS = 10 }; // 用于拾取关节的包围球半径

	InteractionMode  interactionMode() const { return interaction_mode_; }
	void setInteractionMode( InteractionMode mode ) { interaction_mode_ = mode; }

	DisplayMode displayMode() const { return display_mode_; }
	void setDisplayMode( DisplayMode mode ) { display_mode_ = mode; }

	// Camera control
	void rotate(const QPoint& prevPos, const QPoint& curPos);
	void pan(float dx, float dy);
	void zoom(float factor);  

	// wnf添加，导入OBJ服装
	void importCloth(QString file_name);

private:
	// uncopyable
	Scene( const Scene& );
	Scene& operator=( const Scene& rhs );

	void scaleAvatar();
	void prepareFloor();    // 准备地板 灯光等舞台道具
	void prepareAvatar();   // 准备模特绘制数据
	void prepareSkeleton(); // 准备骨架绘制数据
	void prepareCloth();   // 准备服装绘制数据
	void reset_transform();

private:
	const aiScene*	ai_scene_;	// ASSIMP场景

	// 仿真场景对象
	Camera*			camera_;	// 相机
	QVector<Light*>	lights_;	// 光源
	Avatar*			avatar_;	// 模特
	QVector<Cloth*>	clothes_;	// 布料

	// 装饰性对象
	DecorativeObject* floor_;
	TexturePtr floor_tex_;
	SamplerPtr floor_sampler_;

	TexturePtr avatar_tex_;
	SamplerPtr avatar_sampler_;

	MaterialPtr	shading_display_material_;
	MaterialPtr simple_line_material_;

	QMatrix4x4	model_matrix_;

	DisplayMode			display_mode_;
	QStringList			display_mode_names_;
	QVector<GLuint>		display_mode_subroutines_;

	InteractionMode		interaction_mode_;
	QStringList			interaction_mode_names_;

	QOpenGLFunctions_4_0_Core*	glfunctions_;
	
	bool is_dual_quaternion_skinning_; // 是否采用双四元数
	bool is_joint_label_visible_; // 是否显示关节名称
	bool is_normal_visible_;

	// wnf添加，服装动画模拟功能模块
	typedef size_t ClothIndex;
	ClothHandler * cloth_handler_;
	QVector<QVector4D> color_;
	static const QVector4D ori_color_[4];
	ClothIndex cur_cloth_index_;
	ClothIndex hover_cloth_index_;
	float transform_[8];
};

// UI相关类
class SceneGraphTreeWidget : public QTreeWidget
{
public:
	SceneGraphTreeWidget(QWidget* parent = 0);
};

// 工程文件格式采用json格式
// *Simulation
// Scene
//	Avatar
//	Cloth
//	Light
//	Camera
//	Force Field (wind gravity)
// *Design
// Pattern

#endif // SCENE_H
