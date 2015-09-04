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
void SetupModel(HWND hwnd);
void UpdateVertexBuffer();
void Paint(HWND hwnd);

static FILE *logFile;

static GLuint positionBuffer;
static GLuint vertexArray;
static GLfloat points[3 * 2];

static int drag_index;
static bool is_mouse_down = false;

static GLuint color;

static const float pi = std::acosf(-1.0f);

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
        class_name, TEXT("Step 13"),
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
    static GLuint width, height;

    switch (uMsg)
    {
    case WM_CREATE:
        hglrc = CreateOpenGLContext(hwnd);
        InitOpenGLFunctions();
        RegisterErrorCallback();
        program = LoadShader();
        width = glGetUniformLocation(program, "width");
        height = glGetUniformLocation(program, "height");
        color = glGetUniformLocation(program, "color");
        SetupModel(hwnd);
        UpdateVertexBuffer();
        glClearColor(0.6f, 0.8f, 0.8f, 1.0f);
        glPointSize(8.0f);
        glLineWidth(2.0f);
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

    case WM_LBUTTONDOWN:
    {
        float mx = LOWORD(lParam);
        float my = HIWORD(lParam);

        for (int i = 0; i < 3; ++i)
        {
            float px = points[i * 2];
            float py = points[i * 2 + 1];
            if (std::sqrtf((px - mx) * (px - mx) + (py - my) * (py - my)) < 5.0f)
            {
                drag_index = i;
                is_mouse_down = true;
                points[drag_index * 2] = mx;
                points[drag_index * 2 + 1] = my;
                UpdateVertexBuffer();
                InvalidateRect(hwnd, nullptr, FALSE);
                break;
            }
        }
    }
    return 0;

    case WM_LBUTTONUP:
        is_mouse_down = false;
        UpdateVertexBuffer();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_MOUSEMOVE:
        if (is_mouse_down)
        {
            points[drag_index * 2] = LOWORD(lParam);
            points[drag_index * 2 + 1] = HIWORD(lParam);
            UpdateVertexBuffer();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
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

void SetupModel(HWND hwnd)
{
    RECT rect;
    GetClientRect(hwnd, &rect);

    float x = rect.right / 2.0f;
    float y = rect.bottom / 2.0f;
    float r = 100.0f;

    points[0] = x + r * std::cosf(pi * 2.0f / 12.0f * 1.0f);
    points[1] = y + r * std::sinf(pi * 2.0f / 12.0f * 1.0f);
    points[2] = x + r * std::cosf(pi * 2.0f / 12.0f * 5.0f);
    points[3] = y + r * std::sinf(pi * 2.0f / 12.0f * 5.0f);
    points[4] = x + r * std::cosf(pi * 2.0f / 12.0f * 9.0f);
    points[5] = y + r * std::sinf(pi * 2.0f / 12.0f * 9.0f);

    // Vertex Buffer
    GLuint positionLocation = 0;
    GLuint positionBindindex = 0;

    glCreateBuffers(1, &positionBuffer);

    glNamedBufferData(positionBuffer,
        sizeof(points), points, GL_DYNAMIC_DRAW);

    // Vertex Array
    glCreateVertexArrays(1, &vertexArray);

    glEnableVertexArrayAttrib(vertexArray, positionLocation);
    glVertexArrayAttribFormat(vertexArray, positionLocation,
        2, GL_FLOAT, GL_FALSE, 0);

    glVertexArrayAttribBinding(vertexArray, positionLocation,
        positionBindindex);
    glVertexArrayVertexBuffer(vertexArray, positionBindindex,
        positionBuffer, static_cast<GLintptr>(0), sizeof(GLfloat) * 2);
}

void UpdateVertexBuffer()
{
    glNamedBufferSubData(positionBuffer,
        drag_index * 2 * sizeof(float), sizeof(float) * 2, points + drag_index * 2);
}

void Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vertexArray);
    glUniform3f(color, 0.0f, 0.6f, 0.6f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glUniform3f(color, 0.0f, 0.4f, 0.4f);
    glDrawArrays(GL_POINTS, 0, 3);

    SwapBuffers(hdc);
    EndPaint(hwnd, &ps);
}
