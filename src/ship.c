#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <signal.h>

#include "include/const.h"
#include "include/sem.h"
#include "include/shm_general.h"
#include "include/shm_port.h"
#include "include/shm_ship.h"
#include "include/shm_cargo.h"
#include "include/utils.h"
#include "include/shm_offer_demand.h"
#include "include/cargo_list.h"
#include "include/msg_commerce.h"

#define GET_DISTANCE(dest)\
	(sqrt(pow(dest.x - shm_ship_get_coords(state.ship, state.id).x, 2) + pow(dest.y - shm_ship_get_coords(state.ship, state.id).y, 2)))

void signal_handler(int signal);

void init_location(void);
int pick_first_destination_port(void);
void trade(void);
int sell(int cargo_type);
void move(int port_id);

void close_all(void);
void loop(void);
int find_new_destination_port(void);

struct state {
	int id;
	shm_general_t *general;
	shm_port_t *port;
	shm_ship_t *ship;
	shm_cargo_t *cargo;

	shm_demand_t *demand;
	o_list_t *cargo_hold;

	int curr_port_id;
	int current_day;
};

struct state state;

int main(int argc, char *argv[])
{
	struct sigaction sa;
	sigset_t mask;
	bzero(&state, sizeof (struct state));
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &signal_handler;

	sigfillset(&mask);
	sa.sa_mask = mask;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	state.id = (int)strtol(argv[1], NULL, 10);
	shm_general_attach(&state.general);
	state.port = shm_port_attach(state.general);
	state.ship = shm_ship_attach(state.general);
	state.cargo = shm_cargo_attach(state.general);
	state.demand = shm_demand_init(state.general);

	state.cargo_hold = cargo_list_create(state.general);

	srand(time(NULL) * getpid());
	init_location();

	sigemptyset(&mask);
	sa.sa_mask = mask;
	sigaction(SIGSTORM, &sa, NULL);
	sigaction(SIGMAELSTROM, &sa, NULL);

	sem_execute_semop(get_sem_port_init_id(), 0, 0, 0);
	sem_execute_semop(get_sem_start_id(), 0, 0, 0);

	dprintf(1, "ship %d: reached loop\n", state.id);
	loop();
}

void loop(void) {
	int receiver_port;
	int day;
	int id_dest_port;
	struct coord dest_port_coords;
	struct commerce_msg msg;

	id_dest_port = pick_first_destination_port();
	move(id_dest_port);
	trade();

	/*receiver_port = RANDOM_INTEGER(0, get_porti(state.general) - 1);
	dprintf(1, "ship %d: sending message to port %d\n", state.id, receiver_port);
	msg = msg_commerce_create(receiver_port, state.id, 1, 10, 8, 0);
	msg_commerce_send(get_msg_in_id(state.general), &msg);*/
	while (1) {
		day = get_current_day(state.general);
		if (state.current_day < day) {
			state.current_day = day;
			cargo_list_ship_remove_expired(state.cargo_hold,state.general, state.cargo);
		}

		if(cargo_list_get_quantity(state.cargo_hold, state.general) == 0) {
			shm_ship_set_dump_with_cargo(state.ship, state.id, FALSE);
		} else {
			shm_ship_set_dump_with_cargo(state.ship, state.id, TRUE);
		}
/*		id_dest_port = find_new_destination_port();
		move(id_dest_port);
		trade();*/
	}
}

void handle_message(void)
{
	struct commerce_msg msg;
	int sender_id, cargo_id, quantity, expiry_date, status;

	if (!msg_commerce_receive(get_msg_out_id(state.general), state.id,
				  &sender_id, &cargo_id, &quantity,
				  &expiry_date, NULL, &status, FALSE)) {	/* TODO: a cosa si riferisce NULL? C'è un parametro di troppo. */
		return;
	}
	switch (status) {
	case STATUS_ACCEPTED:
		/* Struttura per salvare il trading da fare con la barca */
		break;
	case STATUS_LOAD_ACCEPTED:
		cargo_list_add(state.cargo_hold, cargo_id, quantity, expiry_date);
	case STATUS_REFUSED:
	default:
		break;
	}
}

/**
 * @brief initializes ship's location.
 */
void init_location(void)
{
	struct coord coords;
	/* generate a random location on the map */
	coords.x = RANDOM_DOUBLE(0, get_lato(state.general));
	coords.y = RANDOM_DOUBLE(0, get_lato(state.general));

	shm_ship_set_coords(state.ship, state.id, coords);
	shm_ship_set_is_moving(state.ship, state.id, TRUE);
	/* TODO
	 * ship_shm_set_dump_with_cargo(state.ship, state.id, FALSE);*/
}

int pick_first_destination_port(void)
{
	int target_port;
	target_port = RANDOM_INTEGER(0, get_porti(state.general) - 1);
	return target_port;
}

/**
 * @brief simulates the movement of the ship and updates the location.
 */
void move(int port_id)
{
	struct coord dest_coords;
	double time_required;

	dest_coords = shm_port_get_coordinates(state.port, port_id);
	shm_ship_set_is_moving(state.ship, state.id, TRUE);
	/* calculate time required to arrive (in days) */
	time_required = GET_DISTANCE(dest_coords) / get_speed(state.general);
	convert_and_sleep(time_required);
	/* set new location */
	shm_ship_set_coords(state.ship, state.id, dest_coords);
	shm_ship_set_is_moving(state.ship, state.id, FALSE);
	state.curr_port_id = port_id;
}

int find_new_destination_port(void)
{
	int cargo_type;
	int port, best_port = -1;
	int n_ports;
	int port_demand;
	int sale_amount, sale_best_amount = 0;
	int amount_not_expired;
	double time_required, best_time;

	n_ports = get_porti(state.general);
	for (port = 0; port < n_ports; port++) {
		if (port == state.curr_port_id) continue;

		/* Check port distance */
		time_required = GET_DISTANCE(shm_port_get_coordinates(state.port, port)) / get_speed(state.general);

		sale_amount = 0;
		for (cargo_type = 0; cargo_type < get_merci(state.general); cargo_type++) {
			amount_not_expired = get_not_expired_by_day(state.cargo_hold, cargo_type, state.current_day + (int) time_required);
			port_demand = shm_demand_get_quantity(state.general, state.demand, port, cargo_type);
			sale_amount += MIN(amount_not_expired, port_demand);
		}

		if (best_port == -1 || sale_amount > sale_best_amount
		    || (sale_best_amount == sale_amount && time_required < best_time)) {
			best_port = port;
			sale_best_amount = sale_amount;
			best_time = time_required;
		}
	}
	return best_port;
}

void trade(void)
{
	int i, n_cargo, sem_docks_id;
	int load_speed, tons_moved;
	sigset_t mask;
	load_speed = get_load_speed(state.general);
	n_cargo = get_merci(state.general);

	sem_docks_id = shm_port_get_sem_docks_id(state.port);
	sigemptyset(&mask);
	sigaddset(&mask, SIGMAELSTROM);

	/* Requesting dock */
	sem_execute_semop(sem_docks_id, state.curr_port_id, -1, 0);
	shm_ship_set_is_at_dock(state.ship, state.id, TRUE);

	/* Selling */
	for (i = 0; i < n_cargo; i++) {
		sigprocmask(SIG_BLOCK, &mask, NULL);
		tons_moved = sell(i);
		sigprocmask(SIG_UNBLOCK, &mask, NULL);

		convert_and_sleep((tons_moved / load_speed) / 24.0);
	}

	/* Releasing dock */
	sem_execute_semop(sem_docks_id, state.curr_port_id, 1, 0);
	shm_ship_set_is_at_dock(state.ship, state.id, FALSE);

}

int sell(int cargo_type)
{
	struct commerce_msg msg;
	int available_in_ship, port_demand;
	int amount_to_sell, tons_moved;
	int msg_port_in_id, msg_port_out_id;
	int quantity, expire_date, status;

	msg_port_in_id = get_msg_in_id(state.general);
	msg_port_out_id = get_msg_out_id(state.general);

	available_in_ship = cargo_list_get_quantity_by_id(state.cargo_hold, cargo_type);
	port_demand = shm_demand_get_quantity(state.general, state.demand, state.curr_port_id, cargo_type);
	amount_to_sell = MIN(available_in_ship, port_demand);
	if (amount_to_sell <= 0) return 0;

	msg_commerce_create(state.curr_port_id, state.id, cargo_type, amount_to_sell, 0, -1);
	msg_commerce_send(msg_port_in_id, &msg);
	msg_commerce_receive(msg_port_out_id, state.id, 0, 0, &quantity, 0, &status, TRUE);

	/* TODO check for status and act accordingly */

}

void signal_handler(int signal)
{
	switch (signal) {
	case SIGSTORM:
		dprintf(1, "Ship %d: Received SIGSTORM signal.\n", state.id);
		shm_ship_set_dump_had_storm(state.ship, state.id);
		convert_and_sleep(get_storm_duration(state.general) / 24.0);
		break;
	case SIGMAELSTROM:
		dprintf(1, "Ship %d: Received SIGMAELSTROM signal.\n",
			state.id);
		shm_ship_set_is_dead(state.ship, state.id);
		close_all();
	case SIGSEGV:
		dprintf(1, "Received SIGSEGV signal.\n");
		dprintf(2, "ship.c: id: %d: Segmentation fault. Terminating.\n",
			state.id);
	case SIGINT:
		close_all();
	}
}

void close_all(void)
{
	cargo_list_delete(state.cargo_hold, state.general);
	shm_port_detach(state.port);
	shm_ship_detach(state.ship);
	shm_cargo_detach(state.cargo);
	shm_general_detach(state.general);
	exit(0);
}
