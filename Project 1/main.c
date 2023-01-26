//
// A multi-thread streaming compression demo using ZSTD_compressStream2
//
// references:
//
// documentation: http://facebook.github.io/zstd/zstd_manual.html
// example: https://github.com/facebook/zstd/blob/dev/examples/
//             streaming_compression.c

#include <stdio.h>          // printf
#include <stdlib.h>         // free
#include <string.h>         // memset, strcat, strlen
#include <zstd.h>           // presumes zstd library is installed
#include <pthread.h>

#define BUF_SIZE 16*1024    // 16KiB

int nThreads = 8;          // Ddfault thread number 8

// marco functions
//
// CHECK_ZSTD
// Check the zstd error code and die if an error occurred after printing a
// message.

#define CHECK(cond, ...)                        \
    do {                                        \
        if (!(cond)) {                          \
            fprintf(stderr,                     \
                    "%s:%d CHECK(%s) failed: ", \
                    __FILE__,                   \
                    __LINE__,                   \
                    #cond);                     \
            fprintf(stderr, "" __VA_ARGS__);    \
            fprintf(stderr, "\n");              \
            exit(1);                            \
        }                                       \
    } while (0)

#define CHECK_ZSTD(fn)                                           \
    do {                                                         \
        size_t const err = (fn);                                 \
        CHECK(!ZSTD_isError(err), "%s", ZSTD_getErrorName(err)); \
    } while (0)

// stream compress argument struct
typedef struct compress_args
{
  const char *fname;
  char *outName;
  int cLevel;

} compress_args_t;

static void *streamCompress(void *data)
{

    compress_args_t *args = (compress_args_t *)data;
    
    FILE* const fin  = fopen(args->fname, "rb");
    FILE* const fout = fopen(args->outName, "wb");
    void*  const buffIn  = malloc(BUF_SIZE);
    void*  const buffOut = malloc(BUF_SIZE);

    // create the context
    ZSTD_CCtx* const cctx = ZSTD_createCCtx();
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, args->cLevel);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, nThreads);

    // compress chunk by chunk
    for (;;) {
        size_t read = fread(buffIn, 1, BUF_SIZE, fin);
        int const lastChunk = (read < BUF_SIZE);
        ZSTD_EndDirective const mode = lastChunk ? ZSTD_e_end : ZSTD_e_continue;
        ZSTD_inBuffer input = { buffIn, read, 0 };
        int finished;

        do {
            ZSTD_outBuffer output = { buffOut, BUF_SIZE, 0 };
            size_t const remaining = ZSTD_compressStream2(cctx, &output , &input, mode);
            CHECK_ZSTD(remaining);
            fwrite(buffOut, 1, output.pos, fout);

            finished = lastChunk ? (remaining == 0) : (input.pos == input.size);
        } while (!finished);

        if (lastChunk)
            break;
    }

    ZSTD_freeCCtx(cctx);
    fclose(fout);
    fclose(fin);
    free(buffIn);
    free(buffOut);
    free(args->outName);

    return NULL;
}


static char* createOutFilename(const char* filename)
{
    size_t const inL = strlen(filename);
    size_t const outL = inL + 5;
    void* const outSpace = malloc(outL);
    memset(outSpace, 0, outL);
    strcat(outSpace, filename);
    strcat(outSpace, ".zst");
    return (char*)outSpace;
}

int main(int argc, const char** argv)
{
    const char* const exeName = argv[0];

    if (argc!=4) {
        printf("Bad arguments\n");
        printf("usage:\n");
        printf("%s THREADS LEVEL FILE\n", exeName);
        return 1;
    }

    nThreads = atoi (argv[1]);
    if (nThreads<=0) {
        perror("Bad number of threads.\n");
        exit(1);
    }
        
    // compression level can be negative
    int level = atoi (argv[2]);

    compress_args_t args;
    args.fname = argv[3];
    args.outName = createOutFilename(args.fname);
    args.cLevel = level;

    streamCompress(&args);

    return 0;
}