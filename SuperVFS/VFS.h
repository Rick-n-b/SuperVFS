#pragma once
#include "File.h"
#include <vector>

class VFS {
public:
	VFS(const char filename[], uint32_t VF_SIZE, uint16_t CLUSTER_SIZE);



	void createFile(std::string& name);

	std::vector<std::string> splitPath(std::string path);


private:
	IndexFile indexFile;
	FAT FAT;
	
	
};