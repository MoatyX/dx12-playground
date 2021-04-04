#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <Windows.h>

//link dx12 libraries
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#include <wrl.h>
#include <string>
#include <shellapi.h>

#include "d3dx12.h"
#include "macros.h"
#include "math_help.h"
//#include "upload_buffer.h"


