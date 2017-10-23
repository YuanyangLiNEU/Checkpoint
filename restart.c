#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <ctype.h>
#include <memory.h>
#include <unistd.h>

char ckptImg[100];

void restoreMemory();
void parseImg();
void unmapStack();

typedef struct memoryRegion
{
	void *startAddr;
	void *endAddr;
	unsigned long size;
	int isReadable;
	int isWriteable;
	int isExecutable;

	//indicate whether the memoryRegion is Context
	int isContext;
} MemoryRegion;

int main(int args, char* argv[]) {
	if (args < 2) {
		printf("lack arguements\n");
	}

	strcpy(ckptImg, argv[1]);
	void* stack_ptr = (void*)strtoul("05310000", NULL, 16);
	mmap(stack_ptr, 0x100000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	stack_ptr += 0x100000;
	asm volatile ("mov %0,%%rsp" : : "g" (stack_ptr) : "memory");
	unmapStack();
	restoreMemory();

	return 0;
}

void restoreMemory() {
	printf("In restoreMemory\n");
	parseImg();
}

void unmapStack() {
	FILE* fd = fopen("/proc/self/maps", "r");
	char* line = NULL;
	size_t len = 0;
	ssize_t read;
	
	char stackWord[] = "stack";
	void* startAddr;
	unsigned long memSize;
	while ((read = getline(&line, &len, fd)) != -1) {
		//printf("%s", line);
		if (strstr(line, stackWord) != NULL) {
			char perm[5], devId[6], name[512];
			unsigned long start, end, inode, offset;
			sscanf(line, "%lx-%lx %4s %lx %5s %lx %s", &start, &end, perm, &offset, devId, &inode, name);
			startAddr = (void *)start;
			memSize = (void *)end - startAddr;
			
			break;
		}	
	}
	fclose(fd);

	printf("start exec unmap. start address:%p, mem size:%lx\n", startAddr, memSize);
	int ret = -1;
	if ((ret = munmap(startAddr, memSize)) < 0) {
		printf("Error in unmap.\n");
	}

	printf("end ummap.\n");
}

void parseImg() {
	int fdr = open(ckptImg, O_RDONLY);
	if (fdr < 0) {
		printf("Error in open ckptImg\n");
	}

	// read memory
	MemoryRegion mr;
	ucontext_t uct;
	while (read(fdr, &mr, sizeof(mr)) > 0) {
		//printf("start:%p, end:%p, size:%lx, r:%d, w:%d, x:%d\n", 
		//	mr.startAddr, mr.endAddr, mr.size, mr.isReadable, mr.isWriteable, mr.isExecutable);	

		if (mr.isContext == 1) {
			if (read(fdr, &uct, sizeof(uct)) < 0) {
				printf("Error when read context\n");
			}
			break;
		}

		int prot = PROT_WRITE;
		if (mr.isReadable == 1) {
			prot |= PROT_READ;
		}
		if (mr.isWriteable == 1) {
			prot |= PROT_WRITE;
		}
		if (mr.isExecutable == 1) {
			prot |= PROT_EXEC;
		}

		mmap(mr.startAddr, mr.size, prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

		if (read(fdr, mr.startAddr, mr.size) < 0) {
			//printf("Error when restore memory\n");
		}
	}
	close(fdr);

	printf("begin restore context.\n");
	int ret = -1;
	if ((ret = setcontext(&uct)) < 0) {
		printf("Error in set context.\n");
	}
	printf("set context ret:%d\n", ret);
}

