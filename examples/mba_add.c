#include<stdio.h>


int add(int x, int y){
	return x+y;
}


int sub(int x, int y){
	return x - y;
}

int oper(int x, int y){
  return x-y+1;
}

int main(){

  printf("add(1,3) = %i\n", add(1, 3));
  printf("sub(15,10) = %i\n", sub(15,10));
  printf("oper(199,100) = %i\n", oper(199,100));

  return -1;
}
