#ifndef ANIMATION_H
#define ANIMATION_H

//#include <tuple>

#include <QMatrix4x4>
#include <QPair>
#include <QVector4D>
#include <QVector3D>
#include <QVector2D>
#include <QQuaternion>
#include <QAbstractTableModel>
#include <QAbstractItemModel>
#include <QList>
#include <QStringList>
#include <QGraphicsObject>
#include <QGraphicsItemGroup>
#include <QOpenGLVertexArrayObject>

#include <assimp/scene.h>
#include <assimp/matrix4x4.h>

#include "material.h"
#include "bounding_volume.h"
#include "scene_node.h"

// 自由度
#define DOF_NONE 0
#define DOF_TX 1
#define DOF_TY 2
#define DOF_TZ 4
#define DOF_RX 8
#define DOF_RY 16
#define DOF_RZ 32
#define DOF_TXY 3
#define DOF_TYZ 6
#define DOF_TXZ 5
#define DOF_T 7
#define DOF_RXY 24
#define DOF_RYZ 48
#define DOF_RXZ 40
#define DOF_R 56 
#define DOF_TR 63

/************************************************************************/
/* 关节                                                                 */
/************************************************************************/
struct Joint : public SceneNode
{
	QString name;						// 关节名称
	Joint* parent;						// 父关节
	QVector<Joint*> children;			// 子关节
	QMatrix4x4 inverse_bindpose_matrix;	// 从mesh space到bone space (bind pose)之变换矩阵
	QMatrix4x4 local_transform;			// 相对于父关节的变换
	QMatrix4x4 global_transform;		// 在世界空间中的变换(由根关节累积至此得来)
	int channel_index;					// 动画通道索引
	int index;                          // 索引 用于indexed GPU skinning

    uint dof;
    // DOF约束 (旋转角度范围)
    double min_rx, max_rx;  // X
    double min_ry, max_ry;  // Y
    double min_rz, max_rz;  // Z

	explicit Joint(const char* sName) 
		: name(sName), 
        parent(nullptr), 
        channel_index(-1), 
        dof(DOF_NONE),
        index(-1)	
	{}

	~Joint() 
	{
		qDeleteAll(children.begin(), children.end());
		children.clear();
	}

    QVector4D pos() const
    {
        return global_transform.column(3);
    }
};

/*
 * 骨骼
 */
struct Bone
{
    Joint* from;
    Joint* to;

    Bone(Joint* f, Joint* t) :
        from(f),
        to(t)
    {
    }

    QVector4D fromPos() const
    {
        if (from)
            return from->pos();
        return QVector4D(0, 0, 0, 0);
    }

    QVector4D toPos() const
    {
        if (to)
            return to->pos();
        return QVector4D(0, 0, 0, 0);
    }
};
/************************************************************************/
/* 双四元数                                                             */
/************************************************************************/
struct DualQuaternion
{
    QQuaternion ordinary;
    QQuaternion dual;
};

DualQuaternion quatTransToUnitDualQuaternion(const QQuaternion& q0, const QVector3D& t);
QQuaternion matToQuat(const QMatrix4x4& mat); // convert an orthogonal matrix to unit quaternion
/************************************************************************/
/* 旋转值                                                               */
/************************************************************************/
struct QuaternionKey 
{
	double time;// 单位毫秒
	QQuaternion value;
};

bool quatKeyCmp(const QuaternionKey& key1, const QuaternionKey& k2);
/************************************************************************/
/* 位移值 缩放值                                                        */
/************************************************************************/
// 
struct VectorKey 
{
	double time;// 单位毫秒
	QVector3D value;
};

bool vecKeyCmp(const VectorKey& key1, const VectorKey& key2);

QVector3D quatToEulerAngles(const QQuaternion& quat); 
/************************************************************************/
/* ASSIMP --> Qt数据类型转换函数                                        */
/************************************************************************/
void aiQuatKeyToQautKey(QuaternionKey& qkey, const aiQuatKey& akey); 
void aiMatToQMat(QMatrix4x4& qmat, const aiMatrix4x4& amat);
void aiVecKeyToVecKey(VectorKey& qkey, const aiVectorKey& akey);

/************************************************************************/
/* 动画通道                                                             */
/************************************************************************/
struct AnimationChannel 
{
	Joint*                  joint;
	QVector<VectorKey>		position_keys;
	QVector<QuaternionKey>	rotation_keys;
	QVector<VectorKey>		scaling_keys;

	AnimationChannel(Joint* j = nullptr)
        : joint(j)
    {}

	AnimationChannel(const aiNodeAnim* pNodeAnim);
};
typedef QVector<AnimationChannel> ChannelList;

/************************************************************************/
/* 顶点信息                                                             */
/************************************************************************/
struct VertexInfo 
{
	QVector<QPair<Joint*, float> > joint_weights;
};
typedef QVector<VertexInfo> SkinInfo;

class QOpenGLBuffer;
class QOpenGLVertexArrayObject;

/************************************************************************/
/* 蒙皮                                                                 */
/************************************************************************/
struct Skin 
{
	enum { MAX_NUM_WEIGHTS = 4 };   // 每个顶点最多受四个关节影响

	SkinInfo		    skininfo;	// 蒙皮信息
	QVector<QVector3D>	bindpose_pos;	// 绑定姿态位置
	//QVector<QVector3D>	bindpose_shrinked;	// 绑定姿态位置
	QVector<QVector3D>  bindpose_norm;  // 绑定姿态法线

	QVector<QVector3D>	positions;	// 绑定姿态位置
	QVector<QVector3D>  normals;    // 绑定姿态法线
	QVector<QVector2D>	texcoords;
	QVector<uint>		indices;

	QVector<QVector4D>  joint_indices_; // 四个关节索引依次存储于x y z w
	QVector<QVector4D>  joint_weights_; // 四个关节权重依次存储于x y z w
	uint			    texid;
	uint			    num_triangles;

	// VBO indexing rendering
	QOpenGLBuffer*	position_buffer_;
	QOpenGLBuffer*	normal_buffer_;
	QOpenGLBuffer*	texcoords_buffer_;
	QOpenGLBuffer*	index_buffer_;

	// 用于GPU skinning
	QOpenGLBuffer*  joint_weights_buffer_; // 关节权重VBO
	QOpenGLBuffer*  joint_indices_buffer_; // 关节索引VBO

	// VAO
	QOpenGLVertexArrayObject* vao_;
};
typedef QVector<Skin> SkinList;

class Avatar;
/************************************************************************/
/* 动画                                                                 */
/************************************************************************/
class Animation 
{
public:
    enum { TICKS_PER_SEC = 60 }; //  默认采样频率是每秒60次

    Animation()
        : ai_anim(nullptr),
        avatar(nullptr),
        name("dummy"),
        ticks_per_second(TICKS_PER_SEC),
        ticks(0)
    {
    }

	Animation(const aiAnimation* pAnimation, Avatar* luke);	// 卢克，快使用原力 注释就不能有趣点么

	void addKeyframes(const ChannelList& cl, int start_time, int length, double ticks_per_sec, double play_speed, double weight);	// 添加关键帧 起始时间：单位毫秒
    void clear();   // 清空数据

	const aiAnimation*	ai_anim;	// 原始ASSIMP动画	
    Avatar*				avatar;		// 所属Avatar
	QString				name;		// 名称
	double				ticks_per_second;        // 节拍每秒
    double              ticks;      // 节拍数
	ChannelList			channels;	// 动画通道 关键帧数据
	
};
typedef QList<Animation> AnimList;

class AnimationTrack;
/************************************************************************/
/* 动画片段                                                             */
/************************************************************************/
class AnimationClip : public QGraphicsObject
{
    friend AnimationTrack;
	Q_OBJECT

public:
	enum { SECOND_WIDTH = 100 }; // 每一秒对应宽度为100像素
    enum { SAMPLE_SLICE = 20, SIM_SLICE = 2 };	 // 采样时间片20ms, 模拟时间片2ms 
	enum { Type = UserType + 1 };

	AnimationClip(QMenu* contex_menu, QGraphicsScene *scene, Animation* anim, AnimationTrack* track, int x, int y);

    void setPos( qreal x, qreal y );
    void moveBy(qreal dx, qreal dy);
    void setStartTime(int t);
    void setEndTime(int t);
    void putOffBy(int t);        // 推迟
    void putForwardBy(int t);    // 提前
    
    int type() const;
	Avatar* avatar() const;
    AnimationTrack* track();
    int startTime() const;
    int length() const;
    int endTime() const;
    const ChannelList& channelList() const;
    double ticksPerSecond() const;
    qreal delta() const;

//    bool overlapWith(const AnimationClip* clip);
// signals:
//     void clipMoved();

protected:
	QRectF boundingRect() const;
    QPainterPath shape() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

private:		
    QGraphicsScene* scene_;
    Animation*		animation_;	// 原始动画	
	AnimationTrack* track_;		// 所属轨道
	QString			name_;		// 片段名称
	int				length_;	// 片段长度 单位毫秒
	int				y_pos_;		// 中心点y位置 用于将片断锁定在轨道上
    qreal           delta_;     // 片段移动偏差 delta < 0 左移 delta > 0 右移
    QMenu*          context_menu_;
};

class AnimationTrackScene;
/************************************************************************/
/* 动画轨道                                                             */
/************************************************************************/ 
class AnimationTrack : public QObject
{
    friend AnimationTrackScene;
    Q_OBJECT

public:
    enum { TRACK_HEIGHT = 30 };		// 轨道高度30像素
    enum { OVERLAP_TOLERANCE = 30 };// 判断片段是否重叠的误差限 30ms

	AnimationTrack(AnimationTrackScene* track_edit) 
        : track_edit_(track_edit), 
        locked_(false), 
        visible_(true), 
        weight_(1.0) 
    {
    }

    ~AnimationTrack();

	bool isEmpty() const;
    int endTime() const;
	void addClip(AnimationClip* clip);
    void delClip(AnimationClip* clip);
    void resolveOverlap(AnimationClip* clip);  // 重新调整片段位置 避免重叠

    void moveUp();
    void moveDown();

signals:
    void clipMoved();

private:
    void move(int dist);

private:
    AnimationTrackScene*    track_edit_;    // 轨道编辑区    
	QList<AnimationClip*>   clips_;         // 动画片段

	bool locked_;   // 是否锁定（锁定的轨道不允许编辑）
	bool visible_;  // 是否可见（可见的轨道动画才能播放）
	double weight_; // 权重
};
// typedef QList<AnimationTrack> TrackList;

// Qt项视图类
/************************************************************************/
/* 动画表格模型                                                         */
/************************************************************************/
class AnimationTableModel : public QAbstractTableModel 
{
public:
	AnimationTableModel(AnimList* animations, QObject *parent = 0);

	void clear();
	bool isEmpty() const;

	int rowCount(const QModelIndex& parent) const;
	int columnCount(const QModelIndex& parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	//bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	QMimeData* mimeData(const QModelIndexList &indexes) const;
	QStringList mimeTypes() const;

private:
	enum {COLUMN_COUNT = 2};

	QStringList titles_;
	AnimList* animations_;
};

/************************************************************************/
/* 骨架树型模型                                                         */
/************************************************************************/
class SkeletonTreeModel : public QAbstractItemModel
{
public:
	SkeletonTreeModel(Joint* root, QObject *parent = 0);

	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
	enum {COLUMN_COUNT = 1};
	Joint* itemFromIndex(const QModelIndex &index) const;
	Joint* root_;
};

/************************************************************************/
/* 动作捕捉数据导入器                                                    */
/************************************************************************/
class MocapImporter
{
public:
    MocapImporter(void);
    virtual ~MocapImporter(void);

    virtual bool load(QString& filename) = 0;

    uint frameNumber() const { return frame_number_; }
    double frameTime() const { return frame_time_; }
    uint jointCount() const { return joint_count_; }

protected:
    QString file_name_;
    uint frame_number_;
    double frame_time_;
    uint joint_count_;
};

const float kASFSCALE = 1.0f;
/************************************************************************/
/* Acclaim ASF/AMC 格式动补数据导入器                                    */
/************************************************************************/
class ASFAMCImporter : public MocapImporter
{
    friend class Avatar;
public:
    ASFAMCImporter();
    ~ASFAMCImporter();

    virtual bool load(QString& filename); // 加载ASF文件
    bool loadMotion(QString& filename); // 加载AMC文件

private:
    void clear();

    QVector<Joint*> joints_;
    Joint* root_;

    QMap<QString, int> name_id_; // joint name to id
    bool amc_loaded_;
    QString motion_name_;
    ChannelList channels_; 
    Animation animation_;
};
// aiNode-joint 映射表
typedef QMap<const aiNode*, Joint*> NodeToJointMap;

// Name-aiNode 映射表
typedef QMap<QString, const aiNode*> NameToNodeMap;

// Name-joint 映射表
typedef QMap<QString, Joint*> NameToJointMap;

// Name-animation 映射表
typedef QMap<QString, Animation*> NameToAnimMap;

// Name-channel index 映射表
typedef QMap<QString, int> NameToChIdMap;

class Scene;
/************************************************************************/
/* 虚拟人物                                                             */
/************************************************************************/
class Avatar : public SceneNode
{
	friend Scene;
public:
	Avatar(const aiScene* pScene, const QString& filename);
	~Avatar();

	Joint* finddJointByName(const QString& name) const;
	void updateAnimation(const Animation* animation, int elapsed_time);
	void skinning();	     //已废除 目前完全采用GPU蒙皮                                       

	bool hasAnimations() const;
	bool hasMaterials() const;
	const SkinList& skins() const;
	
	const AnimationTableModel* animationTableModel() const;
	const SkeletonTreeModel* skeletonTreeModel() const;

	bool bindposed() const;
	void setBindposed(bool val);

    void importMocap(QString& asf, QString& amc);
    void addAnimation(const Animation& anim);
    //void addAnimations(const AnimList& anims);

	void updateSkinVBO(); // wnf添加，用来CPU蒙皮

private:
	Joint* buildSkeleton(aiNode* pNode, Joint* pParent);							// 构造骨架	
	void calcGlobalTransform(Joint* pJoint);										// 计算关节全局变换	
	void updateTransforms(Joint* pJoint, const QVector<QMatrix4x4>& vTransforms);	// 对各关节实施变换
	void updateJointMatrices();                                                     // 更新matrix palette
    void updateJointDualQuaternions();                                              // 更新双四元数
    void updateSkeletonVBO();                                                       // 更新关节和骨骼VBO
	void makeAnimationCache();														// 缓存由ASSIMP加载的动画	
	void makeSkinCache();															// 缓存蒙皮	
    void createBoundings();                                                         // 构造包围盒
    void buildNameAnimationMap();		                                            // 建立名称-动画映射表
    void buildNameChannelIndexMap();                                                // 建立名称-通道索引映射表
	void loadDiffuseTexture();
	bool createSkinVBO();
    bool createSkeletonVBO();
    bool loadMH2CMUJointMap();

	const aiScene*			ai_scene_;	    // ASSIMP场景
	Joint*					root_;			// 根关节
	QVector<Joint*>			joints_;		// 所有关节
	AnimList			    animations_;	// 关键帧
	QVector<QMatrix4x4>		transforms_;    // 各个关节的局部变换
	QVector<QMatrix4x4>		joint_matrices_;// matrix palette(关节全局变换 * 逆绑定姿态矩阵)
    QVector<QVector4D>      joint_dual_quaternions_;    // 双四元数
	SkinList				skins_;			// 蒙皮

    QVector<Bone*>              bones_;
    QVector<QVector4D>          joint_positions_;
    QVector<QVector4D>          bone_positions_;
    QOpenGLBuffer*              joint_pos_buffer_;
    QOpenGLBuffer*              bone_pos_buffer_;
    QOpenGLVertexArrayObject*   joint_vao_;
    QOpenGLVertexArrayObject*   bone_vao_;
    QVector<OBB>                bounding_obbs_; // 包围盒 与各子蒙皮一一对应
    AABB                        bounding_aabb_; // 整体包围盒

	bool	has_materials_;	// 是否有材质纹理
	bool    has_animations_;// 是否有动画
	//bool    gpu_skinning_;  // 是否采用GPU蒙皮 已废除 始终采用GPU蒙皮
	bool    bindposed_;     // 是否处于绑定姿态
	QString	file_dir_;		// 模型文件目录

	//std::vector<std::tuple<uint, uint, uint> > last_playheads_;	// 记录上一次播放的关键帧位置

	NodeToJointMap	node_joint_;
	NameToNodeMap	name_node_;
	NameToJointMap	name_joint_;
    NameToAnimMap	name_animation_;		// 名称-动画映射表
    NameToChIdMap   name_channelindex_;     // 名称-通道索引映射表
	QString			diffuse_tex_path_;

	// UI成员
	AnimationTableModel*	animation_table_model_;
	SkeletonTreeModel*		skeleton_tree_model_;

    //----------------------
    // 动作捕捉数据导入
    QMap<QString, QString>  mh2cmu_joint_map_;  // makehuman --> cmu mocap关节名称映射表
    QMap<QString, int>      mh2cmu_channel_map_;
    ASFAMCImporter*         asfamc_importer_;
};

#endif //ANIMATION_H
