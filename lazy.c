#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

typedef struct _promise {
	void *data;
	struct _promise *(*fn)(void *);
	void *args;
} promise_t;

typedef struct _list {
	void *elem;
	void *next;
} list_t;

void *
xmalloc(size_t size)
{
	void *p;
	
	p = memalign(4, size);
	assert(p);

	return p;
}

#define GET_PTR(x) ((void *)((unsigned int)(x)&0xfffffffc))
#define GET_TAG(x) ((unsigned int)(x)&0x3)
#define SET_TAG(x, tag) do { unsigned int __a = (unsigned int)x; __a |= tag; x = (void *)__a; } while (0)
#define TAG_PROMISE 1
#define TAG_GENERATOR 2

promise_t *
promise(void *fn, void *args)
{
	promise_t *p;

	p = xmalloc(sizeof(*p));

	p->data = NULL;
	p->fn = fn;
	p->args = args;

	SET_TAG(p, TAG_PROMISE);

	return p;
}

void *
force(promise_t *_p)
{
	promise_t *p = GET_PTR(_p);

	assert(GET_TAG(_p) == TAG_PROMISE);

	if (p->data) {
		if (GET_TAG(p->data) == TAG_PROMISE)
			return force(p->data);
		return p->data;
	}

	p->data = (p->fn)(p->args);
	if (GET_TAG(p->data) == TAG_PROMISE) {
		return force(p->data);
	}

	return p->data;
}

list_t *
real_cons(void *_args)
{
	void **args = _args;
	list_t *l;
	
	l = xmalloc(sizeof(*l));
	l->elem = args[0];
	l->next = args[1];

	return l;
}

promise_t *
cons(void *elem, void *next)
{
	void **args;
	promise_t *p;

	args = xmalloc(2 * sizeof(void *));
	args[0] = elem;
	args[1] = next;
	p = promise(real_cons, args);

	return p;
}

promise_t *integer_list_generator(int start);


void *
integer_list_generator_worker(void *current)
{
	return cons(current, integer_list_generator((int)current + 1));
}

promise_t *
integer_list_generator(int start)
{
	return promise(integer_list_generator_worker, (void *)start);
}

int
main(int argc, char **argv)
{
	int i;
	promise_t *g = integer_list_generator(5);
	promise_t *g2 = g;

	for (i = 0; i < 5; i++) {
		int x;
		list_t *l = force(g2);

		x = (int)l->elem;
		printf("Value: %d\n", x);
		g2 = l->next;
	}

	g2 = g;
	for (i = 0; i < 5; i++) {
		int x;
		list_t *l = force(g2);

		x = (int)l->elem;
		printf("Value: %d\n", x);
		g2 = l->next;
	}

	return 0;
}
