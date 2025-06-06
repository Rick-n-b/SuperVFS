#include "VFS.h"
#include <iostream>
int main() {
	VFS vfs("SupVFS.bin", 1024 * 1024, 128);
	
	return 0;
}




/*Отладка
 
 uint32_t a = 2;
	char* aa = new char[2 * sizeof(uint32_t) + 1];
	wchar_t* aaa = new wchar_t[2 * sizeof(uint32_t) + 1];
	aa = reinterpret_cast<char*>(&a);
	aa += sizeof(uint32_t);
	aa = reinterpret_cast<char*>(&a);
	aaa = reinterpret_cast<wchar_t*>(&a);
	aaa += sizeof(uint32_t);
	aaa = reinterpret_cast<wchar_t*>(&a);

	aa -=  sizeof(uint32_t);
	aaa -= sizeof(uint32_t);

	std::string str = reinterpret_cast<char*>(&a);
	str += reinterpret_cast<char*>(&a);

	std::cout << aa << "|| " << strlen(aa) << " " << sizeof aa << " " << mblen(aa, 9) << "\n";
	std::cout << aaa << "|| " << wcslen(aaa) << " " << sizeof aaa << "\n";
	std::cout << str << "|| " << str.length() << " " << sizeof str << " " << str.capacity() << "\n";

 	char* data = new char[300]{ 'a' };
	for (int i = 0; i < 300; i++) {
		data[i] = 'a';
	}
	data[299] = '\0';
	std::cout << strlen(data) << "\n";
	std::cout << data << "\n\n";
	File myFile;

	myFile.create(file, false, "abobuss.txt");
	myFile.rewrite(data);
	std::cout << myFile;
	myFile.save(file);
	std::cout << "Saved\n" << myFile << "\n";
	myFile.open(file, 2);
	std::cout << "Opened\n" << myFile;
	myFile.close();
	myFile.del(file);
	printf("del\n");

	MetaFile meta;
	MetaFile meta2;
	Serializer<MetaFile> ser;
	meta(2, 100, true, "asdasdasdas.txt");
	meta2(1212, 122, false, "daasd.txt");

	std::fstream file("test.bin", std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	if (!file.is_open()) file.open("test.bin", std::ios::out | std::ios::binary);

	if (!file.tellg()) {
		ser.serialize(meta, file);
		ser.serialize(meta2, file);
	}
	else {
		file.seekg(std::ios::beg);
		file.seekp(std::ios::beg);
		ser.deserialize(meta, file);
		ser.deserialize(meta2, file);
	}




	std::cout << meta.nextCluster << " " << meta.id << " " << meta.size << " " << meta.isDir << " " << meta.name << "\n";
	std::cout << meta2.nextCluster << " " << meta2.id << " " << meta2.size << " " << meta2.isDir << " " << meta2.name;

*/