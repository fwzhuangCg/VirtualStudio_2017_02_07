#version 400

in vec4 Color;

layout(location = 0) out vec4 FragColor;

void main()
{
	FragColor = Color;
	FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}