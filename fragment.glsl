#version 330 core

in vec3 passColorAttribute;
out vec4 fragmentColor;

void main()
{
	fragmentColor = vec4(passColorAttribute, 1.0);
};