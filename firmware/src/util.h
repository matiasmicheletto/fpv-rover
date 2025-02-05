int parseValue(char * ptr, int size) {
  char val[size+1];
  memcpy(val, ptr, size);
  val[size] = '\0';
  return atoi(val);
}

bool areEqual(int value1, int value2, int eps = 1){
  return abs(value1-value2) < eps;
}
