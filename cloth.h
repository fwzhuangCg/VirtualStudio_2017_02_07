#ifndef CLOTH_H
#define CLOTH_H

#include <QVector3D>
#include "scene_node.h"

// 物理常量
const double DAMPING = 0.01;							// 阻尼
const double TIME_STEP = 0.5;							// 时间步长
const double TIME_STEP_SQUARE = TIME_STEP * TIME_STEP;	// 时间步长平方
const int	CONSTRAINT_ITERATIONS = 15;					// 每帧中约束补偿（constraint satisfaction）迭代次数

// Pattern
// 2D Design

// 粒子（质点）
class Particle
{
public:
	Particle(const QVector3D& pos)
		: position_(pos), old_position_(pos), acceleration_(0,0,0), mass_(1), movable_(true), accumulated_normal_(0,0,0)
	{}

	// 施加作用力
	void addForce(const QVector3D& f)
	{
		acceleration_ += f/mass_;
	}

	// 时间片积分
	void timeStep()
	{
		if (movable_) {
			QVector3D temp = position_;
			position_ = position_ + (position_ - old_position_) * (1.0 - DAMPING) + acceleration_ * TIME_STEP_SQUARE; // verlet积分
			old_position_ = temp;
			acceleration_ = QVector3D(0, 0, 0);	// 积分完毕后 加速度归零
		}
	}

	// 加速度归零
	void resetAcceleration()
	{
		acceleration_ = QVector3D(0, 0, 0);
	}

	// 移动粒子
	void offsetPostion(const QVector3D& v)
	{
		if (movable_)
			position_ += v;
	}

	// 累加法线
	void addToNormal(const QVector3D& normal)
	{
		accumulated_normal_ += normal.normalized();
	}

	QVector3D position() const { return position_; }
	void setPosition(const QVector3D& pos) { position_ = pos; }
	bool movable() const { return movable_; }
	void setMovable(bool val) { movable_ = val; }
	QVector3D accumulaterNormal() const { return accumulated_normal_; }
	void resetAccumulatedNormal() { accumulated_normal_ = QVector3D(0, 0, 0); }

private:
	bool		movable_;				// 是否可移动 用于锚定布料
	float		mass_;					// 质量
	QVector3D	position_;				// 当前位置
	QVector3D	old_position_;			// 上一个时间步长的位置 用于Verlet数值积分
	QVector3D	acceleration_;			// 加速度
	QVector3D	accumulated_normal_;	// 累积法线
};

// 约束（弹簧）
class Constraint
{
public:
	Constraint(Particle *pt1, Particle* pt2)
		: p1(pt1), p2(pt2), rest_length_((pt1->position() - pt2->position()).length())
	{
	}

	// 约束补偿
	void satisfyConstraint()
	{
		QVector3D p1_to_p2 = p2->position() - p1->position();
		float current_distance = p1_to_p2.length();
		QVector3D correctionVector = p1_to_p2 * (1 - rest_length_/current_distance);	// 将两质点拉回自然长度之向量
		p1->offsetPostion( 0.5 * correctionVector);
		p2->offsetPostion(-0.5 * correctionVector);
	}

	// 弹簧连接的两个质点
	Particle*	p1;
	Particle*	p2;

private:
	float rest_length_;	// 弹簧自然长度（无拉伸 无压缩 受力平衡） 止动长度
};

// 布料
// 3D Simulation
class Cloth : public SceneNode
{
public:
	Cloth(void);
	~Cloth(void);

private:
	// 网格

	// 物理参数
};

#endif // CLOTH_H
