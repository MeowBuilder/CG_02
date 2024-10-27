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

const int WIN_X = 10, WIN_Y = 10;
const int WIN_W = 800, WIN_H = 800;

const glm::vec3 background_rgb = glm::vec3(1.0f, 1.0f, 1.0f);

GLUquadricObj* Sun;

GLUquadricObj* planet1;
GLUquadricObj* moon1;
glm::vec3 planet1_translate = { 1.5f,0.0f,0.0f };
glm::vec3 planet1_rotate;
glm::vec3 moon1_rotate;

GLUquadricObj* planet2;
GLUquadricObj* moon2;
glm::vec3 planet2_translate = { 1.5f,1.5f,0.0f };
glm::vec3 planet2_rotate;
glm::vec3 moon2_rotate;

GLUquadricObj* planet3;
GLUquadricObj* moon3;
glm::vec3 planet3_translate = { -1.5f,-1.5f,0.0f };
glm::vec3 planet3_rotate = { 30.0f,0.0f,0.0f };
glm::vec3 moon3_rotate;

float radius = 2.0f;

bool isCulling = true;
bool yRotate = false;
bool zRotate = false;

GLfloat mx = 0.0f;
GLfloat my = 0.0f;

int framebufferWidth, framebufferHeight;
GLuint shaderProgramID;

float world_y_rot = 0.0f;
float world_z_rot = 0.0f;
float y_rot = 1.0f;
float z_rot = 1.0f;

bool isSolid = true;
bool isortho = false;

glm::vec3 translate_all = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint Line_VAO;
GLuint Line_VBO;
std::vector<std::vector<float>> Planet_lines = { {},{},{} };
glm::vec3 line_color = { 0.0f,0.0f,0.0f };

glm::vec4 moonvertex = { 0,0,0,1.0f };
std::vector<float> moon_lines = { };
glm::vec3 moon_line_color = { 0.0f,0.0f,0.0f };

std::vector<glm::vec3> colors = { {1.0,0.0,0.0}, {0.5,0.5,0.0}, {0.0,1.0,0.0}, {0.0,0.5,0.5}, {0.0,0.0,1.0}, {0.5,0.0,0.5} };

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

bool Make_Shader_Program() {
	//세이더 코드 파일 불러오기
	const GLchar* vertexShaderSource = File_To_Buf("vertex.glsl");
	const GLchar* fragmentShaderSource = File_To_Buf("fragment.glsl");

	//세이더 객체 만들기
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//세이더 객체에 세이더 코드 붙이기
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	//세이더 객체 컴파일하기
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];

	//세이더 상태 가져오기
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << endl;
		return false;
	}

	//세이더 객체 만들기
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//세이더 객체에 세이더 코드 붙이기
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	//세이더 객체 컴파일하기
	glCompileShader(fragmentShader);
	//세이더 상태 가져오기
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << endl;
		return false;
	}

	//세이더 프로그램 생성
	shaderProgramID = glCreateProgram();
	//세이더 프로그램에 세이더 객체들을 붙이기
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	//세이더 프로그램 링크
	glLinkProgram(shaderProgramID);

	//세이더 객체 삭제하기
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//프로그램 상태 가져오기
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program 연결 실패\n" << errorLog << endl;
		return false;
	}
	//세이더 프로그램 활성화
	glUseProgram(shaderProgramID);

	return true;
}

void world_rotation(glm::mat4* TR) {
	*TR = glm::rotate(*TR, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
	*TR = glm::rotate(*TR, glm::radians(30.0f), glm::vec3(0.0, 1.0, 0.0));
	*TR = glm::translate(*TR, translate_all);
	*TR = glm::rotate(*TR, glm::radians(world_y_rot), glm::vec3(0.0, 1.0, 0.0));
	*TR = glm::rotate(*TR, glm::radians(world_z_rot), glm::vec3(0.0, 0.0, 1.0));
}

GLvoid drawScene()
{
	glClearColor(background_rgb.x, background_rgb.y, background_rgb.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	isCulling ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	isCulling ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	isSolid ? gluQuadricDrawStyle(Sun, GLU_FILL) : gluQuadricDrawStyle(Sun, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(planet1, GLU_FILL) : gluQuadricDrawStyle(planet1, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(moon1, GLU_FILL) : gluQuadricDrawStyle(moon1, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(planet2, GLU_FILL) : gluQuadricDrawStyle(planet2, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(moon2, GLU_FILL) : gluQuadricDrawStyle(moon2, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(planet3, GLU_FILL) : gluQuadricDrawStyle(planet3, GLU_LINE);
	isSolid ? gluQuadricDrawStyle(moon3, GLU_FILL) : gluQuadricDrawStyle(moon3, GLU_LINE);

	glUseProgram(shaderProgramID);

	glm::mat4 view = glm::mat4(1.0f);
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 cameraDir = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	view = glm::lookAt(cameraPos, cameraDir, cameraUp);
	unsigned viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 projection = glm::mat4(1.0f);
	if (isortho)
	{
		projection = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, -5.0f, 5.0f);
	}
	else
	{
		projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f);
		projection = glm::translate(projection, glm::vec3(0.0, 0.0, -5.0));
	}

	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projectionTransform");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

	glm::mat4 TR = glm::mat4(1.0f);
	world_rotation(&TR);
	unsigned modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, 1.0, 1.0, 0.0);
	gluSphere(Sun, 0.5, 50, 50);


	glBindVertexArray(Line_VAO);
	for (int i = 0; i < Planet_lines.size(); i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, Line_VBO);
		glBufferData(GL_ARRAY_BUFFER, Planet_lines[i].size() * sizeof(float), Planet_lines[i].data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
		glUniform3f(modelLocation, colors[i*2].x, colors[i * 2].y, colors[i * 2].z);

		glDrawArrays(GL_LINE_STRIP, 0, Planet_lines[i].size() / 6);
	}

	//행성1
	TR = glm::mat4(1.0f);
	world_rotation(&TR);

	TR = glm::translate(TR, planet1_translate);
	modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));


	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, colors[0].x, colors[0].y, colors[0].z);
	gluSphere(planet1, 0.25, 25, 25);


	glBindBuffer(GL_ARRAY_BUFFER, Line_VBO);
	glBufferData(GL_ARRAY_BUFFER, moon_lines.size() * sizeof(float), moon_lines.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, colors[1].x, colors[1].y, colors[1].z);
	glDrawArrays(GL_LINE_STRIP, 0, moon_lines.size() / 6);


	


	//위성1
	TR = glm::mat4(1.0f);
	world_rotation(&TR);

	TR = glm::translate(TR, planet1_translate);
	TR = glm::rotate(TR, glm::radians(moon1_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::translate(TR, glm::vec3(0.4, 0.0, 0.0));
	modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, colors[1].x, colors[1].y, colors[1].z);
	gluSphere(moon1, 0.1, 10, 10);


	//행성2
	TR = glm::mat4(1.0f);
	world_rotation(&TR);

	TR = glm::translate(TR, planet2_translate);
	modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, colors[2].x, colors[2].y, colors[2].z);
	gluSphere(planet2, 0.25, 25, 25);

	glBindBuffer(GL_ARRAY_BUFFER, Line_VBO);
	glBufferData(GL_ARRAY_BUFFER, moon_lines.size() * sizeof(float), moon_lines.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, colors[3].x, colors[3].y, colors[3].z);
	glDrawArrays(GL_LINE_STRIP, 0, moon_lines.size() / 6);

	//위성2
	TR = glm::mat4(1.0f);
	world_rotation(&TR);

	TR = glm::translate(TR, planet2_translate);
	TR = glm::rotate(TR, glm::radians(moon2_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::translate(TR, glm::vec3(0.4, 0.0, 0.0));
	modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, colors[3].x, colors[3].y, colors[3].z);
	gluSphere(moon2, 0.1, 10, 10);

	//행성3
	TR = glm::mat4(1.0f);
	world_rotation(&TR);

	TR = glm::translate(TR, planet3_translate);
	modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, colors[4].x, colors[4].y, colors[4].z);
	gluSphere(planet3, 0.25, 25, 25);

	glBindBuffer(GL_ARRAY_BUFFER, Line_VBO);
	glBufferData(GL_ARRAY_BUFFER, moon_lines.size() * sizeof(float), moon_lines.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, colors[5].x, colors[5].y, colors[5].z);
	glDrawArrays(GL_LINE_STRIP, 0, moon_lines.size() / 6);

	//위성3
	TR = glm::mat4(1.0f);
	world_rotation(&TR);

	TR = glm::translate(TR, planet3_translate);
	TR = glm::rotate(TR, glm::radians(moon3_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::translate(TR, glm::vec3(0.4, 0.0, 0.0));
	modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	glUniform3f(modelLocation, colors[5].x, colors[5].y, colors[5].z);
	gluSphere(moon3, 0.1, 10, 10);


	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

void make_moon_line() {
	glm::vec4 newvertex = { 0,0,0,1.0 };

	glm::mat4 TR = glm::mat4(1.0f);

	TR = glm::rotate(TR, glm::radians(moon1_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::translate(TR, glm::vec3(0.4, 0.0, 0.0));

	newvertex = TR * moonvertex;

	moon_lines.push_back(newvertex.x);
	moon_lines.push_back(newvertex.y);
	moon_lines.push_back(newvertex.z);
	moon_lines.push_back(moon_line_color.x);
	moon_lines.push_back(moon_line_color.y);
	moon_lines.push_back(moon_line_color.z);
}

GLvoid TimerFunction1(int value)
{
	glutPostRedisplay();
	if (yRotate) world_y_rot += y_rot;
	if (zRotate) world_z_rot += z_rot;

	planet1_translate.x = cos(glm::radians(planet1_rotate.x)) * radius;
	planet1_translate.z = sin(glm::radians(planet1_rotate.x)) * radius;
	planet1_rotate.x += 1.0f;
	moon1_rotate.y += 1.0f;


	planet2_translate.x = cos(glm::radians(planet2_rotate.x)) * radius;
	planet2_translate.y = cos(glm::radians(planet2_rotate.x)) * radius;
	planet2_translate.z = sin(glm::radians(planet2_rotate.x)) * radius;
	planet2_rotate.x += 1.0f;
	moon2_rotate.y += 1.0f;

	planet3_translate.x = -cos(glm::radians(planet3_rotate.x)) * radius;
	planet3_translate.y = cos(glm::radians(planet3_rotate.x)) * radius;
	planet3_translate.z = sin(glm::radians(planet3_rotate.x)) * radius;
	planet3_rotate.x += 1.0f;
	moon3_rotate.y += 1.0f;

	if (planet1_rotate.x <= 720.0f)
	{
		Planet_lines[0].push_back(planet1_translate.x);
		Planet_lines[0].push_back(planet1_translate.y);
		Planet_lines[0].push_back(planet1_translate.z);
		Planet_lines[0].push_back(line_color.x);
		Planet_lines[0].push_back(line_color.y);
		Planet_lines[0].push_back(line_color.z);
	}
	if (planet2_rotate.x <= 720.0f)
	{
		Planet_lines[1].push_back(planet2_translate.x);
		Planet_lines[1].push_back(planet2_translate.y);
		Planet_lines[1].push_back(planet2_translate.z);
		Planet_lines[1].push_back(line_color.x);
		Planet_lines[1].push_back(line_color.y);
		Planet_lines[1].push_back(line_color.z);
	}
	if (planet3_rotate.x <= 720.0f)
	{
		Planet_lines[2].push_back(planet3_translate.x);
		Planet_lines[2].push_back(planet3_translate.y);
		Planet_lines[2].push_back(planet3_translate.z);
		Planet_lines[2].push_back(line_color.x);
		Planet_lines[2].push_back(line_color.y);
		Planet_lines[2].push_back(line_color.z);
	}
	if (moon1_rotate.y <= 360.0f)
	{
		make_moon_line();
	}

	glutTimerFunc(10, TimerFunction1, 1);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	vector<int> new_opnenface = {};
	switch (key) {
	case 'h':
		isCulling = 1 - isCulling;
		break;
	case 'y':
		yRotate = !yRotate;
		y_rot = 0.5f;
		break;
	case 'Y':
		yRotate = !yRotate;
		y_rot = -0.5f;
		break;
	case 'z':
		zRotate = !zRotate;
		z_rot = 0.5f;
		break;
	case 'Z':
		zRotate = !zRotate;
		z_rot = -0.5f;
		break;
	case 'q':
		glutLeaveMainLoop();
		break;
	case 'p': //직각 / 원근투영
		isortho = !isortho;
		break;
	case 'm':
		isSolid = !isSolid;
		break;
	case 'w':
		translate_all.y += 0.1f;
		break;
	case 'a':
		translate_all.x -= 0.1f;
		break;
	case 's':
		translate_all.y -= 0.1f;
		break;
	case 'd':
		translate_all.x += 0.1f;
		break;
	case '+':
		translate_all.z += 0.1f;
		break;
	case '-':
		translate_all.z -= 0.1f;
		break;
	};
	glutPostRedisplay();
}

GLvoid specialKeyboard(int key, int x, int y) {
	switch (key)
	{
	default:
		break;
	}
}

int main(int argc, char** argv)
{
	//윈도우 생성
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(WIN_X, WIN_Y);
	glutInitWindowSize(WIN_W, WIN_H);
	glutCreateWindow("Example1");

	//GLEW 초기화하기
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

	glutTimerFunc(10, TimerFunction1, 1);

	Sun = gluNewQuadric();
	gluQuadricNormals(Sun, GLU_SMOOTH);
	gluQuadricOrientation(Sun, GLU_OUTSIDE);


	planet1 = gluNewQuadric();
	gluQuadricNormals(planet1, GLU_SMOOTH);
	gluQuadricOrientation(planet1, GLU_OUTSIDE);


	moon1 = gluNewQuadric();
	gluQuadricNormals(moon1, GLU_SMOOTH);
	gluQuadricOrientation(moon1, GLU_OUTSIDE);


	planet2 = gluNewQuadric();
	gluQuadricNormals(planet2, GLU_SMOOTH);
	gluQuadricOrientation(planet2, GLU_OUTSIDE);

	moon2 = gluNewQuadric();
	gluQuadricNormals(moon2, GLU_SMOOTH);
	gluQuadricOrientation(moon2, GLU_OUTSIDE);

	planet3 = gluNewQuadric();
	gluQuadricNormals(planet3, GLU_SMOOTH);
	gluQuadricOrientation(planet3, GLU_OUTSIDE);


	moon3 = gluNewQuadric();
	gluQuadricNormals(moon3, GLU_SMOOTH);
	gluQuadricOrientation(moon3, GLU_OUTSIDE);

	glGenVertexArrays(1, &Line_VAO);
	glBindVertexArray(Line_VAO);
	glGenBuffers(1, &Line_VBO);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(specialKeyboard);
	glutMainLoop();
}