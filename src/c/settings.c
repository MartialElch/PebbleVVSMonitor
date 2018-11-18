#include <pebble.h>

#include "departure.h"

static Window *settings_window;
static MenuLayer *menu_layer;

static GBitmap *s_tick_black_bitmap, *s_tick_white_bitmap;
static char *MenuItem[SETTINGS_CHECKBOX_NUM_ROWS + 1];
static bool MenuSelection[SETTINGS_CHECKBOX_NUM_ROWS];

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return SETTINGS_CHECKBOX_NUM_ROWS + 1;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return  SETTINGS_CHECKBOX_CELL_HEIGHT;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  static char title[16];
  int i = (int)cell_index->row;

  if (i == SETTINGS_CHECKBOX_NUM_ROWS) {
    menu_cell_basic_draw(ctx, cell_layer, SETTINGS_CHECKBOX_SUBMIT_HINT, NULL, NULL);
  } else {
    snprintf(title, sizeof(title), "%s", MenuItem[i]);
    menu_cell_basic_draw(ctx, cell_layer, title, NULL, NULL);

    // Selected?
    GBitmap *ptr = s_tick_black_bitmap;
    if(menu_cell_layer_is_highlighted(cell_layer)) {
      graphics_context_set_stroke_color(ctx, GColorWhite);
      ptr = s_tick_white_bitmap;
    }

    GRect bounds = layer_get_bounds(cell_layer);
    GRect bitmap_bounds = gbitmap_get_bounds(ptr);

    // Draw checkbox
    GRect r = GRect(
      bounds.size.w - (2 * SETTINGS_CHECKBOX_BOX_SIZE),
      (bounds.size.h / 2) - (SETTINGS_CHECKBOX_BOX_SIZE / 2),
      SETTINGS_CHECKBOX_BOX_SIZE, SETTINGS_CHECKBOX_BOX_SIZE);
    graphics_draw_rect(ctx, r);
    if(MenuSelection[cell_index->row]) {
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, ptr, GRect(r.origin.x, r.origin.y - 3, bitmap_bounds.size.w, bitmap_bounds.size.h));
    }
  }
}

static void select_click_handler(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  int n = (int)cell_index->row;

  if (n == SETTINGS_CHECKBOX_NUM_ROWS) {
    persist_write_data(PKEY_SELECTION, MenuSelection, sizeof(MenuSelection));

    const bool animated = true;
    window_stack_pop(animated);
  } else {
    MenuSelection[n] = !MenuSelection[n];
    menu_layer_reload_data(menu_layer);
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  if (persist_exists(PKEY_SELECTION)) {
    persist_read_data(PKEY_SELECTION, MenuSelection, sizeof(MenuSelection));
  } else {
    for (int i=0; i<SETTINGS_CHECKBOX_NUM_ROWS; i++) {
      MenuSelection[i] = true;
    }
  }

  s_tick_black_bitmap = gbitmap_create_with_resource(RESOURCE_ID_TICK_BLACK);
  s_tick_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_TICK_WHITE);

  // Create Menu layer
  menu_layer = menu_layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, 165));
  menu_layer_set_click_config_onto_window(menu_layer, window);
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_click_handler,
  });

  MenuItem[0] = SETTINGS_CHECKBOX_CELL_0_HINT;
  MenuItem[1] = SETTINGS_CHECKBOX_CELL_1_HINT;
  MenuItem[2] = SETTINGS_CHECKBOX_CELL_2_HINT;
  MenuItem[3] = SETTINGS_CHECKBOX_CELL_3_HINT;

  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

static void window_unload(Window *window) {
  gbitmap_destroy(s_tick_black_bitmap);
  gbitmap_destroy(s_tick_white_bitmap);

  menu_layer_destroy(menu_layer);

  window_destroy(window);
  settings_window = NULL;
}

void settings_window_push(void) {
  if (!settings_window) {
    settings_window = window_create();
    window_set_window_handlers(settings_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }

  // show the Window on the watch, with animated=true
  const bool animated = true;
  window_stack_push(settings_window, animated);
}
