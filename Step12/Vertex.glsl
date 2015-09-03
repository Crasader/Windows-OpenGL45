#version 450 core

layout (location = 0) in vec2 position;

uniform float width;
uniform float height;

void main()
{
	float x = position.x / width * 2.0 - 1.0;
	float y = -position.y / height * 2.0 + 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
