#pragma once
#include <cstdint>

namespace VkUtils
{
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;

		bool IsValid(){ return graphicsFamily >= 0; }
			
	};
}

