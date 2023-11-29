#ifndef OS_PROJECT_OFFER_H
#define OS_PROJECT_OFFER_H

#include <stdlib.h>

#include "shm_general.h"
#include "shm_cargo.h"
#include "cargo_list.h"
#include "types.h"

typedef struct shm_offer shm_offer_t;
typedef struct shm_demand shm_demand_t;

/**
 * @brief Initializes and attaches shared memory for offer data in ports.
 * @param g pointer to general SHM
 * @return Pointer to the attached offer data structure or NULL on failure.
 */
shm_offer_t *shm_offer_ports_init(shm_general_t *g);

/**
 * @brief Attaches shared memory for offer data in ports.
 * @param g pointer to general shm
 * @return Pointer to the attached offer data structure.
 */
shm_offer_t *shm_offer_ports_attach(shm_general_t *g);

/**
 * @brief Adds a specified quantity to the offer data in shared memory.
 * @param o pointer to the array of offers
 * @param id the id of the port for which the offer data is updated
 * @param type the type of the offer data to be updated
 * @param quantity quantity to be added to the offer data
 */
void shm_offer_add_quantity(shm_offer_t *o, shm_general_t *g, int id, int type,
		   int quantity);

/**
 * @brief Removes a specified quantity from the offer data in shared memory.
 * @param o pointer to the array of offers in shared memory
 * @param id identifier of the port for which the offer data is updated
 * @param type type of the offer data to be updated
 * @param quantity quantity to be subtracted from the offer data
 */
void shm_offer_remove_quantity(shm_offer_t *o, shm_general_t *g, int id, int type,
		      int quantity);

/**
 * @brief Delete a shared memory offer structure, freeing allocated memory.
 * @param o Pointer to the shared memory offer structure to be deleted.
 */
void shm_offer_delete(shm_offer_t *o);

/**
 * @brief Combines data from two arrays of shm_offer_t structures
 * @param src Pointer to the array of source offers in shared memory.
 * @param merge Pointer to local array of offers to be merged.
 * @param c pointer to general SHMo
 * @param id Identifier of the port for which the demand data is updated.
 */
void shm_offer_merge(shm_offer_t *src, shm_offer_t *merge, shm_general_t *c,
		     int id);

/**
 * @brief Gets a portion of goods from a specific offer in shared memory.
 * @param o Pointer to the array of shared memory offers.
 * @param g Pointer to the shared memory structure containing general information.
 * @param id The index of the offer to retrieve goods from.
 * @param capacity The maximum capacity of goods to retrieve.
 *
 * @return A pointer to a new shared memory offer containing the retrieved goods.
 */
shm_offer_t *shm_offer_get_order(shm_offer_t *o, shm_general_t *g, int id,
				 int capacity);
/**
 * @brief Initializes and attaches shared memory for demand data.
 * @param g pointer to general SHM
 * @return Pointer to the attached demand data structure or NULL on failure.
 */
shm_demand_t *shm_demand_init(shm_general_t *g);

/**
 * @brief Sets the demand data in shared memory for a specific port.
 * @param o pointer to the array of demands
 * @param g pointer to general SHM
 * @param id the id of the port for which the demand data is updated
 * @param type the type of the demand data to be updated
 * @param quantity quantity to be added to the demand data
 */
void shm_demand_set(shm_demand_t *d, shm_general_t *g, int id, int type,
		    int quantity);

/**
 * @brief Removes a specified quantity from the demand data in shared memory.
 * @param d Pointer to the array of demands in shared memory.
 * @param g pointer to general SHM
 * @param id Identifier of the port for which the demand data is updated.
 * @param type Type of the demand data to be updated.
 * @param quantity Quantity to be subtracted from the demand data.
 */
void shm_demand_remove(shm_demand_t *d, shm_general_t *g, int id, int type,
		       int quantity);

/**
 * @brief Return the quantity of requested item
 * @param d Pointer to the array of demands in shared memory.
 * @param id Identifier of the port for which the demand data is requested.
 * @param type Type of the demand data to be requested.
 */
/*int demand_shm_get(shm_demand_t *d, int id, int type);*/


/**
 * Delete offer and demand shared memory
 * @param g pointer to general SHM
 */
void shm_offer_demand_delete(shm_general_t *g);

/**
 * @brief Generates random offers and demands
 * @param o pointer to the array of offers
 * @param d pointer to the array of demands
 * @param l pointer to the array of expires
 * @param id the id of the port offering or demanding
 * @param c pointer to cargo SHM
 * @param g pointer to general SHM
 */
void shm_offer_demand_generate(shm_offer_t *o, shm_demand_t *d, o_list_t **l,
			       int port_id, shm_cargo_t *c, shm_general_t *g);

int shm_offer_get_quantity(shm_general_t *g, shm_offer_t *o, int port_id, int cargo_id);
int shm_demand_get_quantity(shm_general_t *g, shm_demand_t *d, int port_id, int cargo_id);
int shm_offer_get_tot_quantity(shm_general_t *g, shm_offer_t *o, int port_id);

#endif
