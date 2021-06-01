#include <stdio.h>
#include <stdlib.h>

static void idmapping_test()
{
    int len = 0;
    int nums[2];
    nums[0] = snprintf(NULL, 0, "%d %d %d\n", 0, 111, 1000);
    len += nums[0];
    nums[1] = snprintf(NULL, 0, "%d %d %d\n", 1000, 1000, 100);
    len += nums[1];

    char *mapping = calloc(len + 1, sizeof(char));
    int n = 0;
    snprintf(mapping + n, nums[0] + 1, "%d %d %d\n", 0, 111, 1000);
    n += nums[0];
    snprintf(mapping + n, nums[1] + 1, "%d %d %d\n", 1000, 1000, 100);
    n += nums[1];

    printf("%s\n", mapping);
}