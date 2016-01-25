#include <iostream>
#include "tomo_tiff.h"

using namespace std;

int main()
{
    tomo_tiff test("data/tomo_0036.tif");
    test.save("data/test.tif");
    return 0;
}

