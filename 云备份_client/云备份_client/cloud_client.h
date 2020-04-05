#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <boost/filesystem.hpp>     
#include <boost/algorithm/string.hpp>   //split头文件
using namespace std;

class FileUtil {
public:
	//从文件中读取所有内容
	static bool Read(const std::string &name, std::string *body) {
		//必须要以二进制打开
		std::ifstream fs(name, std::ios::binary);//输入文件流
		if (fs.is_open() == false) {
			std::cout << "open file " << name << " failed\n";
			return false;
		}
		//boost::filesystem::file_size() 获取文件大小
		int64_t fsize = boost::filesystem::file_size(name);
		body->resize(fsize);
		//ifstream.read(char *,int);
		fs.read(&(*body)[0], fsize);
		if (fs.good() == false) {
			std::cout << "file " << name << " read data failed!\n";
			return false;
		}
		fs.close();
		return true;
	}
	//从文件中写入数据
	static bool Write(const string &name, const string &body) {
		//输出流  ofstream默认打开文件的时候会清空原有内容
		//当前是覆盖写入
		std::ofstream ofs(name, ios::binary);
		if (ofs.is_open() == false) {
			cout << "open file " << name << " failed\n";
			return false;
		}
		ofs.write(&body[0], body.size());
		if (ofs.good() == false) {
			cout << "file " << name << " write data failed!\n";
			return false;
		}
		ofs.close();
		return true;
	}
};

class DataManager {
private:
	string m_store_file;    //持久化存储文件
	unordered_map<string, string> m_backup_list;   //备份的文件信息
public:
	DataManager(const string &filename)
		: m_store_file(filename)
	{}
	bool Insert(const string &key, const string &val) { //插入/更新数据
		m_backup_list[key] = val;
		return true;
	}
	bool GetETag(const string &key,  string *val) {  //通过文件名获取etag信息
		auto it = m_backup_list.find(key);
		if (it == m_backup_list.end()) {
			return false;
		}
		*val = it->second;
		return true;
	}
	bool Storage() {     //持久化存储
	 //将m_backup_list中的数据进行持久化的存储
	 //将数据对象进行持久化存储---序列化
	 //filename etag\r\n
		stringstream tmp;
		for (auto it = m_backup_list.begin();it != m_backup_list.end();++it) {
			tmp << it->first << " " << it->second << "\r\n";
		}
		FileUtil::Write(m_store_file, tmp.str());
		return true;
	}
	bool InitLoad() {  //初始化加载原有数据
	 //从数据的持久化存储文件中加载数据
	//1.将这个备份文件的数据读取出来
		std::string body;
		if (FileUtil::Read(m_store_file, &body) == false) {
			return false;
		}
		//2.进行字符串处理,按照\r\n划分
		//boost::split(vector,src,sep,flag)
		std::vector<std::string> list;
		boost::split(list, body, boost::is_any_of("\r\n"), boost::token_compress_off);
		//3.每一行按照空格进行分割  前面是key  后面是val
		for (auto &i : list) {
			size_t pos = i.find(" ");
			if (pos == std::string::npos) {
				continue;
			}
			string key = i.substr(0, pos);
			string val = i.substr(pos + 1);
			//4.将<key,val>添加到_file_list中
			Insert(key, val);
		}
		return true;
	}
};
#define STORE_FILE "./list.backup"

DataManager data_manage(STORE_FILE);

class Cloud_Client {
private:
	string m_listen_dir;  //监控的目录名称
public:
	Cloud_Client(const string &filename)
		:m_listen_dir(filename)
	{}
	bool Start() {   //完成整体法人文件备份流程

		return true;
	}
	bool GetBackuoFileList(vector<string> *list) {  //获取需要备份的文件列表
		//1.进行目录监控,获取指定目录下的所有文件名称
		boost::filesystem::directory_iterator begin(m_listen_dir);
		boost::filesystem::directory_iterator end;
		for (;begin != end;++begin) {
			if (boost::filesystem::is_directory(begin->status())) {
				//目录不需要进行北非
				//当前不做多层级目录备份,遇到目录跳过
				continue;
			}
			string pathname = begin->path().string();
			string name = begin->path().filename().string();
			//2.逐个文件计算自身当前etag
			string cur_etag;
			GetETag(pathname, &cur_etag);
			string old_etag;
			//3.获取已经备份过的etag信息
			data_manage.GetETag(name,&old_etag);
			//4.与data_manage中保存的原有etag进行比对
			//   1.没有找到原有etag  ---> 文件需要备份
			//   2.找到原有etag,但是当前eatg和原有etag不相等,需要备份
			//   3/找到原有etag,并且与当前etag相等,则不需要备份
			if (cur_etag != old_etag) {
				list->push_back(name);    //当前etag与原来的etag不同 则备份
			}
		}
	}
	bool GetETag(const string &pathname, string *etag) {  //计算文件的etag信息
		//etag: 文件大小 - 文件最后一次修改时间
		int64_t fsize = boost::filesystem::file_size(pathname);
		time_t mtime = boost::filesystem::last_write_time(pathname);
		*etag = to_string(fsize) + "-" + to_string(mtime);
		return true;
	}
};
