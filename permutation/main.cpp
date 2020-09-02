#include <iostream>
#include <vector>

using namespace std;

void swap(int &a,int &b){
  int tmp=a;
      a=b;
      b=tmp;
}

void swap2(int* a,int* b){
  int tmp=*a;
      *a=*b;
      *b=tmp;
}

void permutation(vector<int> list,int low,int high){
  if(low == high){
    for(int i = 0;i <= low;++i){
      cout<<list[i];
    }
      cout<<endl; 
  }
  else{
    for(int i = low;i <= high;++i){
      swap(list[i],list[low]);
      permutation(list,low+1,high);
      swap(list[i],list[low]);
    }
  }
}

int main(){
  vector<int> list = {1,2,3};
  permutation(list,0,2);
  return 0;
}
