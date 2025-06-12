#include "MetaFile.h"

void MetaFile::operator()(uint16_t size, bool isDir, const char name[]) {
	this->size = size;
	this->isDir = isDir;
	this->name = name;
}
void MetaFile::operator()(bool isDir, const char name[]) {
	this->operator()(0, isDir, name);
}
void MetaFile::operator()(uint16_t id) {
	this->operator()(0, false, "Unnamed");
}
