    #include <zephyr/kernel.h>
    #include <zephyr/logging/log.h>
    #include <dk_buttons_and_leds.h>
    #include <zephyr/drivers/pwm.h>
    #include "remote.h"
    #include <stdlib.h>
    #include "motor_control.h"
    #include "mpu_sensor.h"

    #define LOG_MODULE_NAME app
    #define RUN_STATUS_LED DK_LED1
    #define RUN_LED_BLINK_INTERVAL 1000 // setter led1 til å blinke med 1 sekund av/på
    LOG_MODULE_REGISTER(LOG_MODULE_NAME);
    volatile int motor_angle    = 1500000;
    #define MEAN_DUTY_CYCLE       1400000
    #define MAX_DUTY_CYCLE        2400000
    #define MIN_DUTY_CYCLE         400000
    #define CONN_STATUS_LED DK_LED2
    #define CONN_STATUS_LED1 DK_LED3
    #define CONN_STATUS_LED2 DK_LED4
  
    #define PWM_PERIOD_NS 20000000
    #define STEGLENGDE 55556
    static struct bt_conn *current_conn;
    void on_connected(struct bt_conn *conn, uint8_t err);
    void on_disconnected(struct bt_conn *conn, uint8_t reason);
    void on_notif_changed(enum bt_button_notifications_enabled status);
    void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len);

    void helloworld(void){
            LOG_INF("Hello World! %s\n", CONFIG_BOARD);
    }

    void setleds(void){
        int LYS = 1;
        int button_pressed=0;
        uint32_t flerelys = 0;
        dk_leds_init(); //kjører leds
        dk_set_led(button_pressed-LYS, LYS);

        for (flerelys=1; flerelys<10; flerelys++)
                                {
                        dk_set_leds(flerelys); 
                        k_sleep(K_MSEC(1000));
        if (flerelys > 10)
            {
                flerelys=0; 
            }
                                } 

    }

    void button_handler(uint32_t button_state, uint32_t has_changed)
    {
        int err = 0;
        int button_pressed = 0;
        if (has_changed & button_state)
        {
            switch (has_changed)
            {
                case DK_BTN1_MSK:
                    button_pressed = 1;
                    err = set_motor_angle(1000000);
                    break;
                case DK_BTN2_MSK:
                    button_pressed = 2;
                    err = set_motor_angle(1333000);
                    break;
                case DK_BTN3_MSK:
                    button_pressed = 3;
                    motor_angle -= 100000;
                    err = set_motor_angle(motor_angle);
                    LOG_INF("motor angle = %d", motor_angle);
                    break;
                case DK_BTN4_MSK:
                    button_pressed = 4;
                    motor_angle += 100000;
                    err = set_motor_angle(motor_angle);
                    LOG_INF("motor angle = %d", motor_angle);
                    break;
                default:
                    break;
            }
            LOG_INF("Button %d pressed.", button_pressed);
            if (err) {
                LOG_ERR("couldn't set duty cycle. Err %d", err);
            }
            set_button_press(button_pressed); 
            err = send_button_notification(current_conn, button_pressed);
            if (err) {
                LOG_ERR("Couldn't send notification. Err %d", err);
            }
        }
    }

    static void configure_dk_buttons_and_leds(void)
    {
        int err;
        err = dk_leds_init();
        if (err) {
            LOG_ERR("Couldn't init LEDs (err %d)", err);
        }
        err = dk_buttons_init(button_handler);
        if (err) {
            LOG_ERR("Couldn't init buttons (err %d)", err);
        }
    }

    void knapperoglys(void)
    {	
    
        LOG_INF("Hello World! %s\n", CONFIG_BOARD);
        configure_dk_buttons_and_leds();
        LOG_INF("Running...");
    
    }

    static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

    int servoinit(void){
        if (!device_is_ready(pwm_led0.dev)) {
            LOG_ERR("Error: PWM device %s is not ready", pwm_led0.dev->name);
            return -EBUSY;
        }

        // Bevegerrr servomotoren fra -90 grader til +90 grader kontinuerlig og raskt
        int pwm_value = (PWM_PERIOD_NS / 20) * ((-90 / 180.0) + 1.5) + STEGLENGDE;
        pwm_set_dt(&pwm_led0, PWM_PERIOD_NS, pwm_value);
        k_sleep(K_MSEC(200));

        while (1) {
            for (int i = -230; i <= 150; i += 1) {
                pwm_value = (PWM_PERIOD_NS / 20) * ((i / 180.0) + 1.5) + STEGLENGDE;
                pwm_set_dt(&pwm_led0, PWM_PERIOD_NS, pwm_value);
                k_sleep(K_MSEC(2));
            }
            for (int i = 150; i >= -230; i -= 1) {
                pwm_value = (PWM_PERIOD_NS / 20) * ((i / 180.0) + 1.5) + STEGLENGDE;
                pwm_set_dt(&pwm_led0, PWM_PERIOD_NS, pwm_value);
                k_sleep(K_MSEC(2));
            }
        }
    }

    struct bt_conn_cb bluetooth_callbacks = {
        .connected      = on_connected,
        .disconnected   = on_disconnected,
    };

    struct bt_remote_service_cb remote_callbacks = {
        .notif_changed = on_notif_changed,
        .data_received = on_data_received,
    };


    void on_connected(struct bt_conn *conn, uint8_t err)
    {
        if (err) {
            LOG_ERR("connection failed, err %d", err);
        }
        LOG_INF("Connected to central");
        current_conn = bt_conn_ref(conn);
        dk_set_led_on(CONN_STATUS_LED);
        dk_set_led_on(CONN_STATUS_LED1);
        dk_set_led_on(CONN_STATUS_LED2);
    }

    void on_disconnected(struct bt_conn *conn, uint8_t reason)
    {
        LOG_INF("Disconnected (reason: %d)", reason);
        dk_set_led_off(CONN_STATUS_LED);
        dk_set_led_off(CONN_STATUS_LED1);
        dk_set_led_off(CONN_STATUS_LED2);
        if(current_conn) {
            bt_conn_unref(current_conn);
            current_conn = NULL;
        }
    }

    static uint32_t degree_to_duty_cycle(uint32_t value){
        return 500000+((value)*(2500000-500000)/180);
    }

    static uint32_t current_position = 180;

    void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
    {
        uint8_t temp_str[len+1];
        memcpy(temp_str, data, len);
        temp_str[len] = 0x00;

        LOG_INF("Received data on conn %p. Len: %d", (void *)conn, len);
        LOG_INF("Data: %s", temp_str);

        
        //Dette er en sjekk om stringen inneholder koden "7Jf3", "52ok", "ok", "OK", "oK", "Ok", "o", eller "O"
         if (strstr(temp_str, "7Jf3") || strstr(temp_str, "52ok") || strstr(temp_str, "ok")|| strstr(temp_str, "OK")
         || strstr(temp_str, "oK")|| strstr(temp_str, "Ok")|| strstr(temp_str, "o")|| strstr(temp_str, "O")) {
        run_servo_sequence();
        }
    }

    void run_servo_sequence()
    {
        set_motor_angle(degree_to_duty_cycle(180));
        current_position = 0; // Endre til 0 for å starte på 180 grader
        k_sleep(K_MSEC(1000)); // Vent i 1 sekund på at servoen skal nå 180 grader

        set_motor_angle(degree_to_duty_cycle(0));
        current_position = 180; // Endre til 180 for å nå 0 grader
        k_sleep(K_MSEC(1000)); // Vent i 1 sekund på at servoen skal nå 0 grader

        set_motor_angle(degree_to_duty_cycle(180));
        current_position = 0; // Endre til 0 for å nå 180 grader igjen
    }



    void on_notif_changed(enum bt_button_notifications_enabled status)
    {
        if (status == BT_BUTTON_NOTIFICATIONS_ENABLED) {
            LOG_INF("Notifications enabled");
        } else {
            LOG_INF("Notifications disabled");
        }
    }

    void blaatann(void){
        int err;
        int blink_status = 0;
        LOG_INF("Hello World! %s", CONFIG_BOARD);

        configure_dk_buttons_and_leds();
        err = motor_init();
        if (err) {
            LOG_ERR("motor_init() failed. (err %d)", err);
        }

        err = bluetooth_init(&bluetooth_callbacks, &remote_callbacks );
        if (err) {
            LOG_ERR("Bluetooth_init() failed. (err %d)", err);
        }

        LOG_INF("Running");

        for (;;) {
            dk_set_led(RUN_STATUS_LED, (blink_status++)%2);
            k_sleep(K_MSEC(1000));
        }
    }

    void blaatann2(void)
    {
        int err;
        int blink_status = 0;
        LOG_INF("Hello World! %s", CONFIG_BOARD);

        configure_dk_buttons_and_leds();
        err = motor_init();
        if (err) {
            LOG_ERR("motor_init() failed. (err %d)", err);
        }

        err = bluetooth_init(&bluetooth_callbacks, &remote_callbacks);
        if (err) {
            LOG_ERR("Bluetooth_init() failed. (err %d)",  err);
        }

        LOG_INF("Running");

        for (;;) {
            dk_set_led(RUN_STATUS_LED, (blink_status++)%2);
            k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
        }
    }


    void control_motor(accel_values_t * accel_values, accel_values_t * min_values, accel_values_t * max_values)
    {
        //update min/max:
        int16_t offset_x;
        int16_t calibrated_x;
        uint32_t duty_cycle;
        double gain = 50;
        if (accel_values->x < min_values->x) {
            min_values->x = accel_values->x;
        }
        else if (accel_values->x > max_values->x) {
            max_values->x = accel_values->x;
        }

        offset_x = (max_values->x + min_values->x)/2;
        calibrated_x = accel_values->x - offset_x;
        //translate x to motor angle:
        duty_cycle = MEAN_DUTY_CYCLE; // middle position
        duty_cycle += (calibrated_x*gain);

        LOG_INF("calibrated x: %06d, duty_cycle: %08d", calibrated_x, duty_cycle);

        set_motor_angle(duty_cycle);
    }

    void gyrotest(void)
    {
        int err;
        int blink_status = 0;
        LOG_INF("Hello World! %s", CONFIG_BOARD);
        accel_values_t accel_values;
        accel_values_t min_values = {0};
        accel_values_t max_values = {0};
        configure_dk_buttons_and_leds();
        err = mpu_sensor_init();
        if (err) {
            LOG_ERR("mpu_init() failed. (err %08x)", err);
        }

        err = motor_init();
        if (err) {
            LOG_ERR("motor_init() failed. (err %d)", err);
        }

        err = bluetooth_init(&bluetooth_callbacks, &remote_callbacks);
        if (err) {
            LOG_ERR("Bluetooth_init() failed. (err %d)", err);
        }

        LOG_INF("Running");

        for (;;) {
            dk_set_led(RUN_STATUS_LED, (blink_status++)%2);
            if (read_accel_values(&accel_values) == 0) {
                LOG_INF("# %d, Accel: X: %06d, Y: %06d, Z: %06d", blink_status, accel_values.x, accel_values.y, accel_values.z);
                control_motor(&accel_values, &min_values, &max_values);
            }
            k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
        }
    }

    int main (void)
    {
        const char* str = "1"; 
        int y = atoi(str);
        LOG_INF("value; %i\n ", y);
        //gyrotest();
        blaatann2();
        for(;;){
            k_sleep(K_MSEC(1000));
        }
    }
    




