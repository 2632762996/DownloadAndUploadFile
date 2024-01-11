#pragma once
#include <cstdint>
#include <fstream>

using namespace std;

unsigned long GetFileSize(const char* filename);

long GetFileMd5ValueA(const char* pszFileName, char* pszMd5, long nMd5Length, int64_t nFileSize);

class filetask {
public:
	filetask() :filename(nullptr) {}
	filetask(const char* name) :filename(name){}
	~filetask() {  }
	int UploadFile();
	int DownloadFile();
	void Setfilename(const char* name);
private:
	const char*  filename;
};