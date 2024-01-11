#pragma once
#include<string>
using namespace std;

//文件上传返回结果码
enum FILE_UPLOAD_RETCODE
{
	FILE_UPLOAD_FAILED,
	FILE_UPLOAD_SUCCESS,
	FILE_UPLOAD_USERCANCEL		//用户取消上传
};


//文件下载返回结果码
enum FILE_DOWNLOAD_RETCODE
{
	FILE_DOWNLOAD_FAILED,
	FILE_DOWNLOAD_SUCCESS,
	FILE_DOWNLOAD_USERCANCEL	//用户取消下载
};

//获取文件md5值结果码
enum GET_FILE_MD5_RETCODE
{
	GET_FILE_MD5_FAILED,
	GET_FILE_MD5_SUCESS,
	GET_FILE_MD5_USERCANCEL
};

class IOSocket {
public:
	static IOSocket& GetInstance();
	bool Connect(int timeout = 3);
	void CloseFileServerConnection();
	bool Send(const char* pBuffer, int64_t nSize, int nTimeout = 30);
	bool Recv(char* pBuffer, int64_t nSize, int nTimeout = 30);


private:
	IOSocket();
	~IOSocket(void);
	IOSocket(const IOSocket& rhs) = delete;
	IOSocket& operator = (const IOSocket& rhs) = delete;

private:
	int				sock;
	string			_server_addr;
	unsigned short  _server_port;
};