#version 400

layout (location = 0) in vec4 VertexPosition;
layout (location = 1) in vec4 VertexColor;

out vec4 Color;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * VertexPosition;
    Color = VertexColor;
}