
#pragma once
#include <deque>
#include <functional>

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void Push(std::function<void()>&& func)
	{
		deletors.push_back(func);
	}

	void Cleanup()
	{
		for (auto it = deletors.rbegin(); it != deletors.rend(); ++it)
		{
			(*it)();
		}
		deletors.clear();
	}
};
