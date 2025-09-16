#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* Time simulation module for SIMSCRIPT */

/* Simulation time representation */
typedef double SimTime;

/* Event types */
typedef enum {
    EVENT_ARRIVAL,
    EVENT_DEPARTURE,
    EVENT_TIMEOUT,
    EVENT_CUSTOM
} EventType;

/* Event structure */
typedef struct Event {
    SimTime time;              /* Event time */
    EventType type;            /* Event type */
    int event_id;              /* Unique event identifier */
    void* data;                /* Event-specific data */
    void (*handler)(struct Event*); /* Event handler function */
    struct Event* next;        /* Next event in queue */
} Event;

/* Event queue (priority queue by time) */
typedef struct EventQueue {
    Event* head;
    int count;
} EventQueue;

/* Simulation clock */
typedef struct SimClock {
    SimTime current_time;
    SimTime end_time;
    bool running;
} SimClock;

/* Statistics collector */
typedef struct SimStats {
    int total_events;
    int processed_events;
    SimTime total_time;
    SimTime idle_time;
} SimStats;

/* Event queue operations */
EventQueue* event_queue_create(void);
void event_queue_destroy(EventQueue* queue);
bool event_queue_is_empty(EventQueue* queue);
void event_queue_schedule(EventQueue* queue, Event* event);
Event* event_queue_next(EventQueue* queue);
Event* event_queue_peek(EventQueue* queue);
void event_queue_cancel(EventQueue* queue, int event_id);

/* Event creation and management */
Event* event_create(SimTime time, EventType type, void* data,
                   void (*handler)(Event*));
void event_destroy(Event* event);

/* Simulation clock operations */
SimClock* sim_clock_create(SimTime start_time, SimTime end_time);
void sim_clock_destroy(SimClock* clock);
void sim_clock_advance(SimClock* clock, SimTime delta);
bool sim_clock_is_finished(SimClock* clock);
SimTime sim_clock_get_time(SimClock* clock);

/* Statistics operations */
SimStats* sim_stats_create(void);
void sim_stats_destroy(SimStats* stats);
void sim_stats_record_event(SimStats* stats, EventType type);
void sim_stats_record_idle_time(SimStats* stats, SimTime idle_time);
void sim_stats_print(SimStats* stats);

/* Simulation runner */
typedef struct Simulator {
    SimClock* clock;
    EventQueue* event_queue;
    SimStats* stats;
    bool paused;
} Simulator;

Simulator* simulator_create(SimTime start_time, SimTime end_time);
void simulator_destroy(Simulator* sim);
void simulator_schedule_event(Simulator* sim, Event* event);
void simulator_run(Simulator* sim);
void simulator_step(Simulator* sim);
void simulator_pause(Simulator* sim);
void simulator_resume(Simulator* sim);
bool simulator_is_running(Simulator* sim);

/* Utility functions */
SimTime sim_time_now(void);
void sim_time_delay(SimTime delay);
uint64_t sim_time_to_microseconds(SimTime time);
SimTime sim_time_from_microseconds(uint64_t microseconds);

#ifdef __cplusplus
}
#endif