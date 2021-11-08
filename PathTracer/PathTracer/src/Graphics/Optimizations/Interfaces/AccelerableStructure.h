#pragma once

#include <Oblivion.h>


template <class primitiveType>
class AccelerableStructure {
public:
	virtual std::vector<std::shared_ptr<primitiveType>>& GetPrimitives() = 0;
	virtual void ReorderPrimitives(std::vector<std::shared_ptr<primitiveType>>&) = 0;

};
