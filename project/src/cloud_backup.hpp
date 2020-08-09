#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <zlib.h>
#include <pthread.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "httplib.h"

#define NONHOT_TIME 10  //最后一次访问时间在十秒以外
#define INTERVAL_TIME 30 //非热点的检测每30秒一次
#define BACKUP_DIR "./backup/" //文件的备份路径
#define GZFILE_DIR "./gzfile/" //压缩包存放路径
#define DATA_FILE "./list.backup"  //数据管理模块的数据备份文件名称


namespace _cloud_sys{
  class FileUtil{
    public:
      static bool Read(const std::string &name,std::string *body){
        std::ifstream fs(name,std::ios::binary);
        if (fs.is_open() == false){
          std::cout<<"open file "<< name <<" failed\n";
          return false;

        }
        int64_t fsize = boost::filesystem::file_size(name);
        body->resize(fsize);
        fs.read(&(*body)[0],fsize);
        if(fs.good() == false){
          std::cout<<"file "<< name << " read data failed!\n";
          return false;

        }
        fs.close();
        return true;

      }      
      static bool Write(const std::string &name,const std::string &body){
        std::ofstream ofs(name,std::ios::binary);
        if (ofs.is_open() == false){
          std::cout<<"open file "<< name <<" failed\n";
          return false;

        }
        ofs.write(&body[0],body.size());
        if(ofs.good() == false){
          std::cout<<"file "<< name << " write data failed!\n";
          return false;

        }
        ofs.close();
        return true;

      }


  };
  /********************************************************/

  class CompressUtil{
    public:
      //  文件压缩-原文件名称-压缩包名称
      static bool Compress(const std::string &src,const std::string &dst){
        std::string body;
        FileUtil::Read(src,&body);
        gzFile gf = gzopen(dst.c_str(),"wb");//打开压缩包
        if(gf == NULL){
          std::cout<< "open file " << dst << " failed!"<<std::endl;
          return false;

        }
        int wlen = 0;
        int len = body.size();
        while(wlen < len){//防止body中的数据没有一次性压缩成功
          //                                                      若一次没有全部压缩,则从未压缩的数据开始继续压缩
          int ret = gzwrite(gf,&body[wlen],body.size()-wlen);
          if(ret == 0){
            std::cout<< "file" << dst << "write compress data failed!"<<std::endl;
            return false;

          }
          wlen += ret;
        }
        gzclose(gf);
        return true;

      }
      //   文件解压缩-压缩包名称-原文件名称
      static bool UnCompress(const std::string &src,const std::string &dst){
        std::ofstream ofs(dst,std::ios::binary);
        if(ofs.is_open() == false){
          std::cout<< "open file " << dst << " failed!"<<std::endl;
          return false;

        }
        gzFile gf = gzopen(src.c_str(),"rb");//rb读取数据
        if(gf == NULL){
          std::cout<< "open file " <<src << " failed!"<<std::endl;
          ofs.close();
          return false;

        }
        char tmp[4096]={0};
        int ret ;
        // gzread(句柄,缓冲区,缓冲区大小)
        //       返回实际读取到的解压后的数据大小
        while((ret = gzread(gf,tmp,4096)) > 0){
          ofs.write(tmp,ret);

        }
        ofs.close();
        gzclose(gf);
        return true;

      }

  };
  /********************************************************/
  class DataManager{
    private:
      std::string _back_file; //持久化数据存储文件名称
      std::unordered_map<std::string,std::string> _file_list;//数据管理容器
      pthread_rwlock_t _rwlock;
    public:
      DataManager(const std::string &path)
        : _back_file(path)
      {}
      ~DataManager(){


      }
      bool Exists(const std::string &name){
        pthread_rwlock_rdlock(&_rwlock);
        auto it = _file_list.find(name);
        if(it == _file_list.end()){
          pthread_rwlock_unlock(&_rwlock);
          return false;

        }
        pthread_rwlock_unlock(&_rwlock);
        return true;

      }
      bool IsCompressed(const std::string &name){
        pthread_rwlock_rdlock(&_rwlock);
        auto it = _file_list.find(name);
        if(it == _file_list.end()){
          pthread_rwlock_unlock(&_rwlock);
          return false;

        }
        if(it->first == it->second){
          pthread_rwlock_unlock(&_rwlock);
          return false;   //两个名称一致,则表示未压缩

        }
        pthread_rwlock_unlock(&_rwlock);
        return true;

      }
      bool NonCompressedList(std::vector<std::string> *list){
        pthread_rwlock_rdlock(&_rwlock);
        for(auto it = _file_list.begin();it != _file_list.end();++it) {
          if(it->first == it->second){
            list->push_back(it->first);

          }

        }
        pthread_rwlock_unlock(&_rwlock);
        return true;

      }
      bool Insert(const std::string &src,const std::string &dst){
        pthread_rwlock_wrlock(&_rwlock); //更新修改需要加写锁
        _file_list[src] = dst;
        pthread_rwlock_unlock(&_rwlock);
        Storage();  //更新修改之后重新备份
        return true;

      }
      bool GetAllName(std::vector<std::string> *list){
        pthread_rwlock_wrlock(&_rwlock); //更新修改需要加写锁
        for(auto it = _file_list.begin();it != _file_list.end();++it) {
          list->push_back(it->first);  //获取的是源文件名称

        }
        pthread_rwlock_unlock(&_rwlock);
        return true; 

      }

      bool GetGzname(const std::string &src,std::string *dst){
        auto it = _file_list.find(src);
        if(it == _file_list.end()){
          return false;

        }
        *dst = it->second;
        return true;

      }

      bool Storage(){
        std::stringstream tmp;
        pthread_rwlock_rdlock(&_rwlock);
        for(auto it = _file_list.begin();it != _file_list.end();++it){
          tmp<<it->first<<" "<<it->second<<"\r\n";

        }
        pthread_rwlock_unlock(&_rwlock);
        FileUtil::Write(_back_file,tmp.str());
        return true;

      }
      bool InitLoad(){
        // 从数据的持久化存储文件中加载数据
        // 1.将这个备份文件的数据读取出来
        std::string body;
        if(FileUtil::Read(_back_file,&body) == false){
          return false;

        }
        //  2.进行字符串处理,按照\r\n划分
        std::vector<std::string> list;
        boost::split(list,body,boost::is_any_of("\r\n"),boost::token_compress_off);
        //   3.每一行按照空格进行分割  前面是key  后面是val
        for(auto &i:list){
          size_t pos=i.find(" ");
          if(pos == std::string::npos){
            continue;

          }
          std::string key = i.substr(0,pos);
          std::string val = i.substr(pos+1);
          //  4.将<key,val>添加到_file_list中
          Insert(key,val);

        }
        return true;

      }

  };
  _cloud_sys::DataManager data_manage(DATA_FILE);
  /******************************************************************/
  class NonHotCompress{   //非热点压缩
    private:
      std::string _gz_dir; //压缩后的文件存储路径
      std::string _ex_dir; //压缩前文件的所在路径
    private:
      bool FileIshot(const std::string &name){
        time_t cur_t = time(NULL);  //获取当前时间
        struct stat st;
        if(stat(name.c_str(),&st) < 0){
          std::cout<< "get file " << name << " stat failed\n";
          return false;

        }
        if((cur_t - st.st_atime) > NONHOT_TIME){
          return false; //非热点返回false

        }
        return true;  //NONHOT_TIME以内都是热点文件

      }
    public:
      NonHotCompress(const std::string gz_dir,const std::string ex_dir )
        :_gz_dir(gz_dir),
        _ex_dir(ex_dir)
    {}
      bool Start() {
        while(1){
          std::vector<std::string> list;
          data_manage.NonCompressedList(&list);
          for(int i = 0;i<list.size();++i){
            bool ret = FileIshot(list[i]);
            if (ret == false){
              std::string src_filename = list[i]; //源文件名称
              std::string dst_filename = list[i] + ".gz"; //压缩文件名称
              std::string src_name = _ex_dir + src_filename;//源文件路径
              std::string dst_name = _gz_dir + dst_filename;//压缩文件路径

              if(CompressUtil::Compress(src_name,dst_name) == true){
                data_manage.Insert(src_filename,dst_filename); //更新数据信息
                unlink(src_name.c_str()); //删除源文件

              } 

            } 

          }

          sleep(INTERVAL_TIME);

        }
        return true;

      }   
  };
  /******************************************************************/
  class Server{
    private:
      std::string _file_dir;//文件上传备份路径
      httplib::Server _server;
    private:

      static void FileUpload(const httplib::Request &req,httplib::Response &rsp){

        std::string filename = req.matches[1]; //单纯是文件名称
        std::string pathname = BACKUP_DIR + filename; //文件备份在指定路径
        FileUtil::Write(pathname,req.body); //向文件写入数据,文件不存在会创建
        data_manage.Insert(filename,filename);//添加文件信息到数据管理模块
        rsp.status = 200;
        return;

      }


      static void FileList(const httplib::Request &req,httplib::Response &rsp){

        std::vector<std::string> list;
        data_manage.GetAllName(&list);


        std::stringstream tmp;
        tmp << "<html><body><hr />";
        for(int i = 0;i<list.size(); ++i){
          tmp << "<a href='/Filedownload/" << list[i] << "'>" <<list[i] << "</a>";

          tmp << "<hr />";

        }
        tmp << "<hr /></body></html>";


        rsp.set_content(tmp.str().c_str(),tmp.str().size(),"text/html");
        rsp.status = 200;

      }


      static void FileDownload(const httplib::Request &req,httplib::Response &rsp){

        std::string filename = req.matches[1]; //这就是前面路由注册时捕捉的(.*)
        if(data_manage.Exists(filename) == false){
          rsp.status = 404; //文件不存在 page not found
          return;

        }

        std::string pathname = BACKUP_DIR + filename; //源文件的备份路径
        if(data_manage.IsCompressed(filename) == true){
          std::string gzfile;
          data_manage.GetGzname(filename,&gzfile); //获取压缩包名称
          std::string gzpathname = GZFILE_DIR + gzfile; //压缩包的路径名
          CompressUtil::UnCompress(gzpathname,pathname); //将压缩包解压
          unlink(gzpathname.c_str()); //删除压缩包
          data_manage.Insert(filename,filename); //更新数据信息

        }

        FileUtil::Read(pathname,&rsp.body); //直接将文件数据读取到rsp的body中
        rsp.set_header("Content-Type","application/octet-stream"); //二进制流下载
        rsp.status = 200;
        return;

      }
    public: 
      Server(){  }
      ~Server(){}
      bool Start(){                //启动网络通信模块接口
        _server.Put("/(.*)",FileUpload);
        _server.Get("/Filelist",FileList);
        _server.Get("/Filedownload/(.*)",FileDownload);
        _server.listen("0.0.0.0",9000);//搭建tcp服务器,进行http数据接收处理
        return true;
      }


  };

}
