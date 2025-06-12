#include "File.h"
#include "Serializer.h"

auto cmp = [](std::pair<uint32_t, MetaFile> const& a, std::pair<uint32_t, MetaFile> const& b)
{
	return a.second.name < b.second.name;
};

void File::create(std::fstream& file, bool isDir, std::string name) {

	this->startClusterId = FAT::addCluster(file);
	file.seekp(FAT::getClusterAbsLoc(startClusterId), std::ios::beg);
	metaInfo(isDir, name.c_str());
	Serializer<MetaFile>::serialize(metaInfo, file);
	file.flush();
}

void File::del(std::fstream& file) {
	if (isOpen == true || (startClusterId <= 1))
		return;
	if (metaInfo.isDir) {
		for (auto it : getFiles(file))
			FAT::remCluster(file, it.first);
		close();
	}
	FAT::remCluster(file, startClusterId);
	file.flush();
}

void File::open(std::fstream& file, uint32_t startClusterId) {
	if (isOpen == true || startClusterId >= UINT32_MAX - 1)
		return;

	if (!file.good()) {
		if (file.eof())
			std::cerr << "EOF\n";
		if (file.fail())
			std::cerr << "Format\n";
		if (file.bad())
			std::cerr << "Critical strike\n";
		if (!file.is_open())
			std::cerr << "File isnt open\n";
		file.clear();
	}
	this->startClusterId = startClusterId;
	file.seekg(FAT::getClusterAbsLoc(startClusterId), std::ios::beg);
	size_t metaSize = Serializer<MetaFile>::deserialize(metaInfo, file);
	file.sync();
	isOpen = true;
	data = new char[metaInfo.size + 1];

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
			file.sync();

		} while (dataRemain > 0);

		data -= metaInfo.size;
	}
	file.sync();
}

void File::save(std::fstream& file) {
	if (startClusterId >= UINT32_MAX - 1 && !isOpen)
		return;
	//сохранение метаданных на место нечала первого кластера

	if (!file.good()) {
		if (file.eof())
			std::cerr << "EOF\n";
		if (file.fail())
			std::cerr << "Format\n";
		if (file.bad())
			std::cerr << "Critical strike\n";
		if (!file.is_open())
			std::cerr << "File isnt open\n";
		file.clear();
	}

	std::cout << metaInfo.name << "\tcluster:" << startClusterId << "\n";

	file.seekp(FAT::getClusterAbsLoc(startClusterId), std::ios::beg);
	size_t metaSize = Serializer<MetaFile>::serialize(metaInfo, file);

	if (metaInfo.size + metaSize < IndexFile::CLUSTER_SIZE) {
		file.write(data, metaInfo.size);
	}
	else {
		int32_t dataRemain = metaInfo.size;
		uint32_t nextClusterId = startClusterId;
		uint32_t thisCluster = startClusterId;

		file.write(data, metaInfo.size - IndexFile::CLUSTER_SIZE);
		file.flush();

		data += IndexFile::CLUSTER_SIZE - metaSize;
		dataRemain -= IndexFile::CLUSTER_SIZE - metaSize;
	
		do {
			FAT::readCluster(file, thisCluster, nextClusterId);
			file.sync();

			if (nextClusterId == UINT32_MAX) {
				nextClusterId = FAT::addCluster(file);
				FAT::writeCluster(file, thisCluster, nextClusterId);
			}

			file.seekp(FAT::getClusterAbsLoc(nextClusterId));

			if (dataRemain < IndexFile::CLUSTER_SIZE) {
				file.write(data, dataRemain);
				data += dataRemain;
			}
			else {
				file.write(data, IndexFile::CLUSTER_SIZE);
				data += IndexFile::CLUSTER_SIZE;
			}

			file.flush();

			dataRemain -= IndexFile::CLUSTER_SIZE;

			thisCluster = nextClusterId;

		} while (dataRemain > 0);



		//if (nextClusterId != UINT32_MAX)//???
		//	FAT::remCluster(file, nextClusterId);


		data -= metaInfo.size;
	}
	file.flush();

}

void File::rename(std::fstream& file, std::string newName) {
	if (isOpen || startClusterId >= UINT32_MAX - 1) return;

	metaInfo.name = newName;
	save(file);
}

void File::rewrite(const char* data) {
	delete[] this->data;
	metaInfo.size = len(data);
	this->data = new char[metaInfo.size + 1]{ 0 };

	strcpy_s(this->data, metaInfo.size + 1, data);
}

void File::rewrite(const char* data, size_t len) {
	delete[] this->data;
	metaInfo.size = len;
	this->data = new char[metaInfo.size];
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
	char* tmp = new char[metaInfo.size + len];
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
std::map<uint32_t, MetaFile> File::getFiles(std::fstream& file) {
	std::map<uint32_t, MetaFile> files;
	if (!dirCheck(file))
		return files;
	if (!file.good()) {
		if (file.eof())
			std::cerr << "EOF\n";
		if (file.fail())
			std::cerr << "Format\n";
		if (file.bad())
			std::cerr << "Critical strike\n";
		if (!file.is_open())
			std::cerr << "File isnt open\n";
		file.clear();
	}
	uint16_t count = metaInfo.size / sizeof(uint32_t);
	MetaFile meta;
	uint32_t metaLoc = UINT32_MAX - 1;
	for (int i = 0; i < count; i++) {
		file.sync();
		metaLoc = *reinterpret_cast<uint32_t*>(data);
		file.seekg(FAT::getClusterAbsLoc(metaLoc), std::ios::beg);
		Serializer<MetaFile>::deserialize(meta, file);
		file.sync();
		files.insert({ metaLoc, meta });
		data += sizeof(uint32_t);
	}
	data -= metaInfo.size;
	return files;
}

bool File::isInDir(std::fstream& file, uint32_t fileClusterId) {
	for (auto curFile : getFiles(file))
		if (curFile.first == fileClusterId)
			return true;
	return false;
}

bool File::addFile(std::fstream& file, uint32_t fileClusterId) {
	if (!dirCheck(file))
		return false;

	if (!isInDir(file, fileClusterId)) {
		add(reinterpret_cast<char*>(&fileClusterId), sizeof(uint32_t));
		if (metaInfo.size == sizeof(uint32_t)) {
			save(file);
			return true;
		}
		std::map<uint32_t, MetaFile> files = getFiles(file);
		std::vector<std::pair<std::string, uint32_t>> vec;
		for (auto& it : files) {
			vec.push_back({it.second.name, it.first});
		}
		std::sort(vec.begin(), vec.end());
		files.clear();

		for (auto &it : vec) {
			memcpy(data, &it.second, sizeof(uint32_t));
			data += sizeof(uint32_t);
		}
		data -= metaInfo.size;
		save(file);
		close();
		return true;
	}
	return false;
}

bool File::remFile(std::fstream& file, uint32_t fileClusterId) {
	if (!dirCheck(file))
		return false;

	char* tmp = new char[metaInfo.size + 1]{ 0 };
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
	if (startClusterId < UINT32_MAX - 1 && metaInfo.isDir) {
		if (!isOpen)
			open(file, startClusterId);
		return true;
	}
	else
		return false;
}

std::ostream& operator<<(std::ostream& os, File& file) {
	os << "Name: " << file.metaInfo.name;
	os << "Size: " << file.metaInfo.size << "\n";
	if (file.isOpen) 
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

MetaFile File::getMeta(std::fstream& file, uint32_t startClusterId){
	MetaFile meta;
	file.seekg(startClusterId, std::ios::beg);
	Serializer<MetaFile>::deserialize(meta, file);
	file.sync();
	return meta;
}

