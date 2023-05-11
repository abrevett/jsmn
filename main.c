#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define JSMN_HEADER

#include "jasmine.h"


int main( void ){
    const char* input = "library.json";
    const int num_thr = JSMN_PARA_THR;

    // TODO: run parallel jsmn code:
    jsmn_meta* metadata = jsmn_init_par(num_thr, input);
    if( !metadata ) return EXIT_FAILURE;
    pthread_t *jsmn_threads = calloc(num_thr, sizeof(pthread_t));       // Thread list
    
    for(int i=0; i<num_thr; i++){
        pthread_create( &jsmn_threads[i], NULL, jsmn_parse_par, &metadata[i] );
    }
    for(int i=0; i<num_thr; i++){
        pthread_join( jsmn_threads[i], NULL );
        printf("\tThread %d:\n", i);
        jsmn_print_tokens( metadata->tokens, metadata->parser->toknext );
        printf("\n");
    }
}