#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned read4(FILE *f, long long *off) {
	unsigned char v[4];
	if (fread(v, sizeof(char), 4, f) != 4) {
		perror("short read 4");
		exit(EXIT_FAILURE);
	}

	*off += 4;
	return (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | (v[3]);
}

unsigned read2(FILE *f, long long *off) {
	unsigned char v[2];
	if (fread(v, sizeof(char), 2, f) != 2) {
		perror("short read 2");
		exit(EXIT_FAILURE);
	}

	*off += 2;
	return (v[0] << 8) | (v[1]);
}

unsigned readvar(FILE *f, long long *off) {
	unsigned ret = 0;

	while (1) {
		int c = getc(f);
		if (c == EOF) {
			perror("short read var");
			exit(EXIT_FAILURE);
		}

		*off = *off + 1;
		ret = (ret << 7) | (c & 0x7F);
		if ((c & 0x80) == 0) {
			return ret;
		}
	}
}

void process(FILE *f, const char *fname) {
	long long off;
	char sig[4];
	if (fread(sig, sizeof(char), 4, f) != 4) {
		perror("short read header");
		exit(EXIT_FAILURE);
	}
	off += 4;
	if (memcmp(sig, "MThd", 4) != 0) {
		fprintf(stderr, "%s: Not a MIDI file\n", fname);
		exit(EXIT_FAILURE);
	}

	unsigned headerlen = read4(f, &off);
	unsigned track_types = read2(f, &off);
	unsigned tracks = read2(f, &off);
	unsigned pulses_per_q = read2(f, &off);

	printf("reading %s\n", fname);
	printf("headerlen %u\n", headerlen);
	printf("track_types %u\n", track_types);
	printf("tracks %u\n", tracks);
	printf("pulses_per_q %u\n", pulses_per_q);

	for (size_t i = 0; i < tracks; i++) {
		if (fread(sig, sizeof(char), 4, f) != 4) {
			perror("short read track header");
			exit(EXIT_FAILURE);
		}
		off += 4;
		if (memcmp(sig, "MTrk", 4) != 0) {
			fprintf(stderr, "%s: Didn't find track header %zu\n", fname, i);
			exit(EXIT_FAILURE);
		}

		size_t tracklen = read4(f, &off);
		printf("%zu bytes in track\n", tracklen);
		size_t end = off + tracklen;
		unsigned event = 0;

		while (off < end) {
			unsigned delta = readvar(f, &off);
			printf("delta %u\n", delta);

			unsigned data = getc(f);
			off++;
			if (data >= 0x80) {
				event = data;
				data = getc(f);
				off++;
			} else {
				printf("repeat event ");
			}

			if (event >= 0x80 && event <= 0x8F) {
				unsigned data2 = getc(f);
				off++;
				printf("note off %u %u %u\n", event, data, data2);
				if (data2 != 0) {
					printf("expected note off 0\n");
				}
			}
			if (event >= 0x90 && event <= 0x9F) {
				unsigned data2 = getc(f);
				off++;
				printf("note on %u %u %u\n", event, data, data2);
			}
			if (event >= 0xA0 && event <= 0xAF) {
				unsigned data2 = getc(f);
				off++;
				printf("pressure %u %u %u\n", event, data, data2);
			}
			if (event >= 0xB0 && event <= 0xBF) {
				unsigned data2 = getc(f);
				off++;
				printf("control %u %u %u\n", event, data, data2);
			}
			if (event >= 0xC0 && event <= 0xCF) {
				printf("program %u %u\n", event, data);
			}
			if (event >= 0xD0 && event <= 0xDF) {
				printf("channel pressure %u %u\n", event, data);
			}
			if (event >= 0xE0 && event <= 0xEF) {
				printf("pitch bend %u %u\n", event, data);
			}
			if (event == 0xFF) {
				unsigned metalen = readvar(f, &off);
				char meta[metalen];
				if (fread(meta, sizeof(char), metalen, f) != metalen) {
					fprintf(stderr, "%s: short read of %u for meta\n", fname, metalen);
					exit(EXIT_FAILURE);
				}
				off += metalen;
				printf("meta %u %u\n", event, data);
			}
		}
		if (off > end) {
			printf("ran long\n");
		}
	}
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
