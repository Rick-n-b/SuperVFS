#pragma once
#include "IndexFile.h"

class FAT{
public:
	uint32_t init();
	
	static uint32_t addCluster(std::fstream& file);
	static uint32_t addCluster(std::fstream& file, uint32_t nextCluster);
	static uint32_t addCluster(std::fstream& file, uint32_t clusterId, uint32_t nextCluster);

	static uint32_t remCluster(std::fstream& file, uint32_t clusterId);

	static uint32_t getClusterFatLoc(uint32_t clusterId);
	static uint32_t getClusterAbsLoc(uint32_t clusterId);

private:
	static uint32_t readCluster(std::fstream& file, uint32_t clusterId);
	static void writeCluster(std::fstream& file, uint32_t clusterId, uint32_t nextId);
};

