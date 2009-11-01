#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

typedef struct _promise {
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
#define TAG_DATA	0
#define TAG_PROMISE	1
#define TAG_SCALAR	2

void *
scalar(unsigned int value)
{
	void *v;
	
	v = (void *)(value << 2);
	SET_TAG(v, TAG_SCALAR);

	return v;
}

unsigned int
scalar_value(void *_v)
{
	unsigned int v = (unsigned int)GET_PTR(_v);

	assert(GET_TAG(_v) == TAG_SCALAR);

	return v>>2;
}

promise_t *
promise(void *fn, void *args)
{
	promise_t *p;

	p = xmalloc(sizeof(*p));

	p->fn = fn;
	p->args = args;

	SET_TAG(p, TAG_PROMISE);

	return p;
}

void *
force(promise_t *_p)
{
	void *data;
	promise_t *p = GET_PTR(_p);

	assert(GET_TAG(_p) == TAG_PROMISE);

	if (!p->fn)
		return p->args;

	data = (p->fn)(p->args);
	
	p->fn = NULL;
	while (GET_TAG(data) == TAG_PROMISE)
		data = force(data);

	if (GET_TAG(data) == TAG_SCALAR)
		data = (void *)scalar_value(data);

	p->args = data;
	return data;
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

void *
square_promise(void *args)
{
	unsigned int x = (unsigned int)args;

	x = x*x;
	
	return scalar(x);
}

promise_t *
square(int value)
{
	return promise(square_promise, (void *)value);
}

int
main(int argc, char **argv)
{
	int i;
	promise_t *s;
	promise_t *g = integer_list_generator(5);
	promise_t *g2 = g;

	for (i = 0; i < 5; i++) {
		int x;
		list_t *l = force(g2);

		x = (int)l->elem;
		printf("Value: %d\n", x);
		assert(x == i+5);
		g2 = l->next;
	}

	g2 = g;
	for (i = 0; i < 5; i++) {
		int x;
		list_t *l = force(g2);

		x = (int)l->elem;
		printf("Value: %d\n", x);
		assert(x == i+5);
		g2 = l->next;
	}

	s = square(5);
	printf("%d\n", (unsigned int)force(s));
	printf("%d\n", (unsigned int)force(s));

	return 0;
}
