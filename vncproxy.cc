#include <iostream>
#include <pthread.h>

#include "config.h"

#include "d3des.h"

using namespace std;

pthread_mutex_t vnc_mapping_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {
    cout << "This shall be done!" << endl;
    return 0;
}
