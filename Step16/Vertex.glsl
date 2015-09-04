#version 450 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv_in;

uniform float width;
uniform float height;

out vec2 uv;

void main()
{
	uv = uv_in;

	float x = (position.x) / width * 2.0 - 1.0;
	float y = -(position.y) / height * 2.0 + 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
