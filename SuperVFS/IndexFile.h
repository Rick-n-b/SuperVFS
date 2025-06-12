#pragma once

#include <fstream>
#include <iostream>

class IndexFile {
public:
	//�������������:
	static uint16_t TYPE; //FAT32++ - ��� ��
	static uint32_t VF_SIZE; // ����� ����� ������� 
	static uint16_t CLUSTER_SIZE; // ������ ��������
	static uint32_t FAT_LOC; // ��������� �� ������� ���� 
	//���������� ��������� ��������� 
	static uint32_t FAT_FREE;
	//�������, �� �������� ��� ���������� ������������
	static uint32_t START_CLUSTER;
	// ��������� �� ���. ������
	static uint32_t DATA_LOC;

	void init(std::fstream& file, uint32_t VF_SIZE, uint16_t CLUSTER_SIZE);
	void load(std::fstream& file);
};