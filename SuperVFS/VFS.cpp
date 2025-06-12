#include "VFS.h"


VFS::VFS(const char filename[], uint32_t VF_SIZE, uint16_t CLUSTER_SIZE)
	
{
	std::fstream FS(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	if (!FS.is_open()) {
		FS.open(filename, std::ios::out | std::ios::binary);
		FS.close();
		FS.open(filename, std::ios::in | std::ios::out | std::ios::binary);
	}

	if (!FS.tellg()) {
		FS.clear();
		FS.seekp((IndexFile::DATA_LOC + VF_SIZE + 1), std::ios::beg);
		FS.write("", 1);
		indexFile.init(FS, VF_SIZE, CLUSTER_SIZE);
		FAT.init(FS);
		File root;
		root.create(FS, true, "root");

		char* str = new char[3 * IndexFile::CLUSTER_SIZE];
		for (int i = 0; i < 3 * IndexFile::CLUSTER_SIZE - 1; i++)
			str[i] = 'b';
		str[3 * IndexFile::CLUSTER_SIZE - 1] = '\0';

		createFile("root/bbb.txt", FS);//2
		write("root/bbb.txt", str, FS);
		createFile("root/aaa/", FS);//3
		createFile("root/ccc.txt", FS);//4
		
		
		write("root/ccc.txt", str, FS);
		//deleteFile("root/bbb.txt", FS);

		createFile("root/aaa/aaa.txt", FS);//5
		for (int i = 0; i < 3 * IndexFile::CLUSTER_SIZE; i++)
			str[i] = 'a';
		str[3 * IndexFile::CLUSTER_SIZE - 1] = '\0';

		//write("root/ccc.txt", str, FS);
		createFile("root/aaa/eee.txt", FS);
		for (int i = 0; i < 2 * IndexFile::CLUSTER_SIZE; i++)
			str[i] = 'e';
		str[2 * IndexFile::CLUSTER_SIZE - 1] = '\0';
		//write("root/aaa/eee.txt", str, FS);//6
		createFile("root/777.txt", FS);//7
		deleteFile("root/bbb.txt", FS);
		defrag(FS);

		FS.close();
	}
	else {
		indexFile.load(FS);
		FS.close();
		remove(filename);
	}

	
}

std::vector<std::string> VFS::splitPath(std::string path) {
	std::vector<std::string> dirs;
	size_t pos = path.find_last_of('/');

	while (path.size()) {
		pos = path.find_last_of('/');

		if (pos > path.size()) {
			dirs.push_back(path);
			path.erase(0);
			break;
		}

		dirs.push_back(path.substr(pos + 1));
		path.erase(pos);
	}

	//if (dirs[dirs.size() - 1] != "root")
	//	dirs.push_back("root");

	return dirs;
}

void VFS::createFile(std::string path, std::fstream& file) {
	std::vector<std::string> dirs = splitPath(path);
	if (dirs.size() <= 2 && dirs[0] == "" || dirs.size() <= 1) {
		std::cerr << "Empty file name\n";
		return;
	}
		
	File newFile;
	if (dirs[0] == "") {
		dirs.erase(dirs.begin());
		newFile.create(file, true, dirs[0]);
	}
	else {
		newFile.create(file, false, dirs[0]);
	}
;
	File dir;
	uint32_t dirClusterId = seekParent(path, file);
	if (dirClusterId < UINT32_MAX - 1) {
		dir.open(file, dirClusterId);
		dir.addFile(file, newFile.startClusterId);
		dir.close();
	}
	else {
		newFile.del(file);
	}
}

uint32_t VFS::seek(std::string path, std::fstream& file) {
	File dir;
	dir.open(file, 1);
	std::vector<std::string> dirs = splitPath(path);
	std::map<uint32_t, MetaFile> files;
	uint32_t fileClusterId = 1;
	bool found = false;

	for (int i = dirs.size() - 2; i >= 0; i--) {
		files = dir.getFiles(file);
		if (files.empty())
			if (i == 0) {
				return dir.startClusterId;
			}
			else
				return UINT32_MAX;

		found = false;
		for (auto &it : files) {
			if (it.second.name == dirs[i]) {
				found = true;
				fileClusterId = it.first;
				break;
			}
		}
		if (!found)
			return UINT32_MAX;
		dir.close();
		dir.open(file, fileClusterId);
	}
	dir.close();
	return dir.startClusterId;
}

std::vector<uint32_t> VFS::seek(std::string path, std::string name, std::fstream& fs) {
	File dir;
	
	std::vector<uint32_t> filesFirstClusters;
	std::map<uint32_t, MetaFile> files;

	if (name == "")
		return filesFirstClusters;
	if (path == "")
		path = "root";

	uint32_t dirClusterId = seek(path, fs);
	dir.open(fs, dirClusterId);
	if (dirClusterId < UINT32_MAX - 1) {
		dir.getFiles(fs);
		
		for (auto& file : files) {
			if (file.second.name == name)
				filesFirstClusters.push_back(file.first);
			else if(file.second.isDir)
				seek(path + "/" + file.second.name, fs);
		}
	}
	else {
		return filesFirstClusters;
	}

	return filesFirstClusters;
}

uint32_t VFS::seekParent(std::string path, std::fstream& file) {
	std::vector<std::string> dirs = splitPath(path);
	if (dirs[0] == "") {
		dirs.erase(dirs.begin());
	}
	dirs.erase(dirs.begin());
	File dir;
	uint32_t fileClusterId = 1;
	bool found = false;
	dir.open(file, 1);
	std::map<uint32_t, MetaFile> files;
	
	if (dirs.empty()) {
		return 1;
	}
	for (int i = dirs.size() - 2; i >= 0; i--) {
		files = dir.getFiles(file);
		if (files.empty())
			if (i == 0) {
				return dir.startClusterId;
			}
			else
				return UINT32_MAX;

		found = false;
		for (auto& it : files) {
			if (it.second.name == dirs[i]) {
				found = true;
				fileClusterId = it.first;
				break;
			}
		}
		if (!found)
			return UINT32_MAX;
		dir.close();
		dir.open(file, fileClusterId);
	}
	dir.close();
	return dir.startClusterId;
}

void VFS::rename(std::string path, std::string newName, std::fstream& fs) {
	File rn;
	rn.open(fs, seek(path, fs));
	rn.rename(fs, newName);
	rn.close();
}

void VFS::deleteFile(std::string path, std::fstream& fs) {
	File del;

	del.startClusterId = seek(path, fs);
	del.del(fs);
}

void VFS::write(std::string path, const char* data, std::fstream& fs) {
	File f;
	if (!fs.good()) {
		if (fs.eof())
			std::cerr << "EOF\n";
		if (fs.fail())
			std::cerr << "Format\n";
		if (fs.bad())
			std::cerr << "Critical strike\n";
		if (!fs.is_open())
			std::cerr << "File isnt open\n";
		fs.clear();
	}
	f.open(fs, seek(path, fs));
	if (!f.metaInfo.isDir) {
		f.rewrite(data, strlen(data));
		f.save(fs);
	}
	f.close();
}

void count_frequency(uint32_t startClusterId, std::vector<uint64_t>& letter_count, std::fstream& fs)
{
	File forCount;
	forCount.open(fs, startClusterId);
	uint8_t c;

	for (int i = 0; i < forCount.metaInfo.size; i++) {
		letter_count[*forCount.data]++;
		forCount.data++;
	}
	forCount.close();
}

struct data_about_letter_vector
{
	std::vector<uint8_t> letter_vector;
	uint64_t count;
};

void create_alphabet(std::vector<data_about_letter_vector>& alphabet, const std::vector<uint64_t>& letter_count)
{
	size_t size = 0;

	for (size_t i = 0; i < 256; i++)
		if (letter_count[i]) {
			alphabet[size].letter_vector.push_back(i);
			alphabet[size].count = letter_count[i];
			size++;
		}

	alphabet.resize(size);
}

void count_frequency_and_create_alphabet(uint32_t startClusterId, std::vector<data_about_letter_vector>& alphabet, std::fstream& fs)
{
	std::vector<uint64_t> letter_count(256);

	count_frequency(startClusterId, letter_count, fs);

	create_alphabet(alphabet, letter_count);
}

bool compare(const data_about_letter_vector& left, const data_about_letter_vector& right)
{
	return left.count > right.count;
}

void create_elementary_codes(std::vector<uint64_t>& B, std::vector<uint8_t>& shift, uint32_t startClusterId, std::fstream& fs)
{
	std::vector<data_about_letter_vector> alphabet(256);

	count_frequency_and_create_alphabet(startClusterId, alphabet, fs);

	size_t size = alphabet.size();

	std::sort(alphabet.begin(), alphabet.end(), compare);

	std::vector<uint8_t> temp_array(256);

	while (size > 2) {
		size_t pos0 = size - 2, pos1 = pos0 + 1;

		for (uint8_t a : alphabet[pos0].letter_vector)
			shift[a]++;

		for (uint8_t a : alphabet[pos1].letter_vector) {
			B[a] = (1ULL << shift[a]) | B[a];
			shift[a]++;
		}

		size_t temp_array_size = 0;

		for (uint8_t a : alphabet[pos0].letter_vector)
			temp_array[temp_array_size++] = a;

		for (uint8_t a : alphabet[pos1].letter_vector)
			temp_array[temp_array_size++] = a;

		uint64_t sum_count = alphabet[pos0].count + alphabet[pos1].count;

		size -= 2;

		alphabet.resize(size);

		uint8_t pos_to_insert = size;

		while (pos_to_insert && (alphabet[pos_to_insert - 1].count < sum_count))
			pos_to_insert--;

		alphabet.insert(alphabet.begin() + pos_to_insert, data_about_letter_vector());

		alphabet[pos_to_insert].count = sum_count;

		alphabet[pos_to_insert].letter_vector.resize(temp_array_size);

		for (size_t i = 0; i < temp_array_size; i++)
			alphabet[pos_to_insert].letter_vector[i] = temp_array[i];

		size++;
	}

	if (size == 1)
		shift[alphabet[0].letter_vector[0]]++;
	else {
		size_t pos0 = size - 2, pos1 = pos0 + 1;

		for (uint8_t a : alphabet[pos0].letter_vector)
			shift[a]++;

		for (uint8_t a : alphabet[pos1].letter_vector) {
			B[a] = (1ULL << shift[a]) | B[a];
			shift[a]++;
		}
	}
}

void write_elementary_codes_into_file(const std::vector<uint64_t>& B, const std::vector<uint8_t>& shift, const uint8_t& length_of_last_byte, const uint8_t& last_byte, File& compressed)
{
	compressed.add((char*)&length_of_last_byte, 1);
	compressed.add((char*)&last_byte, 1);
	compressed.add((char*)&(B[0]), 2048);
	compressed.add((char*)&(shift[0]), 256);
}

void VFS::compress_file(const std::vector<uint64_t>& B, const std::vector<uint8_t>& shift, std::string path, std::fstream& fs)
{
	std::cout << "Compress\n";
	File dir;
	File file;
	File compressedFile;
	file.open(fs, seek(path, fs));
	file.open(fs, seekParent(path, fs));
	compressedFile.create(fs, false, "Compressed " + splitPath(path)[0]);
	uint8_t byte = 0, length = 0, c;

	//while (file.read((char*)(&c), 1)) {
	for (int i = 0; i < file.metaInfo.size; i++) {
		c = *file.data;
		file.data++;
		uint64_t elementary_code = B[c];
		uint8_t s = shift[c];
		uint64_t mask = (1ULL << s) - 1;

		while (s) {
			uint8_t need_bit_length = 8 - length;

			if (need_bit_length > s) {
				byte |= elementary_code << (need_bit_length - s);
				length += s;
				s = 0;
			}
			else {
				s -= need_bit_length;
				byte |= elementary_code >> s;
				mask >>= need_bit_length;
				elementary_code &= mask;
				compressedFile.add((char*)&byte, 1);
				byte = 0;
				length = 0;
			}
		}
	}
	file.data -= file.metaInfo.size;


	file.close();
	dir.close();
	write_elementary_codes_into_file(B, shift, length, byte, compressedFile);
	compressedFile.close();
}

void VFS::defrag(std::fstream& fs) {
	uint32_t inners;
	uint32_t nextInners;
	uint32_t zero = 0;
	uint16_t end = IndexFile::VF_SIZE / IndexFile::CLUSTER_SIZE - 1;

	for (uint32_t i = 1; i < end; i++) {
		
		
	}
}

uint32_t findParent(std::fstream& fs, uint32_t clusterId) {
	uint32_t i, val;
	if (!clusterId) return 0;
	for (i = 1; i < IndexFile::VF_SIZE / IndexFile::CLUSTER_SIZE; i++) {
		FAT::readCluster(fs, clusterId, val);
		if (val == clusterId) return i;
	}
	return 0;
}

bool isStarting() {
	return 0;
}

std::vector<std::vector<std::uint32_t>> VFS::getFilewsClusters(std::fstream& fs) {
	std::vector<std::vector<std::uint32_t>> files;
	std::vector<std::uint32_t> clusters;
	return files;
}