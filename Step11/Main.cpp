#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <vector>
#include "glcorearb.h"
#include "wglext.h"
#include "OpenGL45.h"
#include "resource.h"

struct Line
{
    float x0;
    float y0;
    float x1;
    float y1;

    Line() : x0(0.0f), y0(0.0f), x1(0.0f), y1(0.0f) {}
    Line(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1)
        : x0(x0), y0(y0), x1(x1), y1(y1) { }
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HGLRC CreateOpenGLContext(HWND hwnd);
void DeleteOpenGLContext(HGLRC hglrc);
void RegisterErrorCallback();
GLuint LoadShader();
void SetupModel();
void UpdateVertexBuffer();
void Paint(HWND hwnd);

static FILE *logFile;

static GLuint positionBuffer;
static GLuint vertexArray;

static std::vector<Line> lines;
static float drag_start_x;
static float drag_start_y;
static bool is_mouse_down = false;

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
        class_name, TEXT("Step 11"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 680, 480,
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
        SetupModel();
        glClearColor(0.6f, 0.8f, 0.8f, 1.0f);
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
        is_mouse_down = true;
        drag_start_x = LOWORD(lParam);
        drag_start_y = HIWORD(lParam);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONUP:
        is_mouse_down = false;
        lines.push_back(Line(
            drag_start_x, drag_start_y,
            LOWORD(lParam), HIWORD(lParam)));
        UpdateVertexBuffer();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_MOUSEMOVE:
        if (is_mouse_down)
        {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);

            GLfloat vertices[] = {
                drag_start_x, drag_start_y,
                pt.x, pt.y
            };

            glNamedBufferSubData(positionBuffer,
                0, 4 * sizeof(GLfloat), vertices);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_RBUTTONDOWN:
        lines.clear();
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

void SetupModel()
{
    // Vertex Buffer
    GLuint positionLocation = 0;
    GLuint positionBindindex = 0;

    glCreateBuffers(1, &positionBuffer);

    GLfloat positionData[] = {
        0.0f, 0.0f, 0.0f, 0.0f
    };

    glNamedBufferData(positionBuffer,
        sizeof(positionData), positionData, GL_DYNAMIC_DRAW);

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
    std::size_t size = (lines.size() + 1) * 4 * sizeof(GLfloat);
    GLfloat *vertices = new GLfloat[size];

    for (int i = 0; i < lines.size(); ++i)
    {
        vertices[(i + 1) * 4 + 0] = lines[i].x0;
        vertices[(i + 1) * 4 + 1] = lines[i].y0;
        vertices[(i + 1) * 4 + 2] = lines[i].x1;
        vertices[(i + 1) * 4 + 3] = lines[i].y1;
    }

    glNamedBufferData(positionBuffer, size, vertices, GL_DYNAMIC_DRAW);

    delete[] vertices;
}

void Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vertexArray);
    if (is_mouse_down)
    {
        glDrawArrays(GL_LINES, 0, 
            static_cast<GLsizei>(lines.size()) * 2 + 2);
    }
    else
    {
        glDrawArrays(GL_LINES, 2, 
            static_cast<GLsizei>(lines.size()) * 2);
    }
    
    SwapBuffers(hdc);
    EndPaint(hwnd, &ps);
}
