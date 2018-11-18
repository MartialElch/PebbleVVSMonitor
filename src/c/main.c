#include <pebble.h>

#include "departure.h"

static Window *main_window;
static MenuLayer *main_menu_layer;

static char *MenuItem[MAIN_MENU_NUM_ROWS];

static bool JSREADY = false;

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return MAIN_MENU_NUM_ROWS;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return  MAIN_MENU_CELL_HEIGHT;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  static char title[16];
  int i = (int)cell_index->row;

  snprintf(title, sizeof(title), "%s", MenuItem[i]);
  menu_cell_basic_draw(ctx, cell_layer, title, NULL, NULL);
}

static void select_click_handler(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  int i;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu Select %d", (int)cell_index->row);
  i = (int)cell_index->row;
  if (i == 0) {
    if (JSREADY)
      station_window_push();
  } else if (i == 1) {
    settings_window_push();
  }
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  Tuple *msg;
  
  msg = dict_find(iterator, MESSAGE_KEY_Ready);
  if (msg) {
    JSREADY = true;
  }

  msg = dict_find(iterator, MESSAGE_KEY_FuncStationList);
  if (msg) {
    station_message_handler(iterator, context);
  }

  msg = dict_find(iterator, MESSAGE_KEY_FuncDeparture);
  if (msg) {
    countdown_message_handler(iterator, context);
  }

  msg = dict_find(iterator, MESSAGE_KEY_FuncDepartureList);
  if (msg) {
    departure_message_handler(iterator, context);
  }

  msg = dict_find(iterator, MESSAGE_KEY_FuncServingLines);
  if (msg) {
    servinglines_message_handler(iterator, context);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "message dropped");
}

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "message failed");
}

static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "message sent");
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create Menu layer
  main_menu_layer = menu_layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h - STATION_LIST_CELL_HEIGHT ));
  menu_layer_set_click_config_onto_window(main_menu_layer, window);
  menu_layer_set_callbacks(main_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_click_handler,
  });

  MenuItem[0] = MAIN_MENU_CELL_0_HINT;
  MenuItem[1] = MAIN_MENU_CELL_1_HINT;

  layer_add_child(window_layer, menu_layer_get_layer(main_menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(main_menu_layer);
}

static void init(void) {
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  // show the Window on the watch, with animated=true
  const bool animated = true;
  window_stack_push(main_window, animated);

  // register callbacks
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_outbox_sent(outbox_sent_handler);

  // open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit(void) {
  window_destroy(main_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", main_window);

  app_event_loop();
  deinit();
}
