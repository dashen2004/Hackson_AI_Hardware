#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qm_struct_mapper.h"
#include "esp_log.h" // Assuming ESP environment

// 1. Define your structures
typedef struct {
    int id;
    char *name;
} sub_data_t;

typedef struct {
    int active;
    double score;
    sub_data_t info;
    int values[3];
} app_config_t;

// 2. Define the schema mappings
// Note: Define sub-structures first

// Map for sub_data_t
QM_STRUCT_BEGIN(sub_data_t)
    QM_FIELD_INT(sub_data_t, id)
    QM_FIELD_STRING(sub_data_t, name)
QM_STRUCT_END(sub_data_t)

// Map for app_config_t
QM_STRUCT_BEGIN(app_config_t)
    QM_FIELD_INT(app_config_t, active)
    QM_FIELD_DOUBLE(app_config_t, score)
    QM_FIELD_STRUCT(app_config_t, info, sub_data_t)
    QM_FIELD_ARRAY_INT(app_config_t, values, 3)
QM_STRUCT_END(app_config_t)

void struct_mapper_example(void) {
    printf("Starting Struct Mapper Example...\n");

    // Initialize data
    app_config_t config = {
        .active = 1,
        .score = 98.5,
        .info = { .id = 101, .name = "Test Module" },
        .values = { 10, 20, 30 }
    };

    // Serialize to JSON
    cJSON *json = qm_struct_to_json(&config, &QM_STRUCT_DESC_NAME(app_config_t));
    if (json) {
        char *str = cJSON_Print(json);
        printf("Serialized JSON:\n%s\n", str);
        free(str);
        // Don't delete json yet, we might use it or just delete it
    }

    // Deserialize to new struct
    app_config_t new_config;
    memset(&new_config, 0, sizeof(app_config_t));
    
    // Simulating generic JSON input
    if (json) {
        qm_json_to_struct(json, &new_config, &QM_STRUCT_DESC_NAME(app_config_t));
        
        printf("Deserialized Data:\n");
        printf("Active: %d\n", new_config.active);
        printf("Score: %.2f\n", new_config.score);
        printf("Info ID: %d\n", new_config.info.id);
        if (new_config.info.name) {
            printf("Info Name: %s\n", new_config.info.name);
            // Remember to free the string if it was allocated by strdup
            free(new_config.info.name); 
        }
        printf("Values: %d, %d, %d\n", new_config.values[0], new_config.values[1], new_config.values[2]);
        
        cJSON_Delete(json);
    }
}
