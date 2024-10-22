#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <random>
#include <fstream>
#include <iterator>

using namespace std;

random_device seeder;
const auto seed = seeder.entropy() ? seeder() : time(nullptr);
mt19937 eng(static_cast<mt19937::result_type>(seed));
uniform_int_distribution<int> rand_obj(0, 4);

const int WIN_X = 10, WIN_Y = 10;
const int WIN_W = 800, WIN_H = 800;

const glm::vec3 background_rgb = glm::vec3(1.0f, 1.0f, 1.0f);

bool isCulling = true;
bool isFill = true;
float xRotateAni = 30.0f;
float yRotateAni = -30.0f;
float yRotateworld = -30.0f;
glm::vec3 scale_personal_left = { 0.1f,0.1f,0.1f };
glm::vec3 scale_personal_right = { 0.1f,0.1f,0.1f };
glm::vec3 scale_world_left = { 1.0f,1.0f,1.0f };
glm::vec3 scale_world_right = { 1.0f,1.0f,1.0f };

glm::vec3 left_start = { 1.0f,1.0f,1.0f};
glm::vec3 right_start = { 1.0f,1.0f,1.0f};

glm::vec3 translate_left = { -0.5f,0.0f,0.0f };
glm::vec3 translate_right = { 0.5f,0.0f,0.0f };

int spiral_left = 0;
int spiral_right = 0;

int selected = 0;
int rotateKey = 0;

bool anim5 = 0;

GLfloat mx = 0.0f;
GLfloat my = 0.0f;

int animationKey = 0;
std::vector<float> spiral = { 0.0f,0.0f,0.0f,0.0f,0.5f,0.5f };
float seta = 0.0f;
float radius = 0.05f;

float t = 0;

int framebufferWidth, framebufferHeight;
GLuint triangleVertexArrayObject;
GLuint shaderProgramID;
GLuint trianglePositionVertexBufferObjectID, triangleColorVertexBufferObjectID;
GLuint trianglePositionElementBufferObject;
GLuint Line_VAO, Line_VBO;
GLuint spiral_VAO, spiral_VBO;

GLuint triangleVertexArrayObject_2;
GLuint trianglePositionVertexBufferObjectID_2, triangleColorVertexBufferObjectID_2;
GLuint trianglePositionElementBufferObject_2;

GLUquadricObj* qobj;

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

int left_obj = 0;
int right_obj = 1;

char* File_To_Buf(const char* file)
{
	ifstream in(file, ios_base::binary);

	if (!in) {
		cerr << file << "파일 못찾음";
		exit(1);
	}

	in.seekg(0, ios_base::end);
	long len = in.tellg();
	char* buf = new char[len + 1];
	in.seekg(0, ios_base::beg);

	int cnt = -1;
	while (in >> noskipws >> buf[++cnt]) {}
	buf[len] = 0;

	return buf;
}

bool  Load_Object(const char* path) {
	vertexIndices.clear();
	uvIndices.clear();
	normalIndices.clear();
	vertices.clear();
	uvs.clear();
	normals.clear();

	ifstream in(path);
	if (!in) {
		cerr << path << "파일 못찾음";
		exit(1);
	}


	while (in) {
		string lineHeader;
		in >> lineHeader;
		if (lineHeader == "v") {
			glm::vec3 vertex;
			in >> vertex.x >> vertex.y >> vertex.z;
			vertices.push_back(vertex);
		}
		else if (lineHeader == "vt") {
			glm::vec2 uv;
			in >> uv.x >> uv.y;
			uvs.push_back(uv);
		}
		else if (lineHeader == "vn") {
			glm::vec3 normal;
			in >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (lineHeader == "f") {
			char a;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];

			for (int i = 0; i < 3; i++)
			{
				in >> vertexIndex[i] >> a >> uvIndex[i] >> a >> normalIndex[i];
				vertexIndices.push_back(vertexIndex[i] - 1);
				uvIndices.push_back(uvIndex[i] - 1);
				normalIndices.push_back(normalIndex[i] - 1);
			}
		}
	}

	return true;
}

bool Make_Shader_Program() {
	const GLchar* vertexShaderSource = File_To_Buf("vertex.glsl");
	const GLchar* fragmentShaderSource = File_To_Buf("fragment.glsl");

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << endl;
		return false;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << endl;
		return false;
	}

	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program 연결 실패\n" << errorLog << endl;
		return false;
	}
	glUseProgram(shaderProgramID);

	return true;
}

bool Set_VAO() {
	Load_Object("cube.obj");

	float color[] = {
	   0.5f, 0.0f, 0.5f,//4
	   0.0f, 0.0f, 1.0f,//0
	   0.0f, 0.0f, 0.0f,//3

	   0.5f, 0.0f, 0.5f,//4
	   0.0f, 0.0f, 0.0f,//3
	   1.0f, 0.0f, 0.0f,//7

	   0.0f, 1.0f, 0.0f,//2
	   0.5f, 0.5f, 0.0f,//6
	   1.0f, 0.0f, 0.0f,//7

	   0.0f, 1.0f, 0.0f,//2
	   1.0f, 0.0f, 0.0f,//7
	   0.0f, 0.0f, 0.0f,//3

	   0.0f, 0.5f, 0.5f,//1
	   1.0f, 1.0f, 1.0f,//5
	   0.0f, 1.0f, 0.0f,//2

	   1.0f, 1.0f, 1.0f,//5
	   0.5f, 0.5f, 0.0f,//6
	   0.0f, 1.0f, 0.0f,//2

	   0.0f, 0.0f, 1.0f,//0
	   0.5f, 0.0f, 0.5f,//4
	   0.0f, 0.5f, 0.5f,//1

	   0.5f, 0.0f, 0.5f,//4
	   1.0f, 1.0f, 1.0f,//5
	   0.0f, 0.5f, 0.5f,//1

	   0.5f, 0.0f, 0.5f,//4
	   1.0f, 0.0f, 0.0f,//7
	   1.0f, 1.0f, 1.0f,//5

	   1.0f, 0.0f, 0.0f,//7
	   0.5f, 0.5f, 0.0f,//6
	   1.0f, 1.0f, 1.0f,//5

	   0.0f, 0.0f, 1.0f,//0
	   0.0f, 0.5f, 0.5f,//1
	   0.0f, 1.0f, 0.0f,//2

	   0.0f, 0.0f, 1.0f,//0
	   0.0f, 1.0f, 0.0f,//2
	   0.0f, 0.0f, 0.0f,//3

	   0.0f, 0.0f, 0.0f,
	   0.0f, 0.0f, 0.0f,
	   0.0f, 0.0f, 0.0f,
	   0.0f, 0.0f, 0.0f
	};

	glGenVertexArrays(1, &Line_VAO);
	glBindVertexArray(Line_VAO);
	glGenBuffers(1, &Line_VBO);

	glGenVertexArrays(1, &spiral_VAO);
	glBindVertexArray(spiral_VAO);
	glGenBuffers(1, &spiral_VBO);

	glGenVertexArrays(1, &triangleVertexArrayObject);
	glBindVertexArray(triangleVertexArrayObject);


	glGenBuffers(1, &trianglePositionVertexBufferObjectID);
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &trianglePositionElementBufferObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglePositionElementBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	if (positionAttribute == -1) {
		cerr << "position 속성 설정 실패" << endl;
		return false;
	}
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionAttribute);

	glGenBuffers(1, &triangleColorVertexBufferObjectID);
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);

	GLint colorAttribute = glGetAttribLocation(shaderProgramID, "colorAttribute");
	if (colorAttribute == -1) {
		cerr << "color 속성 설정 실패" << endl;
		return false;
	}
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	glVertexAttribPointer(colorAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colorAttribute);

	glBindVertexArray(0);

	Load_Object("tetrahedron.obj");

	glGenVertexArrays(1, &triangleVertexArrayObject_2);
	glBindVertexArray(triangleVertexArrayObject_2);


	glGenBuffers(1, &trianglePositionVertexBufferObjectID_2);
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID_2);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &trianglePositionElementBufferObject_2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglePositionElementBufferObject_2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	if (positionAttribute == -1) {
		cerr << "position 속성 설정 실패" << endl;
		return false;
	}
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionAttribute);

	glGenBuffers(1, &triangleColorVertexBufferObjectID_2);
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID_2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);

	colorAttribute = glGetAttribLocation(shaderProgramID, "colorAttribute");
	if (colorAttribute == -1) {
		cerr << "color 속성 설정 실패" << endl;
		return false;
	}
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID_2);
	glVertexAttribPointer(colorAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colorAttribute);

	glBindVertexArray(0);


	return true;
}

GLvoid drawScene()
{
	glClearColor(background_rgb.x, background_rgb.y, background_rgb.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	vector<float> line = {
		-10.0f, 0.0f, 0.0f, 1.0f,0.0f,0.0f,
		10.0f, 0.0f, 0.0f, 1.0f,0.0f,0.0f,

		0.0f, -10.0f, 0.0f, 0.0f,1.0f,0.0f,
		0.0f, 10.0f, 0.0f, 0.0f,1.0f,0.0f,

		0.0f, 0.0f, -10.0f, 0.0f,0.0f,1.0f,
		0.0f, 0.0f, 10.0f, 0.0f,0.0f,1.0f
	};


	isCulling ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	isCulling ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
	isFill ? glPolygonMode(GL_FRONT_AND_BACK, GL_FILL) : glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glUseProgram(shaderProgramID);


	//좌표축
	glBindVertexArray(Line_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, Line_VBO);
	glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(float), line.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glm::mat4 TR = glm::mat4(1.0f);
	TR = glm::rotate(TR, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
	TR = glm::rotate(TR, glm::radians(yRotateworld), glm::vec3(0.0, 1.0, 0.0));
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawArrays(GL_LINES, 0, line.size() / 3);


	//스파이럴
	glBindVertexArray(spiral_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, spiral_VBO);
	glBufferData(GL_ARRAY_BUFFER, spiral.size() * sizeof(float), spiral.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_LINE_STRIP, 0, spiral.size() / 6);


	//오른쪽 도형
	TR = glm::mat4(1.0f);
	TR = glm::rotate(TR, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
	TR = glm::rotate(TR, glm::radians(yRotateworld), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::scale(TR, scale_world_right);
	TR = glm::translate(TR, translate_right);
	TR = glm::rotate(TR, glm::radians(xRotateAni), glm::vec3(1.0, 0.0, 0.0));
	TR = glm::rotate(TR, glm::radians(yRotateAni), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::scale(TR, scale_personal_right);
	modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

	switch (right_obj)
	{
	case 0:
		glBindVertexArray(triangleVertexArrayObject);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		break;
	case 1:
		glBindVertexArray(triangleVertexArrayObject_2);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		break;
	case 2:
		qobj = gluNewQuadric();
		gluQuadricDrawStyle(qobj, GLU_LINE);
		gluQuadricNormals(qobj, GLU_SMOOTH);
		gluQuadricOrientation(qobj, GLU_OUTSIDE);
		gluSphere(qobj, 1.5, 50, 50);
		break;
	case 3:
		qobj = gluNewQuadric();
		gluQuadricDrawStyle(qobj, GLU_LINE);
		gluQuadricNormals(qobj, GLU_SMOOTH);
		gluQuadricOrientation(qobj, GLU_OUTSIDE);
		gluCylinder(qobj, 1.0, 0.0, 1.0, 20, 8);
		break;
	case 4:
		qobj = gluNewQuadric();
		gluQuadricDrawStyle(qobj, GLU_LINE);
		gluQuadricNormals(qobj, GLU_SMOOTH);
		gluQuadricOrientation(qobj, GLU_OUTSIDE);
		gluCylinder(qobj, 1.0, 1.0, 2.0, 20, 8);
		break;
	default:
		break;
	}

	//왼쪽 도형

	TR = glm::mat4(1.0f);
	TR = glm::rotate(TR, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
	TR = glm::rotate(TR, glm::radians(yRotateworld), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::scale(TR, scale_world_left);
	TR = glm::translate(TR, translate_left);
	TR = glm::rotate(TR, glm::radians(xRotateAni), glm::vec3(1.0, 0.0, 0.0));
	TR = glm::rotate(TR, glm::radians(yRotateAni), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::scale(TR, scale_personal_left);
	modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

	switch (left_obj)
	{
	case 0:
		glBindVertexArray(triangleVertexArrayObject);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		break;
	case 1:
		glBindVertexArray(triangleVertexArrayObject_2);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		break;
	case 2:
		qobj = gluNewQuadric();
		gluQuadricDrawStyle(qobj, GLU_LINE);
		gluQuadricNormals(qobj, GLU_SMOOTH);
		gluQuadricOrientation(qobj, GLU_OUTSIDE);
		gluSphere(qobj, 1.5, 50, 50);
		break;
	case 3:
		qobj = gluNewQuadric();
		gluQuadricDrawStyle(qobj, GLU_LINE);
		gluQuadricNormals(qobj, GLU_SMOOTH);
		gluQuadricOrientation(qobj, GLU_OUTSIDE);
		gluCylinder(qobj, 1.0, 0.0, 1.0, 20, 8);
		break;
	case 4:
		qobj = gluNewQuadric();
		gluQuadricDrawStyle(qobj, GLU_LINE);
		gluQuadricNormals(qobj, GLU_SMOOTH);
		gluQuadricOrientation(qobj, GLU_OUTSIDE);
		gluCylinder(qobj, 1.0, 1.0, 2.0, 20, 8);
		break;
	default:
		break;
	}

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

GLvoid TimerFunction1(int value)
{
	glutPostRedisplay();
	if (rotateKey == 1)
		xRotateAni += 0.5f;
	if (rotateKey == 2)
		xRotateAni -= 0.5f;
	if (rotateKey == 3)
		yRotateAni += 0.5f;
	if (rotateKey == 4)
		yRotateAni -= 0.5f;
	if (rotateKey == 5)
		yRotateworld += 0.5f;
	if (rotateKey == 6)
		yRotateworld -= 0.5f;

	std::vector<float> newspiralpoint = {
		spiral[spiral.size() - 6],
		spiral[spiral.size() - 5],
		spiral[spiral.size() - 4],
		spiral[spiral.size() - 3],
		spiral[spiral.size() - 2],
		spiral[spiral.size() - 1],
	};

	switch (animationKey)
	{
	case 1: //스파이럴
		if (radius < 75)
		{
			newspiralpoint[0] = spiral[0] + (glm::radians(cos(seta)) * radius);
			newspiralpoint[2] = spiral[2] + (glm::radians(sin(seta)) * radius);

			for (int i = 0; i < 6; i++)
			{
				spiral.push_back(newspiralpoint[i]);
			}
			seta -= 0.1f;
			radius += 0.1f;

			spiral_left++;
		}

		if (radius >= 75)
		{
			if (spiral_left > 0)
			{
				translate_left.x = spiral[spiral_left * 6];
				translate_left.y = 0.0f;
				translate_left.z = spiral[(spiral_left * 6) + 2];
				spiral_left--;
			}

			if (spiral_right * 6 < (spiral.size() - 6))
			{
				translate_right.x = spiral[spiral_right * 6];
				translate_right.y = 0.0f;
				translate_right.z = spiral[spiral_right * 6 + 2];
				spiral_right++;
			}
		}
		break;
	case 2:
		translate_left = glm::vec3(1 - t, 1 - t, 1 - t) * left_start + glm::vec3(t, t, t) * right_start;
		translate_right = glm::vec3(1 - t, 1 - t, 1 - t) * right_start + glm::vec3(t, t, t) * left_start;
		t += 0.01f;
		if (t >= 1)
		{
			animationKey = 0;
			translate_left = right_start;
			translate_right = left_start;
		}
		break;
	case 3:
		translate_left = glm::vec3((1 - t)* (1 - t), (1 - t) * (1 - t), (1 - t) * (1 - t)) * left_start +
			glm::vec3((1 - t) * t, (1 - t) * t, (1 - t) * t) * (((left_start + right_start) / glm::vec3(2, 2, 2)) + glm::vec3(2, 2, 2)) +
			glm::vec3(t*t, t * t, t * t) * right_start;
		translate_right = glm::vec3((1 - t) * (1 - t), (1 - t) * (1 - t), (1 - t) * (1 - t)) * right_start +
			glm::vec3((1 - t) * t, (1 - t) * t, (1 - t) * t) * (((left_start + right_start) / glm::vec3(2, 2, 2)) + glm::vec3(-2, -2, -2)) +
			glm::vec3(t * t, t * t, t * t) * left_start;
		t += 0.01f;
		if (t >= 1)
		{
			animationKey = 0;
			translate_left = right_start;
			translate_right = left_start;
		}
		break;
	case 4:
		if (t < 1)
		{
			translate_left.x = (1 - t) * left_start.x + (t)*right_start.x;
			translate_right.x = (1 - t) * right_start.x + (t)*left_start.x;
			t += 0.01f;
		}
		else if (t < 2)
		{
			translate_left.x = right_start.x;
			translate_right.x = left_start.x;
			translate_left.y = (1 - (t - 1)) * left_start.y + (t - 1) * right_start.y;
			translate_right.y = (1 - (t - 1)) * right_start.y + (t - 1) * left_start.y;
			t += 0.01f;
		}
		else if (t < 3)
		{
			translate_left.y = right_start.y;
			translate_right.y = left_start.y;
			translate_left.z = (1 - (t - 2)) * left_start.z + (t - 2) * right_start.z;
			translate_right.z = (1 - (t - 2)) * right_start.z + (t - 2) * left_start.z;
			t += 0.01f;
		}
		else
		{
			animationKey = 0;
			translate_left = right_start;
			translate_right = left_start;
		}
		break;
	case 5:
		if (anim5)
		{
			scale_personal_left += glm::vec3(0.001f, 0.001f, 0.001f);
			scale_personal_right -= glm::vec3(0.001f, 0.001f, 0.001f);
			if (scale_personal_right.x < 0.0f)
			{
				anim5 = !anim5;
			}
		}
		else
		{
			scale_personal_left -= glm::vec3(0.001f, 0.001f, 0.001f);
			scale_personal_right += glm::vec3(0.001f, 0.001f, 0.001f);
			if (scale_personal_left.x < 0.0f)
			{
				anim5 = !anim5;
			}
		}
		xRotateAni += 0.5f;
		yRotateAni += 0.5f;
		yRotateworld += 0.5f;
		break;
	default:
		break;
	}

	glutTimerFunc(10, TimerFunction1, 1);
}

void change_obj() {
	left_obj = rand_obj(eng);
	right_obj = rand_obj(eng);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case '1':
		selected = 1;
		break;
	case '2':
		selected = 2;
		break;
	case 'h':
		isCulling = !isCulling;
		break;
	case 'x':
		rotateKey = 1;
		break;
	case 'X':
		rotateKey = 2;
		break;
	case 'y':
		rotateKey = 3;
		break;
	case 'Y':
		rotateKey = 4;
		break;
	case 'r':
		rotateKey = 5;
		break;
	case 'R':
		rotateKey = 6;
		break;
	case 'p':
		if (selected == 1)
		{
			scale_personal_left.x += 0.1f;
			scale_personal_left.y += 0.1f;
			scale_personal_left.z += 0.1f;
		}
		else if (selected == 2)
		{
			scale_personal_right.x += 0.1f;
			scale_personal_right.y += 0.1f;
			scale_personal_right.z += 0.1f;
		}
		break;
	case 'P':
		if (selected == 1)
		{
			scale_personal_left.x -= 0.1f;
			scale_personal_left.y -= 0.1f;
			scale_personal_left.z -= 0.1f;
		}
		else if (selected == 2)
		{
			scale_personal_right.x -= 0.1f;
			scale_personal_right.y -= 0.1f;
			scale_personal_right.z -= 0.1f;
		}
		break;
	case 'w':
		if (selected == 1)
		{
			scale_world_left.x += 0.1f;
			scale_world_left.y += 0.1f;
			scale_world_left.z += 0.1f;
		}
		else if (selected == 2)
		{
			scale_world_right.x += 0.1f;
			scale_world_right.y += 0.1f;
			scale_world_right.z += 0.1f;
		}
		break;
	case 'W':
		if (selected == 1)
		{
			scale_world_left.x -= 0.1f;
			scale_world_left.y -= 0.1f;
			scale_world_left.z -= 0.1f;
		}
		else if (selected == 2)
		{
			scale_world_right.x -= 0.1f;
			scale_world_right.y -= 0.1f;
			scale_world_right.z -= 0.1f;
		}
		break;
	case 'j':
		if (selected == 1)
		{
			translate_left.x += 0.1f;
		}
		else if (selected == 2)
		{
			translate_right.x += 0.1f;
		}
		break;
	case 'J':
		if (selected == 1)
		{
			translate_left.x -= 0.1f;
		}
		else if (selected == 2)
		{
			translate_right.x -= 0.1f;
		}
		break;
	case 'k':
		if (selected == 1)
		{
			translate_left.y += 0.1f;
		}
		else if (selected == 2)
		{
			translate_right.y += 0.1f;
		}
		break;
	case 'K':
		if (selected == 1)
		{
			translate_left.y -= 0.1f;
		}
		else if (selected == 2)
		{
			translate_right.y -= 0.1f;
		}
		break;
	case 'l':
		if (selected == 1)
		{
			translate_left.z += 0.1f;
		}
		else if (selected == 2)
		{
			translate_right.z += 0.1f;
		}
		break;
	case 'L':
		if (selected == 1)
		{
			translate_left.z -= 0.1f;
		}
		else if (selected == 2)
		{
			translate_right.z -= 0.1f;
		}
		break;
	case 'v': //애니메이션 1
		animationKey = 1;
		break;
	case 'b': //2
		right_start = translate_right;
		left_start = translate_left;
		t = 0;
		animationKey = 2;
		break;
	case 'n': //3
		right_start = translate_right;
		left_start = translate_left;
		t = 0;
		animationKey = 3;
		break;
	case 'm': //4
		right_start = translate_right;
		left_start = translate_left;
		t = 0;
		animationKey = 4;
		break;
	case ',': //5
		animationKey = 5;
		break;
	case 'f':
		isFill = !isFill;
		break;
	case 's':
		rotateKey = 0;
		animationKey = 0;
		break;
	case 'S':
		rotateKey = 0;
		animationKey = 0;
		xRotateAni = 30.0f;
		yRotateAni = -30.0f;
		yRotateworld = -30.0f;
		translate_left = { -0.5f,0.0f,0.0f };
		translate_right = { 0.5f,0.0f,0.0f };
		break;
	case 'c':
		change_obj();
		break;
	case 'C': //Clear
		spiral = { 0.0f,0.0f,0.0f,0.0f,0.5f,0.5f };
		seta = 0.0f;
		radius = 0.05f;

		spiral_left = 0;
		spiral_right = 0;
		translate_left = { -0.5f,0.0f,0.0f };
		translate_right = { 0.5f,0.0f,0.0f };
		break;
	case 'q':
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay();
}


void Mouse(int button, int state, int x, int y)
{
	GLfloat half_w = WIN_W / 2.0f;
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		mx = (x - half_w) / half_w;
		my = (half_w - y) / half_w;
	}
	Set_VAO();
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(WIN_X, WIN_Y);
	glutInitWindowSize(WIN_W, WIN_H);
	glutCreateWindow("Example1");

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Unable to initialize GLEW" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";

	if (!Make_Shader_Program()) {
		cerr << "Error: Shader Program 생성 실패" << endl;
		std::exit(EXIT_FAILURE);
	}

	if (!Set_VAO()) {
		cerr << "Error: VAO 생성 실패" << endl;
		std::exit(EXIT_FAILURE);
	}

	glutTimerFunc(10, TimerFunction1, 1);
	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(Mouse);
	glutMainLoop();
}