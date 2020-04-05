#pragma once
#include "httplib.h"
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
		Storage();
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
#define LISTEN_DIR "./backup/"

class Cloud_Client {
private:
	string m_srv_ip;
	uint16_t m_srv_port;
	string m_listen_dir;  //��ص�Ŀ¼����
	DataManager data_manage;
public:
	Cloud_Client(const string &filename,const string &store_file,const string &srv_ip,uint16_t srv_port)
		:m_listen_dir(filename),
		data_manage(store_file),
		m_srv_ip(srv_ip),
		m_srv_port(srv_port)
	{}
	bool Start() {   
		data_manage.InitLoad();  //������ǰ�ı�����Ϣ
		while (1) {
			vector<string> list;
			GetBackuoFileList(&list); //��ȡ�����е���Ҫ���ݵ��ļ�����
			for (int i = 0;i < list.size();++i) {
				string name = list[i];  //�ļ���
				string pathname = m_listen_dir + name;  //�ļ�·����
				//��ȡ�ļ�����,��Ϊ��������
				string body;
				FileUtil::Read(pathname, &body);
				//ʵ����client����׼������HTTP�ϴ��ļ�����
				httplib::Client client(m_srv_ip, m_srv_port);
				string req_path = "/" + name;
				auto rsp = client.Put(req_path.c_str(), body, "application/octet-stream");
				if (rsp == nullptr || (rsp != nullptr&&rsp->status != 200)) {  //����ļ��ϴ�����ʧ��
					continue;
				}
				string etag;
				GetETag(pathname, &etag);
				data_manage.Insert(name, etag);   //���ݳɹ������/������Ϣ
			}
			Sleep(1000);   //����1000����  �� 1����ؼ��
		}
		return true;
	}
	bool GetBackuoFileList(vector<string> *list) {  //��ȡ��Ҫ���ݵ��ļ��б�
		if (boost::filesystem::exists(m_listen_dir) == false) {
		boost::filesystem::create_directory(m_listen_dir);   //��Ŀ¼�������򴴽�
		}
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
