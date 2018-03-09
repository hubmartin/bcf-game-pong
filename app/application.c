#include <application.h>

// LED instance
bc_led_t led;

// Button instance
bc_button_t button;

bc_button_t lcd_left;
bc_button_t lcd_right;

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_set_mode(&led, BC_LED_MODE_TOGGLE);
    }
}

void lcd_button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) event_param;

	if (event == BC_BUTTON_EVENT_CLICK)
	{
        if (self->_channel.virtual_channel == BC_MODULE_LCD_BUTTON_LEFT)
        {
           bc_led_pulse(&led, 200);
        }
        else if (self->_channel.virtual_channel == BC_MODULE_LCD_BUTTON_RIGHT)
        {
            bc_led_pulse(&led, 400);
        }

	}
}

void application_init(void)
{
    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize LCD
    // The parameter is internal buffer in SDK, no need to define it
    bc_module_lcd_init(&_bc_module_lcd_framebuffer);

    // Initialize LCD button left
    bc_button_init_virtual(&lcd_left, BC_MODULE_LCD_BUTTON_LEFT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_left, lcd_button_event_handler, NULL);

    // Initialize LCD button right
    bc_button_init_virtual(&lcd_right, BC_MODULE_LCD_BUTTON_RIGHT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_right, lcd_button_event_handler, NULL);

    bc_system_pll_enable();
}



int32_t paddle_x = 50;
int32_t paddle_y = 120;
float paddle_width = 60;
int32_t paddle_height = 7;

float ball_x = 64;
int32_t ball_y = 64;
float ball_vect_x = 1.3f;
float ball_vect_y = 3;
int32_t ball_diameter = 4;

uint32_t game_state = 1;
uint32_t score = 0;

#define GAME_STATE_RUN 1
#define GAME_STATE_OVER 2

void game_reset()
{
    paddle_x = 50;
    paddle_width = 60;

    ball_x = 32 + rand() % 64;
    ball_y = 10 + rand() % 10;
    ball_vect_x = -2.5f + (rand() % 50) * 0.1f;
    ball_vect_y = 3;

    score = 0;
}

uint8_t game_run()
{

  char buf[16];

    const bc_button_driver_t *btn_driver = bc_module_lcd_get_button_driver();

    uint32_t btn_left = btn_driver->get_input(&lcd_left);
    uint32_t btn_right = btn_driver->get_input(&lcd_right);

    score += 1;

    if (btn_left && paddle_x > 0)
    {
        paddle_x -= 5;
    }
    if (btn_right && (paddle_x + paddle_width) < 127)
    {
        paddle_x += 5;
    }

    if (game_state == GAME_STATE_RUN)
    {
        ball_x += ball_vect_x;
        ball_y += ball_vect_y;
    }

    // ball-paddle
    if (ball_y + ball_diameter > paddle_y)
    {
        if (ball_x >= paddle_x && ball_x <= paddle_x + paddle_width)
        {
            // Make paddle smaller
            if(paddle_width > 10)
            {
                paddle_width -= 0.5f;
            }
            // Speed up the ball
            ball_vect_y += 0.5f;
            ball_vect_y = -ball_vect_y;
            ball_y = -ball_diameter + paddle_y;
            score += 50;

            //faleš
            if (btn_left)
            {
                ball_vect_x -= 1.0f + (rand() % 20) * 0.1f;
            }
            if (btn_right)
            {
                ball_vect_x += 1.0f + (rand() % 20) * 0.1f;
            }
        }
    }

    if (ball_y + ball_diameter > paddle_y + paddle_height)
    {
        return GAME_STATE_OVER;
    }

    if (ball_x > 127 || ball_x < 0)
    {
        ball_vect_x = -ball_vect_x;
    }

    // TOP edge
    if (ball_y < 0)
    {
        ball_vect_y = -ball_vect_y;
        ball_y = 0;
    }

    bc_module_lcd_clear();

    bc_module_lcd_set_font(&bc_font_ubuntu_15);

    snprintf(buf, sizeof(buf), "Score: %d", (signed)score);
    bc_module_lcd_draw_string(20, 5, buf, 1);

    // BALL
    bc_module_lcd_draw_circle(ball_x, ball_y, ball_diameter, true);
    // PADDLE
    bc_module_lcd_draw_rectangle(paddle_x, paddle_y, paddle_x + paddle_width, paddle_y + paddle_height, true);

    bc_module_lcd_update();

    return GAME_STATE_RUN;

}


uint8_t game_over()
{
    static uint8_t btn_flag = 1;
    char buf[16];
    static bc_tick_t timestamp;

    const bc_button_driver_t *btn_driver = bc_module_lcd_get_button_driver();

    uint32_t btn_left = btn_driver->get_input(&lcd_left);
    uint32_t btn_right = btn_driver->get_input(&lcd_right);

    if (!btn_flag && (btn_left || btn_right) && (bc_tick_get() - timestamp > 500))
    {
        btn_flag = 1;
        game_reset();
        return GAME_STATE_RUN;
    }

    if(btn_flag && !btn_left &&!btn_right)
    {
        btn_flag = 0;
        timestamp = bc_tick_get();
    }

    bc_module_lcd_clear();

    bc_module_lcd_set_font(&bc_font_ubuntu_15);

    bc_module_lcd_draw_string(25, 65, "GAME OVER", 1);
    snprintf(buf, sizeof(buf), "Score: %d", (signed)score);
    bc_module_lcd_draw_string(25, 80, buf, 1);

    bc_module_lcd_update();

    return GAME_STATE_OVER;
}

void application_task(void)
{
    switch(game_state)
    {
        case GAME_STATE_RUN:
            game_state = game_run();
        break;

        case GAME_STATE_OVER:
            game_state = game_over();
        break;

        default:
            break;

    }

    bc_scheduler_plan_current_relative(50);
}
