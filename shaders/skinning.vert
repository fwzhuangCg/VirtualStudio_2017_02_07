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
uniform mat4 MVP;
uniform mat4 JointMatrices[128]; // an avatar contains at most 128 joints
uniform vec4 JointDQs[256]; // dual quaternions, each represented by 2 vec4
uniform bool Skinning; // skinning or not
uniform bool DQSkinning;   // dual quaternion skinning or linear blend skinning

// linear blend skinning
void LBS(in vec3 normal, in vec3 position, out vec3 newNormal, out vec3 newPosition)
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

// helper function -- transform dual quaternion to matrix
mat4 dualQuatToMatrix(vec4 Qn, vec4 Qd)
{
    mat4 M;
    float len2 = dot(Qn, Qn);
    float  w = Qn.w,  x = Qn.x,  y = Qn.y,  z = Qn.z;
    float t0 = Qd.w, t1 = Qd.x, t2 = Qd.y, t3 = Qd.z;

    M[0][0] = w*w + x*x - y*y - z*z;
    M[0][1] = 2*x*y + 2*w*z;
    M[0][2] = 2*x*z - 2*w*y;
    M[0][3] = 0;

    M[1][0] = 2*x*y - 2*w*z;
    M[1][1] = w*w + y*y - x*x - z*z;
    M[1][2] = 2*y*z + 2*w*x;
    M[1][3] = 0;

    M[2][0] = 2*x*z + 2*w*y;
    M[2][1] = 2*y*z - 2*w*x;
    M[2][2] = w*w + z*z - x*x - y*y;
    M[2][3] = 0;

    M[3][0] = -2*t0*x + 2*w*t1 - 2*t2*z + 2*y*t3;
    M[3][1] = -2*t0*y + 2*t1*z - 2*x*t3 + 2*w*t2;
    M[3][2] = -2*t0*z + 2*x*t2 + 2*w*t3 - 2*t1*y;
    M[3][3] = len2;

    M /=  len2;

    return M;
}

// dual quaternion blend
void DLB(in vec3 normal, in vec3 position, out vec3 newNormal, out vec3 newPosition)
{
    float yc = 1.0, zc = 1.0, wc = 1.0;

    if ( dot( JointDQs[int(JointIndices.x) * 2], JointDQs[int(JointIndices.y) * 2] ) < 0.0 )
        yc = -1.0;
    if ( dot( JointDQs[int(JointIndices.x) * 2], JointDQs[int(JointIndices.z) * 2] ) < 0.0 )
        zc = -1.0;
    if ( dot( JointDQs[int(JointIndices.x) * 2], JointDQs[int(JointIndices.w) * 2] ) < 0.0 )
        wc = -1.0;

    vec4 blendDQ[2]; // the blended dual quaternion
    vec4 jointWeights = JointWeights;
    //make sure all the weights add up to 1.0
    jointWeights.w = 1.0 - dot( jointWeights.xyz, vec3(1.0, 1.0, 1.0) );

    blendDQ[0]  = JointDQs[int(JointIndices.x) * 2 + 0] * jointWeights.x;
    blendDQ[1]  = JointDQs[int(JointIndices.x) * 2 + 1] * jointWeights.x;

    blendDQ[0] += yc * JointDQs[int(JointIndices.y) * 2 + 0] * jointWeights.y;
    blendDQ[1] += yc * JointDQs[int(JointIndices.y) * 2 + 1] * jointWeights.y;

    blendDQ[0] += zc * JointDQs[int(JointIndices.z) * 2 + 0] * jointWeights.z;
    blendDQ[1] += zc * JointDQs[int(JointIndices.z) * 2 + 1] * jointWeights.z;

    blendDQ[0] += wc * JointDQs[int(JointIndices.w) * 2 + 0] * jointWeights.w;
    blendDQ[1] += wc * JointDQs[int(JointIndices.w) * 2 + 1] * jointWeights.w;

    mat4 transformMatrix = dualQuatToMatrix(blendDQ[0], blendDQ[1]);

    // transform normal and position
    newPosition = (transformMatrix * vec4(position, 1.0)).xyz;
    newNormal = (transformMatrix * vec4(normal, 0.0)).xyz;
}

void main()
{
	vec3 transformedNorm;
	vec3 transformedPos;
    if (Skinning)
	{
        if (DQSkinning)
        {
            DLB(VertexNormal, VertexPosition, transformedNorm, transformedPos);
        }
        else
        {
            LBS(VertexNormal, VertexPosition, transformedNorm, transformedPos);
        }

		Normal = normalize( NormalMatrix * transformedNorm );
		Position = vec3( ModelViewMatrix * vec4(transformedPos, 1.0) );
		gl_Position = MVP * vec4(transformedPos, 1.0);
	}
	else
	{
		Normal = normalize( NormalMatrix * VertexNormal );
		Position = vec3( ModelViewMatrix * vec4(VertexPosition, 1.0) );
		gl_Position = MVP * vec4(VertexPosition, 1.0);
	}

    TexCoord = VertexTexCoord;
}
