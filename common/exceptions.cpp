#include "exceptions.h"

dx12_exception::dx12_exception(const HRESULT hr, const std::wstring& function_name, const std::wstring& filename,
	const int line_number)
{
    error_code_ = hr;
    this->function_name_ = function_name;
    this->filename_ = filename;
    this->line_number_ = line_number;
}

std::wstring dx12_exception::to_string() const
{
	const _com_error error(error_code_);
	const std::wstring msg = error.ErrorMessage();

    return function_name_ + L" failed in " + filename_ + L"; line " + std::to_wstring(line_number_) + L"; error: " + msg;
}
