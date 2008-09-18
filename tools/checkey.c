#include <stdio.h>
#include <sys/select.h>

int main(int argc, char **argv)
{
	fd_set f;
	struct timeval t = {0, 200*1000L};

	FD_ZERO(&f);
	FD_SET(0,&f );
	return select( 1, &f, NULL, NULL, &t );
}
