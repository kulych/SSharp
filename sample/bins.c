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
u _ssharp_bins(u _ssharp_l,u _ssharp_r,u _ssharp_ans) {
	 return ((((_ssharp_l==(_ssharp_r-1))) ? 
	(_ssharp_write(_ssharp_l))
	 : ((((_ssharp_ans==0)) ? 
	(_ssharp_write(((_ssharp_l+((_ssharp_l+_ssharp_r)/2))/2)),_ssharp_bins(_ssharp_l,((_ssharp_l+_ssharp_r)/2),_ssharp_read()))
	 : ((((_ssharp_ans==1)) ? 
	(_ssharp_write(((_ssharp_r+((_ssharp_l+_ssharp_r)/2))/2)),_ssharp_bins(((_ssharp_l+_ssharp_r)/2),_ssharp_r,_ssharp_read()))
	 : ((((_ssharp_ans==2)) ? 
	(_ssharp_write(((_ssharp_l+_ssharp_r)/2)))
	 : (_ssharp_write(9999))))))))));
}
int main() {
	 return (_ssharp_write(50),_ssharp_bins(1,101,_ssharp_read()));
}
