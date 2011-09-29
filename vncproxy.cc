#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <pthread.h>

#include "config.h"

#include "d3des.h"


static inline bool streq(const char* str1, const char* str2) {
    return strcmp(str1, str2) == 0;
}


static void print_version() {
    printf("vncproxy version %s\n", VERSION);
}


static void print_help() {
    printf("usage: vncproxy start HOST:PORT\n");
    printf("           Start vncproxy service, will serve on HOST:PORT\n");
    printf("\n");
    printf("       vncproxy stop\n");
    printf("           Stop currently running vncproxy service\n");
    printf("\n");
    printf("       vncproxy restart [-b HOST] [-p PORT]\n");
    printf("           Restart currently running vncproxy, will serve on HOST:PORT\n");
    printf("\n");
    printf("       vncproxy list\n");
    printf("           List all current mappings\n");
    printf("\n");
    printf("       vncproxy add NAME HOST:PORT\n");
    printf("           Add a mapping named NAME to HOST:PORT\n");
    printf("\n");
    printf("       vncproxy remove NAME\n");
    printf("           Remove a mapping named NAME\n");
    printf("\n");
}


static int start_server() {
    printf("This shall be done!\n");
    return 0;
}


static int stop_server() {
    printf("This shall be done!\n");
    return 0;
}

static int list_mapping() {
    printf("This shall be done!\n");
    return 0;
}


static int add_mapping() {
    printf("This shall be done!\n");
    return 0;
}


static int remove_mapping() {
    printf("This shall be done!\n");
    return 0;
}



int main(int argc, char* argv[]) {
    if (argc <= 1) {
        print_help();
        exit(0);
    }
    
    for (int i = 0; i < argc; i++) {
        if (streq(argv[i], "-h") || streq(argv[i], "--help") || streq(argv[i], "help")) {
            print_help();
            exit(0);
            
        } else if (streq(argv[i], "--version")) {
            print_version();
            exit(0);
        }
    }
    
    char* cmd = argv[1];
    if (streq(cmd, "start")) {
        start_server();
        
    } else if (streq(cmd, "stop")) {
        stop_server();
        
    } else if (streq(cmd, "restart")) {
        stop_server();
        start_server();
        
    } else if (streq(cmd, "list")) {
        list_mapping();
        
    } else if (streq(cmd, "add")) {
        add_mapping();
        
    } else if (streq(cmd, "remove")) {
        remove_mapping();
        
    } else {
        printf("vncproxy: '%s' is not a command. See 'vncproxy --help'.\n", cmd);
        
    }
    
    return 0;
}
