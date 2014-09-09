#include "animation_editor_widget.h"

#include <algorithm>

#include <QtGui>
#include <QGraphicsSceneDragDropEvent>
#include <QApplication>
#include <QtWidgets/QtWidgets>

#include <qcustomplot.h>

#include "mocap_import_dialog.h"

/************************************************************************/
/* 动画轨道场景                                                          */
/************************************************************************/
AnimationTrackScene::AnimationTrackScene(RemixerWidget* remixer, QObject *parent /*= 0*/)
    : QGraphicsScene(parent), 
    remixer_(remixer),
    start_frame_(0), 
    cur_frame_(0), 
    end_frame_(0), 
    cur_track_(-1),
    name_animation_(nullptr),
    synthetic_animation_(new Animation())
{
    setItemIndexMethod(QGraphicsScene::NoIndex);
    cur_frame_pen_ = new QPen(QColor("orchid"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    drop_pos_hint_pen_ = new QPen(QColor("lightgray"), 2, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
    end_frame_pen_ = new QPen(QColor("gold"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    cur_track_brush_ = new QBrush(QColor("skyblue"), Qt::SolidPattern);

    createContextMenu();
    connect(delete_clip_action_, SIGNAL(triggered()), this, SLOT(deleteClip()));
    connect(this, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
}

AnimationTrackScene::~AnimationTrackScene()
{
    delete synthetic_animation_;
}

void AnimationTrackScene::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
{
    if (event->mimeData()->hasFormat("application/x-animation")) 
    {
        // 仅在有轨道时允许拖放片段
        if (!tracks_.isEmpty())
        {
            drop_pos_hint_pen_->setColor(Qt::red);
            update();
            event->accept();
        }
        else
        {
            event->setAccepted(false);
        }
    }
    else 
    {
        event->ignore();
    }
}

void AnimationTrackScene::dragLeaveEvent( QGraphicsSceneDragDropEvent *event )
{
    drop_pos_hint_pen_->setColor(QColor("lightgray"));
    update();
    event->accept();
}

void AnimationTrackScene::dragMoveEvent( QGraphicsSceneDragDropEvent *event )
{
    if (event->mimeData()->hasFormat("application/x-animation") && 
        event->scenePos().x() >= 0 && 
        event->scenePos().y() >= 0 ) 
    {
        drop_pos_ = event->scenePos();
        int track_index = (int)(drop_pos_.y() / AnimationTrack::TRACK_HEIGHT);
        bool valid = true;
        if (track_index < tracks_.size())
        {
            // 检查拖放位置是否与其他片段重叠
            foreach (AnimationClip* clip, tracks_[track_index]->clips_)
            {
                if (clip->contains(clip->mapFromScene(drop_pos_))) 
                {
                    valid = false;
                    break;
                }
            }
        }
        else
        {
            valid = false;
        }
        event->setDropAction(Qt::MoveAction);
        event->setAccepted(valid);
    }
    else 
    {
        event->ignore();
    }
    update();
}

void AnimationTrackScene::dropEvent( QGraphicsSceneDragDropEvent *event )
{
    if (event->mimeData()->hasFormat("application/x-animation")) 
    {
        QByteArray clipData = event->mimeData()->data("application/x-animation");
        QDataStream stream(&clipData, QIODevice::ReadOnly);
        QString text;
        double duration;
        stream >> text >> duration;
        AnimationClip* clip = addClip(text, drop_pos_);

        drop_pos_hint_pen_->setColor(QColor("lightgray"));
        update();

        event->setDropAction(Qt::MoveAction);
        event->accept();
		remixer_->frameChanged(0);
    }
    else 
    {
        event->ignore();
    }
}

AnimationClip* AnimationTrackScene::addClip(const QString& name, const QPointF& pos)
{
    // 根据动画片段名称查询片断对应的原始动画
    Animation* anim = findAnimation(name);
    Q_ASSERT(anim);	
    // 计算片段y坐标
    int y = qMin(static_cast<int>(drop_pos_.y() / AnimationTrack::TRACK_HEIGHT), tracks_.size() - 1) * AnimationTrack::TRACK_HEIGHT; 
    // 将片段放置到对应的轨道 我们根据片段中心y位置来判断所属轨道
    AnimationTrack* track = findTrackByYPos(y + AnimationTrack::TRACK_HEIGHT / 2); 
    AnimationClip* clip = new AnimationClip(context_menu_, this, anim, track, drop_pos_.x(), y);

    updateSyntheticAnim(remixer_->playSpeed());
    updateEndFrame();
    adjustSize();
    updateCurTrack();
    remixer_->updateFrameRangeUI();
    // update();
    return clip;
}

void AnimationTrackScene::deleteClip()
{
    for (QGraphicsItem *item : selectedItems()) 
    {
        if (item->type() == AnimationClip::Type)
        {
            AnimationClip* clip = qgraphicsitem_cast<AnimationClip *>(item);
            if (clip != nullptr)
            {
                AnimationTrack* track = findTrackByYPos(clip->y());
                track->delClip(clip);
                removeItem(item);

                updateSyntheticAnim(remixer_->playSpeed());
                updateEndFrame();
                adjustSize();
                remixer_->updateFrameRangeUI();
            }
        }
    }
}

void AnimationTrackScene::drawBackground( QPainter *painter, const QRectF &rect )
{
    Q_UNUSED(rect);

    double frame_pixel_width = AnimationClip::SECOND_WIDTH * 0.001 * AnimationClip::SAMPLE_SLICE;
    int padding = 10;
    // 绘制当前轨道高亮背景
    painter->setPen(Qt::NoPen);
    painter->setBrush(*cur_track_brush_);
    painter->drawRect(0, cur_track_ * AnimationTrack::TRACK_HEIGHT, end_frame_ * frame_pixel_width, AnimationTrack::TRACK_HEIGHT);
    
    // 绘制拖放点
    painter->setPen(*drop_pos_hint_pen_);
    int y1 = qMin(static_cast<int>(drop_pos_.y() / AnimationTrack::TRACK_HEIGHT), tracks_.size() - 1)  * AnimationTrack::TRACK_HEIGHT; 
    painter->drawLine(drop_pos_.x(), y1, drop_pos_.x(), y1 + AnimationTrack::TRACK_HEIGHT);

    // 绘制时间线
    painter->setPen(*cur_frame_pen_);
    painter->setBrush(cur_frame_pen_->color());
    // SECOND_WIDTH = 100 pixels --> 1ms = 0.1 pixels --> SAMPLE_SLICE = 16ms = 1.6 pixels  
    painter->drawRect(cur_frame_ * frame_pixel_width, 0, cur_frame_pen_->width(), AnimationTrack::TRACK_HEIGHT * tracks_.size() + padding); 

    // 结束时间线
    painter->setPen(*end_frame_pen_);
    painter->setBrush(end_frame_pen_->color());
    painter->drawRect(end_frame_ * frame_pixel_width, 0, end_frame_pen_->width(), AnimationTrack::TRACK_HEIGHT * tracks_.size() + padding);

    // 绘制轨道线
    painter->setPen(palette().foreground().color());
    for (int j = 1; j <= tracks_.size(); ++j)
        painter->drawLine(0, AnimationTrack::TRACK_HEIGHT * j, width(), AnimationTrack::TRACK_HEIGHT * j);
}

Animation* AnimationTrackScene::findAnimation(const QString& name)
{
    auto it = name_animation_->find(name);
    Q_ASSERT(it != name_animation_->end());
    return it.value();
}

AnimationTrack* AnimationTrackScene::findTrackByYPos( qreal y )
{
    int i = y / AnimationTrack::TRACK_HEIGHT;
    Q_ASSERT(i >= 0 && i < tracks_.size());
    return tracks_[i];
}

int AnimationTrackScene::endFrame() const
{
    return end_frame_;
}

int AnimationTrackScene::endTime() const
{
    return end_frame_ * AnimationClip::SAMPLE_SLICE;
}

bool AnimationTrackScene::isEmpty() const
{
    return tracks_.isEmpty();
}

Animation* AnimationTrackScene::syntheticAnimation()
{
    return synthetic_animation_;
}

void AnimationTrackScene::setStartFrame(int frame)
{
    start_frame_ = frame;
}

void AnimationTrackScene::setCurrentFrame(int frame)
{
    cur_frame_ = frame;
}

void AnimationTrackScene::setEndFrame(int frame)
{
    end_frame_ = frame;
    updateEndFrame();
}

void AnimationTrackScene::setNameAnimationMap(NameToAnimMap* name_anim)
{
    name_animation_ = name_anim;
}

void AnimationTrackScene::updateSyntheticAnim(float play_speed/* = 1.0f*/)
{
    synthetic_animation_->clear();
    foreach(AnimationTrack* track, tracks_) 
    {
        if (!track->isEmpty()) 
        {
            foreach(AnimationClip* clip, track->clips_)
            {
                 synthetic_animation_->addKeyframes(clip->channelList(), 
                     clip->startTime(), 
                     clip->length(), 
                     clip->ticksPerSecond(),
                     play_speed, 
                     track->weight_);
            }
        }
    }
    // 按时间顺序排列关键帧数据 添加首帧
    for(AnimationChannel& channel : synthetic_animation_->channels)
    {  
        if (!channel.position_keys.isEmpty())
        {
            qSort(channel.position_keys.begin(), channel.position_keys.end(), vecKeyCmp);
            channel.position_keys.prepend(channel.position_keys[0]);
            channel.position_keys[0].time = 0;
        }       

        if (!channel.rotation_keys.isEmpty())
        {
            qSort(channel.rotation_keys.begin(), channel.rotation_keys.end(), quatKeyCmp);
            channel.rotation_keys.prepend(channel.rotation_keys[0]);
            channel.rotation_keys[0].time = 0;
        }       

        if (channel.scaling_keys.isEmpty())
        {
            qSort(channel.scaling_keys.begin(), channel.scaling_keys.end(), vecKeyCmp);
            channel.scaling_keys.prepend(channel.scaling_keys[0]);
            channel.scaling_keys[0].time = 0;
        } 
    }    
    emit syntheticAnimationUpdated();
}

AnimationTrack* AnimationTrackScene::addTrack()
{
    AnimationTrack* new_track = new AnimationTrack(this);
    tracks_.append(new_track);
    connect(new_track, SIGNAL(clipMoved()), this, SLOT(onClipMoved()));
    
    updateEndFrame();
    adjustSize();
    update();
    return new_track;
}

void AnimationTrackScene::delTrack()
{
    if (cur_track_ >= 0 && cur_track_ < tracks_.size())
    {
        delete tracks_[cur_track_];
        tracks_.removeAt(cur_track_);
        if (cur_track_ == tracks_.size())
        {
            --cur_track_;
        }
        else
        {
            // 将其后的轨道全部提高
            for (int i = cur_track_; i < tracks_.size(); ++i)
                tracks_[i]->moveUp();
        }
    }
    
    updateEndFrame();
    adjustSize();
    updateCurTrack();
    remixer_->updateFrameRangeUI();
    update();
}

void AnimationTrackScene::moveUpTrack()
{
    if (cur_track_ > 0)
    {
        int pre_track = cur_track_-1;
        tracks_[pre_track]->moveDown();
        tracks_[cur_track_]->moveUp();
        tracks_.swap(pre_track, cur_track_);
        --cur_track_;
    }
    updateCurTrack();
    update();
}

void AnimationTrackScene::moveDownTrack()
{
    if (cur_track_ >= 0 && cur_track_ < tracks_.size()-1)
    {
        int nxt_track = cur_track_+1;
        tracks_[nxt_track]->moveUp();
        tracks_[cur_track_]->moveDown();
        tracks_.swap(nxt_track, cur_track_);
        ++cur_track_;
    }
    updateCurTrack();
    update();
}

void AnimationTrackScene::updateEndFrame()
{
    end_frame_ = 0;
    foreach(const AnimationTrack* track, tracks_)
        end_frame_ = qMax(end_frame_, track->endTime() / AnimationClip::SAMPLE_SLICE);
}

void AnimationTrackScene::updateCurTrack()
{
    QList<QGraphicsItem*> items = selectedItems();
    if (items.count() == 1)
    {
        AnimationClip* clip = dynamic_cast<AnimationClip*>(items.first());
        if (clip)
        {
            cur_track_ = clip->scenePos().y() / AnimationTrack::TRACK_HEIGHT;
        }
    }
}

void AnimationTrackScene::adjustSize()
{
    double frame_pixel_width = AnimationClip::SECOND_WIDTH * 0.001 * AnimationClip::SAMPLE_SLICE;
    int width = qMax(static_cast<int>(INITIAL_WIDTH), static_cast<int>(end_frame_ * frame_pixel_width));
    int height = qMax(static_cast<int>(INITIAL_HEIGHT), static_cast<int>(tracks_.size() * AnimationTrack::TRACK_HEIGHT));
    const int padding = 10;
    setSceneRect(0, 0, width+padding, height+padding);
}

void AnimationTrackScene::onClipMoved()
{
    // 1.防止片段重叠（这一步已经在AnimationTrack中完成）
    // 2.更新终止帧
    updateEndFrame();
    // 3.调整轨道编辑区大小
    adjustSize();
    // 4.更新合成动画
    updateSyntheticAnim(remixer_->playSpeed());
    // 5.更新动画混合器控件
    remixer_->updateFrameRangeUI();
    update();
}

void AnimationTrackScene::onSelectionChanged()
{
    updateCurTrack();
    update();
}

void AnimationTrackScene::reset()
{
    qDeleteAll(tracks_.begin(), tracks_.end());
    tracks_.clear();
    start_frame_ = end_frame_ = cur_frame_ = 0;
    cur_track_ = -1;
    setSceneRect(0, 0, INITIAL_WIDTH, INITIAL_HEIGHT);
    
    synthetic_animation_->clear();
}

void AnimationTrackScene::createContextMenu()
{
    delete_clip_action_ = new QAction(QIcon(":images/cross.png"), tr("&Delete"), this);
    delete_clip_action_->setShortcut(tr("Delete"));
    delete_clip_action_->setStatusTip(tr("Delete animation clip"));
    context_menu_ = new QMenu(tr("&Editor"));
    context_menu_->addAction(delete_clip_action_);
}

/************************************************************************/
/* 动画轨道视图                                                         */
/************************************************************************/
AnimationTrackView::AnimationTrackView( AnimationTrackScene* scene, QWidget *parent /*= 0*/ )
    : scene_(scene), 
    QGraphicsView(parent)
{
    setScene(scene);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setAcceptDrops(true);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setStyleSheet("background: lightgray;");
}

void AnimationTrackView::setCurrentFrame( int frame )
{
    scene_->setCurrentFrame(frame);
    update();
}
/************************************************************************/
/* 动画表格视图                                                          */
/************************************************************************/
AnimationTableView::AnimationTableView( AnimationTableModel* model /*= 0*/, QWidget *parent /*= 0*/ )
{
    Q_UNUSED(model);
    Q_UNUSED(parent);
    setAlternatingRowColors(true);
    setSelectionBehavior(QTableView::SelectRows);
    setSelectionMode(QTableView::SingleSelection);
    resizeColumnsToContents();
    horizontalHeader()->setStretchLastSection(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void AnimationTableView::mousePressEvent( QMouseEvent *event )
{
    if (event->button() == Qt::LeftButton)
        start_pos_ = event->pos();
    QTableView::mousePressEvent(event);
}

void AnimationTableView::mouseMoveEvent( QMouseEvent *event )
{
    if (event->buttons() & Qt::LeftButton) 
    {
        int distance = (event->pos() - start_pos_).manhattanLength();
        if (distance >= QApplication::startDragDistance()) 
        {
            QModelIndex index = indexAt(start_pos_);
            if (index.isValid()) 
            {
                QModelIndexList list;
                list << index;
                QDrag *drag = new QDrag(this);
                drag->setMimeData(model()->mimeData(list));
                drag->exec();
            }
        }
    }
    QTableView::mouseMoveEvent(event);
}

/************************************************************************/
/* 动画混合部件                                                          */
/************************************************************************/
RemixerWidget::RemixerWidget( QWidget *parent /*= 0*/ )
    : QWidget(parent), 
    paused_(true), 
    loop_(true),
    play_speed_(1.0f)
{
    createWidgets();
    createSceneView();
    createLayout();
    createStates();

    setAcceptDrops(true);

    state_machine_.setInitialState(play_state_);
    QTimer::singleShot(0, &state_machine_, SLOT(start()));

    // 动画时间线
    timer_ = new QTimer(this);
	timer_->start(/*AnimationClip::SAMPLE_SLICE*/(1.f / play_speed_) * 10.f);
    createConnections();

    updateTrackEditUI();
}

RemixerWidget::~RemixerWidget()
{
    delete timer_;
}

QPushButton * RemixerWidget::createToolButton( const QString &toolTip, const QIcon &icon, const char *member )
{
    QPushButton* button = new QPushButton(this);
    button->setToolTip(toolTip);
    button->setIcon(icon);
    button->setIconSize(QSize(16, 16));
    connect(button, SIGNAL(clicked()), this, member);

    return button;
}

void RemixerWidget::createWidgets()
{
    play_control_layout_ = new QHBoxLayout;
    loop_button_ = createToolButton(tr("Loop"), QIcon(":/images/control_repeat_blue.png"), SLOT(loop()));
    loop_button_->setCheckable(true);
    loop_button_->setChecked(loop_);
    start_button_ = createToolButton(tr("Start"), QIcon(":/images/control_start_blue.png"), SLOT(start()));
    play_pause_button_ = createToolButton(tr("Play/Pause"), QIcon(":/images/control_play_blue.png"), SLOT(playpause()));
    end_button_ = createToolButton(tr("End"), QIcon(":/images/control_end_blue.png"), SLOT(end()));
    rewind_button_ = createToolButton(tr("Rewind"), QIcon(":/images/control_rewind_blue.png"), SLOT(rewind()));
    add_keyframe_button_ = createToolButton(tr("Add keyframe"), QIcon(":/images/add_keyframe.png"), SLOT(addKeyframe()));
    import_mocap_button_ = createToolButton(tr("Import mocap"), QIcon(":/images/import.png"), SLOT(showImportMocapDialog()));
    play_control_layout_->addWidget(loop_button_);
    play_control_layout_->addWidget(start_button_);
    play_control_layout_->addWidget(play_pause_button_);
    play_control_layout_->addWidget(end_button_);
    play_control_layout_->addWidget(rewind_button_);

    current_frame_lcd_ = new QLCDNumber(this);		
    current_frame_lcd_->setToolTip(tr("Current Frame"));	
    play_control_layout_->addWidget(current_frame_lcd_);
    fast_forward_button_ = createToolButton(tr("Fast Forward"), QIcon(":/images/control_fastforward_blue.png"), SLOT(ffw()));
    play_control_layout_->addWidget(fast_forward_button_);
    play_control_layout_->addWidget(add_keyframe_button_);
    play_control_layout_->addWidget(import_mocap_button_);

    add_track_button_ = createToolButton(tr("Add Track"), QIcon(":/images/add.png"), SLOT(addTrack()));
    del_track_button_ = createToolButton(tr("Delete Track"), QIcon(":/images/cross.png"), SLOT(delTrack()));
    move_track_up_button_ = createToolButton(tr("Move Track Up"), QIcon(":images/arrow_up.png"), SLOT(moveTrackUp()));
    move_track_down_button_ = createToolButton(tr("Move Track Down"), QIcon(":images/arrow_down.png"), SLOT(moveTrackDown()));

    frame_slider_ = new QSlider(Qt::Horizontal);
    frame_slider_->setRange(0, 0);
    frame_slider_->setTickInterval(5);
    frame_slider_->setTickPosition(QSlider::TicksBelow);
    frame_slider_->setMinimumSize(400, 55);
    frame_slider_->setEnabled(paused_);

    end_frame_lcd_ = new QLCDNumber(this);	
    end_frame_lcd_->setToolTip(tr("Total Frame Number"));
    end_frame_lcd_->setMaximumHeight(25);

    speed_combox_ = new QComboBox(this);
    QStringList speed;
    speed << "0.5x" << "1x" << "2x";
    speed_combox_->addItems(speed);
    speed_combox_->setCurrentIndex(1);

    bindpose_button_ = createToolButton(tr("Restore to Bindpose"), QIcon(":/images/bindpose.png"), SIGNAL(bindposeRestored()));
    mocap_import_dialog_ = new MocapImportDialog;
}

void RemixerWidget::createSceneView()
{
    anim_table_view_ = new AnimationTableView(nullptr, this);
    anim_table_view_->setMinimumWidth(350);

    track_scene_ = new AnimationTrackScene(this);
    // INITIAL_WIDTH = 1000 INITIAL_HEIGHT = 90 这两个数值是不断试验后取得的经验值 主要是让轨道初始时可全部显示出来
    track_scene_->setSceneRect(0, 0, AnimationTrackScene::INITIAL_WIDTH, AnimationTrackScene::INITIAL_HEIGHT); 
    track_view_ = new AnimationTrackView(track_scene_, this);
}

void RemixerWidget::createLayout()
{
    QGridLayout* main_layout = new QGridLayout;
    main_layout->addLayout(play_control_layout_, 0, 0, 1, 1);
    main_layout->addWidget(anim_table_view_, 1, 0, 1, 1);

    QWidget* track_editor = new QWidget;
    QGridLayout* track_editor_layout = new QGridLayout;
    track_editor_layout->addWidget(add_track_button_, 0, 0, 1, 1);
    track_editor_layout->addWidget(del_track_button_, 0, 1, 1, 1);
    track_editor_layout->addWidget(move_track_up_button_, 0, 2, 1, 1);
    track_editor_layout->addWidget(move_track_down_button_, 0, 3, 1, 1);

    track_editor_layout->addWidget(frame_slider_, 0, 4, 1, 1);
    track_editor_layout->addWidget(end_frame_lcd_, 0, 5, 1, 1);
    track_editor_layout->addWidget(speed_combox_, 0, 6, 1, 1);
    track_editor_layout->addWidget(bindpose_button_, 0, 7, 1, 1);
    track_editor_layout->addWidget(track_view_,1, 0, 1, 8);
    track_editor->setLayout(track_editor_layout);

    main_layout->addWidget(track_editor, 0, 1, 2, 1);
    setLayout(main_layout);
}

void RemixerWidget::createConnections()
{
    connect(frame_slider_, SIGNAL(valueChanged(int)), this, SLOT(setFrame(int)));

    connect(this, SIGNAL(frameChanged(int)), frame_slider_, SLOT(setValue(int)));
    connect(this, SIGNAL(frameChanged(int)), current_frame_lcd_, SLOT(display(int)));
    connect(this, SIGNAL(frameChanged(int)), track_view_, SLOT(setCurrentFrame(int)));

    connect(track_scene_, SIGNAL(syntheticAnimationUpdated()), this, SIGNAL(syntheticAnimationUpdated()));
    connect(timer_, SIGNAL(timeout()), this, SLOT(updateAnimation()));
    connect(speed_combox_, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSpeed(int)));
    connect(mocap_import_dialog_, SIGNAL(mocapSelected(QString& , QString&)), this, SIGNAL(mocapSelected(QString& , QString&)));
}

void RemixerWidget::createStates()
{
    play_state_ = new QState(&state_machine_);
    play_state_->assignProperty(play_pause_button_, "icon", QIcon(":/images/control_play_blue.png"));
    paused_state_ = new QState(&state_machine_);
    paused_state_->assignProperty(play_pause_button_, "icon", QIcon(":/images/control_pause_blue.png"));
    play_state_->addTransition(play_pause_button_, SIGNAL(clicked()), paused_state_);
    paused_state_->addTransition(play_pause_button_, SIGNAL(clicked()), play_state_);
}

void RemixerWidget::addTrack()		
{
    track_scene_->addTrack();
    track_view_->update();

    updateTrackEditUI();
}

void RemixerWidget::delTrack()		
{
    int ret = QMessageBox::warning(this, tr("Remixer"),
        tr("Are you sure to delete this track ?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (ret == QMessageBox::Yes)
    {
        track_scene_->delTrack();
        track_view_->update();
    }

    updateTrackEditUI();
}

void RemixerWidget::moveTrackUp()		
{
    track_scene_->moveUpTrack();
    track_view_->update();
}

void RemixerWidget::moveTrackDown()	
{
    track_scene_->moveDownTrack();
    track_view_->update();
}

void RemixerWidget::loop()		
{
    loop_ = !loop_;
    loop_button_->setChecked(loop_);
}

void RemixerWidget::start()			
{
    track_scene_->cur_frame_ = track_scene_->start_frame_;
    emit frameChanged(track_scene_->cur_frame_);
}

void RemixerWidget::playpause()		
{
    paused_ = !paused_;
    track_scene_->clearSelection();
    // 更新UI
    frame_slider_->setEnabled(paused_);
    start_button_->setEnabled(paused_);
    end_button_->setEnabled(paused_);
    rewind_button_->setEnabled(paused_);
    fast_forward_button_->setEnabled(paused_);

    updateTrackEditUI();
    anim_table_view_->setEnabled(paused_); 
}

void RemixerWidget::end()				
{
    track_scene_->cur_frame_ = track_scene_->end_frame_;
    emit frameChanged(track_scene_->cur_frame_);
}

void RemixerWidget::rewind()			
{
    track_scene_->cur_frame_ = qMax(track_scene_->start_frame_, track_scene_->cur_frame_-1);
    emit frameChanged(track_scene_->cur_frame_);
}

void RemixerWidget::ffw()				
{
    track_scene_->cur_frame_ = qMin(track_scene_->end_frame_, track_scene_->cur_frame_+1);
    emit frameChanged(track_scene_->cur_frame_);
}

void RemixerWidget::updateAnimation()
{
    if (!paused_) 
    {
        if (loop_)
            track_scene_->cur_frame_ = (track_scene_->cur_frame_ + 1) % (track_scene_->end_frame_ - track_scene_->start_frame_ + 1);
        else
            track_scene_->cur_frame_ = qMin(track_scene_->end_frame_, track_scene_->cur_frame_ + 1);
        emit frameChanged(track_scene_->cur_frame_);
    }
}

void RemixerWidget::setFrame(int frame)
{
    if (track_scene_->cur_frame_ != frame) 
    {
        track_scene_->cur_frame_ = frame;
        emit frameChanged(track_scene_->cur_frame_);
    }
}

void RemixerWidget::updateFrameRangeUI()
{
    frame_slider_->setRange(track_scene_->start_frame_, track_scene_->end_frame_);
    end_frame_lcd_->display(track_scene_->end_frame_);
}

void RemixerWidget::updateTrackEditUI()
{
    add_track_button_->setEnabled(paused_);
    del_track_button_->setEnabled(paused_ && !track_scene_->isEmpty());
    move_track_up_button_->setEnabled(paused_ && !track_scene_->isEmpty());
    move_track_down_button_->setEnabled(paused_ && !track_scene_->isEmpty());
//    import_mocap_button_->setEnabled(); 注意 当场景中有avatar时 import_mocap_button_才enable
    track_view_->setInteractive(paused_);
}

void RemixerWidget::changeSpeed( int index )
{
    switch(index) 
    {
    case 0: 
        play_speed_ = 0.5f; 
        break;
    case 1: 
        play_speed_ = 1.0f; 
        break;
    case 2: 
        play_speed_ = 2.0f; 
        break;
    default: 
        qDebug() << "Invalid index"; 
    }
	timer_->start((1.f / play_speed_) * 10.f);
}

double RemixerWidget::playSpeed() const
{
    return play_speed_;
}

void RemixerWidget::addKeyframe()
{

}

void RemixerWidget::showImportMocapDialog()
{
    mocap_import_dialog_->show();
}

/************************************************************************/
/* 动画通道编辑部件                                                      */
/************************************************************************/
PoserWidget::PoserWidget( QWidget *parent /*= 0*/ )
    : QWidget(parent),
    cur_channel_(-1)
{
    skeleton_tree_view_ = new QTreeView(this);
    skeleton_tree_view_->setSelectionMode(QTreeView::SingleSelection);
    skeleton_tree_view_->setSelectionBehavior(QTreeView::SelectRows);
    skeleton_tree_view_->setAnimated(true);
    skeleton_tree_view_->setMaximumWidth(300);

    channel_plotter_ = new QCustomPlot(this);
    channel_plotter_->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                                    QCP::iSelectLegend | QCP::iSelectPlottables);
    channel_plotter_->xAxis->setRange(0, 10000);
    channel_plotter_->yAxis->setRange(-180, 180);
    channel_plotter_->axisRect()->setupFullAxesBox();

    //channel_plotter_->plotLayout()->insertRow(0);
    //channel_plotter_->plotLayout()->addElement(0, 0, new QCPPlotTitle(channel_plotter_, "Animation Channel"));
    channel_plotter_->xAxis->setLabel("time");
    channel_plotter_->yAxis->setLabel("rotation");
    channel_plotter_->legend->setVisible(true);
    channel_plotter_->legend->setSelectableParts(QCPLegend::spItems);

    addGraph();

    QHBoxLayout* main_layout = new QHBoxLayout;
    main_layout->addWidget(skeleton_tree_view_);
    main_layout->addWidget(channel_plotter_);

    setLayout(main_layout);
    // 选择通道
    connect(skeleton_tree_view_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(setCurChannel(const QModelIndex&)));
    // connect slot that ties some axis selections together (especially opposite axes):
    connect(channel_plotter_, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
    // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
    connect(channel_plotter_, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress()));
    connect(channel_plotter_, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));

    // make bottom and left axes transfer their ranges to top and right axes:
    connect(channel_plotter_->xAxis, SIGNAL(rangeChanged(QCPRange)), channel_plotter_->xAxis2, SLOT(setRange(QCPRange)));
    connect(channel_plotter_->yAxis, SIGNAL(rangeChanged(QCPRange)), channel_plotter_->yAxis2, SLOT(setRange(QCPRange)));
}

void PoserWidget::selectionChanged()
{
  /*
   normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
   the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
   and the axis base line together. However, the axis label shall be selectable individually.
   
   The selection state of the left and right axes shall be synchronized as well as the state of the
   bottom and top axes.
   
   Further, we want to synchronize the selection of the graphs with the selection state of the respective
   legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
   or on its legend item.
  */
  
  // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (channel_plotter_->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || channel_plotter_->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      channel_plotter_->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || channel_plotter_->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    channel_plotter_->xAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    channel_plotter_->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  // make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (channel_plotter_->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || channel_plotter_->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      channel_plotter_->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || channel_plotter_->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    channel_plotter_->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    channel_plotter_->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  
  // synchronize selection of graphs with selection of corresponding legend items:
  for (int i=0; i<channel_plotter_->graphCount(); ++i)
  {
    QCPGraph *graph = channel_plotter_->graph(i);
    QCPPlottableLegendItem *item = channel_plotter_->legend->itemWithPlottable(graph);
    if (item->selected() || graph->selected())
    {
      item->setSelected(true);
      graph->setSelected(true);
    }
  }
}

void PoserWidget::mousePress()
{
    // if an axis is selected, only allow the direction of that axis to be dragged
    // if no axis is selected, both directions may be dragged

    if (channel_plotter_->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        channel_plotter_->axisRect()->setRangeDrag(channel_plotter_->xAxis->orientation());
    else if (channel_plotter_->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        channel_plotter_->axisRect()->setRangeDrag(channel_plotter_->yAxis->orientation());
    else
        channel_plotter_->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}

void PoserWidget::mouseWheel()
{
    // if an axis is selected, only allow the direction of that axis to be zoomed
    // if no axis is selected, both directions may be zoomed

    if (channel_plotter_->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        channel_plotter_->axisRect()->setRangeZoom(channel_plotter_->xAxis->orientation());
    else if (channel_plotter_->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        channel_plotter_->axisRect()->setRangeZoom(channel_plotter_->yAxis->orientation());
    else
        channel_plotter_->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}

void PoserWidget::addGraph()
{
    QPen graphPen(Qt::red);
    graphPen.setWidthF(2.0);

    // x_rot curve
    channel_plotter_->addGraph();
    channel_plotter_->graph(0)->setName(QString("rotation x"));
    channel_plotter_->graph(0)->setData(time_, x_rot_);
    channel_plotter_->graph(0)->setLineStyle(QCPGraph::lsLine);
    channel_plotter_->graph(0)->setScatterStyle(QCPScatterStyle::ssDisc);
    channel_plotter_->graph(0)->setPen(graphPen);

    // y_rot curve
    channel_plotter_->addGraph();
    channel_plotter_->graph(1)->setName(QString("rotation y"));
    channel_plotter_->graph(1)->setData(time_, y_rot_);
    channel_plotter_->graph(1)->setLineStyle(QCPGraph::lsLine);
    channel_plotter_->graph(1)->setScatterStyle(QCPScatterStyle::ssDisc);
    graphPen.setColor(Qt::green);
    channel_plotter_->graph(1)->setPen(graphPen);

    // z_rot curve
    channel_plotter_->addGraph();
    channel_plotter_->graph(2)->setName(QString("rotation z"));
    channel_plotter_->graph(2)->setData(time_, z_rot_);
    channel_plotter_->graph(2)->setLineStyle(QCPGraph::lsLine);
    channel_plotter_->graph(2)->setScatterStyle(QCPScatterStyle::ssDisc);
    graphPen.setColor(Qt::blue);
    channel_plotter_->graph(2)->setPen(graphPen);

    channel_plotter_->replot();
}

void PoserWidget::reset()
{
    time_.clear();
    x_rot_.clear();
    y_rot_.clear();
    z_rot_.clear();
}

void PoserWidget::updateGraphData()
{
    if (cur_channel_ < 0)
        return;

    reset();
    channel_plotter_->replot();

    if (sampled_rot_keys_.isEmpty())
        return;

    for (int i = 0; i < sampled_rot_keys_[cur_channel_].size(); ++i)
    {
        time_.append(i * AnimationClip::SAMPLE_SLICE);
        // (yaw, pitch, roll)
        QVector3D euler_angles = quatToEulerAngles(sampled_rot_keys_[cur_channel_][i]);
        x_rot_.append(euler_angles.z());
        y_rot_.append(euler_angles.x());
        z_rot_.append(euler_angles.y());
    }
    // update
    channel_plotter_->graph(0)->setData(time_, x_rot_);
    channel_plotter_->graph(1)->setData(time_, y_rot_);
    channel_plotter_->graph(2)->setData(time_, z_rot_);
    if (!time_.isEmpty())
        channel_plotter_->xAxis->setRange(0, qMin(10000.0, time_.last()));
}

void PoserWidget::setNameChannelIndexMap(NameToChIdMap* name_chid)
{
    name_channelindex_ = name_chid;
}

void PoserWidget::updateChannelData(const Animation* animation)
{
    if (animation == nullptr)
        return;

    int end_time = animation->ticks / animation->ticks_per_second * 1000;
    sampled_rot_keys_.clear();
    sampled_rot_keys_.resize(animation->channels.size());
    for (int channel_index = 0; channel_index < animation->channels.size(); ++channel_index)
    {
        const QVector<QuaternionKey>& original_rot_keys = animation->channels[channel_index].rotation_keys;

        double time_in_tick;
        for (int t = 0; t <= end_time; t += AnimationClip::SAMPLE_SLICE)
        {
            time_in_tick = t * 0.001 * animation->ticks_per_second;
            QQuaternion present_rotation(1, 0, 0, 0);
            if (!original_rot_keys.empty()) 
            {
                int frame = 0;
                while (frame < original_rot_keys.size() - 1) 
                {
                    if (time_in_tick < original_rot_keys[frame+1].time)
                        break;
                    ++frame;
                }

                // SLERP
                unsigned int next_frame = (frame+1) % original_rot_keys.size();
                const QuaternionKey& key = original_rot_keys[frame];
                const QuaternionKey& next_key = original_rot_keys[next_frame];
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
            sampled_rot_keys_[channel_index].append(present_rotation);
        }
    }

    updateGraphData();
    channel_plotter_->replot();
}

void PoserWidget::setCurChannel(const QModelIndex& index)
{
    auto it = name_channelindex_->find(index.data(0).toString());
    cur_channel_ = -1;
    if (it != name_channelindex_->end())
        cur_channel_ = it.value();
    updateGraphData();
    channel_plotter_->replot();
#ifdef _DEBUG
    qDebug() << "Select " << index.data(0).toString() << " " << cur_channel_;
#endif
}

/************************************************************************/
/* 动画编辑器部件                                                        */
/************************************************************************/
AnimationEditorWidget::AnimationEditorWidget( QWidget *parent /*= 0*/ )
    : QTabWidget(parent)
{
    remixer_ = new RemixerWidget(this);
    poser_ = new PoserWidget(this);

    addTab(remixer_, tr("Remixer"));
    addTab(poser_, tr("Poser"));

    setAcceptDrops(true);
    connect(remixer_, SIGNAL(frameChanged(int)), this, SIGNAL(frameChanged(int)));
    connect(remixer_, SIGNAL(bindposeRestored()), this, SIGNAL(bindposeRestored()));
    connect(remixer_, SIGNAL(mocapSelected(QString& , QString&)), this, SIGNAL(mocapSelected(QString& , QString&)));
    connect(remixer_, SIGNAL(syntheticAnimationUpdated()), this, SLOT(updateChannelData()));
}

AnimationEditorWidget::~AnimationEditorWidget()
{
    delete remixer_;
    delete poser_;
}

void AnimationEditorWidget::reset()
{
    Q_ASSERT(remixer_);
    Q_ASSERT(poser_);
    remixer_->track_scene_->reset();
    poser_->reset();
}

void AnimationEditorWidget::setAnimationTableModel( AnimationTableModel* model )
{
    Q_ASSERT(remixer_);
    remixer_->anim_table_view_->setModel(model);
}

void AnimationEditorWidget::setSkeletonTreeModel( SkeletonTreeModel* model )
{
    Q_ASSERT(poser_);
    poser_->skeleton_tree_view_->setModel(model);
}

void AnimationEditorWidget::setNameAnimationMap( NameToAnimMap* name_anim )
{
    Q_ASSERT(remixer_);
    remixer_->track_scene_->setNameAnimationMap(name_anim);
}

void AnimationEditorWidget::setNameChannelIndexMap(NameToChIdMap* name_chid)
{
    Q_ASSERT(poser_);
    poser_->setNameChannelIndexMap(name_chid);
}

Animation* AnimationEditorWidget::syntheticAnimation()
{
    Q_ASSERT(remixer_);
    return remixer_->track_scene_->syntheticAnimation();
}

void AnimationEditorWidget::updateChannelData()
{
    poser_->updateChannelData(remixer_->track_scene_->syntheticAnimation());
}

