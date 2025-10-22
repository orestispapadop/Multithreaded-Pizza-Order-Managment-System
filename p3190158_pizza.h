#pragma
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* Διαθέσιμοι Πόροι */
#define Ntel       2    /* τηλεφωνητές */
#define Ncook      2    /* μάγειρες */
#define Noven      10   /* φούρνοι */
#define Npacker    2    /* συσκευαστές */
#define Ndeliverer 10   /* διανομείς */

/* Διάστημα τυχαίων αφίξεων (δευτερόλεπτα) ανά παραγγελία */
#define TorderLow   1   /* ελάχιστη καθυστέρηση */
#define TorderHigh  5   /* μέγιστη καθυστέρηση */

/* Χρόνοι πληρωμής(δευτερόλεπτα) */
#define TpaymentLow 1
#define TpaymentHigh 3

/* Πιθανότητα αποτυχίας πληρωμής (1-100%) */
#define Pfail      5

/* Πλήθος πτσών ανά παραγγελία (τυχαίο) */
#define NorderLow  1
#define NorderHigh 5

/* Χρόνοι προετοιμασίας/ψήσιματος/συσκευασίας (δευτερόλεπτα) */
#define Tprep      1
#define Tbake      10
#define Tpack      1

/* Χρόνοι παράδοσης (δευτερόλεπτα) */
#define TdelLow    10
#define TdelHigh   15

/* Πιθανότητες επιλογής τύπου πίτσας (%) */
#define Pm         45       /* Μαργαρίτα */
#define Pp         35       /* Πεπερόνι */
#define Ps         20       /* Σπέσιαλ */

/* Τιμή (ευρώ) για κάθε τύπο πίτσας */
#define Cm         12
#define Cp         14
#define Cs         15

enum PizzaType { MARGHERITA, PEPPERONI, SPECIAL };

/* Εξωτερικές μεταβλητές για πόρους */
extern int available_tels;
extern int available_cooks;
extern int available_ovens;
extern int available_packers;
extern int available_deliverers;

/* Διαθέσιμα mutex/ cond-var */
extern pthread_mutex_t tel_mutex;
extern pthread_cond_t  tel_cond;
extern pthread_mutex_t cook_mutex;
extern pthread_cond_t  cook_cond;
extern pthread_mutex_t oven_mutex;
extern pthread_cond_t  oven_cond;
extern pthread_mutex_t packer_mutex;
extern pthread_cond_t  packer_cond;
extern pthread_mutex_t deliverer_mutex;
extern pthread_cond_t  deliverer_cond;
extern pthread_mutex_t stats_mutex;
extern pthread_mutex_t print_mutex;
extern pthread_mutex_t time_mutex;

extern int total_income;            /* συνολικά έσοδα */
extern int total_m;                 /* πωλήσεις Margherita */
extern int total_p;                 /* πωλήσεις Pepperoni */
extern int total_s;                 /* πωλήσεις Special */
extern int successful_orders;       /* επιτυχείς */
extern int failed_orders;           /* αποτυχίες */


extern double total_service_time;   /* συνολικά χρόνος εξυπηρέτισης */
extern double max_service_time;     /* μέγιστος χρόνος εξυπηρέτισης μίας παραγγελίας */
extern double total_cool_time;      /* συνολικός χρόνος κρυώματος */
extern double max_cool_time;        /* μέγιστος χρόνος κρυώματος */