#include <pebble.h>

#include "departure.h"

static Window *departure_window;
static MenuLayer *departure_menu_layer;
static TextLayer *departure_text_time;
static TextLayer *departure_text_station;
static TextLayer *departure_text_countdown;
static TextLayer *departure_text_direction;

static int NRows;
static departure Departure[DEPARTURE_LIST_NUM_ROWS];

static uint32_t StationID;
static char StationName[SIZE_STATION];
static char LineID[SIZE_LINEID];

static bool COUNTDOWN;

// forward declaration
static void update(void);

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  // return DEPARTURE_LIST_NUM_ROWS;
  return NRows;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return  DEPARTURE_LIST_CELL_HEIGHT;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  static char title[16], subtitle[20];
  int i = (int)cell_index->row;

  snprintf(title, sizeof(title), "%-3s   %2s", Departure[i].line, Departure[i].countdown);
  snprintf(subtitle, sizeof(subtitle), "%s", Departure[i].destination);
  menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
}

static void activate_countdown(int n) {
  Layer *window_layer = window_get_root_layer(departure_window);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "activate departure countdown");
  COUNTDOWN = true;

  layer_remove_from_parent((Layer*)departure_menu_layer);

  layer_add_child(window_layer, text_layer_get_layer(departure_text_direction));
  layer_add_child(window_layer, text_layer_get_layer(departure_text_countdown));

  snprintf(LineID, SIZE_LINEID, Departure[n].lineid);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "n = %d lineid = %s StationID = %ld", n, LineID, StationID);

  update();
}

static void deactivate_countdown(void) {
  Layer *window_layer = window_get_root_layer(departure_window);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "deactivate departure countdown");
  COUNTDOWN = false;

  layer_remove_from_parent((Layer*)departure_text_countdown);
  layer_remove_from_parent((Layer*)departure_text_direction);

  layer_add_child(window_layer, menu_layer_get_layer(departure_menu_layer));

  update();
}

static void select_click_handler(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  if (COUNTDOWN) {
    deactivate_countdown();
  } else {
    if (NRows > 0) {
      activate_countdown((int)cell_index->row);
    }
  }
}

static void departure_update_time(void) {
  // get a tm structure
  time_t t = time(NULL);
  struct tm *tick_time = localtime(&t);

  // create long-lived buffer
  static char buffer[] = "00:00:00";

  // write current hours and minutes into buffer
  strftime(buffer, sizeof("00:00:00"), "%H:%M:%S", tick_time);

  // display this on the TextLayer
  text_layer_set_text(departure_text_time, buffer);
}

void countdown_message_handler(DictionaryIterator *iterator, void *context) {
  static char destination[SIZE_DESTINATION], countdown[SIZE_COUNTDOWN];
  Tuple *msg;
  int n;

  msg = dict_find(iterator, MESSAGE_KEY_Count);
  n = msg->value->int8;
  if (n != 1)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "ERROR: something went wrong n ="+ n);

  msg = dict_find(iterator, MESSAGE_KEY_Countdown);
  snprintf(countdown, SIZE_COUNTDOWN, msg->value->cstring);
  msg = dict_find(iterator, MESSAGE_KEY_Direction);
  snprintf(destination, SIZE_DESTINATION, msg->value->cstring);
  filterChar(destination, 0x83);
  filterChar(destination, 0xc2);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Countdown = %s", countdown);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Destination = %s", destination);

  text_layer_set_text(departure_text_countdown, countdown);
  text_layer_set_text(departure_text_direction, destination);
}

void departure_message_handler(DictionaryIterator *iterator, void *context) {
  Tuple *msg;
  int i, n;

  msg = dict_find(iterator, MESSAGE_KEY_Count);
  n = msg->value->int8;
  if (n > DEPARTURE_LIST_NUM_ROWS)
    n = DEPARTURE_LIST_NUM_ROWS;

  NRows = n;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "departure list message received");
  uint8_t line_keys[n];
  msg = dict_find(iterator, MESSAGE_KEY_Line);
  memcpy(line_keys, msg->value->data, n);
  uint8_t destination_keys[n];
  msg = dict_find(iterator, MESSAGE_KEY_Direction);
  memcpy(destination_keys, msg->value->data, n);
  uint8_t countdown_keys[n];
  msg = dict_find(iterator, MESSAGE_KEY_Countdown);
  memcpy(countdown_keys, msg->value->data, n);
  uint8_t lineid_keys[n];
  msg = dict_find(iterator, MESSAGE_KEY_LineID);
  memcpy(lineid_keys, msg->value->data, n);

  for (i=0; i<n; i++) {
    msg = dict_find(iterator, line_keys[i]);
    strncpy(Departure[i].line, msg->value->cstring, SIZE_LINE);
    msg = dict_find(iterator, destination_keys[i]);
    strncpy(Departure[i].destination, msg->value->cstring, SIZE_DESTINATION);
    filterChar(Departure[i].destination, 0x83);
    filterChar(Departure[i].destination, 0xc2);
    msg = dict_find(iterator, countdown_keys[i]);
    snprintf(Departure[i].countdown, SIZE_COUNTDOWN, msg->value->cstring);
    msg = dict_find(iterator, lineid_keys[i]);
    strncpy(Departure[i].lineid, msg->value->cstring, SIZE_LINEID);
  }

  menu_layer_reload_data(departure_menu_layer);
}

static void update() {
  DictionaryIterator *iterator;

  app_message_outbox_begin(&iterator);

  if (COUNTDOWN) {
    dict_write_uint8(iterator, MESSAGE_KEY_FuncDeparture, 1);
    dict_write_uint8(iterator, MESSAGE_KEY_FuncDepartureList, 0);
    dict_write_uint32(iterator, MESSAGE_KEY_StationID, StationID);
    dict_write_cstring(iterator, MESSAGE_KEY_LineID, LineID);
  } else {
    dict_write_uint8(iterator, MESSAGE_KEY_FuncDepartureList, 1);
    dict_write_uint8(iterator, MESSAGE_KEY_FuncDeparture, 0);
    dict_write_uint32(iterator, MESSAGE_KEY_StationID, StationID);
  }

  app_message_outbox_send();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  departure_update_time();

  if (units_changed & MINUTE_UNIT) {
    update();
  }
}

static void departure_window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "departure window load");

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  NRows = 0;

  // create time TextLayer
  departure_text_time = text_layer_create(GRect(99, 0, 45, 30));
  text_layer_set_background_color(departure_text_time, GColorClear);
  text_layer_set_text_color(departure_text_time, GColorBlack);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(departure_text_time));

  // create station TextLayer
  departure_text_station = text_layer_create(GRect(0, 0, 98, 30));
  text_layer_set_background_color(departure_text_station, GColorClear);
  text_layer_set_text_color(departure_text_station, GColorBlack);
  text_layer_set_text(departure_text_station, StationName);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(departure_text_station));

  // Create Menu layer
  departure_menu_layer = menu_layer_create(GRect(bounds.origin.x, bounds.origin.y + 30, bounds.size.w, bounds.size.h - 30));
  menu_layer_set_click_config_onto_window(departure_menu_layer, window);
  menu_layer_set_callbacks(departure_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_click_handler,
  });
  layer_add_child(window_layer, menu_layer_get_layer(departure_menu_layer));

  // create countdown TextLayer
  departure_text_countdown = text_layer_create(GRect(0, 50, 144, 49));
  text_layer_set_background_color(departure_text_countdown, GColorClear);
  text_layer_set_text_color(departure_text_countdown, GColorBlack);
  text_layer_set_font(departure_text_countdown, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(departure_text_countdown, GTextAlignmentCenter);
  text_layer_set_text(departure_text_countdown, "0");

  // create direction TextLayer
  departure_text_direction = text_layer_create(GRect(0, 100, 144, 30));
  text_layer_set_background_color(departure_text_direction, GColorClear);
  text_layer_set_text_color(departure_text_direction, GColorBlack);
  text_layer_set_font(departure_text_direction, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(departure_text_direction, GTextAlignmentCenter);
  text_layer_set_text(departure_text_direction, "Loading ...");

  departure_update_time();
  // register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  COUNTDOWN = false;
}

static void departure_window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "departure window unload");

  tick_timer_service_unsubscribe();

  text_layer_destroy(departure_text_time);
  text_layer_destroy(departure_text_station);
  text_layer_destroy(departure_text_direction);
  text_layer_destroy(departure_text_countdown);
  menu_layer_destroy(departure_menu_layer);

  window_destroy(window);
  departure_window = NULL;
}

void departure_window_push(uint32_t id, char *name) {
  if (!departure_window) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "departure window create");
    departure_window = window_create();
    window_set_window_handlers(departure_window, (WindowHandlers) {
      .load = departure_window_load,
      .unload = departure_window_unload,
    });
  }

  // show the Window on the watch, with animated=true
  const bool animated = true;
  window_stack_push(departure_window, animated);

  StationID = id;
  snprintf(StationName, SIZE_STATION, name);

  update();
}