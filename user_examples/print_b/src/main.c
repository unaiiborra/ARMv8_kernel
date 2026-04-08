#include <stddef.h>
#include <stdio.h>

int main()
{
    for (size_t i = 0; i < 10; i++) {
        print("Hello from process B\n\r");
    }

    return 0;
}