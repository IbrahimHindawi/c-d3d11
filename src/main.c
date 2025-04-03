#include <stdio.h>
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "core.h"

#include <intrin.h>

// #define _DEBUG 1
// replace this with your favorite Assert() implementation
#define Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define AssertHR(hr) Assert(SUCCEEDED(hr))

#pragma comment (lib, "gdi32")
#pragma comment (lib, "user32")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

#define STR2(x) #x
#define STR(x) STR2(x)

// Global variables
global HINSTANCE hInst;
global HWND hWnd;
global RECT rc = { 0, 0, 640, 480 };
global UINT nWidth;
global UINT nHeight;

// DirectX
typedef struct GfxState GfxState;
struct GfxState {
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11RenderTargetView*	render_target_view;
    IDXGISwapChain* swap_chain;
    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3D11InputLayout *input_layout;
    ID3D11Buffer *vertex_buffer;
};
global GfxState gfx = {};

typedef struct vec3 vec3;
struct vec3 { f32 x; f32 y; f32 z; };

typedef struct vec2 vec2;
struct vec2 { f32 x; f32 y; };

typedef struct Vertex Vertex;
struct Vertex {
    vec3 pos;
    // vec2 tex;
};

ATOM MyRegisterClass(HINSTANCE);
BOOL CreateMainWnd(int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Processes messages for the main window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;
    switch (message) {
        case WM_PAINT: {
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        } break;
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        default: {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    return 0;
}

void DX11_initialize() {
    // DirectX init
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {
        .BufferCount = 1,
        .BufferDesc.Width = rc.right,
        .BufferDesc.Height = rc.bottom,
        .BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .BufferDesc.RefreshRate.Numerator = 60,
        .BufferDesc.RefreshRate.Denominator = 1,
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .OutputWindow = (HWND)hWnd,
        .SampleDesc.Count = 1,
        .SampleDesc.Quality = 0,
        .Windowed = true,
    };

    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE, D3D_DRIVER_TYPE_SOFTWARE
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    // Flags
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
        flags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION,
        &swapChainDesc, &gfx.swap_chain, &gfx.device, &featureLevel, &gfx.context);

    if (FAILED(hr))	{
        MessageBox(hWnd, TEXT("A DX11 Video Card is Required"), TEXT("ERROR"), MB_OK);
        return;
    }

    ID3D11Texture2D *pBackBuffer;
    hr = IDXGISwapChain1_GetBuffer(gfx.swap_chain, 0, &IID_ID3D11Texture2D, (void**)&pBackBuffer);
    if (FAILED(hr)) {
        MessageBox(hWnd, TEXT("Unable to get back buffer"), TEXT("ERROR"), MB_OK);
        return;
    }

    hr = ID3D11Device_CreateRenderTargetView(gfx.device, (ID3D11Resource*)pBackBuffer, NULL, &gfx.render_target_view);

    if (pBackBuffer != NULL) {
        ID3D11Texture2D_Release(pBackBuffer);
    }

    if (FAILED(hr)) {
        MessageBox(hWnd, TEXT("Unable to create render target view"), TEXT("ERROR"), MB_OK);
        return;
    }

    ID3D11DeviceContext_OMSetRenderTargets(gfx.context, 1, &gfx.render_target_view, NULL);

    D3D11_VIEWPORT viewPort;
    viewPort.Width = (float)nWidth;
    viewPort.Height = (float)nHeight;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;
    ID3D11DeviceContext_RSSetViewports(gfx.context, 1, &viewPort);

}

bool CompileShader(LPCWSTR szFilePath, LPCSTR szFunc, LPCSTR szShaderModel, ID3DBlob** buffer) {
    DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif
    HRESULT hr;
    ID3DBlob* errBuffer = 0;
    hr = D3DCompileFromFile(szFilePath, 0, 0, szFunc, szShaderModel, flags, 0, buffer, &errBuffer);
    if (FAILED(hr)) {
        if (errBuffer != NULL) {
            // printf("%s\n", (char*)ID3D10Blob_GetBufferPointer(errBuffer));
            OutputDebugStringA((char*)ID3D10Blob_GetBufferPointer(errBuffer));
            ID3D10Blob_Release(errBuffer);
        }
        return false;
    }
    if (errBuffer != NULL) {
        ID3D10Blob_Release(errBuffer);
    }
    return true;
}

bool Load() {
    HRESULT hr = S_FALSE;
    bool result = false;
    ID3DBlob *vertex_buffer_blob = NULL;
    result = CompileShader(L"shaders/shader_green_color.fx", TEXT("VS_Main"), TEXT("vs_4_0"), &vertex_buffer_blob);
    if (!result) {
        MessageBox(hWnd, TEXT("Unable to load vertex shader"), TEXT("Error"), MB_OK);
        return false;
    }
    hr = ID3D11Device_CreateVertexShader(gfx.device, ID3D10Blob_GetBufferPointer(vertex_buffer_blob), ID3D10Blob_GetBufferSize(vertex_buffer_blob), 0, &gfx.vertex_shader);
    if (FAILED(hr)) {
        if (gfx.vertex_buffer) {
            ID3D10Blob_Release(vertex_buffer_blob);
            vertex_buffer_blob = NULL;
        }
        MessageBox(hWnd, TEXT("Unable to create vertex shader"), TEXT("Error"), MB_OK);
        return false;
    }
    D3D11_INPUT_ELEMENT_DESC shader_input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        // { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    u32 num_layout_elements = sizeofarray(shader_input_layout);
    hr = ID3D11Device_CreateInputLayout(gfx.device, shader_input_layout, num_layout_elements, ID3D10Blob_GetBufferPointer(vertex_buffer_blob), ID3D10Blob_GetBufferSize(vertex_buffer_blob), &gfx.input_layout);
    if (FAILED(hr)) {
        MessageBox(hWnd, TEXT("Unable to create input layout"), TEXT("Error"), MB_OK);
        return false;
    }
    ID3D10Blob_Release(vertex_buffer_blob);
    vertex_buffer_blob = NULL;
    ID3DBlob *pixel_buffer_blob = NULL;
    result = CompileShader(L"shaders/shader_green_color.fx", TEXT("PS_Main"), TEXT("ps_4_0"), &pixel_buffer_blob);
    if (!result) {
        MessageBox(hWnd, TEXT("Unable to load pixel shader"), TEXT("Error"), MB_OK);
        return false;
    }
    hr = ID3D11Device_CreatePixelShader(gfx.device, ID3D10Blob_GetBufferPointer(pixel_buffer_blob), ID3D10Blob_GetBufferSize(pixel_buffer_blob), 0, &gfx.pixel_shader);
    if (FAILED(hr)) {
        MessageBox(hWnd, TEXT("Unable to create pixel shader"), TEXT("Error"), MB_OK);
        return false;
    }
    ID3D10Blob_Release(pixel_buffer_blob);
    pixel_buffer_blob = NULL;
    Vertex vertices[] = {
        { 0.0f,  0.5f, 0.5f},
        { 0.5f, -0.5f, 0.5f},
        {-0.5f, -0.5f, 0.5f},
    };
    D3D11_BUFFER_DESC vertex_buffer_desc = {};
    vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertex_buffer_desc.ByteWidth = sizeof(Vertex) * 3;
    D3D11_SUBRESOURCE_DATA resource_data = {};
    resource_data.pSysMem = vertices;
    hr = ID3D11Device_CreateBuffer(gfx.device, &vertex_buffer_desc, &resource_data, &gfx.vertex_buffer);
    if (FAILED(hr)) {
        MessageBox(hWnd, TEXT("Unable to create vertex buffer"), TEXT("Error"), MB_OK);
        return false;
    }
    return true;
}

void Unload() {
    if (gfx.vertex_shader) {
        ID3D11VertexShader_Release(gfx.vertex_shader);
        gfx.vertex_shader = NULL;
    }
    if (gfx.pixel_shader) {
        ID3D11PixelShader_Release(gfx.pixel_shader);
        gfx.pixel_shader = NULL;
    }
    if (gfx.input_layout) {
        ID3D11InputLayout_Release(gfx.input_layout);
        gfx.input_layout = NULL;
    }
    if (gfx.vertex_buffer) {
        ID3D11Buffer_Release(gfx.vertex_buffer);
        gfx.vertex_buffer = NULL;
    }
}

void Render() {
    if (!gfx.context) {
        return;
    }
    float a[] = {.1f, .1f, .1f, 1.f};
    ID3D11DeviceContext_ClearRenderTargetView(gfx.context, gfx.render_target_view, a);
    u32 stride = sizeof(Vertex);
    u32 offset = 0;
    ID3D11DeviceContext_IASetInputLayout(gfx.context, gfx.input_layout);
    ID3D11DeviceContext_IASetVertexBuffers(gfx.context, 0, 1, &gfx.vertex_buffer, &stride, &offset);
    ID3D11DeviceContext_IASetPrimitiveTopology(gfx.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(gfx.context, gfx.vertex_shader, 0, 0);
    ID3D11DeviceContext_PSSetShader(gfx.context, gfx.pixel_shader, 0, 0);
    ID3D11DeviceContext_Draw(gfx.context, 3, 0);
    IDXGISwapChain_Present(gfx.swap_chain, 0, 0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE previnstance, LPSTR cmdline, int nCmdShow) {
    UNREFERENCED_PARAMETER(previnstance);
    UNREFERENCED_PARAMETER(cmdline);

    hInst = hInstance;
    hWnd = NULL;

    WNDCLASSEX wcex = {
        .cbSize = sizeof(WNDCLASSEX),
        .style			= CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc	= WndProc,
        .cbClsExtra		= 0,
        .cbWndExtra		= 0,
        .hInstance		= hInstance,
        .hIcon			= NULL,
        .hCursor		= LoadCursor(NULL, IDC_ARROW),
        .hbrBackground	= (HBRUSH)(COLOR_WINDOW+1),
        .lpszMenuName	= NULL,
        .lpszClassName	= TEXT("BlankWndClass"),
        .hIconSm		= NULL,
    };
    RegisterClassEx(&wcex);

    // Calculate main window size
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    GetClientRect(hWnd, &rc);
    nWidth = rc.right - rc.left;
    nHeight = rc.bottom - rc.top;

    // Create the main window
    hWnd = CreateWindow(TEXT("BlankWndClass"), TEXT("Blank Window"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
        rc.right - rc.left, rc.bottom - rc.top, 
        NULL, NULL, hInst, NULL);
    
    // Check window handle
    if (hWnd == NULL) {
        printf("window creation failed");
        return FALSE;
    }

    DX11_initialize();
    if (!Load()) {
        MessageBox(hWnd, TEXT("Unable to load resources"), TEXT("Error"), MB_OK);
        return -1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Render();
    }

    return (int)msg.wParam;
}
