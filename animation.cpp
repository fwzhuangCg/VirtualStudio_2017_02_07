#include "animation.h"

#include <cmath>
#include <fstream>
#include <QFileInfo>
#include <QMimeData>
#include <QtGui>
#include <QApplication>
#include <QGraphicsScene>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QStyleOptionGraphicsItem>
#include <QMessageBox>
#include <algorithm>

/************************************************************************/
/* ASSIMP --> Qt数据类型转换函数                                         */
/************************************************************************/
void aiQuatKeyToQautKey( QuaternionKey& qkey, const aiQuatKey& akey )
{
	qkey.value.setScalar(akey.mValue.w);
	qkey.value.setVector(akey.mValue.x, akey.mValue.y, akey.mValue.z);
	qkey.time = akey.mTime;
}
//	aiMatrix4x4
// | a1 a2 a3 a4 |
// | b1 b2 b3 b4 |
// | c1 c2 c3 c4 |
// | d1 d2 d3 d4 |
// 
// QMatrix4x4
// | m11 m12 m13 m14 |
// | m21 m22 m23 m24 |
// | m31 m32 m33 m34 |
// | m41 m42 m43 m44 |
void aiMatToQMat(QMatrix4x4& qmat, const aiMatrix4x4& amat) 
{
	qmat = QMatrix4x4(amat.a1, amat.a2, amat.a3, amat.a4,
		amat.b1, amat.b2, amat.b3, amat.b4,
		amat.c1, amat.c2, amat.c3, amat.c4,
		amat.d1, amat.d2, amat.d3, amat.d4);
}

void aiVecKeyToVecKey( VectorKey& qkey, const aiVectorKey& akey )
{
	qkey.value.setX(akey.mValue.x);
	qkey.value.setY(akey.mValue.y);
	qkey.value.setZ(akey.mValue.z);
	qkey.time = akey.mTime;
}

bool quatKeyCmp(const QuaternionKey& key1, const QuaternionKey& k2)
{
	return key1.time < k2.time;
}

bool vecKeyCmp(const VectorKey& key1, const VectorKey& key2)
{
	return key1.time < key2.time;
}

QVector3D quatToEulerAngles(const QQuaternion& q)
{
	double sqw = q.scalar() * q.scalar();
	double sqx = q.x() * q.x();
	double sqy = q.y() * q.y();
	double sqz = q.z() * q.z();

	double unit = sqw + sqx + sqy + sqz;
	double test = q.x() * q.y() + q.z() * q.scalar();
	double yaw, pitch, roll;
	if (test > 0.51*unit) // singularity at north pole
	{
		yaw = qRadiansToDegrees(2 * atan2(q.x(), q.scalar()));
		pitch = 90;
		roll = 0;
		return QVector3D(yaw, pitch, roll);
	}
	if (test < -0.51*unit) // singularity at south pole
	{
		yaw = qRadiansToDegrees(-2 * atan2(q.x(), q.scalar()));
		pitch = -90;
		roll = 0;
		return QVector3D(yaw, pitch, roll);
	}

	yaw = qRadiansToDegrees(atan2(2*q.y()*q.scalar() - 2*q.x()*q.z(), 1 - 2*sqy - 2*sqz));
	pitch = qRadiansToDegrees(asin(2*test)/unit);
	roll = qRadiansToDegrees(atan2(2*q.x()*q.scalar() - 2*q.y()*q.z(), 1 - 2*sqx - 2*sqz));
	return QVector3D(yaw, pitch, roll);
}

DualQuaternion quatTransToUnitDualQuaternion(const QQuaternion& q0, const QVector3D& t)
{
	DualQuaternion ret;
	ret.ordinary = q0;
	ret.dual.setScalar(-0.5f * ( t.x() * q0.x() + t.y() * q0.y() + t.z() * q0.z()) );
	ret.dual.setX(      0.5f * ( t.x() * q0.scalar() + t.y() * q0.z() - t.z() * q0.y()) );
	ret.dual.setY(      0.5f * (-t.x() * q0.z() + t.y() * q0.scalar() + t.z() * q0.x()) );
	ret.dual.setZ(      0.5f * ( t.x() * q0.y() - t.y() * q0.x() + t.z() * q0.scalar()));
	return ret;
}

// 下面的矩阵--四元数转换函数建议阅读Realtime Rendering 3E p77 注意实现时关于矩阵迹的计算细节
// 很好的参考资料 http://www.geometrictools.com/LibMathematics/Algebra/Wm5Quaternion.inl
// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
// http://www.flipcode.com/documents/matrfaq.html#Q55
QQuaternion matToQuat(const QMatrix4x4& m)
{
	QQuaternion q;
	float trace = m(0, 0) + m(1, 1) + m(2, 2);
	if ( trace > 0.0 )
	{
		float s = 0.5f / sqrtf(trace + 1.0f);
		q.setScalar(0.25f / s);
		q.setX( (m(2, 1) - m(1, 2)) * s );
		q.setY( (m(0, 2) - m(2, 0)) * s );
		q.setZ( (m(1, 0) - m(0, 1)) * s );
	} 
	else
	{
		if ( m(0, 0) > m(1, 1) && m(0, 0) > m(2, 2) )
		{
			float s = 2.0f * sqrtf( 1.0f + m(0, 0) - m(1, 1) - m(2, 2) );
			q.setScalar( (m(2, 1) - m(1, 2)) / s );
			q.setX( 0.25f * s );
			q.setY( (m(0, 1) + m(1, 0)) / s );
			q.setZ( (m(0, 2) + m(2, 0)) / s );
		} 
		else if ( m(1, 1) > m(2, 2) )
		{
			float s = 2.0f * sqrtf( 1.0f + m(1, 1) - m(0, 0) - m(2, 2) );
			q.setScalar( (m(0, 2) - m(2, 0)) / s );
			q.setX( (m(0, 1) + m(1, 0)) / s );
			q.setY( 0.25f * s );
			q.setZ( (m(1, 2) + m(2, 1)) / s );
		}
		else
		{
			float s = 2.0f * sqrtf( 1.0f + m(2, 2) - m(0, 0) - m(1, 1) );
			q.setScalar( (m(1, 0) - m(0, 1)) / s );
			q.setX( (m(0, 2) + m(2, 0)) / s );
			q.setY( (m(1, 2) + m(2, 1)) / s );
			q.setZ( 0.25f * s );
		}
	}
	return q;
}

/************************************************************************/
/* 动画通道                                                             */
/************************************************************************/
AnimationChannel::AnimationChannel( const aiNodeAnim* pNodeAnim)
{
	for (uint pos_key_index = 0; pos_key_index < pNodeAnim->mNumPositionKeys; ++pos_key_index) 
	{
		VectorKey temp;
		aiVecKeyToVecKey(temp, pNodeAnim->mPositionKeys[pos_key_index]);
		position_keys.push_back(temp);
	}

	for (uint rot_key_index = 0; rot_key_index < pNodeAnim->mNumRotationKeys; ++rot_key_index)
	{
		QuaternionKey temp;
		aiQuatKeyToQautKey(temp, pNodeAnim->mRotationKeys[rot_key_index]);
		rotation_keys.push_back(temp);
	}

	for (uint scale_key_index = 0; scale_key_index < pNodeAnim->mNumScalingKeys; ++scale_key_index) 
	{
		VectorKey temp;
		aiVecKeyToVecKey(temp, pNodeAnim->mScalingKeys[scale_key_index]);
		scaling_keys.push_back(temp);
	}
}

/************************************************************************/
/* 动画                                                                 */
/************************************************************************/
Animation::Animation( const aiAnimation* pAnimation, Avatar* luke )
	: ai_anim(pAnimation), 
	avatar(luke),
	ticks(pAnimation->mDuration),
	name(pAnimation->mName.data)    
{
	ticks_per_second = /*(*/pAnimation->mTicksPerSecond/* != 0) ? pAnimation->mDuration : static_cast<double>(TICKS_PER_SEC)*/;

	for (uint channel_index = 0; channel_index < pAnimation->mNumChannels; ++channel_index) 
	{
		const aiNodeAnim* pNodeAnim = pAnimation->mChannels[channel_index];
		AnimationChannel new_channel(pNodeAnim);
		QString joint_name(pNodeAnim->mNodeName.C_Str());
		new_channel.joint = luke->finddJointByName(joint_name);
		channels.push_back(new_channel);
	}
}

void Animation::addKeyframes( const ChannelList& cl, int start_time, int length,  double ticks_per_sec, double play_speed, double weight )
{
	// 单位转换毫秒-->节拍
	ticks_per_second = ticks_per_sec;
	double offset_ticks = start_time * 0.001 * ticks_per_second;

	// 如果是首次添加关键帧 则简单地复制关键帧数据
	if (channels.isEmpty())
	{
		channels = cl;
		for(int channel_index = 0; channel_index < cl.size(); ++channel_index) 
		{
			// 平移
			for (int key_index = 0; key_index < cl[channel_index].position_keys.size(); ++key_index) 
			{
				VectorKey& key = channels[channel_index].position_keys[key_index];
				key.time += offset_ticks;
				key.value *= weight;
			}
			// 旋转
			for (int key_index = 0; key_index < cl[channel_index].rotation_keys.size(); ++key_index) 
			{
				QuaternionKey& key = channels[channel_index].rotation_keys[key_index];
				key.time += offset_ticks;
				key.value *= weight;
			}
			// 缩放
			for (int key_index = 0; key_index < cl[channel_index].scaling_keys.size(); ++key_index) 
			{
				VectorKey& key = channels[channel_index].scaling_keys[key_index];
				key.time += offset_ticks;
				key.value *= weight;
			}
		}
	}
	else 
	{
		for(int channel_index = 0; channel_index < cl.size(); ++channel_index) 
		{
			// 把关键帧数据放到对应的通道
			auto it = channels.begin();
			while (it != channels.end()) 
			{
				if (it->joint->name == cl[channel_index].joint->name)
					break;

				++it;
			}

			Q_ASSERT(it != channels.end());
			// 平移
			for (int key_index = 0; key_index < cl[channel_index].position_keys.size(); ++key_index) 
			{
				VectorKey key = cl[channel_index].position_keys[key_index];
				key.time += offset_ticks;
				key.value *= weight;
				it->position_keys.push_back(key);
			}
			// 旋转
			for (int key_index = 0; key_index < cl[channel_index].rotation_keys.size(); ++key_index) 
			{
				QuaternionKey key = cl[channel_index].rotation_keys[key_index];
				key.time += offset_ticks;
				key.value *= weight;
				it->rotation_keys.push_back(key);
			}
			// 缩放
			for (int key_index = 0; key_index < cl[channel_index].scaling_keys.size(); ++key_index) 
			{
				VectorKey key = cl[channel_index].scaling_keys[key_index];
				key.time += offset_ticks;
				key.value *= weight;
				it->scaling_keys.push_back(key);
			}
		}
	}


	double tmp = (start_time + length) * 0.001 * ticks_per_second;
	ticks = qMax(ticks, tmp);
}

void Animation::clear()
{
	ai_anim = nullptr;
	name = "synthetic_animation";
	ticks = 0.0;
	ticks_per_second = 0.0;
	channels.clear();
}

/************************************************************************/
/* 动画片段                                                             */
/************************************************************************/

int AnimationClip::SAMPLE_SLICE = 20;
int AnimationClip::SIM_SLICE = 2;

AnimationClip::AnimationClip( QMenu* contex_menu, QGraphicsScene *scene, Animation* anim, AnimationTrack* track, int x, int y )
	: context_menu_(contex_menu),
	QGraphicsObject(), 
	animation_(anim), 
	track_(track), 
	name_(anim->name), 
	y_pos_(0), 
	delta_(0)
{
	// 节拍数换算成时长(单位毫秒)
	length_ = (anim->ticks / anim->ticks_per_second) * 1000; 

	setPos(x, y);
	setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges | ItemIsFocusable);
	scene->clearSelection();
	scene->addItem(this);
	track->addClip(this);
	setSelected(true);
	setToolTip(tr("Drag the clip to change start time"));
	setFocus();
}

void AnimationClip::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
	Q_UNUSED(painter);
	Q_UNUSED(widget);
	QPen pen("royalblue");
	if (option->state & QStyle::State_Selected) 
	{
		pen.setColor("crimson");
		pen.setStyle(Qt::DotLine);
		pen.setWidth(2);
	}

	painter->setPen(pen);
	QColor color("yellowgreen");
	painter->setBrush(color);
	painter->drawRect(boundingRect());

	painter->setFont(QFont("Consolas"));
	painter->setPen(Qt::black);
	painter->drawText(boundingRect(), Qt::AlignCenter, name_);
}

QRectF AnimationClip::boundingRect() const
{
	// TRACK_HEIGHT = 30 pixels
	QRectF rect(0, 0, 0, 30);     
	// SECOND_WIDTH = 100 pixels i.e. 1ms = 0.1 pixels
	rect.setWidth(length_* SECOND_WIDTH * 0.001);
	return rect;
}

QPainterPath AnimationClip::shape() const
{
	QPainterPath path;
	path.addRect(boundingRect());
	return path;
}

QVariant AnimationClip::itemChange( GraphicsItemChange change, const QVariant &value )
{
	switch (change) 
	{
	case ItemPositionChange:
		delta_ = (value.toPointF() - scenePos()).x();
		break;
	case ItemPositionHasChanged:
		// 锁定X值 限制片段在轨道编辑区域内
		setX(qMax(0.0, scenePos().x()));
		// 锁定Y值 限制片段只在同一轨道上移动
		setY(y_pos_);
		// 通知AnimationTrack/AnimationTrackScene片段已移动
		track_->resolveOverlap(this);
		/*emit clipMoved();*/
		break;
	default:
		break;
	};

	return QGraphicsObject::itemChange(change, value);
}

Avatar* AnimationClip::avatar() const
{
	return animation_->avatar;
}

AnimationTrack* AnimationClip::track()
{
	return track_;
}

qreal AnimationClip::delta() const
{
	return delta_;
}

// bool AnimationClip::overlapWith(const AnimationClip* clip)
// {
//     if (startTime() < clip->endTime() ||
//         endTime() > clip->startTime())
//         return true;
//     return false;
// }

void AnimationClip::setPos( qreal x, qreal y )
{
	y_pos_ = y; // 锁定y值
	QGraphicsObject::setPos(x, y);
}

void AnimationClip::moveBy(qreal dx, qreal dy)
{
	y_pos_ += dy;
	QGraphicsObject::moveBy(dx, dy);
}

void AnimationClip::setStartTime(int t)
{
	setPos(t / 10, y_pos_);
}

void AnimationClip::setEndTime(int t)
{
	setPos((t - length_) / 10, y_pos_);
}

void AnimationClip::putOffBy(int t)
{
	moveBy(t / 10, 0);
}

void AnimationClip::putForwardBy(int t)
{
	moveBy(-t / 10, 0);
}
int AnimationClip::startTime() const
{
	return x() * 10;
}

int AnimationClip::length() const
{
	return length_;
}

int AnimationClip::endTime() const
{
	return startTime() + length_;
}

const ChannelList& AnimationClip::channelList() const
{
	return animation_->channels;
}

double AnimationClip::ticksPerSecond() const
{
	return animation_->ticks_per_second;
}

void AnimationClip::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	scene()->clearSelection();
	setSelected(true);
	context_menu_->exec(event->screenPos());
}

int AnimationClip::type() const
{
	return Type;
}

/************************************************************************/
/* 动画轨道                                                             */
/************************************************************************/ 
void AnimationTrack::addClip( AnimationClip* clip )
{
	int index = 0;
	while (index < clips_.size())
	{
		if (clips_[index]->startTime() >= clip->startTime())
			break;
		++index;
	}
	clips_.insert(index, clip);
	// 处理当前片段与后续片段的重叠
	if (index != clips_.size()-1)
	{
		AnimationClip* nxt_clip = clips_[index+1];
		if (clip->collidesWithItem(nxt_clip))
		{
			// 将后续片段整体后移
			while (++index < clips_.size())
			{
				clips_[index]->putOffBy(clip->length());
			}
		}
	}
}

AnimationTrack::~AnimationTrack()
{
	qDeleteAll(clips_.begin(), clips_.end());
	clips_.clear();
}

void AnimationTrack::delClip(AnimationClip* clip)
{
	Q_ASSERT(clip);
	int index = clips_.indexOf(clip);
	clips_.removeAt(index);
}

bool AnimationTrack::isEmpty() const
{
	return clips_.isEmpty();
}

int AnimationTrack::endTime() const
{
	if (!clips_.isEmpty())
		return clips_.back()->startTime() + clips_.back()->length_;
	return 0;
}

void AnimationTrack::moveUp()
{
	move(-TRACK_HEIGHT);
}

void AnimationTrack::moveDown()
{
	move(TRACK_HEIGHT);
}

void AnimationTrack::move(int dist)
{
	foreach(AnimationClip* clip, clips_)
		clip->moveBy(0, dist);
//        clip->y_pos_ += dist;
}

void AnimationTrack::resolveOverlap(AnimationClip* clip)
{
	int clip_index = clips_.indexOf(clip);
	Q_ASSERT(clip_index >= 0);

	if (clip_index > 0)
	{
		AnimationClip* pre_clip = clips_[clip_index-1];
		// if (clip->overlapWith(pre_clip)) 
			if (clip->collidesWithItem(pre_clip))
			clip->setStartTime(pre_clip->endTime() + OVERLAP_TOLERANCE);
	}

	if (clip_index != clips_.size()-1)
	{
		AnimationClip* nxt_clip = clips_[clip_index+1];
		// if (clip->overlapWith(nxt_clip)) 
			if (clip->collidesWithItem(nxt_clip))
			clip->setEndTime(nxt_clip->startTime() - OVERLAP_TOLERANCE);
	}   
// #if _DEBUG
//     qDebug() << "--------";
//     for (int i = 0; i < clips_.size(); ++i)
//         qDebug() << i << " [" << clips_[i]->startTime() << ", " << clips_[i]->endTime() << "]";
// #endif
	// 编辑区域更新
	emit clipMoved();
}
// UI相关方法
/************************************************************************/
/* 动画表格模型                                                         */
/************************************************************************/
AnimationTableModel::AnimationTableModel( AnimList* animations, QObject *parent /*= 0*/ )
	: animations_(animations), 
	QAbstractTableModel(parent)
{
	titles_ << tr("Name") << tr("Ticks");
}

void AnimationTableModel::clear()
{
	animations_ = nullptr;
	removeRows(0, rowCount(QModelIndex()), QModelIndex());
	//reset();
}

bool AnimationTableModel::isEmpty() const
{
	if (animations_)
		return animations_->empty();
	return false;
}

int AnimationTableModel::rowCount( const QModelIndex& parent ) const
{
	Q_UNUSED(parent);
	if (animations_)
		return static_cast<int>(animations_->size());
	return 0;
}

int AnimationTableModel::columnCount( const QModelIndex& parent ) const
{
	Q_UNUSED(parent);
	return COLUMN_COUNT;
}

QVariant AnimationTableModel::data( const QModelIndex &index, int role ) const
{
	if (!index.isValid())
		return QVariant();

	if (role == Qt::TextAlignmentRole) 
	{
		return int(Qt::AlignVCenter | Qt::AlignHCenter);
	} 
	else if (role == Qt::DisplayRole) 
	{
		if (animations_) 
		{
			if (index.column() == 0) 
				return (*animations_).at(index.row()).name;
			else if (index.column() == 1)
				return (*animations_).at(index.row()).ticks;
		}
	}
	else if (role == Qt::UserRole) 
	{
		if (animations_) 
		{
			QString name((*animations_).at(index.row()).name);
			if (name.isEmpty())
				name.append("clip_" + QString::number(index.row()));

			return name;
		}
	}
	else if (role == Qt::UserRole+1) 
	{
		if (animations_)
			return (*animations_).at(index.row()).ticks;
	}
	return QVariant();
}

QVariant AnimationTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal)
			return titles_[section];
		else 
			return section+1;
	}
	return QVariant();
}

QMimeData* AnimationTableModel::mimeData( const QModelIndexList &indexes ) const
{
	QMimeData* mime = new QMimeData;
	QByteArray encodedData;

	QDataStream stream(&encodedData, QIODevice::WriteOnly);

	foreach (QModelIndex index , indexes) 
	{
		if (index.isValid() && animations_) 
		{
			//Animation* anim = itemFromIndex(index);
			QString text = qvariant_cast<QString>(data(index, Qt::UserRole));
			double duration = qvariant_cast<double>(data(index, Qt::UserRole+1));
			stream << text << duration;
		}
	}
	mime->setData("application/x-animation", encodedData);
	return mime;
}

QStringList AnimationTableModel::mimeTypes() const
{
	QStringList types;
	types << "application/x-animation";
	return types;
}

Qt::ItemFlags AnimationTableModel::flags( const QModelIndex &index ) const
{
	if (!index.isValid())
		return 0;
	return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
}

/************************************************************************/
/* 骨架树型模型                                                          */
/************************************************************************/
SkeletonTreeModel::SkeletonTreeModel( Joint* root, QObject *parent /*= 0*/ )
	: root_(root), 
	QAbstractItemModel(parent)
{
}

QVariant SkeletonTreeModel::data( const QModelIndex &index, int role ) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	Joint *joint = itemFromIndex(index);
	if (!joint)
		return QVariant();

	return joint->name;
}

Qt::ItemFlags SkeletonTreeModel::flags( const QModelIndex &index ) const
{
	if (!index.isValid())
		return 0;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant SkeletonTreeModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/ ) const
{
	Q_UNUSED(section);
	Q_UNUSED(orientation);
	Q_UNUSED(role);
	return QVariant();
}

QModelIndex SkeletonTreeModel::index( int row, int column, const QModelIndex &parent /*= QModelIndex()*/ ) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	Joint* parentItem = itemFromIndex(parent);
	Joint* child = parentItem->children[row];
	if (!child)
		return QModelIndex();
	return createIndex(row, column, child);
}

QModelIndex SkeletonTreeModel::parent( const QModelIndex &index ) const
{
	Joint* joint = itemFromIndex(index);
	if (!joint)
		return QModelIndex();
	Joint* parent = joint->parent;
	if (!parent)
		return QModelIndex();
	Joint* grandparent = parent->parent;
	if (!grandparent)
		return QModelIndex();

	int row = 0;
	while (row < grandparent->children.size()) 
	{
		if (parent == grandparent->children[row])
			break;
		++row;
	}

	return createIndex(row, 0, parent);
}

int SkeletonTreeModel::rowCount( const QModelIndex &parent /*= QModelIndex()*/ ) const
{
	Joint* parentItem = itemFromIndex(parent);
	if (!parentItem)
		return 0;
	return parentItem->children.size();
}

int SkeletonTreeModel::columnCount( const QModelIndex &parent /*= QModelIndex()*/ ) const
{
	Q_UNUSED(parent);
	return COLUMN_COUNT;
}

Joint* SkeletonTreeModel::itemFromIndex( const QModelIndex &index ) const
{
	if (index.isValid())
		return static_cast<Joint*>(index.internalPointer());
	return root_;
}

/************************************************************************/
/* 虚拟人物                                                             */
/************************************************************************/
Avatar::Avatar(const aiScene* pScene, const QString& filename)
	: SceneNode(),
	ai_scene_(pScene), 
	root_(nullptr),
	has_materials_(ai_scene_->HasMaterials()),
	has_animations_(ai_scene_->HasAnimations()),
	bindposed_( true ),
	//gpu_skinning_(false),
	file_dir_(QFileInfo(filename).absolutePath()),
	asfamc_importer_(nullptr),
	scale_factor_(1.f),
	xtranslate_(0.f),
	ytranslate_(0.f),
	ztranslate_(0.f)
{
	// 从ASSIMP读取的数据中抽取骨骼，构造骨架
	for (uint mesh_index = 0; mesh_index < pScene->mNumMeshes; ++mesh_index) 
	{
		const aiMesh* pMesh = pScene->mMeshes[mesh_index];
		for (unsigned int bone_index = 0; bone_index < pMesh->mNumBones; ++bone_index) 
		{
			const aiBone* pJoint = pMesh->mBones[bone_index];
			QString bone_name(pJoint->mName.data);
			name_node_[bone_name] = pScene->mRootNode->FindNode(pJoint->mName);
		}
	}

	root_ = buildSkeleton(ai_scene_->mRootNode, nullptr);
	// 设置关节的索引 用于indexed GPU skinning
#if _DEBUG
	qDebug() << joints_.count();
#endif
	for (int i = 0; i < joints_.size(); ++i) 
	{
		joints_[i]->index = i;
#if _DEBUG
	qDebug() << joints_[i]->name << " " << joints_[i]->index;
#endif     
	}
	skeleton_tree_model_ = new SkeletonTreeModel(root_);
	makeSkinCache();
	createBoundings(); // 计算包围盒 用于碰撞检测 拾取 相机自适应调整
	createSkinVBO();
	createSkeletonVBO();
	if (has_materials_)
	{
		loadDiffuseTexture();
	}

	if (has_animations_) 
	{
		makeAnimationCache();
		buildNameAnimationMap();
		buildNameChannelIndexMap();
	}
	// 否则提示用户缺少动画 是否导入动画数据
	else 
	{
		QMessageBox::warning(NULL, "VirtualStudio", "The avatar has no animations. You can import mocap data as animation.\n", QMessageBox::Ok);
		loadMH2CMUJointMap();
	}

	animation_table_model_ = new AnimationTableModel(&animations_);	

	initSimParameters();
}

Avatar::~Avatar()
{
	delete root_;	// 删除某个关节时 会递归删除其子关节
	delete animation_table_model_;
	delete skeleton_tree_model_;

	// 销毁蒙皮VBO
	for (auto skin_it = skins_.begin(); skin_it != skins_.end(); ++skin_it) 
	{
		delete skin_it->position_buffer_;
		delete skin_it->normal_buffer_;
		delete skin_it->texcoords_buffer_;
		delete skin_it->index_buffer_;
	}
	qDeleteAll(bones_.begin(), bones_.end());
	bones_.clear();
}

Joint* Avatar::buildSkeleton( aiNode* pNode, Joint* pParent )
{
	Joint* pJoint = new Joint(pNode->mName.data);
	pJoint->parent = pParent;

	node_joint_[pNode] = pJoint;
	name_joint_[pJoint->name] = pJoint;
	joints_.append(pJoint);
	// 构造骨骼
	bones_.push_back(new Bone(pParent, pJoint));

	aiMatToQMat(pJoint->local_transform, pNode->mTransformation);

	calcGlobalTransform(pJoint);

	// 确定关节对应的animation channel索引
	if (has_animations_) 
	{
		pJoint->channel_index = -1;
		const aiAnimation* current_animation = ai_scene_->mAnimations[0];
		for (uint channel_index = 0; channel_index < current_animation->mNumChannels; ++channel_index) 
		{
			if (current_animation->mChannels[channel_index]->mNodeName.data == pJoint->name) 
			{
				pJoint->channel_index = channel_index;
				break;
			}
		}
	}
	// 递归地构造整个人体骨架
	for (uint child_index = 0; child_index < pNode->mNumChildren; ++child_index) 
	{
		Joint* child = buildSkeleton(pNode->mChildren[child_index], pJoint);
		pJoint->children.push_back(child);
		// 构造骨骼
		bones_.push_back(new Bone(pJoint, child));
	}

	return pJoint;
}

void Avatar::calcGlobalTransform( Joint* pJoint )
{
	pJoint->global_transform = pJoint->local_transform;
	Joint* pParent = pJoint->parent;

	while (pParent) 
	{
		pJoint->global_transform = pParent->local_transform * pJoint->global_transform;
		pParent = pParent->parent;
	}
}

void Avatar::updateTransforms( Joint* pJoint, const QVector<QMatrix4x4>& vTransforms )
{
	if (pJoint->channel_index != -1) 
	{
		if (pJoint->channel_index >= vTransforms.size())
			return;

		pJoint->local_transform = vTransforms[pJoint->channel_index];
	}

	pJoint->global_transform = pJoint->local_transform;
	Joint* pParentJoint = pJoint->parent;

	while (pParentJoint) 
	{
		pJoint->global_transform = pParentJoint->local_transform * pJoint->global_transform;
		pParentJoint = pParentJoint->parent;
	}

	for (auto it = pJoint->children.begin(); it != pJoint->children.end(); ++it)
		updateTransforms(*it, vTransforms);
}

void Avatar::makeAnimationCache() 
{
	animations_.clear();
	for (uint anim_index = 0; anim_index < ai_scene_->mNumAnimations; ++anim_index) 
	{
		const aiAnimation* pAnimation = ai_scene_->mAnimations[anim_index];
		Animation new_animation(pAnimation, this);
		animations_.push_back(new_animation);
	}
}

void Avatar::updateAnimation( const Animation* animation, int elapsed_time )
{
	// 以下所有涉及时间的计算，单位均采用节拍tick
	double time_in_tick = elapsed_time * 0.001 * animation->ticks_per_second;   // 将时间换算成节拍

	if (time_in_tick > animation->ticks) 
		return;

	if (transforms_.size() != animation->channels.size())
		transforms_.resize(animation->channels.size());

	// 计算各channel之变换
	for (int channel_index = 0; channel_index < animation->channels.size();  ++channel_index) 
	{
		const AnimationChannel& channel = animation->channels.at(channel_index);
		/************************************************************************/
		/* 位移                                                                 */
		/************************************************************************/
		QVector3D present_position(0, 0, 0);
		if (!channel.position_keys.empty()) 
		{
			int frame = 0;
			while (frame < channel.position_keys.size() - 1) 
			{
				if (time_in_tick < channel.position_keys[frame+1].time)
					break;
				++frame;
			}

			// 在相邻两个关键帧之间插值
			uint next_frame = (frame+1) % channel.position_keys.size();
			const VectorKey& key = channel.position_keys[frame];
			const VectorKey& next_key = channel.position_keys[next_frame];
			double diff_time = next_key.time - key.time;
			if (diff_time < 0.0)
				diff_time += animation->ticks;
			if (diff_time > 0) 
			{
				float factor =  static_cast<float>((time_in_tick - key.time) / diff_time);
				present_position = key.value + (next_key.value - key.value) * factor;
			} 
			else 
			{
				present_position = key.value;
			}
		}

		/************************************************************************/
		/* 旋转                                                                 */
		/************************************************************************/
		QQuaternion present_rotation(1, 0, 0, 0);
		if (!channel.rotation_keys.empty()) 
		{
			int frame = 0;
			while (frame < channel.rotation_keys.size() - 1) 
			{
				if (time_in_tick < channel.rotation_keys[frame+1].time)
					break;
				++frame;
			}

			// SLERP
			unsigned int next_frame = (frame+1) % channel.rotation_keys.size();
			const QuaternionKey& key = channel.rotation_keys[frame];
			const QuaternionKey& next_key = channel.rotation_keys[next_frame];
			double diff_time = next_key.time - key.time;
			if (diff_time < 0)
				diff_time += animation->ticks;
			if (diff_time > 0) 
			{
				float factor =  static_cast<float>((time_in_tick - key.time) / diff_time);
				present_rotation = QQuaternion::slerp(key.value, next_key.value, factor);
			} 
			else 
			{
				present_rotation = key.value;
			}
		}

		/************************************************************************/
		/* 缩放                                                                 */
		/************************************************************************/
		QVector3D present_scaling(1, 1, 1);
		if (!channel.scaling_keys.empty()) 
		{
			int frame = 0;
			while (frame < channel.scaling_keys.size() - 1) 
			{
				if (time_in_tick < channel.scaling_keys[frame+1].time)
					break;
				++frame;
			}

			present_scaling = channel.scaling_keys[frame].value;
		}

		/************************************************************************/
		/* 构造变换矩阵                                                          */
		/************************************************************************/
		// Matrix Pallette
		QMatrix4x4& mat = transforms_[channel_index];
		mat.setToIdentity();
		mat.translate(present_position);
		mat.rotate(present_rotation);
		mat.scale(present_scaling);
	}

	updateTransforms(root_, transforms_);
	updateJointMatrices();
	updateJointDualQuaternions();
	updateSkeletonVBO();
}

void Avatar::makeSkinCache()
{
	skins_.clear();

	for (uint mesh_index = 0; mesh_index < ai_scene_->mNumMeshes; ++mesh_index) 
	{
		const aiMesh* pMesh = ai_scene_->mMeshes[mesh_index];
		Skin new_skin;
		new_skin.positions.resize(pMesh->mNumVertices);
		new_skin.normals.resize(pMesh->mNumVertices);
		new_skin.texcoords.resize(pMesh->mNumVertices);
		new_skin.indices.resize(pMesh->mNumFaces * 3);
		new_skin.skininfo.resize(pMesh->mNumVertices);
		new_skin.joint_indices_.resize(pMesh->mNumVertices);
		new_skin.joint_weights_.resize(pMesh->mNumVertices);
		new_skin.texid = pMesh->mMaterialIndex;

		// 缓存蒙皮信息
		for (uint bone_index = 0; bone_index < pMesh->mNumBones; ++bone_index) 
		{
			const aiBone* pBone = pMesh->mBones[bone_index];
			const QString bone_name(pBone->mName.C_Str());
			NameToJointMap::iterator it = name_joint_.find(bone_name);
			Joint* joint = it.value();
			// 注意设置关节的inverse bindpose matrix
			aiMatToQMat(joint->inverse_bindpose_matrix, pBone->mOffsetMatrix);

			for (uint weight_index = 0; weight_index < pBone->mNumWeights; ++weight_index) 
			{
				uint vertex_index = pBone->mWeights[weight_index].mVertexId;
				new_skin.skininfo[vertex_index].joint_weights.append(qMakePair(joint, pBone->mWeights[weight_index].mWeight));
			}
		}

		new_skin.num_triangles = pMesh->mNumFaces;
		for (unsigned int face_index = 0; face_index < pMesh->mNumFaces; ++face_index) 
		{
			aiFace& face = pMesh->mFaces[face_index];
			new_skin.indices[face_index * 3 + 0] = face.mIndices[0];
			new_skin.indices[face_index * 3 + 1] = face.mIndices[1];
			new_skin.indices[face_index * 3 + 2] = face.mIndices[2];
		}

		for (uint vertex_index = 0; vertex_index < pMesh->mNumVertices; ++vertex_index) 
		{
			new_skin.positions[vertex_index].setX(pMesh->mVertices[vertex_index].x);
			new_skin.positions[vertex_index].setY(pMesh->mVertices[vertex_index].y);
			new_skin.positions[vertex_index].setZ(pMesh->mVertices[vertex_index].z);

			new_skin.normals[vertex_index].setX(pMesh->mNormals[vertex_index].x); 
			new_skin.normals[vertex_index].setY(pMesh->mNormals[vertex_index].y);
			new_skin.normals[vertex_index].setZ(pMesh->mNormals[vertex_index].z);

			if ( (pMesh->mTextureCoords != nullptr) && (pMesh->mTextureCoords[0] != nullptr) ) 
			{
				new_skin.texcoords[vertex_index].setX(pMesh->mTextureCoords[0][vertex_index].x);
				new_skin.texcoords[vertex_index].setY(pMesh->mTextureCoords[0][vertex_index].y);
			} 

			// 生成关节索引 关节权重数组
			// 最多受四个关节影响
			Q_ASSERT(new_skin.skininfo[vertex_index].joint_weights.size() <= Skin::MAX_NUM_WEIGHTS);
			QVector<QPair<Joint*, float> >& jw = new_skin.skininfo[vertex_index].joint_weights;

			int x = 0; float p = 0;
			int y = 0; float q = 0;
			int z = 0; float r = 0;
			int w = 0; float s = 0;

			Q_ASSERT(!jw.isEmpty());
			x = jw[0].first->index;
			p = jw[0].second;

			if (jw.size() > 1) 
			{
				y = jw[1].first->index;
				q = jw[1].second;
			}
			if (jw.size() > 2) 
			{
				z = jw[2].first->index;
				r = jw[2].second;
			}
			if (jw.size() > 3) 
			{
				w = jw[3].first->index;
				s = jw[3].second;
			}

			new_skin.joint_indices_[vertex_index].setX(x);
			new_skin.joint_indices_[vertex_index].setY(y);
			new_skin.joint_indices_[vertex_index].setZ(z);
			new_skin.joint_indices_[vertex_index].setW(w);
			
			new_skin.joint_weights_[vertex_index].setX(p);
			new_skin.joint_weights_[vertex_index].setY(q);
			new_skin.joint_weights_[vertex_index].setZ(r);
			new_skin.joint_weights_[vertex_index].setW(s);
		}
		
		new_skin.bindpose_pos = new_skin.positions;
		new_skin.bindpose_norm = new_skin.normals;
		skins_.push_back(new_skin);
	}
}

void Avatar::createBoundings()
{
	const float FLOAT_MAX = std::numeric_limits<float>::max();
	const float FLOAT_MIN = std::numeric_limits<float>::min();

	QVector3D pt_max(FLOAT_MIN, FLOAT_MIN, FLOAT_MIN);
	QVector3D pt_min(FLOAT_MAX, FLOAT_MAX, FLOAT_MAX);
	for(auto skin_it = skins_.begin(); skin_it != skins_.end(); ++skin_it)
	{
		float min_x = FLOAT_MAX; float max_x = FLOAT_MIN;
		float min_y = FLOAT_MAX; float max_y = FLOAT_MIN;
		float min_z = FLOAT_MAX; float max_z = FLOAT_MIN;
		for (auto pos_it = skin_it->positions.begin(); pos_it != skin_it->positions.end(); ++pos_it)
		{
			min_x = std::min(min_x, pos_it->x()); 
			min_y = std::min(min_y, pos_it->y()); 
			min_z = std::min(min_z, pos_it->z()); 

			max_x = std::max(max_x, pos_it->x());
			max_y = std::max(max_y, pos_it->y());
			max_z = std::max(max_z, pos_it->z());
		}

		OBB obb(min_x, min_y, min_z, max_x, max_y, max_z);
		bounding_obbs_.push_back(obb);

		pt_min.setX(std::min(min_x, pt_min.x()));
		pt_min.setY(std::min(min_y, pt_min.y()));
		pt_min.setZ(std::min(min_z, pt_min.z()));

		pt_max.setX(std::max(max_x, pt_max.x()));
		pt_max.setY(std::max(max_y, pt_max.y()));
		pt_max.setZ(std::max(max_z, pt_max.z()));
	}
	bounding_aabb_.pt_min = pt_min;
	bounding_aabb_.pt_max = pt_max;
}

 void Avatar::skinning()
 {
	 //if (gpu_skinning_) 
	 //{
	 //    // transform feedback
	 //}
	// else 
	 {
		 for (auto skin_it = skins_.begin(); skin_it != skins_.end(); ++skin_it) 
		 {
			 for (int info_index = 0; info_index < skin_it->skininfo.size(); ++info_index) 
			 {
				 VertexInfo& info = skin_it->skininfo[info_index];
				 QVector4D position;
				 QVector4D normal;
 
				 for (auto jw_it = info.joint_weights.begin(); jw_it != info.joint_weights.end(); ++jw_it) 
				 {
					 Joint* joint = jw_it->first;
					 float weight = jw_it->second;
 
					 QMatrix4x4 transform;
					 transform = joint->global_transform * joint->inverse_bindpose_matrix;
					 QVector4D pos(skin_it->bindpose_pos[info_index], 1.0);
					 position += transform * pos * weight;
					 // 法线变换
					 QVector4D norm(skin_it->bindpose_norm[info_index]);
					 normal += transform * norm * weight;
				 }
 
				 skin_it->positions[info_index].setX(position.x() * scale_factor_ + xtranslate_);
				 skin_it->positions[info_index].setY(position.y() * scale_factor_ + ytranslate_);
				 skin_it->positions[info_index].setZ(position.z() * scale_factor_ + ztranslate_);
 
				 skin_it->normals[info_index].setX(normal.x());
				 skin_it->normals[info_index].setY(normal.y());
				 skin_it->normals[info_index].setZ(normal.z());
			 }
		 }
	 }
 }

Joint* Avatar::finddJointByName( const QString& name ) const
{
	auto it = name_joint_.find(name);
	if (it != name_joint_.end())
		return it.value();
	return nullptr;
}

void Avatar::buildNameAnimationMap()
{
	name_animation_.clear();
	for (int i = 0; i < animations_.size(); ++i) 
	{
		// 如果动画名称为空 就为之设置一个默认名称
		if (animations_[i].name.isEmpty())
			animations_[i].name = "anim" + QString::number(i);
		name_animation_.insert(animations_[i].name, &animations_[i]);
	}
}

void Avatar::buildNameChannelIndexMap()
{
	name_channelindex_.clear();
	Q_ASSERT(!animations_.isEmpty());
	for (int i = 0; i < animations_[0].channels.size(); i++)
	{
		name_channelindex_.insert(animations_[0].channels[i].joint->name, i);
	}
}

void Avatar::loadDiffuseTexture()
{
	// 仅加载一张diffuse texture
	aiString path;
	aiReturn texFound = ai_scene_->mMaterials[0]->GetTexture(aiTextureType_DIFFUSE, 0, &path);
	if (texFound == AI_SUCCESS) 
	{
		diffuse_tex_path_ = file_dir_;
		diffuse_tex_path_.append("/");
		diffuse_tex_path_ += path.data;
	}
}

bool Avatar::createSkinVBO()
{
	for (auto skin_it = skins_.begin(); skin_it != skins_.end(); ++skin_it) 
	{
		skin_it->position_buffer_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		skin_it->position_buffer_->create();
		if ( !skin_it->position_buffer_->bind() ) 
		{
			qWarning() << "Could not bind vertex buffer to the context";
			return false;
		}
		skin_it->position_buffer_->setUsagePattern( QOpenGLBuffer::DynamicDraw );
		skin_it->position_buffer_->allocate( skin_it->positions.data(), skin_it->positions.size() * sizeof(QVector3D) );
		skin_it->position_buffer_->release();

		skin_it->normal_buffer_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		skin_it->normal_buffer_->create();
		if ( !skin_it->normal_buffer_->bind() ) 
		{
			qWarning() << "Could not bind normal buffer to the context";
			return false;
		}
		skin_it->normal_buffer_->setUsagePattern(QOpenGLBuffer::DynamicDraw);
		skin_it->normal_buffer_->allocate(skin_it->normals.data(), skin_it->normals.size() * sizeof(QVector3D));
		skin_it->normal_buffer_->release();

		if (hasMaterials()) 
		{
			skin_it->texcoords_buffer_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
			skin_it->texcoords_buffer_->create();
			if (!skin_it->texcoords_buffer_->bind()) 
			{
				qWarning() << "Could not bind uv buffer to the context";
				return false;
			}
			skin_it->texcoords_buffer_->setUsagePattern(QOpenGLBuffer::DynamicDraw);
			skin_it->texcoords_buffer_->allocate(skin_it->texcoords.data(), skin_it->texcoords.size() * sizeof(QVector2D));
			skin_it->texcoords_buffer_->release();
		}

		skin_it->index_buffer_ = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
		skin_it->index_buffer_->create();
		if (!skin_it->index_buffer_->bind()) 
		{
			qWarning() << "Could not bind index buffer to the context";
			return false;
		}
		skin_it->index_buffer_->setUsagePattern(QOpenGLBuffer::DynamicDraw);
		skin_it->index_buffer_->allocate( skin_it->indices.data(), skin_it->indices.size() * sizeof(uint));
		skin_it->index_buffer_->release();

		// 创建关节索引VBO 关节权重VBO
		skin_it->joint_indices_buffer_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		skin_it->joint_indices_buffer_->create();
		skin_it->joint_indices_buffer_->bind();
		skin_it->joint_indices_buffer_->setUsagePattern(QOpenGLBuffer::StaticRead);
		skin_it->joint_indices_buffer_->allocate(skin_it->joint_indices_.data(), skin_it->joint_indices_.size() * sizeof(QVector4D));
		skin_it->joint_indices_buffer_->release();

		skin_it->joint_weights_buffer_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		skin_it->joint_weights_buffer_->create();
		skin_it->joint_weights_buffer_->bind();
		skin_it->joint_weights_buffer_->setUsagePattern(QOpenGLBuffer::StaticRead);
		skin_it->joint_weights_buffer_->allocate(skin_it->joint_weights_.data(), skin_it->joint_weights_.size() * sizeof(QVector4D));
		skin_it->joint_weights_buffer_->release();
	}

	return true;
}

bool Avatar::createSkeletonVBO()
{ 
	// 关节VBO
	foreach(Joint* joint, joints_)
	{
		joint_positions_.append(joint->pos());
	} 
	joint_pos_buffer_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	if (!joint_pos_buffer_->create())
	{
		qWarning() << "Could not bind joint position buffer to the context";
		return false;
	}
	joint_pos_buffer_->bind();
	joint_pos_buffer_->setUsagePattern(QOpenGLBuffer::DynamicDraw);
	joint_pos_buffer_->allocate(joint_positions_.data(), joint_positions_.size() * sizeof(QVector4D));
	joint_pos_buffer_->release();

	// 骨骼VBO
	foreach(const Bone* bone, bones_)
	{
		bone_positions_.append(bone->fromPos());
		bone_positions_.append(bone->toPos());
	}
	bone_pos_buffer_ = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	bone_pos_buffer_->create();
	if (!bone_pos_buffer_->bind())
	{
		qWarning() << "Could not bind bone position buffer to the context";
		return false;
	}
	bone_pos_buffer_->setUsagePattern(QOpenGLBuffer::DynamicDraw);
	bone_pos_buffer_->allocate(bone_positions_.data(), bone_positions_.size() * sizeof(QVector4D));
	bone_pos_buffer_->release();

	return true;
}

bool Avatar::bindposed() const
{
	return bindposed_;
}

void Avatar::updateSkinVBO()
{
	for (auto skin_it = skins_.begin(); skin_it != skins_.end(); ++skin_it) 
	{
		skin_it->position_buffer_->bind();
		skin_it->position_buffer_->allocate(skin_it->positions.data(), skin_it->positions.size() * sizeof(QVector3D));
		skin_it->position_buffer_->release();
 
		skin_it->normal_buffer_->bind();
		skin_it->normal_buffer_->allocate(skin_it->normals.data(), skin_it->normals.size() * sizeof(QVector3D));
		skin_it->normal_buffer_->release();
	}
}

void Avatar::setBindposed(bool val)
{
	bindposed_ = val; 
	for (auto skin_it = skins_.begin(); skin_it != skins_.end(); ++skin_it) 
	{
		skin_it->positions = skin_it->bindpose_pos;
		skin_it->normals = skin_it->bindpose_norm;
	}
	updateSkinVBO();
}

void Avatar::updateJointMatrices()
{
	joint_matrices_.resize(joints_.size());
	for (int jm_index = 0; jm_index < joint_matrices_.size(); ++jm_index) 
	{
		joint_matrices_[jm_index] = joints_[jm_index]->global_transform * joints_[jm_index]->inverse_bindpose_matrix;
	}
}

void Avatar::updateJointDualQuaternions()
{
	joint_dual_quaternions_.resize(joints_.size() * 2);

	for (int jointIndex = 0; jointIndex < joints_.size(); ++jointIndex) 
	{
		QQuaternion q = matToQuat(joint_matrices_[jointIndex]);
		QVector3D t = joint_matrices_[jointIndex].column(3).toVector3D();
		DualQuaternion temp = quatTransToUnitDualQuaternion(q, t);
		joint_dual_quaternions_[jointIndex * 2 + 0] = temp.ordinary.toVector4D();
		joint_dual_quaternions_[jointIndex * 2 + 1] = temp.dual.toVector4D();
	}
}

void Avatar::updateSkeletonVBO()
{
	// 更新关节VBO
	for(int joint_index = 0; joint_index < joints_.size(); ++joint_index)
	{
		joint_positions_[joint_index] = joints_[joint_index]->pos();
	} 

	joint_pos_buffer_->bind();
	joint_pos_buffer_->allocate(joint_positions_.data(), joint_positions_.size() * sizeof(QVector4D));
	joint_pos_buffer_->release();

	// 骨骼VBO
	for(int bone_index = 0; bone_index < bones_.size(); ++bone_index)
	{
		bone_positions_[bone_index * 2 + 0] = bones_[bone_index]->fromPos();
		bone_positions_[bone_index * 2 + 1] = bones_[bone_index]->toPos();
	}

	bone_pos_buffer_->bind();
	bone_pos_buffer_->allocate(bone_positions_.data(), bone_positions_.size() * sizeof(QVector4D));
	bone_pos_buffer_->release();
}

bool Avatar::hasAnimations() const
{
	return has_animations_;
}

bool Avatar::hasMaterials() const
{
	return has_materials_;
}

void Avatar::initSimParameters()
{
	std::ifstream ifs("parameters/avatar_parameter.txt");
	std::string label;
	if(ifs.is_open())
	{
		ifs >> label >> scale_factor_;
		ifs >> label >> xtranslate_;
		ifs >> label >> ytranslate_;
		ifs >> label >> ztranslate_;
	}
	ifs.close();
}

const SkinList& Avatar::skins() const
{
	return skins_;
}

// QVector<QMatrix4x4>& Avatar::jointMatrices()
// {
//     return joint_matrices_;
// }

const AnimationTableModel* Avatar::animationTableModel() const
{
	return animation_table_model_;
}

const SkeletonTreeModel* Avatar::skeletonTreeModel() const
{
	return skeleton_tree_model_;
}

bool Avatar::loadMH2CMUJointMap()
{
	// makehuman --> CMU mocap joint map
	QTextStream in(":/config/jointMap.cfg");
	int joint_num;
	in >> joint_num;

	QString mh_joint, cmu_joint;
	while (joint_num--)
	{
		in >> mh_joint >> cmu_joint;
		mh2cmu_joint_map_.insert(mh_joint, cmu_joint);
	}

	return true;
}

void Avatar::importMocap(QString& asf, QString& amc)
{
	asfamc_importer_->load(asf);
	asfamc_importer_->loadMotion(amc);
	addAnimation(asfamc_importer_->animation_);
	// 为加快查找速度 直接建立Makehuman-->CMU mocap的动画通道映射表
	mh2cmu_channel_map_.clear();
	for (auto it = mh2cmu_joint_map_.begin(); it != mh2cmu_joint_map_.end(); ++it)
	{
		int cmu_channel_index = asfamc_importer_->name_id_[it.value()];
		mh2cmu_channel_map_.insert(it.key(), cmu_channel_index);
	}
}

void Avatar::addAnimation(const Animation& anim)
{
	// 查Makehuman-->CMU mocap 关节映射表 把两者的动画通道一一对应起来
	Q_ASSERT(!mh2cmu_joint_map_.isEmpty());
	Q_ASSERT(!mh2cmu_channel_map_.isEmpty());

	has_animations_ = true;
	animations_.append(anim);
	name_animation_.insert(asfamc_importer_->motion_name_, &animations_.back()); 
}

//---------------------------------------------------------------------------------------

MocapImporter::MocapImporter(void) :
	frame_number_(0), 
	frame_time_(0),
	joint_count_(0)
{
}

MocapImporter::~MocapImporter(void)
{
}

// Acclaim ASF/AMC importer
ASFAMCImporter::ASFAMCImporter()
{

}

ASFAMCImporter::~ASFAMCImporter()
{
	clear();
}

bool ASFAMCImporter::load(QString& filename)
{
	clear();
	QTextStream in(&filename, QIODevice::ReadOnly);

	QString keyword;
	QString junk;
	double x, y, z, rx, ry, rz;

	// create root
	// 默认root order TX TY TZ RX RY RZ axis XYZ
	root_ = new Joint("root");
	name_id_["root"] = 0;
	in >> keyword;
	while (keyword != ":bonedata")
	{
		if (keyword == "position")
		{  
			in >> x >> y >> z;
			root_->local_transform.translate(x, y, z);
		}
		else if (keyword == "orientation")
		{
			in >> rx >> ry >> rz; 
			root_->local_transform.rotate(rx, 1.0, 0.0, 0.0);
			root_->local_transform.rotate(ry, 0.0, 1.0, 0.0);
			root_->local_transform.rotate(rz, 0.0, 0.0, 1.0);
		}
	}
	joints_.append(root_);

	// parse the other joints
	in >> keyword;
	while (!in.atEnd() && keyword != ":hierarchy")
	{
		in >> keyword;
		if (keyword == "begin")
		{
			Joint* new_joint = new Joint("foo");

			in >> keyword;
			while (keyword != "end")
			{
				in >> keyword;
				if (keyword == "id")
				{
					in >> new_joint->index;
				}
				else if (keyword == "name")
				{
					in >> new_joint->name;
					name_id_[new_joint->name] = new_joint->index;
				}
				else if (keyword == "direction")
				{
					in >> x >> y >> z;
					new_joint->local_transform.translate(x, y, z);
					new_joint->local_transform.translate(x, y, z);
					new_joint->local_transform.translate(x, y, z);
				}
				else if (keyword == "length")
				{
					//in >> new_joint->length;
				}
				else if (keyword == "axis")
				{
					in >> x >> y >> z >> junk;
					new_joint->local_transform.rotate(rx, 1.0, 0.0, 0.0);
					new_joint->local_transform.rotate(ry, 0.0, 1.0, 0.0);
					new_joint->local_transform.rotate(rz, 0.0, 0.0, 1.0);
				}
				else if (keyword == "dof")
				{
					QString freedom;
					int dof_count = 0;
					bool rx = false, ry = false, rz = false;
					in >> freedom;
					while (freedom != "limits")
					{
						if (freedom == "rx")
						{
							rx = true;
							++dof_count;
							new_joint->dof += DOF_RX;
						}
						else if (freedom == "ry")
						{
							ry = true;
							++dof_count;
							new_joint->dof += DOF_RY;
						}
						else if (freedom == "rz")
						{
							rz = true;
							++dof_count;
							new_joint->dof += DOF_RZ;
						}
					}

					// read limits
					QString s1, s2;
					while (dof_count--)
					{
						in >> s1 >> s2;
						s1.remove(QChar('(')); // eliminate "("
						s2.remove(QChar(')')); // eliminate ")"
						double start = s1.toDouble();
						double end = s2.toDouble();
						if (rx)
						{
							new_joint->min_rx = start;
							new_joint->max_rx = end;
						}
						else if (ry)
						{
							new_joint->min_ry = start;
							new_joint->max_ry = end;
						}
						else if (rz)
						{
							new_joint->min_rz = start;
							new_joint->max_rz = end;
						}
					}
				}
			}
		}
	}

	// parse hierarchy
	in >> keyword; // this keyword should be "begin"
	Q_ASSERT(keyword == "begin");
	while (!in.atEnd() && keyword != "end")
	{
		QString link;
		in.readLine();
		QStringList joints = link.split(" ", QString::SkipEmptyParts);
		int parent_index = name_id_.find(joints[0]).value();
		Joint* parent = joints_[parent_index];
		for (int i = 1; i < joints.size(); ++i)
		{
			int child_index = name_id_.find(joints[i]).value();
			Joint* child = joints_[child_index];
			parent->children.append(child);
			child->parent = parent;
		}
	}

	// build channels
	for (int i = 0; i < joints_.size(); ++i)
	{
		channels_.append(AnimationChannel(joints_[i]));
	}

	return true;
}

bool ASFAMCImporter::loadMotion(QString& filename)
{
	motion_name_ = filename;
	QTextStream in(&filename, QIODevice::ReadOnly);

	QString str;
	double x, y, z, rx, ry, rz;
	x = y = z = rx = ry = rz = 0;
	// ignore comments
	str = in.readLine();
	while (str.startsWith(QChar('#')) || str.startsWith(QChar(':')))
	{
		in.readLine();
	}
	// should be reading frame 1 now
	int next_frame_number = 2;
	in >> str;
	while (str != QString::number(next_frame_number))
	{
		int joint_index = name_id_.find(str).value();
		int dof = joints_[joint_index]->dof;
		switch (dof)
		{
		case DOF_NONE:
			break;
		case DOF_RXY:
			in >> rx >> ry;
			break;
		case DOF_RYZ:
			in >> ry >> rz;
			break;
		case DOF_RXZ:
			in >> rx >> rz;
			break;
		case DOF_R:
			in >> rx >> ry >> rz;
			break; 
		case DOF_TX:
			in >> x;
			break;
		case DOF_TY:
			in >> y;
			break;
		case DOF_TZ:
			in >> z;
			break;
		case DOF_RX:
			in >> rx;
			break;
		case DOF_RY:
			in >> ry;
			break;
		case DOF_RZ:
			in >> rz;
			break;
		case DOF_TXY:
			in >> x >> y;
			break;
		case DOF_TYZ:
			in >> y >> z;
			break;
		case DOF_TXZ:
			in >> x >> z;
			break;
		case DOF_T:
			in >> x >> y >> z;
			break;
		case DOF_TR:
			break;
		default:
			break;
		}

		animation_.channels = channels_;
		animation_.name = motion_name_;
		VectorKey pk;
		pk.time = (next_frame_number-1) * AnimationClip::SAMPLE_SLICE;
		pk.value = QVector3D(x, y, z);

		QuaternionKey qk;
		qk.time = pk.time;
		QQuaternion qkx = QQuaternion::fromAxisAndAngle(1.0, 0.0, 0.0, rx);
		QQuaternion qky = QQuaternion::fromAxisAndAngle(0.0, 1.0, 0.0, ry);
		QQuaternion qkz = QQuaternion::fromAxisAndAngle(0.0, 0.0, 1.0, rz);
		qk.value = qkx * qky * qkz;

		VectorKey sk;
		sk.time = pk.time;
		sk.value = QVector3D(kASFSCALE, kASFSCALE, kASFSCALE);

		animation_.channels[joint_index].position_keys.append(pk);
		animation_.channels[joint_index].rotation_keys.append(qk);
		animation_.channels[joint_index].scaling_keys.append(sk);

		++next_frame_number;
	}
	amc_loaded_ = true;
	return amc_loaded_;
}

void ASFAMCImporter::clear()
{
	name_id_.clear();
	joints_.clear();
	delete root_; // 删除某个关节时 会递归删除其子关节
	channels_.clear();
	animation_.clear();
	motion_name_ = "";
	amc_loaded_ = false;
}