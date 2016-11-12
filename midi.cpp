#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <map>

struct note {
	long long start;
	long long end;
	unsigned pitch;
};

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

void note_off(std::vector<note> &notes, std::map<std::pair<size_t, size_t>, size_t> &playing, unsigned event, unsigned data, long long when, bool warn) {
	auto i = playing.find(std::pair<size_t, size_t>(event, data));
	if (i != playing.end()) {
		if (notes[i->second].pitch != data) {
			fprintf(stderr, "wrong pitch\n");
			exit(EXIT_FAILURE);
		}
		notes[i->second].end = when;
		playing.erase(i);
	} else if (warn) {
		// printf("%u %u is not playing\n", event, data);
	}
}

std::vector<note> process(FILE *f, const char *fname) {
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

#if 0
	printf("reading %s\n", fname);
	printf("headerlen %u\n", headerlen);
	printf("track_types %u\n", track_types);
	printf("tracks %u\n", tracks);
	printf("pulses_per_q %u\n", pulses_per_q);
#endif

	std::vector<note> notes;

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
		// printf("%zu bytes in track\n", tracklen);
		size_t end = off + tracklen;
		unsigned event = 0;
		long long now = 0;

		std::map<std::pair<size_t, size_t>, size_t> playing;

		while (off < end) {
			unsigned delta = readvar(f, &off);
			// printf("delta %u\n", delta);
			now += delta;

			unsigned data = getc(f);
			off++;
			if (data >= 0x80) {
				event = data;
				data = getc(f);
				off++;
				if (data >= 0x80) {
					// printf("bad param %u %u\n", event, data);
					exit(EXIT_FAILURE);
				}
			} else {
				// printf("repeat event ");
			}

			if (event >= 0x80 && event <= 0x8F) {
				unsigned data2 = getc(f);
				off++;
				// printf("note off %u %u %u\n", event, data, data2);
				if (data2 >= 0x80) {
					// printf("bad param\n");
				}

				note_off(notes, playing, event & 0x0F, data, now, true);
			} else if (event >= 0x90 && event <= 0x9F) {
				unsigned data2 = getc(f);
				off++;
				// printf("note on %u %u %u\n", event, data, data2);
				if (data2 >= 0x80) {
					// printf("bad param\n");
				}

				note_off(notes, playing, event & 0x0F, data, now, data2 == 0);

				note n;
				n.start = now;
				n.end = 0;
				n.pitch = data;

				if (data2 != 0) {
					playing.insert(std::pair<std::pair<size_t, size_t>, size_t>(std::pair<size_t, size_t>(event & 0x0F, data), notes.size()));
					notes.push_back(n);
				}
			} else if (event >= 0xA0 && event <= 0xAF) {
				unsigned data2 = getc(f);
				off++;
				// printf("pressure %u %u %u\n", event, data, data2);
				if (data2 >= 0x80) {
					// printf("bad param\n");
				}
			} else if (event >= 0xB0 && event <= 0xBF) {
				unsigned data2 = getc(f);
				off++;
				// printf("control %u %u %u\n", event, data, data2);
				if (data2 >= 0x80) {
					// printf("bad param\n");
				}
			} else if (event >= 0xC0 && event <= 0xCF) {
				// printf("program %u %u\n", event, data);
			} else if (event >= 0xD0 && event <= 0xDF) {
				// printf("channel pressure %u %u\n", event, data);
			} else if (event >= 0xE0 && event <= 0xEF) {
				unsigned data2 = getc(f);
				off++;
				// printf("pitch bend %u %u %u\n", event, data, data2);
			} else if (event == 0xF0) {
				ungetc(data, f);
				off--;
				unsigned metalen = readvar(f, &off);
				unsigned char meta[metalen];
				if (fread(meta, sizeof(char), metalen, f) != metalen) {
					fprintf(stderr, "%s: short read of %u for meta\n", fname, metalen);
					exit(EXIT_FAILURE);
				}
				off += metalen;
				// printf("meta %02x %02x: %u\n", event, data, metalen);
			} else if (event == 0xFF) {
				unsigned metalen = readvar(f, &off);
				unsigned char meta[metalen];
				if (fread(meta, sizeof(char), metalen, f) != metalen) {
					fprintf(stderr, "%s: short read of %u for meta\n", fname, metalen);
					exit(EXIT_FAILURE);
				}
				off += metalen;
#if 0
				printf("meta %02x %02x: %u\n", event, data, metalen);
				for (size_t i = 0; i < metalen; i++) {
					if (meta[i] < ' ' || meta[i] > 0x7E) {
						printf("\\x%02X", meta[i]);
					} else {
						printf("%c", meta[i]);
					}
				}
				printf("\n");
#endif
			} else {
				// printf("unknown event %u %u\n", event, data);
			}
		}
		if (off > end) {
			// printf("ran long\n");
		}

		for (auto i = playing.begin(); i != playing.end(); ++i) {
			printf("never finished: %lu %lu\n", i->first.first, i->first.second);
		}
	}

#if 0
	for (size_t i = 0; i < notes.size(); i++) {
		printf("%u %lld %lld\n", notes[i].pitch, notes[i].start, notes[i].end);
	}
#endif

	return notes;
}

void identify_key(std::vector<note> const &notes, const char *name) {
	size_t pitches[12];
	for (size_t i = 0; i < 12; i++) {
		pitches[i] = 0;
	}

	for (size_t i = 0; i < notes.size(); i++) {
		if (notes[i].end > notes[i].start) {
			pitches[notes[i].pitch % 12] += notes[i].end - notes[i].start;
		}
	}

	size_t key = 0;
	double max = 0;

	const char *accidentals = ".#.#..#.#.#.";
	for (size_t i = 0; i < 12; i++) {
		size_t nat = 0, art = 0;

		for (size_t j = 0; j < 12; j++) {
			if (accidentals[j] == '#') {
				art += pitches[(i + j) % 12];
			} else {
				nat += pitches[(i + j) % 12];
			}
		}

		printf("%.3f ", nat / (double) (art + nat));

		if (nat / (double) (art + nat) > max) {
			max = nat / (double) (art + nat);
			key = i;
		}
	}

	const char *keys = "C DbD EbE F GbG AbA BbB ";
	printf("%c%c ", keys[key * 2], keys[key * 2 + 1]);

	printf("%s\n", name);
}

int main(int argc, char **argv) {
	extern int optind;
	extern char *optarg;
	int i;

	while ((i = getopt(argc, argv, "")) != -1) {
	}

	if (optind >= argc) {
		std::vector<note> notes = process(stdin, "standard input");
		identify_key(notes, "standard input");
	} else {
		for (i = optind; i < argc; i++) {
			FILE *f = fopen(argv[i], "rb");
			if (f == NULL) {
				perror(argv[i]);
				exit(EXIT_FAILURE);
			}

			std::vector<note> notes = process(f, argv[i]);
			identify_key(notes, argv[i]);
			fclose(f);
		}
	}

	return 0;
}
