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
bool yRotate = false;

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

std::vector<glm::vec3> translate_face(6, glm::vec3(0.0f, 0.0f, 0.0f));
std::vector<glm::vec2> rotate_face(6, glm::vec2(0.0f, 0.0f));
std::vector<glm::vec3> scale_face(6, glm::vec3(1.0f, 1.0f, 1.0f));
std::vector<float> trans_dir = { 1,0.01f,0.01f };

std::vector<int> open_face = { 0,1,2,3,4,5 };
std::vector<int> tet_rot_dir(4, 1);
float world_y_rot = 30.0f;

int face = 0;
bool isCube = true;

std::vector<bool> animKey(6, false);
std::vector<bool> anim5_face(4, false);

bool isortho = false;

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

	isCube ? Load_Object("cube.obj") : Load_Object("tetrahedron.obj");
	isCube ? open_face = { 0,1,2,3,4,5 } : open_face = { 0,1,2,3,4 };

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

	float color_tetrahedron[] = {
	   1.0f, 0.0f, 0.0f,//3
	   0.0f, 1.0f, 0.0f,//2
	   0.0f, 0.0f, 1.0f,//1

	   1.0f, 0.5f, 0.5f,//4
	   1.0f, 0.5f, 0.0f,//3
	   1.0f, 0.0f, 0.5f,//1

	   0.5f, 1.0f, 0.5f,//1
	   0.5f, 1.0f, 0.0f,//5
	   0.0f, 1.0f, 0.5f,//2

	   0.5f, 0.5f, 1.0f,//2
	   0.5f, 0.0f, 1.0f,//5
	   0.0f, 0.5f, 1.0f,//3

	   0.5f, 0.5f, 1.0f,//3
	   0.5f, 0.0f, 1.0f,//5
	   0.0f, 0.5f, 1.0f,//4

	   0.5f, 0.5f, 1.0f,//4
	   0.5f, 0.0f, 1.0f,//5
	   0.0f, 0.5f, 1.0f,//1
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
	isCube ? glBufferData(GL_ARRAY_BUFFER, sizeof(color_cube), color_cube, GL_STATIC_DRAW) : glBufferData(GL_ARRAY_BUFFER, sizeof(color_tetrahedron), color_tetrahedron, GL_STATIC_DRAW);

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

glm::vec3 move_to_origin(int face_index) {
	if (isCube)//육면체
	{
		switch (face_index)
		{
		case 2:
			return glm::vec3(0.0f, -0.5f, 0.0f);
			break;
		case 5:
			return glm::vec3(0.0f, 0.5f, -0.5f);
			break;
		default:
			break;
		}
	}
	else//사각뿔
	{
		switch (face_index)
		{
		case 1:
			return glm::vec3(0.0f, 0.5f, 0.5f);
			break;
		case 2:
			return glm::vec3(-0.5f, 0.5f, 0.0f);
			break;
		case 3:
			return glm::vec3(0.0f, 0.5f, -0.5f);
			break;
		case 4:
			return glm::vec3(0.5f, 0.5f, 0.0f);
			break;
		default:
			break;
		}
	}
}

void rotate_origin_x(int face_index, glm::mat4* TR) {
	if (isCube)//육면체
	{
		switch (face_index)
		{
		case 2:
			*TR = glm::translate(*TR, -move_to_origin(face_index));
			*TR = glm::rotate(*TR, glm::radians(rotate_face[face_index].x), glm::vec3(1.0, 0.0, 0.0));
			*TR = glm::translate(*TR, move_to_origin(face_index));
			break;
		case 5:
			*TR = glm::translate(*TR, -move_to_origin(face_index));
			*TR = glm::rotate(*TR, glm::radians(rotate_face[face_index].x), glm::vec3(1.0, 0.0, 0.0));
			*TR = glm::translate(*TR, move_to_origin(face_index));
			break;
		default:
			break;
		}
	}
	else//사각뿔
	{
		switch (face_index)
		{
		case 1:
			*TR = glm::translate(*TR, -move_to_origin(face_index));
			*TR = glm::rotate(*TR, glm::radians(-rotate_face[face_index].x), glm::vec3(1.0, 0.0, 0.0));
			*TR = glm::translate(*TR, move_to_origin(face_index));
			break;
		case 2:
			*TR = glm::translate(*TR, -move_to_origin(face_index));
			*TR = glm::rotate(*TR, glm::radians(-rotate_face[face_index].x), glm::vec3(0.0, 0.0, 1.0));
			*TR = glm::translate(*TR, move_to_origin(face_index));
			break;
		case 3:
			*TR = glm::translate(*TR, -move_to_origin(face_index));
			*TR = glm::rotate(*TR, glm::radians(rotate_face[face_index].x), glm::vec3(1.0, 0.0, 0.0));
			*TR = glm::translate(*TR, move_to_origin(face_index));
			break;
		case 4:
			*TR = glm::translate(*TR, -move_to_origin(face_index));
			*TR = glm::rotate(*TR, glm::radians(rotate_face[face_index].x), glm::vec3(0.0, 0.0, 1.0));
			*TR = glm::translate(*TR, move_to_origin(face_index));
			break;
		default:
			break;
		}
	}
}

void calcndraw(int face_index) {
	glBindVertexArray(triangleVertexArrayObject);

	glm::mat4 TR = glm::mat4(1.0f);
	TR = glm::rotate(TR, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
	TR = glm::rotate(TR, glm::radians(world_y_rot), glm::vec3(0.0, 1.0, 0.0));
	TR = glm::translate(TR, translate_face[face_index]);
	TR = glm::scale(TR, scale_face[face_index]);
	rotate_origin_x(face_index, &TR);
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

	if (isCube)
	{
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * 6 * face_index));
	}
	else
	{
		if (face_index == 0)
		{
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		else
		{
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(((sizeof(unsigned int) * 6) + (sizeof(unsigned int) * 3 * (face_index - 1)))));
		}
	}
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


	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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
		projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, -2.0f, 2.0f);
	}
	else
	{
		projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f);
		projection = glm::translate(projection, glm::vec3(0.0, 0.0, -5.0));
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

	glm::mat4 TR = glm::mat4(1.0f);
	TR = glm::rotate(TR, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
	TR = glm::rotate(TR, glm::radians(30.0f), glm::vec3(0.0, 1.0, 0.0));
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "transform");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
	glDrawArrays(GL_LINES, 0, line.size() / 3);

	for (int i = 0; i < open_face.size(); i++)
	{
		calcndraw(open_face[i]);
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
	if (yRotate)
		for (int i = 0; i < 6; i++)
		{
			world_y_rot += 0.5f;
		}


	if (animKey[0])
	{
		//윗면 가운데 축 중심 회전(x축 기준?)
		rotate_face[2].x += 0.5;
	}
	if (animKey[1])
	{
		//앞면 열고닫기
		rotate_face[5].x += trans_dir[0];
		if (rotate_face[5].x >= 90 || rotate_face[5].x <= 0)
		{
			trans_dir[0] = -trans_dir[0];
		}
	}
	if (animKey[2])
	{
		//옆면 위로 열고닫기
		translate_face[1].y += trans_dir[1];
		translate_face[3].y += trans_dir[1];
		if (translate_face[1].y >= 0.5 || translate_face[1].y <= -0.5f)
		{
			trans_dir[1] = -trans_dir[1];
		}
	}
	if (animKey[3])
	{
		//뒷면 커졌다 작아졌다
		scale_face[0].y += trans_dir[2];
		scale_face[0].x += trans_dir[2];
		if (scale_face[0].y >= 0.25f || scale_face[0].y <= 0.0f || scale_face[0].x >= 0.25f || scale_face[0].x <= 0.0f)
		{
			trans_dir[2] = -trans_dir[2];
		}
	}

	//사각뿔
	if (animKey[4])
	{
		// 모든 면이 열리고 닫힌다.
		for (int i = 1; i < 5; i++)
		{
			rotate_face[i].x += trans_dir[0];
		}
		if (rotate_face[1].x >= 235 || rotate_face[1].x <= 0)
		{
			trans_dir[0] = -trans_dir[0];
		}
		
	}
	if (animKey[5])
	{
		//하나씩 차례대로 90열기/닫기
		for (int i = 0; i < anim5_face.size(); i++)
		{
			if (anim5_face[i])
			{
				rotate_face[i+1].x += tet_rot_dir[i];
				if (rotate_face[i+1].x >= 120)
				{
					tet_rot_dir[i] = -tet_rot_dir[i];
				}
				else if (rotate_face[i + 1].x <= 0)
				{
					anim5_face[i] = false;
					tet_rot_dir[i] = -tet_rot_dir[i];
				}
			}
		}
	}
	glutTimerFunc(10, TimerFunction1, 1);
}


// 0:아랫면 1:뒷 오 앞 왼 순
GLvoid Keyboard(unsigned char key, int x, int y)
{
	vector<int> new_opnenface = {};
	switch (key) {
	case 'h':
		isCulling = 1 - isCulling;
		break;
	case 'y':
		yRotate = !yRotate;
		break;
	case 'q':
		glutLeaveMainLoop();
		break;

	case 'p': //직각 / 원근투영
		isortho = !isortho;
		break;

	case 't':
		isCube = true;
		Set_VAO();
		animKey[0] = !animKey[0];
		break;
	case 'f':
		isCube = true;
		Set_VAO();
		animKey[1] = !animKey[1];
		break;
	case 's':
		isCube = true;
		Set_VAO();
		animKey[2] = !animKey[2];
		break;
	case 'b':
		isCube = true;
		Set_VAO();
		animKey[3] = !animKey[3];
		break;
	case 'o':
		isCube = false;
		Set_VAO();
		animKey[4] = !animKey[4];
		break;
	case 'r':
		isCube = false;
		Set_VAO();
		if (!animKey[5])
		{
			animKey[5] = true;
		}
		anim5_face[face++] = true;
		if (face == 4) face = 0;
		break;
	case 'C':
		for (int i = 0; i < 6; i++)
		{
			translate_face[i] = glm::vec3(0.0f, 0.0f, 0.0f);
			rotate_face[i] = glm::vec2(0.0f, 0.0f);
			scale_face[i] = glm::vec3(1.0f, 1.0f, 1.0f);
			animKey[i] = false;
		}

		trans_dir = { 1,0.01f,0.01f };

		open_face = { 0,1,2,3,4,5 };
		for (int i = 0; i < 4; i++)
		{
			tet_rot_dir[i] = 1;
			anim5_face[i] = false;
		}
		world_y_rot = 30.0f;

		face = 0;
		isCube = true;
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