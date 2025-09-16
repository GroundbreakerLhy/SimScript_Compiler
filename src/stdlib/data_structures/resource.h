#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Resource data structure for SIMSCRIPT simulation */
typedef struct Resource {
    char* name;           /* Resource name */
    int total_units;      /* Total number of resource units */
    int available_units;  /* Currently available units */
    int busy_units;       /* Units currently in use */
    void* user_data;      /* Optional user data */
} Resource;

/* Create a new resource */
Resource* resource_create(const char* name, int total_units);

/* Destroy a resource */
void resource_destroy(Resource* resource);

/* Request resource units (returns number of units allocated, 0 if none available) */
int resource_request(Resource* resource, int requested_units);

/* Release resource units */
bool resource_release(Resource* resource, int units_to_release);

/* Get resource name */
const char* resource_get_name(Resource* resource);

/* Get total units */
int resource_get_total_units(Resource* resource);

/* Get available units */
int resource_get_available_units(Resource* resource);

/* Get busy units */
int resource_get_busy_units(Resource* resource);

/* Check if resource has available units */
bool resource_has_available_units(Resource* resource, int requested_units);

/* Get utilization percentage (0.0 to 1.0) */
double resource_get_utilization(Resource* resource);

/* Reset resource to initial state */
void resource_reset(Resource* resource);

/* Set user data */
void resource_set_user_data(Resource* resource, void* user_data);

/* Get user data */
void* resource_get_user_data(Resource* resource);

#ifdef __cplusplus
}
#endif