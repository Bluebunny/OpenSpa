#include "_openspa.h"
#include "console.h"
#include "io.h"
#include "i2c.h"
#include "onewire.h"
#include "io_expander.h"
#include "hot_tub.h"
#include "balboa_display.h"

hot_tub jacuzzi(epin_o_main,    epin_o_heater,          epin_o_circulation_pump,
                epin_o_pump_1,  epin_o_pump_1_speed,    epin_o_pump_2,
                epin_o_blower,  epin_o_ozone_generator, epin_o_light,
                epin_i_pressure_switch);

balboa_display bb_display(pin_o_bb_display_data_out, pin_i_bb_display_data_in, pin_o_bb_display_clock);

uint8_t openspa_error = 0;

void setup()
{
  consoleInit();
  ioInit();
  i2cInit();
  ioExpanderInit();
  jacuzziInit();
  if (openspa_wifi_enable)
    wifiInit();
  delay(10);
  consolePrintCommands();
  mainRelay(true);
}

void loop()
{
  openspaErrorHandler();

  if (openspa_error == false)
    openspa_error = jacuzzi.poll();
  else
    mainRelay(false);

  displayHandler();
  consoleHandler();

  if (openspa_wifi_enable)
    mqttHandler();
}



uint8_t jacuzziInit()
{
  jacuzzi.setMaxTemperature(openspa_max_temperature);
  jacuzzi.setMinTemperature(openspa_min_temperature);
  jacuzzi.setDesiredTemperature(openspa_init_desired_temp);

  jacuzzi.setMaxTotalPower(openspa_power_total_max);
  jacuzzi.setHeaterPower(openspa_power_heater);
  jacuzzi.setCircPumpPower(openspa_power_pump_circ);
  jacuzzi.setPump_1_Power(openspa_power_pump_1);
  jacuzzi.setPump_2_Power(openspa_power_pump_2);
  jacuzzi.setBlowerPower(openspa_power_blower);

  jacuzzi.setPump_1_Timing(openspa_runtime_pump_1, openspa_resttime_pump_1);
  jacuzzi.setPump_2_Timing(openspa_runtime_pump_2, openspa_resttime_pump_2);
  jacuzzi.setBlowerTiming(openspa_runtime_blower, openspa_resttime_blower);

  jacuzzi.setFiltering(openspa_filter_window_start_time, openspa_filter_window_stop_time, openspa_ozone_window_start_time,
                       openspa_ozone_window_stop_time, openspa_filter_daily_cycles, openspa_filter_time);
  jacuzzi.setHeating(openspa_heating_timeout, openspa_heating_timeout_delta_degrees);
  jacuzzi.setFlushing(openspa_flush_window_start_time, openspa_flush_window_stop_time, openspa_flush_daily_cycles,
                      openspa_flush_time, openspa_flush_time, openspa_flush_time);

  return 0;
}

uint8_t openspaErrorHandler()
{
  static uint8_t io_expander_reset_state_prv = true;

  if ((io_expander_reset_state_prv == false) and (ioReadIOExpanderReset() == true)) //Reinit when reset is released (error reset button on pcb)
  {
    delay(10);
    io_expander_reset_state_prv = false;
    ioExpanderInit();
    digitalWrite(pin_o_io_expander_reset, HIGH);
    jacuzzi.reset();
    openspa_error = 0;
    Serial.println("Reset released");
  }

  io_expander_reset_state_prv = ioReadIOExpanderReset();

  if (io_expander_reset_state_prv == false)
  {
    openspa_error = 255; //External error (Heater overtemp)
  }

  return 0;
}

void mainRelay(uint8_t state)
{
  static uint8_t current_state = 0;

  if (state != current_state)
  {
    current_state = state;
    ioExpanderWrite(epin_o_main, state);
  }
}