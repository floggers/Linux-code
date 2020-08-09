#include <iostream>
#include <unistd.h>
using namespace std;

//单例模式--饿汉模式
//  饿汉模式是线程安全的
class Singleton{
  private:
    Singleton () {
    }
static Singleton* single;
  public:
static Singleton* GetTarget(){
  return single;
  }
};

//单例模式--懒汉模式
//懒汉模式是有可能线程不安全的
//下面展示不安全的写法
class Singleton2{
  private:
    Singleton2(){
    }
   static  Singleton2* single;
  public:
   static Singleton2* GetTarget(){
     if(single == NULL){
        single = new Singleton2();
     }
        return single;
   } 
};

//为什么不安全？
//考虑两个线程同时首次调用GetTarget方法且同时检测到single是NULL值
//则两个线程会同时构造一个实例给single，这是严重的错误！同时，这也不是单例的唯一实现！

//单例模式--懒汉模式
//线程安全的写法1--加锁
class Singleton_safe{
  private:
    Singleton_safe(){
    }
   static Singleton_safe* single;
   static pthread_mutex_t mutex;
  public:
   static Singleton_safe* GetTarget(){
     if(single == NULL){
          pthread_mutex_lock(&mutex);
          if(single == NULL){
            single = new Singleton_safe();
          }
          pthread_mutex_unlock(&mutex);
     }
    return single;
   } 
};


//单例模式--懒汉模式
//线程安全的写法2--内部静态变量实现
class Singleton_safe2{
  private:
    Singleton_safe2(){
    }
    static pthread_mutex_t mutex;
  public:
    static Singleton_safe2* GetTarget(){
      pthread_mutex_lock(&mutex);
      static Singleton_safe2 obj;
      pthread_mutex_unlock(&mutex);
      return &obj;
    }
};
