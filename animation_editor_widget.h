#ifndef ANIMATION_EDITOR_WIDGET_H
#define ANIMATION_EDITOR_WIDGET_H

#include <QTabWidget>
#include <QTableView>
#include <QItemDelegate>
#include <QPainterPath>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QStateMachine>

#include "animation.h"

class QHBoxLayout;
class QPushButton;
class QLCDNumber;
class QTreeView;
class QCheckBox;
class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;

// Qtͼ��/��ͼ��
class RemixerWidget;
/************************************************************************/
/* �����������                                                          */
/************************************************************************/
class AnimationTrackScene : public QGraphicsScene
{
	friend RemixerWidget;
	Q_OBJECT

public:
    enum 
    { 
        INITIAL_WIDTH = 1000, 
        INITIAL_HEIGHT = 90
    };

	AnimationTrackScene( RemixerWidget* remixer, QObject *parent = 0 );
    ~AnimationTrackScene();

    int endTime() const;
	int endFrame() const;
    bool isEmpty() const;
    Animation* syntheticAnimation();

	void setStartFrame(int frame);
	void setCurrentFrame(int frame);
	void setEndFrame(int frame);

	void setNameAnimationMap(NameToAnimMap* name_anim);
	void updateSyntheticAnim(float play_speed = 1.0f);    // ���ºϳɶ���
    void reset();

signals:
    void syntheticAnimationUpdated(); // ֪ͨ�ϳɶ�������

protected:
	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
	void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
	void dropEvent(QGraphicsSceneDragDropEvent *event);
	void drawBackground(QPainter *painter, const QRectF &rect);

private slots:
    void onSelectionChanged();
    void onClipMoved();
    void deleteClip();        // ɾ��Ƭ��

private:
    void createContextMenu();

    AnimationTrack* addTrack(); // ��ӹ��
    void delTrack();            // ɾ�����
    void moveUpTrack();         // ��߹��
    void moveDownTrack();       // ���͹��

	AnimationClip* addClip(const QString&, const QPointF&);	// ���Ƭ��

	Animation* findAnimation(const QString& name);  // �������Ʋ��Ҷ���
	AnimationTrack* findTrackByYPos(qreal y);       // ����Ƭ������λ�ò����������

    void updateEndFrame();  // ������ֹ֡λ��
    void updateCurTrack();  // ���µ�ǰ���
    void adjustSize();      // �����༭����С

private:
    RemixerWidget* remixer_;    // ����������ϲ���

	QPen*	cur_frame_pen_;	    // ��ǰ֡����
	QPen*	drop_pos_hint_pen_;	// ����Ƭ���Ϸ�λ����ʾ�߻���
	QPen*	end_frame_pen_;		// ��ֹ֡����
    QBrush* cur_track_brush_;   // ��ǰ�����ˢ
	QPointF	drop_pos_;			// ����Ƭ���Ϸ�λ��
	int		start_frame_;		// ��ʼ֡
	int		cur_frame_;		    // ��ǰ֡
	int		end_frame_;			// ��ֹ֡
    int     cur_track_;         // ��ǰ���

	QList<AnimationTrack*>  tracks_;			    // �������
	NameToAnimMap*          name_animation_;	    // ����-����ӳ���
    Animation*              synthetic_animation_;	// �ϳɶ���

    QAction* delete_clip_action_;
    QMenu*   context_menu_;
};

/************************************************************************/
/* ���������ͼ                                                         */
/************************************************************************/
class AnimationTrackView : public QGraphicsView
{
	Q_OBJECT

public:
	AnimationTrackView(AnimationTrackScene* scene, QWidget *parent = 0);

public slots:
	void setCurrentFrame(int frame);

private:
	AnimationTrackScene* scene_;
};

/************************************************************************/
/* ���������ͼ                                                         */
/************************************************************************/
class AnimationTableView : public QTableView
{
public:
	AnimationTableView(AnimationTableModel* model = 0, QWidget *parent = 0);

protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	QSize sizeHint() const { return QSize(300, 100); }

private:
	QPoint start_pos_;
};

class MocapImportDialog;
class AnimationEditorWidget;
/************************************************************************/
/* ������ϲ���                                                         */
/************************************************************************/
// ����ؼ�֡��� ��ֵ
class RemixerWidget : public QWidget
{
    friend AnimationEditorWidget;
	Q_OBJECT

public:
	explicit RemixerWidget(QWidget *parent = 0);
	~RemixerWidget();

	void createLayout();
	void createSceneView();
	void createWidgets();
	void createStates();
	void createConnections();

    double playSpeed() const;

public slots:
	void addTrack();		// ��ӹ��

    void delTrack();		// ɾ�����
	void moveTrackUp();		// �������
	void moveTrackDown();	// ���͹�� 	

	void updateFrameRangeUI();
    void updateTrackEditUI();

	void loop();			// ѭ������
	void start();			// ����ʼ
	void playpause();		// ����/��ͣ
	void end();				// ����ֹ
	void rewind();			// ǰһ֡
	void ffw();				// ��һ֡
    void addKeyframe();     // ��ӹؼ�֡
    void showImportMocapDialog();
	void updateAnimation();
	void setFrame(int);
	void changeSpeed(int);

signals:
	void frameChanged(int frame);
	void bindposeRestored();                // �л���bindpose
    void syntheticAnimationUpdated();       // �ϳɶ����Ѹ���
    void mocapSelected(QString& , QString&);  // ��ѡ��mocap�����ļ�

private:
	QPushButton*    createToolButton(const QString &toolTip, const QIcon &icon, const char *member);

private:
	QHBoxLayout*	play_control_layout_;
	QPushButton*	loop_button_;
	QPushButton*	start_button_;
	QPushButton*	play_pause_button_;
	QPushButton*	end_button_;
	QPushButton*	rewind_button_;
	QLCDNumber*		current_frame_lcd_;		
	QPushButton*	fast_forward_button_;
	QPushButton*	add_track_button_;
	QPushButton*	del_track_button_;
	QPushButton*	move_track_up_button_;
    QPushButton*    add_keyframe_button_;
    QPushButton*    import_mocap_button_;
	QPushButton*	move_track_down_button_;
	QSlider*		frame_slider_;
	QLCDNumber*		end_frame_lcd_;	
	QComboBox*		speed_combox_;
	QPushButton*	bindpose_button_;
	
	AnimationTableView*		anim_table_view_;
	AnimationTrackScene*	track_scene_;
	AnimationTrackView*		track_view_;
    MocapImportDialog*      mocap_import_dialog_;

	QTimer*			timer_;			// ������ʱ��
	QStateMachine	state_machine_;	// ״̬�� ���ڹ����š���ͣ��״̬
	QState*			play_state_;    // ����״̬
	QState*			paused_state_;  // ��ͣ״̬
	bool			paused_;		// �Ƿ�����ͣ״̬ 
	bool			loop_;			// �Ƿ�ѭ������
	double          play_speed_;	// ˢ��ʱ�� Ĭ��1x�ٶ�
};

class QCustomPlot;
/************************************************************************/
/* ����ͨ���༭����                                                     */
/************************************************************************/
// ����maya��Graph Editor
class PoserWidget : public QWidget
{
    friend AnimationEditorWidget;
	Q_OBJECT
	
public:
	explicit PoserWidget(QWidget *parent = 0);

    void reset();
    void updateGraphData();
    void setNameChannelIndexMap(NameToChIdMap* name_chid);
    
    void updateChannelData(const Animation* animation);

private slots:
	void selectionChanged();
	void mousePress();
	void mouseWheel();
	void addGraph(); 
    void setCurChannel(const QModelIndex& index);

private:
	QTreeView*		skeleton_tree_view_;
	QCustomPlot*	channel_plotter_;
    NameToChIdMap*  name_channelindex_;     // ����-ͨ������ӳ���

    int cur_channel_; 
    // ��������ת���� ����poser_
    QVector<QVector<QQuaternion> > sampled_rot_keys_;

    QVector<double> time_;
    QVector<double> x_rot_;
    QVector<double> y_rot_;
    QVector<double> z_rot_;
};

/************************************************************************/
/* �����༭������                                                       */
/************************************************************************/
class AnimationEditorWidget : public QTabWidget
{
	Q_OBJECT

public:
	AnimationEditorWidget(QWidget *parent = 0); 
	~AnimationEditorWidget();

    void reset();
	void setAnimationTableModel(AnimationTableModel* model);// ���ö������ģ��
	void setSkeletonTreeModel(SkeletonTreeModel* model);	// ���ùǼ�����ģ��
	void setNameAnimationMap(NameToAnimMap* name_anim);     // ��������-����ӳ���
    void setNameChannelIndexMap(NameToChIdMap* name_chid);  // ��������-ͨ������ӳ���

    Animation* syntheticAnimation();

signals:
	void frameChanged(int);
	void bindposeRestored();	                       // �л���bindpose
//	void clipUpdated(AnimationClip*, AnimationTrack*); // �༭Ƭ��ʱ֪ͨ�����ؼ����� �����ºϳɶ���
    void mocapSelected(QString& , QString&);  // ��ѡ��mocap�����ļ�

private slots:
    void updateChannelData();

private:
	RemixerWidget*	remixer_;	// �����Զ����༭��
	PoserWidget*	poser_;		// ͨ���༭��
};
#endif // ANIMATION_EDITOR_WIDGET_H