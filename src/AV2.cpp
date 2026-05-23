/* Módulo 3 - Texturização de objetos 3D
 *
 * Baseado no projeto AV1
 * Disciplina de Computação Gráfica - Unisinos
 *
 * Novidades em relação ao AV1:
 *   - Leitura das coordenadas de textura (vt) do arquivo .OBJ
 *   - Leitura do arquivo .MTL para obter o nome da textura (map_Kd)
 *   - Carregamento da imagem com stb_image e envio para a GPU
 *   - Shader atualizado para amostrar a textura via sampler2D
 *
 * Controles:
 *   TAB          - seleciona o próximo objeto
 *   R            - modo rotação  -> X / Y / Z escolhe o eixo
 *   T            - modo translação -> setas (X/Y) | W/E (Z)
 *   S            - modo escala   -> = aumenta | - diminui
 *   ESC          - fecha a janela
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb_image para carregamento de texturas
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// stb_easy_font para renderização de texto 2D
#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"


// ---------------------------------------------------------------------------
// Protótipos
// ---------------------------------------------------------------------------
void   key_callback(GLFWwindow* window, int key, int scancode, int action, int mod);
int    setupShader();
int    setupTextShader();
void   renderText(const string& texto, float x, float y, float escala, glm::vec3 cor);
int    loadSimpleOBJ(const string& filePath, int& nVertices, GLuint& textureID);
GLuint loadTexture(const string& path);

// ---------------------------------------------------------------------------
// Dimensões da janela
// ---------------------------------------------------------------------------
const GLuint WIDTH = 1000, HEIGHT = 1000;

// ---------------------------------------------------------------------------
// Shader 3D — usa coordenadas de textura no lugar da cor por vértice
// ---------------------------------------------------------------------------
const GLchar* vertexShaderSource = "#version 330\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec2 texCoord;\n"
"uniform mat4 model;\n"
"out vec2 TexCoord;\n"
"void main()\n"
"{\n"
"gl_Position = model * vec4(position, 1.0);\n"
"TexCoord = texCoord;\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 330\n"
"in vec2 TexCoord;\n"
"uniform sampler2D textura;\n"
"uniform float brightness;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = texture(textura, TexCoord) * brightness;\n"
"}\n\0";

// ---------------------------------------------------------------------------
// Shader 2D para texto
// ---------------------------------------------------------------------------
const GLchar* textVertSrc = "#version 330\n"
"layout (location = 0) in vec2 position;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"gl_Position = projection * vec4(position, 0.0, 1.0);\n"
"}\0";

const GLchar* textFragSrc = "#version 330\n"
"uniform vec3 textColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = vec4(textColor, 1.0);\n"
"}\n\0";

// ---------------------------------------------------------------------------
// Modos de interação
// ---------------------------------------------------------------------------
enum Modo { NENHUM, ROTACAO, TRANSLACAO, ESCALA };

// ---------------------------------------------------------------------------
// Estrutura que representa um objeto 3D na cena
// ---------------------------------------------------------------------------
struct Objeto3D
{
    GLuint    VAO;
    int       nVertices;
    GLuint    textureID;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotAxis;
    float     rotAngle;
    bool      rotating;
    string    nome;

    Objeto3D(GLuint vao, int nv, GLuint tex, glm::vec3 pos, string n)
        : VAO(vao), nVertices(nv), textureID(tex), position(pos),
          scale(glm::vec3(0.3f)),
          rotAxis(glm::vec3(0.0f, 1.0f, 0.0f)),
          rotAngle(0.0f), rotating(false), nome(n) {}
};

// ---------------------------------------------------------------------------
// Estado global
// ---------------------------------------------------------------------------
vector<Objeto3D> objetos;
int  selecionado = 0;
Modo modoAtual   = NENHUM;

const float PASSO_TRANS  = 0.05f;
const float PASSO_ESCALA = 0.1f;
const float ESCALA_MIN   = 0.05f;
const float ROT_SPEED    = 1.5f;

// Recursos de texto
GLuint textShaderID = 0;
GLuint textVAO = 0, textVBO = 0, textEBO = 0;
GLint  textProjLoc = -1, textColorLoc = -1;


// ---------------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------------
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
        "Modulo 3 - Texturas -- Anderson Koefender!", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    GLint modelLoc      = glGetUniformLocation(shaderID, "model");
    GLint brightnessLoc = glGetUniformLocation(shaderID, "brightness");
    GLint texturaLoc    = glGetUniformLocation(shaderID, "textura");
    glUniform1i(texturaLoc, 0); // GL_TEXTURE0

    textShaderID = setupTextShader();
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glGenBuffers(1, &textEBO);
    textProjLoc  = glGetUniformLocation(textShaderID, "projection");
    textColorLoc = glGetUniformLocation(textShaderID, "textColor");

    glEnable(GL_DEPTH_TEST);

    // Carregamento dos modelos OBJ com textura
    int    nVerts = 0;
    GLuint texID  = 0;

    GLuint vaoSuzanne = loadSimpleOBJ("assets/Modelos3D/Suzanne.obj", nVerts, texID);
    if (vaoSuzanne != (GLuint)-1)
        objetos.push_back(Objeto3D(vaoSuzanne, nVerts, texID, glm::vec3(-0.4f, 0.0f, 0.0f), "Suzanne"));

    GLuint vaoCubo = loadSimpleOBJ("assets/Modelos3D/Cube.obj", nVerts, texID);
    if (vaoCubo != (GLuint)-1)
        objetos.push_back(Objeto3D(vaoCubo, nVerts, texID, glm::vec3( 0.4f, 0.0f, 0.0f), "Cubo"));

    if (objetos.empty())
    {
        cout << "Nenhum modelo carregado." << endl;
        glfwTerminate();
        return -1;
    }

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        float currentTime = (float)glfwGetTime();
        float deltaTime   = currentTime - lastTime;
        lastTime          = currentTime;

        glfwPollEvents();

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Renderização 3D
        glUseProgram(shaderID);
        for (int i = 0; i < (int)objetos.size(); i++)
        {
            if (objetos[i].rotating)
                objetos[i].rotAngle += ROT_SPEED * deltaTime;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, objetos[i].position);
            model = glm::rotate(model, objetos[i].rotAngle, objetos[i].rotAxis);
            model = glm::scale(model, objetos[i].scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1f(brightnessLoc, (i == selecionado) ? 1.0f : 0.6f);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, objetos[i].textureID);

            glBindVertexArray(objetos[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, objetos[i].nVertices);
        }
        glBindVertexArray(0);

        // HUD de texto
        glDisable(GL_DEPTH_TEST);

        string nomeObjeto = "Objeto: " + objetos[selecionado].nome;
        string nomeModo;
        glm::vec3 corModo;
        switch (modoAtual)
        {
            case ROTACAO:    nomeModo = "Modo: ROTACAO   (X/Y/Z = eixo)";          corModo = glm::vec3(1.0f, 0.8f, 0.2f); break;
            case TRANSLACAO: nomeModo = "Modo: TRANSLACAO (setas X/Y | W/E = Z)";  corModo = glm::vec3(0.3f, 1.0f, 0.4f); break;
            case ESCALA:     nomeModo = "Modo: ESCALA    (= aumenta | - diminui)"; corModo = glm::vec3(0.4f, 0.7f, 1.0f); break;
            default:         nomeModo = "Modo: nenhum";                             corModo = glm::vec3(0.6f, 0.6f, 0.6f); break;
        }

        float s = 2.0f;
        renderText(nomeObjeto, 10, 10, s, glm::vec3(1.0f, 1.0f, 1.0f));
        renderText(nomeModo,   10, 30, s, corModo);

        float yBase = HEIGHT - 120.0f;
        renderText("TAB   : proximo objeto",             10, yBase,      s, glm::vec3(0.75f, 0.75f, 0.75f));
        renderText("R     : rotacao",                    10, yBase + 18, s, glm::vec3(0.75f, 0.75f, 0.75f));
        renderText("T     : translacao (setas + W/E=Z)", 10, yBase + 36, s, glm::vec3(0.75f, 0.75f, 0.75f));
        renderText("S     : escala",                     10, yBase + 54, s, glm::vec3(0.75f, 0.75f, 0.75f));
        renderText("ESC   : sair",                       10, yBase + 72, s, glm::vec3(0.75f, 0.75f, 0.75f));

        glEnable(GL_DEPTH_TEST);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}


// ---------------------------------------------------------------------------
// Callback de teclado (idêntico ao AV1)
// ---------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
        selecionado = (selecionado + 1) % (int)objetos.size();

    if (key == GLFW_KEY_R && action == GLFW_PRESS) modoAtual = ROTACAO;
    if (key == GLFW_KEY_T && action == GLFW_PRESS) modoAtual = TRANSLACAO;
    if (key == GLFW_KEY_S && action == GLFW_PRESS) modoAtual = ESCALA;

    if (modoAtual == ROTACAO && action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_X) { objetos[selecionado].rotAxis = glm::vec3(1,0,0); objetos[selecionado].rotating = true; }
        if (key == GLFW_KEY_Y) { objetos[selecionado].rotAxis = glm::vec3(0,1,0); objetos[selecionado].rotating = true; }
        if (key == GLFW_KEY_Z) { objetos[selecionado].rotAxis = glm::vec3(0,0,1); objetos[selecionado].rotating = true; }
    }

    if (modoAtual == TRANSLACAO && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        glm::vec3& pos = objetos[selecionado].position;
        if (key == GLFW_KEY_LEFT)  pos.x -= PASSO_TRANS;
        if (key == GLFW_KEY_RIGHT) pos.x += PASSO_TRANS;
        if (key == GLFW_KEY_UP)    pos.y += PASSO_TRANS;
        if (key == GLFW_KEY_DOWN)  pos.y -= PASSO_TRANS;
        if (key == GLFW_KEY_W) pos.z = glm::max(pos.z - PASSO_TRANS, -0.5f);
        if (key == GLFW_KEY_E) pos.z = glm::min(pos.z + PASSO_TRANS,  0.5f);
    }

    if (modoAtual == ESCALA && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        glm::vec3& sc = objetos[selecionado].scale;
        if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_UP)
            sc += glm::vec3(PASSO_ESCALA);
        if (key == GLFW_KEY_MINUS || key == GLFW_KEY_DOWN)
        {
            sc -= glm::vec3(PASSO_ESCALA);
            if (sc.x < ESCALA_MIN) sc = glm::vec3(ESCALA_MIN);
        }
    }
}


// ---------------------------------------------------------------------------
// Renderiza texto na tela usando stb_easy_font
// ---------------------------------------------------------------------------
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


// ---------------------------------------------------------------------------
// Compilação de shaders (helper interno)
// ---------------------------------------------------------------------------
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
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

int setupTextShader()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER,   textVertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, textFragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}


// ---------------------------------------------------------------------------
// Carrega uma imagem e cria uma textura OpenGL
// Retorna 0 e gera textura branca 1x1 se o arquivo não for encontrado
// ---------------------------------------------------------------------------
GLuint loadTexture(const string& path)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int w, h, channels;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);

    if (data)
    {
        GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        cout << "Textura carregada: " << path << " (" << w << "x" << h << ")" << endl;
    }
    else
    {
        // Textura branca 1x1 como fallback
        unsigned char white[] = { 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        cout << "Textura nao encontrada: " << path << " (usando branco)" << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}


// ---------------------------------------------------------------------------
// Lê o arquivo .MTL e retorna o caminho da textura difusa (map_Kd)
// ---------------------------------------------------------------------------
static string loadMTL(const string& mtlPath, const string& baseDir)
{
    ifstream arq(mtlPath.c_str());
    if (!arq.is_open()) return "";

    string line, texName;
    while (getline(arq, line))
    {
        istringstream ss(line);
        string token;
        ss >> token;
        if (token == "map_Kd")
        {
            ss >> texName;
            break;
        }
    }
    return texName.empty() ? "" : baseDir + texName;
}


// ---------------------------------------------------------------------------
// Carregamento de .OBJ com coordenadas de textura
// Formato do vBuffer: x, y, z, s, t  (5 floats por vértice)
// ---------------------------------------------------------------------------
int loadSimpleOBJ(const string& filePath, int& nVertices, GLuint& textureID)
{
    // Determina o diretório base para localizar o .mtl e a textura
    string baseDir;
    size_t sep = filePath.find_last_of("/\\");
    if (sep != string::npos) baseDir = filePath.substr(0, sep + 1);

    vector<glm::vec3> positions;
    vector<glm::vec2> texCoords;
    vector<GLfloat>   vBuffer;
    string            mtlFile;

    ifstream arqEntrada(filePath.c_str());
    if (!arqEntrada.is_open())
    {
        cerr << "Erro ao abrir: " << filePath << endl;
        return -1;
    }

    string line;
    while (getline(arqEntrada, line))
    {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "mtllib")
        {
            ssline >> mtlFile;
        }
        else if (word == "v")
        {
            glm::vec3 v;
            ssline >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "f")
        {
            // Cada token: vi/ti/ni — extrai vi e ti
            vector<pair<int,int>> face;
            while (ssline >> word)
            {
                istringstream ss(word);
                string vi_s, ti_s;
                getline(ss, vi_s, '/');
                getline(ss, ti_s, '/');
                int vi = vi_s.empty() ? 0 : stoi(vi_s) - 1;
                int ti = ti_s.empty() ? 0 : stoi(ti_s) - 1;
                face.push_back({vi, ti});
            }
            // Fan triangulation
            for (int j = 1; j + 1 < (int)face.size(); j++)
            {
                pair<int,int> tri[3] = { face[0], face[j], face[j+1] };
                for (int k = 0; k < 3; k++)
                {
                    glm::vec3 p = positions[tri[k].first];
                    glm::vec2 t = texCoords.empty() ? glm::vec2(0.0f) : texCoords[tri[k].second];
                    vBuffer.push_back(p.x);
                    vBuffer.push_back(p.y);
                    vBuffer.push_back(p.z);
                    vBuffer.push_back(t.s);
                    vBuffer.push_back(t.t);
                }
            }
        }
    }
    arqEntrada.close();

    // Carrega a textura a partir do MTL
    string texPath = mtlFile.empty() ? "" : loadMTL(baseDir + mtlFile, baseDir);
    textureID = loadTexture(texPath.empty() ? "" : texPath);

    cout << "Carregado: " << filePath << " (" << vBuffer.size() / 5 << " vertices)" << endl;

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Atributo 0: posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: coordenada de textura (s, t)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = (int)(vBuffer.size() / 5);
    return VAO;
}
