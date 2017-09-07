#include "node-table.h"
#include <string.h>
#include <limits.h>

/** The table itself */
static netctrl_node_t node_table[NODE_TABLE_SIZE];
/** Points to an empty entry in the table or NULL if the table its full. */
static netctrl_node_t *empty_entry;
/** A node is considered invalid if it is idle for more then the value in this variable. */
static clock_time_t timeout_value;
/*---------------------------------------------------------------------------*/
static netctrl_node_t *get_first_empty_entry() {
	netctrl_node_t *entry = NULL;
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
static clock_time_t timedout(netctrl_node_t *node) {
	clock_time_t currentTime = clock_time();
	clock_time_t elapsed;

	// handle overflows
	if(currentTime < node->last_req_time) {
		// TODO Use other defenition then UINT_MAX to be more portable - try using -1 with cast to unsigned...
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
	timeout_value = timeout;
}
/*---------------------------------------------------------------------------*/
netctrl_node_t * node_table_get_node(uip_ip6addr_t * nodeIp) {
	int i;
	for(i = 0; i < NODE_TABLE_SIZE; i++) {
		if(memcmp(nodeIp, &(node_table[i].ip_addr), sizeof(uip_ip6addr_t)) == 0) {
			return &node_table[i];
		}
	}
	return NULL;
}
/*---------------------------------------------------------------------------*/
void
node_table_update_node(netctrl_node_t *node, uint16_t version, uint32_t data) {
	node->entry_version = version;
	node->data = data;
	node->last_req_time = clock_time();
}
/*---------------------------------------------------------------------------*/
int
node_table_add_node(uip_ip6addr_t *ip_addr, uint32_t hash, uint16_t reqId,
		uint32_t data, uint8_t eqType) {
	/* full table? */
	if(empty_entry == NULL) {
		return -1;
	}
	memcpy(&(empty_entry->ip_addr), ip_addr, sizeof(uip_ip6addr_t));
	empty_entry->hash = hash;
	empty_entry->data = data;
	empty_entry->timestamp = clock_time();;
	empty_entry->last_req_time = empty_entry->timestamp;
	empty_entry->entry_version = reqId;
	empty_entry->type = eqType;

	empty_entry = get_first_empty_entry();

	return 0;
}
/*---------------------------------------------------------------------------*/
void
node_table_remove_node(netctrl_node_t *node) {
	memset(&(node->ip_addr), 0x0, sizeof(node->ip_addr));
	empty_entry = node;
}
/*---------------------------------------------------------------------------*/
clock_time_t node_table_refresh() {
	// TODO: A cleanner way to do this (but more memory-consuming) is add a
	// ctimer to each entry. Each ctimer is set with its timeout value and
	// its callback removes its corresponding entry from the table.
	// Every time a node send a new request its ctimer is reset.

	// Default check time of 1 second
	// TODO Use other defenition then UINT_MAX to be more portable - try using -1 with cast to unsigned...
	clock_time_t nextCheck = UINT_MAX;
	int i;
	for(i = 0; i < NODE_TABLE_SIZE; i++) {
		if(!uip_is_addr_unspecified(&node_table[i].ip_addr)) {
			clock_time_t node_timeout = timedout(&node_table[i]);
			if(node_timeout == 0) {
				node_table_remove_node(&node_table[i]);
			} else {
				nextCheck = MIN(nextCheck, node_timeout);
			}
		}
	}

	// When the table is empty, dont need to refresh it
	if(node_table_is_empty()) {
		// TODO Use other defenition then UINT_MAX to be more portable - try using -1 with cast to unsigned...
		nextCheck = UINT_MAX;
	}

	return nextCheck;
}
