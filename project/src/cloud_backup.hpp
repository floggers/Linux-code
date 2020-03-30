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
      //从文件中读取所有内容
      static bool Read(const std::string &name,std::string *body){
        //必须要以二进制打开
        std::ifstream fs(name,std::ios::binary);//输入文件流
        if (fs.is_open() == false){
          std::cout<<"open file"<< name <<"faile\n";
          return false;
        }
        //boost::filesystem::file_size() 获取文件大小
        int64_t fsize = boost::filesystem::file_size(name);
         body->resize(fsize);
        //ifstream.read(char *,int);
        fs.read(&(*body)[0],fsize);
        if(fs.good() == false){
          std::cout<<"file"<< name << "read data failed!\n";
          return false;
        }
        fs.close();
        return true;
      }      
      //从文件中写入数据
      static bool Write(const std::string &name,const std::string &body){
      //输出流  ofstream默认打开文件的时候会清空原有内容
      //当前是覆盖写入
      std::ofstream ofs(name,std::ios::binary);
        if (ofs.is_open() == false){
          std::cout<<"open file"<< name <<"faile\n";
          return false;
        }
        ofs.write(&body[0],body.size());
        if(ofs.good() == false){
          std::cout<<"file"<< name << "write data failed!\n";
          return false;
        }
          ofs.close();
          return true;
      }

  };
  /********************************************************/
  class CompressUtil{
    public:
      //文件压缩-原文件名称-压缩包名称
      static bool Compress(const std::string &src,const std::string &dst){
        std::string body;
        FileUtil::Read(src,&body);
        gzFile gf = gzopen(dst.c_str(),"wb");//打开压缩包
        if(gf == NULL){
          std::cout<< "open file" << dst << "failed!"<<std::endl;
          return false;
        }
      int wlen = 0;
      int len = body.size();
      while(wlen < len){//防止body中的数据没有一次性压缩成功
        //若一次没有全部压缩,则从未压缩的数据开始继续压缩
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
      //文件解压缩-压缩包名称-原文件名称
      static bool UnCompress(const std::string &src,const std::string &dst){
        std::ofstream ofs(dst,std::ios::binary);
        if(ofs.is_open() == false){
          std::cout<< "open file" << dst << "failed!"<<std::endl;
          return false;
        }
        gzFile gf = gzopen(src.c_str(),"rb");//rb读取数据
        if(gf == NULL){
          std::cout<< "open file" <<src << "failed!"<<std::endl;
          ofs.close();
          return false;
        }
        char tmp[4096]={0};
        int ret = 0;
        //gzread(句柄,缓冲区,缓冲区大小)
        //返回实际读取到的解压后的数据大小
        while(ret = gzread(gf,tmp,4096) > 0){
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
//判断文件是否存在
bool Exists(const std::string &name){
  //是否能够从_file_list找到这个文件信息
  pthread_rwlock_rdlock(&_rwlock);
  auto it = _file_list.find(name);
  if(it == _file_list.end()){
  pthread_rwlock_unlock(&_rwlock);
  return false;
  }
  pthread_rwlock_unlock(&_rwlock);
  return true;
}
//判断文件是否已经压缩
bool IsCompressed(const std::string &name){
  //管理的数据: 源文件名称--压缩包名称
  //文件上传后,源文件名称和压缩包名称一致
  //文件压缩后,将压缩包文成更新为具体的包名
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
//获取未压缩文件列表
bool NonCompressedList(std::vector<std::string> *list){
  //遍历_file_list; 将没有压缩的文件名称添加到list中
  pthread_rwlock_rdlock(&_rwlock);
  for(auto it = _file_list.begin();it != _file_list.end();++it) {
    if(it->first == it->second){
      list->push_back(it->first);
    }
  }
  pthread_rwlock_unlock(&_rwlock);
  return true;
}
//插入/更新数据
bool Insert(const std::string &src,const std::string &dst){
  pthread_rwlock_wrlock(&_rwlock); //更新修改需要加写锁
  _file_list[src] = dst;
  pthread_rwlock_unlock(&_rwlock);
  Storage();  //更新修改之后重新备份
  return true;
}
//获取所有文件名称, 向外展示文件列表使用
bool GetAllName(std::vector<std::string> *list){
  pthread_rwlock_wrlock(&_rwlock); //更新修改需要加写锁
  for(auto it = _file_list.begin();it != _file_list.end();++it) {
      list->push_back(it->first);  //获取的是源文件名称
    }
  pthread_rwlock_unlock(&_rwlock);
  return true; 
}

//根据源文件名称获取压缩包名称
bool GetGzname(const std::string &src,std::string &dst){
  auto it = _file_list.find(src);
  if(it == _file_list.end()){
    return false;
  }
  dst = it->second;
  return true;
}

//数据改变后持久化存储,存储的是管理的文件名数据
bool Storage(){
  //将_file_list中的数据进行持久化的存储
  //将数据对象进行持久化存储---序列化
  //src dst\r\n
  std::stringstream tmp;
  pthread_rwlock_rdlock(&_rwlock);
  for(auto it = _file_list.begin();it != _file_list.end();++it){
    tmp<<it->first<<" "<<it->second<<"\r\n";
  }
  pthread_rwlock_unlock(&_rwlock);
  FileUtil::Write(_back_file,tmp.str());
  return true;
}
//启动时初始化加载原有数据
//原有数据格式: filename gzfilename\r\nfilename gzfilename\r\n.....
bool InitLoad(){
  //从数据的持久化存储文件中加载数据
  //1.将这个备份文件的数据读取出来
  std::string body;
  if(FileUtil::Read(_back_file,&body) == false){
    return false;
  }
  //2.进行字符串处理,按照\r\n划分
  //boost::split(vector,src,sep,flag)
  std::vector<std::string> list;
  boost::split(list,body,boost::is_any_of("\r\n"),boost::token_compress_off);
  //3.每一行按照空格进行分割  前面是key  后面是val
  for(auto &i:list){
    size_t pos=i.find(" ");
    if(pos == std::string::npos){
      continue;
    }
    std::string key = i.substr(0,pos);
    std::string val = i.substr(pos+1);
    //4.将<key,val>添加到_file_list中
    Insert(key,val);
  }
  return true;
}
 };
DataManager data_manage(DATA_FILE);
/******************************************************************/
class NonHotCompress{   //非热点压缩
  private:
    std::string _gz_dir; //压缩后的文件存储路径
    std::string _ex_dir; //压缩前文件的所在路径
  private:
    bool FileIshot(const std::string &name){
      //非热点文件:当前时间减去最后一次访问时间 > n秒
      time_t cur_t = time(NULL);  //获取当前时间
      struct stat st;
      if(stat(name.c_str(),&st) < 0){
        std::cout<< "get file" << name << "stat failed\n";
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
      //是一个循环的,持续的过程-每隔一段时间,判断有没有非热点文件,然后进行压缩
      while(1){
        //1.获取一下所有的未压缩的文件列表
        std::vector<std::string> list;
        data_manage.NonCompressedList(&list);
        //2.逐个判断这个文件是否是热点文件
        for(int i = 0;i<list.size();++i){
        bool ret = FileIshot(list[i]);
        if (ret == false){
        //非热点文件则组织源文件的路径名称以及压缩包的路径名称
        //进行压缩存储
       std::string src_filename = list[i]; //源文件名称
       std::string dst_filename = list[i] + ".gz"; //压缩文件名称
       std::string src_name = _ex_dir + src_filename;//源文件路径
       std::string dst_name = _gz_dir + dst_filename;//压缩文件路径
        //3.如果是非热点文件,则压缩这个文件,删除源文件
        if(CompressUtil::Compress(src_name,dst_name)){
          data_manage.Insert(src_name,dst_name); //更新数据信息
          unlink(src_name.c_str()); //删除源文件
           } 
          } 
        }
      }
      //4.休眠一段时间再检测
     sleep(INTERVAL_TIME);
      return true;
    }   
};
/******************************************************************/
class Server{
  private:
    std::string _file_dir;//文件上传备份路径
    httplib::Server _server;
  private:
    //文件上传处理回调函数
    static void FileUpload(const httplib::Request &req,httplib::Response &rsp){
   //req.method -- 解析出的请求方法
   //req.path -- 解析出的请求的资源路径
   //req.headers -- 这是一个头部信息键值对
   //req.body -- 存放请求数据的正文
      std::string filename = req.matches[1]; //单纯是文件名称
      std::string pathname = BACKUP_DIR + filename; //文件备份在指定路径
      FileUtil::Write(pathname,req.body); //向文件写入数据,文件不存在会创建
      data_manage.Insert(filename,filename);//添加文件信息到数据管理模块
      rsp.status = 200;

      // rsp.status = 200;
    //set_content(正文数据,正文数据长度,正文数据类型)
      // rsp.set_content("Fileupload",10,"text/html");
   //上面set_content这一步相当于下面这两步,可用下面这两步替换
   //rsp.body = "Fileupload";
   //rsp.set_header("Content-Type","text/html");
    }

    //文件列表处理回调函数
    static void FileList(const httplib::Request &req,httplib::Response &rsp){
      //1.通过data_manage数据管理获取文件名称列表
      std::vector<std::string> list;
      data_manage.GetAllName(&list);

      //2.组织响应的html网页数据
      std::stringstream tmp;
      tmp << "<html><body><hr />";
      for(int i = 0;i<list.size(); ++i){
        tmp << "<a href='/filedownload/'" << list[i] << "'>" <<list[i] << "</a>";
        //相当于 tmp << "<a href='/filedown/a.txt'>a.txt</a>";
        tmp << "<hr />";
      }
      tmp << "<hr /></body></html>";

      //3.填充rsp的正文与状态码还有头部信息
      //http响应格式:首行(协议版本 状态码 状态码描述) 头部 空行 正文
      //set_content(正文数据,数据大小,正文类型);
      rsp.set_content(tmp.str().c_str(),tmp.str().size(),"text/html");
      rsp.status = 200;
    }

    //文件下载处理回调函数
    static void FileDownload(const httplib::Request &req,httplib::Response &rsp){
    //1.从数据模块判断文件是否存在
      std::string filename = req.matches[1]; //这就是前面路由注册时捕捉的(.*)
      if(data_manage.Exists(filename) == false){
        rsp.status = 404; //文件不存在 page not found
        return;
      }
      //2.判断文件是否已经被压缩, 压缩了则要先解压缩,然后再读取文件数据
      std::string pathname = BACKUP_DIR + filename; //源文件的备份路径
      if(data_manage.IsCompressed(filename) == true){
        //文件被压缩,先将文件解压出来
        std::string gzfile;
        data_manage.GetGzname(filename,gzfile); //获取压缩包名称
        std::string gzpathname = GZFILE_DIR + gzfile; //压缩包的路径名
        CompressUtil::UnCompress(gzpathname,pathname); //将压缩包解压
        unlink(gzpathname.c_str()); //删除压缩包
      }
      //从文件中读取数据,响应客户端
      FileUtil::Read(pathname,&rsp.body); //直接将文件数据读取到rsp的body中
      rsp.set_header("Content-Type","application/octet-stream"); //二进制流下载
      rsp.status = 200;
      return;
    }
  public: 
    Server(){ }
    ~Server(){}
    bool Start(){                //启动网络通信模块接口
  _server.Put("/(.*)",FileUpload);
  _server.Get("/Filelist",FileList);
  //正则表达式:.*表示匹配任意字符串  ()表示捕捉这个字符串
  _server.Get("/Filedownload/(.*)",FileDownload);

  _server.listen("0.0.0.0",9000);//搭建tcp服务器,进行http数据接收处理
  return true;
    }
 
};

}
