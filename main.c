#include <pebble.h>
#include <string.h>
#include <stdlib.h>

#define SECTIONS 4
#define FIRST_SECTION 4
#define SECOND_SECTION 2
#define THIRD_SECTION 2
#define FOURTH_SECTION 3
#define sleepTime 3000
#define sleepTime2 1000

static Window *window;
static MenuLayer *s_menu_layer;
static TextLayer *hello_layer;
static char msg[100];
static AppTimer *timer;
static AppTimer *timer2;
static int flag = 0;
static int flag2 = 0;


static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return SECTIONS;
}

//callback for layout of the menu sections 
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0:
            return FIRST_SECTION;
        case 1:
            return SECOND_SECTION;
        case 2:
            return THIRD_SECTION;
        case 3:
            return FOURTH_SECTION;
        default:
            return 0;
    }
}

//callback for the header height of the layer
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

//callback for the header information of each section
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0:
            menu_cell_basic_header_draw(ctx, cell_layer, "==Temp Info==");
            break;
        case 1:
            menu_cell_basic_header_draw(ctx, cell_layer, "==Scale Convert== ");
            break;
        case 2:
            menu_cell_basic_header_draw(ctx, cell_layer, "==Arduino==");
            break;
        case 3:
            menu_cell_basic_header_draw(ctx, cell_layer, "==Aditional Features== ");
            break;
    }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Determine which section we're going to draw in
    switch (cell_index->section) {
        case 0:
            // Use the row to specify which item we'll draw
            switch (cell_index->row) {
                case 0:
                // There is title draw for something more simple than a basic menu item
                    menu_cell_basic_draw(ctx, cell_layer, "Current Temp", "current", NULL);
                    break;
                case 1:
                    menu_cell_basic_draw(ctx, cell_layer, "Average Temp", "avg", NULL);
                    break;
                case 2:
                    menu_cell_basic_draw(ctx, cell_layer, "Highest Temp", "max", NULL);
                    break;
                case 3:
                    menu_cell_basic_draw(ctx, cell_layer, "Lowest Temp", "min", NULL);
                    break;
            }
            break;
        case 1:
            switch (cell_index->row) {
                case 0:
                    menu_cell_basic_draw(ctx, cell_layer, "Celsius Temp", "C", NULL);
                    break;
                case 1:
                    menu_cell_basic_draw(ctx, cell_layer, "Fahrenheit Temp", "F", NULL);
                    break;
            }
            break;
        case 2:
            switch (cell_index->row) {
                case 0:
                    menu_cell_basic_draw(ctx, cell_layer, "Stand By", "off", NULL);
                    break;
                case 1:
                    menu_cell_basic_draw(ctx, cell_layer, "Active", "on", NULL);
                    break;
            }
            break;
        case 3:
            switch (cell_index->row){
                case 0:
                    menu_cell_basic_draw(ctx, cell_layer, "Phili Whether", "outdoor", NULL);
                    break;
                case 1:
                    menu_cell_basic_draw(ctx, cell_layer, "Show Outdoor", "on/off", NULL);
                    break;
                case 2:
                    menu_cell_basic_draw(ctx, cell_layer, "Motion Sensor", "detecting", NULL);
                    break;
            }
            break;
    }
}


void out_sent_handler(DictionaryIterator *sent, void *context) {
  // outgoing message was delivered -- do nothing
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
  text_layer_set_text(hello_layer, "Pebble watch is not connected with phone!");
}


void in_received_handler(DictionaryIterator *received, void *context)
{
  //  incoming message received
  // looks for key #0 in the incoming message
  int key = 0;
  Tuple *text_tuple = dict_find(received, key);
  if (text_tuple) {
    if (text_tuple->value) {
      // put it in this global variable
      strcpy(msg, text_tuple->value->cstring);
      if(strcmp(msg, "Motion Detected!!") == 0){
        vibes_long_pulse();
      }
    }
    else strcpy(msg, "no value!");

    text_layer_set_text(hello_layer, msg);
  }
  else {
    text_layer_set_text(hello_layer, "no message!");
  }
}



void in_dropped_handler(AppMessageResult reason, void *context) {
// incoming message dropped
text_layer_set_text(hello_layer, "Error in!");
}

//this is the callback function for click handler which send out message for user operation 
static void send_message(char *msg1, char *msg2) {
  flag = 0;//set the flag to non-auto updating mode
  flag2 = 0;//set the flag to non-updating motion sensor
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  
  int key = 0;
  Tuplet value = TupletCString(key, msg1);
  dict_write_tuplet(iter, &value);
  app_message_outbox_send();//send request
  
  strcpy(msg, msg2);
  text_layer_set_text(hello_layer, msg);
  //layer_mark_dirty(hello_layer);
}

//this is the time callback for updating the most recent temperature from the sensor
static void timer_callback(void *data) {
  if(flag == 1){
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);//send request
  
    char* msg1 = (char*) data;
    int key = 0;
    Tuplet value = TupletCString(key, msg1);
    dict_write_tuplet(iter, &value);
    app_message_outbox_send();

    //register the global timer recursively with callback function whenever the flag indicates updating.    
    timer = app_timer_register(sleepTime, timer_callback, msg1);
  }
}

//this is the callback function for current temperature click handler.
static void update_temperature(char *msg1, char *msg2) {
  flag = 1;//set the flag to auto updating mode
  flag2 = 0;//set the flag to non-motion sensing mode
  strcpy(msg, msg2);
  text_layer_set_text(hello_layer, msg);
  
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  
  int key = 0;
  Tuplet value = TupletCString(key, msg1);
  dict_write_tuplet(iter, &value);
  app_message_outbox_send();
  
  timer = app_timer_register(sleepTime, timer_callback, msg1);//register for the global timer with the callback function.

}

//this is the time callback for updating the most recent temperature from the sensor
static void sensor_callback(void *data) {
  if(flag2 == 1){
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);//send request
  
    char* msg1 = (char*) data;
    int key = 0;
    Tuplet value = TupletCString(key, msg1);
    dict_write_tuplet(iter, &value);
    app_message_outbox_send();

    //register the global timer recursively with callback function whenever the flag indicates updating.    
    timer2 = app_timer_register(sleepTime2, sensor_callback, msg1);
  }
}

//this is the callback function for motion sensor click handler.
static void update_sensor(char *msg1, char *msg2) {
  flag = 0;//set the flag to non-auto updating mode
  flag2 = 1;//set the flag to auto sensing motion
  strcpy(msg, msg2);
  text_layer_set_text(hello_layer, msg);
  
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  
  int key = 0;
  Tuplet value = TupletCString(key, msg1);
  dict_write_tuplet(iter, &value);
  app_message_outbox_send();
  
  timer2 = app_timer_register(sleepTime2, sensor_callback, msg1);//register for the global timer with the callback function.

}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    // Use the row to specify which item will receive the select action
    switch (cell_index->section) {
        case 0:
            // Use the row to specify which item we'll draw
            switch (cell_index->row) {
                case 0:
                    update_temperature("CurTemp", "Current Temp: ");
                    break;
                case 1:
                    send_message("AvgTemp", "Average Temp: ");
                    break;
                case 2:
                    send_message("MaxTemp", "Highest Temp: ");
                    break;
                case 3:
                    send_message("MinTemp", "Lowest Temp: ");
                    break;
            }
            break;
        case 1:
            switch (cell_index->row) {
                case 0:
                    send_message("celsius", "Temp shown in Celsius");
                    break;
                case 1:
                    send_message("fahrenheit", "Temp shown in Fahrenheit");
                    break;
            }
            break;
        case 2:
            switch (cell_index->row) {
                case 0:
                    send_message("off", "Arduino stands off.");
                    break;
                case 1:
                    send_message("on", "Arduino turns on.");
                    break;
            }
            break;
        case 3:
            switch (cell_index->row){
                case 0:
                    send_message("whether", "Temp outdoor: ");
                    break;
                case 1:
                    send_message("outdoor", "Turn on/off outdoor temp on Arduino.");
                    break;
                case 2:
                    update_sensor("motion", "Begin Detecting Motion Sensor.");
                    break;
            }
            break;
    }
}


//set the window size and format
static void window_load(Window *window) {
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  hello_layer = text_layer_create((GRect)
  { .origin = { 0, bounds.size.h * 3.5 / 5 }, //72
  .size = { bounds.size.w, bounds.size.h * 2 / 5 } });
  
//   text_layer_set_text(hello_layer, "Hello World!!");
//   text_layer_set_text_alignment(hello_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(hello_layer));

  
  s_menu_layer = menu_layer_create((GRect) { .origin = { 0, bounds.size.h * 0 / 5}, .size = { bounds.size.w , bounds.size.h * 3 / 5  } });
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
        .select_click = menu_select_callback,
  });
  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
}

//distroy the menu and hello layer
static void window_unload(Window *window) {
  text_layer_destroy(hello_layer);
  menu_layer_destroy(s_menu_layer);
}


//initialize the window and click handler
static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
  .load = window_load,
  .unload = window_unload,
  });
  
  
  // for registering AppMessage handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
    
  const bool animated = true;
  window_stack_push(window, animated);
}

/* deinit */
static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}