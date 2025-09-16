#include "time_simulation.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Global simulation time (for single-threaded simulations) */
static SimTime global_sim_time = 0.0;

/* Event ID counter */
static int next_event_id = 1;

/* Event queue implementation */
EventQueue* event_queue_create(void) {
    EventQueue* queue = (EventQueue*)malloc(sizeof(EventQueue));
    if (queue == NULL) {
        return NULL;
    }

    queue->head = NULL;
    queue->count = 0;
    return queue;
}

void event_queue_destroy(EventQueue* queue) {
    if (queue == NULL) {
        return;
    }

    Event* current = queue->head;
    while (current != NULL) {
        Event* next = current->next;
        event_destroy(current);
        current = next;
    }

    free(queue);
}

bool event_queue_is_empty(EventQueue* queue) {
    return queue == NULL || queue->head == NULL;
}

void event_queue_schedule(EventQueue* queue, Event* event) {
    if (queue == NULL || event == NULL) {
        return;
    }

    event->event_id = next_event_id++;

    if (queue->head == NULL || event->time < queue->head->time) {
        event->next = queue->head;
        queue->head = event;
    } else {
        Event* current = queue->head;
        while (current->next != NULL && current->next->time <= event->time) {
            current = current->next;
        }
        event->next = current->next;
        current->next = event;
    }

    queue->count++;
}

Event* event_queue_next(EventQueue* queue) {
    if (event_queue_is_empty(queue)) {
        return NULL;
    }

    Event* event = queue->head;
    queue->head = event->next;
    queue->count--;

    event->next = NULL;
    return event;
}

Event* event_queue_peek(EventQueue* queue) {
    return (queue != NULL) ? queue->head : NULL;
}

void event_queue_cancel(EventQueue* queue, int event_id) {
    if (queue == NULL || queue->head == NULL) {
        return;
    }

    if (queue->head->event_id == event_id) {
        Event* cancelled = queue->head;
        queue->head = cancelled->next;
        queue->count--;
        event_destroy(cancelled);
        return;
    }

    Event* current = queue->head;
    while (current->next != NULL) {
        if (current->next->event_id == event_id) {
            Event* cancelled = current->next;
            current->next = cancelled->next;
            queue->count--;
            event_destroy(cancelled);
            return;
        }
        current = current->next;
    }
}

/* Event management */
Event* event_create(SimTime time, EventType type, void* data,
                   void (*handler)(Event*)) {
    Event* event = (Event*)malloc(sizeof(Event));
    if (event == NULL) {
        return NULL;
    }

    event->time = time;
    event->type = type;
    event->event_id = 0;  /* Will be set by event_queue_schedule */
    event->data = data;
    event->handler = handler;
    event->next = NULL;

    return event;
}

void event_destroy(Event* event) {
    if (event != NULL) {
        free(event);
    }
}

/* Simulation clock */
SimClock* sim_clock_create(SimTime start_time, SimTime end_time) {
    SimClock* clock = (SimClock*)malloc(sizeof(SimClock));
    if (clock == NULL) {
        return NULL;
    }

    clock->current_time = start_time;
    clock->end_time = end_time;
    clock->running = false;

    global_sim_time = start_time;

    return clock;
}

void sim_clock_destroy(SimClock* clock) {
    if (clock != NULL) {
        free(clock);
    }
}

void sim_clock_advance(SimClock* clock, SimTime delta) {
    if (clock == NULL || delta < 0.0) {
        return;
    }

    clock->current_time += delta;
    global_sim_time = clock->current_time;
}

bool sim_clock_is_finished(SimClock* clock) {
    return clock == NULL || clock->current_time >= clock->end_time;
}

SimTime sim_clock_get_time(SimClock* clock) {
    return (clock != NULL) ? clock->current_time : global_sim_time;
}

/* Statistics */
SimStats* sim_stats_create(void) {
    SimStats* stats = (SimStats*)malloc(sizeof(SimStats));
    if (stats == NULL) {
        return NULL;
    }

    stats->total_events = 0;
    stats->processed_events = 0;
    stats->total_time = 0.0;
    stats->idle_time = 0.0;

    return stats;
}

void sim_stats_destroy(SimStats* stats) {
    if (stats != NULL) {
        free(stats);
    }
}

void sim_stats_record_event(SimStats* stats, EventType type) {
    if (stats == NULL) {
        return;
    }

    stats->total_events++;
    stats->processed_events++;
}

void sim_stats_record_idle_time(SimStats* stats, SimTime idle_time) {
    if (stats != NULL) {
        stats->idle_time += idle_time;
    }
}

void sim_stats_print(SimStats* stats) {
    if (stats == NULL) {
        return;
    }

    printf("Simulation Statistics:\n");
    printf("  Total Events: %d\n", stats->total_events);
    printf("  Processed Events: %d\n", stats->processed_events);
    printf("  Total Simulation Time: %.3f\n", stats->total_time);
    printf("  Idle Time: %.3f (%.1f%%)\n",
           stats->idle_time,
           stats->total_time > 0.0 ? (stats->idle_time / stats->total_time) * 100.0 : 0.0);
}

/* Simulator */
Simulator* simulator_create(SimTime start_time, SimTime end_time) {
    Simulator* sim = (Simulator*)malloc(sizeof(Simulator));
    if (sim == NULL) {
        return NULL;
    }

    sim->clock = sim_clock_create(start_time, end_time);
    sim->event_queue = event_queue_create();
    sim->stats = sim_stats_create();
    sim->paused = false;

    if (sim->clock == NULL || sim->event_queue == NULL || sim->stats == NULL) {
        simulator_destroy(sim);
        return NULL;
    }

    return sim;
}

void simulator_destroy(Simulator* sim) {
    if (sim == NULL) {
        return;
    }

    sim_clock_destroy(sim->clock);
    event_queue_destroy(sim->event_queue);
    sim_stats_destroy(sim->stats);
    free(sim);
}

void simulator_schedule_event(Simulator* sim, Event* event) {
    if (sim == NULL || event == NULL) {
        return;
    }

    event_queue_schedule(sim->event_queue, event);
    sim_stats_record_event(sim->stats, event->type);
}

void simulator_run(Simulator* sim) {
    if (sim == NULL) {
        return;
    }

    sim->clock->running = true;
    sim->paused = false;

    while (!sim_clock_is_finished(sim->clock) && !sim->paused) {
        simulator_step(sim);
    }

    sim->clock->running = false;
}

void simulator_step(Simulator* sim) {
    if (sim == NULL || event_queue_is_empty(sim->event_queue)) {
        return;
    }

    Event* event = event_queue_next(sim->event_queue);
    if (event == NULL) {
        return;
    }

    /* Advance clock to event time */
    SimTime time_advance = event->time - sim->clock->current_time;
    if (time_advance > 0.0) {
        sim_clock_advance(sim->clock, time_advance);
        sim_stats_record_idle_time(sim->stats, time_advance);
    }

    /* Execute event handler */
    if (event->handler != NULL) {
        event->handler(event);
    }

    /* Update statistics */
    sim->stats->total_time = sim->clock->current_time;

    event_destroy(event);
}

void simulator_pause(Simulator* sim) {
    if (sim != NULL) {
        sim->paused = true;
        sim->clock->running = false;
    }
}

void simulator_resume(Simulator* sim) {
    if (sim != NULL) {
        sim->paused = false;
        sim->clock->running = true;
    }
}

bool simulator_is_running(Simulator* sim) {
    return sim != NULL && sim->clock->running && !sim->paused;
}

/* Utility functions */
SimTime sim_time_now(void) {
    return global_sim_time;
}

void sim_time_delay(SimTime delay) {
    if (delay > 0.0) {
        global_sim_time += delay;
    }
}

uint64_t sim_time_to_microseconds(SimTime time) {
    return (uint64_t)(time * 1000000.0);
}

SimTime sim_time_from_microseconds(uint64_t microseconds) {
    return (SimTime)microseconds / 1000000.0;
}