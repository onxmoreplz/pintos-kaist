#include <syscall.h>

int main(int, char *[]);
void _start(int argc, char *argv[]);

void _start(int argc, char *argv[]) //User Program 이 시작되는 부분
{
	exit(main(argc, argv));
}
