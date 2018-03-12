#include <stdio.h>
#include <syscall.h>

int
main (int argc, char **argv)
{
	// we are going to test the arguments, see if they are put on the stack properly
	//
	/* int i=0;
	 printf("\ncmdline args count=%s", argc);

	//   First argument is executable name only 
	  printf("\nexe name=%s", argv[0]);

	   for (i=1; i< argc; i++) {
		        printf("\narg%d=%s", i, argv[i]);
			 }

	    printf("\n");
	     return 0; */
printf("hello world");
//__asm__ __volatile__("int $0x30");
/*  int i;

  for (i = 0; i < argc; i++)
    printf ("%s ", argv[i]);
  printf ("\n");
  */
  //return EXIT_SUCCESS;
//	printf(argv);
////	for(;;);
}
