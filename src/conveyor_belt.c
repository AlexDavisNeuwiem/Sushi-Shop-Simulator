#include <stdio.h>
#include <stdlib.h>

#include "conveyor_belt.h"
#include "virtual_clock.h"
#include "globals.h"
#include "menu.h"


void* conveyor_belt_run(void* arg) {
    /* NÃO PRECISA ALTERAR ESSA FUNÇÃO */
    conveyor_belt_t* self = (conveyor_belt_t*) arg;
    virtual_clock_t* virtual_clock = globals_get_virtual_clock();

    while (globals_get_sushi_shop_fechado() == 0) {
        msleep(CONVEYOR_IDLE_PERIOD/virtual_clock->clock_speed_multiplier);
        print_virtual_time(globals_get_virtual_clock());
        
        fprintf(stdout, GREEN "[INFO]" NO_COLOR " Conveyor belt started moving...\n");
        print_conveyor_belt(self);
        msleep(CONVEYOR_MOVING_PERIOD/virtual_clock->clock_speed_multiplier);

        /* O próximo "for" bloqueia todos os mutexes do vetor _pratos_mutex */
        /* Dessa forma, a esteira só irá se mover assim que mais ninguém tiver acesso à região crítica */

        /* Bloqueando os mutexes */
        for (int i=0; i<self->_size; i++) {
            pthread_mutex_lock(&self->_pratos_mutex[i]);
        }

        /* Movendo a esteira */
        int last = self->_food_slots[0];
        for (int i=0; i<self->_size-1; i++) {
            self->_food_slots[i] = self->_food_slots[i+1];
        }
        self->_food_slots[self->_size-1] = last;

        /* Desbloqueando os mutexes */
        for (int i=0; i<self->_size; i++) {
            pthread_mutex_unlock(&self->_pratos_mutex[i]);
        }

        print_virtual_time(globals_get_virtual_clock());
        fprintf(stdout, GREEN "[INFO]" NO_COLOR " Conveyor belt finished moving...\n");
        print_conveyor_belt(self);
    }
    pthread_exit(NULL);
}

conveyor_belt_t* conveyor_belt_init(config_t* config) {
    /* NÃO PRECISA ALTERAR ESSA FUNÇÃO */
    conveyor_belt_t* self = malloc(sizeof(conveyor_belt_t));
    if (self == NULL) {
        fprintf(stdout, RED "[ERROR] Bad malloc() at `conveyor_belt_t* conveyor_belt_init()`.\n" NO_COLOR);
        exit(EXIT_FAILURE);
    }
    self->_size = config->conveyor_belt_capacity;
    self->_seats = (int*) malloc(sizeof(int)* self->_size);
    self->_food_slots = (int*) malloc(sizeof(int)* self->_size);
    self->_pratos_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)* self->_size);
    for (int i=0; i<self->_size; i++) {
        self->_food_slots[i] = -1;
        self->_seats[i] = -1;
        pthread_mutex_init(&self->_pratos_mutex[i], NULL);
    }
    sem_init(&self->vagas_sem, 0, (self->_size) - 1);
    sem_init(&self->pratos_sem, 0, self->_size);
    pthread_mutex_init(&self->_seats_mutex, NULL);
    pthread_mutex_init(&self->_food_slots_mutex, NULL);
    pthread_create(&self->thread, NULL, conveyor_belt_run, (void *) self);
    print_conveyor_belt(self);
    return self;
}

void conveyor_belt_finalize(conveyor_belt_t* self) {
    /* NÃO PRECISA ALTERAR ESSA FUNÇÃO */
    pthread_join(self->thread, NULL);

    for (int i=0; i<self->_size; i++) {
        pthread_mutex_destroy(&self->_pratos_mutex[i]);
    }

    sem_destroy(&self->vagas_sem);
    sem_destroy(&self->pratos_sem);
    pthread_mutex_destroy(&self->_seats_mutex);
    pthread_mutex_destroy(&self->_food_slots_mutex);
    free(self->_seats);
    free(self->_food_slots);
    free(self->_pratos_mutex);
    free(self);
}

void print_conveyor_belt(conveyor_belt_t* self) {
    /* NÃO PRECISA ALTERAR ESSA FUNÇÃO */
    print_virtual_time(globals_get_virtual_clock());
    fprintf(stdout, BROWN "[DEBUG] Conveyor Belt " NO_COLOR "{\n");
    fprintf(stdout, BROWN "    _size" NO_COLOR ": %d\n", self->_size);
    fprintf(stdout, BROWN "    _food_slots" NO_COLOR ": [");
    for (int i=0; i<self->_size; i++) {
        if (i%25 == 0) {
            fprintf(stdout, NO_COLOR "\n        ");
        }
        switch (self->_food_slots[i]) {
            case -1:
                fprintf(stdout, NO_COLOR "%s, ", EMPTY_SLOT);
                break;
            case 0:
                fprintf(stdout, NO_COLOR "%s, ", SUSHI);
                break;
            case 1:
                fprintf(stdout, NO_COLOR "%s, ", DANGO);
                break;
            case 2:
                fprintf(stdout, NO_COLOR "%s, ", RAMEN);
                break;
            case 3:
                fprintf(stdout, NO_COLOR "%s, ", ONIGIRI);
                break;
            case 4:
                fprintf(stdout, NO_COLOR "%s, ", TOFU);
                break;
            default:
                fprintf(stdout, RED "[ERROR] Invalid menu item code in the Conveyor Belt.\n" NO_COLOR);
                exit(EXIT_FAILURE);
        }
    }
    fprintf(stdout, NO_COLOR "\n    ]\n");
    fprintf(stdout, BROWN "    _seats" NO_COLOR ": [");
    for (int i=0; i<self->_size; i++) {
        if (i%25 == 0) {
            fprintf(stdout, NO_COLOR "\n        ");
        }
        switch (self->_seats[i]) {
            case -1:
                fprintf(stdout, NO_COLOR "%s, ", EMPTY_SLOT);
                break;
            case 0:
                fprintf(stdout, NO_COLOR "%s, ", SUSHI_CHEF);
                break;
            case 1:
                fprintf(stdout, NO_COLOR "%s, ", CUSTOMER);
                break;
            default:
                fprintf(stdout, RED "[ERROR] Invalid seat state code in the Conveyor Belt.\n" NO_COLOR);
                exit(EXIT_FAILURE);
        }
    }
    fprintf(stdout, NO_COLOR "\n    ]\n");
    fprintf(stdout, NO_COLOR "}\n" NO_COLOR);
}
