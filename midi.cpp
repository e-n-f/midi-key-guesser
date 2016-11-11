#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void process(FILE *f, const char *fname) {
	printf("%s\n", fname);
}

int main(int argc, char **argv) {
	extern int optind;
	extern char *optarg;
	int i;

	while ((i = getopt(argc, argv, "")) != -1) {

	}

	if (optind >= argc) {
		process(stdin, "standard input");
	} else {
		for (i = optind; i < argc; i++) {
			FILE *f = fopen(argv[i], "rb");
			if (f == NULL) {
				perror(argv[i]);
				exit(EXIT_FAILURE);
			}

			process(f, argv[i]);
			fclose(f);
		}
	}

	return 0;
}
