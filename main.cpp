#include <iostream>
#include "tomo_tiff.h"
#include <cstring>
#include <cstdlib>
#include <unistd.h>

using namespace std;

int main(int argc, char **argv){
    if(argc < 2){
        return -1;
    }

    tomo_super_tiff sample((char*)argv[1]);
    sample.neuron_detection(5);
    cout << "saveing measure..." <<endl;
    sample.save_measure("result");
    cout << "saveing merged measure..." <<endl;
    sample.save_measure_merge("result_merged");

    return 0;
}
