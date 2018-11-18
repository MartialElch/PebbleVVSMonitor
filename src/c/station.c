#include <pebble.h>

#include "departure.h"

static Window *station_window;
static MenuLayer *station_menu_layer;
static TextLayer *station_message;

static station Station[STATION_LIST_NUM_ROWS];

static uint32_t StationID;
static char *StationName;

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return STATION_LIST_NUM_ROWS;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return  STATION_LIST_CELL_HEIGHT;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  static char title[16];
  int i = (int)cell_index->row;

  snprintf(title, sizeof(title), "%s", Station[i].name);
  menu_cell_basic_draw(ctx, cell_layer, title, NULL, NULL);
}

static void getServingLines(uint32_t id) {
  DictionaryIterator *iterator;
  bool MenuSelection[SETTINGS_CHECKBOX_NUM_ROWS];
  int productid[] = SETTINGS_PRODUCTID;
  int Selection;

  if (persist_exists(PKEY_SELECTION)) {
    persist_read_data(PKEY_SELECTION, MenuSelection, sizeof(MenuSelection));
  } else {
    for (int i=0; i<SETTINGS_CHECKBOX_NUM_ROWS; i++) {
      MenuSelection[i] = true;
    }
  }
  Selection = 0;
  for (int i=0; i<SETTINGS_CHECKBOX_NUM_ROWS; i++) {
    Selection = Selection + (MenuSelection[i] << productid[i]);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Selection = %d %d", Selection, MenuSelection[i]);
  }

  app_message_outbox_begin(&iterator);
  dict_write_uint8(iterator, MESSAGE_KEY_FuncServingLines, 1);
  dict_write_uint32(iterator, MESSAGE_KEY_StationID, id);
  dict_write_uint32(iterator, MESSAGE_KEY_Product, Selection);
  app_message_outbox_send();
}

static void select_click_handler(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu Select %d", (int)cell_index->row);
  StationID = Station[(int)cell_index->row].id;
  StationName = Station[(int)cell_index->row].name;
  getServingLines(StationID);
}

void station_message_handler(DictionaryIterator *iterator, void *context) {
  Tuple *msg;
  int n;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "station list message received");

  msg = dict_find(iterator, MESSAGE_KEY_Count);
  n = msg->value->int8;
  if (n > STATION_LIST_NUM_ROWS)
    n = STATION_LIST_NUM_ROWS;

  uint8_t name_keys[n];
  msg = dict_find(iterator, MESSAGE_KEY_StationName);
  memcpy(name_keys, msg->value->data, n);
  uint8_t id_keys[n];
  msg = dict_find(iterator, MESSAGE_KEY_StationID);
  memcpy(id_keys, msg->value->data, n);

  int i;
  for (i=0; i<n; i++) {
    // Station ID
    msg = dict_find(iterator, id_keys[i]);
    Station[i].id = atoi(msg->value->cstring);
    // Station Name
    msg = dict_find(iterator, name_keys[i]);
    snprintf(Station[i].name, SIZE_STATION, msg->value->cstring);
    filterChar(Station[i].name, 0x83);
    filterChar(Station[i].name, 0xc2);
  }

  menu_layer_reload_data(station_menu_layer);
}

void servinglines_message_handler(DictionaryIterator *iterator, void *context) {
  departure_window_push(StationID, StationName);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create Menu layer
  station_menu_layer = menu_layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h - STATION_LIST_CELL_HEIGHT ));
  menu_layer_set_click_config_onto_window(station_menu_layer, window);
  menu_layer_set_callbacks(station_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_click_handler,
  });
  layer_add_child(window_layer, menu_layer_get_layer(station_menu_layer));

  const GEdgeInsets message_insets = {.top = bounds.size.h - STATION_LIST_CELL_HEIGHT};
  station_message = text_layer_create(grect_inset(bounds, message_insets));
  text_layer_set_text_alignment(station_message, GTextAlignmentCenter);
  text_layer_set_text(station_message, STATION_MESSAGE_TEXT);
  layer_add_child(window_layer, text_layer_get_layer(station_message));
}

static void window_unload(Window *window) {
  menu_layer_destroy(station_menu_layer);

  window_destroy(window);
  station_window = NULL;
}

void station_window_push(void) {
  DictionaryIterator *iterator;

  if (!station_window) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "station window create");
    station_window = window_create();
    window_set_window_handlers(station_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }

  // show the Window on the watch, with animated=true
  const bool animated = true;
  window_stack_push(station_window, animated);

  app_message_outbox_begin(&iterator);
  dict_write_uint8(iterator, MESSAGE_KEY_FuncStationList, 1);
  app_message_outbox_send();
}
