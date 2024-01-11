#include<iostream>
#include "task.h"
#include "FileMsg.h"
#include "protocolstream.h"
#include "IOSocket.h"
#include "MD5Sum.h"


using namespace std;

unsigned long GetFileSize(const char* filename) {
    if (filename == nullptr) {
        return 0;
    }
    struct stat statbuf;
    stat(filename, &statbuf);
    return statbuf.st_size;
}

long GetFileMd5ValueA(const char* FileName, char* Md5, long nMd5Length, int64_t nFileSize)
{

    fstream hFile;
    //获取文件句柄
    hFile.open(FileName, ios::in|ios::binary);
    if (hFile) {
        cout << "Succed to open file when get Md5" << endl;
    }
    else {
        cout << "Fail to open file when get Md5" << endl;
        return GET_FILE_MD5_FAILED;
    }


    bool bError = false;

    //nFileSize = (((int64_t)(((int64_t)dwFileSizeHigh) << 32)) + (int64_t)dwFileSizeLow);  大文件
    //每次最多读取500k大小
    int64_t nSizeEachRead = 500 * 1024;
    if (nFileSize <= nSizeEachRead)
        nSizeEachRead = nFileSize;

    int64_t nFileOffset = 0;
    MD5_CTX ctx;
    MD5Init(&ctx);

    while (TRUE)
    {
        //如果剩下的文件大小已经不足一个nSizeEachRead
        if (nFileOffset + nSizeEachRead > nFileSize)
            nSizeEachRead = nFileSize - nFileOffset;

        char* miniBuffer = new char[nSizeEachRead]; //TODO智能指针

        memset(miniBuffer, 0, (size_t)nSizeEachRead);
        hFile.read(miniBuffer, nSizeEachRead);
        int readsize = hFile.gcount();
        if (readsize != nSizeEachRead) {
            cout << "Fail to read file" << endl;
            bError = true;
            break;
        }

        MD5Update(&ctx, (unsigned char*)miniBuffer, (unsigned int)nSizeEachRead);

        nFileOffset += nSizeEachRead;

        delete[] miniBuffer;

        if (nFileOffset >= nFileSize)
            break;

        ::Sleep(1);

    }// end while-loop

    if (!bError)
    {
        unsigned char szTempMd5[16] = { 0 };
        MD5Final(szTempMd5, &ctx);
        CStringA strTemp;
        CStringA strMd5;
        for (int i = 0; i < 16; i++)
        {
            strTemp.Format("%02x", szTempMd5[i]);
            strMd5 += strTemp;
        }

        strcpy_s((char*)Md5, nMd5Length, strMd5);
    }
    hFile.close();

    return !bError ? GET_FILE_MD5_SUCESS : GET_FILE_MD5_FAILED;
}


void filetask::Setfilename(const char* name)
{
    filename = name;
}


int filetask::UploadFile() {

    fstream	     inFile;
    inFile.open(filename, ios::in | ios::binary);
    if (inFile) {
        cout << "open file success" << endl;
    }
    else {
        cout << "open file failed" << endl;
        return FILE_UPLOAD_FAILED;
    }
    
    IOSocket& iusocket = IOSocket::GetInstance();
    if (!iusocket.Connect()) {
        inFile.close();
        return FILE_UPLOAD_FAILED;
    }
    
    int64_t fsize = GetFileSize(filename);
    //文件md5值
    char szMd5[64] = { 0 };
    long nRetCode = GetFileMd5ValueA(filename, szMd5, 64, fsize);
    if (nRetCode == GET_FILE_MD5_FAILED)
    {
        cout<<"Failed to upload file as unable to get file md5."<<endl;
        inFile.close();
        return FILE_UPLOAD_FAILED;
    }
    int64_t offsetX = 0;
    int32_t m_seq = 0;
    while (true) {
        //每次发包包含文件大小和偏移量
        std::string outbuf;
        net::BinaryStreamWriter writeStream(&outbuf);
        writeStream.WriteInt32(msg_type_upload_req);
        writeStream.WriteInt32(m_seq);
        writeStream.WriteCString(szMd5, 32);
        writeStream.WriteInt64(offsetX);
        writeStream.WriteInt64(fsize);
        int64_t eachfilesize = 512 * 1024;
        if (fsize - offsetX < eachfilesize)
            eachfilesize = fsize - offsetX;

        
        char* eachfilebuf = new char[(int)eachfilesize];
        memset(eachfilebuf, 0, (size_t)eachfilesize);
        inFile.read(eachfilebuf, eachfilesize);
        int readsize = inFile.gcount();
        if (readsize != eachfilesize) {
            cout << "Fail to read file" << endl;
            break;
        }
        string filedata;
        filedata.append(eachfilebuf, (size_t)eachfilesize);
        writeStream.WriteString(filedata);
        writeStream.Flush();
        file_msg headerx = { outbuf.length() };
        outbuf.insert(0, (const char*)&headerx, sizeof(headerx));

        if (!iusocket.Send(outbuf.c_str(), (int64_t)outbuf.length())) {
            cout << "Failed to upload file" << endl;
            break;
        }

        delete[] eachfilebuf;

        offsetX += eachfilesize;
        file_msg header;
        if (!iusocket.Recv((char*)&header, (int64_t)sizeof(header)))
        {
            cout<<"Failed to upload file"<<endl;
            break;
        }

        char *recvBuf = new char[header.packagesize];
        if (!iusocket.Recv(recvBuf, header.packagesize))
        {
            cout<<"Failed to upload file"<<endl;
            break;
        }

        net::BinaryStreamReader readStream(recvBuf, (size_t)header.packagesize);
        int32_t cmd;
        if (!readStream.ReadInt32(cmd) || cmd != msg_type_upload_resp)
        {
            cout << "Failed to upload file as read cmd error." << endl;
            break;
        }

        //int seq;
        if (!readStream.ReadInt32(m_seq))
        {
            cout<<"Failed to upload file as read seq error."<<endl;
            break;
        }

        int32_t nErrorCode = 0;
        if (!readStream.ReadInt32(nErrorCode))
        {
            cout << "Failed to upload file as read ErrorCode error." << endl;
            break;
        }

        std::string filemd5;
        size_t md5length;
        if (!readStream.ReadString(&filemd5, 0, md5length) || md5length != 32)
        {
            cout << "Failed to upload file as read filemd5 error." << endl;
            break;
        }

        int64_t offset;
        if (!readStream.ReadInt64(offset))
        {
            cout << "Failed to upload file: %s as read offset error." << endl;
            break;
        }

        int64_t filesize;
        if (!readStream.ReadInt64(filesize))
        {
            cout << "Failed to upload file: %s as read filesize error."<< endl;
            break;
        }

        std::string dummyfiledata;
        size_t filedatalength;
        if (!readStream.ReadString(&dummyfiledata, 0, filedatalength) || filedatalength != 0)
        {
            cout << "Failed to upload file as read dummyfiledata error."<<endl;
            break;
        }

        cout << "UploadFile Complete " << (offset / (double)fsize) * 100 << "%" << endl;

        if (nErrorCode == file_msg_error_complete)
        {
            
            cout << "Succeed to upload file as there already exist file on server." << endl;
            
            inFile.close();
            iusocket.CloseFileServerConnection();
            return FILE_UPLOAD_SUCCESS;
        }

        

        delete[] recvBuf;

    }
    inFile.close();
    iusocket.CloseFileServerConnection();
    return FILE_UPLOAD_FAILED;
}

int filetask::DownloadFile() {

    fstream	     outFile;
    outFile.open(filename, ios::out | ios::binary);
    if (outFile) {
        cout << "Open file success" << endl;
    }
    else {
        cout << "Fail to open file" << endl;
        return FILE_DOWNLOAD_FAILED;
    }
   
    long nBreakType = FILE_DOWNLOAD_SUCCESS;

    IOSocket& iusocket = IOSocket::GetInstance();
    if (!iusocket.Connect()) {
        return FILE_DOWNLOAD_FAILED;
    }

    int32_t m_seq = 0;
    //偏移量
    size_t offset = 0;
    while (true) {
        std::string outbuf;
        net::BinaryStreamWriter writeStream(&outbuf);
        writeStream.WriteInt32(msg_type_download_req);
        writeStream.WriteInt32(m_seq);
        writeStream.WriteCString(filename, strlen(filename));   
        int64_t dummyoffset = 0;
        writeStream.WriteInt64(dummyoffset);
        int64_t dummyfilesize = 0;
        writeStream.WriteInt64(dummyfilesize);
        std::string dummyfiledata;
        writeStream.WriteString(dummyfiledata);
        int32_t clientNetType = client_net_type_broadband;
        writeStream.WriteInt32(clientNetType);
        writeStream.Flush();

        file_msg header = { outbuf.length() };
        outbuf.insert(0, (const char*)&header, sizeof(header));

        if (!iusocket.Send(outbuf.c_str(), (int64_t)outbuf.length()))
        {
            cout << "DownloadFile error when SendOnFilePort error" << endl;
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        file_msg recvheader;
        if (!iusocket.Recv((char*)&recvheader, (int64_t)sizeof(recvheader)))
        {
            cout << "DownloadFile error when recv header error" << endl;
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        char *buffer = new char[recvheader.packagesize];
        if (!iusocket.Recv(buffer, recvheader.packagesize))
        {
            cout << "DownloadFile error when recv body error" << endl; 
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        net::BinaryStreamReader readStream(buffer, (size_t)recvheader.packagesize);
        int32_t cmd;
        if (!readStream.ReadInt32(cmd) || cmd != msg_type_download_resp)
        {
            cout << "DownloadFile error when read cmd error" << endl;
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        //int seq;
        if (!readStream.ReadInt32(m_seq))
        {
            cout << "DownloadFile error when read seq error" << endl;
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        int32_t nErrorCode;
        if (!readStream.ReadInt32(nErrorCode))
        {
            cout << "DownloadFile error when read nErrorCode error" << endl;
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        if (nErrorCode == file_msg_error_not_exist)
        {
            cout << "DownloadFile error as file not exist on server" << endl;
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        std::string filemd5;
        size_t md5length;
        if (!readStream.ReadString(&filemd5, 0, md5length) || md5length == 0)
        {
            cout << "DownloadFile %s error when read filemd5 error" << endl;
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        int64_t offset;
        if (!readStream.ReadInt64(offset))
        {
            cout << "DownloadFile error when read offset error" << endl;
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        int64_t filesize;
        if (!readStream.ReadInt64(filesize) || filesize <= 0)
        {
            cout << "DownloadFile error when read filesize error" << endl;
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        std::string filedata;
        size_t filedatalength;
        if (!readStream.ReadString(&filedata, 0, filedatalength) || filedatalength == 0)
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            cout << "DownloadFile error when read filedata error" << endl;
            break;
        }

        outFile.write(filedata.c_str(), filedata.length());

        delete[] buffer;

        offset += (int64_t)filedata.length();
        cout << "Download File Complete " << (offset / (double)filesize) * 100 << "%" << endl;

        if (nErrorCode == file_msg_error_complete)
        {
            nBreakType = FILE_DOWNLOAD_SUCCESS;
            break;
        }

    }//end-while

    iusocket.CloseFileServerConnection();
    outFile.close();

    //下载成功
    if (nBreakType == FILE_DOWNLOAD_SUCCESS)
    {
        //::CloseHandle(hFile);
        cout << "Succeed to download file" << endl;
    }
    //下载失败或者用户取消下载
    else
    {
        if (nBreakType == FILE_DOWNLOAD_FAILED)
            cout << "Failed to download file" << endl;
        else
            cout << "User canceled to download file" << endl;
        //为了能删除下载的半成品，显式关闭文件句柄
        remove(filename);
    }
    
    return nBreakType;

}




