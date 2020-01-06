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

struct test_data;

struct lcore_data {
	bool done;
	bool started;
	uint64_t total_cycles;
	uint64_t total_calls;
} __rte_cache_aligned;

struct test_data {
	unsigned int nb_workers;
	struct lcore_data ldata[];
} __rte_cache_aligned;

static double
calibrate_overhead(struct test_data *data)
{
	uint64_t total_cycles = 0;
	uint64_t total_calls = 0;
	double cycles;
	unsigned int workers;

	for (workers = 0; workers < data->nb_workers; workers++) {
		total_cycles += data->ldata[workers].total_cycles;
		total_calls += data->ldata[workers].total_calls;
	}

	cycles = total_calls ? (double)total_cycles / (double)total_calls : 0;
	return cycles;
}


static void
measure_perf(struct test_data *data, double ref_cycles)
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
	cycles -= ref_cycles;

	ns = (cycles / (double)hz) * 1E9;
	printf("cycles=%f ns=%f\n", cycles, ns);
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

static void __rte_noinline
__cal_worker(struct lcore_data *ldata)
{
	uint64_t start;

	while (!ldata->done) {
		start = rte_rdtsc();


		ldata->total_cycles += rte_rdtsc() - start;
		ldata->total_calls++;
	}
}

static void __rte_noinline
__worker(struct lcore_data *ldata)
{
	uint64_t start;

	while (!ldata->done) {
		start = rte_rdtsc();


__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");
__asm__ volatile ("nop");

		ldata->total_cycles += rte_rdtsc() - start;
		ldata->total_calls++;
	}
}


static int
cal_worker_fn(void *arg)
{
	struct lcore_data *ldata = arg;

	ldata->started = 1;
	rte_smp_wmb();

	__cal_worker(ldata);

	return 0;
}

static int
worker_fn(void *arg)
{
	struct lcore_data *ldata = arg;

	ldata->started = 1;
	rte_smp_wmb();

	__worker(ldata);

	return 0;
}


int
main(int argc, char **argv)
{
	unsigned int id, nb_cores, nb_workers, worker = 0;
	struct test_data *data;
	double ref_cycles;
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

	/* First pass to calibrate the overhead */
	data->nb_workers = nb_workers;
	RTE_LCORE_FOREACH_SLAVE(id)
		rte_eal_remote_launch(cal_worker_fn, &data->ldata[worker++], id);

	wait_till_workers_are_ready(data);
	rte_delay_ms(1E2); /* Wait for some time to accumalate the stats */
	ref_cycles = calibrate_overhead(data);
	signal_workers_to_finish(data);

	RTE_LCORE_FOREACH_SLAVE(id)
		rte_eal_wait_lcore(id);

	/* Second pass to find the real cycles */
	worker = 0;
	memset(data, 0, sz);
	data->nb_workers = nb_workers;
	RTE_LCORE_FOREACH_SLAVE(id)
		rte_eal_remote_launch(worker_fn, &data->ldata[worker++], id);

	wait_till_workers_are_ready(data);
	rte_delay_ms(1E3); /* Wait for some time to accumalate the stats */
	measure_perf(data, ref_cycles);
	signal_workers_to_finish(data);

	RTE_LCORE_FOREACH_SLAVE(id)
		rte_eal_wait_lcore(id);

	rte_free(data);
	return rte_eal_cleanup();
}
