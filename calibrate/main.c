#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <rte_debug.h>
#include <rte_eal.h>

int
main(int argc, char **argv)
{
	int rc;

	rc = rte_eal_init(argc, argv);
	if (rc < 0)
		rte_panic("Cannot init EAL\n");

	rte_eal_cleanup();

	return 0;
}
