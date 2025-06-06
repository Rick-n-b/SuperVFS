#include "FAT.h"

uint32_t END = UINT32_MAX;
uint32_t ERR = UINT32_MAX - 1;
uint32_t FREE = 0;

uint32_t FAT::getClusterFatLoc(uint32_t clusterId)
{
	return IndexFile::FAT_LOC + clusterId * uint32_t(sizeof(uint32_t));
}

uint32_t FAT::getClusterAbsLoc(uint32_t clusterId)
{
	return IndexFile::DATA_LOC + clusterId * uint32_t(IndexFile::CLUSTER_SIZE);
}

uint32_t FAT::init(){
	std::fstream file(IndexFile::filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	if (!file.is_open()) file.open(IndexFile::filename, std::ios::out | std::ios::binary);

	file.seekp(IndexFile::FAT_LOC, std::ios::beg);
	
	for (uint16_t i = 0; i < IndexFile::VF_SIZE / IndexFile::CLUSTER_SIZE; i++) {
		file.write(reinterpret_cast<char*>(&FREE), sizeof(uint32_t));
		std::flush(file);
	}
	file.close();
	return IndexFile::VF_SIZE / IndexFile::CLUSTER_SIZE;
}

uint32_t FAT::addCluster(std::fstream& file) {
	uint32_t clusterId = IndexFile::START_CLUSTER;
	uint32_t nextCluster = END;
	while (nextCluster != FREE) {
		nextCluster = readCluster(file, clusterId);
		if (nextCluster == FREE) {
			writeCluster(file, clusterId, END);
			IndexFile::START_CLUSTER = clusterId;
			return clusterId;
		}
		clusterId++;
	}
	return ERR;
	
}

uint32_t FAT::addCluster(std::fstream& file, uint32_t nextCluster) {
	if (nextCluster < IndexFile::START_CLUSTER)
		return ERR;

	uint32_t clusterId = IndexFile::START_CLUSTER;
	uint32_t clusterInners = END;
	while (nextCluster != 0) {
		clusterInners = readCluster(file, clusterId);
		if (nextCluster == 0) {
			writeCluster(file, clusterId, END);
			std::flush(file);
			return clusterId;
		}
		clusterId++;
	}
	return ERR;
}


uint32_t FAT::addCluster(std::fstream& file, uint32_t clusterId, uint32_t nextCluster) {
	if (clusterId < IndexFile::START_CLUSTER || nextCluster < IndexFile::START_CLUSTER)
		return ERR;

	uint32_t clusterInners = readCluster(file, clusterId);
	if (clusterInners == 0) {
		writeCluster(file, clusterId, nextCluster);
		std::flush(file);
		return clusterId;
	}
	
	return ERR;
}


uint32_t FAT::remCluster(std::fstream& file, uint32_t clusterId) {
	uint32_t cluster = clusterId;
	uint32_t nextCluster = 0;
	do
	{
		nextCluster = readCluster(file, cluster);
		writeCluster(file, cluster, FREE);
		std::flush(file);
		IndexFile::START_CLUSTER = IndexFile::START_CLUSTER > cluster ? cluster : IndexFile::START_CLUSTER;
		cluster = nextCluster;
		
	} while (nextCluster != END && nextCluster != FREE);
	file.seekp(3 * sizeof(uint32_t) + 2 * sizeof(uint32_t), std::ios::beg);
	file.write(reinterpret_cast<char*>(&IndexFile::START_CLUSTER), sizeof(uint32_t));
	file.flush();
	return cluster;
}

uint32_t FAT::readCluster(std::fstream& file, uint32_t clusterId) {
	uint32_t clusterInners;
	file.seekg(getClusterFatLoc(clusterId), std::ios::beg);
	file.read(reinterpret_cast<char*>(&clusterInners), sizeof(uint32_t));
	return clusterInners;
}

void FAT::writeCluster(std::fstream& file, uint32_t clusterId, uint32_t nextId) {
	file.seekp(getClusterFatLoc(clusterId), std::ios::beg);
	file.write(reinterpret_cast<char*>(&nextId), sizeof(uint32_t));
	file.flush();
}