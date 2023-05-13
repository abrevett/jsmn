#include "jasmine.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

static inline void *realloc_it(void *ptrmem, size_t size) {
    void *p = realloc(ptrmem, size);
    if (!p) {
        free(ptrmem);
        fprintf(stderr, "realloc(): errno=%d\n", errno);
    }
    return p;
}

const static char* INPUT_FILE="test_large_8M.json";

void jsmn_print_tokens(jsmntok_t* tokens, int num_tokens){
    for(int i=0; i<num_tokens; i++){
        printf("type:%d, start:%d, end:%d\n", 
        tokens[i].type, tokens[i].start, tokens[i].end);
    }
}

jsmn_meta* jsmn_init_par(int num_thr, const char* file){
    if( !num_thr ) return NULL;
    FILE* fp; fp = fopen(file, "r");
    fseek(fp, 0, SEEK_END); long long int flen = ftell(fp);  // get file length
    fclose(fp);
    int foff = (int)(flen / num_thr);

    jsmn_parser *parsers = calloc(num_thr, sizeof(jsmn_parser));     // Parser list
    FILE** scanners = calloc(num_thr, sizeof(FILE*));
    jsmn_meta *metadata = calloc(num_thr, sizeof(jsmn_meta));

    for(int i=0; i<num_thr; i++){
        // Initialize every jsmn parser
        jsmn_init( &parsers[i] );
        parsers[i].start = parsers[i].end = i*foff;
#ifdef JSMN_PARALLEL
        sem_init( &(parsers[i].lock), 0, 1);
#endif
        // Initializing FILE pointers
        scanners[i] = fopen( file, "r" ); fseek(scanners[i], i*foff, SEEK_SET);
        printf("Thread %d starts @%ld\n", i, ftell(scanners[i]));
        // moving over pointers
        metadata[i].scanner = scanners[i]; metadata[i].parser = &parsers[i];
        if(i < num_thr-1) metadata[i].next = &metadata[i+1];
    }

    for(int i=0; i< num_thr; i++){
	printf("Thread");
    }

    return metadata;
}

// run jsondump code in parallel here
void* jsmn_parse_par(void* arg){
    jsmn_meta *toolkit = (jsmn_meta*) arg;
    int r;
    int eof_expected = 0;
    char *js = NULL;
    size_t jslen = 0;
    char buf[BUFSIZ];

    jsmn_parser *p = toolkit->parser;
    jsmntok_t *tok;
    size_t tokcount = 2;
    FILE* input = toolkit->scanner;
#ifdef JSMN_PARALLEL
    unsigned int* next_start = toolkit->next? &(toolkit->next->parser->start) : NULL;
    unsigned int* next_end = toolkit->next? &(toolkit->next->parser->end) : NULL;
    sem_t* next_parser_lock = toolkit->next? &(toolkit->next->parser->lock) : NULL;
    sem_t* parser_lock = &(toolkit->parser->lock);

    // We now move the scanner to the next available '{'
    sem_wait(parser_lock);
    char c; 
    while( p->start < p->end ){
        c = fgetc(input);
        if(c != '{'){
            p->start++;
        }
        else{
            ungetc(c, input);
            p->start--;
        }
    }
    sem_post(parser_lock);
#endif 

    /* Allocate some tokens as a start */
    tok = toolkit->tokens = malloc(sizeof(*tok) * tokcount);
    if (tok == NULL) {
        fprintf(stderr, "malloc(): errno=%d\n", errno);
        return (void*)3;
    }

    for (;;) {
#ifdef JSMN_PARALLEL
        //TODO: we need to skip over previously-parsed tokens; increment counters.
        if( next_start ){
            sem_wait(next_parser_lock);
            if( *next_start && jslen >= *next_start ){
                int offset = *next_end - *next_start;
                fseek(input, offset, SEEK_CUR); // we fast forward our buffered reader
                printf("start: %d, end: %d\n", *next_start, *next_end);
                printf("hit boundary, now at %ld\n", ftell(input));
            }
            sem_post(next_parser_lock);
        }
#endif

      /* Read another chunk */
        r = fread(buf, 1, sizeof(buf), input);
        if (r < 0) {
            fprintf(stderr, "fread(): %d, errno=%d\n", r, errno);
            return (void*)1;
        }
        if (r == 0) {
            if (eof_expected != 0) {
                return (void*)0;
            } else {
                    fprintf(stderr, "fread(): unexpected EOF\n");
                    return (void*)2;
            }
        }
#ifdef JSMN_PARALLEL
        sem_wait(parser_lock);
#endif
	printf("Thread %ld: parsing buffer ' %s '\n", pthread_self(), buf);
#ifdef JSMN_PARALLEL
        sem_post(parser_lock);
#endif

        js = realloc_it(js, jslen + r + 1);
        if (js == NULL) {
            return (void*)3;
        }
        strncpy(js + jslen, buf, r);
        jslen = jslen + r;

    again:
#ifdef JSMN_PARALLEL
        sem_wait(parser_lock);
#endif
        r = jsmn_parse(toolkit, js, jslen, tok, tokcount);
#ifdef JSMN_PARALLEL
        sem_post(parser_lock);
#endif
        if (r < 0) {
            if (r == JSMN_ERROR_NOMEM) {
            tokcount = tokcount * 2;
            tok = realloc_it(tok, sizeof(*tok) * tokcount);
            if (tok == NULL) {
                return (void*)3;
            }
            goto again;
            }
        } else {
            toolkit->tokens = tok; // We get all the tokens at the end
            eof_expected = 1;
        }
#ifdef JSMN_PARALLEL
        sem_wait(parser_lock);
#endif
        p->end = p->start + p->pos;
#ifdef JSMN_PARALLEL
        printf("Thread %ld: endpoint: %d\n", pthread_self(), p->end );
        sem_post(parser_lock);
#endif
    }

    return EXIT_SUCCESS;
}
