/*
 *                          cod5.com computer
 *
 *                   copy file from host to .vhd image
 * 
 *                      1 april MMXXII PUBLIC DOMAIN
 *           The author disclaims copyright to this source code.
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/fat.h"

#define unused(x) (void)(x)
#define PTBL_OFF 446
#define PTBL_START_LBA 8
#define PTBL_LEN_LBA 12

void PosMonitor()
{
	printf("Panic\n");
	exit(-1);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-sign"
#include "../src/fat.c"
#pragma clang diagnostic pop


FAT fat;
FATFILE fatfile;
DIRENT dirent;
unsigned char bpb[512];
unsigned char mbr[512];
int hidden_sectors = 0;

void getDateTime(FAT_DATETIME* ptr)
{
	ptr->day = 0;
}

void readLogical(void* diskptr, size_t sector, void* buf)
{
	sector += hidden_sectors;
	/*fprintf(stderr, "read sector %d %p\n", sector, buf);*/
	fseek((FILE*)diskptr, sector * 512, SEEK_SET);
	if (!buf) {
		fprintf(stderr, "error null buffer sector %ld\n", (long)sector);
		exit(-9);
	}
	if (fread(buf, 1, 512, (FILE*)diskptr) != 512) {
		fprintf(stderr, "error reading sector %ld\n", (long)sector);
		exit(-8);
	}
}

void writeLogical(void* diskptr, size_t sector, void* buf)
{
	sector += hidden_sectors;
	/*fprintf(stderr, "write sector %d %p\n", sector, buf);*/
	fseek((FILE*)diskptr, sector * 512, SEEK_SET);
	fwrite(buf, 1, 512, (FILE*)diskptr);
}

int vhdGet16(unsigned char* blk, int byoff)
{
	int v;
	v = blk[byoff];
	v += blk[byoff] << 8;
	return v;
}

int vhdGet32(unsigned char* blk, int byoff)
{
	int v;
	v = blk[byoff];
	v += blk[byoff + 1] << 8;
	v += blk[byoff + 2] << 16;
	v += blk[byoff + 3] << 24;
	return v;
}


void vhdInit(FAT* fat, FILE *vhd)
{
	int r;
	int partid = 0;
	int s;

	if (fread(mbr, 1, 512, vhd) != 512) {
		fprintf(stderr, "Unable to read vhd mbr file\n");
		exit(-3);
	}
	s = vhdGet32(mbr, PTBL_OFF + PTBL_START_LBA + (16 * partid));
	/*l = vhdGet32(mbr, PTBL_OFF + PTBL_START_LBA + (16 * partid));*/
	

	fseek((FILE*)vhd, s * 512, SEEK_SET);
	if (fread(bpb, 1, 512, vhd) != 512) {
		fprintf(stderr, "Unable to read vhd vbr file\n");
		exit(-3);
	}
	hidden_sectors = vhdGet32(bpb, 0x1C);
	/*fprintf(stderr, "PTBL_START_LBA %d hidden: %d\n", s, hidden_sectors);*/

	r = fatInit(fat, bpb + 11, readLogical,
		writeLogical, (void*)vhd,
		getDateTime);
	if (r) {
		fprintf(stderr,"Unable to init fat file system\n");
		exit(-4);
	}
}

int main(int argc, char* argv[])
{
	FILE* vhd;
	FILE* fp;
	char buf[512];
	char cpbuf[512];
	char* path;
	unsigned int r, rt;

	if (argc < 3) {
		fprintf(stderr, "USAGE: %s file vhd://./disk.vhd:/path \n", argv[0]);
		exit(-1);
	}
	if (strncmp("vhd://", argv[2], 6)) {
		fprintf(stderr, "USAGE: %s file vhd://./disk.vhd:/path \n", argv[0]);
		exit(-5);
	}
	buf[0] = 0;
	strcat(buf, argv[2] + 6);
	path = strchr(buf, ':');
	path[0] = 0;
	path++;
	path++;
	fp = fopen(argv[1], "rb");
	vhd = fopen(buf, "rb+");
	fatDefaults(&fat);
	vhdInit(&fat, vhd);

	fatOpenFile(&fat, "efi/boot", &fatfile);
	if (fatfile.dir != 1 || fatfile.attr != DIRENT_SUBDIR) {
		fatCreatDir(&fat, "efi", "", DIRENT_SUBDIR);
		fatCreatDir(&fat, "efi/boot", "", DIRENT_SUBDIR);
		fatCreatDir(&fat, "overlays", "", DIRENT_SUBDIR);
	}
	fatOpenFile(&fat, "", &fatfile);
	if (fatfile.dir != 1 || fatfile.attr != DIRENT_SUBDIR) {
		fprintf(stderr, "No filesystem in %s\n", buf);
		exit(-6);
	}

#if 0
	r = 1;
	while (r > 0) {
		r = 0;
		i = fatReadFile(&fat, &fatfile, &dirent, sizeof(DIRENT), &r);
		if (r == sizeof(DIRENT) && dirent.file_name[0] != '\0') {
			/* LFNs should be ignored when checking if the directory is empty. */
			if (dirent.file_name[0] != DIRENT_DEL &&
				dirent.file_attr != DIRENT_LFN &&
				dirent.file_attr != 0xCD)
			{
				name[0] = 0;
				strncat(name, dirent.file_name, 11);
				fprintf(stderr, "FILE%02x%02x %s %x\n", name[0], name[1], name, dirent.file_attr);

			}
		}
		else {
			fprintf(stderr, "end of dir %d\n", r);
			break;
		}
	}
#endif 
	r = fatCreatNewFile(&fat, path, &fatfile, 0);
	if (r != 0) {
		fprintf(stderr, "Failed to create %s on vhd %s\n", path, buf);
		exit(-11);
	}
	r = fatOpenFile(&fat, path, &fatfile);
	if (r != 0) {
		fprintf(stderr, "Failed to open %s on vhd %s\n", path, buf);
		exit(-11);
	}
	r = fread(cpbuf, 1, 512, fp);
	while (r > 0) {
		fatWriteFile(&fat, &fatfile, cpbuf, r, &rt);
		if (rt != r) {
			fprintf(stderr, "Failed to write on vhd %s\n", buf);
			exit(-10);
		}
		r = fread(cpbuf, 1, 512, fp);
	}
	fclose(fp);
	fclose(vhd);
	/*fprintf(stderr, "Copied %s to %s / %s (bytes %d)\n", argv[1], buf, path, i);*/
	fprintf(stderr, "Copied %s\n", argv[1]);
	return 0;
}


