#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

void sig12Handler();
__attribute__ ((constructor))
void myconstructor() {
	signal(SIGUSR2, sig12Handler);
}

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

void parseMR(char*, MemoryRegion*);
void write2File(int, MemoryRegion*);
void writeContext2File(int, ucontext_t*);

//int main(int args, char* argv[]) {
void sig12Handler() {
	printf("begin checkpoint.\n");

	FILE* fdr = fopen("/proc/self/maps", "r");
	int fdw = open("./myckpt", O_RDWR | O_APPEND | O_CREAT, 0666);
	char* line = NULL;
	size_t len = 0;
	ssize_t read;
	MemoryRegion mr;

	while ((read = getline(&line, &len, fdr)) != -1) {
		//printf("%s", line);
		parseMR(line, &mr);
		mr.isContext = 0;
		write2File(fdw, &mr);
	}

	ucontext_t context;
	if (getcontext(&context) < 0) {
		printf("Error in getcontext.\n");
	}
	printf("Restore process from here.\n");
	writeContext2File(fdw, &context);
	
	fclose(fdr);
	close(fdw);
}

void writeContext2File(int fd, ucontext_t* context) {
	MemoryRegion mr;
	mr.startAddr = NULL;
	mr.endAddr = NULL;
	mr.isReadable = 0;
	mr.isWriteable = 0;
	mr.isExecutable = 0;
	mr.isContext = 1;
	mr.size = sizeof(*context);

	if (write(fd, &mr, sizeof(mr)) < 0) {
		//printf("write error in context header\n");
	}
	if (write(fd, context, sizeof(*context)) < 0) {
		//printf("write error in context body\n");
	}
}

//08048000-08053000 r-xp 00000000 08:01 655385     /bin/cat
void parseMR(char *str, MemoryRegion* mr) {
	char perm[5], devId[6], name[512];
	unsigned long start, end, inode, offset;
	sscanf(str, "%lx-%lx %4s %lx %5s %lx %s", &start, &end, perm, &offset, devId, &inode, name);
	mr->startAddr = (void*)start;
	mr->endAddr = (void*)end;

	mr->isReadable = perm[0] == 'r' ? 1 : 0;
	mr->isWriteable = perm[1] == 'w' ? 1 : 0;
	mr->isExecutable = perm[2] == 'x' ? 1 : 0;
	mr->size = mr->endAddr - mr->startAddr;
	//printf("start:%p, end:%p, size:%lu, r:%d, w:%d, x:%d\n", 
	//	mr->startAddr, mr->endAddr, mr->size, mr->isReadable, mr->isWriteable, mr->isExecutable);	
}

//write memory to given file
void write2File(int fd, MemoryRegion* mr) {
	//printf("%d\n", mr->endAddr - mr->startAddr);
	if (mr->isReadable == 0) {
		return ;
	}
	if (write(fd, mr, sizeof(*mr)) < 0) {
		//printf("write error\n");
	}

	if (write(fd, mr->startAddr, mr->size) < 0) {
		//printf("write error\n");
	}
	//printf("%d\n", strlen(mem));
}
