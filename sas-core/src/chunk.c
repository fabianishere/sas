#include <sas/chunk.h>

void sas_chunk_dealloc(struct sas_chunk *chunk)
{
    chunk->dealloc(chunk);
}
