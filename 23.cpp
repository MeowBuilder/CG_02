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

void draw_bottom();

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

bool isopen = true;

glm::vec3 Door_trans = { 5.0,0.0,0.0 };

//로봇 관련 벡터
glm::vec3 robot_translate = { 0.0,0.0,0.0 };
glm::vec3 robot_speed = { 0.05,0,0.05 };
float speed_ = 0.05f;

glm::vec3 robot_rotation = { 0.0,90.0,0.0 };

float rotate_seta = 2.0f;
glm::vec3 arm_rotate = { 0,0,0 };
float arm_rotate_seta = 2.0f;
glm::vec3 leg_rotate = { 0,0,0 };
float leg_rotate_seta = 2.0f;

//장애물 관련 벡터
std::vector<glm::vec3> Block_location = { {},{},{} };
std::vector<glm::vec3> Block_scale = { {0.1,0.1,0.1},{0.1,0.1,0.1},{0.1,0.1,0.1} };
glm::vec3 Block_Color = { 0.2,0.5,0.8 };

bool is_on = false;
int on_index = -1;

void set_body(int body_index, glm::mat4* TR);
void set_cube(int face_index, glm::mat4* TR);
void set_block(int block_index, glm::mat4* TR);

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

	is_reverse ? Load_Object("cube_reverse.obj") : Load_Object("cube.obj");

	if (is_reverse)
	{
		//버텍스 배열 오브젝트 (VAO) 이름 생성
		glGenVertexArrays(1, &triangleVertexArrayObject_reversed);
		//VAO를 바인드한다.
		glBindVertexArray(triangleVertexArrayObject_reversed);
	}
	else
	{
		//버텍스 배열 오브젝트 (VAO) 이름 생성
		glGenVertexArrays(1, &triangleVertexArrayObject);
		//VAO를 바인드한다.
		glBindVertexArray(triangleVertexArrayObject);
	}

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

	glBindVertexArray(0);

	return true;
}

void Viewport1() {
	vector<float> line = {
		-10.0f, 0.0f, 0.0f, 1.0f,0.0f,0.0f,
		10.0f, 0.0f, 0.0f, 1.0f,0.0f,0.0f,

		0.0f, -10.0f, 0.0f, 0.0f,1.0f,0.0f,
		0.0f, 10.0f, 0.0f, 0.0f,1.0f,0.0f,

		0.0f, 0.0f, -10.0f, 0.0f,0.0f,1.0f,
		0.0f, 0.0f, 10.0f, 0.0f,0.0f,1.0f
	};

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

	//TR 초기화, transform위치 가져오기
	glm::mat4 TR = glm::mat4(1.0f);
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	unsigned int colorLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");


	//도향 그리는 파트
	//큐브
	glBindVertexArray(triangleVertexArrayObject_reversed);
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
	draw_bottom();
	// 뒤집힌 큐브의 정상화
	glBindVertexArray(triangleVertexArrayObject);


	glUniform3f(colorLocation, colors[6].x, colors[6].y, colors[6].z);
	// 몸통
	TR = glm::mat4(1.0f);
	set_body(0, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	// 머리
	TR = glm::mat4(1.0f);
	set_body(1, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	glUniform3f(colorLocation, colors[1].x, colors[1].y, colors[1].z);
	//코
	TR = glm::mat4(1.0f);
	set_body(6, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);


	glUniform3f(colorLocation, colors[7].x, colors[7].y, colors[7].z);
	//다리
	TR = glm::mat4(1.0f);
	set_body(2, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);


	glUniform3f(colorLocation, colors[7].x + 1, colors[7].y / 2, colors[7].z / 2);
	TR = glm::mat4(1.0f);
	set_body(3, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);


	glUniform3f(colorLocation, colors[8].x, colors[8].y, colors[8].z);
	//팔
	TR = glm::mat4(1.0f);
	set_body(4, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);


	glUniform3f(colorLocation, colors[8].x + 1, colors[8].y / 2, colors[8].z / 2);
	TR = glm::mat4(1.0f);
	set_body(5, &TR);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

	for (int i = 0; i < Block_location.size(); i++)
	{
		//glUniform3f(colorLocation, Block_Color.x, Block_Color.y, Block_Color.z);
		glUniform3f(colorLocation, colors[i].x, colors[i].y, colors[i].z);
		TR = glm::mat4(1.0f);
		set_block(i, &TR);
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);
	}
}

GLvoid drawScene()
{
	glClearColor(background_rgb.x, background_rgb.y, background_rgb.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//isCulling ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	Viewport1();

	glutSwapBuffers();
}
std::vector<glm::vec3> bottom_location = {
};

void add_u() {
	Block_location.push_back(glm::vec3(-1.5, -2.5, 0));
	Block_scale.push_back(glm::vec3(1, 5, 1));

	Block_location.push_back(glm::vec3(1.5, -2.5, 0));
	Block_scale.push_back(glm::vec3(1, 5, 1));

	Block_location.push_back(glm::vec3(0, 0, 0));
	Block_scale.push_back(glm::vec3(3, 1, 1));
}

void set_bottom() {
	for (int i = -4; i <= 5; i++)
	{
		for (int j = -4; j <= 5; j++)
		{
			glm::vec3 newvec = { i-0.5,-5,j -0.5};
			bottom_location.push_back(newvec);
		}
	}
}

void draw_bottom() {
	std:vector<glm::vec3> bottom_color = {
		{1,0,1},
		{0,1,1},
		{1,1,0},
	};

	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	unsigned int colorLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
	for (int i = 0; i < bottom_location.size(); i++)
	{
		glm::mat4 TR = glm::mat4(1.0f);
		TR = glm::translate(TR, bottom_location[i]);
		TR = glm::scale(TR, glm::vec3(1, 0, 1));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glUniform3f(colorLocation, bottom_color[i % 3].x, bottom_color[i % 3].y, bottom_color[i % 3].z);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * 6 * 4));
	}
}

void set_cube(int body_index, glm::mat4* TR) {
	switch (body_index)
	{
	case 0://뒷면
		*TR = glm::scale(*TR, Box_border);
		break;
	case 1://옆면
		*TR = glm::scale(*TR, Box_border);
		break;
	case 2://윗면
		*TR = glm::scale(*TR, Box_border);
		break;
	case 3://옆면
		*TR = glm::scale(*TR, Box_border);
		break;
	case 4://아랫면
		*TR = glm::scale(*TR, glm::vec3(0, 0, 0));
		break;
	case 5://앞면왼쪽
		*TR = glm::translate(*TR, -Door_trans);
		*TR = glm::scale(*TR, Box_border);
		break;
	case 6://앞면오른쪽
		*TR = glm::translate(*TR, Door_trans);
		*TR = glm::scale(*TR, Box_border);
		break;
		break;
	default:
		break;
	}
}

void set_body(int body_index, glm::mat4* TR) {
	//로봇 자체의 이동
	*TR = glm::translate(*TR, robot_translate);
	*TR = glm::rotate(*TR, glm::radians(robot_rotation.x), glm::vec3(1.0, 0.0, 0.0));
	*TR = glm::rotate(*TR, glm::radians(robot_rotation.z), glm::vec3(0.0, 0.0, 1.0));
	*TR = glm::rotate(*TR, robot_rotation.y, glm::vec3(0.0, 1.0, 0.0));

	switch (body_index)
	{
	case 0://몸통
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.0f, 0.0f));

		*TR = glm::scale(*TR, glm::vec3(1, 1.5, 1));
		break;
	case 1://머리
		*TR = glm::translate(*TR, glm::vec3(0.0f, 1.0f, 0.0f));

		*TR = glm::scale(*TR, glm::vec3(0.5, 0.5, 0.5));
		break;

	case 2://왼다리
		*TR = glm::translate(*TR, glm::vec3(-0.25f, -0.75f, 0.0f));

		*TR = glm::rotate(*TR, glm::radians(leg_rotate.x), glm::vec3(1.0, 0.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(0.0, -0.5f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.25, 1.0, 0.25));
		break;
	case 3://오른다리
		*TR = glm::translate(*TR, glm::vec3(0.25f, -0.75f, 0.0f));

		*TR = glm::rotate(*TR, glm::radians(-leg_rotate.x), glm::vec3(1.0, 0.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(0.0, -0.5f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.25, 1.0, 0.25));
		break;

	case 4://왼팔
		*TR = glm::translate(*TR, glm::vec3(-0.625f, 0.5f, 0.0f));

		*TR = glm::rotate(*TR, glm::radians(-arm_rotate.x), glm::vec3(1.0, 0.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(0.0, -0.5f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.25, 1.0, 0.25));
		break;
	case 5://오팔
		*TR = glm::translate(*TR, glm::vec3(0.625f, 0.5f, 0.0f));
		//이곳에 회전관련
		*TR = glm::rotate(*TR, glm::radians(arm_rotate.x), glm::vec3(1.0, 0.0, 0.0));
		*TR = glm::translate(*TR, glm::vec3(0.0, -0.5f, 0.0f));
		*TR = glm::scale(*TR, glm::vec3(0.25, 1.0, 0.25));
		break;
	case 6://코
		*TR = glm::translate(*TR, glm::vec3(0.0f, 0.95f, 0.35f));

		*TR = glm::scale(*TR, glm::vec3(0.15, 0.15, 0.15));
		break;
	}


}

void set_block(int block_index, glm::mat4* TR) {
	*TR = glm::translate(*TR, Block_location[block_index]);
	*TR = glm::scale(*TR, Block_scale[block_index]);
}

void make_block() {
	for (int i = 0; i < 3; i++)
	{
		float scale = rand_size(eng);
		Block_scale[i] = { scale,scale,scale };

		Block_location[i].x = rand_location(eng);
		Block_location[i].z = rand_location(eng);
		Block_location[i].y = (Block_scale[i].y / 2) - (Box_border.y / 2);
	}
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, WIN_W, WIN_H);
}

bool is_onBlock(glm::vec3 robot_position, int block_index) {
	float distance = glm::distance(robot_position, glm::vec3(Block_location[block_index].x, 0, Block_location[block_index].z));
	bool is_on_y = false;
	if (robot_position.y > Block_location[block_index].y + Block_scale[block_index].y / 2)
		is_on_y = true;
	return distance <= 1.0f && is_on_y;
}

GLvoid TimerFunction1(int value)
{
	glutPostRedisplay();

	if (camera_rotate_y)
	{
		camera_rotate.y += camera_y_seta;
	}

	if (isopen)
	{
		if (Door_trans.x < 4.9f)
		{
			Door_trans.x += 0.1f;
		}
	}
	else
	{
		if (Door_trans.x > 0.1f)
		{
			Door_trans.x -= 0.1f;
		}
	}

	for (int i = 0; i < Block_location.size(); i++)
	{
		if (is_onBlock(glm::vec3(robot_translate.x, 0, robot_translate.z), i) &&
			robot_translate.y - 1.5 < (Block_location[i].y + (Block_scale[i].y / 2)))
		{
			robot_speed.y = 0.8f;
			is_on = true;
			on_index = i;
		}
	}

	if (is_on)
	{
		if (robot_translate.y >= (Block_location[on_index].y + (Block_scale[on_index].y / 2)) + 1.5)
		{
			robot_translate.y += robot_speed.y;
			if (robot_speed.y > -9.8) robot_speed.y -= 0.098f;

			if (robot_translate.y <= ((Block_location[on_index].y + (Block_scale[on_index].y / 2)) + 1.5))
			{
				robot_translate.y = (Block_location[on_index].y + (Block_scale[on_index].y / 2)) + 1.6;
			}
		}
		else
		{
			robot_translate.y += robot_speed.y;
		}


		Block_location[on_index].y -= 0.01f;
		if (Block_location[on_index].y + Block_scale[on_index].y / 2 < (-Box_border.y / 2))
		{
			Block_location.erase(Block_location.begin() + on_index);
			Block_scale.erase(Block_scale.begin() + on_index);
			is_on = false;
			on_index = -1;
		}
	}
	else
	{
		if (robot_translate.y >= (-Box_border.y / 2) + 1.57f)
		{
			robot_translate.y += robot_speed.y;
			if (robot_speed.y > -9.8) robot_speed.y -= 0.098f;

			if (robot_translate.y <= (-Box_border.y / 2) + 1.57f)
			{
				robot_translate.y = (-Box_border.y / 2) + 1.58f;
			}

			if (robot_translate.y >= (Box_border.y / 2) - 1.5f)
			{
				robot_translate.y = (Box_border.y / 2) - 1.5f;
			}
		}
		else
		{
			robot_translate.y = (-Box_border.y / 2) + 1.58f;
		}
	}

	if (is_on)
	{
		if (!is_onBlock(glm::vec3(robot_translate.x, 0, robot_translate.z), on_index))
		{
			is_on = false;
			on_index = -1;
		}
	}


	for (int i = 0; i < Block_location.size(); i++)
	{
		//블럭 충돌
	}


	robot_translate.x += robot_speed.x;
	if (robot_translate.x >= 5.0f) {
		robot_speed.x = -speed_;
		robot_translate.x = 4.9f;
	}
	else if (robot_translate.x <= -5.0f) {
		robot_speed.x = speed_;
		robot_translate.x = -4.9f;
	}

	robot_translate.z += robot_speed.z;
	if (robot_translate.z >= 5.0f) {
		robot_speed.z = -speed_;
		robot_translate.z = 4.9f;
	}
	else if (robot_translate.z <= -5.0f) {
		robot_speed.z = speed_;
		robot_translate.z = -4.9f;
	}




	robot_rotation.y = atan2(robot_speed.x, robot_speed.z);

	arm_rotate.x += arm_rotate_seta;
	if (arm_rotate.x < -60.0f)
		arm_rotate_seta = rotate_seta;
	else if (arm_rotate.x > 60.0f)
		arm_rotate_seta = -rotate_seta;

	leg_rotate.x += leg_rotate_seta;
	if (leg_rotate.x < -60.0f)
		leg_rotate_seta = rotate_seta;
	else if (leg_rotate.x > 60.0f)
		leg_rotate_seta = -rotate_seta;

	glutTimerFunc(10, TimerFunction1, 1);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	vector<int> new_opnenface = {};
	switch (key) {
	case 'o':
		isopen = !isopen;
		break;
	case 'y':
		camera_rotate_y = !camera_rotate_y;
		camera_y_seta = 0.1f;
		break;
	case 'Y':
		camera_rotate_y = !camera_rotate_y;
		camera_y_seta = -0.1f;
		break;
	case 'z':
		cameraPos.z -= 0.1f;
		break;
	case 'Z':
		cameraPos.z += 0.1f;
		break;
	case 'x':
		cameraPos.x += 0.1f;
		break;
	case 'X':
		cameraPos.x -= 0.1f;
		break;

	case '+':
		speed_ += 0.01f;

		arm_rotate_seta = abs(arm_rotate_seta) + 1.0f;
		leg_rotate_seta = abs(leg_rotate_seta) + 1.0f;
		rotate_seta = abs(arm_rotate_seta);
		break;
	case '-':
		speed_ -= 0.01f;

		arm_rotate_seta = abs(abs(arm_rotate_seta) - 1.0f);
		leg_rotate_seta = abs(abs(leg_rotate_seta) - 1.0f);
		rotate_seta = abs(arm_rotate_seta);
		break;
	case 'j':
		robot_speed.y = 0.8f;
		break;

	case 'w':
		robot_speed.z = speed_;
		robot_speed.x = 0;
		break;
	case 'a':
		robot_speed.x = -speed_;
		robot_speed.z = 0;
		break;
	case 's':
		robot_speed.z = -speed_;
		robot_speed.x = 0;
		break;
	case 'd':
		robot_speed.x = speed_;
		robot_speed.z = 0;
		break;
	case 'i':
		cameraPos = { 0.0f,0.0f,15.0f };
		camera_rotate = { 0.0f,0.0f,0.0f };
		camera_rotate_y = false;

		isopen = false;

		Door_trans = { 0.0,0.0,0.0 };

		robot_translate = { 0.0,0.0,0.0 };
		robot_speed = { 0.05,0,0.05 };
		speed_ = 0.05f;

		robot_rotation = { 0.0,90.0,0.0 };


		rotate_seta = 2.0f;
		arm_rotate = { 0,0,0 };
		arm_rotate_seta = 2.0f;
		leg_rotate = { 0,0,0 };
		leg_rotate_seta = 2.0f;
		break;
	case 'q':
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay();
}

void Mouse(int x, int y)
{
	GLfloat half_w = WIN_W / 2.0f;
	mx = (x - half_w) / half_w;
	my = (half_w - y) / half_w;

	robot_speed.z = (-my) / 10;
	robot_speed.x = (mx) / 10;
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
	is_reverse = false;
	if (!Set_VAO()) {
		cerr << "Error: VAO 생성 실패" << endl;
		std::exit(EXIT_FAILURE);
	}
	glutTimerFunc(10, TimerFunction1, 1);

	make_block();
	set_bottom();
	add_u();

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	//glutMouseFunc(Mouse);
	glutMotionFunc(Mouse);
	glutMainLoop();
}