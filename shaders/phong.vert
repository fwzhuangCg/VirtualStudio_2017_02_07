#version 400

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec4 JointIndices;
layout (location = 4) in vec4 JointWeights;

out vec3 Position;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 MVP;
uniform mat4 JointMatrices[128]; // an avatar contains at most 128 joints
uniform bool GPUSkinning;   // CPU or GPU skinning

void lbs(in vec3 normal, in vec3 position, out vec3 newNormal, out vec3 newPosition)
{
	vec4 jointWeights = JointWeights;
	//make sure all the weights add up to 1.0
	jointWeights.w = 1.0 - dot( jointWeights.xyz, vec3(1.0, 1.0, 1.0) );

	// linear blend
	// a vertex can be affected by at most 4 joints
	mat4 transformMatrix = jointWeights.x * JointMatrices[int(JointIndices.x)];
	transformMatrix += jointWeights.y * JointMatrices[int(JointIndices.y)];
	transformMatrix += jointWeights.z * JointMatrices[int(JointIndices.z)];
	transformMatrix += jointWeights.w * JointMatrices[int(JointIndices.w)];

	// transform normal and position
	newPosition = (transformMatrix * vec4(position, 1.0)).xyz;
	newNormal = (transformMatrix * vec4(normal, 0.0)).xyz;
}

void main()
{
	vec3 transformedNorm;
	vec3 transformedPos;
	if (GPUSkinning)
	{
		lbs(VertexNormal, VertexPosition, transformedNorm, transformedPos);
		Normal = normalize( NormalMatrix * transformedNorm );
		Position = vec3( ModelViewMatrix * vec4(transformedPos, 1.0) );
		gl_Position = MVP * vec4(transformedPos, 1.0);
	}
	else
	{
		TexCoord = VertexTexCoord;
		Normal = normalize( NormalMatrix * VertexNormal );
		Position = vec3( ModelViewMatrix * vec4(VertexPosition, 1.0) );
		gl_Position = MVP * vec4(VertexPosition, 1.0);
	}	
}
