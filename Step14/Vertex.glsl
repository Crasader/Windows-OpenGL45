#version 450 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv_in;

uniform float width;
uniform float height;
uniform float img_width;
uniform float img_height;

out vec2 uv;

void main()
{
	uv = uv_in;

	float dx = (width - img_width) / 2.0;
	float dy = (height - img_height) / 2.0;
	float x = (position.x + dx) / width * 2.0 - 1.0;
	float y = -(position.y + dy) / height * 2.0 + 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
