#include "IndexFile.h"

char IndexFile::filename[64] = "VFS.bin";
std::fstream FS(IndexFile::filename, std::ios::in | std::ios::out | std::ios::binary);
uint16_t IndexFile::TYPE = 0xFF00;
uint32_t IndexFile::VF_SIZE = 1024*1024;
uint16_t IndexFile::CLUSTER_SIZE = 128;
uint32_t IndexFile::FAT_LOC = 24;
uint32_t IndexFile::FAT_FREE = (VF_SIZE / CLUSTER_SIZE) - 1;
uint32_t IndexFile::START_CLUSTER = 2;
uint32_t IndexFile::DATA_LOC = IndexFile::FAT_LOC + (IndexFile::VF_SIZE / IndexFile::CLUSTER_SIZE) * sizeof(uint32_t);

IndexFile::IndexFile(const char filename[]) {
	strcpy_s(this->filename, 64, filename);
}

IndexFile::IndexFile() {
	if (this->filename == "")
		std::cerr << "File not loaded\n";
}

void IndexFile::init(std::fstream& file, uint32_t VF_SIZE, uint16_t CLUSTER_SIZE) {
	
	this->VF_SIZE = VF_SIZE;
	this->CLUSTER_SIZE = CLUSTER_SIZE;
	this->FAT_FREE = (VF_SIZE / CLUSTER_SIZE) - 1;
	this->DATA_LOC = IndexFile::FAT_LOC + ((VF_SIZE / CLUSTER_SIZE) * sizeof(uint32_t));
	file.seekg(VF_SIZE, std::ios::beg);
	file.clear();
	file.write("", 1);
	file.seekg(std::ios::beg);
	file.write(reinterpret_cast<char*>(&this->TYPE), sizeof(uint16_t));
	file.write(reinterpret_cast<char*>(&this->VF_SIZE), sizeof(uint32_t));
	file.write(reinterpret_cast<char*>(&this->CLUSTER_SIZE), sizeof(uint16_t));
	file.write(reinterpret_cast<char*>(&this->FAT_LOC), sizeof(uint32_t));
	file.write(reinterpret_cast<char*>(&this->FAT_FREE), sizeof(uint32_t));
	file.write(reinterpret_cast<char*>(&this->START_CLUSTER), sizeof(uint32_t));
	file.write(reinterpret_cast<char*>(&this->DATA_LOC), sizeof(uint32_t));
	std::cout << "Initialized\n";
}

void IndexFile::load(std::fstream& file) {
	file.seekp(std::ios::beg);
	file.read(reinterpret_cast<char*>(&this->TYPE), sizeof(uint16_t));
	file.read(reinterpret_cast<char*>(&this->VF_SIZE), sizeof(uint32_t));
	file.read(reinterpret_cast<char*>(&this->CLUSTER_SIZE), sizeof(uint16_t));
	file.read(reinterpret_cast<char*>(&this->FAT_LOC), sizeof(uint32_t));
	file.read(reinterpret_cast<char*>(&this->FAT_FREE), sizeof(uint32_t));
	file.read(reinterpret_cast<char*>(&this->START_CLUSTER), sizeof(uint32_t));
	file.read(reinterpret_cast<char*>(&this->DATA_LOC), sizeof(uint32_t));
	std::cout << "Loaded\n";
	
}
