#version 400

subroutine vec4 ShadeModelType();
subroutine uniform ShadeModelType shadeModel;

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;
uniform sampler2D Tex1;

struct LightInfo {
	vec4 Position;
	vec3 Intensity;
};

struct LightInfo2 {
	vec4 Direction;
	vec3 Intensity;
};

struct MaterialInfo {
	vec3 Ka;
	vec3 Kd;
	vec3 Ks;
	float Shininess;
};

uniform LightInfo Light;
uniform LightInfo2 Light2;
uniform MaterialInfo Material;
uniform vec4 Color;
uniform mat4 ViewMatrix;

layout( location = 0 ) out vec4 FragColor;

// Helper function
void phongModel( vec3 pos, vec3 norm, out vec3 ambAndDiff, out vec3 spec)
{
	vec3 n = normalize( norm );
	vec3 s = normalize(vec3(ViewMatrix * Light2.Direction))/*normalize( vec3( Light.Position ) - pos )*/;
	vec3 v = normalize( vec3(-pos) );
	vec3 h = normalize( v + s );
	vec3 ambient = Light2.Intensity * Material.Ka;
	float sDotN = max( dot( s, n ), 0.0 );
	vec3 diffuse = Light2.Intensity * Material.Kd * sDotN;
	spec = vec3(0.0);
	if (sDotN > 0.0)
		spec = Light2.Intensity * Material.Ks * pow( max( dot( h, n ), 0.0 ), Material.Shininess);
	ambAndDiff = ambient + diffuse;
}

// ShadeModelType Subroutines
subroutine( ShadeModelType )
vec4 shade()
{
	vec3 ambAndDiff, spec;
	vec4 texColor = texture( Tex1, TexCoord );
	phongModel(Position, Normal, ambAndDiff, spec);
	return vec4(ambAndDiff, 1.0) * texColor + vec4(spec, 1.0);
}

subroutine( ShadeModelType )
vec4 shadeWithNoMaterial()
{
	vec3 ambAndDiff, spec;
	phongModel(Position, Normal, ambAndDiff, spec);
	return vec4(ambAndDiff, 1.0) * Color + vec4(spec, 1.0);
}

subroutine( ShadeModelType )
vec4 shadeWithPureColor()
{
	return Color;
}

void main()
{
	FragColor = shadeModel();
}
