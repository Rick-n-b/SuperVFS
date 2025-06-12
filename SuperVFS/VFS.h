#pragma once
#include "File.h"
#include <vector>

class VFS {
public:
	VFS(const char filename[], uint32_t VF_SIZE, uint16_t CLUSTER_SIZE);


	std::vector<uint32_t> seek(std::string path, std::string name, std::fstream&);
	uint32_t seek(std::string path, std::fstream& file);
	uint32_t seekParent(std::string path, std::fstream& file);
	void createFile(std::string path, std::fstream&);
	void rename(std::string path, std::string newName, std::fstream&);
	void deleteFile(std::string path, std::fstream&);
	void write(std::string path, const char* data, std::fstream&);

	void compress_file(const std::vector<uint64_t>& B, const std::vector<uint8_t>& shift, std::string path, std::fstream& fs);
	void defrag(std::fstream&);

	void defragment(std::fstream&);
	void zip(std::fstream&);

	std::vector<std::string> splitPath(std::string path);


private:
	IndexFile indexFile;
	FAT FAT;
	
	std::vector<std::vector<std::uint32_t>> getFilewsClusters(std::fstream& fs);
};