#include "resource.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

Resource* resource_create(const char* name, int total_units) {
    if (name == NULL || total_units <= 0) {
        return NULL;
    }

    Resource* resource = (Resource*)malloc(sizeof(Resource));
    if (resource == NULL) {
        return NULL;
    }

    resource->name = (char*)malloc(strlen(name) + 1);
    if (resource->name == NULL) {
        free(resource);
        return NULL;
    }

    strcpy(resource->name, name);
    resource->total_units = total_units;
    resource->available_units = total_units;
    resource->busy_units = 0;
    resource->user_data = NULL;

    return resource;
}

void resource_destroy(Resource* resource) {
    if (resource == NULL) {
        return;
    }

    free(resource->name);
    free(resource);
}

int resource_request(Resource* resource, int requested_units) {
    if (resource == NULL || requested_units <= 0) {
        return 0;
    }

    if (resource->available_units < requested_units) {
        return 0;  /* Not enough units available */
    }

    resource->available_units -= requested_units;
    resource->busy_units += requested_units;

    return requested_units;
}

bool resource_release(Resource* resource, int units_to_release) {
    if (resource == NULL || units_to_release <= 0) {
        return false;
    }

    if (units_to_release > resource->busy_units) {
        return false;  /* Cannot release more than busy units */
    }

    resource->busy_units -= units_to_release;
    resource->available_units += units_to_release;

    return true;
}

const char* resource_get_name(Resource* resource) {
    return (resource != NULL) ? resource->name : NULL;
}

int resource_get_total_units(Resource* resource) {
    return (resource != NULL) ? resource->total_units : 0;
}

int resource_get_available_units(Resource* resource) {
    return (resource != NULL) ? resource->available_units : 0;
}

int resource_get_busy_units(Resource* resource) {
    return (resource != NULL) ? resource->busy_units : 0;
}

bool resource_has_available_units(Resource* resource, int requested_units) {
    if (resource == NULL || requested_units <= 0) {
        return false;
    }

    return resource->available_units >= requested_units;
}

double resource_get_utilization(Resource* resource) {
    if (resource == NULL || resource->total_units == 0) {
        return 0.0;
    }

    return (double)resource->busy_units / (double)resource->total_units;
}

void resource_reset(Resource* resource) {
    if (resource == NULL) {
        return;
    }

    resource->available_units = resource->total_units;
    resource->busy_units = 0;
}

void resource_set_user_data(Resource* resource, void* user_data) {
    if (resource != NULL) {
        resource->user_data = user_data;
    }
}

void* resource_get_user_data(Resource* resource) {
    return (resource != NULL) ? resource->user_data : NULL;
}