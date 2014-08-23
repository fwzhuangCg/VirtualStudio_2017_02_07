#define NOMINMAX // for the stupid name pollution, you can also use #undef max
#include "scene.h"

#include <limits>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <QString>
#include <QImage>
#include <QtOpenGL/QGLWidget>
#include <QOpenGLContext>
#include <QOpenGLFunctions_4_0_Core>

#include "camera.h"
#include "light.h"
#include "gadget.h"
#include "bounding_volume.h"
/************************************************************************/
/* 仿真场景                                                              */
/************************************************************************/
Scene::Scene( QObject* parent ) 
	: ai_scene_( nullptr ), 
	  camera_( new Camera ),
      floor_(nullptr),
	  avatar_( nullptr ),
	  display_mode_(SHADING),
	  display_mode_subroutines_(DISPLAY_MODE_COUNT),
	  interaction_mode_(ROTATE),
	  glfunctions_(nullptr),
      floor_sampler_(new Sampler),
      avatar_sampler_(new Sampler),
      avatar_tex_(new Texture),
      floor_tex_(new Texture),
      is_dual_quaternion_skinning_(true),
      is_joint_label_visible_(false)
{
	model_matrix_.setToIdentity();

	// Initialize the camera position and orientation
	display_mode_names_ << QStringLiteral( "shade" )
						<< QStringLiteral( "shadeWithNoMaterial" )
						<< QStringLiteral( "shadingwireframe" );

	interaction_mode_names_ << QStringLiteral( "rotate" )
							<< QStringLiteral( "pan" )
							<< QStringLiteral( "zoom" )
							<< QStringLiteral( "select" );
}

Scene::~Scene()
{
	for (int i = 0; i < lights_.size(); ++i)
		delete lights_[i];

	delete avatar_;

	for (int i = 0; i < clothes_.size(); ++i)
		delete clothes_[i];

	aiReleaseImport(ai_scene_);
}

void Scene::initialize()
{
	glfunctions_ = m_context->versionFunctions<QOpenGLFunctions_4_0_Core>();
	if ( !glfunctions_ ) 
    {
		qFatal("Requires OpenGL >= 4.0");
		exit(-1);
	}
	glfunctions_->initializeOpenGLFunctions();

	// Initialize resources
    shading_display_material_ = MaterialPtr(new Material);
    shading_display_material_->setShaders(":/shaders/skinning.vert", ":/shaders/phong.frag");
    simple_line_material_ = MaterialPtr(new Material);
    simple_line_material_->setShaders(":/shaders/simple.vert", ":/shaders/simple.frag");

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glClearColor( 0.65f, 0.77f, 1.0f, 1.0f );

    // 默认为shading显示
	QOpenGLShaderProgramPtr shader = shading_display_material_->shader();

	// Get subroutine indices DISPLAY_MODE_COUNT
 	for ( int i = 0; i < 2; ++i) 
    {
 		display_mode_subroutines_[i] = 
            glfunctions_->glGetSubroutineIndex( shader->programId(), GL_FRAGMENT_SHADER, display_mode_names_.at( i ).toLatin1() );
 	}

    // prepare floor rendering data
    prepareFloor();

    // position the cemara
    camera_->setEye(10, 10, 10);
    camera_->setCenter(0, 0, 0);
}

void Scene::render()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    //model_matrix_.setToIdentity();
    QMatrix4x4 view_matrix = camera_->viewMatrix();
    QMatrix4x4 mv = view_matrix * model_matrix_;
    QMatrix4x4 projection_matrix = camera_->projectionMatrix();
    QMatrix4x4 MVP = projection_matrix * mv;

    shading_display_material_->bind();
    QOpenGLShaderProgramPtr shading_display_shader = shading_display_material_->shader();
    shading_display_shader->bind();

    // Set the fragment shader display mode subroutine
    glfunctions_->glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &display_mode_subroutines_[display_mode_]);

    // Pass in the usual transformation matrices
    shading_display_shader->setUniformValue("ModelViewMatrix", mv);
    shading_display_shader->setUniformValue("NormalMatrix", mv.normalMatrix());
    shading_display_shader->setUniformValue("MVP", MVP);  
    shading_display_shader->setUniformValue("Skinning", false);
    shading_display_shader->setUniformValue("DQSkinning", false);
    shading_display_shader->setUniformValue("Light.Position", view_matrix * QVector4D(1.0f, 1.0f, 1.0f, 0.0f));
    shading_display_shader->setUniformValue("Light.Intensity", QVector3D(0.5f, 0.5f, 0.5f));
    shading_display_shader->setUniformValue("Material.Ka", QVector3D( 0.1f, 0.1f, 0.1f ));
    shading_display_shader->setUniformValue("Material.Kd", QVector3D( 0.9f, 0.9f, 0.9f ));
    shading_display_shader->setUniformValue("Material.Ks", QVector3D( 0.9f, 0.9f, 0.9f ));
    shading_display_shader->setUniformValue("Material.Shininess", 5.0f);

    renderFloor();

    is_dual_quaternion_skinning_ = false;

    switch (display_mode_)
    {
    case SHADING:
        {
            if (avatar_ && !avatar_->bindposed_) // 已废除软件蒙皮 完全采用GPU蒙皮
            {
                shading_display_shader->setUniformValueArray("JointMatrices", avatar_->joint_matrices_.data(), avatar_->joint_matrices_.count());
                shading_display_shader->setUniformValueArray("JointDQs", avatar_->joint_dual_quaternions_.data(), avatar_->joint_dual_quaternions_.count());
                shading_display_shader->setUniformValue("Skinning", true);
                shading_display_shader->setUniformValue("DQSkinning", is_dual_quaternion_skinning_);
            }

            renderAvatar();

            shading_display_shader->release();
        }
        break;
    case SKELETON:
        {
            simple_line_material_->bind();
            QOpenGLShaderProgramPtr simple_line_shader = simple_line_material_->shader();
            simple_line_shader->bind();
            simple_line_shader->setUniformValue("MVP", MVP);

            renderSkeleton();

            simple_line_shader->release();
        }
        break;
    case XRAY:
        {
            if (avatar_ && !avatar_->bindposed_)
            {
                shading_display_shader->setUniformValueArray("JointMatrices", avatar_->joint_matrices_.data(), avatar_->joint_matrices_.count());
                shading_display_shader->setUniformValueArray("JointDQs", avatar_->joint_dual_quaternions_.data(), avatar_->joint_dual_quaternions_.count());
                shading_display_shader->setUniformValue("Skinning", true);
                shading_display_shader->setUniformValue("DQSkinning", is_dual_quaternion_skinning_);
            }
            renderAvatar();

            shading_display_shader->release();
        }

        glDisable(GL_DEPTH_TEST);
        {
            simple_line_material_->bind();
            QOpenGLShaderProgramPtr simple_line_shader = simple_line_material_->shader();
            simple_line_shader->bind();
            simple_line_shader->setUniformValue("MVP", MVP);

            renderSkeleton();

            simple_line_shader->release();
        }
        glEnable(GL_DEPTH_TEST);
        break;
    }
}

void Scene::update(float t)
{
}

void Scene::resize( int width, int height )
{
	glViewport( 0, 0, width, height );
 	camera_->setViewportWidth(width);
 	camera_->setViewportHeight(height);
}

int Scene::numClothes() const
{
    return clothes_.size();
}

QString Scene::avatarDiffuseTexPath()
{
    Q_ASSERT(avatar_);
    return avatar_->diffuse_tex_path_;
}

NameToAnimMap* Scene::avatarNameAnimationMap()
{
    Q_ASSERT(avatar_);
    return &avatar_->name_animation_;
}

NameToChIdMap* Scene::avatarNameChannelIndexMap()
{
    Q_ASSERT(avatar_);
    return &avatar_->name_channelindex_;
}

void Scene::importAvatar( const QString& filename )
{
	ai_scene_ = aiImportFile(filename.toStdString().c_str(),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights); // 三角化 法线平滑 限制每个关节最多4个权重
	delete avatar_;
	avatar_ = new Avatar(ai_scene_, filename);
    //scaleAvatar();
	prepareAvatar();
    prepareSkeleton();
    Sphere s(avatar_->bounding_aabb_);
    camera_->fitBoundingSphere(s);
	aiReleaseImport(ai_scene_);
}

void Scene::renderFloor() const
{
    if (!floor_)
        return;
    //shading_display_material_->setTextureUnitConfiguration(0, floor_tex_, floor_sampler_, QByteArrayLiteral("Tex1"));
    floor_tex_->bind();
    //glfunctions_->glBindTexture(GL_TEXTURE_2D, floor_tex_->textureId());
    QOpenGLVertexArrayObject::Binder binder( floor_->vao );
    glDrawElements(GL_TRIANGLES, floor_->indices.size(), GL_UNSIGNED_INT, floor_->indices.constData());
}

void Scene::renderAvatar() const
{
	if (!avatar_)
		return;
    //shading_display_material_->setTextureUnitConfiguration(0, avatar_tex_, avatar_sampler_, QByteArrayLiteral("Tex1"));
    avatar_tex_->bind();
    //glfunctions_->glBindTexture(GL_TEXTURE_2D, avatar_tex_->textureId());
    for (auto skin_it = avatar_->skins().begin(); skin_it != avatar_->skins().end(); ++skin_it) 
    {
        QOpenGLVertexArrayObject::Binder binder( skin_it->vao_ );
        glDrawElements(GL_TRIANGLES, skin_it->indices.size(), GL_UNSIGNED_INT, skin_it->indices.constData());
    }
}

void Scene::renderClothes() const
{

}

void Scene::renderSkeleton() const
{
    if (!avatar_)
        return;
    // draw joints and bones
    glPointSize(5.0f);
    QOpenGLVertexArrayObject::Binder joint_binder(avatar_->joint_vao_);
    glDrawArrays(GL_POINTS, 0, avatar_->joint_positions_.size());
    glPointSize(1.0f);

    glLineWidth(2.0f);
    QOpenGLVertexArrayObject::Binder bone_binder(avatar_->bone_vao_);        
    glDrawArrays(GL_LINES, 0, avatar_->bone_positions_.size());
    glLineWidth(1.0f);
}

Avatar* Scene::avatar()
{
    return avatar_;
}

AnimationTableModel* Scene::avatarAnimationTableModel()
{
	if (avatar_ && avatar_->hasAnimations())
		return avatar_->animation_table_model_;
	return nullptr;
}

SkeletonTreeModel* Scene::avatarSkeletonTreeModel()
{
	if (avatar_ )
		return avatar_->skeleton_tree_model_;
	return nullptr;
}

void Scene::updateAvatarAnimation(const Animation* anim, int frame)
{
    avatar_->setBindposed(false);
    if (anim) 
    {	
        avatar_->updateAnimation(anim, frame * AnimationClip::SAMPLE_SLICE);
    }
}

void Scene::restoreToBindpose()
{
	avatar_->setBindposed(true);
}

void Scene::scaleAvatar()
{
    Sphere bounding_sphere(avatar_->bounding_aabb_);

    QVector3D vHalf =  bounding_sphere.c;
    float fScale = 10.0f / bounding_sphere.r;

    model_matrix_.setToIdentity();
    model_matrix_.translate(vHalf);
    model_matrix_.scale(fScale);
}

void Scene::prepareAvatar()
{
    // 准备蒙皮VAO
	for (auto skin_it = avatar_->skins_.begin(); skin_it != avatar_->skins_.end(); ++skin_it) 
    {
		skin_it->vao_ = new QOpenGLVertexArrayObject(this);
		skin_it->vao_->create();
		QOpenGLVertexArrayObject::Binder binder( skin_it->vao_ );
		QOpenGLShaderProgramPtr shading_display_shader = shading_display_material_->shader();
		shading_display_shader->bind();

		skin_it->position_buffer_->bind();
		shading_display_shader->enableAttributeArray( "VertexPosition" );
		shading_display_shader->setAttributeBuffer( "VertexPosition", GL_FLOAT, 0, 3 );	

		skin_it->normal_buffer_->bind();
		shading_display_shader->enableAttributeArray( "VertexNormal" );
		shading_display_shader->setAttributeBuffer( "VertexNormal", GL_FLOAT, 0, 3 );	

		if (avatar_->hasMaterials()) 
        {
			skin_it->texcoords_buffer_->bind();
			shading_display_shader->enableAttributeArray( "VertexTexCoord" );
			shading_display_shader->setAttributeBuffer( "VertexTexCoord", GL_FLOAT, 0, 2 );
		}

        // 关节索引和关节权重
        skin_it->joint_indices_buffer_->bind();
        shading_display_shader->enableAttributeArray("JointIndices");
        shading_display_shader->setAttributeBuffer("JointIndices", GL_FLOAT, 0, 4);

        skin_it->joint_weights_buffer_->bind();
        shading_display_shader->enableAttributeArray("JointWeights");
        shading_display_shader->setAttributeBuffer("JointWeights", GL_FLOAT, 0, 4);
	}

    // 准备纹理数据
    avatar_sampler_->create();
    avatar_sampler_->setMinificationFilter( GL_LINEAR );
    avatar_sampler_->setMagnificationFilter( GL_LINEAR );
    avatar_sampler_->setWrapMode( Sampler::DirectionS, GL_REPEAT );//GL_CLAMP_TO_EDGE
    avatar_sampler_->setWrapMode( Sampler::DirectionT, GL_REPEAT );//GL_CLAMP_TO_EDGE
    QImage avatarImage(avatar_->diffuse_tex_path_);//
    glfunctions_->glActiveTexture(GL_TEXTURE0);

    avatar_tex_->create();
    avatar_tex_->bind();
    avatar_tex_->setImage(avatarImage);
    shading_display_material_->setTextureUnitConfiguration(0, avatar_tex_, avatar_sampler_, QByteArrayLiteral("Tex1"));
    glfunctions_->glActiveTexture( GL_TEXTURE0 );
}

void Scene::prepareSkeleton()
{
    // 准备关节
    avatar_->joint_vao_ = new QOpenGLVertexArrayObject(this);
    avatar_->joint_vao_->create();
    QOpenGLVertexArrayObject::Binder joint_binder(avatar_->joint_vao_);
    QOpenGLShaderProgramPtr simple_line_shader = simple_line_material_->shader();
    simple_line_shader->bind();

    avatar_->joint_pos_buffer_->bind();
    simple_line_shader->enableAttributeArray("VertexPosition");
    simple_line_shader->setAttributeBuffer( "VertexPosition", GL_FLOAT, 0, 4 );	

    // 准备骨架
    avatar_->bone_vao_ = new QOpenGLVertexArrayObject(this);
    avatar_->bone_vao_->create();
    QOpenGLVertexArrayObject::Binder bone_binder(avatar_->bone_vao_);
    simple_line_shader->bind();

    avatar_->bone_pos_buffer_->bind();
    simple_line_shader->enableAttributeArray("VertexPosition");
    simple_line_shader->setAttributeBuffer( "VertexPosition", GL_FLOAT, 0, 4 );	
}

void Scene::prepareFloor()
{
    floor_ = new DecorativeObject();
    float s = 100.f;
    float rep = 100.f;
    float h = 0.0f;
    floor_->positions.append(QVector3D(-s, h, -s)); floor_->normals.append(QVector3D(0.0, 1.0, 0.0)); floor_->texcoords.append(QVector2D(0.f, 0.f));
    floor_->positions.append(QVector3D(-s, h, s)); floor_->normals.append(QVector3D(0.0, 1.0, 0.0)); floor_->texcoords.append(QVector2D(rep, 0.f));
    floor_->positions.append(QVector3D(s, h, s)); floor_->normals.append(QVector3D(0.0, 1.0, 0.0)); floor_->texcoords.append(QVector2D(rep, rep)); 
    floor_->positions.append(QVector3D(s, h, -s)); floor_->normals.append(QVector3D(0.0, 1.0, 0.0)); floor_->texcoords.append(QVector2D(0.f, rep));
    floor_->indices.append(0);
    floor_->indices.append(1);
    floor_->indices.append(2);
    floor_->indices.append(2);
    floor_->indices.append(3);
    floor_->indices.append(0);

    if (!floor_->createVBO())
    {
        qWarning() << "Failed to create floor";
        return;
    }
    // 创建地板VAO
    floor_->vao = new QOpenGLVertexArrayObject(this);
    floor_->vao->create();
    QOpenGLVertexArrayObject::Binder binder(floor_->vao);
    QOpenGLShaderProgramPtr shading_display_shader = shading_display_material_->shader();
    shading_display_shader->bind();

    floor_->pb->bind();
    shading_display_shader->enableAttributeArray("VertexPosition");
    shading_display_shader->setAttributeBuffer( "VertexPosition", GL_FLOAT, 0, 3 );	

    floor_->nb->bind();
    shading_display_shader->enableAttributeArray( "VertexNormal" );
    shading_display_shader->setAttributeBuffer( "VertexNormal", GL_FLOAT, 0, 3 );	

    floor_->tb->bind();
    shading_display_shader->enableAttributeArray( "VertexTexCoord" );
    shading_display_shader->setAttributeBuffer( "VertexTexCoord", GL_FLOAT, 0, 2 );

    // 加载地板纹理
    floor_sampler_->create();
    floor_sampler_->setMinificationFilter(GL_LINEAR);
    floor_sampler_->setMagnificationFilter(GL_LINEAR);
    floor_sampler_->generateMipmap(GL_TEXTURE_2D);
    floor_sampler_->setWrapMode(Sampler::DirectionS, GL_REPEAT);
    floor_sampler_->setWrapMode(Sampler::DirectionT, GL_REPEAT);
    QImage floorImage(":/images/floortile.ppm");
    glfunctions_->glActiveTexture(GL_TEXTURE0);

    floor_tex_->create();
    floor_tex_->bind();
    floor_tex_->setImage(floorImage);
    shading_display_material_->setTextureUnitConfiguration(0, floor_tex_, floor_sampler_, QByteArrayLiteral("Tex1"));
    glfunctions_->glActiveTexture( GL_TEXTURE0 );
}

void Scene::rotate( const QPoint& prevPos, const QPoint& curPos )
{
    camera_->rotate(prevPos, curPos);
}

void Scene::pan( float dx, float dy )
{
    camera_->pan(dx, dy);
}

void Scene::zoom( float factor )
{
    camera_->zoom(factor);
}

// pt : mouse position
bool Scene::pick(const QPoint& pt)
{
    if (!avatar_)
        return false;
    // Get the Pick ray from the mouse position
    // Compute the vector of the Pick ray in screen space
    QMatrix4x4 matProj = camera_->projectionMatrix();

    // Screen to projection window transformation 推导过程详见龙书Picking章节
    QVector3D vPickRayDir;
    vPickRayDir.setX(  ( ( ( 2.0f * pt.x() ) / camera_->viewportWidth()  ) - 1 ) / matProj(0, 0));
    vPickRayDir.setY( -( ( ( 2.0f * pt.y() ) / camera_->viewportHeight() ) - 1 ) / matProj(1, 1));
    vPickRayDir.setZ(1.0f);

    // Get the inverse view matrix
    QMatrix4x4 view_matrix = camera_->viewMatrix();
    QMatrix4x4 model_view = view_matrix * model_matrix_ ;
    QMatrix4x4 m = model_view.inverted();
    QVector3D vPickRayOrig(0, 0, 0);  
    
    // Transform the screen space Pick ray into 3D space
    vPickRayOrig = m * vPickRayOrig;

    QMatrix3x3 normal_matrix = m.normalMatrix(); 
    QMatrix4x4 temp(normal_matrix);
    vPickRayDir = temp * vPickRayDir;
    vPickRayDir.normalize();

    // 遍历各个关节 看是否选中
    QVector<int> selected_joint_indices;
    for (int i = 0; i < avatar_->joints_.size(); ++i)
    {
        Sphere s(avatar_->joints_[i]->pos().toVector3D(), 2);
        if (IntersectSphere( vPickRayOrig, vPickRayDir, s))
        {
            selected_joint_indices.append(i);
        }
    }

    qDebug() << "----------------";
    qDebug() << selected_joint_indices.size() << " selected";
    for (int i = 0; i < selected_joint_indices.size(); ++i)
    {
        qDebug() << avatar_->joints_[i]->name;
    }

    return true;
}

// UI 场景树形结构
SceneGraphTreeWidget::SceneGraphTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
    setColumnCount(1);
    setHeaderHidden(true);

    QTreeWidgetItem* root = invisibleRootItem();
    QTreeWidgetItem* avatar_group = new QTreeWidgetItem(root);
    avatar_group->setText(0, "Avatar");
    QTreeWidgetItem* avatar_0_item = new QTreeWidgetItem(avatar_group);
    avatar_0_item->setText(0, "man");

    QTreeWidgetItem* cloth_item = new QTreeWidgetItem(root);
    cloth_item->setText(0, "Cloth");
    QTreeWidgetItem* cloth_panel_0 = new QTreeWidgetItem(cloth_item);
    cloth_panel_0->setText(0, "cloth_panel_0");
    QTreeWidgetItem* cloth_panel_1 = new QTreeWidgetItem(cloth_item);
    cloth_panel_1->setText(0, "cloth_panel_1");

    QTreeWidgetItem* light_group = new QTreeWidgetItem(root);
    light_group->setText(0, "Light");
    QTreeWidgetItem* light_0_item = new QTreeWidgetItem(light_group);
    light_0_item->setText(0, "light_0");

    QTreeWidgetItem* camera_group = new QTreeWidgetItem(root);
    camera_group->setText(0, "Camera");
    QTreeWidgetItem* camera_0 = new QTreeWidgetItem(camera_group);
    camera_0->setText(0, "camera_0");

    QTreeWidgetItem* env_group = new QTreeWidgetItem(root);
    env_group->setText(0, "Environment");
    QTreeWidgetItem* floor_item = new QTreeWidgetItem(env_group);
    floor_item->setText(0, "floor");
    QTreeWidgetItem* gravity_item = new QTreeWidgetItem(env_group);
    gravity_item->setText(0, "gravity");
}
