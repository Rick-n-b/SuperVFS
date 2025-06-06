#include "File.h"
#include "Serializer.h"

void File::create(std::fstream& file, bool isDir, std::string name) {
	this->startClusterId = FAT::addCluster(file);
	file.seekp(FAT::getClusterAbsLoc(startClusterId), std::ios::beg);
	metaInfo(isDir, name.c_str());
	Serializer<MetaFile>::serialize(metaInfo, file);
}

void File::del(std::fstream& file) {
	if (isOpen == true || (startClusterId <= 1))
		return;
	FAT::remCluster(file, startClusterId);
}

void File::open(std::fstream& file, uint32_t startClusterId) {
	if (isOpen == true || (startClusterId == 0))
		return;
	this->startClusterId = startClusterId;
	file.seekg(FAT::getClusterAbsLoc(startClusterId), std::ios::beg);
	size_t metaSize = Serializer<MetaFile>::deserialize(metaInfo, file);
	isOpen = true;
	data = new char[metaInfo.size + 1]{0};

	if (metaInfo.size + metaSize < IndexFile::CLUSTER_SIZE) {
		file.read(data, metaInfo.size);
	}
	else {
		int32_t dataRemain = metaInfo.size;
		uint32_t thisCluster = startClusterId;
		uint32_t nextClusterId = startClusterId;

		file.read(data, metaInfo.size - IndexFile::CLUSTER_SIZE);
		data += IndexFile::CLUSTER_SIZE - metaSize;
		dataRemain -= IndexFile::CLUSTER_SIZE - metaSize;

		do {
			if (thisCluster >= UINT32_MAX - 1)
				break;

			file.seekg(FAT::getClusterFatLoc(thisCluster));
			file.read(reinterpret_cast<char*>(&nextClusterId), sizeof(uint32_t));

			file.seekg(FAT::getClusterAbsLoc(nextClusterId));
			if (dataRemain < IndexFile::CLUSTER_SIZE) {
				file.read(data, dataRemain);
				data += dataRemain;
			}	
			else {
				file.read(data, IndexFile::CLUSTER_SIZE);
				data += IndexFile::CLUSTER_SIZE;
			}
			
			dataRemain -= IndexFile::CLUSTER_SIZE;

			thisCluster = nextClusterId;
		
		} while (dataRemain > 0);

		data -= metaInfo.size;
	}
}

void File::save(std::fstream& file) {
	if (startClusterId >= UINT32_MAX - 1 && !isOpen)
		return;
	//сохранение метаданных на место нечала первого кластера
	if(metaInfo.isDir)
		metaInfo.size = len(data);
	
	VFS::FS.seekp(FAT::getClusterAbsLoc(startClusterId), std::ios::beg);
	size_t metaSize = Serializer<MetaVFS::FS>::serialize(metaInfo, VFS::FS);

	if (metaInfo.size + metaSize < IndexVFS::FS::CLUSTER_SIZE) {
		VFS::FS.write(data, metaInfo.size);
	}
	else {
		int32_t dataRemain = metaInfo.size;
		uint32_t nextClusterId = startClusterId;
		uint32_t thisCluster = startClusterId;

		VFS::FS.write(data, metaInfo.size - IndexVFS::FS::CLUSTER_SIZE);
		data += IndexVFS::FS::CLUSTER_SIZE - metaSize;
		dataRemain -= IndexVFS::FS::CLUSTER_SIZE - metaSize;
		do {
			
			VFS::FS.seekg(FAT::getClusterFatLoc(thisCluster));
			VFS::FS.read(reinterpret_cast<char*>(&nextClusterId), sizeof(uint32_t));

			if (nextClusterId == UINT32_MAX) {
				nextClusterId = FAT::addCluster(VFS::FS);
				VFS::FS.seekp(FAT::getClusterFatLoc(thisCluster));
				VFS::FS.write(reinterpret_cast<char*>(&nextClusterId), sizeof(uint32_t));
			}
			VFS::FS.seekp(FAT::getClusterAbsLoc(nextClusterId));

			if (dataRemain < IndexVFS::FS::CLUSTER_SIZE) {
				VFS::FS.write(data, dataRemain);
				data += dataRemain;
			}
			else {
				VFS::FS.write(data, IndexVFS::FS::CLUSTER_SIZE);
				data += IndexVFS::FS::CLUSTER_SIZE;
			}
			dataRemain -= IndexVFS::FS::CLUSTER_SIZE;

			thisCluster = nextClusterId;

		} while (dataRemain > 0);

		if (nextClusterId != UINT32_MAX)//???
			FAT::remCluster(VFS::FS, nextClusterId);

		data -= metaInfo.size;
	}
	VFS::FS.flush();

}

void File::rename(std::fstream& file, std::string newName) {
	if (isOpen || startClusterId >= UINT32_MAX - 1) return;

	metaInfo.name = newName;
	save(file);
}

void File::rewrite(const char* data) {
	delete[] this->data;
	metaInfo.size = len(data);
	this->data = new char[metaInfo.size + 1]{0};
	
	strcpy_s(this->data, metaInfo.size + 1, data);
}

void File::rewrite(const char* data, size_t len) {
	delete[] this->data;
	metaInfo.size = len;
	this->data = new char[metaInfo.size + 1]{ 0 };
	memcpy(this->data, data, len);
}

void File::add(const char* addData) {
	char* tmp = new char[strlen(this->data) + strlen(addData) + 1]{ 0 };
	strcpy_s(tmp, strlen(data) + 1, data);
	tmp += strlen(data);
	strcpy_s(tmp, strlen(addData) + 1, addData);
	tmp -= strlen(data);
	rewrite(tmp);
	delete[] tmp;
}

void File::add(const char* addData, size_t len) {
	char* tmp = new char[metaInfo.size + len + 1]{ 0 };
	memcpy(tmp, data, metaInfo.size);
	tmp += metaInfo.size;
	memcpy(tmp, addData, len);
	tmp -= metaInfo.size;
	rewrite(tmp, metaInfo.size + len);
	delete[] tmp;
}

void File::close() {
	if (isOpen) {
		isOpen = false;
		delete[] data;
	}
}

//возвращает указатели на первые кластеры файлов
std::vector<uint32_t> File::getFiles(std::fstream& file) {
	std::vector<uint32_t> files;
	if (!dirCheck(file))
		return files;
	uint16_t count = metaInfo.size / sizeof(uint32_t);
	for (int i = 0; i < count; i++) {
		files.push_back(reinterpret_cast<uint32_t>(data));
		data += sizeof(uint32_t);
	}
	data -= metaInfo.size;
	return files;
}

bool File::isInDir(std::fstream& file, uint32_t fileClusterId) {
	for (auto fileIn : getFiles(file))
		if (fileIn == fileClusterId)
			return true;
	return false;
}

bool File::addFile(std::fstream& file, uint32_t fileClusterId) {
	if (!dirCheck(file))
		return false;

	if (!isInDir(file, fileClusterId)) {
		add(reinterpret_cast<char*>(&fileClusterId), sizeof(uint32_t));
		save(file);
		return true;
	}
	return false;
}

bool File::remFile(std::fstream& file, uint32_t fileClusterId) {
	if (!dirCheck(file))
		return false;

	char* tmp = new char[metaInfo.size + 1]{0};
	uint16_t count = metaInfo.size / sizeof(uint32_t);
	uint32_t clusterId = UINT32_MAX;
	bool inDir = false;
	for (int i = 0; i < count; i++)
	{
		tmp = data;
		clusterId = reinterpret_cast<uint32_t>(data);
		if (clusterId != fileClusterId)
			tmp += sizeof(uint32_t);
		else
			inDir = true;
		data += sizeof(uint32_t);

	}
	data -= metaInfo.size;
	if (inDir) {
		tmp -= metaInfo.size - sizeof(uint32_t);
		rewrite(tmp, metaInfo.size - sizeof(uint32_t));
		save(file);
	}
	else {
		tmp -= metaInfo.size;
	}
	delete[] tmp;
	close();
	return inDir;
}

bool File::dirCheck(std::fstream& file) {
	if (startClusterId <= UINT32_MAX - 1 && metaInfo.isDir) {
		if (!isOpen)
			open(file, startClusterId);
		return true;
	}
	else
		return false;
}

std::ostream& operator<<(std::ostream& os, File& file) {
	os << "startClusterId " << file.startClusterId << "\n";
	os << "FileName: " << file.metaInfo.name << "\n";
	os << "Size " << file.metaInfo.size << "\n";
	os << "Dir " << file.metaInfo.isDir << "\n";
	os << "Opened " << file.isOpen << "\n";
	os << "data:\t\n" << file.data << "\n" << "\n";
	return os;
}

size_t File::dataLen() {
	size_t s = 0;
	
	while (*data != 0 && s < 2048) {
		s++;
		data++;
	}
		
	data -= s;
	return s;
}

size_t File::len(const char* data) {
	size_t s = 0;

	while (*data != 0 && s < 2048) {
		s++;
		data++;
	}

	data -= s;
	return s;
}