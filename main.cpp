#include <iostream>
#include "tomo_tiff.h"

using namespace std;

int main(int argc, char **argv){
    if(argc < 2){
        return -1;
    }

    tomo_super_tiff sample((char*)argv[1]);
    sample.neuron_detection(50);
    cout << "it works" <<endl;
    return 0;
}
