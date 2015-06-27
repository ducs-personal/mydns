#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv)
{
	char host[32];
	char uri[56];
	sscanf(argv[1],"http://%[^/]%s",host,uri);
	puts(host);
	puts(uri);

char* s = "iios/12DDWDFF/122";
char buf[20];
sscanf( s, "%*[^/]/%[^/]", buf );
printf( "%s\n", buf );

return 0;

}
