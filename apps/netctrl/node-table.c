/**
 * \addtogroup node-table
 * @{
 */

#include "node-table.h"
#include <string.h>
#include <limits.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/** The table itself */
static node_table_entry_t node_table[NODE_TABLE_SIZE];
/** Points to an empty entry in the table or NULL if the table its full. */
static node_table_entry_t *empty_entry;
/** A node is considered invalid if it is idle for more then the value in this variable. */
static clock_time_t timeout_value;
/*---------------------------------------------------------------------------*/
static node_table_entry_t *get_first_empty_entry() {
	node_table_entry_t *entry = NULL;
	int i;
	for(i = 0; i < NODE_TABLE_SIZE; i++) {
		if(uip_is_addr_unspecified(&node_table[i].ip_addr)) {
			entry = &node_table[i];
			break;
		}
	}

	return entry;
}
/*---------------------------------------------------------------------------*/
/**
 * Check if the node reach the maximum idle time.
 *
 * \return Returns the time in system ticks it need to wait until the timeout, or zero if it reached the timeout.
 */
static clock_time_t timedout(node_table_entry_t *node) {
	clock_time_t currentTime = clock_time();
	clock_time_t elapsed;

	// handle overflows
	if(currentTime < node->last_req_time) {
		currentTime = currentTime + (UINT_MAX - node->last_req_time) + 1;
	}

	elapsed = currentTime - node->last_req_time;
	if(elapsed >= timeout_value) {
		// timeout
		return 0;
	}
	return timeout_value - elapsed;
}

static int node_table_is_empty() {
	int i;
	for(i = 0; i < NODE_TABLE_SIZE; i++) {
		if(!uip_is_addr_unspecified(&node_table[i].ip_addr)) {
			return 0;
		}
	}
	return 1;
}
/*---------------------------------------------------------------------------*/
void node_table_init(clock_time_t timeout) {
	memset(&node_table, 0x0, sizeof(node_table));
	empty_entry = node_table;
	timeout_value = timeout * CLOCK_SECOND;
}
/*---------------------------------------------------------------------------*/
node_table_entry_t * node_table_get_node(uip_ip6addr_t * nodeIp) {
	int i;
	for(i = 0; i < NODE_TABLE_SIZE; i++) {
		if(memcmp(nodeIp, &(node_table[i].ip_addr), sizeof(uip_ip6addr_t)) == 0) {
			return &node_table[i];
		}
	}
	return NULL;
}
/*---------------------------------------------------------------------------*/
node_table_entry_t * node_table_get_node_by_hash(uint32_t nodeHash) {
	int i;
	for(i = 0; i < NODE_TABLE_SIZE; i++) {
		if(nodeHash == node_table[i].hash) {
			return &node_table[i];
		}
	}
	return NULL;
}
/*---------------------------------------------------------------------------*/
void
node_table_update_node(node_table_entry_t *node, uint16_t version, uint32_t data) {
	node->entry_version = version;
	node->data = data;
	node->last_req_time = clock_time();
}
/*---------------------------------------------------------------------------*/
node_table_entry_t *
node_table_add_node(uip_ip6addr_t *ip_addr, uint32_t hash, uint16_t reqId,
		uint32_t data, uint8_t eqType) {
	/* full table? */
	if(empty_entry == NULL) {
		return NULL;
	}
	memcpy(&(empty_entry->ip_addr), ip_addr, sizeof(uip_ip6addr_t));
	empty_entry->hash = hash;
	empty_entry->data = data;
	empty_entry->timestamp = clock_time();;
	empty_entry->last_req_time = empty_entry->timestamp;
	empty_entry->entry_version = reqId;
	empty_entry->type = eqType;

	node_table_entry_t * tmp_ptr = empty_entry;
	empty_entry = get_first_empty_entry();

	return tmp_ptr;
}
/*---------------------------------------------------------------------------*/
void
node_table_remove_node(node_table_entry_t *node) {
	memset(&(node->ip_addr), 0x0, sizeof(node->ip_addr));
	node->hash = 0;
	empty_entry = node;
}
/*---------------------------------------------------------------------------*/
clock_time_t node_table_refresh() {
	// TODO: A cleanner way to do this (but more memory-consuming) is add a
	// ctimer to each entry. Each ctimer is set with its timeout value and
	// its callback removes its corresponding entry from the table.
	// Every time a node send a new request its ctimer is reset.

	clock_time_t nextCheck = UINT_MAX;
	int i;
	for(i = 0; i < NODE_TABLE_SIZE; i++) {
		if(!uip_is_addr_unspecified(&node_table[i].ip_addr)) {
			clock_time_t node_timeout = timedout(&node_table[i]);
			if(node_timeout == 0) {
				PRINTF("Node Table: Removing timed out Node with hash %lu\n", (unsigned long)node_table[i].hash);
				node_table_remove_node(&node_table[i]);
			} else {
				nextCheck = MIN(nextCheck, node_timeout);
			}
		}
	}

	// When the table is empty, dont need to refresh it
	if(node_table_is_empty()) {
		PRINTF("Empty Node Table.\n");
		nextCheck = UINT_MAX;
	}

	return nextCheck;
}
/*---------------------------------------------------------------------------*/
void node_table_init_iterator(node_table_iterator_t *it) {
	it->idx = -1;
	it->node = NULL;
}
/*---------------------------------------------------------------------------*/
int node_table_iterate(node_table_iterator_t *it) {
	int i;

	// Select the beginning or the next one
	i = (it->idx < 0)? 0 : it->idx + 1;
	for(; i < NODE_TABLE_SIZE; i++) {
		if(!uip_is_addr_unspecified(&node_table[i].ip_addr)) {
			it->node = &node_table[i];
			it->idx = i;
			return 0;
		}
	}

	return -1;
}
/**
 * @}
 */
