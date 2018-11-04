#include <stdio.h>

static const unsigned char partab[256] = {
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
   1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
};

char line[1000];

void main() {
   int data, b, l, v_expected, v_actual;
   size_t len = sizeof(line);
   char* lineptr = line;
   int count_0 = 0;
   int count_1 = 0;
   int fail = 0;
   while (getline(&lineptr, &len, stdin) != -1) {
      char v = 0;
      sscanf(line, "%x %x %x %c\n", &data, &b, &l, &v);
      v_actual = (v == 'V');
      if (v_actual) {
         count_0++;
      } else {
         count_1++;
      }
      // This is the expression for the non-interrupted case
      v_expected = partab[b ^ ((data + l) & 0x07)];
      // This is the additional changes seen in the interrupted case
      if (data & 0x80) {
         v_expected ^= partab[(b - 1) & 7] ^ 1;
      } else {
         v_expected ^= partab[(b + 1) & 7] ^ 1;
      }
      if (v_expected != v_actual) {
         printf("fail data=%02x b=%02x l=%02x v_expected=%x v_actual=%x\n",
                data, b, l, v_expected, v_actual);
         fail++;
      }
   }
   printf("Processed %d records with V=0\n", count_0);
   printf("Processed %d records with V=1\n", count_1);
   printf("%d failures\n", fail);
}
