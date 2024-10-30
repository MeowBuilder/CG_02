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
uniform_int_distribution<int> rand_cube(0, 5);
uniform_int_distribution<int> rand_tet(0, 3);

const int WIN_X = 10, WIN_Y = 10;
const int WIN_W = 800, WIN_H = 800;

const glm::vec3 background_rgb = glm::vec3(0.0f, 0.0f, 0.0f);

bool isCulling = true;

GLfloat mx = 0.0f;
GLfloat my = 0.0f;

int framebufferWidth, framebufferHeight;
GLuint triangleVertexArrayObject;
GLuint shaderProgramID;
GLuint trianglePositionVertexBufferObjectID, triangleColorVertexBufferObjectID;
GLuint trianglePositionElementBufferObject;
GLuint Line_VAO, Line_VBO;

std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;

std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > uvs;
std::vector< glm::vec3 > normals;

glm::vec3 world_rotate = { 0.0,0.0,0.0 };
bool world_rotate_y = false;
float world_y_seta = 0.5f;

bool isortho = false;

glm::vec3 trans_down_body = { 0.0f,0.0f,0.0f };
glm::vec3 rotate_mid_body = { 0.0f,0.0f,0.0f };
bool rotate_mid = false;
float rotate_mid_seta = 1.0f;

glm::vec3 rotate_barrel_body = { 0.0f,0.0f,0.0f };
bool rotate_barrel = false;
float rotate_barrel_seta = 1.0f;

float barrel_trans_mid = 0.5f;
bool make_barrel_one = false;

glm::vec3 rotate_arms = { 0.0f,0.0f,0.0f };
bool rotate_arms_bool = false;
float rotate_arm_seta = 0.5f;

//카메라 zx
glm::vec3 cameraPos = { -1.0f,1.0f,1.0f };
glm::vec3 camera_rotate = { 0.0f,0.0f,0.0f };
bool camera_rotate_y = false;
float camera_y_seta = 0.5f;
bool camera_rotate_all = false;

glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

void set_body(int body_index, glm::mat4* TR);

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

	return true;
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

bool Set_VAO() {
	//삼각형을 구성하는 vertex 데이터 - position과 color

	Load_Object("cube.obj");

	float color_cube[] = {
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

	//버텍스 배열 오브젝트 (VAO) 이름 생성
	glGenVertexArrays(1, &triangleVertexArrayObject);
	//VAO를 바인드한다.
	glBindVertexArray(triangleVertexArrayObject);

	//Vertex Buffer Object(VBO)를 생성하여 vertex 데이터를 복사한다.

	//버텍스 버퍼 오브젝트 (VBO) 이름 생성
	glGenBuffers(1, &trianglePositionVertexBufferObjectID);
	//버퍼 오브젝트를 바인드 한다.
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	//버퍼 오브젝트의 데이터를 생성
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	//엘리멘트 버퍼 오브젝트 (EBO) 이름 생성
	glGenBuffers(1, &trianglePositionElementBufferObject);
	//버퍼 오브젝트를 바인드 한다.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglePositionElementBufferObject);
	//버퍼 오브젝트의 데이터를 생성
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	//위치 가져오기 함수
	GLint positionAttribute = glGetAttribLocation(shaderProgramID, "positionAttribute");
	if (positionAttribute == -1) {
		cerr << "position 속성 설정 실패" << endl;
		return false;
	}
	//버텍스 속성 데이터의 배열을 정의
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//버텍스 속성 배열을 사용하도록 한다.
	glEnableVertexAttribArray(positionAttribute);

	//칼라 버퍼 오브젝트 (VBO) 이름 생성
	glGenBuffers(1, &triangleColorVertexBufferObjectID);
	//버퍼 오브젝트를 바인드 한다.
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	//버퍼 오브젝트의 데이터를 생성
	glBufferData(GL_ARRAY_BUFFER, sizeof(color_cube), color_cube, GL_STATIC_DRAW);

	//위치 가져오기 함수
	GLint colorAttribute = glGetAttribLocation(shaderProgramID, "colorAttribute");
	if (colorAttribute == -1) {
		cerr << "color 속성 설정 실패" << endl;
		return false;
	}
	//버퍼 오브젝트를 바인드 한다.
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	//버텍스 속성 데이터의 배열을 정의
	glVertexAttribPointer(colorAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//버텍스 속성 배열을 사용하도록 한다.
	glEnableVertexAttribArray(colorAttribute);


	glBindVertexArray(0);

	return true;
}

void world_trans(glm::mat4* TR) {

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
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUseProgram(shaderProgramID);
	glm::vec3 rotatedTarget = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(world_rotate.y), glm::vec3(0.0f, 1.0f, 0.0))* glm::vec4(cameraTarget - cameraPos, 1.0f)) + cameraPos;

	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(cameraPos, rotatedTarget, cameraUp);
	view = glm::rotate(view, glm::radians(camera_rotate.x), glm::vec3(1.0, 0.0, 0.0));
	view = glm::rotate(view, glm::radians(camera_rotate.y), glm::vec3(0.0, 1.0, 0.0));
	view = glm::rotate(view, glm::radians(camera_rotate.z), glm::vec3(0.0, 0.0, 1.0));
	unsigned viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 projection = glm::mat4(1.0f);
	if (isortho)
	{
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -10.0f, 10.0f);
	}
	else
	{
		projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f);
		projection = glm::translate(projection, glm::vec3(0.0, 0.0, -10.0));
	}

	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projectionTransform");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

	glBindVertexArray(Line_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, Line_VBO);
	glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(float), line.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	//좌표축그리는 파트
	glm::mat4 TR = glm::mat4(1.0f);
	world_trans(&TR);
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawArrays(GL_LINES, 0, line.size() / 3);

	//도향 그리는 파트
	glBindVertexArray(triangleVertexArrayObject);

	//바닥
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(0, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//아래몸체
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(1, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//위몸체
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(2, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//왼쪽포신
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(3, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//왼쪽포신
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(4, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//왼쪽크레인
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(5, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	//왼쪽크레인
	TR = glm::mat4(1.0f);
	world_trans(&TR);
	set_body(6, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	glutSwapBuffers();
}

void set_body(int body_index,glm::mat4* TR) {
	*TR = glm::translate(*TR, trans_down_body);
	switch (body_index)
	{
	case 0://바닥
		*TR = glm::translate(*TR, glm::vec3(0.0f, -0.5f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(50.0f, 0.5f, 50.0f));
		break;
	case 1://아래몸체
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.0f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(1.5f, 0.5f, 1.5f));
		break;
	case 2://위몸체
		*TR = glm::rotate(*TR, glm::radians(rotate_mid_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.375f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.75f, 0.25f, 0.75f));
		break;
	case 3://왼쪽포신
		*TR = glm::rotate(*TR, glm::radians(-rotate_barrel_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(-barrel_trans_mid, 0.0f, 1.0f));
		*TR = glm::scale(*TR, glm::vec3(0.25, 0.1, 0.75f));
		break;
	case 4://오른쪽포신
		*TR = glm::rotate(*TR, glm::radians(rotate_barrel_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(barrel_trans_mid, 0.0f, 1.0f));
		*TR = glm::scale(*TR, glm::vec3(0.25, 0.1, 0.75f));
		break;
	case 5://왼쪽크레인
		*TR = glm::rotate(*TR, glm::radians(rotate_mid_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(0.2f, 0.25f, 0.0f));
		*TR = glm::rotate(*TR, glm::radians(rotate_arms.z), glm::vec3(0.0, 0.0, 1.0));
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.25f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.15, 0.5f, 0.15f));
		break;
	case 6://오른쪽크레인
		*TR = glm::rotate(*TR, glm::radians(rotate_mid_body.y), glm::vec3(0.0, 1.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(-0.2f, 0.25f, 0.0f));
		*TR = glm::rotate(*TR, glm::radians(-rotate_arms.z), glm::vec3(0.0, 0.0, 1.0));
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.25f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.15, 0.5f, 0.15f));
		break;
	default:
		break;
	}
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

GLvoid TimerFunction1(int value)
{
	glutPostRedisplay();
	if (camera_rotate_y)
	{
		camera_rotate.y += camera_y_seta;
	}
	if (world_rotate_y)
	{
		world_rotate.y += world_y_seta;
	}
	if (camera_rotate_all)
	{
		camera_rotate.x += camera_y_seta;
		camera_rotate.z += camera_y_seta;
	}

	if (rotate_mid)
	{
		rotate_mid_body.y += rotate_mid_seta;
	}
	if (rotate_barrel)
	{
		rotate_barrel_body.y += rotate_barrel_seta;
	}
	if (rotate_arms_bool)
	{
		if (rotate_arms.z < 90.0f && rotate_arms.z > -90.0f)
		{
			rotate_arms.z += rotate_arm_seta;
			if (rotate_arms.z >= 90.0f || rotate_arms.z <= -90.0f)
				rotate_arms.z -= rotate_arm_seta;
		}
	}

	if (make_barrel_one)
	{
		if (rotate_barrel_body.y < 0)
		{
			rotate_barrel_body.y += 0.5f;
		}
		else if (rotate_barrel_body.y > 0)
		{
			rotate_barrel_body.y -= 0.5f;
		}
		else if (rotate_barrel_body.y == 0)
		{
			if (barrel_trans_mid > 0)
			{
				barrel_trans_mid -= 0.01f;
			}
		}
	}
	else
	{
		if (barrel_trans_mid < 0.5f)
		{
			barrel_trans_mid += 0.01f;
		}
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
		world_rotate_y = !world_rotate_y;
		world_y_seta = 0.5f;
		break;
	case 'Y':
		world_rotate_y = !world_rotate_y;
		world_y_seta = -0.5f;
		break;
	case 'r':
		camera_rotate_y = !camera_rotate_y;
		camera_y_seta = 0.5f;
		break;
	case 'R':
		camera_rotate_y = !camera_rotate_y;
		camera_y_seta = -0.5f;
		break;
	case 'a':
		camera_rotate_all = !camera_rotate_all;
		break;
	case 'p':
		isortho = !isortho;
		break;
	case 'z':
		cameraPos.z += 0.1f;
		break;
	case 'Z':
		cameraPos.z -= 0.1f;
		break;
	case 'x':
		cameraPos.x += 0.1f;
		break;
	case 'X':
		cameraPos.x -= 0.1f;
		break;

	case 'b':
		if (trans_down_body.x < 10.0f && trans_down_body.x > -10.0f)
		{
			trans_down_body.x += 0.1f;
		}
		break;
	case 'B':
		if (trans_down_body.x < 10.0f && trans_down_body.x > -10.0f)
		{
			trans_down_body.x -= 0.1f;
		}
		break;
	case 'm':
		rotate_mid = !rotate_mid;
		rotate_mid_seta = 0.5f;
		break;
	case 'M':
		rotate_mid = !rotate_mid;
		rotate_mid_seta = -0.5f;
		break;
	case 'f':
		rotate_barrel = !rotate_barrel;
		rotate_barrel_seta = 0.5f;
		break;
	case 'F':
		rotate_barrel = !rotate_barrel;
		rotate_barrel_seta = -0.5f;
		break;

	case 'e':
		make_barrel_one = !make_barrel_one;
		break;
	case 't':
		rotate_arms_bool = !rotate_arms_bool;
		rotate_arm_seta = 0.5f;
		break;
	case 'T':
		rotate_arms_bool = !rotate_arms_bool;
		rotate_arm_seta = -0.5f;
		break;
	case 's':
		camera_rotate_y = false;
		world_rotate_y = false;
		camera_rotate_all = false;
		rotate_mid = false;
		rotate_barrel = false;
		rotate_arms_bool = false;
		make_barrel_one = false;
		break;
	case 'c':
		world_rotate = { 0.0,0.0,0.0 };
		world_rotate_y = false;
		isortho = false;

		trans_down_body = { 0.0f,0.0f,0.0f };
		rotate_mid_body = { 0.0f,0.0f,0.0f };
		rotate_mid = false;
		rotate_mid_seta = 1.0f;

		rotate_barrel_body = { 0.0f,0.0f,0.0f };
		rotate_barrel = false;
		rotate_barrel_seta = 1.0f;

		barrel_trans_mid = 0.5f;
		make_barrel_one = false;

		rotate_arms = { 0.0f,0.0f,0.0f };
		rotate_arms_bool = false;
		rotate_arm_seta = 0.5f;

		cameraPos = { -1.0f,1.0f,1.0f };
		camera_rotate = { 0.0f,0.0f,0.0f };
		camera_rotate_y = false;
		camera_y_seta = 0.5f;
		camera_rotate_all = false;

		cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case 'q':
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay();
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

	if (!Set_VAO()) {
		cerr << "Error: VAO 생성 실패" << endl;
		std::exit(EXIT_FAILURE);
	}
	glutTimerFunc(10, TimerFunction1, 1);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMainLoop();
}