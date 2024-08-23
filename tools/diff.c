/******************************************************************************
 *                       OS-3o3 operating system
 * 
 *               project builder using .vscode/tasks.diff
 *
 *            19 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/
/*
 * https://github.com/mirror/busybox/blob/master/editors/diff.c
 * https://github.com/paulgb/simplediff/blob/master/php/simplediff.php
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#include <unistd.h>
#else
#include <unistd.h>
#include <wait.h>
#endif

/*
 * types definition
 */
typedef long os_intn;
typedef unsigned long os_size;
typedef unsigned int os_uint32;
typedef unsigned char os_utf8;
typedef char os_bool;
typedef int os_result;


/**
 * load a full file in RAM
 * \param path file name
 * \param size return size of the file
 * \returns an alloced buffer (use free() to release it) containing the contenet of the file
 */
os_utf8 *diff__load_file(os_utf8 *path, os_size *size)
{
	FILE *fp;
	os_utf8 *buf;
	os_size ret;

	fp = fopen((char *)path, "rb");
	if (!fp)
	{
		fprintf(stderr, "ERROR: cannot open %s\n", path);
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	*size = (os_size)ftell(fp);
	if (*size < 1)
	{
		fprintf(stderr, "ERROR: %s is empty\n", path);
		return 0;
	}
	fseek(fp, 0, SEEK_SET);
	buf = malloc(*size + 1);
	ret = fread(buf, 1, *size, fp);
	if (ret != *size)
	{
		fprintf(stderr, "ERROR: read fail on %s\n", path);
		return 0;
	}
	fclose(fp);
	buf[*size] = '\0';
	return buf;
}

/**
 * save buffer in file
 * \param path name of the file
 * \param size length of buf
 * \param buf buffer
 * \returns 0 on success
 */
os_result diff__save_file(os_utf8 *path, os_size size, os_utf8 *buf)
{
	FILE *fp;
	os_size ret;
	fp = fopen((char *)path, "wb");
	if (!fp)
	{
		fprintf(stderr, "ERROR: cannot open %s\n", path);
		return -1;
	}

	ret = fwrite(buf, 1, size, fp);
	if (ret != size)
	{
		fprintf(stderr, "ERROR: write fail on %s\n", path);
		return -1;
	}
	fclose(fp);
	return 0;
}


/**
 * main entry point
 */
int main(int argc, char *argv[])
{
	os_utf8 *diff;
	os_size size;

	if (argc != 3)
	{
		fprintf(stderr, "usage: %s <old> <new>\n", argv[0]);
		exit(-1);
	}
	diff = diff__load_file(path, &size);
	if (!diff)
	{
		exit(-1);
	}
	free(diff);
	return 0;
}
