#include <application.h>
#include <bc_onewire_relay.h>

#define SEND_DATA_INTERVAL (5 * 60 * 1000)
#define DEBOUNCE_TIME 1000

bool light1 = false;
bool light2 = false;
bool light3 = false;
bool light4 = false;
bool switch1_state = false;
bool switch2_state = false;
bool switch3_state = false;
bool switch4_state = false;
bool init = true;

// 1-wire relay instance
bc_onewire_relay_t relay;
// switches instancies
bc_switch_t switch1;
bc_switch_t switch2;
bc_switch_t switch3;
bc_switch_t switch4;

bool lights_get(void);
bool lights_set(bc_atci_param_t *param);
void switch_event_handler(bc_switch_t *self, bc_switch_event_t event, void *event_param);

void application_init(void)
{
    // Initialize AT command interface
    static const bc_atci_command_t commands[] = {
        {"$LIGHTS", NULL, lights_set, lights_get, NULL, ""},
        BC_ATCI_COMMAND_HELP
    };
    bc_atci_init(commands, BC_ATCI_COMMANDS_LENGTH(commands));

    // Initialize sensor module
    bc_module_sensor_init();
    // Pull Up for 1 wire
    bc_module_sensor_set_pull(BC_MODULE_SENSOR_CHANNEL_A, BC_MODULE_SENSOR_PULL_UP_4K7);

    // Initialize onewire relay
    bc_onewire_relay_init(&relay, BC_GPIO_P4, 0x00);

    // Initialize switches
    bc_switch_init(&switch1, BC_GPIO_P17, BC_SWITCH_TYPE_NO, BC_SWITCH_PULL_UP_DYNAMIC);
    bc_switch_set_event_handler(&switch1, switch_event_handler, NULL);
    bc_switch_set_debounce_time(&switch1, DEBOUNCE_TIME);
    bc_switch_init(&switch2, BC_GPIO_P16, BC_SWITCH_TYPE_NO, BC_SWITCH_PULL_UP_DYNAMIC);
    bc_switch_set_event_handler(&switch2, switch_event_handler, NULL);
    bc_switch_set_debounce_time(&switch2, DEBOUNCE_TIME);
    bc_switch_init(&switch3, BC_GPIO_P15, BC_SWITCH_TYPE_NO, BC_SWITCH_PULL_UP_DYNAMIC);
    bc_switch_set_event_handler(&switch3, switch_event_handler, NULL);
    bc_switch_set_debounce_time(&switch3, DEBOUNCE_TIME);
    bc_switch_init(&switch4, BC_GPIO_P14, BC_SWITCH_TYPE_NO, BC_SWITCH_PULL_UP_DYNAMIC);
    bc_switch_set_event_handler(&switch4, switch_event_handler, NULL);
    bc_switch_set_debounce_time(&switch4, DEBOUNCE_TIME);

    bc_scheduler_plan_current_relative(2000);
}

void application_task(void)
{
    if (init)
    {
        switch1_state = bc_switch_get_state(&switch1);
        switch2_state = bc_switch_get_state(&switch2);
        switch3_state = bc_switch_get_state(&switch3);
        switch4_state = bc_switch_get_state(&switch4);
        init = false;
    }

    lights_get();
    bc_scheduler_plan_current_relative(SEND_DATA_INTERVAL);
}

bool lights_get(void)
{
    bc_atci_printf("$STATUS:%d,%d,%d,%d",
        light1 ? 1 : 0,
        light2 ? 1 : 0,
        light3 ? 1 : 0,
        light4 ? 1 : 0);
    return true;
}

void lights_set_state()
{
    bc_onewire_relay_set_state(&relay, BC_ONEWIRE_RELAY_CHANNEL_Q1, light1);
    bc_onewire_relay_set_state(&relay, BC_ONEWIRE_RELAY_CHANNEL_Q2, light2);
    bc_onewire_relay_set_state(&relay, BC_ONEWIRE_RELAY_CHANNEL_Q3, light3);
    bc_onewire_relay_set_state(&relay, BC_ONEWIRE_RELAY_CHANNEL_Q4, light4);

    lights_get();
}

bool lights_set(bc_atci_param_t *param)
{
    if (param->length != 7)
    {
        return false;
    }
    if ((param->txt[0] != '0' && param->txt[0] != '1') || param->txt[1] != ',' ||
        (param->txt[2] != '0' && param->txt[2] != '1') || param->txt[3] != ',' ||
        (param->txt[4] != '0' && param->txt[4] != '1') || param->txt[5] != ',' ||
        (param->txt[6] != '0' && param->txt[6] != '1'))
    {
        return false;
    }

    light1 = param->txt[0] == '1';
    light2 = param->txt[2] == '1';
    light3 = param->txt[4] == '1';
    light4 = param->txt[6] == '1';

    lights_set_state();

    return true;
}

void switch_event_handler(bc_switch_t *self, bc_switch_event_t event, void *event_param)
{
    bool *switch_state;
    bool *light;
    if (self == &switch1)
    {
        switch_state = &switch1_state;
        light = &light1;
    }
    else if (self == &switch2)
    {
        switch_state = &switch2_state;
        light = &light2;
    }
    else if (self == &switch3)
    {
        switch_state = &switch3_state;
        light = &light3;
    }
    else
    {
        switch_state = &switch4_state;
        light = &light4;
    }

    bool state = bc_switch_get_state(self);
    if (state != *switch_state)
    {
        *switch_state = state;
        *light = !(*light);
        lights_set_state();
    }
}
