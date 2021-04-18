#pragma once
#include <cstdint>

namespace VkUtils
{
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentationFamily = -1;

		bool IsValid(){ return graphicsFamily >= 0 && presentationFamily >= 0; }
			
	};
}

