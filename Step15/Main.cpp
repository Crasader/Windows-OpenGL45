#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <vector>
#include <cmath>
#include "glcorearb.h"
#include "wglext.h"
#include "OpenGL45.h"
#include "resource.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HGLRC CreateOpenGLContext(HWND hwnd);
void DeleteOpenGLContext(HGLRC hglrc);
void RegisterErrorCallback();
GLuint LoadShader();
GLuint LoadTexture(WORD resourceId);
void SetupModel(HWND hwnd);
void Paint(HWND hwnd);

static FILE *logFile;

static GLuint positionBuffer;
static GLuint uvBuffer;
static GLuint indexBuffer;
static GLuint vertexArray;
static GLfloat positions[4 * 2] = {
    0.0f, 0.0f,
    0.0f, 300.0f,
    400.0f, 300.0f,
    400.0f, 0.0f
};
static GLfloat uvs[4 * 2] = {
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f
};
static GLuint indices[6] = {
    0, 1, 2, 0, 2, 3
};

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    fopen_s(&logFile, "log.txt", "w");

    LPCTSTR class_name = TEXT("OpenGL Window");

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = static_cast<HCURSOR>(
        LoadImage(nullptr, MAKEINTRESOURCE(OCR_NORMAL),
            IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = class_name;
    wcex.hIconSm = nullptr;

    RegisterClassEx(&wcex);

    HWND hwnd = CreateWindow(
        class_name, TEXT("Step 15"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    fclose(logFile);

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HGLRC hglrc;
    static GLuint program;
    static GLuint width, height, texture;
    static GLuint imgWidth, imgHeight;

    switch (uMsg)
    {
    case WM_CREATE:
        hglrc = CreateOpenGLContext(hwnd);
        InitOpenGLFunctions();
        RegisterErrorCallback();
        program = LoadShader();

        width = glGetUniformLocation(program, "width");
        height = glGetUniformLocation(program, "height");

        texture = glGetUniformLocation(program, "texture");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, LoadTexture(ID_TEXTURE));
        glUniform1i(texture, 0);

        imgWidth = glGetUniformLocation(program, "img_width");
        imgHeight = glGetUniformLocation(program, "img_height");
        glUniform1f(imgWidth, 400.0f);
        glUniform1f(imgHeight, 300.0f);

        SetupModel(hwnd);
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        return 0;

    case WM_PAINT:
        Paint(hwnd);
        return 0;

    case WM_SIZE:
        glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
        glUniform1f(width, LOWORD(lParam));
        glUniform1f(height, HIWORD(lParam));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_DESTROY:
        DeleteOpenGLContext(hglrc);
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

HGLRC CreateOpenGLContext(HWND hwnd)
{
    PIXELFORMATDESCRIPTOR pfd;
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cRedBits = 0;
    pfd.cRedShift = 0;
    pfd.cGreenBits = 0;
    pfd.cGreenShift = 0;
    pfd.cBlueBits = 0;
    pfd.cBlueShift = 0;
    pfd.cAlphaBits = 0;
    pfd.cAlphaShift = 0;
    pfd.cAccumBits = 0;
    pfd.cAccumRedBits = 0;
    pfd.cAccumGreenBits = 0;
    pfd.cAccumBlueBits = 0;
    pfd.cAccumAlphaBits = 0;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.cAuxBuffers = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.bReserved = 0;
    pfd.dwLayerMask = 0;
    pfd.dwVisibleMask = 0;
    pfd.dwDamageMask = 0;

    HDC hdc = GetDC(hwnd);
    int format = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, format, &pfd);

    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);

    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
        reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
            wglGetProcAddress("wglCreateContextAttribsARB"));

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(hglrc);

    const int attribList[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    hglrc = wglCreateContextAttribsARB(hdc, 0, attribList);
    wglMakeCurrent(hdc, hglrc);

    return hglrc;
}

void DeleteOpenGLContext(HGLRC hglrc)
{
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(hglrc);
}

void APIENTRY OpenGLErrorCallback(
    GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length,
    const GLchar *message, const void *param)
{
    const char *sourceStr = DebugSourceToString(source);
    const char *typeStr = DebugTypeToString(type);
    const char *severityStr = DebugSeverityToString(severity);

    fprintf(logFile, "%s:%s[%s](%d) %s\n",
        sourceStr, typeStr, severityStr, id, message);
}

void RegisterErrorCallback()
{
    glDebugMessageCallback(OpenGLErrorCallback, nullptr);
    glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    glEnable(GL_DEBUG_OUTPUT);
}

void AttachShader(GLuint program, GLuint shaderType, WORD resourceId)
{
    HRSRC hrsrc = FindResource(
        nullptr, MAKEINTRESOURCE(resourceId), TEXT("SHADER"));
    HGLOBAL hglobal = LoadResource(nullptr, hrsrc);
    const GLchar *shaderString = static_cast<const GLchar *>(LockResource(hglobal));
    const GLint shaderLength = SizeofResource(nullptr, hrsrc);

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderString, &shaderLength);
    glCompileShader(shader);

    GLint result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length > 0)
        {
            GLchar *buf = new GLchar[length];
            glGetShaderInfoLog(shader, length, nullptr, buf);
            fprintf(logFile, "Shader compilation failed:\n%s\n", buf);
            delete[] buf;
        }
    }

    glAttachShader(program, shader);
    glDeleteShader(shader);
}

GLuint LoadShader()
{
    GLuint program = glCreateProgram();
    AttachShader(program, GL_VERTEX_SHADER, ID_VERTEX_SHADER);
    AttachShader(program, GL_FRAGMENT_SHADER, ID_FRAGMENT_SHADER);
    glLinkProgram(program);
    glUseProgram(program);
    return program;
}

GLuint LoadTexture(WORD resourceId)
{
    HRSRC hrsrc = FindResource(
        nullptr, MAKEINTRESOURCE(resourceId), RT_BITMAP);
    HGLOBAL hglobal = LoadResource(nullptr, hrsrc);
    const GLbyte *data = static_cast<const GLbyte *>(LockResource(hglobal));

    const int width = *reinterpret_cast<const int *>(data + 4);
    const int height = *reinterpret_cast<const int *>(data + 8);

    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_RGB8, width, height);
    glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, data + 40);

    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    return texture;
}

void SetupModel(HWND hwnd)
{
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Vertex Buffer (position)
    GLuint positionLocation = 0;
    GLuint positionBindindex = 0;

    glCreateBuffers(1, &positionBuffer);

    glNamedBufferData(positionBuffer,
        sizeof(positions), positions, GL_STATIC_DRAW);

    // Vertex Buffer (uv)
    GLuint uvLocation = 1;
    GLuint uvBindindex = 1;

    glCreateBuffers(1, &uvBuffer);

    glNamedBufferData(uvBuffer,
        sizeof(uvs), uvs, GL_STATIC_DRAW);

    // Index Buffer

    glCreateBuffers(1, &indexBuffer);

    glNamedBufferData(indexBuffer,
        sizeof(indices), indices, GL_STATIC_DRAW);

    // Vertex Array
    glCreateVertexArrays(1, &vertexArray);

    glEnableVertexArrayAttrib(vertexArray, positionLocation);
    glVertexArrayAttribFormat(vertexArray, positionLocation,
        2, GL_FLOAT, GL_FALSE, 0);

    glVertexArrayAttribBinding(vertexArray, positionLocation,
        positionBindindex);
    glVertexArrayVertexBuffer(vertexArray, positionBindindex,
        positionBuffer, static_cast<GLintptr>(0), sizeof(GLfloat) * 2);

    glEnableVertexArrayAttrib(vertexArray, uvLocation);
    glVertexArrayAttribFormat(vertexArray, uvLocation,
        2, GL_FLOAT, GL_FALSE, 0);

    glVertexArrayAttribBinding(vertexArray, uvLocation,
        uvBindindex);
    glVertexArrayVertexBuffer(vertexArray, uvBindindex,
        uvBuffer, static_cast<GLintptr>(0), sizeof(GLfloat) * 2);

    glVertexArrayElementBuffer(vertexArray, indexBuffer);
}

void Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vertexArray);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    SwapBuffers(hdc);
    EndPaint(hwnd, &ps);
}
