#pragma once
#include "IndexFile.h"
#include "FAT.h"
#include "MetaFile.h"

#include <vector>

class File {
public:

	void create(std::fstream&, bool, std::string);
	void del(std::fstream&);
	void open(std::fstream&, uint32_t);
	void close();
	void rewrite(const char*);
	void rewrite(const char*, size_t);
	void add(const char*);
	void add(const char*, size_t);
	void save(std::fstream&);
	void rename(std::fstream&, std::string);

	std::vector<uint32_t> getFiles(std::fstream&);
	bool addFile(std::fstream&, uint32_t fileClusterId);
	bool remFile(std::fstream&, uint32_t fileClusterId);
	bool isInDir(std::fstream&, uint32_t fileClusterId);

	friend std::ostream& operator<<(std::ostream&, File&);

	uint32_t startClusterId = UINT32_MAX;//узнаём из FAT таблицы
	MetaFile metaInfo;//загружаем из кластера
	bool isOpen = false;
	char* data = new char[5]{0};//в зависимости от метаинформации читаем данные

private:
	bool dirCheck(std::fstream& file);
	size_t dataLen();
	size_t len(const char*);
};