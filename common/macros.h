#pragma once
#include "utility.h"
#include "exceptions.h"

/**
 * \brief evaluates a HRESULT, and throws a dx12_exception if failed
 * \param x HRESULT
 */
#define THROW_IF_FAILED(x)												\
{																		\
	HRESULT hr__ = (x);													\
	std::wstring wfn = ansi_to_wstring(__FILE__);							\
	if (FAILED(hr__)) { throw dx12_exception(hr__, L#x, wfn, __LINE__); }	\
}

#define RELEASE_COM(x) { if(x){ (x)->Release(); (x) = 0; } }

#define __ALIGNED(x) __declspec(align((x)))

