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
#include <random>

using namespace std;

random_device seeder;
const auto seed = seeder.entropy() ? seeder() : time(nullptr);
mt19937 eng(static_cast<mt19937::result_type>(seed));
uniform_real_distribution<float> rand_location(-4, 4);
uniform_real_distribution<float> rand_size(0.5, 2.5);

const int WIN_X = 10, WIN_Y = 10;
const int WIN_W = 800, WIN_H = 800;

const glm::vec3 background_rgb = glm::vec3(0.0f, 0.0f, 0.0f);
const std::vector<glm::vec3> colors = {
	{1.0, 0.0, 0.0},
	{0.0, 1.0, 0.0},
	{0.0, 0.0, 1.0},
	{1.0, 1.0, 0.0},
	{1.0, 0.0, 1.0},
	{0.0, 1.0, 1.0},

	{0,0,0.501961},
	{0.098039, 0.098039, 0.439216},
	{0.282353, 0.239216, 0.545098},
};

const glm::vec3 Box_border = { 10.0,10.0,10.0 };

// ť�� �� ���� ����
glm::vec3 cubePosition = { 0.0f, 0.0f, 0.0f }; // ť�� �ʱ� ��ġ
glm::vec3 cubeVelocity = { 0.0f, 0.0f, 0.0f };  // �ʱ� �ӵ�
glm::vec3 cubeRotationAxis = { 0.0f, 0.0f, 1.0f }; // ȸ�� ��
float cubeRotationAngle = 0.0f;                     // ȸ�� ����

glm::vec3 gravity = glm::vec3(0.0f, -0.98f, 0.0f); // �߷� ����
glm::vec3 inclineNormal; // ���� ǥ���� ���� (�� �����Ӹ��� ���)

// ��ü�� ǥ�� ������ �����ϴ� ����
std::vector<glm::vec3> surfaceNormals;

bool isOnIncline = false;                          // ť�갡 ���鿡 �����ߴ��� Ȯ��

bool isCulling = true;
bool is_reverse = true;

GLfloat mx = 0.0f;
GLfloat my = 0.0f;

int framebufferWidth, framebufferHeight;
GLuint shaderProgramID;
GLuint triangleVertexArrayObject;
GLuint trianglePositionVertexBufferObjectID;
GLuint trianglePositionElementBufferObject;

GLuint triangleVertexArrayObject_reversed;

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

glm::vec3 cameraPos = { 0.0f,0.0f,15.0f };
glm::vec3 camera_rotate = { 0.0f,0.0f,0.0f };
bool camera_rotate_y = false;
float camera_y_seta = 0.5f;

glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

//��ֹ� ���� ����
std::vector<glm::vec3> Block_location = { {},{},{} };
std::vector<glm::vec3> Block_scale = { {3,3,3},{2,2,2},{1,1,1} };
glm::vec3 Block_Color = { 0.1,0.3,1.0 };

//�� ���� ������
std::vector<glm::vec3> Ball_location = {};
std::vector<glm::vec3> Ball_velocity = {};
glm::vec3 Ball_Color = { 0.1,1.0,0.3 };

bool is_on = false;
int on_index = -1;

void set_cube(int face_index, glm::mat4* TR);
void set_block(int block_index, glm::mat4* TR);

char* File_To_Buf(const char* file)
{
	ifstream in(file, ios_base::binary);

	if (!in) {
		cerr << file << "���� ��ã��";
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
		cerr << path << "���� ��ã��";
		exit(1);
	}

	//vector<char> lineHeader(istream_iterator<char>{in}, {});

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


	if (path == "cube_reversefor22.obj")
	{
		for (size_t i = 0; i < vertexIndices.size(); i += 3) {
			glm::vec3 v0 = vertices[vertexIndices[i]];
			glm::vec3 v1 = vertices[vertexIndices[i + 1]];
			glm::vec3 v2 = vertices[vertexIndices[i + 2]];

			glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
			surfaceNormals.push_back(normal);
		}
	}

	return true;
}

bool Make_Shader_Program() {
	//���̴� �ڵ� ���� �ҷ�����
	const GLchar* vertexShaderSource = File_To_Buf("vertex.glsl");
	const GLchar* fragmentShaderSource = File_To_Buf("fragment.glsl");

	//���̴� ��ü �����
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//���̴� ��ü�� ���̴� �ڵ� ���̱�
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	//���̴� ��ü �������ϱ�
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];

	//���̴� ���� ��������
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader ������ ����\n" << errorLog << endl;
		return false;
	}

	//���̴� ��ü �����
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//���̴� ��ü�� ���̴� �ڵ� ���̱�
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	//���̴� ��ü �������ϱ�
	glCompileShader(fragmentShader);
	//���̴� ���� ��������
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader ������ ����\n" << errorLog << endl;
		return false;
	}

	//���̴� ���α׷� ����
	shaderProgramID = glCreateProgram();
	//���̴� ���α׷��� ���̴� ��ü���� ���̱�
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	//���̴� ���α׷� ��ũ
	glLinkProgram(shaderProgramID);

	//���̴� ��ü �����ϱ�
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//���α׷� ���� ��������
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program ���� ����\n" << errorLog << endl;
		return false;
	}
	//���̴� ���α׷� Ȱ��ȭ
	glUseProgram(shaderProgramID);

	return true;
}

bool Set_VAO() {
	//�ﰢ���� �����ϴ� vertex ������ - position�� color

	is_reverse ? Load_Object("cube_reversefor22.obj") : Load_Object("cube.obj");

	if (is_reverse)
	{
		//���ؽ� �迭 ������Ʈ (VAO) �̸� ����
		glGenVertexArrays(1, &triangleVertexArrayObject_reversed);
		//VAO�� ���ε��Ѵ�.
		glBindVertexArray(triangleVertexArrayObject_reversed);
	}
	else
	{
		//���ؽ� �迭 ������Ʈ (VAO) �̸� ����
		glGenVertexArrays(1, &triangleVertexArrayObject);
		//VAO�� ���ε��Ѵ�.
		glBindVertexArray(triangleVertexArrayObject);
	}

	//Vertex Buffer Object(VBO)�� �����Ͽ� vertex �����͸� �����Ѵ�.

	//���ؽ� ���� ������Ʈ (VBO) �̸� ����
	glGenBuffers(1, &trianglePositionVertexBufferObjectID);
	//���� ������Ʈ�� ���ε� �Ѵ�.
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	//���� ������Ʈ�� �����͸� ����
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	//������Ʈ ���� ������Ʈ (EBO) �̸� ����
	glGenBuffers(1, &trianglePositionElementBufferObject);
	//���� ������Ʈ�� ���ε� �Ѵ�.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglePositionElementBufferObject);
	//���� ������Ʈ�� �����͸� ����
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	//��ġ �������� �Լ�
	GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	if (positionAttribute == -1) {
		cerr << "position �Ӽ� ���� ����" << endl;
		return false;
	}
	//���ؽ� �Ӽ� �������� �迭�� ����
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//���ؽ� �Ӽ� �迭�� ����ϵ��� �Ѵ�.
	glEnableVertexAttribArray(positionAttribute);

	glBindVertexArray(0);

	return true;
}

void updateCubePosition(float deltaTime) {
    if (!isOnIncline) {
        // ���鿡 ��� ������ �߷����� ���� ����
        cubeVelocity += gravity * deltaTime;
    } else {
        // ť�갡 ���� ǥ�鿡 ��� ���� �� �߷��� ���� ���и� ����
        glm::vec3 gravityAlongIncline = gravity - inclineNormal * glm::dot(gravity, inclineNormal);

        // ������ ���� �̲������� �ӵ� ������Ʈ
        cubeVelocity += gravityAlongIncline * deltaTime;

        // �̵� �Ÿ���ŭ ȸ�� ������ ������Ʈ
        float distance = glm::length(cubeVelocity * deltaTime);
        cubeRotationAngle += distance / 0.5f;  // ť�� �������� ���� ȸ�� ����
        cubeRotationAxis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), inclineNormal); // ǥ�鿡 ������ ȸ�� ��
    }

    // ��ġ ������Ʈ
    cubePosition += cubeVelocity * deltaTime;

    // **1. ���� ť�� ��ġ���� ���� ����� ǥ���� ã�� �ش� ������ �����մϴ�.**
    // ���⼭�� cubePosition�� �� ǥ���� ��ġ�� ���Ͽ� ���� ����� ǥ���� ������ ã�� ������ ����մϴ�.
    // �� �κп����� ���� ��� ǥ�� �߽ɰ��� �Ÿ��� �̿��� ���� �� �ֽ��ϴ�.

    //float minDistance = FLT_MAX;
    //glm::vec3 closestNormal = inclineNormal; // �⺻ ���� ����
    //bool foundSurface = false;

    //for (size_t i = 0; i < surfaceNormals.size(); i++) {
    //    // ǥ���� �߽��� ���Ƿ� ���ϰų�, ���� �� �ﰢ�� �߽��� ����� �� �ֽ��ϴ�.
    //    glm::vec3 surfaceCenter = (vertices[vertexIndices[i * 3]] +
    //                               vertices[vertexIndices[i * 3 + 1]] +
    //                               vertices[vertexIndices[i * 3 + 2]]) / 3.0f;

    //    // ť�� ��ġ�� ǥ�� �߽� ������ �Ÿ� ���
    //    float distance = glm::length(cubePosition - surfaceCenter);
    //    if (distance < minDistance) {
    //        minDistance = distance;
    //        closestNormal = surfaceNormals[i];
    //        foundSurface = true;
    //    }
    //}

    //// ���� ����� ǥ���� ã�� ���, �ش� ������ ���� �������� ����
    //if (foundSurface) {
    //    inclineNormal = closestNormal;
    //    isOnIncline = true;
    //}
}

// ť�긦 �׸��� �Լ�
void drawCube() {
	glBindVertexArray(triangleVertexArrayObject);

	glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePosition);
	model = glm::rotate(model, glm::radians(cubeRotationAngle), cubeRotationAxis);

	GLuint modelLoc = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)(0));

	glBindVertexArray(0);
}

void Viewport1() {
	glUseProgram(shaderProgramID);

	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
	view = glm::rotate(view, glm::radians(camera_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	unsigned viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f);
	projection = glm::translate(projection, glm::vec3(0.0, 0.0, -10.0));

	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projectionTransform");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

	glBindVertexArray(triangleVertexArrayObject_reversed);

	glm::mat4 TR = glm::mat4(1.0f);
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	unsigned int colorLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	for (int face_index = 0; face_index < 7; face_index++)
	{
		TR = glm::mat4(1.0f);
		set_cube(face_index, &TR);
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		if (face_index == 6)
			glUniform3f(colorLocation, colors[face_index - 1].x, colors[face_index - 1].y, colors[face_index - 1].z);
		else
			glUniform3f(colorLocation, colors[face_index].x, colors[face_index].y, colors[face_index].z);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * 6 * face_index));
	}

	drawCube();
}

// ���� �׸��� �Լ�
void drawScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	Viewport1(); // ī�޶� ���� �� ��Ÿ ��ȯ

	drawCube();
	glutSwapBuffers();
}

void set_cube(int body_index, glm::mat4* TR) {
	switch (body_index)
	{
	case 0://�޸�
		*TR = glm::scale(*TR, Box_border);
		break;
	case 1://����
		*TR = glm::scale(*TR, Box_border);
		break;
	case 2://����
		*TR = glm::scale(*TR, Box_border);
		break;
	case 3://����
		*TR = glm::scale(*TR, Box_border);
		break;
	case 4://�Ʒ���
		*TR = glm::scale(*TR, Box_border);
		break;
	case 5://�ո����
		*TR = glm::scale(*TR, Box_border);
		break;
	case 6://�ո������
		*TR = glm::scale(*TR, Box_border);
		break;
		break;
	default:
		break;
	}
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, WIN_W, WIN_H);
}

GLvoid TimerFunction1(int value) {
	updateCubePosition(0.016f); // 60FPS ����
	glutPostRedisplay();
	glutTimerFunc(16, TimerFunction1, 1);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	vector<int> new_opnenface = {};
	switch (key) {
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

	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	//������ ����
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(WIN_X, WIN_Y);
	glutInitWindowSize(WIN_W, WIN_H);
	glutCreateWindow("Example1");

	//GLEW �ʱ�ȭ�ϱ�
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Unable to initialize GLEW" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";

	if (!Make_Shader_Program()) {
		cerr << "Error: Shader Program ���� ����" << endl;
		std::exit(EXIT_FAILURE);
	}

	if (!Set_VAO()) {
		cerr << "Error: VAO ���� ����" << endl;
		std::exit(EXIT_FAILURE);
	}
	is_reverse = false;
	if (!Set_VAO()) {
		cerr << "Error: VAO ���� ����" << endl;
		std::exit(EXIT_FAILURE);
	}
	glutTimerFunc(10, TimerFunction1, 1);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(Mouse);
	glutMainLoop();
}