bins l r ans {
	if(l==r-1) {
		write(l)
	} {
		if (ans == 0) {
			write((l+(l+r)/2)/2);
			bins(l, (l+r)/2, read())
		} {
			if (ans == 1) {
				write((r+(l+r)/2)/2);
				bins((l+r)/2, r, read())
			}
			{
				if (ans == 2) {
					write((l+r)/2)
				} {
					write(9999)
				}
			}
		}
	}
}
main {
	write(51);
	bins(1, 101, read())
}
