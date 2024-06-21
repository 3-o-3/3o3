/*
 *                        OS-3o3 Operating System
 *
 *                        starts a virtual machine
 * 
 *                      13 may MMXXIV PUBLIC DOMAIN
 *           The authors disclaim copyright to this source code.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
#ifdef _WIN32
	if (0) return system("call .\\qemupi.cmd");
	if (0) return system("call .\\32gears.vmx");
    if (1) return system("call .\\qemu64.cmd");
	return system("call .\\64gears.vmx");
#else
	return system("bash ./qemu64.cmd");
#endif
}