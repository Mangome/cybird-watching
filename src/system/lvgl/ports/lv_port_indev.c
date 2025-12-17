/**
 * @file lv_port_indev.c
 * LVGL 9.x input device port implementation
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"

/*********************
 *  STATIC VARIABLES
 *********************/
lv_indev_t* indev_encoder;
int32_t encoder_diff;
lv_indev_state_t encoder_state;

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void encoder_init(void);
static void encoder_read(lv_indev_t* indev, lv_indev_data_t* data);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /* Initialize your encoder if you have */
    encoder_init();

    /* Register a encoder input device - LVGL 9.x API */
    indev_encoder = lv_indev_create();
    lv_indev_set_type(indev_encoder, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_encoder, encoder_read);

    /* Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     * add objects to the group with `lv_group_add_obj(group, obj)`
     * and assign this input device to group to navigate in it:
     * `lv_indev_set_group(indev_encoder, group);` */
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Initialize your keypad */
static void encoder_init(void)
{
    /* Your code comes here */
    encoder_diff = 0;
    encoder_state = LV_INDEV_STATE_RELEASED;
}

/* Will be called by the library to read the encoder - LVGL 9.x API */
static void encoder_read(lv_indev_t* indev, lv_indev_data_t* data)
{
    data->enc_diff = encoder_diff;
    data->state = encoder_state;

    encoder_diff = 0;
    /* Reset state after reading */
    if(encoder_state == LV_INDEV_STATE_PRESSED) {
        encoder_state = LV_INDEV_STATE_RELEASED;
    }
}

/* Call this function in an interrupt to process encoder events (turn, press) */
void lv_port_encoder_handler(int32_t diff, bool pressed)
{
    encoder_diff += diff;
    if(pressed) {
        encoder_state = LV_INDEV_STATE_PRESSED;
    }
}
