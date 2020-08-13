#include <iostream>
#include <vector>
#include <algorithm>
#include <math.h>
using namespace std;

int ThreeWordsSum(vector<int> vi,int aim){
  int size = vi.size();
  sort(vi.begin(),vi.end());
  int mincut = vi[0]+vi[1]+vi[2];
  for(int i = 0;i < size-2;++i){
    int left = i+1;
    int right = size-1;
    while(left < right){
      int sum = vi[i]+vi[left]+vi[right];
      if(abs(sum-aim)<abs(mincut-aim)){
        mincut = sum;
      }
      if(sum == aim){
        return sum;
      }
      else if(sum<aim){
        ++left;
      }
      else{
        --right;
      }
    }
  }
  return mincut;
}

int main(){
  vector<int> test={-1,2,1,-4};
  int aim = 1;
  cout<<ThreeWordsSum(test,aim)<<endl;
  return 0;
}
