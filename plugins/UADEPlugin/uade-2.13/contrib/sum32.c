/* A tool to compute 32-bit big-endian sums of given files. It simply adds
   all the 32-bit words inside file together by using two's complement
   arithmetics. The last 32-bit word in the file is padded with zero bytes
   if the file size is not divisible by 4.

   This tool can be used for creating work-around lists for certain
   eagleplayers. For example, Gobliins 2 (Infogrames replayer) are
   work-arounded by using checksums.

   USAGE: ./sum32 myfile
*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static void err(const char *s)
{
  fprintf(stderr, "Error reading %s\n", s);
}


int main(int argc, char *argv[])
{
  int i, j, retval = 0;
  uint32_t *arr = NULL;
  size_t rsize, nwords, ret;

  for (i = 1; i < argc; i++) {
    FILE *f = fopen(argv[i], "r");
    uint32_t sum;
    struct stat st;

    if (!f)
      goto error;

    if (stat(argv[i], &st))
      goto error;

    if (st.st_size == 0)
      goto error;

    rsize = (st.st_size + 3) & ~3; /* round to divisible by 4 */

    arr = malloc(rsize);
    if (arr == NULL)
      goto error;

    nwords = rsize / 4;

    arr[nwords - 1] = 0;

    ret = fread(arr, st.st_size, 1, f);
    if (ret == 0)
      goto error;

    sum = 0;
    for (j = 0; j < nwords; j++)
      sum += ntohl(arr[j]);

    printf("%s sum32 0x%.8x size %zd\n", argv[i], sum, st.st_size);

    free(arr);
    arr = NULL;
    fclose(f);
    continue;

  error:
    err(argv[i]);
    if (f)
      fclose(f);
    free(arr);
    retval = 1;
  }
  return retval;
}
