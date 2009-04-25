struct x {
  int l;
  char *b;
};

int bar(struct x v) {
  return 0;
}

int foo(void) {
  bar((struct x) { .l = 123, .b = "hi"});
  return 123;
}
