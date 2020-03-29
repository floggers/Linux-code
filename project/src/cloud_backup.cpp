#include "cloud_backup.hpp"
#include <thread>

void compress_test(char *argv[]){
    //argv[1] = 源文件名称
    //argv[2] = 压缩包名称
    _cloud_sys::CompressUtil::Compress(argv[1],argv[2]);
    std::string file = argv[1];
    file +=".txt";
    _cloud_sys::CompressUtil::UnCompress(argv[2],file.c_str());
}

void data_test(){
  _cloud_sys::DataManager data_manage("./test.txt");
  data_manage.InitLoad();
  data_manage.Insert("c.txt","c.txt.gz");
  std::vector<std::string> list;
  //获取所有信息测试
  data_manage.GetAllName(&list);
  for(auto &i:list){
    printf("%s\n",i.c_str());
  }
  printf("---------------------------------------------\n");
}

int main(int argc,char *argv[]){
  /*
  //文件备份路径不存在则创建
  if(boost::filesystem::exists(GZFILE_DIR) == false){
    boost::filesystem::create_directory(GZFILE_DIR);
  }
  //压缩包存放路径不存在则创建 
  if(boost::filesystem::exists(BACKUP_DIR) == false){
    boost::filesystem::create_directory(BACKUP_DIR);
  }
  _cloud_sys::data_manage.Insert("file.tar.gz.txt","file.tar.gz.txt");
  std::thread thr(m_non_compress); //c++11中的线程
  thr.jion();//等待线程退出
  */
  _cloud_sys::Server server;
  server.Start();
    return 0;
}
