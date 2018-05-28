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

// ���ɶ�
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
/* �ؽ�                                                                 */
/************************************************************************/
struct Joint : public SceneNode
{
	QString name;						// �ؽ�����
	Joint* parent;						// ���ؽ�
	QVector<Joint*> children;			// �ӹؽ�
	QMatrix4x4 inverse_bindpose_matrix;	// ��mesh space��bone space (bind pose)֮�任����
	QMatrix4x4 local_transform;			// ����ڸ��ؽڵı任
	QMatrix4x4 global_transform;		// ������ռ��еı任(�ɸ��ؽ��ۻ����˵���)
	int channel_index;					// ����ͨ������
	int index;                          // ���� ����indexed GPU skinning

    uint dof;
    // DOFԼ�� (��ת�Ƕȷ�Χ)
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
 * ����
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
/* ˫��Ԫ��                                                             */
/************************************************************************/
struct DualQuaternion
{
    QQuaternion ordinary;
    QQuaternion dual;
};

DualQuaternion quatTransToUnitDualQuaternion(const QQuaternion& q0, const QVector3D& t);
QQuaternion matToQuat(const QMatrix4x4& mat); // convert an orthogonal matrix to unit quaternion
/************************************************************************/
/* ��תֵ                                                               */
/************************************************************************/
struct QuaternionKey 
{
	double time;// ��λ����
	QQuaternion value;
};

bool quatKeyCmp(const QuaternionKey& key1, const QuaternionKey& k2);
/************************************************************************/
/* λ��ֵ ����ֵ                                                        */
/************************************************************************/
// 
struct VectorKey 
{
	double time;// ��λ����
	QVector3D value;
};

bool vecKeyCmp(const VectorKey& key1, const VectorKey& key2);

QVector3D quatToEulerAngles(const QQuaternion& quat); 
/************************************************************************/
/* ASSIMP --> Qt��������ת������                                        */
/************************************************************************/
void aiQuatKeyToQautKey(QuaternionKey& qkey, const aiQuatKey& akey); 
void aiMatToQMat(QMatrix4x4& qmat, const aiMatrix4x4& amat);
void aiVecKeyToVecKey(VectorKey& qkey, const aiVectorKey& akey);

/************************************************************************/
/* ����ͨ��                                                             */
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
/* ������Ϣ                                                             */
/************************************************************************/
struct VertexInfo 
{
	QVector<QPair<Joint*, float> > joint_weights;
};
typedef QVector<VertexInfo> SkinInfo;

class QOpenGLBuffer;
class QOpenGLVertexArrayObject;

/************************************************************************/
/* ��Ƥ                                                                 */
/************************************************************************/
struct Skin 
{
	enum { MAX_NUM_WEIGHTS = 4 };   // ÿ������������ĸ��ؽ�Ӱ��

	SkinInfo		    skininfo;	// ��Ƥ��Ϣ
	QVector<QVector3D>	bindpose_pos;	// ����̬λ��
	//QVector<QVector3D>	bindpose_shrinked;	// ����̬λ��
	QVector<QVector3D>  bindpose_norm;  // ����̬����

	QVector<QVector3D>	positions;	// ����̬λ��
	QVector<QVector3D>  normals;    // ����̬����
	QVector<QVector2D>	texcoords;
	QVector<uint>		indices;

	QVector<QVector4D>  joint_indices_; // �ĸ��ؽ��������δ洢��x y z w
	QVector<QVector4D>  joint_weights_; // �ĸ��ؽ�Ȩ�����δ洢��x y z w
	uint			    texid;
	uint			    num_triangles;

	// VBO indexing rendering
	QOpenGLBuffer*	position_buffer_;
	QOpenGLBuffer*	normal_buffer_;
	QOpenGLBuffer*	texcoords_buffer_;
	QOpenGLBuffer*	index_buffer_;

	// ����GPU skinning
	QOpenGLBuffer*  joint_weights_buffer_; // �ؽ�Ȩ��VBO
	QOpenGLBuffer*  joint_indices_buffer_; // �ؽ�����VBO

	// VAO
	QOpenGLVertexArrayObject* vao_;
};
typedef QVector<Skin> SkinList;

class Avatar;
/************************************************************************/
/* ����                                                                 */
/************************************************************************/
class Animation 
{
public:
    enum { TICKS_PER_SEC = 60 }; //  Ĭ�ϲ���Ƶ����ÿ��60��

    Animation()
        : ai_anim(nullptr),
        avatar(nullptr),
        name("dummy"),
        ticks_per_second(TICKS_PER_SEC),
        ticks(0)
    {
    }

	Animation(const aiAnimation* pAnimation, Avatar* luke);	// ¬�ˣ���ʹ��ԭ�� ע�;Ͳ�����Ȥ��ô

	void addKeyframes(const ChannelList& cl, int start_time, int length, double ticks_per_sec, double play_speed, double weight);	// ��ӹؼ�֡ ��ʼʱ�䣺��λ����
    void clear();   // �������

	const aiAnimation*	ai_anim;	// ԭʼASSIMP����	
    Avatar*				avatar;		// ����Avatar
	QString				name;		// ����
	double				ticks_per_second;        // ����ÿ��
    double              ticks;      // ������
	ChannelList			channels;	// ����ͨ�� �ؼ�֡����
	
};
typedef QList<Animation> AnimList;

class AnimationTrack;
/************************************************************************/
/* ����Ƭ��                                                             */
/************************************************************************/
class AnimationClip : public QGraphicsObject
{
    friend AnimationTrack;
	Q_OBJECT

public:
	enum { SECOND_WIDTH = 100 }; // ÿһ���Ӧ���Ϊ100����
    static int SAMPLE_SLICE, SIM_SLICE;	 // ����ʱ��ƬĬ��20ms, ģ��ʱ��ƬĬ��2ms, ����ο������ļ� 
	enum { Type = UserType + 1 };

	AnimationClip(QMenu* contex_menu, QGraphicsScene *scene, Animation* anim, AnimationTrack* track, int x, int y);

    void setPos( qreal x, qreal y );
    void moveBy(qreal dx, qreal dy);
    void setStartTime(int t);
    void setEndTime(int t);
    void putOffBy(int t);        // �Ƴ�
    void putForwardBy(int t);    // ��ǰ
    
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
    Animation*		animation_;	// ԭʼ����	
	AnimationTrack* track_;		// �������
	QString			name_;		// Ƭ������
	int				length_;	// Ƭ�γ��� ��λ����
	int				y_pos_;		// ���ĵ�yλ�� ���ڽ�Ƭ�������ڹ����
    qreal           delta_;     // Ƭ���ƶ�ƫ�� delta < 0 ���� delta > 0 ����
    QMenu*          context_menu_;
};

class AnimationTrackScene;
/************************************************************************/
/* �������                                                             */
/************************************************************************/ 
class AnimationTrack : public QObject
{
    friend AnimationTrackScene;
    Q_OBJECT

public:
    enum { TRACK_HEIGHT = 30 };		// ����߶�30����
    enum { OVERLAP_TOLERANCE = 30 };// �ж�Ƭ���Ƿ��ص�������� 30ms

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
    void resolveOverlap(AnimationClip* clip);  // ���µ���Ƭ��λ�� �����ص�

    void moveUp();
    void moveDown();

signals:
    void clipMoved();

private:
    void move(int dist);

private:
    AnimationTrackScene*    track_edit_;    // ����༭��    
	QList<AnimationClip*>   clips_;         // ����Ƭ��

	bool locked_;   // �Ƿ������������Ĺ��������༭��
	bool visible_;  // �Ƿ�ɼ����ɼ��Ĺ���������ܲ��ţ�
	double weight_; // Ȩ��
};
// typedef QList<AnimationTrack> TrackList;

// Qt����ͼ��
/************************************************************************/
/* �������ģ��                                                         */
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
/* �Ǽ�����ģ��                                                         */
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
/* ������׽���ݵ�����                                                    */
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
/* Acclaim ASF/AMC ��ʽ�������ݵ�����                                    */
/************************************************************************/
class ASFAMCImporter : public MocapImporter
{
    friend class Avatar;
public:
    ASFAMCImporter();
    ~ASFAMCImporter();

    virtual bool load(QString& filename); // ����ASF�ļ�
    bool loadMotion(QString& filename); // ����AMC�ļ�

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
// aiNode-joint ӳ���
typedef QMap<const aiNode*, Joint*> NodeToJointMap;

// Name-aiNode ӳ���
typedef QMap<QString, const aiNode*> NameToNodeMap;

// Name-joint ӳ���
typedef QMap<QString, Joint*> NameToJointMap;

// Name-animation ӳ���
typedef QMap<QString, Animation*> NameToAnimMap;

// Name-channel index ӳ���
typedef QMap<QString, int> NameToChIdMap;

class Scene;
/************************************************************************/
/* ��������                                                             */
/************************************************************************/
class Avatar : public SceneNode
{
	friend Scene;
public:
	Avatar(const aiScene* pScene, const QString& filename);
	~Avatar();

	Joint* finddJointByName(const QString& name) const;
	void updateAnimation(const Animation* animation, int elapsed_time);
	void skinning();	     //�ѷϳ� Ŀǰ��ȫ����GPU��Ƥ                                       

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

	void updateSkinVBO(); // wnf��ӣ�����CPU��Ƥ

private:
	Joint* buildSkeleton(aiNode* pNode, Joint* pParent);							// ����Ǽ�	
	void calcGlobalTransform(Joint* pJoint);										// ����ؽ�ȫ�ֱ任	
	void updateTransforms(Joint* pJoint, const QVector<QMatrix4x4>& vTransforms);	// �Ը��ؽ�ʵʩ�任
	void updateJointMatrices();                                                     // ����matrix palette
    void updateJointDualQuaternions();                                              // ����˫��Ԫ��
    void updateSkeletonVBO();                                                       // ���¹ؽں͹���VBO
	void makeAnimationCache();														// ������ASSIMP���صĶ���	
	void makeSkinCache();															// ������Ƥ	
    void createBoundings();                                                         // �����Χ��
    void buildNameAnimationMap();		                                            // ��������-����ӳ���
    void buildNameChannelIndexMap();                                                // ��������-ͨ������ӳ���
	void loadDiffuseTexture();
	bool createSkinVBO();
    bool createSkeletonVBO();
    bool loadMH2CMUJointMap();
	void initSimParameters();

	const aiScene*			ai_scene_;	    // ASSIMP����
	Joint*					root_;			// ���ؽ�
	QVector<Joint*>			joints_;		// ���йؽ�
	AnimList			    animations_;	// �ؼ�֡
	QVector<QMatrix4x4>		transforms_;    // �����ؽڵľֲ��任
	QVector<QMatrix4x4>		joint_matrices_;// matrix palette(�ؽ�ȫ�ֱ任 * �����̬����)
    QVector<QVector4D>      joint_dual_quaternions_;    // ˫��Ԫ��
	SkinList				skins_;			// ��Ƥ

    QVector<Bone*>              bones_;
    QVector<QVector4D>          joint_positions_;
    QVector<QVector4D>          bone_positions_;
    QOpenGLBuffer*              joint_pos_buffer_;
    QOpenGLBuffer*              bone_pos_buffer_;
    QOpenGLVertexArrayObject*   joint_vao_;
    QOpenGLVertexArrayObject*   bone_vao_;
    QVector<OBB>                bounding_obbs_; // ��Χ�� �������Ƥһһ��Ӧ
    AABB                        bounding_aabb_; // �����Χ��

	bool	has_materials_;	// �Ƿ��в�������
	bool    has_animations_;// �Ƿ��ж���
	//bool    gpu_skinning_;  // �Ƿ����GPU��Ƥ �ѷϳ� ʼ�ղ���GPU��Ƥ
	bool    bindposed_;     // �Ƿ��ڰ���̬
	QString	file_dir_;		// ģ���ļ�Ŀ¼

	//std::vector<std::tuple<uint, uint, uint> > last_playheads_;	// ��¼��һ�β��ŵĹؼ�֡λ��

	NodeToJointMap	node_joint_;
	NameToNodeMap	name_node_;
	NameToJointMap	name_joint_;
    NameToAnimMap	name_animation_;		// ����-����ӳ���
    NameToChIdMap   name_channelindex_;     // ����-ͨ������ӳ���
	QString			diffuse_tex_path_;

	// UI��Ա
	AnimationTableModel*	animation_table_model_;
	SkeletonTreeModel*		skeleton_tree_model_;

    //----------------------
    // ������׽���ݵ���
    QMap<QString, QString>  mh2cmu_joint_map_;  // makehuman --> cmu mocap�ؽ�����ӳ���
    QMap<QString, int>      mh2cmu_channel_map_;
    ASFAMCImporter*         asfamc_importer_;

	// ��������ӵ�ģ��λ���������ݣ��㷨��Ҫ�ض���ģ�������ģ���Լ����ԭ��λ��
	double scale_factor_;
	double xtranslate_;
	double ytranslate_;
	double ztranslate_;
};

#endif //ANIMATION_H
