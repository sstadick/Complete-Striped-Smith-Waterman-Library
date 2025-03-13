/**
 * ishc.c
 * A fast pattern search tool using the Smith-Waterman algorithm with SSE
 * acceleration, treating all data as raw bytes
 *
 * Usage: ishc <pattern> <input_file>
 * Output: <input_file>:<line_number> <line> for each match where score >=
 * pattern_length
 *
 * Compilation (M1 Mac):
 * gcc -o ishc ishc.c ssw.c ss_helpers.c -I. -lm -lz
 * -march=armv8-a+fp+simd+crypto+crc
 *
 * Compilation (Intel/x86):
 * gcc -o ishc ishc.c ssw.c ss_helpers.c -I. -lm -lz
 */

#include "ssw.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_MATCH 2      // Match score
#define DEFAULT_MISMATCH -2  // Mismatch penalty
#define DEFAULT_GAP_OPEN 3   // Gap open penalty
#define DEFAULT_GAP_EXTEND 1 // Gap extension penalty

// Buffer sizes for optimal I/O performance
#define INPUT_BUFFER_SIZE (64 * 1024)  // 64KB buffer for input
#define OUTPUT_BUFFER_SIZE (64 * 1024) // 64KB buffer for output
#define INITIAL_LINE_SIZE 4096         // Initial allocation for getline

// Create a scoring matrix that properly handles all 256 possible byte values
int8_t *create_scoring_matrix(int8_t match, int8_t mismatch) {
  const int alphabet_size = 256;
  int8_t *matrix =
      (int8_t *)calloc(alphabet_size * alphabet_size, sizeof(int8_t));
  if (!matrix) {
    fprintf(stderr, "Error: Failed to allocate memory for scoring matrix\n");
    return NULL;
  }

  // Fill the matrix with mismatch score
  for (int i = 0; i < alphabet_size * alphabet_size; i++) {
    matrix[i] = mismatch;
  }

  // Fill the diagonal with match score (same byte value)
  for (int i = 0; i < alphabet_size; i++) {
    matrix[i * alphabet_size + i] = match;
  }

  return matrix;
}

// Process each line with SSW alignment using getline
void process_file(const char *filename, const char *pattern, s_profile *profile,
                  int pattern_length) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Error: Could not open file %s\n", filename);
    return;
  }

  // Set a large buffer for file reading
  if (setvbuf(file, NULL, _IOFBF, INPUT_BUFFER_SIZE) != 0) {
    fprintf(stderr, "Warning: Could not set file buffer size\n");
  }

  // Pre-allocate line buffer with a reasonable initial size for getline
  char *line = (char *)malloc(INITIAL_LINE_SIZE);
  size_t line_capacity = INITIAL_LINE_SIZE;
  ssize_t line_length;
  int line_num = 0;

  // Read file line by line using getline
  while ((line_length = getline(&line, &line_capacity, file)) != -1) {
    line_num++;

    // Remove trailing newline if present
    if (line_length > 0 &&
        (line[line_length - 1] == '\n' || line[line_length - 1] == '\r')) {
      line[--line_length] = '\0';
      // Handle CR+LF if needed
      if (line_length > 0 && line[line_length - 1] == '\r') {
        line[--line_length] = '\0';
      }
    }

    if (line_length > 0) { // Skip empty lines
      // Perform SSW alignment - treat line as raw bytes
      // Use a fixed flag of 0 for more deterministic results
      s_align *result =
          ssw_align(profile, (int8_t *)line, line_length, DEFAULT_GAP_OPEN,
                    DEFAULT_GAP_EXTEND, 0, 0, 0, 15);

      // Check if score meets threshold - only check score
      if (result && result->score1 >= pattern_length) {
        printf("%s:%d %s\n", filename, line_num, line);
      }

      // Cleanup
      if (result)
        align_destroy(result);
    }
  }

  // Free the line buffer allocated by getline
  free(line);
  fclose(file);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <pattern> <input_file>\n", argv[0]);
    return 1;
  }

  // Set optimal buffer sizes for stdio
  if (setvbuf(stdin, NULL, _IOFBF, INPUT_BUFFER_SIZE) != 0) {
    fprintf(stderr, "Warning: Could not set stdin buffer size\n");
  }

  if (setvbuf(stdout, NULL, _IOFBF, OUTPUT_BUFFER_SIZE) != 0) {
    fprintf(stderr, "Warning: Could not set stdout buffer size\n");
  }

  const char *pattern = argv[1];
  const char *input_file = argv[2];
  int pattern_length = strlen(pattern);

  if (pattern_length == 0) {
    fprintf(stderr, "Error: Empty pattern\n");
    return 1;
  }

  // Create scoring matrix for all possible byte values
  int8_t *scoring_matrix =
      create_scoring_matrix(DEFAULT_MATCH, DEFAULT_MISMATCH);
  if (!scoring_matrix) {
    return 1;
  }

  // Initialize SSW profile with fixed parameters
  // Force score_size to 0 (byte-sized scores) for consistency
  s_profile *profile =
      ssw_init((int8_t *)pattern, pattern_length, scoring_matrix, 256, 2);
  if (!profile) {
    fprintf(stderr, "Error: Failed to initialize SSW profile\n");
    free(scoring_matrix);
    return 1;
  }

  // Process input file
  process_file(input_file, pattern, profile, pattern_length);

  // Cleanup
  init_destroy(profile);
  free(scoring_matrix);

  return 0;
}