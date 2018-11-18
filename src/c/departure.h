#define MAIN_MENU_NUM_ROWS    2
#define MAIN_MENU_CELL_HEIGHT 30
#define MAIN_MENU_HEIGHT      MAIN_MENU_NUM_ROWS*MAIN_MENU_CELL_HEIGHT
#define MAIN_MENU_CELL_0_HINT "Haltestelle"
#define MAIN_MENU_CELL_1_HINT "Einstellungen"
#define MAIN_MENU_CELL_2_HINT "Status"

#define SETTINGS_CHECKBOX_NUM_ROWS    4
#define SETTINGS_CHECKBOX_CELL_HEIGHT 30
#define SETTINGS_CHECKBOX_HEIGHT      SETTINGS_CHECKBOX_NUM_ROWS*(SETTINGS_CHECKBOX_NUM_ROWS + 1)
#define SETTINGS_CHECKBOX_BOX_SIZE    12
#define SETTINGS_CHECKBOX_CELL_0_HINT "S-Bahn"
#define SETTINGS_CHECKBOX_CELL_1_HINT "Stadtbahn"
#define SETTINGS_CHECKBOX_CELL_2_HINT "Bus"
#define SETTINGS_CHECKBOX_CELL_3_HINT "SEV BUS"
#define SETTINGS_CHECKBOX_SUBMIT_HINT "Submit"
#define SETTINGS_PRODUCTID {0, 2, 5, 13}

#define STATION_LIST_NUM_ROWS    10
#define STATION_LIST_CELL_HEIGHT 20
#define STATION_LIST_MENU_HEIGHT 135

#define STATION_MESSAGE_TEXT "Select Station"

#define DEPARTURE_LIST_NUM_ROWS    10
#define DEPARTURE_LIST_CELL_HEIGHT 40
#define DEPARTURE_LIST_MENU_HEIGHT 135

#define SIZE_STATION     40
#define SIZE_LINE        5
#define SIZE_DESTINATION 25
#define SIZE_COUNTDOWN   5
#define SIZE_LINEID      20

#define PKEY_SELECTION 1

typedef struct {
  char line[SIZE_LINE];
  char destination[SIZE_DESTINATION];
  char countdown[SIZE_COUNTDOWN];
  char lineid[SIZE_LINEID];
} departure;

typedef struct {
  char name[SIZE_STATION];
  int32_t id;
} station;

// departure.c
void countdown_message_handler(DictionaryIterator *iterator, void *context);
void departure_message_handler(DictionaryIterator *iterator, void *context);
void servinglines_message_handler(DictionaryIterator *iterator, void *context);
void station_message_handler(DictionaryIterator *iterator, void *context);
void departure_window_push(uint32_t id, char *name);

// function.c
void filterChar(char *str, char garbage);

// settings.c
void settings_window_push(void);

// station.c
void station_window_push(void);
