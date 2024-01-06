/*
 Abstract: A pass-through fragment shader.
 */

#version 150

in vec4 color;
out vec4 fragColor;

void main()
{
	fragColor = color;
}
