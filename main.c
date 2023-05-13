#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define JSMN_HEADER

#include "jasmine.h"


int main( void ){
    const char* input = "library.json";
//    const char* input = "test-large-8M.json";
    const int num_thr = JSMN_PARA_THR;

    // TODO: run parallel jsmn code:
    jsmn_meta* metadata = jsmn_init_par(num_thr, input);
    if( !metadata ) return EXIT_FAILURE;
    pthread_t *jsmn_threads = calloc(num_thr, sizeof(pthread_t));       // Thread list
    
    for(int i=0; i<num_thr; i++){
        pthread_create( &jsmn_threads[i], NULL, jsmn_parse_par, &metadata[i] );
    }
    for(int i=0; i<num_thr; i++){
        int ret = (int) pthread_join( jsmn_threads[i], NULL );
        printf("\tThread %ld: (%d) [ %d, %d ] \n",
            jsmn_threads[i], ret, metadata[i].parser->start, metadata[i].parser->end);
        jsmn_print_tokens( metadata[i].tokens, metadata[i].parser->toknext );
        printf("\n");
    }
    printf("\tThread %ld: [ %d, %d ] \n",
        jsmn_threads[0],  metadata[0].parser->start, metadata[0].parser->end);
    jsmn_print_tokens( metadata[0].tokens, metadata[0].parser->toknext );
    printf("\n");
}
