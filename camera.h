#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>

#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>

#include "scene_node.h"

// forward declaration
struct Sphere;

// Arcball camera
class Camera : public SceneNode
{
public:
	explicit Camera( SceneNode* parent = 0 );

	QVector3D eye() const { return eye_; }
	void setEye(const QVector3D& eye) { eye_= eye; }
	void setEye(qreal x, qreal y, qreal z) { eye_= QVector3D(x, y, z); }
	QVector3D center() const { return center_; }
	void setCenter(const QVector3D& c) { center_ = c; }
	void setCenter(qreal x, qreal y, qreal z) { center_ = QVector3D(x, y, z); }
	QVector3D viewDirection();

	int viewportWidth() const { return viewport_width_; }
	void setViewportWidth(int width) { viewport_width_ = width; }
	int viewportHeight() const { return viewport_height_; }
	void setViewportHeight(int height) { viewport_height_ = height; }
	void setNearPlane(float n) { near_ = n; }
	void setFarPlane(float f) { far_ = f; }
	void fitBoundingSphere(const Sphere& sphere);

	QMatrix4x4 viewMatrix();
	QMatrix4x4 projectionMatrix();

	void rotate(const QPoint& prevPos, const QPoint& curPos);
	void pan(float dx, float dy);
	void zoom(float factor);  

	static QVector3D screenToBall(const QPoint& pt);
	static QQuaternion calcRotation(const QVector3D& vFrom, const QVector3D& vTo);

private:

	static int viewport_width_;
	static int viewport_height_;

	QQuaternion rotation_;

	QVector3D eye_;
	QVector3D center_;
	float distance_exponent_;
	float fovy_;
	float near_;
	float far_;
};

#endif // CAMERA_H
