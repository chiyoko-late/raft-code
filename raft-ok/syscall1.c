#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char c;
    while (read(STDIN_FILENO, &c, 1) != 0)
    {
        write(STDOUT_FILENO, &c, 1);
    }
    exit(0);
}

// int main(void)
// {
//     char c;
//     int fdo;
//     fdo = open("syscall.txt", (O_CREAT | O_APPEND | O_RDWR));
//     while (read(fdo, &c, 1) != 0)
//     {
//         write(STDOUT_FILENO, &c, 1);
//     }
//     exit(0);
// }