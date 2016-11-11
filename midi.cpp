#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned read4(FILE *f) {
	unsigned char v[4];
	if (fread(v, sizeof(char), 4, f) != 4) {
		perror("short read");
		exit(EXIT_FAILURE);
	}

	return (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | (v[3]);
}

unsigned read2(FILE *f) {
	unsigned char v[2];
	if (fread(v, sizeof(char), 2, f) != 2) {
		perror("short read");
		exit(EXIT_FAILURE);
	}

	return (v[0] << 8) | (v[1]);
}

void process(FILE *f, const char *fname) {
	char sig[4];
	if (fread(sig, sizeof(char), 4, f) != 4) {
		perror("short read");
		exit(EXIT_FAILURE);
	}
	if (memcmp(sig, "MThd", 4) != 0) {
		fprintf(stderr, "%s: Not a MIDI file\n", fname);
		exit(EXIT_FAILURE);
	}

	unsigned headerlen = read4(f);
	unsigned track_types = read2(f);
	unsigned tracks = read2(f);
	unsigned pulses_per_q = read2(f);

	printf("%s\n", fname);
	printf("headerlen %u\n", headerlen);
	printf("track_types %u\n", track_types);
	printf("tracks %u\n", tracks);
	printf("pulses_per_q %u\n", pulses_per_q);
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
