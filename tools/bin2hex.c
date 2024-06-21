
/*
 *                          cod5.com computer
 *
 *                   convert binary file to C header
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
	fwrite("0x", 1, 2, out);
	fwrite(hex + ((c >> 4) & 0xF), 1, 1, out);
	fwrite(hex + (c & 0xF), 1, 1, out);
	return 0;
}

int writeLine(FILE *out, int c)
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

int main(int argc, char *argv[])
{
	FILE *in;
	FILE *out;
	int n = 0;
	char a[8] = {0};
	int p;
	char c[4];
	char *vn;
	char *s;

	if (argc < 3)
	{
		fprintf(stderr, "USAGE: %s  infile outfile\n", argv[0]);
		exit(-1);
	}
	in = fopen(argv[1], "rb");
	if (!in)
	{
		fprintf(stderr, "cannot open %s\n", argv[1]);
		exit(-1);
	}
	out = fopen(argv[2], "w+b");
	if (!out)
	{
		fprintf(stderr, "cannot open %s\n", argv[2]);
		exit(-1);
	}
	fwrite("char ", 1, 5, out);
	vn = argv[2];
	while (*vn == '.' || *vn == '/' || *vn == '\\')
	{
		vn++;
	}
	s = vn;
	while (*vn && *vn != '.')
	{
		if (*vn == '/' || *vn == '\\')
		{
			s = vn + 1;
		}
		vn++;
	}
	while (*s && *s != '.')
	{
		fwrite(s, 1, 1, out);
		s++;
	}
	fwrite("[]={\n", 1, 5, out);
	p = 0;
	writeLine(out, n);
	while (fread(c, 1, 1, in) == 1)
	{
		if (n > 0)
		{
			fwrite(", ", 1, 2, out);
			if (p == 8)
			{
				p = 0;
				fwrite(" /", 1, 2, out);
				fwrite("*", 1, 1, out);
				fwrite(a, 1, 8, out);
				fwrite("*/\n", 1, 3, out);
				writeLine(out, n);
			}
		}
		writeOut(out, c[0]);
		if (c[0] >= ' ' && c[0] < 0x7F && c[0] != '\\')
		{
			a[p] = c[0];
		}
		else
		{
			a[p] = '.';
		}
		p++;
		n++;
	}
	if (p > 0)
	{
		while (p < 8)
		{
			a[p] = ' ';
			p++;
		}
		fwrite("   /", 1, 4, out);
		fwrite("*", 1, 1, out);
		fwrite(a, 1, 8, out);
		fwrite("*/\n", 1, 3, out);
	}
	fwrite("};\n", 1, 3, out);
	fclose(in);
	fclose(out);
	return 0;
}
