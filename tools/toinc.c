
/*
 *                          cod5.com computer
 *
 *                 convert binary file to assembly header
 * 
 *                      17 may MMXXI PUBLIC DOMAIN
 *           The author disclaims copyright to this source code.
 *
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char hex[] = "0123456789ABCDEF";
int writeOut(FILE *out, int c)
{
	fwrite(".byte 0x", 1, 8, out); 
	fwrite(hex + ((c >> 4) & 0xF), 1, 1, out); 
	fwrite(hex + (c & 0xF), 1, 1, out); 
	return 0;
}

int writeLine(FILE* out, int c)
{
	fwrite("/*", 1, 2, out);
	fwrite(hex + ((c >> 16) & 0xF), 1, 1, out);
	fwrite(hex + ((c >> 12) & 0xF), 1, 1, out);
	fwrite(hex + ((c >> 8) & 0xF), 1, 1, out);
	fwrite(hex + ((c >> 4) & 0xF), 1, 1, out);
	fwrite(hex + (c & 0xF), 1, 1, out);
	fwrite("*/ ", 1, 3, out);
	return 0;
}

int main (int argc, char *argv[])
{
	FILE *in;
	FILE *out;
	int n = 0;
	char c[4];

	if (argc < 3) {
		fprintf(stderr, "USAGE: %s  infile outfile\n", argv[0]);
		exit(-1);
	}	
	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "cannot open %s\n", argv[1]);
		exit(-1);
	}
	out = fopen(argv[2], "w+b");
	if (!out) {
		fprintf(stderr, "cannot open %s\n", argv[2]);
		exit(-1);
	}
	while (fread(c, 1, 1, in) == 1) {
		if (n > 0) {
			fwrite("\n ", 1, 2, out); 
		}
		writeOut(out, c[0]);
		n++;
	}
	
	fwrite("\n", 1, 1, out); 
	fclose(in);
	fclose(out);
	return 0;
}


