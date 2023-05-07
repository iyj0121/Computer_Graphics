#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "toys.h"
#include <glm/gtc/type_ptr.hpp>
#include "j3a.hpp"
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace glm;
const float PI = 3.141592;

vec3 lightPos = vec3(3, 5, 5);
vec3 lightInt = vec3(1, 1, 1);
vec3 color = vec3(1, .3, 0);
vec3 ambInt = vec3(0.1, 0.1, 0.1);
float shin = 20;

void render(GLFWwindow* window);
void init();
float clearB = .0f;
float theta = .0f;
float phi = .0f;
float fovy = 60;
float cameraDistance = 1; //perspective쓸때 사용한다

float triangleSize = 0.3;

//mat3 rotation;

Program program;
Program shadowProg;

GLuint vertbuf = 0;//데이터를 드라이브에서 받아와서 저장
GLuint normbuf = 0;
GLuint tcoordbuf = 0;
GLuint vertexArray = 0;
GLuint tribuf = 0;
GLuint texID = 0;
GLuint bumpID = 0;

GLuint fbo = 0;
GLuint shadowTex = 0;
GLuint shadowDepth = 0;

vec3 cameraPosition = vec3(0, 0, 5);

double oldX;
double oldY;

void cursorCallback(GLFWwindow* window, double xpos, double ypos) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        theta -= (xpos - oldX) / width * PI; //차이만큼 경도를 움직여준다 움직이는 범위가 윈도우크기만큼
        phi -= (ypos - oldY) / height * PI;
        //printf("%0.2f  ", phi);
        //if (phi < (-PI / 2 + 0.01) / (PI)) phi = (-PI / 2 + 0.01) / (PI);
        //if (phi > (PI / 2 - 0.01) / (PI)) phi = (PI / 2 - 0.01) / (PI);
    }
    oldX = xpos;
    oldY = ypos;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    fovy = fovy * pow(1.01, -yoffset); //스크롤의 위 아래
    //if (fovy < 10) fovy = 10;
    //if (fovy > 170) fovy = 170;
    //cameraDistance = cameraDistance * pow(1.01, -yoffset);
}
//if를 설정. 범위를 벗어날 경우 변화를 주지 말아야 한다. 
int main(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); //glfw설정
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 800, "yeong_jae", nullptr, nullptr);

    glfwSetCursorPosCallback(window, cursorCallback); //우리함수를 필요할 때 불러 주는 것, 윈도우에 붙이는 것
    glfwSetScrollCallback(window, scrollCallback);

    glfwMakeContextCurrent(window); //컨텍스트를 통해 접근 가능
    glewInit(); //초기설정
    //glfwSwapInterval(1); //10이면  화면이 10번 refresh될 떄마다 1번 그려라, 
    init();
    while (!glfwWindowShouldClose(window)) {
        render(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

const int shadowMapSize = 1024;
void init() {

    glGenTextures(1, &shadowTex);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, shadowMapSize, shadowMapSize, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &shadowDepth);
    glBindTexture(GL_TEXTURE_2D, shadowDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadowTex, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowDepth, 0);
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) printf("FBO Error\n");
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

    program.loadShaders("shader.vert", "shader.frag"); //vec3 정의
    shadowProg.loadShaders("shadow.vert", "shadow.frag"); //vec3 정의
    loadJ3A("C:\\Users\\iyj01\\source\\MyProject\\OpenGLTest\\OpenGLTest\\dwarf.j3a");
    stbi_set_flip_vertically_on_load(true);
    int w = 0, h = 0, n = 0;
    
    void* buf = stbi_load(("C:\\Users\\iyj01\\source\\MyProject\\OpenGLTest\\OpenGLTest\\" + diffuseMap[0]).c_str(), &w, &h, &n, 4);

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(buf);

    buf = stbi_load(("C:\\Users\\iyj01\\source\\MyProject\\OpenGLTest\\OpenGLTest\\" + bumpMap[0]).c_str(), &w, &h, &n, 4);

    glGenTextures(1, &bumpID);
    glBindTexture(GL_TEXTURE_2D, bumpID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(buf);

    glGenBuffers(1, &vertbuf);//1개 만든다
    glBindBuffer(GL_ARRAY_BUFFER, vertbuf);
    glBufferData(GL_ARRAY_BUFFER, nVertices[0] * sizeof(glm::vec3), vertices[0], GL_STATIC_DRAW); //vertex의 개수와 좌표가 이미 있다

    glGenBuffers(1, &normbuf);//1개 만든다
    glBindBuffer(GL_ARRAY_BUFFER, normbuf);
    glBufferData(GL_ARRAY_BUFFER, nVertices[0] * sizeof(glm::vec3), normals[0], GL_STATIC_DRAW);

    glGenBuffers(1, &tcoordbuf);//1개 만든다, vertex shader가 받는다
    glBindBuffer(GL_ARRAY_BUFFER, tcoordbuf);
    glBufferData(GL_ARRAY_BUFFER, nVertices[0] * sizeof(glm::vec2), texCoords[0], GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray); //타겟을 주지 않아도 된다. 0번 자리에 들어가야한다고 표시를 해줘야한다.  
    glEnableVertexAttribArray( 0 ); //bind되어서 vertexArray안써줘도된다, vertaxarray의 0번 자리
    glBindBuffer(GL_ARRAY_BUFFER, vertbuf); //혹시 모르니까 다시 묶어줌
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);//0번 자리에 하나의 단위의 사이즈, 형태. 묶인 vertexArray에 vertbuf를 연결

    glEnableVertexAttribArray(1); //bind되어서 vertexArray안써줘도된다, vertaxarray의 0번 자리
    glBindBuffer(GL_ARRAY_BUFFER, normbuf); //혹시 모르니까 다시 묶어줌
    glVertexAttribPointer(1, 3, GL_FLOAT, 0, 0, 0);//0번 자리에 하나의 단위의 사이즈, 형태. 묶인 vertexArray에 normbuf를 연결
    //1번 자리에 vec3
    glEnableVertexAttribArray(2); //bind되어서 vertexArray안써줘도된다, vertaxarray의 0번 자리
    glBindBuffer(GL_ARRAY_BUFFER, tcoordbuf); //혹시 모르니까 다시 묶어줌
    glVertexAttribPointer(2, 2, GL_FLOAT, 0, 0, 0);//0번 자리에 하나의 단위의 사이즈, 형태. 묶인 vertexArray에 tcoordbuf를 연결
    //2번 자리에 vec2
    //삼각형의 정보를 담기위한 처리
    glGenBuffers(1, &tribuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tribuf); //arraybuffer가 아니다.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, nTriangles[0] * sizeof(glm::u32vec3), triangles[0], GL_STATIC_DRAW); //삼각형의 개수와 좌표가 이미 있다
}

void render(GLFWwindow* window) { //그리기는 render에서 한다

    glEnable(GL_DEPTH_TEST);

    GLuint loc;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    mat4 modelMat = mat4(1);

    mat4 shadowMVP = ortho(-2.f, 2.f, -2.f, 2.f, 0.01f, 10.f)
        * lookAt(lightPos, vec3(0), vec3(0, 1, 0))
        * modelMat;

    glUseProgram(shadowProg.programID);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    loc = glGetUniformLocation(shadowProg.programID, "shadowMVP"); //shader에 보내는 과정(uniform을)
    glUniformMatrix4fv(loc, 1, false, glm::value_ptr(shadowMVP));
   
    glBindVertexArray(vertexArray); //묶어준다
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tribuf);
    glDrawElements(GL_TRIANGLES, nTriangles[0] * 3, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

   
    vec3 cameraPos = vec3(rotate(theta, vec3(0, 1, 0)) * rotate(phi, vec3(1, 0, 0)) * vec4(cameraPosition, 1)); //cameraPosition* cameraDistance
    //세타는 y축 기준으로 돈다
    mat4 viewMat = lookAt(cameraPos, vec3(0, 0, 0), vec3(0, 1, 0));
    mat4 projMat = perspective(fovy * PI / 180, width / (float)height, 0.1f, 100.f); //aspect는 비율
    
    glViewport(0, 0, width, height); //전체를 다 쓴다
    glClearColor(0, 0, 0.2, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //지우려는 버퍼의 비트 전달
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    
    glUseProgram(program.programID);
    loc = glGetUniformLocation(program.programID, "modelMat"); //shader에 보내는 과정(uniform을)
    glUniformMatrix4fv(loc, 1, false, glm::value_ptr(modelMat));
    loc = glGetUniformLocation(program.programID, "viewMat");
    glUniformMatrix4fv(loc, 1, false, glm::value_ptr(viewMat));
    loc = glGetUniformLocation(program.programID, "projMat");
    glUniformMatrix4fv(loc, 1, false, glm::value_ptr(projMat));

    loc = glGetUniformLocation(program.programID, "lightPos");
    glUniform3fv(loc, 1, glm::value_ptr(lightPos));
    loc = glGetUniformLocation(program.programID, "lightInt");
    glUniform3fv(loc, 1, glm::value_ptr(lightInt));
    loc = glGetUniformLocation(program.programID, "ambInt");
    glUniform3fv(loc, 1, glm::value_ptr(ambInt));
    loc = glGetUniformLocation(program.programID, "color");
    glUniform3fv(loc, 1, glm::value_ptr(diffuseColor[0])); //j3a파일에 이미 색깔이 존재한다
    loc = glGetUniformLocation(program.programID, "specularColor");
    glUniform3fv(loc, 1, glm::value_ptr(specularColor[0])); //j3a파일에 이미 색깔이 존재한다
    loc = glGetUniformLocation(program.programID, "shininess");
    glUniform1f(loc, shininess[0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    loc = glGetUniformLocation(program.programID, "colorTexture"); 
    glUniform1i(loc, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bumpID);
    loc = glGetUniformLocation(program.programID, "bumpTexture");
    glUniform1i(loc, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    loc = glGetUniformLocation(program.programID, "shadowMap");
    glUniform1i(loc, 2);
    loc = glGetUniformLocation(program.programID, "shadowMVP");
    glUniformMatrix4fv(loc, 1, false, value_ptr(shadowMVP));

    loc = glGetUniformLocation(program.programID, "cameraPos");
    glUniform3fv(loc, 1, glm::value_ptr(cameraPos));

    glBindVertexArray(vertexArray); //묶어준다
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tribuf);
    glDrawElements(GL_TRIANGLES, nTriangles[0] * 3, GL_UNSIGNED_INT, 0);//사용해서 primitive를 그린다, 3은 삼각형을 이루는 정점의 갯수
    glfwSwapBuffers(window);
}
/*
    loc = glGetUniformLocation(program.programID, "transf");
    rotation = mat3(cos(theta * PI / 180), -sin(theta * PI / 180), 0, sin(theta * PI / 180), cos(theta * PI / 180), 0, 0, 0, 1);
    mat3 transf = rotation;
    glUniformMatrix3fv(loc, 1, false, value_ptr(transf));
*/