#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <boost/filesystem.hpp>     
#include <boost/algorithm/string.hpp>   //splitͷ�ļ�
using namespace std;

class FileUtil {
public:
	//���ļ��ж�ȡ��������
	static bool Read(const std::string &name, std::string *body) {
		//����Ҫ�Զ����ƴ�
		std::ifstream fs(name, std::ios::binary);//�����ļ���
		if (fs.is_open() == false) {
			std::cout << "open file " << name << " failed\n";
			return false;
		}
		//boost::filesystem::file_size() ��ȡ�ļ���С
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
	//���ļ���д������
	static bool Write(const string &name, const string &body) {
		//�����  ofstreamĬ�ϴ��ļ���ʱ������ԭ������
		//��ǰ�Ǹ���д��
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
	string m_store_file;    //�־û��洢�ļ�
	unordered_map<string, string> m_backup_list;   //���ݵ��ļ���Ϣ
public:
	DataManager(const string &filename)
		: m_store_file(filename)
	{}
	bool Insert(const string &key, const string &val) { //����/��������
		m_backup_list[key] = val;
		return true;
	}
	bool GetETag(const string &key,  string *val) {  //ͨ���ļ�����ȡetag��Ϣ
		auto it = m_backup_list.find(key);
		if (it == m_backup_list.end()) {
			return false;
		}
		*val = it->second;
		return true;
	}
	bool Storage() {     //�־û��洢
	 //��m_backup_list�е����ݽ��г־û��Ĵ洢
	 //�����ݶ�����г־û��洢---���л�
	 //filename etag\r\n
		stringstream tmp;
		for (auto it = m_backup_list.begin();it != m_backup_list.end();++it) {
			tmp << it->first << " " << it->second << "\r\n";
		}
		FileUtil::Write(m_store_file, tmp.str());
		return true;
	}
	bool InitLoad() {  //��ʼ������ԭ������
	 //�����ݵĳ־û��洢�ļ��м�������
	//1.����������ļ������ݶ�ȡ����
		std::string body;
		if (FileUtil::Read(m_store_file, &body) == false) {
			return false;
		}
		//2.�����ַ�������,����\r\n����
		//boost::split(vector,src,sep,flag)
		std::vector<std::string> list;
		boost::split(list, body, boost::is_any_of("\r\n"), boost::token_compress_off);
		//3.ÿһ�а��տո���зָ�  ǰ����key  ������val
		for (auto &i : list) {
			size_t pos = i.find(" ");
			if (pos == std::string::npos) {
				continue;
			}
			string key = i.substr(0, pos);
			string val = i.substr(pos + 1);
			//4.��<key,val>��ӵ�_file_list��
			Insert(key, val);
		}
		return true;
	}
};
#define STORE_FILE "./list.backup"

DataManager data_manage(STORE_FILE);

class Cloud_Client {
private:
	string m_listen_dir;  //��ص�Ŀ¼����
public:
	Cloud_Client(const string &filename)
		:m_listen_dir(filename)
	{}
	bool Start() {   //������巨���ļ���������

		return true;
	}
	bool GetBackuoFileList(vector<string> *list) {  //��ȡ��Ҫ���ݵ��ļ��б�
		//1.����Ŀ¼���,��ȡָ��Ŀ¼�µ������ļ�����
		boost::filesystem::directory_iterator begin(m_listen_dir);
		boost::filesystem::directory_iterator end;
		for (;begin != end;++begin) {
			if (boost::filesystem::is_directory(begin->status())) {
				//Ŀ¼����Ҫ���б���
				//��ǰ������㼶Ŀ¼����,����Ŀ¼����
				continue;
			}
			string pathname = begin->path().string();
			string name = begin->path().filename().string();
			//2.����ļ���������ǰetag
			string cur_etag;
			GetETag(pathname, &cur_etag);
			string old_etag;
			//3.��ȡ�Ѿ����ݹ���etag��Ϣ
			data_manage.GetETag(name,&old_etag);
			//4.��data_manage�б����ԭ��etag���бȶ�
			//   1.û���ҵ�ԭ��etag  ---> �ļ���Ҫ����
			//   2.�ҵ�ԭ��etag,���ǵ�ǰeatg��ԭ��etag�����,��Ҫ����
			//   3/�ҵ�ԭ��etag,�����뵱ǰetag���,����Ҫ����
			if (cur_etag != old_etag) {
				list->push_back(name);    //��ǰetag��ԭ����etag��ͬ �򱸷�
			}
		}
	}
	bool GetETag(const string &pathname, string *etag) {  //�����ļ���etag��Ϣ
		//etag: �ļ���С - �ļ����һ���޸�ʱ��
		int64_t fsize = boost::filesystem::file_size(pathname);
		time_t mtime = boost::filesystem::last_write_time(pathname);
		*etag = to_string(fsize) + "-" + to_string(mtime);
		return true;
	}
};
