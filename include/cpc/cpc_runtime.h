/* 
Copyright (c) 2008, 2009 by Gabriel Kerneis.
Copyright (c) 2004, 2005 by Juliusz Chroboczek.
Experimental; do not redistribute.
*/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define EV_STANDALONE 1
#include "ev.h"

struct cpc_continuation;
typedef void cpc_function(void*);
typedef struct cpc_condvar cpc_condvar;

typedef struct cpc_continuation {
    struct cpc_continuation *next;
    struct cpc_condvar *condvar;
    struct cpc_continuation *cond_next;
    struct cpc_continuation **ready;
    union ev_any_watcher watcher;
    int state;
    unsigned short length;
    unsigned short size;
    char c[1];
} cpc_continuation;

void cpc_continuation_free(struct cpc_continuation *c);
struct cpc_continuation *cpc_continuation_expand(struct cpc_continuation *c,
                                                 int n);
struct cpc_continuation *cpc_continuation_copy(struct cpc_continuation *c);

static void
print_continuation(struct cpc_continuation *c, char *s)
{
    int i;
    fprintf(stderr,"(%s) At %p %u:\n%p\t%p\t%p\t%p\t%d\t%u\t%u\n",s,c,
    sizeof(cpc_function*),
    c->next,c->condvar,c->cond_next,c->ready,c->state,c->length,c->size);
    for(i=0; i < c->length; i++)
        fprintf(stderr,"%d ",c->c[i]);
    fprintf(stderr,"\n");
}

static inline void* 
cpc_alloc(struct cpc_continuation **cp, int s)
{
    struct cpc_continuation *c;
    void *p;

    c = *cp;
    if(c == (void*)0 || s > c->size - c->length)
        c = cpc_continuation_expand(c, s);
    p = c->c + c->length;
    c->length += s;
    *cp = c;
    return p;
}

static inline void*
cpc_dealloc(struct cpc_continuation *c, int s)
{
    c->length -= s;
    return c->c + c->length;
}

static void
cpc_really_invoke_continuation(struct cpc_continuation *c)
{
    extern void abort();
    extern void free();

    cpc_function *f;

    if(c == (void*)0)
        return;

    if(c->length == 0) {
        cpc_continuation_free(c);
        return;
    }

    c->length -= sizeof(cpc_function*);
    f = *(cpc_function**)(c->c + c->length);
    (*f)(c);
}

#ifdef CPC_TAIL_RECURSIVE_COMPILER
#define cpc_invoke_continuation cpc_really_invoke_continuation
#else
static inline void
cpc_invoke_continuation(struct cpc_continuation *c)
{
    assert(c->ready);
    *(c->ready) = c;
    return;
}
#endif

#define CPC_IO_IN EV_READ
#define CPC_IO_OUT EV_WRITE

static inline struct cpc_continuation *
cpc_continuation_push(cpc_continuation *c, cpc_function *f)
{
    if(c == (void*)0 || sizeof(cpc_function*) > c->size - c->length)
        c = cpc_continuation_expand(c, sizeof(cpc_function*));

    *(cpc_function**)(c->c + c->length) = f;
    c->length += sizeof(cpc_function*);
    return c;
}

static inline void
cpc_continuation_patch(cpc_continuation *cont, size_t size, void *value)
{
  void *cpc_arg;
  cpc_arg =
    ((cont)->c + (cont)->length - sizeof(cpc_function*) - size);
  memcpy(cpc_arg, value, size);
  return;
}

void cpc_schedule(struct cpc_continuation *cont);
extern void cpc_main_loop(void);
extern void cpc_prim_sleep(int, int, cpc_condvar*, cpc_continuation*);
cpc_condvar *cpc_condvar_get(void);
cpc_condvar *cpc_condvar_retain(cpc_condvar*);
void cpc_condvar_release(cpc_condvar*);
void cpc_prim_wait(cpc_condvar*, cpc_continuation*);
void cpc_signal(cpc_condvar*);
void cpc_signal_all(cpc_condvar*);
int cpc_condvar_count(cpc_condvar*);
void cpc_prim_io_wait(int, int, cpc_condvar*, cpc_continuation*);

void cpc_prim_attach(cpc_continuation*);
void cpc_prim_detach(cpc_continuation*);