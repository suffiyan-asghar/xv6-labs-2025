#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data);

int main(int argc, char *argv[])
{
  if(argc == 1){
    printf("Example 1:\n");
    int a[2] = { 61810, 2025 };
    memdump("ii", (char*) a);
    
    printf("Example 2:\n");
    memdump("S", "a string");
    
    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *) &s);

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;
    
    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");
    
    printf("Example 4:\n");
    memdump("pihcS", (char*) &example);
    
    printf("Example 5:\n");
    memdump("sccccc", (char*) &example);
  } else if(argc == 2){
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while(n < sizeof(data)){
      int nn = read(0, data + n, sizeof(data) - n);
      if(nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

void memdump(char *fmt, char *data)
{
  char *dptr = data;  // Pointer to navigate through the data

  for (int i = 0; fmt[i] != '\0'; i++) {
    switch (fmt[i]) {
      case 'i': {  // 4-byte integer (32-bit)
        int val = *(int*)dptr;
        printf("%d\n", val);
        dptr += 4;  // Move pointer by 4 bytes
        break;
      }
      case 'p': {  // 8-byte pointer (64-bit, print as hex)
        uint64 val = *(uint64*)dptr;
        printf("%lx\n", val);
        dptr += 8;  // Move pointer by 8 bytes
        break;
      }
      case 'h': {  // 2-byte short integer (16-bit)
        short val = *(short*)dptr;
        printf("%d\n", val);
        dptr += 2;  // Move pointer by 2 bytes
        break;
      }
      case 'c': {  // 1-byte ASCII character
        char val = *dptr;
        printf("%c\n", val);
        dptr += 1;  // Move pointer by 1 byte
        break;
      }
      case 's': {  // 8 bytes to a pointer to a string (64-bit)
        char *s_ptr = *(char**)dptr;  // Dereference to get the string
        printf("%s\n", s_ptr);
        dptr += 8;  // Move pointer by 8 bytes (pointer size)
        break;
      }
      case 'S': {  // Null-terminated string (from current position)
        printf("%s\n", dptr);
        break;  // Don't increment the pointer, since we're printing the whole string
      }
      default:
        break;  // Ignore invalid format specifiers
    }
  }
}
