#version 450 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color_in;

out vec3 color;

void main()
{
	color = color_in;
    gl_Position = vec4(position + diff, 0.0, 1.0);
}
