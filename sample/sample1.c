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
u _ssharp_test(u _ssharp_u,u _ssharp_v) {
	 return (((_ssharp_u+_ssharp_v)+(_ssharp_u*_ssharp_v)));
}
u _ssharp_fact(u _ssharp_x) {
	 return ((_ssharp_x*(((_ssharp_x>1)) ? 
	(_ssharp_fact((_ssharp_x-1)))
	 : (1))));
}
int main() {
	 return (_ssharp_write(_ssharp_test(_ssharp_read(),_ssharp_read())),_ssharp_write(_ssharp_fact(5)),0);
}
