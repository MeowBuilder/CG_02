#version 330 core

uniform mat4 transform;		
layout (location = 0) in vec3 positionAttribute;
in vec3 colorAttribute;
out vec3 passColorAttribute;

void main()
{
	gl_Position = transform * vec4(positionAttribute, 1.0);
	passColorAttribute = colorAttribute;
};