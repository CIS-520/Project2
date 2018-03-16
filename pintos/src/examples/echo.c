#include <stdio.h>
#include <syscall.h>

int
main (int argc, char **argv)
{
	printf("the argument count is %d \n",  argc);

printf("Address of argv from user space is <%x>!\n", (int)argv);

printf("Address of argv[0] from user space is <%x>!\n", (int)&argv[0]);

printf("Address of argv[1] from user space is <%x>!\n", (int)&argv[1]);
int i;
  for (i = 0; i < argc; i++)
	      printf ("%s ", argv[i]);
    printf ("\n");
for(;;);
      return EXIT_SUCCESS;
}
