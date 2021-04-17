#pragma once
#include <cstdint>

namespace VkUtils
{
	struct QueueFamilyIndices
	{
		uint32_t graphicsFamily = -1;

		bool IsValid(){ return graphicsFamily >= 0; }
			
	};
}

