#pragma once
#include <fstream>

class MetaFile
{
public:
	std::string name = "Unnamed";
	uint16_t size = 0;
	bool isDir = false;

	void operator()(uint16_t size, bool isDir, const char name[]);
	void operator()(bool isDir, const char name[]);
	void operator()(uint16_t id);
	
};