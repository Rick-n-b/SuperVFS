#include "VFS.h"


VFS::VFS(const char filename[], uint32_t VF_SIZE, uint16_t CLUSTER_SIZE)
	: indexFile(filename)
{
	std::fstream FS(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	if (!FS.is_open()) {
		FS.open(filename, std::ios::out | std::ios::binary);
		FS.close();
		FS.open(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	}

	if (!FS.tellg()) {
		indexFile.init(FS, VF_SIZE, CLUSTER_SIZE);
		FAT.init();
		File root;
		root.create(FS, true, "root");
		
		File ff;
		ff.create(FS, false, "ff.txt");
		FS.flush();
		ff.rewrite("dfddassaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaadadadasdasdasdasda\0");
		ff.save(FS);
		FS.flush();
		root.addFile(FS, ff.startClusterId);
		FS.flush();
	}
	else {
		indexFile.load(FS);
		FS.close();
		remove(filename);
	}

	FS.flush();
	FS.close();
	//
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

	if (dirs[dirs.size() - 1] != "root")
		dirs.push_back("root");

	return dirs;
}

void VFS::createFile(std::string& name) {

}