#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "DirectXTK-main/Inc/WICTextureLoader.h"

ID3D11VertexShader* vertexShader = nullptr;
ID3D11PixelShader* pixelShader = nullptr;
ID3D11InputLayout* inputLayout = nullptr;

IDXGISwapChain* swapChain = nullptr;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;
ID3D11Buffer* vertexBuffer = nullptr;

ID3D11Buffer* constantBuffer = nullptr;
float rotationAngle = 0.0f;

void InitD3D(HWND hwnd);
void RenderFrame();
void CleanD3D();

using namespace DirectX;

struct Vertex {
      float x, y, z;   
      float u, v;      
      float nx, ny, nz; 
      
      Vertex() : x(0), y(0), z(0), u(0), v(0), nx(0), ny(0), nz(0) {}
      
      Vertex(float x, float y, float z, float u, float v, float nx, float ny, float nz) 
            : x(x), y(y), z(z), u(u), v(v), nx(nx), ny(ny), nz(nz) {}                
};

struct ConstantBuffer {
      XMMATRIX WorldViewProjection;

};

Vertex vertices[] = {
      { -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },  
      {  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f }, 
      {  0.0f, -0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f }  
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
      
      switch (uMsg) {
            
            case WM_DESTROY:
                  PostQuitMessage(0);
                  return 0;          
      }
      
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
   
      constexpr char CLASS_NAME[] = "D3DWindowClass";

      WNDCLASS wc = {};
      wc.lpfnWndProc = WindowProc;
      wc.hInstance = hInstance;
      wc.lpszClassName = CLASS_NAME;
    
      RegisterClass(&wc);
    
      HWND hwnd = CreateWindowEx(
          0,
          CLASS_NAME,
          "Direct3D 11",
          WS_OVERLAPPEDWINDOW,
          CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
          nullptr, nullptr, hInstance, nullptr
      );
    
      ShowWindow(hwnd, nCmdShow);
      InitD3D(hwnd);

      MSG msg = {};
      while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                  TranslateMessage(&msg);
                  DispatchMessage(&msg);
            } else {
                  
                  RenderFrame();
            }
      }

      CleanD3D();
      return 0;
}


void InitD3D(HWND hwnd) {
      DXGI_SWAP_CHAIN_DESC scd = {};
      scd.BufferCount = 1;
      scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
      scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      scd.OutputWindow = hwnd;
      scd.SampleDesc.Count = 1;
      scd.Windowed = TRUE;
      scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

      D3D11CreateDeviceAndSwapChain(
          nullptr,
          D3D_DRIVER_TYPE_HARDWARE,
          nullptr,
          0,
          nullptr, 0,
          D3D11_SDK_VERSION,
          &scd,
          &swapChain,
          &device,
          nullptr,
          &context
      );
      
      ID3D11Texture2D* backBuffer = nullptr;
      swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
      device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
      backBuffer->Release();
       
      context->OMSetRenderTargets(1, &renderTargetView, nullptr);
        
      D3D11_VIEWPORT viewport = {};
      viewport.TopLeftX = 0;
      viewport.TopLeftY = 0;
      viewport.Width = 800;
      viewport.Height = 600;
      context->RSSetViewports(1, &viewport);
    
      ID3DBlob* vsBlob = nullptr;
      ID3DBlob* psBlob = nullptr;
      ID3DBlob* errorBlob = nullptr;
        
      HRESULT hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
      if (FAILED(hr))
      {
            if (errorBlob)
            {
                  OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                  errorBlob->Release();
            }
            MessageBox(hwnd, "Vertex shader compilation failed!", "Error", MB_OK);
            return;
      }

      hr = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
      if (FAILED(hr))
      {
            if (errorBlob)
            {
                  OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                  errorBlob->Release();
            }
            MessageBox(hwnd, "Pixel shader compilation failed!", "Error", MB_OK);
            return;
      }

      device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
      device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

      context->VSSetShader(vertexShader, nullptr, 0);
      context->PSSetShader(pixelShader, nullptr, 0);

      D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
      {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
              D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20,
              D3D11_INPUT_PER_VERTEX_DATA, 0 }
      };

      device->CreateInputLayout(layoutDesc, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
      context->IASetInputLayout(inputLayout);   

      vsBlob->Release();
      psBlob->Release();

      D3D11_BUFFER_DESC bd = {};
      bd.Usage = D3D11_USAGE_DEFAULT;
      bd.ByteWidth = sizeof(vertices);
      bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      bd.CPUAccessFlags = 0;

      D3D11_SUBRESOURCE_DATA initData = {};
      initData.pSysMem = vertices;

      device->CreateBuffer(&bd, &initData, &vertexBuffer);

      UINT stride = sizeof(Vertex);
      UINT offset = 0;
      context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
      
      context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      ID3D11ShaderResourceView* textureSRV = nullptr;
      CreateWICTextureFromFile(device, context, L"texture.png", nullptr, &textureSRV);
      context->PSSetShaderResources(0, 1, &textureSRV);

    
      D3D11_BUFFER_DESC cbd = {};
      cbd.Usage = D3D11_USAGE_DYNAMIC;
      cbd.ByteWidth = sizeof(ConstantBuffer);
      cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

      device->CreateBuffer(&cbd, nullptr, &constantBuffer);

      context->VSSetConstantBuffers(0, 1, &constantBuffer);

      ID3D11SamplerState* samplerState = nullptr;

      D3D11_SAMPLER_DESC sampDesc = {};
      sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
      sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
      sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
      sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
      sampDesc.MinLOD = 0;
      sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

      device->CreateSamplerState(&sampDesc, &samplerState);
      context->PSSetSamplers(0, 1, &samplerState);

}

void CleanD3D()
{
      swapChain->Release();
      renderTargetView->Release();
      context->Release();
      device->Release();
      vertexBuffer->Release();
      constantBuffer->Release();

}

void RenderFrame()
{
    float clearColor[4] = { 0.0f, 0.0f, 0.2f, 1.0f };
    context->ClearRenderTargetView(renderTargetView, clearColor);

    rotationAngle += 0.01f;

    // Move triangle back 2 units in Z, then rotate
    XMMATRIX world = XMMatrixRotationZ(rotationAngle) * XMMatrixTranslation(0.0f, 0.0f, 2.0f);
      
    XMMATRIX view = XMMatrixIdentity(); 
      
    XMMATRIX projection = XMMatrixPerspectiveFovLH(
        XM_PIDIV4,
        800.0f / 600.0f,
        0.1f,
        100.0f
    );

    XMMATRIX WVP = world * view * projection;
    WVP = XMMatrixTranspose(WVP);

    // Map and update constant buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    ConstantBuffer* pCB = (ConstantBuffer*)mappedResource.pData;
    pCB->WorldViewProjection = WVP;
    context->Unmap(constantBuffer, 0);

    // draw
    context->Draw(3, 0);
    swapChain->Present(1, 0);
}

