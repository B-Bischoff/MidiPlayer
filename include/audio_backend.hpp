#pragma once

#include "inc.hpp"

template<typename... Args>
std::forward_list<AudioComponent*> combineVectorsToForwardList(const Args&... vectors)
{
	Components result;

	// Lambda function to push elements of a vector into the result forward list
	auto pushVectorElements = [&result](const auto& vec)
	{
		for (const auto& element : vec)
			result.push_front(element);
	};

	// Call the lambda function for each input vector
	(pushVectorElements(vectors), ...);

	return result;
}

template<typename... Args>
void clearVectors(Args&... vectors)
{
	auto clearVector = [](auto& vector)
	{
		vector.clear();
	};

	(clearVector(vectors), ...);
}
