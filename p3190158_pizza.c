#pragma
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "p3190158_pizza.h"

typedef struct {
	int id;							/* αριθμός παραγγελίας */
	unsigned int seed;				/* σπόρος για rand_r */
	struct timespec start_ts;		/* */
	struct timespec ready_ts;		/* */
	struct timespec delivered_ts;	/* */
} Order;

/* Διαθέσιμοι πόροι */
int available_tels = Ntel;
int available_cooks = Ncook;
int available_ovens = Noven;
int available_packers = Npacker;
int available_deliverers = Ndeliverer;

/* Συγχρονισμός Mutex και μεταβλητών συνθήκης*/
pthread_mutex_t tel_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  tel_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cook_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cook_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t oven_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  oven_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t packer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  packer_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t deliverer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  deliverer_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Αρχικοποιήσεις */
int total_income = 0;
int total_m = 0;
int total_p = 0;
int total_s = 0;
int successful_orders = 0;
int failed_orders = 0;

double total_service_time = 0.0;
double max_service_time = 0.0;
double total_cool_time = 0.0;
double max_cool_time = 0.0;


/* δημιουργία τυχαίες μεταβλητής με όρια [low,high] χρησιμοποιώντας rand_r */
static int rand_range_r(unsigned int* seedp, int low, int high) {
	return low + rand_r(seedp) % (high - low + 1);
}

static double elapsed_min(const struct timespec* start, const struct timespec* end) {
	double sec = end->tv_sec - start->tv_sec;
	double nsec = (end->tv_nsec - start->tv_nsec) / 1e9;
	return (sec + nsec);
}

/*Διαδικασία εκτέλεσης κατά τη δημιουργία νήματος*/
void* order_thread(void* arg) {
	Order* o = arg;
	clock_gettime(CLOCK_MONOTONIC, &o->start_ts);

	/* Λήψη παραγγελίας μέσω τηλεφωνητή */
	pthread_mutex_lock(&tel_mutex);
	while (available_tels <= 0)
		pthread_cond_wait(&tel_cond, &tel_mutex);
	available_tels--;
	pthread_mutex_unlock(&tel_mutex);

	/* Πληρωμή */
	sleep(rand_range_r(&o->seed, TpaymentLow, TpaymentHigh));

	if (rand_range_r(&o->seed, 1, 100) <= Pfail) {   /* 'Ελεγχος πιθανότητας αποτυχίας */
		pthread_mutex_lock(&stats_mutex);
		failed_orders++;
		pthread_mutex_unlock(&stats_mutex);

		pthread_mutex_lock(&tel_mutex);
		available_tels++;
		pthread_cond_signal(&tel_cond);
		pthread_mutex_unlock(&tel_mutex);

		pthread_mutex_lock(&print_mutex);
		printf("Η παραγγελία %d απέτυχε (πληρωμή).\n", o->id);
		pthread_mutex_unlock(&print_mutex);
		free(o);
		return NULL;
	}

	/* Επιτυχής καταχώρηση παραγγελίας */
	pthread_mutex_lock(&stats_mutex);
	successful_orders++;
	pthread_mutex_unlock(&stats_mutex);

	pthread_mutex_lock(&tel_mutex);
	available_tels++;
	pthread_cond_signal(&tel_cond);
	pthread_mutex_unlock(&tel_mutex);

	/* Επιλογή πλήθους & τύπου πίτσας */
	int n_pizzas = rand_range_r(&o->seed, NorderLow, NorderHigh);
	int price = 0;
	for (int i = 0; i < n_pizzas; i++) {
		int r = rand_range_r(&o->seed, 1, 100);
		pthread_mutex_lock(&stats_mutex);
		if (r <= Pm) {
			total_m++;				/* Καταχώριση και χρέωση Μαργαρίτας */
			price += Cm;
		}
		else if (r <= Pm + Pp) {
			total_p++;				/* Καταχώριση και χρέωση Πεπερόνι */
			price += Cp;
		}
		else {
			total_s++;				/* Καταχώριση και χρέωση Σπέσιαλ */
			price += Cs;
		}
		pthread_mutex_unlock(&stats_mutex);
	}
	pthread_mutex_lock(&stats_mutex);
	total_income += price;			/* Αύξηση συνολικών εσόδων */
	pthread_mutex_unlock(&stats_mutex);

	/* Προετοιμασία κάθε πίτσας */
	pthread_mutex_lock(&cook_mutex);
	while (available_cooks <= 0)
		pthread_cond_wait(&cook_cond, &cook_mutex);
	available_cooks--;
	pthread_mutex_unlock(&cook_mutex);
	sleep(n_pizzas * Tprep);

	/* Δέσμευση πλήθος φούρνων, όσες και οι πίτσες */
	pthread_mutex_lock(&oven_mutex);
	while (available_ovens < n_pizzas)
		pthread_cond_wait(&oven_cond, &oven_mutex);
	available_ovens -= n_pizzas;
	pthread_mutex_unlock(&oven_mutex);
	sleep(Tbake);

	/* Αποδέσμευση φούρνων/μαγείρων */
	pthread_mutex_lock(&cook_mutex);
	available_cooks++;
	pthread_cond_signal(&cook_cond);
	pthread_mutex_unlock(&cook_mutex);
	pthread_mutex_lock(&oven_mutex);
	available_ovens += n_pizzas;
	pthread_cond_broadcast(&oven_cond);
	pthread_mutex_unlock(&oven_mutex);

	/*  Συσκευασία  */
	pthread_mutex_lock(&packer_mutex);
	while (available_packers <= 0)
		pthread_cond_wait(&packer_cond, &packer_mutex);
	available_packers--;
	pthread_mutex_unlock(&packer_mutex);
	sleep(n_pizzas * Tpack);
	pthread_mutex_lock(&packer_mutex);
	available_packers++;
	pthread_cond_signal(&packer_cond);
	pthread_mutex_unlock(&packer_mutex);

	/* Ολοκλήρωση Ετοιμασίας */
	clock_gettime(CLOCK_MONOTONIC, &o->ready_ts);
	double serv = elapsed_min(&o->start_ts, &o->ready_ts);
	pthread_mutex_lock(&time_mutex);
	total_service_time += serv;
	if (serv > max_service_time) max_service_time = serv;
	pthread_mutex_unlock(&time_mutex);

	/* Παράδοση */
	pthread_mutex_lock(&deliverer_mutex);
	while (available_deliverers <= 0)
		pthread_cond_wait(&deliverer_cond, &deliverer_mutex);
	available_deliverers--;
	pthread_mutex_unlock(&deliverer_mutex);
	sleep(rand_range_r(&o->seed, TdelLow, TdelHigh));
	pthread_mutex_lock(&deliverer_mutex);
	available_deliverers++;
	pthread_cond_signal(&deliverer_cond);
	pthread_mutex_unlock(&deliverer_mutex);

	/* Καταγραφή κρυώματος */
	clock_gettime(CLOCK_MONOTONIC, &o->delivered_ts);
	double cool = elapsed_min(&o->ready_ts, &o->delivered_ts);
	pthread_mutex_lock(&time_mutex);
	total_cool_time += cool;
	if (cool > max_cool_time) max_cool_time = cool;
	pthread_mutex_unlock(&time_mutex);

	/* Εμφάνιση χρόνων ετοιμασίας και παράδοσης (από την ώρα λήψης της παραγγελίας) */
	pthread_mutex_lock(&print_mutex);
	printf("Η παραγγελία %d ετοιμάστηκε σε %.2f λεπτά.\n", o->id, serv);
	printf("Η παραγγελία %d παραδόθηκε σε %.2f λεπτά.\n", o->id,
		elapsed_min(&o->start_ts, &o->delivered_ts));
	pthread_mutex_unlock(&print_mutex);

	free(o);
	return NULL;
}

int main(int argc, char** argv) {
	if (argc != 3) {
		fprintf(stderr, "Χρήση: % s <αριθμός_παραγγελιών> <seed>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	int n_orders = atoi(argv[1]);
	unsigned int seed = (unsigned int)atoi(argv[2]);
	pthread_t* threads = malloc(n_orders * sizeof(*threads));

	/* Δημιουργία πελατών με τυχαία άφιξη */
	for (int i = 0; i < n_orders; i++) {
		if (i > 0) {
			int delay = rand_range_r(&seed, TorderLow, TorderHigh);
			sleep(delay);
		}
		Order* o = malloc(sizeof(*o));
		o->id = i + 1;
		o->seed = seed + i;
		pthread_create(&threads[i], NULL, order_thread, o);
	}
	for (int i = 0; i < n_orders; i++) pthread_join(threads[i], NULL);

	/* Τελικός απολογισμός χρόνων και εσόδων παραγγελιών */
	printf("\n=== Τελικός Απολογισμός ===\n");
	printf("Συνολικά έσοδα: %d ευρώ\n", total_income);
	printf("Μαργαρίτα: %d, Πεπερόνι: %d, Σπέσιαλ: %d\n", total_m, total_p, total_s);
	printf("Επιτυχείς παραγγελίες: %d, Αποτυχημένες παραγγελίες: %d\n",
		successful_orders, failed_orders);
	if (successful_orders > 0) {
		printf("Μέσος χρόνος εξυπηρέτησης: %.2f λεπτά\n",
			total_service_time / successful_orders);
		printf("Μέγιστος χρόνος εξυπηρέτησης: %.2f λεπτά\n",
			max_service_time);
		printf("Μέσος χρόνος κρυώματος: %.2f λεπτά\n",
			total_cool_time / successful_orders);
		printf("Μέγιστος χρόνος κρυώματος: %.2f λεπτά\n",
			max_cool_time);
	}
	free(threads);
	return 0;
}