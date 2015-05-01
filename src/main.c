#include <pebble.h>

#define MAX_COUNTDOWN 10
// size of text fields
#define SIZE_BUFFER    32
#define SIZE_STATION   20
#define SIZE_LINE      4
#define SIZE_DIRECTION 15
#define SIZE_COUNTDOWN 2

static Window *window;
static TextLayer *text_time;
static TextLayer *text_station;
static TextLayer *text_line[MAX_COUNTDOWN];
static TextLayer *text_direction[MAX_COUNTDOWN];
static TextLayer *text_countdown[MAX_COUNTDOWN];

static char stationID[SIZE_BUFFER];
bool stationID_set = false;

enum {
  KEY_NAME           = 0,
  KEY_COUNTDOWN      = 1,
  KEY_DIRECTION      = 2,
  KEY_COUNT          = 3,
  KEY_STATION        = 4,
  KEY_STATIONID      = 5,
  KEY_UPDATE_STATION = 6,
  KEY_UPDATE_MONITOR = 7
};

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  APP_LOG(APP_LOG_LEVEL_DEBUG, "update time start");

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(text_time, buffer);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "update time done %s", buffer);
}

static void update_station(char *name) {
  static char buffer[SIZE_BUFFER];
  int i;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "update station start %s", buffer);

  for (i=0; i<SIZE_BUFFER;i++) {
    buffer[i] = 0;
  }

  snprintf(buffer, SIZE_STATION, "%s", name);
  text_layer_set_text(text_station, buffer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "update station done");
}

static void update_monitor(int n, char *line, char *direction, char *countdown) {
  static char buffer_line[MAX_COUNTDOWN][SIZE_BUFFER];
  static char buffer_direction[MAX_COUNTDOWN][SIZE_BUFFER];
  static char buffer_countdown[MAX_COUNTDOWN][SIZE_BUFFER];
  char buffer[SIZE_BUFFER];
  char c;
  int i, j, k;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "update monitor start");

  // update line name
  j = 0;
  for (i=0; i<n;i++) {
    for (k=0; k<SIZE_LINE;k++) {
      buffer[k] = 0;
    }
    k = 0;
    while ((c = line[j++]) != ':') {
      if ( ((int)c == 131) || ((int)c == 194) ) {
      } else {
        buffer[k++] = c;
      }
    }
    snprintf(buffer_line[i], SIZE_LINE, "%s", buffer);
    text_layer_set_text(text_line[i], buffer_line[i]);
  }

  // update direction
  j = 0;
  for (i=0; i<n;i++) {
    for (k=0; k<SIZE_DIRECTION;k++) {
      buffer[k] = 0;
    }
    k = 0;
    while ((c = direction[j++]) != ':') {
      if ( ((int)c == 131) || ((int)c == 194) ) {
      } else {
        buffer[k++] = c;
      }
    }
    snprintf(buffer_direction[i], SIZE_DIRECTION, "%s", buffer);
    text_layer_set_text(text_direction[i], buffer_direction[i]);
  }

  // update countdown
  j = 0;
  for (i=0; i<n;i++) {
    for (k=0; k<SIZE_COUNTDOWN;k++) {
      buffer[k] = 0;
    }
    k = 0;
    while ((c = countdown[j++]) != ':') {
      if ( ((int)c == 131) || ((int)c == 194) ) {
      } else {
        buffer[k++] = c;
      }
    }
    snprintf(buffer_countdown[i], SIZE_COUNTDOWN, "%s", buffer);
    text_layer_set_text(text_countdown[i], buffer_countdown[i]);
  }

  // blank rest of lines in case max is not used
  for (i=n; i<MAX_COUNTDOWN;i++) {
    text_layer_set_text(text_line[i], "");
    text_layer_set_text(text_direction[i], "");
    text_layer_set_text(text_countdown[i], "");
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "update monitor done");
}

// get closest station name and ID
static void getStation() {
  DictionaryIterator *iter;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "getStation");

  // begin dictionary
  app_message_outbox_begin(&iter);
  // add UPDATE_STATION COMMAND
  dict_write_uint8(iter, KEY_UPDATE_STATION, 1);
  // send message
  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "getStation sent");
}

static void getDeparture() {
  DictionaryIterator *iter;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "getDeparture");

  // begin dictionary
  app_message_outbox_begin(&iter);
  if (stationID_set) {
    // add UPDATE_MONITOR COMMAND and stationID
    dict_write_uint8(iter, KEY_UPDATE_MONITOR, 1);
    dict_write_cstring(iter, KEY_STATIONID, stationID);
  } else {
    // add UPDATE_STATION COMMAND in case stationID is not set
    dict_write_uint8(iter, KEY_UPDATE_STATION, 1);
  }
  // send message
  app_message_outbox_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "getDeparture sent");
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  getDeparture();
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  static char *line, *direction, *station, *countdown;
  int n;
  bool up_station, up_monitor;

  n = 0;
  station = NULL;
  line = NULL;
  direction = NULL;
  countdown = NULL;

  up_station = false;
  up_monitor = false;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "message received");

  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_COUNT:
      n = t->value->int16;
      break;
    case KEY_NAME:
      line = t->value->cstring;
      break;
    case KEY_COUNTDOWN:
      countdown = t->value->cstring;
      break;
    case KEY_DIRECTION:
      direction = t->value->cstring;
      break;
    case KEY_STATION:
      station = t->value->cstring;
      break;
    case KEY_STATIONID:
      strncpy(stationID, t->value->cstring, SIZE_BUFFER);
      stationID_set = true;
      break;
    case KEY_UPDATE_STATION:
      up_station = true;
      break;
    case KEY_UPDATE_MONITOR:
      up_monitor = true;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }

  if (up_station) {
    update_station(station);
  } else if (up_monitor) {
    update_monitor(n, line, direction, countdown);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "message dropped");
}

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "message send failed");
}

static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "message sent");
}

static void main_window_load(Window *window) {
  // Create Time TextLayer
  text_time = text_layer_create(GRect(114, 0, 30, 30));
  text_layer_set_background_color(text_time, GColorClear);
  text_layer_set_text_color(text_time, GColorBlack);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_time));

  // Create Station TextLayer
  text_station = text_layer_create(GRect(0, 0, 144, 30));
  text_layer_set_background_color(text_station, GColorClear);
  text_layer_set_text_color(text_station, GColorBlack);
  text_layer_set_text(text_station, "Stadtmitte Fake");

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_station));

  // Create Line layer
  for (int i=0; i<MAX_COUNTDOWN; i++) {
    text_line[i] = text_layer_create(GRect(5, 15+i*15, 25, 30));
    text_layer_set_background_color(text_line[i], GColorClear);
    text_layer_set_text_color(text_line[i], GColorBlack);
    text_layer_set_text(text_line[i], "U00");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_line[i]));

    // Create Direction layer
    text_direction[i] = text_layer_create(GRect(30, 15+i*15, 99, 30));
    text_layer_set_background_color(text_direction[i], GColorClear);
    text_layer_set_text_color(text_direction[i], GColorBlack);
    text_layer_set_text(text_direction[i], "Wengertsteige");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_direction[i]));

    // Create Countdown layer
    text_countdown[i] = text_layer_create(GRect(132, 15+i*15, 10, 30));
    text_layer_set_background_color(text_countdown[i], GColorClear);
    text_layer_set_text_color(text_countdown[i], GColorBlack);
    text_layer_set_text(text_countdown[i], "0");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_countdown[i]));
  }
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(text_station);
  text_layer_destroy(text_time);
  for (int i=0; i<MAX_COUNTDOWN; i++) {
    text_layer_destroy(text_line[i]);
    text_layer_destroy(text_direction[i]);
    text_layer_destroy(text_countdown[i]);
  }
}

static void init() {
  // Create main Window element and assign to pointer
  window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(window, true);

  // register callbacks
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_outbox_sent(outbox_sent_handler);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // make sure to set display from the start
  update_time();

  // Update departure monitor on start
  getStation();
}

static void deinit() {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}