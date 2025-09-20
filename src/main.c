#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main()
{
    char *user = getenv("USER");
    char *machine = getenv("MACHINE");
    char *pwd = getenv("PWD");

    printf("%s@%s:%s>", user, machine, pwd);

    return 0;
}
