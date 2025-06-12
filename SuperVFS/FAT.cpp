#include "FAT.h"
uint32_t FAT_END = IndexFile::VF_SIZE / IndexFile::CLUSTER_SIZE;
uint32_t END = UINT32_MAX;
uint32_t ERR = UINT32_MAX - 1;
uint32_t FREE = 0;

uint32_t FAT::getClusterFatLoc(uint32_t clusterId)
{
	return IndexFile::FAT_LOC + ((clusterId - 1) * sizeof(uint32_t));
}

uint32_t FAT::getClusterAbsLoc(uint32_t clusterId)
{
	return IndexFile::DATA_LOC + ((clusterId - 1) * IndexFile::CLUSTER_SIZE);
}

uint32_t FAT::init(std::fstream& file) {

	file.seekp(IndexFile::FAT_LOC, std::ios::beg);

	for (uint16_t i = 0; i < (IndexFile::VF_SIZE / IndexFile::CLUSTER_SIZE); i++) {
		file.write(reinterpret_cast<char*>(&FREE), sizeof(uint32_t));
	}
	std::flush(file);
	return IndexFile::VF_SIZE / IndexFile::CLUSTER_SIZE;
}

uint32_t FAT::addCluster(std::fstream& file) {
	uint32_t clusterId = IndexFile::START_CLUSTER;
	uint32_t clusterInners = ERR;

	while (clusterInners != FREE) {
		readCluster(file, clusterId, clusterInners);
		if (clusterInners == ERR) {
			std::cerr << "Error in reading FAT" << "\n";
			return ERR;
		}

		if (clusterInners == FREE) {
			writeCluster(file, clusterId, END);
			IndexFile::START_CLUSTER = clusterId < IndexFile::START_CLUSTER ? clusterId : IndexFile::START_CLUSTER;
			IndexFile::FAT_FREE--;
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
	while (nextCluster != FREE) {
		readCluster(file, clusterId, clusterInners);
		if (nextCluster == 0) {
			writeCluster(file, clusterId, END);
			std::flush(file);
			IndexFile::FAT_FREE--;
			return clusterId;
		}
		clusterId++;
	}
	return ERR;
}


uint32_t FAT::addCluster(std::fstream& file, uint32_t clusterId, uint32_t nextCluster) {
	if (clusterId < IndexFile::START_CLUSTER || nextCluster < IndexFile::START_CLUSTER)
		return ERR;

	uint32_t clusterInners = UINT32_MAX;
	readCluster(file, clusterId, clusterInners);
	if (clusterInners == 0) {
		writeCluster(file, clusterId, nextCluster);
		std::flush(file);
		IndexFile::FAT_FREE--;
		return clusterId;
	}

	return ERR;
}


uint32_t FAT::remCluster(std::fstream& file, uint32_t clusterId) {
	uint32_t cluster = clusterId;
	uint32_t nextCluster = 0;
	do
	{
		readCluster(file, cluster, nextCluster);
		writeCluster(file, cluster, FREE);
		IndexFile::START_CLUSTER = IndexFile::START_CLUSTER > cluster ? cluster : IndexFile::START_CLUSTER;
		cluster = nextCluster;
		IndexFile::FAT_FREE++;

	} while (nextCluster != END && nextCluster != FREE);
	file.seekp(3 * sizeof(uint32_t) + 2 * sizeof(uint16_t), std::ios::beg);
	file.write(reinterpret_cast<char*>(&IndexFile::START_CLUSTER), sizeof(uint32_t));
	file.flush();
	return cluster;
}

void FAT::readCluster(std::fstream& file, uint32_t clusterId, uint32_t &next) {
	file.seekg(getClusterFatLoc(clusterId), std::ios::beg);
	file.read(reinterpret_cast<char*>(&next), sizeof(uint32_t));
	file.sync();
}

void FAT::writeCluster(std::fstream& file, uint32_t clusterId, uint32_t nextId) {
	file.seekp(getClusterFatLoc(clusterId), std::ios::beg);
	file.write(reinterpret_cast<char*>(&nextId), sizeof(uint32_t));
	file.flush();
}

void FAT::swap(std::fstream& file, uint32_t left, uint32_t right) {
	uint32_t leftNext;
	uint32_t rightNext;
	std::cout << left << " " << right << std::endl;
	if (left != END && left != FREE && right != END && right != FREE) {
		file.seekg(getClusterFatLoc(left), std::ios::beg);
		file.read(reinterpret_cast<char*>(&leftNext), sizeof(uint32_t));
		std::cout << "left: " << left << " ";
		file.seekg(getClusterFatLoc(right), std::ios::beg);
		file.read(reinterpret_cast<char*>(&rightNext), sizeof(uint32_t));
		std::cout << "right: " << right << std::endl;
		file.sync();

		file.seekp(getClusterFatLoc(right), std::ios::beg);
		file.write(reinterpret_cast<char*>(&leftNext), sizeof(uint32_t));
		file.seekp(getClusterFatLoc(left), std::ios::beg);
		file.write(reinterpret_cast<char*>(&rightNext), sizeof(uint32_t));
		file.flush();

		file.seekg(getClusterAbsLoc(left), std::ios::beg);
		char* leftData = new char[IndexFile::CLUSTER_SIZE + 1];
		file.read(leftData, IndexFile::CLUSTER_SIZE);
		file.seekg(getClusterAbsLoc(right), std::ios::beg);
		char* rightData = new char[IndexFile::CLUSTER_SIZE + 1];
		file.read(rightData, IndexFile::CLUSTER_SIZE);
		file.sync();

		file.seekp(getClusterAbsLoc(right), std::ios::beg);
		file.write(leftData, IndexFile::CLUSTER_SIZE);
		file.seekp(getClusterAbsLoc(left), std::ios::beg);
		file.write(rightData, IndexFile::CLUSTER_SIZE);
		file.flush();
		delete[] leftData;
		delete[] rightData;
	}
	
}