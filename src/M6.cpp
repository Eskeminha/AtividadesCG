/* Módulo 6 - Trajetórias para Objetos 3D
 * Disciplina de Computação Gráfica - Unisinos
 *
 * Controles:
 *   W / A / S / D       - movimenta a câmera
 *   Mouse               - rotaciona a câmera (pitch / yaw)
 *   Scroll              - zoom (altera o FOV)
 *   TAB                 - seleciona o próximo objeto
 *   R                   - modo rotação -> X / Y / Z escolhe o eixo
 *   Setas               - translada o objeto selecionado em X / Y
 *   Page Up / Page Down - translada o objeto selecionado em Z
 *   P                   - adiciona posição atual como ponto de trajetória
 *   F                   - inicia/para a trajetória do objeto selecionado
 *   C                   - limpa os pontos de trajetória do objeto selecionado
 *   1 / 2 / 3           - liga/desliga key / fill / back light
 *   ESC                 - fecha a janela
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mod);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Protótipos das funções
int    setupShader();
int    setupTextShader();
void   renderText(const string& texto, float x, float y, float escala, glm::vec3 cor);
int    loadSimpleOBJ(const string& filePath, int& nVertices, GLuint& textureID,
                     float& ka, float& kd, float& ks, float& q);
GLuint loadTexture(const string& path);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = R"(
#version 330
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2  TexCoord;
out vec3  vNormal;
out vec3  fragPos;
void main()
{
    vec4 wp     = model * vec4(position, 1.0);
    gl_Position = projection * view * wp;
    fragPos     = vec3(wp);
    TexCoord    = texCoord;
    vNormal     = mat3(transpose(inverse(model))) * normal;
}
)";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = R"(
#version 330
in vec2  TexCoord;
in vec3  vNormal;
in vec3  fragPos;
uniform sampler2D textura;
uniform vec3  camPos;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float q;
uniform vec3  lightPos[3];
uniform float lightInt[3];
uniform int   lightOn[3];
uniform float selMult;
out vec4 color;
void main()
{
    vec3 tex = vec3(texture(textura, TexCoord));
    vec3 N   = normalize(vNormal);
    vec3 V   = normalize(camPos - fragPos);

    vec3 ambient  = ka * tex;
    vec3 diffuse  = vec3(0.0);
    vec3 specular = vec3(0.0);

    for (int i = 0; i < 3; i++)
    {
        if (lightOn[i] == 0) continue;
        vec3  L   = normalize(lightPos[i] - fragPos);
        float d   = length(lightPos[i] - fragPos);
        float att = 1.0 / (1.0 + 0.5 * d + 1.0 * d * d);
        float diff = max(dot(N, L), 0.0);
        diffuse += kd * diff * att * lightInt[i] * tex;
        vec3  R    = reflect(-L, N);
        float spec = pow(max(dot(R, V), 0.0), q);
        specular  += ks * spec * lightInt[i];
    }

    color = vec4((ambient + diffuse + specular) * selMult, 1.0);
}
)";

// Código fonte do Fragment Shader de texto (em GLSL): ainda hardcoded
const GLchar* textVertSrc = R"(
#version 330
layout (location = 0) in vec2 position;
uniform mat4 projection;
void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
}
)";

const GLchar* textFragSrc = R"(
#version 330
uniform vec3 textColor;
out vec4 color;
void main()
{
    color = vec4(textColor, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Classe Câmera
// ---------------------------------------------------------------------------
class Camera
{
public:
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;
    float     yaw;
    float     pitch;
    float     speed;
    float     sensitivity;
    float     fov;
    bool      firstMouse;
    float     lastX, lastY;

    Camera(glm::vec3 startPos)
    {
        pos         = startPos;
        front       = glm::vec3(0.0f, 0.0f, -1.0f);
        up          = glm::vec3(0.0f, 1.0f,  0.0f);
        yaw         = -90.0f;
        pitch       =   0.0f;
        speed       =   3.0f;
        sensitivity =   0.05f;
        fov         =  45.0f;
        firstMouse  = true;
        lastX       = WIDTH  / 2.0f;
        lastY       = HEIGHT / 2.0f;
    }

    void mover(GLFWwindow* window, float deltaTime)
    {
        float vel = speed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            pos += vel * front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            pos -= vel * front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            pos -= glm::normalize(glm::cross(front, up)) * vel;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            pos += glm::normalize(glm::cross(front, up)) * vel;
    }

    void rotacionar(float xoffset, float yoffset)
    {
        xoffset *= sensitivity;
        yoffset *= sensitivity;
        yaw     += xoffset;
        pitch   += yoffset;
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 f;
        f.x   = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y   = sin(glm::radians(pitch));
        f.z   = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);

        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, front));
    }

    glm::mat4 getView()
    {
        return glm::lookAt(pos, pos + front, up);
    }
};

// ---------------------------------------------------------------------------
// Estado global
// ---------------------------------------------------------------------------
enum Modo { NENHUM, ROTACAO };

struct Objeto3D
{
    GLuint    VAO;
    int       nVertices;
    GLuint    textureID;
    float     ka, kd, ks, q;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotAxis;
    float     rotAngle;
    bool      rotating;
    string    nome;

    // Trajetória
    vector<glm::vec3> pontos;
    int   pontoAtual;
    float tTraj;
    bool  seguindoTrajetoria;

    Objeto3D(GLuint vao, int nv, GLuint tex, float ka, float kd, float ks, float q,
             glm::vec3 pos, string n)
        : VAO(vao), nVertices(nv), textureID(tex), ka(ka), kd(kd), ks(ks), q(q),
          position(pos), scale(glm::vec3(0.5f)),
          rotAxis(glm::vec3(0.0f, 1.0f, 0.0f)),
          rotAngle(0.0f), rotating(false), nome(n),
          pontoAtual(0), tTraj(0.0f), seguindoTrajetoria(false) {}
};

vector<Objeto3D> objetos;
int   selecionado = 0;
Modo  modoAtual   = NENHUM;
bool  lightOn[3]  = { true, true, true };

Camera* gCamera = nullptr;

const float ROT_SPEED  = 1.5f;
const float TRAJ_SPEED = 1.0f; // unidades do mundo por segundo
const float TRANS_STEP = 0.1f;

// Recursos de texto
GLuint textShaderID = 0;
GLuint textVAO = 0, textVBO = 0, textEBO = 0;
GLint  textProjLoc = -1, textColorLoc = -1;


// Função MAIN
int main()
{
    // Inicialização da GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Criação da janela GLFW
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Modulo 6 - Trajetorias -- Anderson Koefender!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Fazendo o registro das funções de callback
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD: carrega todos os ponteiros de funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    // Compilando e buildando o programa de shader
    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    GLint modelLoc    = glGetUniformLocation(shaderID, "model");
    GLint viewLoc     = glGetUniformLocation(shaderID, "view");
    GLint projLoc     = glGetUniformLocation(shaderID, "projection");
    GLint texturaLoc  = glGetUniformLocation(shaderID, "textura");
    GLint camPosLoc   = glGetUniformLocation(shaderID, "camPos");
    GLint kaLoc       = glGetUniformLocation(shaderID, "ka");
    GLint kdLoc       = glGetUniformLocation(shaderID, "kd");
    GLint ksLoc       = glGetUniformLocation(shaderID, "ks");
    GLint qLoc        = glGetUniformLocation(shaderID, "q");
    GLint selMultLoc  = glGetUniformLocation(shaderID, "selMult");

    GLint lightPosLoc[3], lightIntLoc[3], lightOnLoc[3];
    for (int i = 0; i < 3; i++)
    {
        string base     = "lightPos[" + to_string(i) + "]";
        lightPosLoc[i]  = glGetUniformLocation(shaderID, base.c_str());
        base            = "lightInt[" + to_string(i) + "]";
        lightIntLoc[i]  = glGetUniformLocation(shaderID, base.c_str());
        base            = "lightOn["  + to_string(i) + "]";
        lightOnLoc[i]   = glGetUniformLocation(shaderID, base.c_str());
    }

    glUniform1i(texturaLoc, 0);

    float lightIntensities[3] = { 1.2f, 0.5f, 0.7f };
    for (int i = 0; i < 3; i++)
        glUniform1f(lightIntLoc[i], lightIntensities[i]);

    // Matriz de projeção perspectiva
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)WIDTH / (float)HEIGHT,
        0.1f, 100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    textShaderID = setupTextShader();
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glGenBuffers(1, &textEBO);
    textProjLoc  = glGetUniformLocation(textShaderID, "projection");
    textColorLoc = glGetUniformLocation(textShaderID, "textColor");

    glEnable(GL_DEPTH_TEST);

    // Câmera iniciando atrás dos objetos, olhando para a origem
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    gCamera = &camera;

    // Carregando os modelos OBJ
    int    nVerts = 0;
    GLuint texID  = 0;
    float  ka = 0.2f, kd = 0.7f, ks = 0.5f, q = 32.0f;

    GLuint vaoSuzanne = (GLuint)loadSimpleOBJ("assets/Modelos3D/Suzanne.obj", nVerts, texID, ka, kd, ks, q);
    if (vaoSuzanne != (GLuint)-1)
        objetos.push_back(Objeto3D(vaoSuzanne, nVerts, texID, ka, kd, ks, q, glm::vec3(-0.8f, 0.0f, 0.0f), "Suzanne"));

    ka = 0.2f; kd = 0.7f; ks = 0.5f; q = 32.0f;
    GLuint vaoCubo = (GLuint)loadSimpleOBJ("assets/Modelos3D/Cube.obj", nVerts, texID, ka, kd, ks, q);
    if (vaoCubo != (GLuint)-1)
        objetos.push_back(Objeto3D(vaoCubo, nVerts, texID, ka, kd, ks, q, glm::vec3( 0.8f, 0.0f, 0.0f), "Cubo"));

    if (objetos.empty())
    {
        cout << "Nenhum modelo carregado." << endl;
        glfwTerminate();
        return -1;
    }

    // Ativando o primeiro buffer de textura da OpenGL
    glActiveTexture(GL_TEXTURE0);

    float lastTime = (float)glfwGetTime();

    // Loop da aplicação - "game loop"
    while (!glfwWindowShouldClose(window))
    {
        float now       = (float)glfwGetTime();
        float deltaTime = now - lastTime;
        lastTime        = now;

        // Checa se houveram eventos de input e chama as funções de callback correspondentes
        glfwPollEvents();
        camera.mover(window, deltaTime);

        // Atualiza a matriz de projeção caso o FOV tenha mudado pelo scroll
        projection = glm::perspective(
            glm::radians(camera.fov),
            (float)WIDTH / (float)HEIGHT,
            0.1f, 100.0f);

        // Limpa o buffer de cor
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getView();

        // Posicionamento automático das luzes a partir do objeto principal
        glm::vec3 mp = objetos[0].position;
        float sc     = glm::length(objetos[0].scale);
        glm::vec3 lp[3] = {
            mp + glm::vec3(-sc * 1.5f,  sc * 1.5f,  sc * 2.5f),
            mp + glm::vec3( sc * 1.5f,  sc * 0.5f,  sc * 2.5f),
            mp + glm::vec3( 0.0f,       sc * 1.0f, -sc * 2.5f)
        };

        glUseProgram(shaderID);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(camPosLoc, 1, glm::value_ptr(camera.pos));

        for (int i = 0; i < 3; i++)
        {
            glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(lp[i]));
            glUniform1i(lightOnLoc[i], lightOn[i] ? 1 : 0);
        }

        for (int i = 0; i < (int)objetos.size(); i++)
        {
            if (objetos[i].rotating)
                objetos[i].rotAngle += ROT_SPEED * deltaTime;

            // Trajetória cíclica
            if (objetos[i].seguindoTrajetoria && (int)objetos[i].pontos.size() >= 2)
            {
                int n    = (int)objetos[i].pontos.size();
                int atual = objetos[i].pontoAtual;
                int prox  = (atual + 1) % n;
                float dist = glm::distance(objetos[i].pontos[atual], objetos[i].pontos[prox]);
                if (dist > 0.0001f)
                    objetos[i].tTraj += (TRAJ_SPEED / dist) * deltaTime;
                if (objetos[i].tTraj >= 1.0f)
                {
                    objetos[i].tTraj -= 1.0f;
                    objetos[i].pontoAtual = prox;
                    atual = objetos[i].pontoAtual;
                    prox  = (atual + 1) % n;
                }
                objetos[i].position = glm::mix(
                    objetos[i].pontos[atual],
                    objetos[i].pontos[prox],
                    glm::clamp(objetos[i].tTraj, 0.0f, 1.0f));
            }

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, objetos[i].position);
            model = glm::rotate(model, objetos[i].rotAngle, objetos[i].rotAxis);
            model = glm::scale(model, objetos[i].scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1f(kaLoc,     objetos[i].ka);
            glUniform1f(kdLoc,     objetos[i].kd);
            glUniform1f(ksLoc,     objetos[i].ks);
            glUniform1f(qLoc,      objetos[i].q);
            glUniform1f(selMultLoc, (i == selecionado) ? 1.2f : 1.0f);

            glBindTexture(GL_TEXTURE_2D, objetos[i].textureID);
            glBindVertexArray(objetos[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, objetos[i].nVertices);
        }
        glBindVertexArray(0);

        glDisable(GL_DEPTH_TEST);

        float s = 2.0f;
        string nomeObjeto = "Objeto: " + objetos[selecionado].nome;
        string nomeModo   = (modoAtual == ROTACAO) ? "Modo: ROTACAO (X/Y/Z = eixo)" : "Modo: nenhum";
        glm::vec3 corModo = (modoAtual == ROTACAO) ? glm::vec3(1.0f, 0.8f, 0.2f) : glm::vec3(0.6f, 0.6f, 0.6f);

        int np = (int)objetos[selecionado].pontos.size();
        bool trajOn = objetos[selecionado].seguindoTrajetoria;
        string nomeTraj = "Traj: " + to_string(np) + " pontos | " + (trajOn ? "ON" : "OFF");
        glm::vec3 corTraj = trajOn ? glm::vec3(0.3f, 1.0f, 0.3f) : glm::vec3(0.6f, 0.6f, 0.6f);

        renderText(nomeObjeto, 10, 10, s, glm::vec3(1.0f, 1.0f, 1.0f));
        renderText(nomeModo,   10, 28, s, corModo);
        renderText(nomeTraj,   10, 46, s, corTraj);

        glm::vec3 corOn  = glm::vec3(0.3f, 1.0f, 0.3f);
        glm::vec3 corOff = glm::vec3(0.5f, 0.5f, 0.5f);
        renderText(string("1 - Key Light:  ") + (lightOn[0] ? "ON" : "OFF"), 10, 74,  s, lightOn[0] ? corOn : corOff);
        renderText(string("2 - Fill Light: ") + (lightOn[1] ? "ON" : "OFF"), 10, 92,  s, lightOn[1] ? corOn : corOff);
        renderText(string("3 - Back Light: ") + (lightOn[2] ? "ON" : "OFF"), 10, 110, s, lightOn[2] ? corOn : corOff);

        float yBase = HEIGHT - 172.0f;
        glm::vec3 corHud = glm::vec3(0.75f, 0.75f, 0.75f);
        renderText("WASD       : mover camera",        10, yBase,       s, corHud);
        renderText("Mouse      : rotacionar camera",   10, yBase + 18,  s, corHud);
        renderText("TAB        : proximo objeto",      10, yBase + 36,  s, corHud);
        renderText("R + X/Y/Z  : rotacionar obj",      10, yBase + 54,  s, corHud);
        renderText("Setas      : mover obj X/Y",       10, yBase + 72,  s, corHud);
        renderText("PgUp/PgDn  : mover obj Z",         10, yBase + 90,  s, corHud);
        renderText("P : add ponto | F : traj | C : limpar", 10, yBase + 108, s, corHud);
        renderText("ESC        : sair",                10, yBase + 126, s, corHud);

        glEnable(GL_DEPTH_TEST);

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }

    // Finaliza a execução da GLFW, limpando os recursos alocados por ela
    glfwTerminate();
    return 0;
}


// Função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
        selecionado = (selecionado + 1) % (int)objetos.size();

    if (key == GLFW_KEY_R && action == GLFW_PRESS) modoAtual = ROTACAO;

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) lightOn[0] = !lightOn[0];
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) lightOn[1] = !lightOn[1];
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) lightOn[2] = !lightOn[2];

    if (modoAtual == ROTACAO && action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_X) { objetos[selecionado].rotAxis = glm::vec3(1,0,0); objetos[selecionado].rotating = true; }
        if (key == GLFW_KEY_Y) { objetos[selecionado].rotAxis = glm::vec3(0,1,0); objetos[selecionado].rotating = true; }
        if (key == GLFW_KEY_Z) { objetos[selecionado].rotAxis = glm::vec3(0,0,1); objetos[selecionado].rotating = true; }
    }

    // Translação do objeto selecionado (setas + PgUp/PgDn)
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (!objetos[selecionado].seguindoTrajetoria)
        {
            if (key == GLFW_KEY_LEFT)      objetos[selecionado].position.x -= TRANS_STEP;
            if (key == GLFW_KEY_RIGHT)     objetos[selecionado].position.x += TRANS_STEP;
            if (key == GLFW_KEY_UP)        objetos[selecionado].position.y += TRANS_STEP;
            if (key == GLFW_KEY_DOWN)      objetos[selecionado].position.y -= TRANS_STEP;
            if (key == GLFW_KEY_PAGE_UP)   objetos[selecionado].position.z -= TRANS_STEP;
            if (key == GLFW_KEY_PAGE_DOWN) objetos[selecionado].position.z += TRANS_STEP;
        }
    }

    // Adiciona posição atual como ponto de trajetória
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        objetos[selecionado].pontos.push_back(objetos[selecionado].position);
        cout << "Ponto " << objetos[selecionado].pontos.size()
             << " adicionado em ("
             << objetos[selecionado].position.x << ", "
             << objetos[selecionado].position.y << ", "
             << objetos[selecionado].position.z << ")" << endl;
    }

    // Inicia ou para a trajetória
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        if ((int)objetos[selecionado].pontos.size() >= 2)
        {
            objetos[selecionado].seguindoTrajetoria = !objetos[selecionado].seguindoTrajetoria;
            if (objetos[selecionado].seguindoTrajetoria)
            {
                objetos[selecionado].pontoAtual = 0;
                objetos[selecionado].tTraj      = 0.0f;
            }
        }
        else
            cout << "Adicione ao menos 2 pontos com P antes de iniciar a trajetoria" << endl;
    }

    // Limpa os pontos de trajetória
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        objetos[selecionado].pontos.clear();
        objetos[selecionado].pontoAtual        = 0;
        objetos[selecionado].tTraj             = 0.0f;
        objetos[selecionado].seguindoTrajetoria = false;
        cout << "Trajetoria limpa" << endl;
    }
}

// Função de callback do mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (gCamera->firstMouse)
    {
        gCamera->lastX     = (float)xpos;
        gCamera->lastY     = (float)ypos;
        gCamera->firstMouse = false;
    }

    float xoffset =  (float)xpos - gCamera->lastX;
    float yoffset =  gCamera->lastY - (float)ypos;
    gCamera->lastX = (float)xpos;
    gCamera->lastY = (float)ypos;

    gCamera->rotacionar(xoffset, yoffset);
}

// Função de callback do scroll
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera->fov -= (float)yoffset;
    if (gCamera->fov <  1.0f) gCamera->fov =  1.0f;
    if (gCamera->fov > 45.0f) gCamera->fov = 45.0f;
}


// Esta função está bastante hardcoded - objetivo é compilar e buildar um programa de
// shader simples e único neste exemplo de código
// A função retorna o identificador do programa de shader
static GLuint compileShader(GLenum type, const GLchar* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok; GLchar log[512];
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(s, 512, NULL, log); cout << log << endl; }
    return s;
}

int setupShader()
{
    GLuint vs   = compileShader(GL_VERTEX_SHADER,   vertexShaderSource);
    GLuint fs   = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

int setupTextShader()
{
    GLuint vs   = compileShader(GL_VERTEX_SHADER,   textVertSrc);
    GLuint fs   = compileShader(GL_FRAGMENT_SHADER, textFragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

void renderText(const string& texto, float x, float y, float escala, glm::vec3 cor)
{
    static char buffer[99999];
    int numQuads = stb_easy_font_print(0, 0, (char*)texto.c_str(), nullptr, buffer, sizeof(buffer));

    int numVerts = numQuads * 4;
    vector<float> verts(numVerts * 2);
    for (int i = 0; i < numVerts; i++)
    {
        float* src = (float*)(buffer + i * 16);
        verts[i * 2 + 0] = src[0] * escala + x;
        verts[i * 2 + 1] = src[1] * escala + y;
    }

    vector<unsigned int> indices;
    indices.reserve(numQuads * 6);
    for (int q = 0; q < numQuads; q++)
    {
        int b = q * 4;
        indices.push_back(b+0); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b+0); indices.push_back(b+2); indices.push_back(b+3);
    }

    glm::mat4 proj = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);

    glUseProgram(textShaderID);
    glUniformMatrix4fv(textProjLoc,  1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(textColorLoc, 1, glm::value_ptr(cor));

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

GLuint loadTexture(const string& path)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (!path.empty())
    {
        stbi_set_flip_vertically_on_load(true);
        int w, h, nrChannels;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &nrChannels, 0);

        if (data)
        {
            GLenum fmt = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            cout << "Failed to load texture " << path << endl;
            unsigned char white[] = { 255, 255, 255, 255 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        }
        stbi_image_free(data);
    }
    else
    {
        unsigned char white[] = { 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

static string loadMTL(const string& mtlPath, const string& baseDir,
                      float& ka, float& kd, float& ks, float& q)
{
    ifstream arq(mtlPath.c_str());
    if (!arq.is_open()) return "";

    string line, texName;
    while (getline(arq, line))
    {
        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "Ka") { float r,g,b; ss>>r>>g>>b; ka = (r+g+b)/3.0f; }
        else if (token == "Kd") { float r,g,b; ss>>r>>g>>b; kd = (r+g+b)/3.0f; }
        else if (token == "Ks") { float r,g,b; ss>>r>>g>>b; ks = (r+g+b)/3.0f; }
        else if (token == "Ns") { ss >> q; }
        else if (token == "map_Kd") { ss >> texName; }
    }
    return texName.empty() ? "" : baseDir + texName;
}

int loadSimpleOBJ(const string& filePath, int& nVertices, GLuint& textureID,
                  float& ka, float& kd, float& ks, float& q)
{
    string baseDir;
    size_t sep = filePath.find_last_of("/\\");
    if (sep != string::npos) baseDir = filePath.substr(0, sep + 1);

    vector<glm::vec3> positions;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat>   vBuffer;
    string            mtlFile;

    ifstream arq(filePath.c_str());
    if (!arq.is_open()) { cerr << "Erro ao abrir: " << filePath << endl; return -1; }

    string line;
    while (getline(arq, line))
    {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "mtllib") { ssline >> mtlFile; }
        else if (word == "v")  { glm::vec3 v; ssline>>v.x>>v.y>>v.z; positions.push_back(v); }
        else if (word == "vt") { glm::vec2 vt; ssline>>vt.s>>vt.t; texCoords.push_back(vt); }
        else if (word == "vn") { glm::vec3 vn; ssline>>vn.x>>vn.y>>vn.z; normals.push_back(vn); }
        else if (word == "f")
        {
            struct FV { int vi, ti, ni; };
            vector<FV> face;
            while (ssline >> word)
            {
                istringstream ss(word);
                string vs, ts, ns;
                getline(ss, vs, '/'); getline(ss, ts, '/'); getline(ss, ns, '/');
                FV fv;
                fv.vi = vs.empty() ? 0 : stoi(vs) - 1;
                fv.ti = ts.empty() ? 0 : stoi(ts) - 1;
                fv.ni = ns.empty() ? 0 : stoi(ns) - 1;
                face.push_back(fv);
            }
            for (int j = 1; j + 1 < (int)face.size(); j++)
            {
                int tri[3] = { 0, j, j + 1 };
                for (int k = 0; k < 3; k++)
                {
                    glm::vec3 p = positions[face[tri[k]].vi];
                    glm::vec2 t = texCoords.empty() ? glm::vec2(0.0f) : texCoords[face[tri[k]].ti];
                    glm::vec3 n = normals.empty()   ? glm::vec3(0,0,1) : normals[face[tri[k]].ni];
                    vBuffer.push_back(p.x); vBuffer.push_back(p.y); vBuffer.push_back(p.z);
                    vBuffer.push_back(t.s); vBuffer.push_back(t.t);
                    vBuffer.push_back(n.x); vBuffer.push_back(n.y); vBuffer.push_back(n.z);
                }
            }
        }
    }
    arq.close();

    string texPath;
    if (!mtlFile.empty()) texPath = loadMTL(baseDir + mtlFile, baseDir, ka, kd, ks, q);
    textureID = loadTexture(texPath.empty() ? "" : texPath);

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    //Atributo posição - coord x, y, z - 3 valores
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    //Atributo coordenada de textura - coord s, t - 2 valores
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    //Atributo componentes vetor normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = (int)(vBuffer.size() / 8);
    return (int)VAO;
}
