#pragma once
#include "MetaFile.h"

template<class T>
class Serializer
{
public:
	static size_t serialize(const T& obj, std::ostream& os);
	static size_t deserialize(T& obj, std::istream& is);
};

//Сериализация

//простых объектов
template<class T>
size_t Serializer<T>::serialize(const T& obj, std::ostream& os)  {
	os.write(reinterpret_cast<char*>(obj), sizeof(T));
	return sizeof(obj);
}
//строк
template<>
size_t Serializer<char*>::serialize(char* const& str, std::ostream& os)  {
	uint16_t strsize = uint16_t(strlen(str)) + 1;
	os.write(reinterpret_cast<char*>(&strsize), sizeof(uint16_t));
	os.write((str), strsize);
	return (strsize + sizeof(uint16_t));
}

template<>
size_t Serializer<MetaFile>::serialize(const MetaFile& meta, std::ostream& os) {
	MetaFile copy;
	copy(meta.size, meta.isDir, meta.name.c_str());

	os.write(reinterpret_cast<char*>(&copy.size), sizeof(uint16_t));
	os.write(reinterpret_cast<char*>(&copy.isDir), sizeof(bool));
	uint16_t strSize = copy.name.size();
	os.write(reinterpret_cast<char*>(&strSize), sizeof(uint16_t));
	os.write(meta.name.c_str(), copy.name.size());
	return sizeof(copy) + sizeof(uint16_t);
}


//Десериализация

//простых объектов
template<class T>
size_t Serializer<T>::deserialize(T& obj, std::istream& is) {
	is.read(reinterpret_cast<char*>(&obj), sizeof(T));
	return sizeof(obj);
}

//строк
template<>
size_t Serializer<char*>::deserialize(char*& str, std::istream& is) {
	std::uint16_t strsize = 0;
	is.read(reinterpret_cast<char*>(&strsize), sizeof(std::uint16_t)); strsize++;
	str = new char[strsize] {0};
	is.read(str, strsize);
	return (strsize + sizeof(uint16_t));
}


template<>
size_t Serializer<MetaFile>::deserialize(MetaFile& meta, std::istream& is) {
	MetaFile copy;

	is.read(reinterpret_cast<char*>(&copy.size), sizeof(uint16_t));
	is.read(reinterpret_cast<char*>(&copy.isDir), sizeof(bool));

	uint16_t strSize = 0;
	is.read(reinterpret_cast<char*>(&strSize), sizeof(uint16_t));
	char* str = new char[strSize + 1]{0};
	is.read(str, strSize);
	copy.name = str;
	meta(copy.size, copy.isDir, copy.name.c_str());
	return sizeof(copy) + sizeof(uint16_t);
}
