/* Módulo 4 - Modelo de Iluminação de Phong
 * Disciplina de Computação Gráfica - Unisinos
 *
 * Controles:
 *   X   - rotaciona no eixo X
 *   Y   - rotaciona no eixo Y
 *   Z   - rotaciona no eixo Z
 *   ESC - fecha a janela
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace glm;

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int    setupShader();
int    loadSimpleOBJ(const string& filePath, int& nVertices, GLuint& textureID,
                     float& ka, float& kd, float& ks, float& q);
GLuint loadTexture(const string& path);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 800;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = R"(
#version 330
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

uniform mat4 projection;
uniform mat4 model;

out vec2 TexCoord;
out vec3 vNormal;
out vec3 fragPos;

void main()
{
    vec4 wp     = model * vec4(position, 1.0);
    gl_Position = projection * wp;
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

uniform sampler2D texBuff;
uniform vec3  lightPos;
uniform vec3  camPos;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float q;

out vec4 color;

void main()
{
    vec3 lightColor  = vec3(1.0, 1.0, 1.0);
    vec3 objectColor = vec3(texture(texBuff, TexCoord));

    //Coeficiente de luz ambiente
    vec3 ambient = ka * lightColor;

    //Coeficiente de reflexão difusa
    vec3  N    = normalize(vNormal);
    vec3  L    = normalize(lightPos - fragPos);
    float diff = max(dot(N, L), 0.0);
    vec3  diffuse = kd * diff * lightColor;

    //Coeficiente de reflexão especular
    vec3  R    = normalize(reflect(-L, N));
    vec3  V    = normalize(camPos - fragPos);
    float spec = max(dot(R, V), 0.0);
    spec = pow(spec, q);
    vec3  specular = ks * spec * lightColor;

    vec3 result = (ambient + diffuse) * objectColor + specular;
    color = vec4(result, 1.0);
}
)";

// Estado de rotação
bool rotX = false, rotY = false, rotZ = false;

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
        "Modulo 4 - Phong -- Anderson Koefender!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Fazendo o registro da função de callback para a janela GLFW
    glfwSetKeyCallback(window, key_callback);

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

    // Carregando o modelo OBJ e os coeficientes do material (.mtl)
    int    nVertices = 0;
    GLuint texID     = 0;
    float  ka = 0.2f, kd = 0.7f, ks = 0.5f, q = 32.0f;

    GLuint VAO = (GLuint)loadSimpleOBJ(
        "assets/Modelos3D/Suzanne.obj", nVertices, texID, ka, kd, ks, q);

    if (VAO == (GLuint)-1)
    {
        cout << "Erro ao carregar modelo." << endl;
        glfwTerminate();
        return -1;
    }

    vec3 lightPos = vec3(2.0f, 2.0f, 2.0f);
    vec3 camPos   = vec3(0.0f, 0.0f, 2.0f);

    // Matriz de projeção paralela ortográfica
    mat4 projection = ortho(-1.0f, 1.0f, -1.0f, 1.0f, -5.0f, 5.0f);

    glUseProgram(shaderID);

    // Enviar a informação de qual variável armazenará o buffer da textura
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"),   0);
    glUniform1f(glGetUniformLocation(shaderID, "ka"),        ka);
    glUniform1f(glGetUniformLocation(shaderID, "kd"),        kd);
    glUniform1f(glGetUniformLocation(shaderID, "ks"),        ks);
    glUniform1f(glGetUniformLocation(shaderID, "q"),         q);
    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1, value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "camPos"),   1, value_ptr(camPos));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    glEnable(GL_DEPTH_TEST);

    // Ativando o primeiro buffer de textura da OpenGL
    glActiveTexture(GL_TEXTURE0);

    float angle   = 0.0f;
    float lastTime = (float)glfwGetTime();

    // Loop da aplicação - "game loop"
    while (!glfwWindowShouldClose(window))
    {
        // Checa se houveram eventos de input e chama as funções de callback correspondentes
        float now   = (float)glfwGetTime();
        float delta = now - lastTime;
        lastTime    = now;

        glfwPollEvents();

        // Limpa o buffer de cor
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (rotX || rotY || rotZ) angle += 1.0f * delta;

        vec3 axis  = rotX ? vec3(1,0,0) : rotY ? vec3(0,1,0) : vec3(0,0,1);
        mat4 model = rotate(mat4(1.0f), angle, axis);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

        glBindTexture(GL_TEXTURE_2D, texID);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }

    // Pede pra OpenGL desalocar os buffers
    glDeleteVertexArrays(1, &VAO);

    // Finaliza a execução da GLFW, limpando os recursos alocados por ela
    glfwTerminate();
    return 0;
}

// Função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_X && action == GLFW_PRESS) { rotX = true;  rotY = false; rotZ = false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) { rotX = false; rotY = true;  rotZ = false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { rotX = false; rotY = false; rotZ = true;  }
}

// Esta função está bastante hardcoded - objetivo é compilar e buildar um programa de
// shader simples e único neste exemplo de código
// A função retorna o identificador do programa de shader
int setupShader()
{
    // Vertex shader
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

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    // Linkando os shaders e criando o identificador do programa de shader
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

GLuint loadTexture(const string& path)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

static void loadMTL(const string& mtlPath, const string& baseDir,
                    float& ka, float& kd, float& ks, float& q, string& texPath)
{
    ifstream arq(mtlPath.c_str());
    if (!arq.is_open()) return;

    string line;
    while (getline(arq, line))
    {
        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "Ka") { float r,g,b; ss>>r>>g>>b; ka = (r+g+b)/3.0f; }
        else if (token == "Kd") { float r,g,b; ss>>r>>g>>b; kd = (r+g+b)/3.0f; }
        else if (token == "Ks") { float r,g,b; ss>>r>>g>>b; ks = (r+g+b)/3.0f; }
        else if (token == "Ns") { ss >> q; }
        else if (token == "map_Kd") { string f; ss >> f; texPath = baseDir + f; }
    }
}

int loadSimpleOBJ(const string& filePath, int& nVertices, GLuint& textureID,
                  float& ka, float& kd, float& ks, float& q)
{
    string baseDir;
    size_t sep = filePath.find_last_of("/\\");
    if (sep != string::npos) baseDir = filePath.substr(0, sep + 1);

    vector<vec3>    positions;
    vector<vec2>    texCoords;
    vector<vec3>    normals;
    vector<GLfloat> vBuffer;
    string          mtlFile;

    ifstream arq(filePath.c_str());
    if (!arq.is_open()) { cerr << "Erro ao abrir: " << filePath << endl; return -1; }

    string line;
    while (getline(arq, line))
    {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "mtllib") { ssline >> mtlFile; }
        else if (word == "v")  { vec3 v; ssline>>v.x>>v.y>>v.z; positions.push_back(v); }
        else if (word == "vt") { vec2 vt; ssline>>vt.s>>vt.t; texCoords.push_back(vt); }
        else if (word == "vn") { vec3 vn; ssline>>vn.x>>vn.y>>vn.z; normals.push_back(vn); }
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
                    vec3 p = positions[face[tri[k]].vi];
                    vec2 t = texCoords.empty() ? vec2(0.0f) : texCoords[face[tri[k]].ti];
                    vec3 n = normals.empty()   ? vec3(0,0,1) : normals[face[tri[k]].ni];
                    vBuffer.push_back(p.x); vBuffer.push_back(p.y); vBuffer.push_back(p.z);
                    vBuffer.push_back(t.s); vBuffer.push_back(t.t);
                    vBuffer.push_back(n.x); vBuffer.push_back(n.y); vBuffer.push_back(n.z);
                }
            }
        }
    }
    arq.close();

    string texPath;
    if (!mtlFile.empty()) loadMTL(baseDir + mtlFile, baseDir, ka, kd, ks, q, texPath);
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
