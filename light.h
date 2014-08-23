#ifndef LIGHT_H
#define LIGHT_H

#include <QColor>
#include <QVector3D>

#include "scene_node.h"

enum LightType
{
	LIGHTTYPE_AMBIENT,
	LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_POINT,
	LIGHTTYPE_SPOT
};

/************************************************************************/
/* 光源                                                                 */
/************************************************************************/
class Light : public SceneNode
{
public:
	QColor		color_;					// 颜色
	QVector3D	spot_direction_;		// 方向
	float		spot_cutoff_;			// cutoff angle of spot light
	float		spot_exponent_;			// falloff exponent of spot light
	float		constant_attenuation_;	// constant attenuation factor for point and spot light
	float		lineaer_attenuation_;	// linear attenuation factor for point and spot light
	float		quadratic_attenuation_;	// quadratic attenuation factor for point and spot light

	Light()
		: SceneNode(),
        color_(0.5f, 0.5f, 0.5f, 1.0f),
		spot_direction_(0.0f, 0.0f, -1.0f),
		spot_cutoff_(180.0f),
		spot_exponent_(0.0f),
		constant_attenuation_(1.0f),
		lineaer_attenuation_(0.0f),
		quadratic_attenuation_(0.0f),
		light_type_(LIGHTTYPE_POINT)
	{}

	~Light() { destroy(); }

protected:
	LightType	light_type_;

public:
	LightType getLightType()			{ return light_type_; }
	void setLightType(LightType type)	{ light_type_ = type; }

private:
	void destroy();
};
#endif // LIGHT_H