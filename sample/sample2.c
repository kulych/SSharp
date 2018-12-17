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
u _ssharp_fun(u _ssharp_x) {
	 return (_ssharp_write(_ssharp_x),(((_ssharp_x>0)) ? 
	(_ssharp_fun((_ssharp_x-1)))
	 : (0)));
}
int main() {
	 return (_ssharp_fun(10));
}
