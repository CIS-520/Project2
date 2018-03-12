#include <stdio.h>
#include <syscall.h>

int
main (int argc, char **argv)
{
printf("hello world");
__asm__ __volatile__("int $0x30");
/*  int i;

  for (i = 0; i < argc; i++)
    printf ("%s ", argv[i]);
  printf ("\n");
  */
  return EXIT_SUCCESS;
//	printf(argv);
////	for(;;);
}
