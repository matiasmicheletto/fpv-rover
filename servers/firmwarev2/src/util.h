#include "config.h"

int parseValue(char * ptr, int size) {
  char val[size+1];
  memcpy(val, ptr, size);
  val[size] = '\0';
  return atoi(val);
}

bool areEqual(int value1, int value2){
  //const int diff = value1 - value2;
  //return diff >= 0 ? diff < INT_EPS : -diff < INT_EPS;
  return abs(value1-value2) < INT_EPS;
}