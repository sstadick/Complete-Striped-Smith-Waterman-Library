#include "ssw.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* My stuff */
#include <stdint.h>
#include <string.h>

/* This table is used to transform numbers to nucleotide letters */
static const int8_t num_to_nt_table[5] = {'A', 'C', 'T', 'G', 'N'};

void print128_num_word(__m128i var) {
  int16_t val[8];
  memcpy(val, &var, sizeof(val));
  printf("%-3i %-3i %-3i %-3i %-3i %-3i %-3i %-3i", val[0], val[1], val[2],
         val[3], val[4], val[5], val[6], val[7]);
}

void print128_num_byte(__m128i var) {
  uint8_t val[16];
  memcpy(val, &var, sizeof(val));
  printf("%-3i %-3i %-3i %-3i %-3i %-3i %-3i %-3i %-3i %-3i %-3i %-3i %-3i "
         "%-3i %-3i %-3i",
         val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7], val[8],
         val[9], val[10], val[11], val[12], val[13], val[14], val[15]);
}

void print_profile(s_profile *prof) {
  int32_t segLen = (prof->readLen + 15) / 16;
  printf("bias: %i\n", prof->bias);
  printf("segLen: %i\n", segLen);
  printf("n: %i\n", prof->n);
  printf("readLen: %i\n", prof->readLen);

  printf("Each col is the index in the query, and the value is the "
         "match/mismatch AFTER accounting for bias.\n");
  printf("The index of the query is striped;\n");
  printf("So if seg len is two the indexes will look like:\n");
  printf("[0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30], [1, 3, "
         "5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31]\n");
  printf("Where the indicies from the query are 'strided' over the profile.\n");
  printf("If the query doesn't use a full segment (SIMD width), then bias "
         "fills in at the end.\n");
  printf("Byte Profile:\n");
  for (int nt = 0; nt < prof->n; nt++) {
    printf("%c: ", num_to_nt_table[nt]);
    for (int i = 0; i < segLen; i++) {
      printf("[");
      print128_num_byte(prof->profile_byte[nt * segLen + i]);
      printf("], ");
    }
    printf("\n");
  }

  printf("Word Profile:\n");
  int32_t wordSegLen = (prof->readLen + 7) / 8;
  for (int nt = 0; nt < prof->n; nt++) {
    printf("%c: ", num_to_nt_table[nt]);
    for (int i = 0; i < wordSegLen; i++) {
      printf("[");
      print128_num_word(prof->profile_word[nt * wordSegLen + i]);
      printf("], ");
    }
    printf("\n");
  }
}