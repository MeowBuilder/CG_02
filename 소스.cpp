// main.cpp
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

glm::vec3 cubePosition = glm::vec3(5.0f, 5.0f, 5.0f); // 육면체 시작 위치
glm::vec3 cubeVelocity = glm::vec3(0.0f);              // 초기 속도
const glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f); // 중력
const float planeAngle = glm::radians(30.0f);           // 경사면 각도 (30도)
const float deltaTime = 0.016f;                         // 프레임 간격 (60FPS)

void initOpenGL() {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        exit(1);
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
}

void drawCube() {
    glColor3f(1.0f, 0.0f, 0.0f); // 빨간색
    glPushMatrix();
    glTranslatef(cubePosition.x, cubePosition.y, cubePosition.z);
    glutSolidCube(115.0);
    glPopMatrix();
}

void drawPlane() {
    glColor3f(0.8f, 0.8f, 0.8f); // 회색
    glPushMatrix();
    glRotatef(glm::degrees(planeAngle), 1.0f, 0.0f, 0.0f); // 경사면 회전
    glBegin(GL_QUADS);
    glVertex3f(-5.0f, 0.0f, -5.0f);
    glVertex3f(5.0f, 0.0f, -5.0f);
    glVertex3f(5.0f, 0.0f, 5.0f);
    glVertex3f(-5.0f, 0.0f, 5.0f);
    glEnd();
    glPopMatrix();
}

void updateCubePhysics() {
    glm::vec3 gravityForce = gravity * sin(planeAngle);
    cubeVelocity += gravityForce * deltaTime;
    cubePosition += cubeVelocity * deltaTime;

    if (cubePosition.y <= 0.5f) {
        cubePosition.y = 0.5f;
        cubeVelocity.y = 0.0f;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 카메라 위치 조정
    gluLookAt(0.0, 7.0, 15.0,  // 카메라 위치 변경
        0.0, 0.0, 0.0,   // 카메라가 바라보는 지점
        0.0, 1.0, 0.0);  // 업 벡터

    drawPlane();
    drawCube();

    glutSwapBuffers();
}

void timer(int value) {
    updateCubePhysics();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Sliding Cube on a Slope");

    initOpenGL();

    glutDisplayFunc(display);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
