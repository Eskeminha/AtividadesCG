/* Módulo 2 - Instanciando objetos na cena 3D
 *
 * Baseado no projeto Hello3D de Rossana Baptista Queiroz
 * Disciplina de Computação Gráfica - Unisinos
 *
 * Objetivo: substituir a pirâmide por um cubo com faces coloridas,
 * adicionar controles de translação, escala uniforme e rotação,
 * e instanciar múltiplos cubos na cena.
 *
 * Controles:
 *   1 / 2 / 3    - selecionar cubo ativo
 *   X / Y / Z    - rotacionar no respectivo eixo
 *   W / S        - mover no eixo Z
 *   A / D        - mover no eixo X
 *   I / J        - mover no eixo Y (I sobe, J desce)
 *   [            - diminuir escala uniformemente
 *   ]            - aumentar escala uniformemente
 *   ESC          - fechar janela
 */

#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


// Protótipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int setupGeometry();

// Dimensões da janela
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Vertex Shader
const GLchar* vertexShaderSource = "#version 330\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = model * vec4(position, 1.0);\n"
"finalColor = vec4(color, 1.0);\n"
"}\0";

// Fragment Shader
const GLchar* fragmentShaderSource = "#version 330\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";


// Estrutura que representa um cubo na cena
struct Cube {
    glm::vec3 position;
    float scale;
    bool rotateX, rotateY, rotateZ;

    Cube(glm::vec3 pos)
        : position(pos), scale(0.35f),
          rotateX(false), rotateY(false), rotateZ(false) {}
};

// Lista de cubos e índice do cubo ativo
vector<Cube> cubos;
int cuboAtivo = 0;

const float PASSO_TRANSLACAO = 0.05f;
const float PASSO_ESCALA     = 0.05f;
const float ESCALA_MINIMA    = 0.05f;


int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Modulo 2 - Cubos 3D -- Anderson Koefender!", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version: " << version << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();
    GLuint VAO      = setupGeometry();

    glUseProgram(shaderID);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    glEnable(GL_DEPTH_TEST);

    // Instanciando 3 cubos em posições distintas no eixo X
    cubos.push_back(Cube(glm::vec3(-0.6f,  0.0f, 0.0f)));
    cubos.push_back(Cube(glm::vec3( 0.0f,  0.0f, 0.0f)));
    cubos.push_back(Cube(glm::vec3( 0.6f,  0.0f, 0.0f)));

    cout << "\n--- Controles ---" << endl;
    cout << "1 / 2 / 3  : selecionar cubo ativo (atual: 1)" << endl;
    cout << "X / Y / Z  : rotacionar no eixo" << endl;
    cout << "W/S        : mover eixo Z | A/D: mover eixo X | I/J: mover eixo Y" << endl;
    cout << "[  /  ]    : diminuir / aumentar escala" << endl;
    cout << "ESC        : sair" << endl;

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (GLfloat)glfwGetTime();

        glBindVertexArray(VAO);

        for (int i = 0; i < (int)cubos.size(); i++)
        {
            glm::mat4 model = glm::mat4(1.0f);

            // Translação
            model = glm::translate(model, cubos[i].position);

            // Rotação
            if (cubos[i].rotateX)
                model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
            else if (cubos[i].rotateY)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            else if (cubos[i].rotateZ)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

            // Escala uniforme
            model = glm::scale(model, glm::vec3(cubos[i].scale));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Desenha o cubo (36 vértices = 6 faces x 2 triângulos x 3 vértices)
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Selecionar cubo ativo
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        cuboAtivo = 0;
        cout << "Cubo ativo: 1" << endl;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        cuboAtivo = 1;
        cout << "Cubo ativo: 2" << endl;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        cuboAtivo = 2;
        cout << "Cubo ativo: 3" << endl;
    }

    // Rotação
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        cubos[cuboAtivo].rotateX = true;
        cubos[cuboAtivo].rotateY = false;
        cubos[cuboAtivo].rotateZ = false;
    }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        cubos[cuboAtivo].rotateX = false;
        cubos[cuboAtivo].rotateY = true;
        cubos[cuboAtivo].rotateZ = false;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        cubos[cuboAtivo].rotateX = false;
        cubos[cuboAtivo].rotateY = false;
        cubos[cuboAtivo].rotateZ = true;
    }

    // Translação - aceita PRESS e REPEAT para movimento contínuo
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubos[cuboAtivo].position.z -= PASSO_TRANSLACAO;

    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubos[cuboAtivo].position.z += PASSO_TRANSLACAO;

    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubos[cuboAtivo].position.x -= PASSO_TRANSLACAO;

    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubos[cuboAtivo].position.x += PASSO_TRANSLACAO;

    if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubos[cuboAtivo].position.y += PASSO_TRANSLACAO;

    if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubos[cuboAtivo].position.y -= PASSO_TRANSLACAO;

    // Escala uniforme
    if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        cubos[cuboAtivo].scale -= PASSO_ESCALA;
        if (cubos[cuboAtivo].scale < ESCALA_MINIMA)
            cubos[cuboAtivo].scale = ESCALA_MINIMA;
    }
    if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubos[cuboAtivo].scale += PASSO_ESCALA;
}


int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}


int setupGeometry()
{
    // Cubo centralizado na origem, lado = 1.0
    // 6 faces, cada face = 2 triângulos, cada triângulo = 3 vértices -> 36 vértices
    // Formato por vértice: x, y, z, r, g, b
    //
    // Cores por face:
    //   Frontal  (z = +0.5) -> Vermelho
    //   Traseira (z = -0.5) -> Verde
    //   Esquerda (x = -0.5) -> Azul
    //   Direita  (x = +0.5) -> Amarelo
    //   Superior (y = +0.5) -> Ciano
    //   Inferior (y = -0.5) -> Magenta

    GLfloat vertices[] = {

        // --- Face frontal (z = +0.5) - VERMELHA ---
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,

        // --- Face traseira (z = -0.5) - VERDE ---
         0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,

         0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,

        // --- Face esquerda (x = -0.5) - AZUL ---
        -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,

        -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 0.0f, 1.0f,

        // --- Face direita (x = +0.5) - AMARELA ---
         0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 0.0f,

         0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 0.0f,

        // --- Face superior (y = +0.5) - CIANO ---
        -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,

        -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,

        // --- Face inferior (y = -0.5) - MAGENTA ---
        -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,

        -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,
    };

    GLuint VBO, VAO;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Atributo 0: posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}
