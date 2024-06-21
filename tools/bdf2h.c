/******************************************************************************
 *                       OS-3o3 operating system
 * 
 *                    convert BDF font to C header
 * 
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	NONE,
	STARTFONT,
	STARTCHAR,
	BITMAP
};

static FILE *fo;

//static char *glyphs[128];
static int state = NONE;
static int encoding = -1;
static int bbx_w = 0;
static int bbx_h = 0;
static int bbx_x = 0;
static int bbx_y = 0;
static int line = 0;
static int width = 0;
static int height = 0;
static int fx = 0;
static int fy = 0;
static int fh = 0;
static int fw = 0;
static int bit = 0;
static int octet = 0;

void valid(void)
{
	if (bit < 1) {
		return;
	}
	while (bit != 8) {
		octet <<= 1;
		bit++;
	}
	fprintf(fo, "0x%02X,\n", octet);
	octet = 0;
	bit = 0;
}

void emit(int b)
{
	octet = (octet << 1) | (b & 1);
	bit++;
	if (bit == 8) {
		valid();
	}
}

void bit_line(char *txt)
{
	int n = 0;
	int t = txt[0];
	int i;
	int x = 0;
	int dh;
	int y;

	dh = (height + fy) - bbx_h - bbx_y;
	if (line == 0) {
		y = 0;
		while (y < dh) {
			for (i = 0; i < width; i++) {
				printf(".");
				emit(0);
			}
			valid();
			printf("\n");
			y++;
		}
	}
	y = line + dh;
	for (i = 0; i < bbx_x; i++) {
		printf(".");
		emit(0);
		x++;
	}
	while (t >= '0' && t <= 'F') {
		if (t >= 'A') {
			t -= ('A' - 10);
			
		} else {
			t -= '0';
		}
		for (i = 0; i < 4 && x < width; i++) {
			if (t & 0x8) {
				emit(1);
				printf("@");
			} else {
				emit(0);
				printf(".");
			}
			t <<= 1;
			x++;
		}	
		n++;
		t = txt[n];
	}
	while (x < width) {
		printf(".");
		emit(0);
		x++;
	}
	valid();
	printf("\n");
	
	line++;
	if (line == bbx_h) {
		y++;
		while (y < height) {
			for (i = 0; i < width; i++) {
				printf(".");
				emit(0);
			}
			valid();
			printf("\n");
			y++;
		}
	}
}

void parse(char *txt)
{
	switch(state) {
	case NONE:
		if (strstr(txt, "STARTFONT")) {
			state = STARTFONT;
		}
		return;
	case STARTFONT:
		if (strstr(txt, "STARTCHAR")) {
			state = STARTCHAR;
		} else if (strstr(txt, "NORM_SPACE")) {
			sscanf(txt, "NORM_SPACE %d", &width);
		} else if (strstr(txt, "FONTBOUNDINGBOX")) {
			sscanf(txt, "FONTBOUNDINGBOX %d %d %d %d", &fw,&fh,&fx,&fy);
		} else if (strstr(txt, "PIXEL_SIZE")) {
			sscanf(txt, "PIXEL_SIZE %d", &height);
		} else if (strstr(txt, "COMMENT")) {
			fprintf(fo, "/* %s */\n\n", txt);
		} else if (strstr(txt, "ENDCHAR")) {
			state = STARTFONT;
		}
		return;	
	case STARTCHAR:
		if (strstr(txt, "BITMAP")) {
			if (encoding >= ' ' && encoding < 128) {
				state = BITMAP;
				line = 0;
				fprintf(fo, "/* %c U+%04X */\n", encoding, encoding);
				printf("%d\n", encoding);
			}
		} else if (strstr(txt, "ENDCHAR")) {
			state = STARTFONT;
		} else if (strstr(txt, "BBX")) {
			sscanf(txt, "BBX %d %d %d %d", 
					&bbx_w, &bbx_h, &bbx_x, &bbx_y);
		} else if (strstr(txt, "ENCODING")) {
			sscanf(txt, "ENCODING %d", &encoding);
		}
		return;	
	case BITMAP:
		if (strstr(txt, "ENDCHAR")) {
			state = STARTFONT;
		} else {
			bit_line(txt);
		}
		return;	
	}
}

int main(int argc, char *argv[])
{
	FILE *fi;
	char buf[1024];

	if (argc < 2) {
		exit(-1);
	}
	
	fi = fopen(argv[1], "r");
	if (!fi) {
		perror("Cannot open file");
		exit(-1);
	}
	
	fo = fopen("font.h", "w");
	fprintf(fo, "char font[] = {\n");

	//memset(glyphs, 0, sizeof(glyphs));

	while (!feof(fi)) {
		if (fgets(buf, sizeof(buf), fi)) {
			parse(buf);
		}
	}
	fprintf(fo, "0};\n");
	fclose(fo);
	fclose(fi);
	return 0;
}
