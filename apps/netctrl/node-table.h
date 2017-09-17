#ifndef CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NODE_TABLE_NODE_TABLE_H_
#define CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NODE_TABLE_NODE_TABLE_H_

/**
 * \defgroup node-table
 * @{
 */

/*
 * @file     node-table.h
 * @brief    Node Table's implementation.
 * @version  0.1
 * @author   Claudio Prates & Ricardo Jesus & Tiago Costa
 */

#include <contiki-net.h>
#include <stdint.h>

#ifndef NODE_TABLE_CONF_SIZE /*!< Number of nodes that the table can store. */
#define NODE_TABLE_SIZE  5
#else
#define NODE_TABLE_SIZE  NODE_TABLE_CONF_SIZE
#endif /* NODE_TABLE_CONF_SIZE */
#define NODE_TABLE_DEFAULT_CHECK_TIME  1 /*!< In seconds */

/** Entry structure of node table. */
typedef struct {
	uip_ip6addr_t ip_addr; /*!< Node's IPv6 address */
	uint32_t hash; /*!< Unique value to identify the entry */
	uint32_t data; /*!< Data sent by the node */
	clock_time_t timestamp; /*!< Timestamp from the registration request. Useful to get the uptime */
	clock_time_t last_req_time; /*!< Timestamp of the last request received. Used to control the node's lifetime in the table */
	uint16_t entry_version; /*!< Version of the entry because it can be updated */
	uint8_t type; /*!< Type of the node */
	uint8_t requests; /*!< Number of requests currently being processed for this Node */
	void * node_data; /*!< normally used to point to the http_state when passed to CoAP client process */
} node_table_entry_t;

/*---------------------------------------------------------------------------*/
/**
 * Initializes the table.
 *
 * \param timeout A node is considered invalid if it is idle for more then the value in this parameter.
 */
void node_table_init(clock_time_t timeout);
/*---------------------------------------------------------------------------*/
/**
 * Try to retreive a node from the table, searching for its NodeId.
 *
 * \param nodeIp Pointer to the node's IPv6 address. A node is identified by is ip address.
 *
 * \return Returns a pointer to the node if found, or NULL otherwise.
 */
node_table_entry_t * node_table_get_node(uip_ip6addr_t * nodeIp);
/*---------------------------------------------------------------------------*/
/**
 * Try to retreive a node from the table, searching for its hash.
 *
 * \param nodeHash Node's hash. A node is identified by is ip address.
 *
 * \return Returns a pointer to the node if found, or NULL otherwise.
 */
node_table_entry_t * node_table_get_node_by_hash(uint32_t nodeHash);
/*---------------------------------------------------------------------------*/
/**
 * Updates a node with new data received.
 *
 * \param node A pointer to the node to be update
 * \param version The version of the last request received
 * \param data New data sent by the node
 */
void
node_table_update_node(node_table_entry_t *node, uint16_t version, uint32_t data);
/*---------------------------------------------------------------------------*/
/**
 * Add a new node to the table
 *
 * \param ip_addr IPv6 address of the node
 * \param hash Hash that uniquely identifies the node. Useful to identify the node on a web platform or mobile app.
 * \param version The version of the last request received
 * \param data New data sent by the node
 * \param eqType Type of the equipment of the node
 *
 * \return Returns a pointer to the new node if it could be added, NULL otherwise.
 */
node_table_entry_t *
node_table_add_node(uip_ip6addr_t *ip_addr, uint32_t hash, uint16_t reqId,
		uint32_t data, uint8_t eqType);
/*---------------------------------------------------------------------------*/
/**
 * Remove the given node from the table.
 *
 * \param node Node to be removed from the table.
 */
void
node_table_remove_node(node_table_entry_t *node);

/**
 * Verify all nodes looking for those who reach the maximum idle time and remove them.
 *
 * \return Returns the time until the next timeout in system ticks.
 */
clock_time_t node_table_refresh();

/**
 * @}
 */

#endif /* CONTIKI_MULTIPLE_INTERFACE_APPS_NETCTRL_NODE_TABLE_NODE_TABLE_H_ */
