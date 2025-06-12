#pragma once

#include <fstream>
#include <iostream>

class IndexFile {
public:
	//сериализуемые:
	static uint16_t TYPE; //FAT32++ - тип ФС
	static uint32_t VF_SIZE; // общий объем системы 
	static uint16_t CLUSTER_SIZE; // размер кластера
	static uint32_t FAT_LOC; // указатель на таблицу жира 
	//количество свободных кластеров 
	static uint32_t FAT_FREE;
	//кластер, до которого нет свободного пространства
	static uint32_t START_CLUSTER;
	// указатель на обл. данных
	static uint32_t DATA_LOC;

	void init(std::fstream& file, uint32_t VF_SIZE, uint16_t CLUSTER_SIZE);
	void load(std::fstream& file);
};