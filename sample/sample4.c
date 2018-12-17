#include <stdio.h>
#include <stdint.h>

typedef uint64_t u;

u _ssharp_write(u _input) {
	 printf("%lu\n", _input);
}
u _ssharp_read() {
	u _tmp;
	scanf("%lu", &_tmp);
	return _tmp;
}
int main() {
	 return (_ssharp_write((_ssharp_write(3),4)));
}
