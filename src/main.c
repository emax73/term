#include <pebble.h>
#include "mktime.h"  

Window *root_window;
Layer *root_layer;
Layer *time_layer, *calendar_layer, *battery_rect_layer, *decorations_layer;
TextLayer *date_layer, *battery_text_layer;
GFont *date_font, *battery_font, *calendar_font, *calendar_bd_font;
int battery_percent;

BitmapLayer *bt_layer, *battery_layer;
GBitmap  *bt_on_bitmap, *bt_off_bitmap, * battery_bitmap;

static const uint32_t const lost_vibe[] = { 1000, 1000, 1000, 1000, 1000 };
VibePattern lost_vibe_pat = {
  .durations = lost_vibe,
  .num_segments = ARRAY_LENGTH(lost_vibe),
};

time_t seconds; // Today
time_t begin_seconds; // Monday previous week

#define WIDTH 144
#define HEIGHT 168 
  
#define DIGITS_Y 24
#define LINE_H 3 
#define BATTERY_DY 6  

#define CALENDAR_Y (HEIGHT - CALENDAR_H * 4 - 1)
#define CALENDAR_X0 2
#define CALENDAR_W 20
#define CALENDAR_H 20
#define CALENDAR_DX 1  
#define CALENDAR_DY -2  
  
#define SECONDS_IN_DAY 86400  

static char time_text[] = "00:00:00", date_text[]  = "                                        ",
bat_text[] = "100%";
int days0 = -1, days = 0;

char * weekdays[] = {
  "ВОСКР.",
  "ПОНЕД.",
  "ВТОРН.",
  "СРЕДА",
  "ЧЕТВ.",
  "ПЯТН.",
  "СУББ."
};

char * weekd[] = {
  "ПН",
  "ВТ",
  "СР",
  "ЧТ",
  "ПТ",
  "СБ",
  "ВС"
};

char * months[] = {
  "ЯНВАР.",
  "ФЕВР.",
  "МАРТА",
  "АПРЕЛ.",
  "МАЯ",
  "ИЮНЯ",
  "ИЮЛЯ",
  "АВГУС.",
  "СЕНТ.",
  "ОКТЯБ.",
  "НОЯБР.",
  "ДЕКАБ."
};


GBitmap *digits_big[11], *digits_small[10];

void load_resources() {
  digits_big[0] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_0);
  digits_big[1] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_1);
  digits_big[2] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_2);
  digits_big[3] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_3);
  digits_big[4] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_4);
  digits_big[5] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_5);
  digits_big[6] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_6);
  digits_big[7] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_7);
  digits_big[8] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_8);
  digits_big[9] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_9);
  digits_big[10] = gbitmap_create_with_resource(RESOURCE_ID_TER_BIG_S);

  digits_small[0] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_0);
  digits_small[1] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_1);
  digits_small[2] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_2);
  digits_small[3] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_3);
  digits_small[4] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_4);
  digits_small[5] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_5);
  digits_small[6] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_6);
  digits_small[7] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_7);
  digits_small[8] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_8);
  digits_small[9] = gbitmap_create_with_resource(RESOURCE_ID_TER_SMALL_9);

}

void unload_resources() {
  int i;
  for (i = 0; i < 11; i++) {
    gbitmap_destroy(digits_big[i]);
  }
  for (i = 0; i < 10; i++) {
    gbitmap_destroy(digits_small[i]);
  }
}

static void time_layer_update_proc(struct Layer *layer, GContext *ctx) {
  int code;
  char sym;
  int x = 0, y = 0, w, h = 21, dx = 3;
  for (int i=0; i < 5; i++) {
    sym = time_text[i];
    if (sym == ':') {
      code = 10;
      w = 8;
    } else {
      code = sym - 48;
      w = 23;
    }  
    graphics_draw_bitmap_in_rect(ctx, digits_big[code],GRect(x,y,w,h));
    x += w + dx;
  }
  y = 8; w = 13; h =13; 
  for (int i = 6; i < 8; i++) {
    sym = time_text[i];
    code = sym - 48;
    graphics_draw_bitmap_in_rect(ctx, digits_small[code],GRect(x,y,w,h));
    x += w + dx;
  }
}

void calculate_calendar(struct tm * tick_time) {
    int week_day = 0;
  
    seconds = pebble_mktime(tick_time);
  
    week_day = tick_time->tm_wday;
    if (!week_day) week_day = 7;
    week_day--; // 0 - Monday, 6 - Sunday
    begin_seconds = seconds - SECONDS_IN_DAY * (week_day + 7); // Monday previous week
}

static void calendar_layer_update_proc(struct Layer *layer, GContext *ctx) {
  GFont *font;
  time_t sec;
  struct tm * time;
  char date[] = "    ";
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_text_color(ctx, GColorWhite);
  for (int i = 0; i < 7; i++) {
    if (i < 5) font = calendar_font;
    else font = calendar_bd_font;
    graphics_draw_text(ctx, weekd[i],font, GRect(CALENDAR_X0 + i * CALENDAR_W + CALENDAR_DX, CALENDAR_DY, CALENDAR_W, CALENDAR_H), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  sec = begin_seconds;
  for (int j = 1; j < 4; j++) {
    for (int i = 0; i < 7; i++) {
      if (i < 5 && sec != seconds) font = calendar_font;
      else font = calendar_bd_font;
      if (sec == seconds) {
        //Today
        graphics_context_set_text_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, GRect(CALENDAR_X0 + i * CALENDAR_W, j * CALENDAR_H, CALENDAR_W, CALENDAR_H), 0, 0);
      } else {
        graphics_context_set_text_color(ctx, GColorWhite);
      }
      time = localtime(&sec);
      snprintf(date, sizeof(date), "%i", time->tm_mday);
      graphics_draw_text(ctx, date,font, GRect(CALENDAR_X0 + i * CALENDAR_W + CALENDAR_DX, j * CALENDAR_H + CALENDAR_DY, CALENDAR_W, CALENDAR_H), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
      sec += SECONDS_IN_DAY;
    }
  }
  int x0 = CALENDAR_X0, xn = CALENDAR_X0 + 7 * CALENDAR_W;
  int y0 = 0, yn = 4 * CALENDAR_H;
  int x, y;
  for(int j = 0; j <= 4; j++) {
    y = j * CALENDAR_H;
    graphics_draw_line(ctx, GPoint(x0, y), GPoint(xn, y));
  }
  for(int i = 0; i <= 7; i++) {
    x = CALENDAR_X0 + i * CALENDAR_W;
    graphics_draw_line(ctx, GPoint(x, y0), GPoint(x, yn));
  }
}

static void battery_rect_layer_update_proc(struct Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, 15 * battery_percent / 100, 7), 0, 0);
}

static void decorations_layer_update_proc(struct Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0,DIGITS_Y + 0, WIDTH, LINE_H), 0, 0);
  graphics_fill_rect(ctx, GRect(0,DIGITS_Y + 48, WIDTH, LINE_H), 0, 0);
}

static void seconds_tick_handler(struct tm * tick_time, TimeUnits units_change) {
    if (tick_time == NULL) {
       time_t tt = time(NULL);
       tick_time = localtime(&tt);
    }
    if (tick_time->tm_sec == 0 && tick_time->tm_min == 0) {
      //Hour line
      vibes_short_pulse();
    }
    strftime(time_text, sizeof(time_text), "%H:%M:%S", tick_time);
    layer_mark_dirty(time_layer);
    days = tick_time->tm_yday;
    if (days != days0) {
      snprintf(date_text, sizeof(date_text), "%i %s, %s", tick_time->tm_mday, months[tick_time->tm_mon], weekdays[tick_time->tm_wday]);
      text_layer_set_text(date_layer, date_text);

      // 12 pm for date calculations
      tick_time->tm_hour = 12;
      tick_time->tm_min = 0;
      tick_time->tm_sec = 0;
      
      calculate_calendar(tick_time);
      layer_mark_dirty(calendar_layer);
      days0 = days;
    }
}

static void battery_state_handler(BatteryChargeState bat) {
    battery_percent = bat.charge_percent;
    snprintf(bat_text, sizeof(bat_text), "%i%%", battery_percent);
    text_layer_set_text(battery_text_layer, bat_text);
    layer_mark_dirty(battery_rect_layer);
}

static void bt_connection_handler(bool connected) {
  if (connected) {
    bitmap_layer_set_bitmap(bt_layer, bt_on_bitmap);
  } else {
    vibes_enqueue_custom_pattern(lost_vibe_pat);
    bitmap_layer_set_bitmap(bt_layer, bt_off_bitmap);
  }
  
}

void handle_init(void) {
  root_window = window_create();
  window_set_fullscreen(root_window, true);
  window_set_background_color(root_window, GColorBlack);
  root_layer = window_get_root_layer(root_window);
  load_resources();
  
  time_layer = layer_create(GRect(0, DIGITS_Y + 7, WIDTH, 21));
  layer_set_update_proc(time_layer, time_layer_update_proc);

  date_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  date_layer = text_layer_create(GRect(0, DIGITS_Y + 26, WIDTH, 20));
  text_layer_set_font(date_layer, date_font);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_background_color(date_layer, GColorBlack);
  text_layer_set_text_color(date_layer, GColorWhite);

  calendar_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  calendar_bd_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  calendar_layer = layer_create(GRect(0, CALENDAR_Y, WIDTH, CALENDAR_H * 4 + 1));
  layer_set_update_proc(calendar_layer, calendar_layer_update_proc);

  decorations_layer = layer_create(GRect(0, 0, WIDTH, HEIGHT));
  layer_set_update_proc(decorations_layer, decorations_layer_update_proc);

  battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  battery_layer = bitmap_layer_create(GRect(87, 1 + BATTERY_DY, 19, 9));
  bitmap_layer_set_bitmap(battery_layer, battery_bitmap);
 
  battery_text_layer = text_layer_create(GRect(106, -7 + BATTERY_DY, 38, 18));
  text_layer_set_font(battery_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)); 
  text_layer_set_text_alignment(battery_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(battery_text_layer, GColorBlack);
  text_layer_set_text_color(battery_text_layer, GColorWhite);                    

  battery_rect_layer = layer_create(GRect(1, 1, 15, 7));
  layer_set_update_proc(battery_rect_layer, battery_rect_layer_update_proc);

  bt_on_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ON);
  bt_off_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_OFF);

  bt_layer = bitmap_layer_create(GRect(0, 0, 19, 19));
  
  layer_add_child(root_layer, text_layer_get_layer(date_layer));
  layer_add_child(root_layer, time_layer);
  layer_add_child(root_layer, calendar_layer);
  layer_add_child(bitmap_layer_get_layer(battery_layer), battery_rect_layer);
  layer_add_child(root_layer, bitmap_layer_get_layer(battery_layer));
  layer_add_child(root_layer, text_layer_get_layer(battery_text_layer));
  layer_add_child(root_layer, bitmap_layer_get_layer(bt_layer));
  layer_add_child(root_layer, decorations_layer);

  battery_state_handler(battery_state_service_peek());
  battery_state_service_subscribe(battery_state_handler);
 
  bt_connection_handler(bluetooth_connection_service_peek());
  bluetooth_connection_service_subscribe(bt_connection_handler);
  
  seconds_tick_handler(NULL, SECOND_UNIT);
  tick_timer_service_subscribe(SECOND_UNIT, seconds_tick_handler);

  window_stack_push(root_window, true);
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
 
  layer_destroy(time_layer);
  layer_destroy(text_layer_get_layer(date_layer));
  layer_destroy(calendar_layer);
  gbitmap_destroy(battery_bitmap);
  layer_destroy(battery_rect_layer);
  bitmap_layer_destroy(battery_layer);
  text_layer_destroy(battery_text_layer);
  gbitmap_destroy(bt_on_bitmap);
  gbitmap_destroy(bt_off_bitmap);
  bitmap_layer_destroy(bt_layer);
  layer_destroy(decorations_layer);
  unload_resources();
  window_destroy(root_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
