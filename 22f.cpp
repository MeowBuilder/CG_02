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
uniform_real_distribution<float> rand_speed(-2, 2);

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

glm::mat4 boxRotationMatrix = glm::mat4(1.0f);
glm::vec3 box_rotation(0,0,0);

//장애물 관련 벡터
std::vector<glm::vec3> Block_location = { {},{},{} };
std::vector<glm::vec3> Block_velocity = { {},{},{} };
std::vector<glm::vec3> Block_scale = { {3,3,3},{2,2,2},{1,1,1} };
glm::vec3 Block_Color = { 0.1,0.3,1.0 };
glm::vec3 gravity = { 0,-0.09,0 };

//공 관련 변수들
std::vector<glm::vec3> Ball_location = {};
std::vector<glm::vec3> Ball_velocity = {};
glm::vec3 Ball_Color = { 0.1,1.0,0.3 };

bool clicked = false;
bool open = false;

class Ball
{
public:
	Ball() {
		ball = gluNewQuadric();
		gluQuadricNormals(ball, GLU_SMOOTH);
		gluQuadricOrientation(ball, GLU_OUTSIDE);

		b_velocity = glm::vec3(rand_speed(eng), rand_speed(eng), rand_speed(eng));
	}

	void Draw() {
		glm::mat4 TR = glm::mat4(1.0f);

		TR = glm::translate(TR, b_location);
		unsigned modelLocation = glGetUniformLocation(shaderProgramID, "transform");
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));


		modelLocation = glGetUniformLocation(shaderProgramID, "colorAttribute");
		glUniform3f(modelLocation, 0,0,0);
		gluSphere(ball, 0.5, 20, 20);
	}
	
	void Update() {
		if (open)
		{
			b_velocity += gravity * 0.16f;
		}
		b_location += b_velocity * 0.16f;

		if (b_location.x + 0.5 > Box_border.x / 2) {
			b_velocity.x = -b_velocity.x;
		}
		else if (b_location.x - 0.5 < -Box_border.x / 2) {
			b_velocity.x = -b_velocity.x;
		}

		if (b_location.y + 0.5 > Box_border.y / 2) {
			b_velocity.y = -b_velocity.y;
		}
		else if ((b_location.y - 0.5 < -Box_border.y / 2) && !open) {
			b_velocity.y = -b_velocity.y;
		}

		if (b_location.z + 0.5 > Box_border.z / 2) {
			b_velocity.z = -b_velocity.z;
		}
		else if (b_location.z - 0.5 < -Box_border.z / 2) {
			b_velocity.z = -b_velocity.z;
		}

	}
	
private:
	glm::vec3 b_location = { 0,0,0 };
	glm::vec3 b_velocity = { 0,0,0 };
	GLUquadricObj* ball;
};

std::vector<Ball> balls;

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

	is_reverse ? Load_Object("cube_reversefor22.obj") : Load_Object("cube.obj");

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

	// 뒤집힌 큐브의 정상화
	glBindVertexArray(triangleVertexArrayObject);


	glUniform3f(colorLocation, Block_Color.x, Block_Color.y, Block_Color.z);
	for (int i = 0; i < 3; i++)
	{
		TR = glm::mat4(1.0f);
		set_block(i, &TR);
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);
	}

	for (int i = 0; i < balls.size(); i++)
	{
		balls[i].Draw();
	}
}

GLvoid drawScene()
{
	glClearColor(background_rgb.x, background_rgb.y, background_rgb.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	Viewport1();

	glutSwapBuffers();
}

void set_cube(int body_index, glm::mat4* TR) {
	*TR = glm::rotate(*TR, (box_rotation.z), glm::vec3(0, 0, 1));
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
		if (open)
		{
			*TR = glm::scale(*TR, glm::vec3(0, 0, 0));
		}
		else
		{
			*TR = glm::scale(*TR, Box_border);
		}
		break;
	case 5://앞면왼쪽
		*TR = glm::scale(*TR, Box_border);
		break;
	case 6://앞면오른쪽
		*TR = glm::scale(*TR, Box_border);
		break;
		break;
	default:
		break;
	}
}

void set_block(int block_index, glm::mat4* TR) {
	*TR = glm::translate(*TR, Block_location[block_index]);
	*TR = glm::rotate(*TR, (box_rotation.z), glm::vec3(0, 0, 1));
	*TR = glm::scale(*TR, Block_scale[block_index]);
}

void make_block() {
	float x_location = rand_location(eng);
	for (int i = 0; i < 3; i++)
	{
		Block_location[i].x = x_location;
		Block_location[i].z = -3 + (i * 1.5);
		Block_location[i].y = (Block_scale[i].y / 2) - (Box_border.y / 2) + 5.0f;
	}
}

GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, WIN_W, WIN_H);
}

GLvoid TimerFunction1(int value)
{
	boxRotationMatrix = glm::mat4(1.0f);
	boxRotationMatrix = glm::rotate(boxRotationMatrix, (box_rotation.z), glm::vec3(0, 0, 1));

	glutPostRedisplay();

	if (camera_rotate_y) {
		camera_rotate.y += camera_y_seta;
	}

	for (int i = 0; i < Block_location.size(); i++) {
		Block_velocity[i] += gravity * 0.16f;
		Block_location[i] += Block_velocity[i] * 0.16f;

		glm::vec4 localPos = glm::inverse(boxRotationMatrix) * glm::vec4(Block_location[i], 1.0f);

		if (localPos.x + Block_scale[i].x / 2 > Box_border.x / 2) {
			localPos.x = Box_border.x / 2 - Block_scale[i].x / 2;
		}
		else if (localPos.x - Block_scale[i].x / 2 < -Box_border.x / 2) {
			localPos.x = -Box_border.x / 2 + Block_scale[i].x / 2;
		}

		if (localPos.y + Block_scale[i].y / 2 > Box_border.y / 2) {
			localPos.y = Box_border.y / 2 - Block_scale[i].y / 2;
		}
		else if ((localPos.y - Block_scale[i].y / 2 < -Box_border.y / 2) && !open) {
			localPos.y = -Box_border.y / 2 + Block_scale[i].y / 2;
		}

		if (localPos.z + Block_scale[i].z / 2 > Box_border.z / 2) {
			localPos.z = Box_border.z / 2 - Block_scale[i].z / 2;
		}
		else if (localPos.z - Block_scale[i].z / 2 < -Box_border.z / 2) {
			localPos.z = -Box_border.z / 2 + Block_scale[i].z / 2;
		}

		Block_location[i] = glm::vec3(boxRotationMatrix * localPos);
	}

	for (size_t i = 0; i < balls.size(); i++)
	{
		balls[i].Update();
	}

	glutTimerFunc(10, TimerFunction1, 1);
}

void make_new_ball() {
	Ball newball;
	balls.push_back(newball);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	vector<int> new_opnenface = {};
	switch (key) {
	case 'y':
		camera_rotate_y = !camera_rotate_y;
		camera_y_seta = 1.0f;
		break;
	case 'Y':
		camera_rotate_y = !camera_rotate_y;
		camera_y_seta = -1.0f;
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
	case 'b':
		make_new_ball();
		break;
	case 'o':
		open = true;
		break;
	case 'q':
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay();
}

void Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		clicked = true;

	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		clicked = false;
	}

	glutPostRedisplay();
}

void Motion(int x, int y)
{
	GLfloat half_w = WIN_W / 2.0f;
	if (clicked) {
		mx = (x - half_w) / half_w;
		my = (half_w - y) / half_w;
		box_rotation.z = glm::atan(my / mx);
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
	is_reverse = false;
	if (!Set_VAO()) {
		cerr << "Error: VAO 생성 실패" << endl;
		std::exit(EXIT_FAILURE);
	}

	glutTimerFunc(10, TimerFunction1, 1);

	make_block();

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	glutMainLoop();
}