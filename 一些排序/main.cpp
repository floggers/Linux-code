#include <iostream>
#include "sort.h"
using namespace std;

void QuickSort(int* arr, int left, int right) {
  if (left >= right) return;
  int start = left;
  int end = right;
  int privot = arr[left];
  while (left < right) {
   while (arr[right] >= privot && left < right) {
            --right;
                
    }
    while (arr[left] <= privot && left < right) {
            ++left;
                
    }
    if (left < right) {
            swap(arr[left], arr[right]);
                
    }
      
  }
      arr[start] = arr[left];
      arr[left] = privot;
        QuickSort(arr, start, left-1);
        QuickSort(arr, right+1, end);
}

int main(){
  int arr[] = {3,4,2,6,10,9,7,8,11,1};
  QuickSort(arr,0,9);
  for(auto &e:arr){
    cout<<e<<" ";
  }
  cout<<endl;
  return 0;
}
