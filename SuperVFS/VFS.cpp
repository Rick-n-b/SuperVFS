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

bool isStarting(std::fstream& fs, uint32_t clusterId, uint32_t& index) {
	unsigned long i;
	uint8_t clusterPos = 0;
	
	fs.seekg(FAT::getClusterAbsLoc(clusterId), std::ios::beg);
	fs.read(reinterpret_cast<char*>(&clusterPos), sizeof(uint8_t));
		
	if (clusterPos == 1) {
		index += 1;
		return true;
	}

	index = 0;
	return false;
}

int findFirstUsable(std::fstream& fs, uint32_t beginCluster, uint32_t& outCluster, uint32_t& outValue)
{
	uint32_t cluster;
	uint32_t value = 0;
	uint8_t found = 0;


	for (cluster = beginCluster; cluster <= IndexFile::VF_SIZE / IndexFile::CLUSTER_SIZE; cluster++) {
		FAT::readCluster(fs, cluster, value);
		if (value == 0) {
			found = 1;
			break;
		}
	}
	if (!found) {
		return 1;
	}
	outCluster = cluster;
	outValue = value;
	return 0;
}

void def_switchClusters(std::fstream& fs, uint32_t cluster1, uint32_t cluster2)
{
	uint32_t isStarting1, isStarting2; /* if the clusters are starting.
											   If yes, they will hold (index + 1)
						   in table aTable
						*/
	uint32_t tmpVal1, tmpVal2;
	uint32_t clus1val, clus2val;
	int i; // temp variable

	if (cluster1 == cluster2)
		return;

	/* 1. find out if clusters are starting. If yes, update dir entry. */
	/* be careful on root! It can be one of the clusters */
	isStarting(fs, cluster1, isStarting1);
	isStarting(fs, cluster2, isStarting2);


	if (isStarting1) {
		if (!aTable[isStarting1 - 1].entryCluster) {
			/* the first cluster is root */
			
			bpb.BPB_RootClus = cluster2;
			d_writeSectors(0, (char*)&bpb, 1, bpb.BPB_SecPerClus);
		}
		else {
			f32_readCluster(aTable[isStarting1 - 1].entryCluster, entries);
			f32_setStartCluster(cluster2, &entries[aTable[isStarting1 - 1].entryIndex]);
			f32_writeCluster(aTable[isStarting1 - 1].entryCluster, entries);
		}
	}
	if (isStarting2) {
		if (!aTable[isStarting2 - 1].entryCluster) {
			/* second cluster is root */
			bpb.BPB_RootClus = cluster1;
			d_writeSectors(0, (char*)&bpb, 1, bpb.BPB_SecPerClus);
		}
		else {
			f32_readCluster(aTable[isStarting2 - 1].entryCluster, entries);
			if (debug_mode) {
				fprintf(output_stream, "    2:'");
				for (i = 0; i < 8; i++)
					fprintf(output_stream, "%c", entries[aTable[isStarting2 - 1].entryIndex].fileName[i]);
				fprintf(output_stream, _("' 0x%lx=(files[%lx]); dir=0x%lx.entry=%d; start=0x%lx (new 0x%lx)\n"),
					cluster2, isStarting2 - 1, aTable[isStarting2 - 1].entryCluster, aTable[isStarting2 - 1].entryIndex,
					f32_getStartCluster(entries[aTable[isStarting2 - 1].entryIndex]), cluster1);
			}
			f32_setStartCluster(cluster1, &entries[aTable[isStarting2 - 1].entryIndex]);
			f32_writeCluster(aTable[isStarting2 - 1].entryCluster, entries);
		}
	}
	/* 2. update FAT */
	if (f32_readFAT(cluster1, &clus1val)) error(0, _("Can't read from FAT !"));
	if (f32_readFAT(cluster2, &clus2val)) error(0, _("Can't read from FAT !"));
	/* If some or both clusters were part of the chain, it is necessary to update its/their
	   parents in FAT.

	   In a case that FAT is wrong and some cluster points at free cluster (i.e. clus1val or clus2val = 0),
	   cruel error will be created, because the parent won't be found. */
	if (!isStarting1 && clus1val)
		tmpVal1 = def_findParent(cluster1);
	else tmpVal1 = 0;
	if (!isStarting2 && clus2val)
		tmpVal2 = def_findParent(cluster2);
	else tmpVal2 = 0;
	if (tmpVal1) {
		
		f32_writeFAT(tmpVal1, cluster2);
	}
	if (tmpVal2) {
	
		f32_writeFAT(tmpVal2, cluster1);
	}
	/* switching FAT values */
	if (clus1val == cluster2) {
		/* precaution */
		f32_writeFAT(cluster1, clus2val);
		f32_writeFAT(cluster2, cluster1);
	}
	else if (clus2val == cluster1) {
		/* precaution from the other side */
		/* If cluster1 < cluster2, we should not consider this option.. */
		f32_writeFAT(cluster1, cluster2);
		f32_writeFAT(cluster2, clus1val);
	}
	else {
		f32_writeFAT(cluster1, clus2val);
		f32_writeFAT(cluster2, clus1val);
	}

	/* update aTable */
	if (isStarting1)
		aTable[isStarting1 - 1].startCluster = cluster2;
	if (isStarting2)
		aTable[isStarting2 - 1].startCluster = cluster1;

	/* If some of switched clusters was direntry of some starting cluster in aTable, we have to update
	   also this value */
	for (tmpVal1 = 0; tmpVal1 < tableCount; tmpVal1++) {
		if (aTable[tmpVal1].entryCluster == cluster1) {
			tmpVal2 = aTable[tmpVal1].entryCluster;
			aTable[tmpVal1].entryCluster = cluster2;
			
		}
		else if (aTable[tmpVal1].entryCluster == cluster2) {
			tmpVal2 = aTable[tmpVal1].entryCluster;
			aTable[tmpVal1].entryCluster = cluster1;
			
		}
	}

	/* 3. physicall switch */
	f32_readCluster(cluster1, cacheCluster1);
	f32_readCluster(cluster2, cacheCluster2);
	f32_writeCluster(cluster1, cacheCluster2);
	f32_writeCluster(cluster2, cacheCluster1);

	

	/* Update "." and ".." entries if one of starting cluster was directory*/

	// if a directory is moving somewhere else, 
	// in all its dir entries we must find subdirectories,
	// load their entries and at every '..' entry put the new value
	// of the directory cluster..
	if ((entries2 = (F32_DirEntry*)malloc(entryCount * sizeof(F32_DirEntry))) == NULL) {
		error(0, _("Out of memory !"));
	}

	if (isStarting1 && aTable[isStarting1 - 1].isDir) {
		// cluster1 will point to cluster2
		for (tmpVal1 = cluster2; !F32_LAST(tmpVal1); tmpVal1 = f32_getNextCluster(tmpVal1)) {
			f32_readCluster(tmpVal1, entries);
			if (!memcmp(entries[0].fileName, ".       ", 8)) {
				// found it
				
				f32_setStartCluster(cluster2, &entries[0]);
			}
			if (!memcmp(entries[1].fileName, "..      ", 8)) {
				tmpVal2 = def_findParent(tmpVal1);
				// found it
				
				f32_setStartCluster(tmpVal2, &entries[1]);
			}
			f32_writeCluster(tmpVal1, entries);

			for (i = 0; i < entryCount; i++) {
				if (!memcmp(entries[i].fileName, ".       ", 8)) continue;
				if (!memcmp(entries[i].fileName, ".       ", 8)) continue;
				if (entries[i].fileName[0] == 0) continue;
				if (entries[i].fileName[0] == 0xe5) continue;
				if ((entries[i].attributes & 0x10) == 0x10) {
					// subdirectory
					tmpVal2 = f32_getStartCluster(entries[i]);
					f32_readCluster(tmpVal2, entries2);
					if (!memcmp(entries2[1].fileName, "..      ", 8)) {
				
						f32_setStartCluster(cluster2, &entries2[1]);
						f32_writeCluster(tmpVal2, entries2);
					}
				}
			}
		}
	}
	if (isStarting2 && aTable[isStarting2 - 1].isDir) {
		for (tmpVal1 = cluster1; !F32_LAST(tmpVal1); tmpVal1 = f32_getNextCluster(tmpVal1)) {
			// cluster2 will point to cluster1
			f32_readCluster(tmpVal1, entries);
			if (!memcmp(entries[0].fileName, ".       ", 8)) {
				// found it
				
				f32_setStartCluster(cluster1, &entries[0]);
			}
			if (!memcmp(entries[1].fileName, "..      ", 8)) {
				tmpVal2 = def_findParent(tmpVal1);
				// found it
				
				f32_setStartCluster(tmpVal2, &entries[1]);
			}
			f32_writeCluster(tmpVal1, entries);
			for (i = 0; i < entryCount; i++) {
				if (!memcmp(entries[i].fileName, ".       ", 8)) continue;
				if (!memcmp(entries[i].fileName, ".       ", 8)) continue;
				if (entries[i].fileName[0] == 0) continue;
				if (entries[i].fileName[0] == 0xe5) continue;
				if ((entries[i].attributes & 0x10) == 0x10) {
					// subdirectory
					tmpVal2 = f32_getStartCluster(entries[i]);
					f32_readCluster(tmpVal2, entries2);
					if (!memcmp(entries2[1].fileName, "..      ", 8)) {
						
						f32_setStartCluster(cluster1, &entries2[1]);
						f32_writeCluster(tmpVal2, entries2);
					}
				}
			}
		}
	}
	free(entries2);
}

int def_optimizeStartCluster(std::fstream& fs, unsigned long startCluster, unsigned long beginCluster, unsigned long* outputCluster)
{
	uint32_t newCluster, value;

	if (startCluster == beginCluster)
		return 0;

	if (findFirstUsable(fs, beginCluster, newCluster, value))
		return 1;
	if (startCluster > newCluster) {
		
		def_switchClusters(fs, startCluster, newCluster);
		if (newCluster > beginCluster)
			*outputCluster = newCluster;
	}
	return 0;
}

int def_defragTable()
{
	unsigned long tableIndex;
	unsigned long defClus = 1;
	unsigned long i, j = 0, k = 0;

	fprintf(output_stream, _("Defragmenting disk...\n"));

	/* Allocation of direntry and temporary clusters */
	entryCount = (bpb.BPB_SecPerClus * info.BPSector) / sizeof(F32_DirEntry);
	if ((entries = (F32_DirEntry*)malloc(entryCount * sizeof(F32_DirEntry))) == NULL) error(0, _("Out of memory !"));
	if ((cacheCluster1 = (unsigned char*)malloc(bpb.BPB_SecPerClus * info.BPSector * sizeof(unsigned char))) == NULL) error(0, _("Out of memory !"));
	if ((cacheCluster2 = (unsigned char*)malloc(bpb.BPB_SecPerClus * info.BPSector * sizeof(unsigned char))) == NULL) error(0, _("Out of memory !"));

	if (debug_mode) {
		fprintf(output_stream, _("(def_defragTable) original aTable values (%lu): "), tableCount);
		for (tableIndex = 0; tableIndex < tableCount; tableIndex++)
			fprintf(output_stream, "%lx | ", aTable[tableIndex].startCluster);
	}

	clusterIndex = 0;
	for (tableIndex = 0; tableIndex < tableCount; tableIndex++) {
		/* Optimally places starting cluster, it can cause additional fragmentation */
		defClus++;
		def_optimizeStartCluster(aTable[tableIndex].startCluster, defClus, &defClus);
		clusterIndex++;
		/* Defragmentation of non-starting clusters */
		defClus = def_defragFile(aTable[tableIndex].startCluster);

		if (!debug_mode)
			print_bar(30);
	}
	fprintf(output_stream, "\n");

	if (debug_mode) {
		fprintf(output_stream, _("(def_defragTable) aTable values (%lu): "), tableCount);
		for (tableIndex = 0; tableIndex < tableCount; tableIndex++)
			fprintf(output_stream, "%lx | ", aTable[tableIndex].startCluster);
	}

	free(cacheCluster2);
	free(cacheCluster1);
	free(entries);

	return 0;
}