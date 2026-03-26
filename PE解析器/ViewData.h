#pragma once
#include <vector>
#include <string>
#include <Windows.h>

struct BaseData
{
	std::string field{};
	std::string value{};
	std::string descriptor{};
};

using DosHeaderData = BaseData;
using NtSignatureData = BaseData;
