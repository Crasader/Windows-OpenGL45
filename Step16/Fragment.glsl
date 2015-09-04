#version 450 core

in vec2 uv;

uniform sampler2D texture;

out vec4 out_color;

void main()
{
	vec4 color = texture2D(texture, uv);
    out_color = vec4(color.r * 0.6, color.g * 0.8, color.b * 0.8, 1.0);
}
