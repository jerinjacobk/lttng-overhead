#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_malloc.h>
#include <rte_lcore.h>

#include "dpdk-tp.h"

struct test_data;

struct lcore_data {
	volatile bool done;
	volatile bool started;
	uint64_t total_cycles;
	uint64_t total_calls;
} __rte_cache_aligned;

struct test_data {
	unsigned int nb_workers;
	struct lcore_data ldata[];
} __rte_cache_aligned;

#define STEP 100
#define CENT_OPS(OP) do {     \
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;\
} while (0)


static void
measure_perf(const char *str, struct test_data *data)
{
	uint64_t hz = rte_get_timer_hz();
	uint64_t total_cycles = 0;
	uint64_t total_calls = 0;
	double cycles, ns;
	unsigned int workers;

	for (workers = 0; workers < data->nb_workers; workers++) {
		total_cycles += data->ldata[workers].total_cycles;
		total_calls += data->ldata[workers].total_calls;
	}

	cycles = total_calls ? (double)total_cycles / (double)total_calls : 0;
	cycles /= STEP;
	cycles /= 100; /* CENT_OPS */

	ns = (cycles / (double)hz) * 1E9;
	printf("%s: cycles=%f ns=%f\n", str, cycles, ns);
}

static void
wait_till_workers_are_ready(struct test_data *data)
{
	unsigned int workers;

	for (workers = 0; workers < data->nb_workers; workers++)
		while (!data->ldata[workers].started)
			rte_pause();
}

static void
signal_workers_to_finish(struct test_data *data)
{
	unsigned int workers;

	for (workers = 0; workers < data->nb_workers; workers++) {
		data->ldata[workers].done = 1;
		rte_smp_wmb();
	}
}

 /* NOP Tests */
#define NOP __asm__ volatile ("nop")

static void __rte_noinline
__worker_NOP(struct lcore_data *ldata)
{
	uint64_t start;
	int i;

	while (!ldata->done) {
		start = rte_rdtsc();

		for (i=0; i < STEP; i++)
			CENT_OPS(NOP);

		ldata->total_cycles += rte_rdtsc() - start;
		ldata->total_calls++;
	}
}

static int
worker_fn_NOP(void *arg)
{
	struct lcore_data *ldata = arg;

	ldata->started = 1;
	rte_smp_wmb();

	__worker_NOP(ldata);

	return 0;
}

/* clock_gettime Test */

static inline uint64_t get_clock(void)
{
	volatile uint64_t rc;

	struct timespec ts;

	if ((clock_gettime(CLOCK_MONOTONIC, &ts))) {
		ts.tv_sec = 0;
		ts.tv_nsec = 0;
	}
	rc = ((uint64_t) ts.tv_sec * 1000000000ULL) + ts.tv_nsec;

	return rc;
}

#define GET_CLOCK get_clock()

static void __rte_noinline
__worker_GET_CLOCK(struct lcore_data *ldata)
{
	uint64_t start;
	int i;

	while (!ldata->done) {
		start = rte_rdtsc();

		for (i=0; i < STEP; i++)
			CENT_OPS(GET_CLOCK);

		ldata->total_cycles += rte_rdtsc() - start;
		ldata->total_calls++;
	}
}

static int
worker_fn_GET_CLOCK(void *arg)
{
	struct lcore_data *ldata = arg;

	ldata->started = 1;
	rte_smp_wmb();

	__worker_GET_CLOCK(ldata);

	return 0;
}

static inline int get_cpu(void)
{
	volatile int rc;

	rc = sched_getcpu();

	return rc;
}

#define GET_CPU get_cpu()

static void __rte_noinline
__worker_GET_CPU(struct lcore_data *ldata)
{
	uint64_t start;
	int i;

	while (!ldata->done) {
		start = rte_rdtsc();

		for (i=0; i < STEP; i++)
			CENT_OPS(GET_CPU);

		ldata->total_cycles += rte_rdtsc() - start;
		ldata->total_calls++;
	}
}

static int
worker_fn_GET_CPU(void *arg)
{
	struct lcore_data *ldata = arg;

	ldata->started = 1;
	rte_smp_wmb();

	__worker_GET_CPU(ldata);

	return 0;
}


/* ZERO_ARG test */

#define ZERO_ARG tracepoint(dpdk, zero_arg)

static void __rte_noinline
__worker_ZERO_ARG(struct lcore_data *ldata)
{
	uint64_t start;
	int i;

	while (!ldata->done) {
		start = rte_rdtsc();

		for (i=0; i < STEP; i++)
			CENT_OPS(ZERO_ARG);

		ldata->total_cycles += rte_rdtsc() - start;
		ldata->total_calls++;
	}
}

static int
worker_fn_ZERO_ARG(void *arg)
{
	struct lcore_data *ldata = arg;

	ldata->started = 1;
	rte_smp_wmb();

	__worker_ZERO_ARG(ldata);

	return 0;
}


static void
run_test(const char *str, lcore_function_t fn, struct test_data *data, size_t sz)
{
	unsigned int id, worker = 0;

	memset(data, 0, sz);
	data->nb_workers = rte_lcore_count() - 1;
	RTE_LCORE_FOREACH_SLAVE(id)
		rte_eal_remote_launch(fn, &data->ldata[worker++], id);

	wait_till_workers_are_ready(data);
	rte_delay_ms(1E3); /* Wait for some time to accumalate the stats */
	measure_perf(str, data);
	signal_workers_to_finish(data);

	RTE_LCORE_FOREACH_SLAVE(id)
		rte_eal_wait_lcore(id);
}

int
main(int argc, char **argv)
{
	unsigned int nb_cores, nb_workers;
	struct test_data *data;
	size_t sz;
	int rc;

	rc = rte_eal_init(argc, argv);
	if (rc < 0)
		rte_panic("Cannot init EAL\n");

	nb_cores = rte_lcore_count();
	nb_workers = nb_cores - 1;
	if (nb_cores < 2)
		rte_panic("need minimum two cores for testing\n");

	printf("CPU Timer freq is %fMHz\n", rte_get_timer_hz()/1E6);
	sz = sizeof(struct test_data);
	sz += nb_workers * sizeof(struct lcore_data);

	data = rte_zmalloc(NULL, sz, RTE_CACHE_LINE_SIZE);
	if (data == NULL)
		rte_panic("failed to allocate memory\n");

	run_test("NOP", worker_fn_NOP, data, sz);
	run_test("GET_CLOCK", worker_fn_GET_CLOCK, data, sz);
	run_test("GET_CPU", worker_fn_GET_CPU, data, sz);
	run_test("ZERO_ARG", worker_fn_ZERO_ARG, data, sz);

	rte_free(data);
	return rte_eal_cleanup();
}
